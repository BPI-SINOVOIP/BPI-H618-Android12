// Copyright 2020 The Chromium OS Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

//! This module provides different implementations of `Elem` that use the alsa-lib control interface
//! API to read and write alsa control elements.
//!
//! The `Elem::type()` returns the type of value that a control element can interact with,
//! and it is one of integer, integer64, boolean, enumerators, bytes or IEC958 structure.
//! The `Elem::size()` returns the number of values it reads from or writes to the hardware
//! at a time.
//! The `Elem::load(..)` and `Elem::save(..)` are used by `ControlOps` trait to read and write
//! the underlying mixer control.
//!
//! Users should use the provided implementations of `Elem` to define the associated type in
//! their owner encapsulation of `Control`.

use std::default::Default;
use std::error;
use std::fmt;

use libc::{c_long, c_uint};
use remain::sorted;

use crate::control_primitive::{self, snd_strerror, Ctl, ElemId, ElemType, ElemValue};

/// The Result type of cros-alsa::elem.
pub type Result<T> = std::result::Result<T, Error>;

#[sorted]
#[derive(Debug)]
/// Possible errors that can occur in cros-alsa::elem.
pub enum Error {
    /// Failed to call AlsaControlAPI.
    AlsaControlAPI(control_primitive::Error),
    /// Failed to call `snd_ctl_elem_read()`.
    ElemReadFailed(i32),
    /// Failed to call `snd_ctl_elem_write()`.
    ElemWriteFailed(i32),
}

impl error::Error for Error {}

impl From<control_primitive::Error> for Error {
    fn from(err: control_primitive::Error) -> Error {
        Error::AlsaControlAPI(err)
    }
}

impl fmt::Display for Error {
    fn fmt(&self, f: &mut fmt::Formatter) -> fmt::Result {
        use Error::*;
        match self {
            AlsaControlAPI(e) => write!(f, "{}", e),
            ElemReadFailed(e) => write!(f, "snd_ctl_elem_read failed: {}", snd_strerror(*e)?),
            ElemWriteFailed(e) => write!(f, "snd_ctl_elem_write failed: {}", snd_strerror(*e)?),
        }
    }
}

// Uses a recursive macro to generate implementation for [bool; n] and [i32; n], n = 1 to 128.
// The `$t:ident $($ts:ident)*` part matches and removes one token at a time. It's used for
// counting recursive steps.
macro_rules! impl_for_array {
    {$n:expr, $type:ty, $t:ident $($ts:ident)*} => {
        impl Elem for [$type; $n] {
            type T = Self;
            /// Reads [$type; $n] data from the mixer control.
            ///
            /// # Errors
            ///
            /// * If it fails to call `snd_ctl_elem_read()`.
            fn load(handle: &mut Ctl, id: &ElemId) -> Result<Self::T>
            {
                let mut elem = ElemValue::new(id)?;
                // Safe because self.handle.as_mut_ptr() is a valid *mut snd_ctl_t and
                // elem.as_mut_ptr() is also a valid *mut snd_ctl_elem_value_t.
                let rc = unsafe { alsa_sys::snd_ctl_elem_read(handle.as_mut_ptr(), elem.as_mut_ptr()) };
                if rc < 0 {
                    return Err(Error::ElemReadFailed(rc));
                }
                let mut ret = [Default::default(); $n];
                for i in 0..$n {
                    // Safe because elem.as_ptr() is a valid snd_ctl_elem_value_t* and i is guaranteed to be
                    // within a valid range.
                    ret[i] = unsafe { <$type>::elem_value_get(&elem, i) };
                }
                Ok(ret)
            }

            /// Updates [$type; $n] data to the mixer control.
            ///
            /// # Results
            ///
            /// * `changed` - false on success.
            ///             - true on success when value was changed.
            ///
            /// # Errors
            ///
            /// * If it fails to call `snd_ctl_elem_write()`.
            fn save(handle: &mut Ctl, id: &ElemId, val: Self::T) -> Result<bool> {
                let mut elem = ElemValue::new(id)?;
                for i in 0..$n {
                    // Safe because elem.as_mut_ptr() is a valid snd_ctl_elem_value_t* and i is guaranteed to be
                    // within a valid range.
                    unsafe { <$type>::elem_value_set(&mut elem, i, val[i]) };
                }
                // Safe because self.handle.as_mut_ptr() is a valid *mut snd_ctl_t and
                // elem.as_mut_ptr() is also a valid *mut snd_ctl_elem_value_t.
                let rc = unsafe { alsa_sys::snd_ctl_elem_write(handle.as_mut_ptr(), elem.as_mut_ptr()) };
                if rc < 0 {
                    return Err(Error::ElemWriteFailed(rc));
                }
                Ok(rc > 0)
            }

            /// Gets the data type itself can read and write.
            fn elem_type() -> ElemType {
                <$type>::elem_type()
            }

            /// Gets the number of value entries itself can read and write.
            fn size() -> usize {
                $n
            }
        }
        impl_for_array!{($n - 1), $type, $($ts)*}
    };
    {$n:expr, $type:ty,} => {};
}

// Implements `Elem` for [i32; n] where n = 1 to 128.
impl_for_array! {128, i32,
T T T T T T T T T T T T T T T T T T T T T T T T T T T T T T T T
T T T T T T T T T T T T T T T T T T T T T T T T T T T T T T T T
T T T T T T T T T T T T T T T T T T T T T T T T T T T T T T T T
T T T T T T T T T T T T T T T T T T T T T T T T T T T T T T T T
}

// Implements `Elem` for [bool; n] where n = 1 to 128.
impl_for_array! {128, bool,
T T T T T T T T T T T T T T T T T T T T T T T T T T T T T T T T
T T T T T T T T T T T T T T T T T T T T T T T T T T T T T T T T
T T T T T T T T T T T T T T T T T T T T T T T T T T T T T T T T
T T T T T T T T T T T T T T T T T T T T T T T T T T T T T T T T
}

impl CtlElemValue for bool {
    type T = bool;
    /// Gets a bool from the ElemValue.
    unsafe fn elem_value_get(elem: &ElemValue, idx: usize) -> bool {
        alsa_sys::snd_ctl_elem_value_get_boolean(elem.as_ptr(), idx as c_uint) != 0
    }
    /// Sets a bool to the ElemValue.
    unsafe fn elem_value_set(elem: &mut ElemValue, idx: usize, val: bool) {
        alsa_sys::snd_ctl_elem_value_set_boolean(elem.as_mut_ptr(), idx as c_uint, val as c_long);
    }
    /// Returns ElemType::Boolean.
    fn elem_type() -> ElemType {
        ElemType::Boolean
    }
}

impl CtlElemValue for i32 {
    type T = i32;
    /// Gets an i32 from the ElemValue.
    unsafe fn elem_value_get(elem: &ElemValue, idx: usize) -> i32 {
        alsa_sys::snd_ctl_elem_value_get_integer(elem.as_ptr(), idx as c_uint) as i32
    }
    /// Sets an i32 to the ElemValue.
    unsafe fn elem_value_set(elem: &mut ElemValue, idx: usize, val: i32) {
        alsa_sys::snd_ctl_elem_value_set_integer(elem.as_mut_ptr(), idx as c_uint, val as c_long);
    }
    /// Returns ElemType::Integer.
    fn elem_type() -> ElemType {
        ElemType::Integer
    }
}

/// All primitive types of a control element should implement `CtlElemValue` trait.
trait CtlElemValue {
    /// The primitive type of a control element.
    type T;
    /// Gets the value from the ElemValue.
    unsafe fn elem_value_get(value: &ElemValue, idx: usize) -> Self::T;
    /// Sets the value to the ElemValue.
    unsafe fn elem_value_set(value: &mut ElemValue, id: usize, val: Self::T);
    /// Gets the data type itself can read and write.
    fn elem_type() -> ElemType;
}

/// Use `Elem` trait to access the underlying control element through the given `Ctl` and `ElemId`.
pub trait Elem: Sized {
    /// The data type of a control element.
    type T;
    /// Reads the value from the mixer control.
    fn load(handle: &mut Ctl, id: &ElemId) -> Result<Self::T>;
    /// Saves the value to the mixer control.
    fn save(handle: &mut Ctl, id: &ElemId, val: Self::T) -> Result<bool>;
    /// Gets the data type itself can read and write.
    fn elem_type() -> ElemType;
    /// Gets the number of value entries itself can read and write.
    fn size() -> usize;
}
