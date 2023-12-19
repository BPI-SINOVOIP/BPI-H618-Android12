// Copyright 2020 The Chromium OS Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

use std::error;
use std::fmt;

use remain::sorted;

use crate::control::{self, Control};
use crate::control_primitive;
use crate::control_primitive::{Ctl, ElemId, ElemIface};
use crate::control_tlv::{self, ControlTLV};

pub type Result<T> = std::result::Result<T, Error>;

#[sorted]
#[derive(Debug)]
/// Possible errors that can occur in cros-alsa::card.
pub enum Error {
    /// Failed to call AlsaControlAPI.
    AlsaControlAPI(control_primitive::Error),
    /// Error occurs in Control.
    Control(control::Error),
    /// Error occurs in ControlTLV.
    ControlTLV(control_tlv::Error),
}

impl error::Error for Error {}

impl From<control::Error> for Error {
    fn from(err: control::Error) -> Error {
        Error::Control(err)
    }
}

impl From<control_tlv::Error> for Error {
    fn from(err: control_tlv::Error) -> Error {
        Error::ControlTLV(err)
    }
}

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
            Control(e) => write!(f, "{}", e),
            ControlTLV(e) => write!(f, "{}", e),
        }
    }
}

/// `Card` represents a sound card.
#[derive(Debug)]
pub struct Card {
    handle: Ctl,
    name: String,
}

impl Card {
    /// Creates a `Card`.
    ///
    /// # Arguments
    ///
    /// * `card_name` - The sound card name, ex: sofcmlmax98390d.
    ///
    /// # Errors
    ///
    /// * If card_name is an invalid CString.
    /// * If snd_ctl_open() fails.
    pub fn new(card_name: &str) -> Result<Self> {
        let handle = Ctl::new(&format!("hw:{}", card_name))?;
        Ok(Card {
            name: card_name.to_owned(),
            handle,
        })
    }

    /// Gets sound card name.
    pub fn name(&self) -> &str {
        &self.name
    }

    /// Creates a `Control` from control name.
    ///
    /// # Errors
    ///
    /// * If control name is an invalid CString.
    /// * If control does not exist.
    /// * If `Control` elem_type() mismatches the type of underlying mixer control.
    /// * If `Control` size() mismatches the number of value entries of underlying mixer control.
    pub fn control_by_name<'a, T: 'a>(&'a mut self, control_name: &str) -> Result<T>
    where
        T: Control<'a>,
    {
        let id = ElemId::new(ElemIface::Mixer, control_name)?;
        Ok(T::from(&mut self.handle, id)?)
    }

    /// Creates a `ControlTLV` from control name.
    ///
    /// # Errors
    ///
    /// * If control name is an invalid CString.
    /// * If control does not exist.
    pub fn control_tlv_by_name<'a>(&'a mut self, control_name: &str) -> Result<ControlTLV<'a>> {
        let id = ElemId::new(ElemIface::Mixer, control_name)?;
        Ok(ControlTLV::new(&mut self.handle, id)?)
    }
}
