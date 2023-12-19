// Copyright 2020 The Chromium OS Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

//! `cros_alsa` crate currently supports interacting with alsa
//! controls by using the control interface API of alsa-lib.
//!
//! # Examples
//! This is an example of how to use the provided `Control` objects.
//!
//! ``` no_run
//! use std::error::Error;
//! use std::result::Result;
//!
//! use cros_alsa::{Card, SwitchControl, IntControl, StereoVolumeControl};
//!
//! fn main() -> Result<(), Box<dyn Error>> {
//!
//!   let mut card = Card::new("sofmax98390d")?;
//!
//!   // Uses a SwitchControl to turn on and off a mixer control that has a single boolean state.
//!   let mut calib_ctrl:SwitchControl = card.control_by_name("Left DSM Calibration")?;
//!   calib_ctrl.on()?;
//!   assert_eq!(calib_ctrl.state()?, true);
//!   calib_ctrl.off()?;
//!
//!   // Uses an IntControl to read and write a mixer control that has a single integer value.
//!   let mut rdc_ctrl:IntControl = card.control_by_name("Left Rdc")?;
//!   let _rdc = rdc_ctrl.get()?;
//!   rdc_ctrl.set(13000)?;
//!
//!   // Uses a StereoVolumeControl to manipulate stereo volume related functionality.
//!   let mut volume_ctrl:StereoVolumeControl = card.control_by_name("Master Playback Volume")?;
//!   volume_ctrl.set_volume(184, 184)?;
//!
//!   Ok(())
//! }
//! ```

// Allow the maximum recursive depth = 256 for macro expansion.
#![recursion_limit = "256"]
#![deny(missing_docs)]

mod card;
mod control;
mod control_primitive;
pub mod control_tlv;
pub mod elem;

pub use self::card::Card;
pub use self::control::{Control, ControlOps, IntControl, StereoVolumeControl, SwitchControl};
pub use self::control_primitive::{Ctl, ElemId};
pub use self::control_tlv::{ControlTLV, TLV};

pub use self::card::Error as CardError;
pub use self::control::Error as ControlError;
pub use self::control_tlv::Error as ControlTLVError;
pub use self::elem::Error as ElemError;

#[allow(unused_imports)]
pub use cros_alsa_derive::*;
