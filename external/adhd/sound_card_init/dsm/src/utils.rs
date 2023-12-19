// Copyright 2020 The Chromium OS Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//! It contains common utils shared within sound_card_init.
#![deny(missing_docs)]

use std::fs::File;
use std::io::{prelude::*, BufReader, BufWriter};
use std::path::PathBuf;
use std::time::Duration;

use crate::datastore::Datastore;
use crate::error::{Error, Result};

fn duration_from_file(path: &PathBuf) -> Result<Duration> {
    let reader =
        BufReader::new(File::open(&path).map_err(|e| Error::FileIOFailed(path.clone(), e))?);
    serde_yaml::from_reader(reader).map_err(|e| Error::SerdeError(path.clone(), e))
}

/// The utils to parse CRAS shutdown time file.
pub mod shutdown_time {
    use super::*;
    // The path of CRAS shutdown time file.
    const SHUTDOWN_TIME_FILE: &str = "/var/lib/cras/stop";

    /// Reads the unix time from CRAS shutdown time file.
    pub fn from_file() -> Result<Duration> {
        duration_from_file(&PathBuf::from(SHUTDOWN_TIME_FILE))
    }
}

/// The utils to create and parse sound_card_init run time file.
pub mod run_time {
    use std::time::SystemTime;

    use super::*;
    // The filename of sound_card_init run time file.
    const RUN_TIME_FILE: &str = "run";

    /// Returns the sound_card_init run time file existence.
    pub fn exists(snd_card: &str) -> bool {
        run_time_file(snd_card).exists()
    }

    /// Reads the unix time from sound_card_init run time file.
    pub fn from_file(snd_card: &str) -> Result<Duration> {
        duration_from_file(&run_time_file(snd_card))
    }

    /// Saves the current unix time to sound_card_init run time file.
    pub fn now_to_file(snd_card: &str) -> Result<()> {
        match SystemTime::now().duration_since(SystemTime::UNIX_EPOCH) {
            Ok(t) => to_file(snd_card, t),
            Err(e) => Err(Error::SystemTimeError(e)),
        }
    }

    /// Saves the unix time to sound_card_init run time file.
    pub fn to_file(snd_card: &str, duration: Duration) -> Result<()> {
        let path = run_time_file(snd_card);
        let mut writer =
            BufWriter::new(File::create(&path).map_err(|e| Error::FileIOFailed(path.clone(), e))?);
        writer
            .write_all(
                serde_yaml::to_string(&duration)
                    .map_err(|e| Error::SerdeError(path.clone(), e))?
                    .as_bytes(),
            )
            .map_err(|e| Error::FileIOFailed(path.clone(), e))?;
        writer
            .flush()
            .map_err(|e| Error::FileIOFailed(path.clone(), e))?;
        Ok(())
    }

    fn run_time_file(snd_card: &str) -> PathBuf {
        PathBuf::from(Datastore::DATASTORE_DIR)
            .join(snd_card)
            .join(RUN_TIME_FILE)
    }
}
