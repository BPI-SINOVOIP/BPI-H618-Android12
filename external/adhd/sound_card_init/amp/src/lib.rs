// Copyright 2021 The Chromium OS Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//! `amp` crate provides `Amp` trait for amplifier initializations and `AmpBuilder`
//! to create `Amp` objects.
#![deny(missing_docs)]

mod max98373d;
mod max98390d;
use std::path::PathBuf;

use dsm::Error;

use max98373d::Max98373;
use max98390d::Max98390;

type Result<T> = std::result::Result<T, Error>;
const CONF_DIR: &str = "/etc/sound_card_init";

/// It creates `Amp` object based on the sound card name.
pub struct AmpBuilder<'a> {
    sound_card_id: &'a str,
    config_path: PathBuf,
}

impl<'a> AmpBuilder<'a> {
    /// Creates an `AmpBuilder`.
    /// # Arguments
    ///
    /// * `card_name` - card name.
    /// * `conf_file` - config file name.
    pub fn new(sound_card_id: &'a str, conf_file: &'a str) -> Self {
        let config_path = PathBuf::from(CONF_DIR).join(conf_file);
        AmpBuilder {
            sound_card_id,
            config_path,
        }
    }

    /// Creates an `Amp` based on the sound card name.
    pub fn build(&self) -> Result<Box<dyn Amp>> {
        match self.sound_card_id {
            "sofcmlmax98390d" => {
                Ok(Box::new(Max98390::new(self.sound_card_id, &self.config_path)?) as Box<dyn Amp>)
            }
            "sofrt5682" => {
                Ok(Box::new(Max98373::new(self.sound_card_id, &self.config_path)?) as Box<dyn Amp>)
            }
            _ => Err(Error::UnsupportedSoundCard(self.sound_card_id.to_owned())),
        }
    }
}

/// It defines the required functions of amplifier objects.
pub trait Amp {
    /// The amplifier boot time calibration flow.
    fn boot_time_calibration(&mut self) -> Result<()>;
}
