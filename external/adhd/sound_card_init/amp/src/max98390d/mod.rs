// Copyright 2020 The Chromium OS Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//! `max98390d` module implements the required initialization workflows for sound
//! cards that use max98390d smart amp.
//! It currently supports boot time calibration for max98390d.
#![deny(missing_docs)]
mod settings;

use std::time::Duration;
use std::{fs, path::Path};

use cros_alsa::{Card, IntControl, SwitchControl};
use dsm::{CalibData, Error, Result, SpeakerStatus, TempConverter, ZeroPlayer, DSM};

use crate::Amp;
use settings::{AmpCalibSettings, DeviceSettings};

/// Amp volume mode emulation used by set_volume().
#[derive(PartialEq, Clone, Copy)]
enum VolumeMode {
    /// Low mode protects the speaker by limiting its output volume if the
    /// calibration has not been completed successfully.
    Low = 138,
    /// High mode removes the speaker output volume limitation after
    /// having successfully completed the calibration.
    High = 148,
}

/// It implements the Max98390 functions of boot time calibration.
#[derive(Debug)]
pub struct Max98390 {
    card: Card,
    setting: AmpCalibSettings,
}

impl Amp for Max98390 {
    /// Performs max98390d boot time calibration.
    ///
    /// # Errors
    ///
    /// If the amplifier fails to complete the calibration.
    fn boot_time_calibration(&mut self) -> Result<()> {
        if !Path::new(&self.setting.dsm_param).exists() {
            return Err(Error::MissingDSMParam);
        }

        let mut dsm = DSM::new(
            &self.card.name(),
            self.setting.num_channels(),
            Self::rdc_to_ohm,
            Self::TEMP_UPPER_LIMIT_CELSIUS,
            Self::TEMP_LOWER_LIMIT_CELSIUS,
        );
        dsm.set_temp_converter(TempConverter::new(
            Self::dsm_unit_to_celsius,
            Self::celsius_to_dsm_unit,
        ));

        self.set_volume(VolumeMode::Low)?;
        let calib = match dsm.check_speaker_over_heated_workflow()? {
            SpeakerStatus::Hot(previous_calib) => previous_calib,
            SpeakerStatus::Cold => self
                .do_calibration()?
                .iter()
                .enumerate()
                .map(|(ch, calib_data)| dsm.decide_calibration_value_workflow(ch, *calib_data))
                .collect::<Result<Vec<_>>>()?,
        };
        self.apply_calibration_value(calib)?;
        self.set_volume(VolumeMode::High)?;
        Ok(())
    }
}

impl Max98390 {
    const TEMP_UPPER_LIMIT_CELSIUS: f32 = 40.0;
    const TEMP_LOWER_LIMIT_CELSIUS: f32 = 0.0;
    const RDC_CALIB_WARM_UP_TIME: Duration = Duration::from_millis(300);

    /// Creates an `Max98390`.
    /// # Arguments
    ///
    /// * `card_name` - card name.
    /// * `config_path` - config file path.
    ///
    /// # Results
    ///
    /// * `Max98390` - It implements the Max98390 functions of boot time calibration.
    ///
    /// # Errors
    ///
    /// * If `Card` creation from sound card name fails.
    pub fn new(card_name: &str, config_path: &Path) -> Result<Self> {
        let conf = fs::read_to_string(config_path)
            .map_err(|e| Error::FileIOFailed(config_path.to_path_buf(), e))?;
        let settings = DeviceSettings::from_yaml_str(&conf)?;
        Ok(Self {
            card: Card::new(card_name)?,
            setting: settings.amp_calibrations,
        })
    }

    /// Sets the card volume control to given VolumeMode.
    fn set_volume(&mut self, mode: VolumeMode) -> Result<()> {
        for control in &self.setting.controls {
            self.card
                .control_by_name::<IntControl>(&control.volume_ctrl)?
                .set(mode as i32)?;
        }
        Ok(())
    }

    /// Applies the calibration value to the amp.
    fn apply_calibration_value(&mut self, calib: Vec<CalibData>) -> Result<()> {
        for (ch, &CalibData { rdc, temp }) in calib.iter().enumerate() {
            self.card
                .control_by_name::<IntControl>(&self.setting.controls[ch].rdc_ctrl)?
                .set(rdc)?;
            self.card
                .control_by_name::<IntControl>(&self.setting.controls[ch].temp_ctrl)?
                .set(Self::celsius_to_dsm_unit(temp))?;
        }
        Ok(())
    }

    /// Triggers the amplifier calibration and reads the calibrated rdc and ambient_temp value
    /// from the mixer control.
    /// To get accurate calibration results, the main thread calibrates the amplifier while
    /// the `zero_player` starts another thread to play zeros to the speakers.
    fn do_calibration(&mut self) -> Result<Vec<CalibData>> {
        let mut zero_player: ZeroPlayer = Default::default();
        zero_player.start(Self::RDC_CALIB_WARM_UP_TIME)?;
        // Playback of zeros is started for Self::RDC_CALIB_WARM_UP_TIME, and the main thread
        // can start the calibration.
        let setting = &self.setting;
        let card = &mut self.card;
        let calib = setting
            .controls
            .iter()
            .map(|control| {
                card.control_by_name::<SwitchControl>(&control.calib_ctrl)?
                    .on()?;
                let rdc = card
                    .control_by_name::<IntControl>(&control.rdc_ctrl)?
                    .get()?;
                let temp = card
                    .control_by_name::<IntControl>(&control.temp_ctrl)?
                    .get()?;
                card.control_by_name::<SwitchControl>(&control.calib_ctrl)?
                    .off()?;
                Ok(CalibData {
                    rdc,
                    temp: Self::dsm_unit_to_celsius(temp),
                })
            })
            .collect::<Result<Vec<CalibData>>>()?;
        zero_player.stop()?;
        Ok(calib)
    }

    /// Converts the ambient temperature from celsius to the DSM unit.
    #[inline]
    fn celsius_to_dsm_unit(celsius: f32) -> i32 {
        (celsius * ((1 << 12) as f32) / 100.0) as i32
    }

    /// Converts the ambient temperature from  DSM unit to celsius.
    #[inline]
    fn dsm_unit_to_celsius(temp: i32) -> f32 {
        temp as f32 * 100.0 / (1 << 12) as f32
    }

    /// Converts the calibrated value to real DC resistance in ohm unit.
    #[inline]
    fn rdc_to_ohm(x: i32) -> f32 {
        3.66 * (1 << 20) as f32 / x as f32
    }
}

#[cfg(test)]
mod tests {
    use super::*;
    #[test]
    fn celsius_to_dsm_unit() {
        assert_eq!(
            Max98390::celsius_to_dsm_unit(Max98390::TEMP_UPPER_LIMIT_CELSIUS),
            1638
        );
        assert_eq!(
            Max98390::celsius_to_dsm_unit(Max98390::TEMP_LOWER_LIMIT_CELSIUS),
            0
        );
    }

    #[test]
    fn dsm_unit_to_celsius() {
        assert_eq!(
            Max98390::dsm_unit_to_celsius(1638).round(),
            Max98390::TEMP_UPPER_LIMIT_CELSIUS
        );
        assert_eq!(
            Max98390::dsm_unit_to_celsius(0),
            Max98390::TEMP_LOWER_LIMIT_CELSIUS
        );
    }

    #[test]
    fn rdc_to_ohm() {
        assert_eq!(Max98390::rdc_to_ohm(1123160), 3.416956);
        assert_eq!(Max98390::rdc_to_ohm(1157049), 3.3168762);
    }
}
