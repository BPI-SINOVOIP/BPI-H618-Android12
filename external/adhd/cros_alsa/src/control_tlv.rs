// Copyright 2020 The Chromium OS Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

//! `ControlTLV` supports read and write of the alsa TLV byte controls
//!  Users can obtain a ControlTLV by Card::control_tlv_by_name().
//! # Examples
//! This is an example of how to use a `TLV`.
//!
//! ```
//! use std::assert_eq;
//! use std::convert::TryFrom;
//! use std::error::Error;
//!
//! use cros_alsa::{TLV, ControlTLVError};
//! use cros_alsa::elem::Elem;
//!
//! type Result<T> = std::result::Result<T, ControlTLVError>;
//!
//!     let mut tlv = TLV::new(0, vec![1,2,3,4]);
//!     assert_eq!(4, tlv.len());
//!     assert_eq!(0, tlv.tlv_type());
//!     assert_eq!(2, tlv[1]);
//!     tlv[1] = 8;
//!     assert_eq!(vec![1,8,3,4], tlv.value().to_vec());
//!     assert_eq!(vec![0,16,1,8,3,4], Into::<Vec<u32>>::into(tlv));
//!
//! ```

use std::{
    convert::TryFrom,
    fmt,
    ops::{Index, IndexMut},
    slice::SliceIndex,
};
use std::{error, mem::size_of};

use remain::sorted;

use crate::control_primitive::{self, Ctl, ElemId, ElemInfo, ElemType};

/// The Result type of cros-alsa::control.
pub type Result<T> = std::result::Result<T, Error>;

#[sorted]
#[derive(Debug, PartialEq)]
/// Possible errors that can occur in cros-alsa::control.
pub enum Error {
    /// Failed to call AlsaControlAPI.
    AlsaControlAPI(control_primitive::Error),
    /// Failed to convert buffer to TLV struct.
    InvalidTLV,
    /// ElemInfo::count() is not multiple of size_of::<u32>.
    InvalidTLVSize(String, usize),
    /// ElemInfo::elem_type() is not ElemType::Bytes.
    InvalidTLVType(String, ElemType),
    /// The control is not readable.
    TLVNotReadable,
    /// The control is not writeable.
    TLVNotWritable,
    /// Failed to call snd_ctl_elem_tlv_read.
    TLVReadFailed(i32),
    /// Failed to call snd_ctl_elem_tlv_write.
    TVLWriteFailed(i32),
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
            InvalidTLV => write!(f, "failed to convert to TLV"),
            InvalidTLVSize(name, elem_size) => write!(
                f,
                "ElemInfo::size() of {} should be multiple of size_of::<u32>, get: {}",
                name, elem_size
            ),
            InvalidTLVType(name, elem_type) => write!(
                f,
                "invalid ElemInfo::elem_type() of {}: expect: {}, get: {}",
                name,
                ElemType::Bytes,
                elem_type
            ),
            TLVNotReadable => write!(f, "the control is not readable."),
            TLVNotWritable => write!(f, "the control is not writable."),
            TLVReadFailed(rc) => write!(f, "snd_ctl_elem_tlv_read failed: {}", rc),
            TVLWriteFailed(rc) => write!(f, "snd_ctl_elem_tlv_write failed: {}", rc),
        }
    }
}

/// TLV struct represents the TLV data to be read from
/// or write to an alsa TLV byte control.
#[derive(Debug)]
pub struct TLV {
    /// data[Self::TYPE_OFFSET] contains the tlv type.
    /// data[Self::LEN_OFFSET] contains the length of the value in bytes.
    /// data[Self::VALUE_OFFSET..] contains the data.
    data: Vec<u32>,
}

impl TLV {
    const TYPE_OFFSET: usize = 0;
    const LEN_OFFSET: usize = 1;
    const VALUE_OFFSET: usize = 2;
    const TLV_HEADER_SIZE_BYTES: usize = Self::VALUE_OFFSET * size_of::<u32>();

    /// Initializes a `TLV` by giving the tlv type and tlv value.
    pub fn new(tlv_type: u32, tlv_value: Vec<u32>) -> Self {
        let mut data = vec![0; 2];
        data[Self::TYPE_OFFSET] = tlv_type;
        data[Self::LEN_OFFSET] = (tlv_value.len() * size_of::<u32>()) as u32;
        data.extend(tlv_value.iter());
        Self { data }
    }

    /// Returns the type of the tlv.
    pub fn tlv_type(&self) -> u32 {
        self.data[Self::TYPE_OFFSET]
    }

    /// Returns the length of the tlv value in dword.
    pub fn len(&self) -> usize {
        self.data[Self::LEN_OFFSET] as usize / size_of::<u32>()
    }

    /// Returns whether the tlv value is empty.
    pub fn is_empty(&self) -> bool {
        self.data[Self::LEN_OFFSET] == 0
    }

    /// Returns the tlv value in slice.
    pub fn value(&self) -> &[u32] {
        &self.data[Self::VALUE_OFFSET..]
    }
}

impl<I: SliceIndex<[u32]>> Index<I> for TLV {
    type Output = I::Output;
    #[inline]
    fn index(&self, index: I) -> &Self::Output {
        &self.data[Self::VALUE_OFFSET..][index]
    }
}

impl<I: SliceIndex<[u32]>> IndexMut<I> for TLV {
    #[inline]
    fn index_mut(&mut self, index: I) -> &mut Self::Output {
        &mut self.data[Self::VALUE_OFFSET..][index]
    }
}

impl TryFrom<Vec<u32>> for TLV {
    type Error = Error;

    /// Constructs a TLV from a vector with the following alsa tlv header validation:
    ///  1 . tlv_buf[Self::LEN_OFFSET] should be multiple of size_of::<u32>
    ///  2 . tlv_buf[Self::LEN_OFFSET] is the length of tlv value in byte and
    ///      should be less than the buffer length * size_of::<u32>.
    fn try_from(data: Vec<u32>) -> Result<Self> {
        if data.len() < 2 {
            return Err(Error::InvalidTLV);
        }

        if data[Self::LEN_OFFSET] % size_of::<u32>() as u32 != 0 {
            return Err(Error::InvalidTLV);
        }

        if data[Self::LEN_OFFSET] / size_of::<u32>() as u32
            > data[Self::VALUE_OFFSET..].len() as u32
        {
            return Err(Error::InvalidTLV);
        }

        Ok(Self { data })
    }
}

impl Into<Vec<u32>> for TLV {
    /// Returns the raw tlv data buffer (including the tlv header).
    fn into(self) -> Vec<u32> {
        self.data
    }
}

/// `ControlTLV` supports read and write of the alsa TLV byte controls.
pub struct ControlTLV<'a> {
    handle: &'a mut Ctl,
    id: ElemId,
}

impl<'a> ControlTLV<'a> {
    /// Called by `Card` to create a `ControlTLV`.
    pub fn new(handle: &'a mut Ctl, id: ElemId) -> Result<Self> {
        let info = ElemInfo::new(handle, &id)?;
        if info.count() % size_of::<u32>() != 0 {
            return Err(Error::InvalidTLVSize(id.name()?.to_owned(), info.count()));
        }
        match info.elem_type()? {
            ElemType::Bytes => Ok(Self { handle, id }),
            _ => Err(Error::InvalidTLVType(
                id.name()?.to_owned(),
                info.elem_type()?,
            )),
        }
    }

    /// Reads data from the byte control by `snd_ctl_elem_tlv_read`
    ///
    /// #
    /// # Errors
    ///
    /// * If it fails to read from the control.
    pub fn load(&mut self) -> Result<TLV> {
        if !ElemInfo::new(self.handle, &self.id)?.tlv_readable() {
            return Err(Error::TLVNotReadable);
        }

        let tlv_size = ElemInfo::new(self.handle, &self.id)?.count() + TLV::TLV_HEADER_SIZE_BYTES;

        let mut tlv_buf = vec![0; tlv_size / size_of::<u32>()];
        // Safe because handle.as_mut_ptr() is a valid *mut snd_ctl_t, id_as_ptr is valid and
        // tlv_buf.as_mut_ptr() is also valid.
        let rc = unsafe {
            alsa_sys::snd_ctl_elem_tlv_read(
                self.handle.as_mut_ptr(),
                self.id.as_ptr(),
                tlv_buf.as_mut_ptr(),
                tlv_size as u32,
            )
        };
        if rc < 0 {
            return Err(Error::TLVReadFailed(rc));
        }
        Ok(TLV::try_from(tlv_buf)?)
    }

    /// Writes to the byte control by `snd_ctl_elem_tlv_write`
    ///
    /// # Results
    ///
    /// * `changed` - false on success.
    ///             - true on success when value was changed.
    /// #
    /// # Errors
    ///
    /// * If it fails to write to the control.
    pub fn save(&mut self, tlv: TLV) -> Result<bool> {
        if !ElemInfo::new(self.handle, &self.id)?.tlv_writable() {
            return Err(Error::TLVNotReadable);
        }
        // Safe because handle.as_mut_ptr() is a valid *mut snd_ctl_t, id_as_ptr is valid and
        // tlv.as_mut_ptr() is also valid.
        let rc = unsafe {
            alsa_sys::snd_ctl_elem_tlv_write(
                self.handle.as_mut_ptr(),
                self.id.as_ptr(),
                Into::<Vec<u32>>::into(tlv).as_mut_ptr(),
            )
        };
        if rc < 0 {
            return Err(Error::TVLWriteFailed(rc));
        }
        Ok(rc > 0)
    }
}

#[cfg(test)]
mod tests {
    use super::*;
    #[test]
    fn test_tlv_try_from_raw_vec() {
        let tlv_buf = vec![0, 12, 2, 3, 4];
        assert!(TLV::try_from(tlv_buf).is_ok());
    }

    #[test]
    fn test_tlv_length_is_not_multiple_of_sizeof_int() {
        // Invalid tlv length in data[Self::LEN_OFFSET].
        let tlv_buf = vec![0, 1, 2, 3, 4];
        assert_eq!(TLV::try_from(tlv_buf).unwrap_err(), Error::InvalidTLV);
    }

    #[test]
    fn test_tlv_length_larger_than_buff_size() {
        // Invalid tlv length in data[Self::LEN_OFFSET].
        let tlv_buf = vec![0, 16, 2, 3, 4];
        assert_eq!(TLV::try_from(tlv_buf).unwrap_err(), Error::InvalidTLV);
    }

    #[test]
    fn test_tlv_length_less_than_two() {
        // tlv buffer length < 2
        let tlv_buf = vec![0];
        assert_eq!(TLV::try_from(tlv_buf).unwrap_err(), Error::InvalidTLV);
    }

    #[test]
    fn test_tlv_length_equal_two() {
        // tlv buffer size = 2.
        let tlv_buf = vec![0, 0];
        assert!(TLV::try_from(tlv_buf).is_ok());
    }
}
