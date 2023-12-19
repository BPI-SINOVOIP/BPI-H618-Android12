// Copyright 2020 The Chromium OS Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
use std::mem;

use cros_alsa::{Card, TLV};
use sof_sys::sof_abi_hdr;

use dsm::{self, Error, Result};

/// Amp volume mode enumeration used by set_volume().
#[derive(Copy, Clone, PartialEq)]
pub enum VolumeMode {
    /// Low mode protects the speaker by limiting its output volume if the
    /// calibration has not been completed successfully.
    Low = 0x1009B9CF,
    /// High mode removes the speaker output volume limitation after
    /// having successfully completed the calibration.
    High = 0x20000000,
}

#[derive(Copy, Clone)]
/// Calibration mode enumeration.
pub enum CalibMode {
    ON = 0x4,
    OFF = 0x1,
}

#[derive(Copy, Clone)]
/// Smart pilot signal mode mode enumeration.
pub enum SPTMode {
    ON = 0x1,
    OFF = 0x0,
}

#[derive(Copy, Clone)]
/// DSM Parem field enumeration.
enum DsmAPI {
    ParamCount = 0x0,
    CalibMode = 0x1,
    MakeupGain = 0x5,
    DsmRdc = 0x6,
    DsmAmbientTemp = 0x8,
    AdaptiveRdc = 0x12,
    SPTMode = 0x68,
}

#[derive(Debug)]
/// It implements functions to access the `DSMParam` fields.
pub struct DSMParam {
    param_count: usize,
    num_channels: usize,
    tlv: TLV,
}

impl DSMParam {
    const DWORD_PER_PARAM: usize = 2;
    const VALUE_OFFSET: usize = 1;
    const SOF_HEADER_SIZE: usize = mem::size_of::<sof_abi_hdr>() / mem::size_of::<i32>();

    /// Creates an `DSMParam`.
    /// # Arguments
    ///
    /// * `card` - `&Card`.
    /// * `num_channels` - number of channels.
    /// * `ctl_name` - the mixer control name to access the DSM param.
    ///
    /// # Results
    ///
    /// * `DSMParam` - It is initialized by the content of the given byte control .
    ///
    /// # Errors
    ///
    /// * If `Card` creation from sound card name fails.
    pub fn new(card: &mut Card, num_channels: usize, ctl_name: &str) -> Result<Self> {
        let tlv = card.control_tlv_by_name(ctl_name)?.load()?;
        Self::try_from_tlv(tlv, num_channels)
    }

    /// Sets DSMParam to the given calibration mode.
    pub fn set_calibration_mode(&mut self, mode: CalibMode) {
        for channel in 0..self.num_channels {
            self.set(channel, DsmAPI::CalibMode, mode as i32);
        }
    }

    /// Sets DSMParam to the given smart pilot signal mode.
    pub fn set_spt_mode(&mut self, mode: SPTMode) {
        for channel in 0..self.num_channels {
            self.set(channel, DsmAPI::SPTMode, mode as i32);
        }
    }

    /// Sets DSMParam to the given VolumeMode.
    pub fn set_volume_mode(&mut self, mode: VolumeMode) {
        for channel in 0..self.num_channels {
            self.set(channel, DsmAPI::MakeupGain, mode as i32);
        }
    }

    /// Reads the calibrated rdc from DSMParam.
    pub fn get_adaptive_rdc(&self) -> Vec<i32> {
        self.get(DsmAPI::AdaptiveRdc)
    }

    /// Sets DSMParam to the given the calibrated rdc.
    pub fn set_rdc(&mut self, ch: usize, rdc: i32) {
        self.set(ch, DsmAPI::DsmRdc, rdc);
    }

    /// Sets DSMParam to the given calibrated temp.
    pub fn set_ambient_temp(&mut self, ch: usize, temp: i32) {
        self.set(ch, DsmAPI::DsmAmbientTemp, temp);
    }

    /// Sets the `id` field to the given `val`.
    fn set(&mut self, channel: usize, id: DsmAPI, val: i32) {
        let pos = Self::value_pos(self.param_count, channel, id);
        self.tlv[pos] = val as u32;
    }

    /// Gets the val from the `id` field from all the channels.
    fn get(&self, id: DsmAPI) -> Vec<i32> {
        (0..self.num_channels)
            .map(|channel| {
                let pos = Self::value_pos(self.param_count, channel, id);
                self.tlv[pos] as i32
            })
            .collect()
    }

    fn try_from_tlv(tlv: TLV, num_channels: usize) -> Result<Self> {
        let param_count_pos = Self::value_pos(0, 0, DsmAPI::ParamCount);

        if tlv.len() < param_count_pos {
            return Err(Error::InvalidDSMParam);
        }

        let param_count = tlv[param_count_pos] as usize;

        if tlv.len() != Self::SOF_HEADER_SIZE + param_count * num_channels * Self::DWORD_PER_PARAM {
            return Err(Error::InvalidDSMParam);
        }

        Ok(Self {
            param_count,
            num_channels,
            tlv,
        })
    }

    #[inline]
    fn value_pos(param_count: usize, channel: usize, id: DsmAPI) -> usize {
        Self::SOF_HEADER_SIZE
            + (channel * param_count + id as usize) * Self::DWORD_PER_PARAM
            + Self::VALUE_OFFSET
    }
}

impl Into<TLV> for DSMParam {
    fn into(self) -> TLV {
        self.tlv
    }
}

#[cfg(test)]
mod tests {
    use super::*;
    const PARAM_COUNT: usize = 138;
    const CHANNEL_COUNT: usize = 2;

    #[test]
    fn test_dsmparam_try_from_tlv_ok() {
        let mut data = vec![
            0u32;
            DSMParam::SOF_HEADER_SIZE
                + CHANNEL_COUNT * PARAM_COUNT * DSMParam::DWORD_PER_PARAM
        ];
        data[DSMParam::value_pos(PARAM_COUNT, 0, DsmAPI::ParamCount)] = PARAM_COUNT as u32;
        data[DSMParam::value_pos(PARAM_COUNT, 1, DsmAPI::ParamCount)] = PARAM_COUNT as u32;

        let tlv = TLV::new(0, data);
        assert!(DSMParam::try_from_tlv(tlv, CHANNEL_COUNT).is_ok());
    }

    #[test]
    fn test_dsmparam_try_from_invalid_len() {
        let data = vec![0u32; DSMParam::SOF_HEADER_SIZE];

        let tlv = TLV::new(0, data);
        assert_eq!(
            DSMParam::try_from_tlv(tlv, CHANNEL_COUNT).unwrap_err(),
            Error::InvalidDSMParam
        );
    }

    #[test]
    fn test_dsmparam_try_from_param_count() {
        let data = vec![
            0u32;
            DSMParam::SOF_HEADER_SIZE
                + CHANNEL_COUNT * PARAM_COUNT * DSMParam::DWORD_PER_PARAM
        ];

        let tlv = TLV::new(0, data);
        assert_eq!(
            DSMParam::try_from_tlv(tlv, CHANNEL_COUNT).unwrap_err(),
            Error::InvalidDSMParam
        );
    }
}
