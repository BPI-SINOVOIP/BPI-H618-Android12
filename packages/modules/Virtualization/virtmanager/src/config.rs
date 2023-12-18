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

//! Function and types for VM configuration.

use anyhow::{bail, Error};
use serde::{Deserialize, Serialize};
use std::fs::File;
use std::io::BufReader;

/// Configuration for a particular VM to be started.
#[derive(Clone, Debug, Deserialize, Eq, PartialEq, Serialize)]
pub struct VmConfig {
    /// The filename of the kernel image, if any.
    pub kernel: Option<String>,
    /// The filename of the initial ramdisk for the kernel, if any.
    pub initrd: Option<String>,
    /// Parameters to pass to the kernel. As far as the VMM and boot protocol are concerned this is
    /// just a string, but typically it will contain multiple parameters separated by spaces.
    pub params: Option<String>,
    /// The bootloader to use. If this is supplied then the kernel and initrd must not be supplied;
    /// the bootloader is instead responsibly for loading the kernel from one of the disks.
    pub bootloader: Option<String>,
    /// Disk images to be made available to the VM.
    #[serde(default)]
    pub disks: Vec<DiskImage>,
}

impl VmConfig {
    /// Ensure that the configuration has a valid combination of fields set, or return an error if
    /// not.
    pub fn validate(&self) -> Result<(), Error> {
        if self.bootloader.is_none() && self.kernel.is_none() {
            bail!("VM must have either a bootloader or a kernel image.");
        }
        if self.bootloader.is_some() && (self.kernel.is_some() || self.initrd.is_some()) {
            bail!("Can't have both bootloader and kernel/initrd image.");
        }
        Ok(())
    }

    /// Load the configuration for a VM from the given JSON file.
    pub fn load(file: &File) -> Result<VmConfig, Error> {
        let buffered = BufReader::new(file);
        Ok(serde_json::from_reader(buffered)?)
    }
}

/// A disk image to be made available to the VM.
#[derive(Clone, Debug, Deserialize, Eq, PartialEq, Serialize)]
pub struct DiskImage {
    /// The filename of the disk image.
    pub image: String,
    /// Whether this disk should be writable by the VM.
    pub writable: bool,
}
