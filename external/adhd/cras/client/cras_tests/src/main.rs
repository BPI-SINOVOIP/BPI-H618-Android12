// Copyright 2019 The Chromium OS Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

mod arguments;
mod audio;
mod control;

use std::error;
use std::fmt;

use crate::arguments::Command;
use crate::audio::{capture, playback};
use crate::control::control;

#[derive(Debug)]
pub enum Error {
    Audio(audio::Error),
    ParseArgs(arguments::Error),
    Control(control::Error),
}

impl error::Error for Error {}

impl fmt::Display for Error {
    fn fmt(&self, f: &mut fmt::Formatter) -> fmt::Result {
        use Error::*;
        match self {
            Audio(e) => e.fmt(f),
            ParseArgs(e) => write!(f, "Failed to parse arguments: {}", e),
            Control(e) => e.fmt(f),
        }
    }
}

type Result<T> = std::result::Result<T, Error>;

fn run() -> Result<()> {
    let args: Vec<String> = std::env::args().collect();
    let command = match Command::parse(&args).map_err(Error::ParseArgs)? {
        None => return Ok(()),
        Some(v) => v,
    };

    match command {
        Command::Capture(audio_opts) => capture(audio_opts).map_err(Error::Audio),
        Command::Control(command) => control(command).map_err(Error::Control),
        Command::Playback(audio_opts) => playback(audio_opts).map_err(Error::Audio),
    }
}

fn main() {
    // Use run() instead of returning a Result from main() so that we can print
    // errors using Display instead of Debug.
    if let Err(e) = run() {
        eprintln!("{}", e);
    }
}
