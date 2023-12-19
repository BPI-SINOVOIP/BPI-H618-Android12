// Copyright 2020 The Chromium OS Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//! `dsm` crate implements the required initialization workflows for smart amps.

mod datastore;
mod error;
pub mod utils;
mod vpd;
mod zero_player;

use std::{
    thread,
    time::{Duration, SystemTime, UNIX_EPOCH},
};

use libcras::{CrasClient, CrasNodeType};
use sys_util::{error, info};

use crate::datastore::Datastore;
pub use crate::error::{Error, Result};
use crate::utils::{run_time, shutdown_time};
use crate::vpd::VPD;
pub use crate::zero_player::ZeroPlayer;

#[derive(Debug, Clone, Copy)]
/// `CalibData` represents the calibration data.
pub struct CalibData {
    /// The DC resistance of the speaker is DSM unit.
    pub rdc: i32,
    /// The ambient temperature in celsius unit at which the rdc is measured.
    pub temp: f32,
}

/// `TempConverter` converts the temperature value between celsius and unit in VPD::dsm_calib_temp.
pub struct TempConverter {
    vpd_to_celsius: fn(i32) -> f32,
    celsius_to_vpd: fn(f32) -> i32,
}

impl Default for TempConverter {
    fn default() -> Self {
        let vpd_to_celsius = |x: i32| x as f32;
        let celsius_to_vpd = |x: f32| x.round() as i32;
        Self {
            vpd_to_celsius,
            celsius_to_vpd,
        }
    }
}

impl TempConverter {
    /// Creates a `TempConverter`
    ///
    /// # Arguments
    ///
    /// * `vpd_to_celsius` - function to convert VPD::dsm_calib_temp to celsius unit`
    /// * `celsius_to_vpd` - function to convert celsius unit to VPD::dsm_calib_temp`
    /// # Results
    ///
    /// * `TempConverter` - it converts the temperature value between celsius and unit in VPD::dsm_calib_temp.
    pub fn new(vpd_to_celsius: fn(i32) -> f32, celsius_to_vpd: fn(f32) -> i32) -> Self {
        Self {
            vpd_to_celsius,
            celsius_to_vpd,
        }
    }
}

/// `SpeakerStatus` are the possible return results of
/// DSM::check_speaker_over_heated_workflow.
pub enum SpeakerStatus {
    ///`SpeakerStatus::Cold` means the speakers are not overheated and the Amp can
    /// trigger the boot time calibration.
    Cold,
    /// `SpeakerStatus::Hot(Vec<CalibData>)` means the speakers may be too hot for calibration.
    /// The boot time calibration should be skipped and the Amp should use the previous
    /// calibration values returned by the enum.
    Hot(Vec<CalibData>),
}

/// `DSM`, which implements the required initialization workflows for smart amps.
pub struct DSM {
    snd_card: String,
    num_channels: usize,
    temp_converter: TempConverter,
    rdc_to_ohm: fn(i32) -> f32,
    temp_upper_limit: f32,
    temp_lower_limit: f32,
}

impl DSM {
    const SPEAKER_COOL_DOWN_TIME: Duration = Duration::from_secs(180);
    const CALI_ERROR_UPPER_LIMIT: f32 = 0.3;
    const CALI_ERROR_LOWER_LIMIT: f32 = 0.03;

    /// Creates a `DSM`
    ///
    /// # Arguments
    ///
    /// * `snd_card` - `sound card name`.
    /// * `num_channels` - `number of channels`.
    /// * `rdc_to_ohm` - `fn(rdc: i32) -> f32 to convert the CalibData::rdc to ohm unit`.
    /// * `temp_upper_limit` - the high limit of the valid ambient temperature in dsm unit.
    /// * `temp_lower_limit` - the low limit of the valid ambient temperature in dsm unit.
    ///
    /// # Results
    ///
    /// * `DSM` - It implements the required initialization workflows for smart amps.
    pub fn new(
        snd_card: &str,
        num_channels: usize,
        rdc_to_ohm: fn(i32) -> f32,
        temp_upper_limit: f32,
        temp_lower_limit: f32,
    ) -> Self {
        Self {
            snd_card: snd_card.to_owned(),
            num_channels,
            rdc_to_ohm,
            temp_converter: TempConverter::default(),
            temp_upper_limit,
            temp_lower_limit,
        }
    }

    /// Sets self.temp_converter to the given temp_converter.
    ///
    /// # Arguments
    ///
    /// * `temp_converter` - the convert function to use.
    pub fn set_temp_converter(&mut self, temp_converter: TempConverter) {
        self.temp_converter = temp_converter;
    }

    /// Checks whether the speakers are overheated or not according to the previous shutdown time.
    /// The boot time calibration should be skipped when the speakers may be too hot
    /// and the Amp should use the previous calibration value returned by the
    /// SpeakerStatus::Hot(Vec<CalibData>).
    ///
    /// # Results
    ///
    /// * `SpeakerStatus::Cold` - which means the speakers are not overheated and the Amp can
    ///    trigger the boot time calibration.
    /// * `SpeakerStatus::Hot(Vec<CalibData>)` - when the speakers may be too hot. The boot
    ///   time calibration should be skipped and the Amp should use the previous calibration values
    ///   returned by the enum.
    ///
    /// # Errors
    ///
    /// * The speakers are overheated and there are no previous calibration values stored.
    /// * Cannot determine whether the speakers are overheated as previous shutdown time record is
    ///   invalid.
    pub fn check_speaker_over_heated_workflow(&self) -> Result<SpeakerStatus> {
        if self.is_first_boot() {
            return Ok(SpeakerStatus::Cold);
        }
        match self.is_speaker_over_heated() {
            Ok(overheated) => {
                if overheated {
                    let calib: Vec<CalibData> = (0..self.num_channels)
                        .map(|ch| -> Result<CalibData> { self.get_previous_calibration_value(ch) })
                        .collect::<Result<Vec<CalibData>>>()?;
                    info!("the speakers are hot, the boot time calibration should be skipped");
                    return Ok(SpeakerStatus::Hot(calib));
                }
                Ok(SpeakerStatus::Cold)
            }
            Err(err) => {
                // We cannot assume the speakers are not replaced or not overheated
                // when the shutdown time file is invalid; therefore we can not use the datastore
                // value anymore and we can not trigger boot time calibration.
                for ch in 0..self.num_channels {
                    if let Err(e) = Datastore::delete(&self.snd_card, ch) {
                        error!("error delete datastore: {}", e);
                    }
                }
                Err(err)
            }
        }
    }

    /// Decides a good calibration value and updates the stored value according to the following
    /// logic:
    /// * Returns the previous value if the ambient temperature is not within a valid range.
    /// * Returns Error::LargeCalibrationDiff if rdc difference is larger than
    ///   `CALI_ERROR_UPPER_LIMIT`.
    /// * Returns the previous value if the rdc difference is smaller than `CALI_ERROR_LOWER_LIMIT`.
    /// * Returns the boot time calibration value and updates the datastore value if the rdc.
    ///   difference is between `CALI_ERROR_UPPER_LIMIT` and `CALI_ERROR_LOWER_LIMIT`.
    ///
    /// # Arguments
    ///
    /// * `card` - `&Card`.
    /// * `channel` - `channel number`.
    /// * `calib_data` - `boot time calibrated data`.
    ///
    /// # Results
    ///
    /// * `CalibData` - the calibration data to be applied according to the deciding logic.
    ///
    /// # Errors
    ///
    /// * VPD does not exist.
    /// * rdc difference is larger than `CALI_ERROR_UPPER_LIMIT`.
    /// * Failed to update Datastore.
    pub fn decide_calibration_value_workflow(
        &self,
        channel: usize,
        calib_data: CalibData,
    ) -> Result<CalibData> {
        if calib_data.temp < self.temp_lower_limit || calib_data.temp > self.temp_upper_limit {
            info!("invalid temperature: {}.", calib_data.temp);
            return self
                .get_previous_calibration_value(channel)
                .map_err(|_| Error::InvalidTemperature(calib_data.temp));
        }
        let (datastore_exist, previous_calib) = match self.get_previous_calibration_value(channel) {
            Ok(previous_calib) => (true, previous_calib),
            Err(e) => {
                info!("{}, use vpd as previous calibration value", e);
                (false, self.get_vpd_calibration_value(channel)?)
            }
        };

        let diff = {
            let calib_rdc_ohm = (self.rdc_to_ohm)(calib_data.rdc);
            let previous_rdc_ohm = (self.rdc_to_ohm)(previous_calib.rdc);
            (calib_rdc_ohm - previous_rdc_ohm) / previous_rdc_ohm
        };
        if diff > Self::CALI_ERROR_UPPER_LIMIT {
            Err(Error::LargeCalibrationDiff(calib_data))
        } else if diff < Self::CALI_ERROR_LOWER_LIMIT {
            if !datastore_exist {
                Datastore::UseVPD.save(&self.snd_card, channel)?;
            }
            Ok(previous_calib)
        } else {
            Datastore::DSM {
                rdc: calib_data.rdc,
                temp: (self.temp_converter.celsius_to_vpd)(calib_data.temp),
            }
            .save(&self.snd_card, channel)?;
            Ok(calib_data)
        }
    }

    /// Gets the calibration values from vpd.
    ///
    /// # Results
    ///
    /// * `Vec<CalibData>` - the calibration values in vpd.
    ///
    /// # Errors
    ///
    /// * Failed to read vpd.
    pub fn get_all_vpd_calibration_value(&self) -> Result<Vec<CalibData>> {
        (0..self.num_channels)
            .map(|ch| self.get_vpd_calibration_value(ch))
            .collect::<Result<Vec<_>>>()
    }

    /// Blocks until the internal speakers are ready.
    ///
    /// # Errors
    ///
    /// * Failed to wait the internal speakers to be ready.
    pub fn wait_for_speakers_ready(&self) -> Result<()> {
        let find_speaker = || -> Result<()> {
            let cras_client = CrasClient::new().map_err(Error::CrasClientFailed)?;
            let _node = cras_client
                .output_nodes()
                .find(|node| node.node_type == CrasNodeType::CRAS_NODE_TYPE_INTERNAL_SPEAKER)
                .ok_or(Error::InternalSpeakerNotFound)?;
            Ok(())
        };
        // TODO(b/155007305): Implement cras_client.wait_node_change and use it here.
        const RETRY: usize = 3;
        const RETRY_INTERVAL: Duration = Duration::from_millis(500);
        for _ in 0..RETRY {
            match find_speaker() {
                Ok(_) => return Ok(()),
                Err(e) => error!("retry on finding speaker: {}", e),
            };
            thread::sleep(RETRY_INTERVAL);
        }
        Err(Error::InternalSpeakerNotFound)
    }

    fn is_first_boot(&self) -> bool {
        !run_time::exists(&self.snd_card)
    }

    // If (Current time - the latest CRAS shutdown time) < cool_down_time, we assume that
    // the speakers may be overheated.
    fn is_speaker_over_heated(&self) -> Result<bool> {
        let last_run = run_time::from_file(&self.snd_card)?;
        let last_shutdown = shutdown_time::from_file()?;
        if last_shutdown < last_run {
            return Err(Error::InvalidShutDownTime);
        }

        let now = SystemTime::now()
            .duration_since(UNIX_EPOCH)
            .map_err(Error::SystemTimeError)?;

        let elapsed = now
            .checked_sub(last_shutdown)
            .ok_or(Error::InvalidShutDownTime)?;

        if elapsed < Self::SPEAKER_COOL_DOWN_TIME {
            return Ok(true);
        }
        Ok(false)
    }

    fn get_previous_calibration_value(&self, ch: usize) -> Result<CalibData> {
        let sci_calib = Datastore::from_file(&self.snd_card, ch)?;
        match sci_calib {
            Datastore::UseVPD => self.get_vpd_calibration_value(ch),
            Datastore::DSM { rdc, temp } => Ok(CalibData {
                rdc,
                temp: (self.temp_converter.vpd_to_celsius)(temp),
            }),
        }
    }

    fn get_vpd_calibration_value(&self, channel: usize) -> Result<CalibData> {
        let vpd = VPD::new(channel)?;
        Ok(CalibData {
            rdc: vpd.dsm_calib_r0,
            temp: (self.temp_converter.vpd_to_celsius)(vpd.dsm_calib_temp),
        })
    }
}
