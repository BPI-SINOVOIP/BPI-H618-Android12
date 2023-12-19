// Copyright 2020 The Chromium OS Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
use std::string::String;

use dsm::{self, Error, Result};
use serde::Deserialize;
/// `DeviceSettings` includes the settings of max98373. It currently includes:
/// * the settings of amplifier calibration.
/// * the path of dsm_param.
#[derive(Debug, Default, PartialEq, Deserialize, Clone)]
pub struct DeviceSettings {
    pub amp_calibrations: AmpCalibSettings,
}

/// `AmpCalibSettings` includes the settings needed for amplifier calibration.
#[derive(Debug, Default, PartialEq, Deserialize, Clone)]
pub struct AmpCalibSettings {
    pub dsm_param_read_ctrl: String,
    pub dsm_param_write_ctrl: String,
    pub temp_ctrl: Vec<String>,
    // Path of the dsm_param.bin file.
    pub dsm_param: String,
    pub boot_time_calibration_enabled: bool,
}

impl AmpCalibSettings {
    /// Returns the number of channels.
    pub fn num_channels(&self) -> usize {
        self.temp_ctrl.len()
    }
}

impl DeviceSettings {
    /// Creates a `DeviceSettings` from a yaml str.
    pub fn from_yaml_str(conf: &str) -> Result<DeviceSettings> {
        let settings: DeviceSettings = serde_yaml::from_str(conf)
            .map_err(|e| Error::DeserializationFailed("DeviceSettings".to_owned(), e))?;
        Ok(settings)
    }
}
