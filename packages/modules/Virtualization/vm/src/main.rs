// Copyright 2021, The Android Open Source Project
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

//! Android VM control tool.

mod run;
mod sync;

use android_system_virtmanager::aidl::android::system::virtmanager::IVirtManager::IVirtManager;
use android_system_virtmanager::binder::{get_interface, ProcessState, Strong};
use anyhow::{Context, Error};
use run::command_run;
use std::path::PathBuf;
use structopt::clap::AppSettings;
use structopt::StructOpt;

const VIRT_MANAGER_BINDER_SERVICE_IDENTIFIER: &str = "android.system.virtmanager";

#[derive(StructOpt)]
#[structopt(no_version, global_settings = &[AppSettings::DisableVersion])]
enum Opt {
    /// Run a virtual machine
    Run {
        /// Path to VM config JSON
        #[structopt(parse(from_os_str))]
        config: PathBuf,

        /// Detach VM from the terminal and run in the background
        #[structopt(short, long)]
        daemonize: bool,
    },
    /// Stop a virtual machine running in the background
    Stop {
        /// CID of the virtual machine
        cid: u32,
    },
    /// List running virtual machines
    List,
}

fn main() -> Result<(), Error> {
    env_logger::init();
    let opt = Opt::from_args();

    // We need to start the thread pool for Binder to work properly, especially link_to_death.
    ProcessState::start_thread_pool();

    let virt_manager = get_interface(VIRT_MANAGER_BINDER_SERVICE_IDENTIFIER)
        .context("Failed to find Virt Manager service")?;

    match opt {
        Opt::Run { config, daemonize } => command_run(virt_manager, &config, daemonize),
        Opt::Stop { cid } => command_stop(virt_manager, cid),
        Opt::List => command_list(virt_manager),
    }
}

/// Retrieve reference to a previously daemonized VM and stop it.
fn command_stop(virt_manager: Strong<dyn IVirtManager>, cid: u32) -> Result<(), Error> {
    virt_manager
        .debugDropVmRef(cid as i32)
        .context("Failed to get VM from Virt Manager")?
        .context("CID does not correspond to a running background VM")?;
    Ok(())
}

/// List the VMs currently running.
fn command_list(virt_manager: Strong<dyn IVirtManager>) -> Result<(), Error> {
    let vms = virt_manager.debugListVms().context("Failed to get list of VMs")?;
    println!("Running VMs: {:#?}", vms);
    Ok(())
}
