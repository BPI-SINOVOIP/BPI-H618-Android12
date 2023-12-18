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

//! This executable works as a child/worker for the main compsvc service. This worker is mainly
//! responsible for setting up the execution environment, e.g. to create file descriptors for
//! remote file access via an authfs mount.

use anyhow::{bail, Result};
use log::warn;
use minijail::Minijail;
use nix::sys::statfs::{statfs, FsType};
use std::fs::{File, OpenOptions};
use std::io;
use std::os::unix::io::AsRawFd;
use std::path::Path;
use std::process::exit;
use std::thread::sleep;
use std::time::{Duration, Instant};

const AUTHFS_BIN: &str = "/apex/com.android.virt/bin/authfs";
const AUTHFS_SETUP_POLL_INTERVAL_MS: Duration = Duration::from_millis(50);
const AUTHFS_SETUP_TIMEOUT_SEC: Duration = Duration::from_secs(10);
const FUSE_SUPER_MAGIC: FsType = FsType(0x65735546);

/// The number that hints the future file descriptor. These are not really file descriptor, but
/// represents the file descriptor number to pass to the task.
type PseudoRawFd = i32;

fn is_fuse(path: &str) -> Result<bool> {
    Ok(statfs(path)?.filesystem_type() == FUSE_SUPER_MAGIC)
}

fn spawn_authfs(config: &Config) -> Result<Minijail> {
    // TODO(b/185175567): Run in a more restricted sandbox.
    let jail = Minijail::new()?;

    let mut args = vec![AUTHFS_BIN.to_string(), config.authfs_root.clone()];
    for conf in &config.in_fds {
        // TODO(b/185178698): Many input files need to be signed and verified.
        // or can we use debug cert for now, which is better than nothing?
        args.push("--remote-ro-file-unverified".to_string());
        args.push(format!("{}:{}:{}", conf.fd, conf.fd, conf.file_size));
    }
    for conf in &config.out_fds {
        args.push("--remote-new-rw-file".to_string());
        args.push(format!("{}:{}", conf.fd, conf.fd));
    }

    let preserve_fds = if config.debuggable {
        vec![1, 2] // inherit/redirect stdout/stderr for debugging
    } else {
        vec![]
    };

    let _pid = jail.run(Path::new(AUTHFS_BIN), &preserve_fds, &args)?;
    Ok(jail)
}

fn wait_until_authfs_ready(authfs_root: &str) -> Result<()> {
    let start_time = Instant::now();
    loop {
        if is_fuse(authfs_root)? {
            break;
        }
        if start_time.elapsed() > AUTHFS_SETUP_TIMEOUT_SEC {
            bail!("Time out mounting authfs");
        }
        sleep(AUTHFS_SETUP_POLL_INTERVAL_MS);
    }
    Ok(())
}

fn open_authfs_file(authfs_root: &str, basename: PseudoRawFd, writable: bool) -> io::Result<File> {
    OpenOptions::new().read(true).write(writable).open(format!("{}/{}", authfs_root, basename))
}

fn open_authfs_files_for_mapping(config: &Config) -> io::Result<Vec<(File, PseudoRawFd)>> {
    let mut fd_mapping = Vec::with_capacity(config.in_fds.len() + config.out_fds.len());

    let results: io::Result<Vec<_>> = config
        .in_fds
        .iter()
        .map(|conf| Ok((open_authfs_file(&config.authfs_root, conf.fd, false)?, conf.fd)))
        .collect();
    fd_mapping.append(&mut results?);

    let results: io::Result<Vec<_>> = config
        .out_fds
        .iter()
        .map(|conf| Ok((open_authfs_file(&config.authfs_root, conf.fd, true)?, conf.fd)))
        .collect();
    fd_mapping.append(&mut results?);

    Ok(fd_mapping)
}

fn spawn_jailed_task(config: &Config, fd_mapping: Vec<(File, PseudoRawFd)>) -> Result<Minijail> {
    // TODO(b/185175567): Run in a more restricted sandbox.
    let jail = Minijail::new()?;
    let mut preserve_fds: Vec<_> = fd_mapping.iter().map(|(f, id)| (f.as_raw_fd(), *id)).collect();
    if config.debuggable {
        // inherit/redirect stdout/stderr for debugging
        preserve_fds.push((1, 1));
        preserve_fds.push((2, 2));
    }
    let _pid =
        jail.run_remap(&Path::new(&config.args[0]), preserve_fds.as_slice(), &config.args)?;
    Ok(jail)
}

struct InFdAnnotation {
    fd: PseudoRawFd,
    file_size: u64,
}

struct OutFdAnnotation {
    fd: PseudoRawFd,
}

struct Config {
    authfs_root: String,
    in_fds: Vec<InFdAnnotation>,
    out_fds: Vec<OutFdAnnotation>,
    args: Vec<String>,
    debuggable: bool,
}

fn parse_args() -> Result<Config> {
    #[rustfmt::skip]
    let matches = clap::App::new("compsvc_worker")
        .arg(clap::Arg::with_name("authfs-root")
             .long("authfs-root")
             .value_name("DIR")
             .required(true)
             .takes_value(true))
        .arg(clap::Arg::with_name("in-fd")
             .long("in-fd")
             .multiple(true)
             .takes_value(true)
             .requires("authfs-root"))
        .arg(clap::Arg::with_name("out-fd")
             .long("out-fd")
             .multiple(true)
             .takes_value(true)
             .requires("authfs-root"))
        .arg(clap::Arg::with_name("debug")
             .long("debug"))
        .arg(clap::Arg::with_name("args")
             .last(true)
             .required(true)
             .multiple(true))
        .get_matches();

    // Safe to unwrap since the arg is required by the clap rule
    let authfs_root = matches.value_of("authfs-root").unwrap().to_string();

    let results: Result<Vec<_>> = matches
        .values_of("in-fd")
        .unwrap_or_default()
        .into_iter()
        .map(|arg| {
            if let Some(index) = arg.find(':') {
                let (fd, size) = arg.split_at(index);
                Ok(InFdAnnotation { fd: fd.parse()?, file_size: size[1..].parse()? })
            } else {
                bail!("Invalid argument: {}", arg);
            }
        })
        .collect();
    let in_fds = results?;

    let results: Result<Vec<_>> = matches
        .values_of("out-fd")
        .unwrap_or_default()
        .into_iter()
        .map(|arg| Ok(OutFdAnnotation { fd: arg.parse()? }))
        .collect();
    let out_fds = results?;

    let args: Vec<_> = matches.values_of("args").unwrap().map(|s| s.to_string()).collect();
    let debuggable = matches.is_present("debug");

    Ok(Config { authfs_root, in_fds, out_fds, args, debuggable })
}

fn main() -> Result<()> {
    let log_level =
        if env!("TARGET_BUILD_VARIANT") == "eng" { log::Level::Trace } else { log::Level::Info };
    android_logger::init_once(
        android_logger::Config::default().with_tag("compsvc_worker").with_min_level(log_level),
    );

    let config = parse_args()?;

    let authfs_jail = spawn_authfs(&config)?;
    let authfs_lifetime = scopeguard::guard(authfs_jail, |authfs_jail| {
        if let Err(e) = authfs_jail.kill() {
            if !matches!(e, minijail::Error::Killed(_)) {
                warn!("Failed to kill authfs: {}", e);
            }
        }
    });

    wait_until_authfs_ready(&config.authfs_root)?;
    let fd_mapping = open_authfs_files_for_mapping(&config)?;

    let jail = spawn_jailed_task(&config, fd_mapping)?;
    let jail_result = jail.wait();

    // Be explicit about the lifetime, which should last at least until the task is finished.
    drop(authfs_lifetime);

    match jail_result {
        Ok(_) => Ok(()),
        Err(minijail::Error::ReturnCode(exit_code)) => {
            exit(exit_code as i32);
        }
        Err(e) => {
            bail!("Unexpected minijail error: {}", e);
        }
    }
}
