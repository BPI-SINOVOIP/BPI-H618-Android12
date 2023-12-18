/*
 * Copyright (C) 2021 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

//! compsvc is a service to run computational tasks in a PVM upon request. It is able to set up
//! file descriptors backed by fd_server and pass the file descriptors to the actual tasks for
//! read/write. The service also attempts to sandbox the execution so that one task cannot leak or
//! impact future tasks.
//!
//! Example:
//! $ compsvc /system/bin/sleep
//!
//! The current architecture / process hierarchy looks like:
//! - compsvc (handle requests)
//!   - compsvc_worker (for environment setup)
//!     - authfs (fd translation)
//!     - actual task

use anyhow::{bail, Context, Result};
use log::error;
use minijail::{self, Minijail};
use std::path::PathBuf;

use compos_aidl_interface::aidl::com::android::compos::ICompService::{
    BnCompService, ICompService,
};
use compos_aidl_interface::aidl::com::android::compos::Metadata::Metadata;
use compos_aidl_interface::binder::{
    add_service, BinderFeatures, Interface, ProcessState, Result as BinderResult, Status,
    StatusCode, Strong,
};

const SERVICE_NAME: &str = "compsvc";
// TODO(b/161470604): Move the executable into an apex.
const WORKER_BIN: &str = "/system/bin/compsvc_worker";
// TODO: Replace with a valid directory setup in the VM.
const AUTHFS_MOUNTPOINT: &str = "/data/local/tmp/authfs_mnt";

struct CompService {
    worker_bin: PathBuf,
    task_bin: String,
    debuggable: bool,
}

impl CompService {
    pub fn new_binder(service: CompService) -> Strong<dyn ICompService> {
        BnCompService::new_binder(service, BinderFeatures::default())
    }

    fn run_worker_in_jail_and_wait(&self, args: &[String]) -> Result<(), minijail::Error> {
        let mut jail = Minijail::new()?;

        // TODO(b/185175567): New user and uid namespace when supported. Run as nobody.
        // New mount namespace to isolate the FUSE mount.
        jail.namespace_vfs();

        let inheritable_fds = if self.debuggable {
            vec![1, 2] // inherit/redirect stdout/stderr for debugging
        } else {
            vec![]
        };
        let _pid = jail.run(&self.worker_bin, &inheritable_fds, &args)?;
        jail.wait()
    }

    fn build_worker_args(&self, args: &[String], metadata: &Metadata) -> Vec<String> {
        let mut worker_args = vec![
            WORKER_BIN.to_string(),
            "--authfs-root".to_string(),
            AUTHFS_MOUNTPOINT.to_string(),
        ];
        for annotation in &metadata.input_fd_annotations {
            worker_args.push("--in-fd".to_string());
            worker_args.push(format!("{}:{}", annotation.fd, annotation.file_size));
        }
        for annotation in &metadata.output_fd_annotations {
            worker_args.push("--out-fd".to_string());
            worker_args.push(annotation.fd.to_string());
        }
        if self.debuggable {
            worker_args.push("--debug".to_string());
        }
        worker_args.push("--".to_string());

        // Do not accept arbitrary code execution. We want to execute some specific task of this
        // service. Use the associated executable.
        worker_args.push(self.task_bin.clone());
        worker_args.extend_from_slice(&args[1..]);
        worker_args
    }
}

impl Interface for CompService {}

impl ICompService for CompService {
    fn execute(&self, args: &[String], metadata: &Metadata) -> BinderResult<i8> {
        let worker_args = self.build_worker_args(args, metadata);

        match self.run_worker_in_jail_and_wait(&worker_args) {
            Ok(_) => Ok(0), // TODO(b/161471326): Sign the output on succeed.
            Err(minijail::Error::ReturnCode(exit_code)) => {
                error!("Task failed with exit code {}", exit_code);
                Err(Status::from(StatusCode::FAILED_TRANSACTION))
            }
            Err(e) => {
                error!("Unexpected error: {}", e);
                Err(Status::from(StatusCode::UNKNOWN_ERROR))
            }
        }
    }
}

fn parse_args() -> Result<CompService> {
    #[rustfmt::skip]
    let matches = clap::App::new("compsvc")
        .arg(clap::Arg::with_name("debug")
             .long("debug"))
        .arg(clap::Arg::with_name("task_bin")
             .required(true))
        .get_matches();

    Ok(CompService {
        task_bin: matches.value_of("task_bin").unwrap().to_string(),
        worker_bin: PathBuf::from(WORKER_BIN),
        debuggable: matches.is_present("debug"),
    })
}

fn main() -> Result<()> {
    android_logger::init_once(
        android_logger::Config::default().with_tag("compsvc").with_min_level(log::Level::Debug),
    );

    let service = parse_args()?;

    ProcessState::start_thread_pool();
    // TODO: switch to remote binder
    add_service(SERVICE_NAME, CompService::new_binder(service).as_binder())
        .with_context(|| format!("Failed to register service {}", SERVICE_NAME))?;
    ProcessState::join_thread_pool();
    bail!("Unexpected exit after join_thread_pool")
}
