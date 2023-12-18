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

//! Android Virt Manager

mod aidl;
mod config;
mod crosvm;

use crate::aidl::{VirtManager, BINDER_SERVICE_IDENTIFIER};
use android_system_virtmanager::aidl::android::system::virtmanager::IVirtManager::BnVirtManager;
use android_system_virtmanager::binder::{add_service, BinderFeatures, ProcessState};
use log::{info, Level};

/// The first CID to assign to a guest VM managed by the Virt Manager. CIDs lower than this are
/// reserved for the host or other usage.
const FIRST_GUEST_CID: Cid = 10;

const LOG_TAG: &str = "VirtManager";

/// The unique ID of a VM used (together with a port number) for vsock communication.
type Cid = u32;

fn main() {
    android_logger::init_once(
        android_logger::Config::default().with_tag(LOG_TAG).with_min_level(Level::Trace),
    );

    let virt_manager = VirtManager::default();
    let virt_manager = BnVirtManager::new_binder(
        virt_manager,
        BinderFeatures { set_requesting_sid: true, ..BinderFeatures::default() },
    );
    add_service(BINDER_SERVICE_IDENTIFIER, virt_manager.as_binder()).unwrap();
    info!("Registered Binder service, joining threadpool.");
    ProcessState::join_thread_pool();
}
