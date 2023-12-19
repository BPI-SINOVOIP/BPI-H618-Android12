// Copyright 2020 The Chromium OS Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

//! `control` module is meant to provide easier to use and more type safe abstractions
//! for various alsa mixer controls.
//!
//! Each mixer control should implement the `Control` trait to allow itself to be created by `Card`.
//! Each mixer control could hold `Ctl` as handle and `ElemID` as id and use `#[derive(ControlOps)]` macro
//! to generate default load / save implementations of the `ControlOps` trait which allows itself to read and
//! write the underlying hardware.
//!
//! # Examples
//! This is an example of how to define a `SwitchControl`.
//!
//! ```
//! use std::error::Error;
//!
//! use cros_alsa::{Ctl, ElemId, Control, ControlError, ControlOps};
//! use cros_alsa::elem::Elem;
//!
//! type Result<T> = std::result::Result<T, ControlError>;
//!
//! #[derive(ControlOps)]
//! pub struct SwitchControl<'a> {
//!     // Must hold `Ctl` as handle and `ElemID` as id to use `#[derive(ControlOps)]`.
//!     handle: &'a mut Ctl,
//!     id: ElemId,
//! }
//!
//! impl<'a> Control<'a> for SwitchControl <'a> {
//!     type Item = [bool; 1];
//!
//!     fn new(handle: &'a mut Ctl, id: ElemId) -> Self {
//!         Self {
//!             handle,
//!             id,
//!         }
//!     }
//! }
//!
//! impl<'a> SwitchControl<'a> {
//!     /// Reads the state of a switch type mix control.
//!     pub fn state(&mut self) -> Result<bool> {
//!         // Uses ControlOps::load() to read the mixer control.
//!         let v = self.load()?;
//!         Ok(v[0])
//!     }
//!
//!     /// Updates the control state to true.
//!     pub fn on(&mut self) -> Result<()> {
//!         // Uses ControlOps::save() to write the mixer control.
//!         self.save([true])?;
//!         Ok(())
//!     }
//! }
//!
//! ```

use std::error;
use std::fmt;

use cros_alsa_derive::ControlOps;
use remain::sorted;

use crate::control_primitive::{self, Ctl, ElemId, ElemInfo, ElemType};
use crate::elem::{self, Elem};

/// The Result type of cros-alsa::control.
pub type Result<T> = std::result::Result<T, Error>;

#[sorted]
#[derive(Debug)]
/// Possible errors that can occur in cros-alsa::control.
pub enum Error {
    /// Failed to call AlsaControlAPI.
    AlsaControlAPI(control_primitive::Error),
    /// Error occurs in Elem.
    Elem(elem::Error),
    /// Elem::size() does not match the element count of the mixer control.
    MismatchElemCount(String, usize, usize),
    /// Elem::elem_type() does not match the data type of the mixer control.
    MismatchElemType(String, ElemType, ElemType),
}

impl error::Error for Error {}

impl From<control_primitive::Error> for Error {
    fn from(err: control_primitive::Error) -> Error {
        Error::AlsaControlAPI(err)
    }
}

impl From<elem::Error> for Error {
    fn from(err: elem::Error) -> Error {
        Error::Elem(err)
    }
}

impl fmt::Display for Error {
    fn fmt(&self, f: &mut fmt::Formatter) -> fmt::Result {
        use Error::*;
        match self {
            AlsaControlAPI(e) => write!(f, "{}", e),
            Elem(e) => write!(f, "{}", e),
            MismatchElemCount(name, count, elem_count) => write!(
                f,
                "invalid `Control::size()` of {}: expect: {}, get: {}",
                name, count, elem_count
            ),
            MismatchElemType(name, t, elem_type) => write!(
                f,
                "invalid `Control::elem_type()` of {}: expect: {}, get: {}",
                name, t, elem_type
            ),
        }
    }
}

/// Each mixer control should implement the `Control` trait to allow itself to be created by `Card`.
pub trait Control<'a>: Sized + 'a {
    /// The data type of the mixer control.
    /// Use `ElemType::load()` and `ElemType::save()` to read or write the mixer control.
    type Item: Elem;

    /// Called by `Self::from(handle: &'a mut Ctl, id: ElemId)` to create a `Control`.
    fn new(handle: &'a mut Ctl, id: ElemId) -> Self;
    /// Called by `Card` to create a `Control`.
    fn from(handle: &'a mut Ctl, id: ElemId) -> Result<Self> {
        let info = ElemInfo::new(handle, &id)?;
        if info.elem_type()? != Self::elem_type() {
            return Err(Error::MismatchElemType(
                id.name()?.to_owned(),
                info.elem_type()?,
                Self::elem_type(),
            ));
        }

        if info.count() != Self::size() {
            return Err(Error::MismatchElemCount(
                id.name()?.to_owned(),
                info.count(),
                Self::size(),
            ));
        }

        Ok(Self::new(handle, id))
    }
    /// Called by `Self::from(handle: &'a mut Ctl, id: ElemId)` to validate the data type of a
    /// `Control`.
    fn elem_type() -> ElemType {
        Self::Item::elem_type()
    }
    /// Called by `Self::from(handle: &'a mut Ctl, id: ElemId)` to validate the number of value
    /// entries of a `Control`.
    fn size() -> usize {
        Self::Item::size()
    }
}

/// Each mixer control could implement the `ControlOps` trait to allow itself to read and
/// write the underlying hardware`. Users could hold `Ctl` and `ElemID` as `handle` and `id`
/// in their control structure and use `#[derive(ControlOps)]` macro to generate default
/// load / save implementations.
pub trait ControlOps<'a>: Control<'a> {
    /// Reads the values of the mixer control.
    fn load(&mut self) -> Result<<Self as Control<'a>>::Item>;
    /// Saves the values to the mixer control.
    fn save(&mut self, val: <Self as Control<'a>>::Item) -> Result<bool>;
}

/// `Control` that reads and writes a single integer value entry.
/// Since this crate is the `cros_alsa` crate, we replace the `cros_alsa`
/// path to `crate` in derive macros by `cros_alsa` attribute.
#[derive(ControlOps)]
#[cros_alsa(path = "crate")]
pub struct IntControl<'a> {
    handle: &'a mut Ctl,
    id: ElemId,
}

impl<'a> IntControl<'a> {
    /// Gets an i32 value from the mixer control.
    ///
    /// # Errors
    ///
    /// * If it fails to read from the control.
    pub fn get(&mut self) -> Result<i32> {
        let val = self.load()?;
        Ok(val[0])
    }

    /// Updates an i32 value to the mixer control.
    ///
    /// # Errors
    ///
    /// * If it fails to write to the control.
    pub fn set(&mut self, val: i32) -> Result<()> {
        self.save([val])?;
        Ok(())
    }
}

impl<'a> Control<'a> for IntControl<'a> {
    type Item = [i32; 1];
    fn new(handle: &'a mut Ctl, id: ElemId) -> Self {
        Self { handle, id }
    }
}

/// Stereo Volume Mixer Control
/// Since this crate is the `cros_alsa` crate, we replace the `cros_alsa`
/// path to `crate` in derive macros by `cros_alsa` attribute.
#[derive(ControlOps)]
#[cros_alsa(path = "crate")]
pub struct StereoVolumeControl<'a> {
    handle: &'a mut Ctl,
    id: ElemId,
}

impl<'a> StereoVolumeControl<'a> {
    /// Reads the left and right volume.
    ///
    /// # Errors
    ///
    /// * If it fails to read from the control.
    pub fn volume(&mut self) -> Result<(i32, i32)> {
        let val = self.load()?;
        Ok((val[0], val[1]))
    }

    /// Updates the left and right volume.
    ///
    /// # Errors
    ///
    /// * If it fails to write to the control.
    pub fn set_volume(&mut self, left: i32, right: i32) -> Result<()> {
        self.save([left, right])?;
        Ok(())
    }
}

impl<'a> Control<'a> for StereoVolumeControl<'a> {
    type Item = [i32; 2];
    fn new(handle: &'a mut Ctl, id: ElemId) -> Self {
        Self { handle, id }
    }
}

/// `Control` that reads and writes a single boolean value entry.
/// Since this crate is the `cros_alsa` crate, we replace the `cros_alsa`
/// path to `crate` in derive macros by `cros_alsa` attribute.
#[derive(ControlOps)]
#[cros_alsa(path = "crate")]
pub struct SwitchControl<'a> {
    handle: &'a mut Ctl,
    id: ElemId,
}

impl<'a> SwitchControl<'a> {
    /// Reads the state of a switch type mix control.
    ///
    /// # Errors
    ///
    /// * If it fails to read from the control.
    pub fn state(&mut self) -> Result<bool> {
        let v = self.load()?;
        Ok(v[0])
    }

    /// Updates the control state to true.
    ///
    /// # Errors
    ///
    /// * If it fails to write to the control.
    pub fn on(&mut self) -> Result<()> {
        self.save([true])?;
        Ok(())
    }

    /// Updates the control state to false.
    ///
    /// # Errors
    ///
    /// * If it fails to write to the control.
    pub fn off(&mut self) -> Result<()> {
        self.save([false])?;
        Ok(())
    }
}

impl<'a> Control<'a> for SwitchControl<'a> {
    type Item = [bool; 1];
    fn new(handle: &'a mut Ctl, id: ElemId) -> Self {
        Self { handle, id }
    }
}
