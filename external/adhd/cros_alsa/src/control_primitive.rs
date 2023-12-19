// Copyright 2020 The Chromium OS Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

use std::convert::TryFrom;
use std::error;
use std::ffi::{CStr, CString, FromBytesWithNulError, NulError};
use std::fmt;
use std::marker::PhantomData;
use std::ptr;
use std::slice;
use std::str;

use alsa_sys::*;
use libc::strlen;
use remain::sorted;

pub type Result<T> = std::result::Result<T, Error>;

#[derive(Debug, PartialEq)]
/// Possible errors that can occur in FFI functions.
pub enum FFIError {
    Rc(i32),
    NullPtr,
}

impl fmt::Display for FFIError {
    fn fmt(&self, f: &mut fmt::Formatter) -> fmt::Result {
        use FFIError::*;
        match self {
            Rc(rc) => write!(f, "{}", snd_strerror(*rc)?),
            NullPtr => write!(f, "the return value is a null pointer"),
        }
    }
}

#[sorted]
#[derive(Debug, PartialEq)]
/// Possible errors that can occur in cros-alsa::control_primitive.
pub enum Error {
    /// Control with the given name does not exist.
    ControlNotFound(String),
    /// Failed to call snd_ctl_open().
    CtlOpenFailed(FFIError, String),
    /// snd_ctl_elem_id_get_name() returns null.
    ElemIdGetNameFailed,
    /// Failed to call snd_ctl_elem_id_malloc().
    ElemIdMallocFailed(FFIError),
    /// Failed to call snd_ctl_elem_info_malloc().
    ElemInfoMallocFailed(FFIError),
    /// Failed to call snd_ctl_elem_value_malloc().
    ElemValueMallocFailed(FFIError),
    /// The slice used to create a CStr does not have one and only one null
    /// byte positioned at the end.
    FromBytesWithNulError(FromBytesWithNulError),
    /// Failed to convert to a valid ElemType.
    InvalidElemType(u32),
    /// An error indicating that an interior nul byte was found.
    NulError(NulError),
    /// Failed to call snd_strerror().
    SndStrErrorFailed(i32),
    /// UTF-8 validation failed
    Utf8Error(str::Utf8Error),
}

impl error::Error for Error {}

impl fmt::Display for Error {
    fn fmt(&self, f: &mut fmt::Formatter) -> fmt::Result {
        use Error::*;
        match self {
            ControlNotFound(name) => write!(f, "control: {} does not exist", name),
            CtlOpenFailed(e, name) => write!(f, "{} snd_ctl_open failed: {}", name, e,),
            ElemIdGetNameFailed => write!(f, "snd_ctl_elem_id_get_name failed"),
            ElemIdMallocFailed(e) => write!(f, "snd_ctl_elem_id_malloc failed: {}", e),
            ElemInfoMallocFailed(e) => write!(f, "snd_ctl_elem_info_malloc failed: {}", e),
            ElemValueMallocFailed(e) => write!(f, "snd_ctl_elem_value_malloc failed: {}", e),
            FromBytesWithNulError(e) => write!(f, "invalid CString: {}", e),
            InvalidElemType(v) => write!(f, "invalid ElemType: {}", v),
            NulError(e) => write!(f, "invalid CString: {}", e),
            SndStrErrorFailed(e) => write!(f, "snd_strerror() failed: {}", e),
            Utf8Error(e) => write!(f, "{}", e),
        }
    }
}

impl From<Error> for fmt::Error {
    fn from(_err: Error) -> fmt::Error {
        fmt::Error
    }
}

impl From<str::Utf8Error> for Error {
    fn from(err: str::Utf8Error) -> Error {
        Error::Utf8Error(err)
    }
}

impl From<FromBytesWithNulError> for Error {
    fn from(err: FromBytesWithNulError) -> Error {
        Error::FromBytesWithNulError(err)
    }
}

impl From<NulError> for Error {
    fn from(err: NulError) -> Error {
        Error::NulError(err)
    }
}

/// [snd_ctl_elem_iface_t](https://www.alsa-project.org/alsa-doc/alsa-lib/group___control.html#ga14baa0febb91cc4c5d72dcc825acf518) wrapper.
#[derive(Debug, Copy, Clone, PartialEq, Eq, PartialOrd, Ord, Hash)]
pub enum ElemIface {
    Card = SND_CTL_ELEM_IFACE_CARD as isize,
    Hwdep = SND_CTL_ELEM_IFACE_HWDEP as isize,
    Mixer = SND_CTL_ELEM_IFACE_MIXER as isize,
    PCM = SND_CTL_ELEM_IFACE_PCM as isize,
    Rawmidi = SND_CTL_ELEM_IFACE_RAWMIDI as isize,
    Timer = SND_CTL_ELEM_IFACE_TIMER as isize,
    Sequencer = SND_CTL_ELEM_IFACE_SEQUENCER as isize,
}

#[derive(Debug, Copy, Clone, PartialEq, Eq)]
/// [snd_ctl_elem_type_t](https://www.alsa-project.org/alsa-doc/alsa-lib/group___control.html#gac42e0ed6713b62711af5e80b4b3bcfec) wrapper.
pub enum ElemType {
    None = SND_CTL_ELEM_TYPE_NONE as isize,
    Boolean = SND_CTL_ELEM_TYPE_BOOLEAN as isize,
    Integer = SND_CTL_ELEM_TYPE_INTEGER as isize,
    Enumerated = SND_CTL_ELEM_TYPE_ENUMERATED as isize,
    Bytes = SND_CTL_ELEM_TYPE_BYTES as isize,
    IEC958 = SND_CTL_ELEM_TYPE_IEC958 as isize,
    Integer64 = SND_CTL_ELEM_TYPE_INTEGER64 as isize,
}

impl fmt::Display for ElemType {
    fn fmt(&self, f: &mut fmt::Formatter<'_>) -> fmt::Result {
        match self {
            ElemType::None => write!(f, "SND_CTL_ELEM_TYPE_NONE"),
            ElemType::Boolean => write!(f, "SND_CTL_ELEM_TYPE_BOOLEAN"),
            ElemType::Integer => write!(f, "SND_CTL_ELEM_TYPE_INTEGER"),
            ElemType::Enumerated => write!(f, "SND_CTL_ELEM_TYPE_ENUMERATED"),
            ElemType::Bytes => write!(f, "SND_CTL_ELEM_TYPE_BYTES"),
            ElemType::IEC958 => write!(f, "SND_CTL_ELEM_TYPE_IEC958"),
            ElemType::Integer64 => write!(f, "SND_CTL_ELEM_TYPE_INTEGER64"),
        }
    }
}

impl TryFrom<u32> for ElemType {
    type Error = Error;
    fn try_from(elem_type: u32) -> Result<ElemType> {
        match elem_type {
            SND_CTL_ELEM_TYPE_NONE => Ok(ElemType::None),
            SND_CTL_ELEM_TYPE_BOOLEAN => Ok(ElemType::Boolean),
            SND_CTL_ELEM_TYPE_INTEGER => Ok(ElemType::Integer),
            SND_CTL_ELEM_TYPE_ENUMERATED => Ok(ElemType::Enumerated),
            SND_CTL_ELEM_TYPE_BYTES => Ok(ElemType::Bytes),
            SND_CTL_ELEM_TYPE_IEC958 => Ok(ElemType::IEC958),
            SND_CTL_ELEM_TYPE_INTEGER64 => Ok(ElemType::Integer64),
            _ => Err(Error::InvalidElemType(elem_type)),
        }
    }
}

/// [snd_ctl_elem_id_t](https://www.alsa-project.org/alsa-doc/alsa-lib/group___control.html#gad6c3746f1925bfec6a4fd0e913430e55) wrapper.
pub struct ElemId(
    ptr::NonNull<snd_ctl_elem_id_t>,
    PhantomData<snd_ctl_elem_id_t>,
);

impl Drop for ElemId {
    fn drop(&mut self) {
        // Safe because self.0.as_ptr() is a valid snd_ctl_elem_id_t*.
        unsafe { snd_ctl_elem_id_free(self.0.as_ptr()) };
    }
}

impl ElemId {
    /// Creates an `ElemId` object by `ElemIface` and name.
    ///
    /// # Errors
    ///
    /// * If memory allocation fails.
    /// * If ctl_name is not a valid CString.
    pub fn new(iface: ElemIface, ctl_name: &str) -> Result<ElemId> {
        let mut id_ptr = ptr::null_mut();
        // Safe because we provide a valid id_ptr to be filled,
        // and we validate the return code before using id_ptr.
        let rc = unsafe { snd_ctl_elem_id_malloc(&mut id_ptr) };
        if rc < 0 {
            return Err(Error::ElemIdMallocFailed(FFIError::Rc(rc)));
        }
        let id = ptr::NonNull::new(id_ptr).ok_or(Error::ElemIdMallocFailed(FFIError::NullPtr))?;

        // Safe because id.as_ptr() is a valid snd_ctl_elem_id_t*.
        unsafe { snd_ctl_elem_id_set_interface(id.as_ptr(), iface as u32) };
        let name = CString::new(ctl_name)?;
        // Safe because id.as_ptr() is a valid snd_ctl_elem_id_t* and name is a safe CString.
        unsafe { snd_ctl_elem_id_set_name(id.as_ptr(), name.as_ptr()) };
        Ok(ElemId(id, PhantomData))
    }

    /// Borrows the const inner pointer.
    pub fn as_ptr(&self) -> *const snd_ctl_elem_id_t {
        self.0.as_ptr()
    }

    /// Safe [snd_ctl_elem_id_get_name()] (https://www.alsa-project.org/alsa-doc/alsa-lib/group___control.html#gaa6cfea3ac963bfdaeb8189e03e927a76) wrapper.
    ///
    /// # Errors
    ///
    /// * If snd_ctl_elem_id_get_name() fails.
    /// * If control element name is not a valid CString.
    /// * If control element name is not valid UTF-8 data.
    pub fn name(&self) -> Result<&str> {
        // Safe because self.as_ptr() is a valid snd_ctl_elem_id_t*.
        let name = unsafe { snd_ctl_elem_id_get_name(self.as_ptr()) };
        if name.is_null() {
            return Err(Error::ElemIdGetNameFailed);
        }
        // Safe because name is a valid *const i8, and its life time
        // is the same as the passed reference of self.
        let s = CStr::from_bytes_with_nul(unsafe {
            slice::from_raw_parts(name as *const u8, strlen(name) + 1)
        })?;
        Ok(s.to_str()?)
    }
}

/// [snd_ctl_elem_value_t](https://www.alsa-project.org/alsa-doc/alsa-lib/group___control.html#ga266b478eb64f1cdd75e337df4b4b995e) wrapper.
pub struct ElemValue(
    ptr::NonNull<snd_ctl_elem_value_t>,
    PhantomData<snd_ctl_elem_value_t>,
);

impl Drop for ElemValue {
    // Safe because self.0.as_ptr() is valid.
    fn drop(&mut self) {
        unsafe { snd_ctl_elem_value_free(self.0.as_ptr()) };
    }
}

impl ElemValue {
    /// Creates an `ElemValue`.
    ///
    /// # Errors
    ///
    /// * If memory allocation fails.
    pub fn new(id: &ElemId) -> Result<ElemValue> {
        let mut v_ptr = ptr::null_mut();
        // Safe because we provide a valid v_ptr to be filled,
        // and we validate the return code before using v_ptr.
        let rc = unsafe { snd_ctl_elem_value_malloc(&mut v_ptr) };
        if rc < 0 {
            return Err(Error::ElemValueMallocFailed(FFIError::Rc(rc)));
        }
        let value =
            ptr::NonNull::new(v_ptr).ok_or(Error::ElemValueMallocFailed(FFIError::NullPtr))?;
        // Safe because value.as_ptr() is a valid snd_ctl_elem_value_t* and id.as_ptr() is also valid.
        unsafe { snd_ctl_elem_value_set_id(value.as_ptr(), id.as_ptr()) };
        Ok(ElemValue(value, PhantomData))
    }

    /// Borrows the mutable inner pointer.
    pub fn as_mut_ptr(&mut self) -> *mut snd_ctl_elem_value_t {
        self.0.as_ptr()
    }

    /// Borrows the const inner pointer.
    pub fn as_ptr(&self) -> *const snd_ctl_elem_value_t {
        self.0.as_ptr()
    }
}

/// [snd_ctl_elem_info_t](https://www.alsa-project.org/alsa-doc/alsa-lib/group___control.html#ga2cae0bb76df919368e4ff9a7021dd3ab) wrapper.
pub struct ElemInfo(
    ptr::NonNull<snd_ctl_elem_info_t>,
    PhantomData<snd_ctl_elem_info_t>,
);

impl Drop for ElemInfo {
    fn drop(&mut self) {
        // Safe because self.0.as_ptr() is a valid snd_ctl_elem_info_t*.
        unsafe { snd_ctl_elem_info_free(self.0.as_ptr()) };
    }
}

impl ElemInfo {
    /// Creates an `ElemInfo`.
    ///
    /// # Errors
    ///
    /// * If memory allocation fails.
    /// * If control does not exist.
    pub fn new(handle: &mut Ctl, id: &ElemId) -> Result<ElemInfo> {
        let mut info_ptr = ptr::null_mut();

        // Safe because we provide a valid info_ptr to be filled,
        // and we validate the return code before using info_ptr.
        let rc = unsafe { snd_ctl_elem_info_malloc(&mut info_ptr) };
        if rc < 0 {
            return Err(Error::ElemInfoMallocFailed(FFIError::Rc(rc)));
        }
        let info =
            ptr::NonNull::new(info_ptr).ok_or(Error::ElemInfoMallocFailed(FFIError::NullPtr))?;

        // Safe because info.as_ptr() is a valid snd_ctl_elem_info_t* and id.as_ptr() is also valid.
        unsafe { snd_ctl_elem_info_set_id(info.as_ptr(), id.as_ptr()) };

        // Safe because handle.as_mut_ptr() is a valid snd_ctl_t* and info.as_ptr() is a valid
        // snd_ctl_elem_info_t*.
        let rc = unsafe { snd_ctl_elem_info(handle.as_mut_ptr(), info.as_ptr()) };
        if rc < 0 {
            return Err(Error::ControlNotFound(id.name()?.to_owned()));
        }
        Ok(ElemInfo(info, PhantomData))
    }

    /// Safe [snd_ctl_elem_info_get_type](https://www.alsa-project.org/alsa-doc/alsa-lib/group___control.html#ga0fec5d22ee58d04f14b59f405adc595e) wrapper.
    pub fn elem_type(&self) -> Result<ElemType> {
        // Safe because self.0.as_ptr() is a valid snd_ctl_elem_info_t*.
        unsafe { ElemType::try_from(snd_ctl_elem_info_get_type(self.0.as_ptr())) }
    }

    /// Safe [snd_ctl_elem_info_get_count](https://www.alsa-project.org/alsa-doc/alsa-lib/group___control.html#gaa75a20d4190d324bcda5fd6659a4b377) wrapper.
    pub fn count(&self) -> usize {
        // Safe because self.0.as_ptr() is a valid snd_ctl_elem_info_t*.
        unsafe { snd_ctl_elem_info_get_count(self.0.as_ptr()) as usize }
    }

    /// Safe [snd_ctl_elem_info_is_tlv_readable](https://www.alsa-project.org/alsa-doc/alsa-lib/group___control.html#gaac6bb412e5a9fffb5509e98a10de45b5) wrapper.
    pub fn tlv_readable(&self) -> bool {
        // Safe because self.0.as_ptr() is a valid snd_ctl_elem_info_t*.
        unsafe { snd_ctl_elem_info_is_tlv_readable(self.0.as_ptr()) as usize == 1 }
    }

    /// Safe [snd_ctl_elem_info_is_tlv_writable](https://www.alsa-project.org/alsa-doc/alsa-lib/group___control.html#gacfbaae80d710b6feac682f8ba10a0341) wrapper.
    pub fn tlv_writable(&self) -> bool {
        // Safe because self.0.as_ptr() is a valid snd_ctl_elem_info_t*.
        unsafe { snd_ctl_elem_info_is_tlv_writable(self.0.as_ptr()) as usize == 1 }
    }
}

/// [snd_ctl_t](https://www.alsa-project.org/alsa-doc/alsa-lib/group___control.html#ga06628f38def84a0fe3da74041db9d51f) wrapper.
#[derive(Debug)]
pub struct Ctl(ptr::NonNull<snd_ctl_t>, PhantomData<snd_ctl_t>);

impl Drop for Ctl {
    fn drop(&mut self) {
        // Safe as we provide a valid snd_ctl_t*.
        unsafe { snd_ctl_close(self.0.as_ptr()) };
    }
}

impl Ctl {
    /// Creates a `Ctl`.
    /// Safe [snd_ctl_open](https://www.alsa-project.org/alsa-doc/alsa-lib/group___control.html#ga58537f5b74c9c1f51699f9908a0d7f56).
    /// Does not support async mode.
    ///
    /// # Errors
    ///
    /// * If `card` is an invalid CString.
    /// * If `snd_ctl_open()` fails.
    pub fn new(card: &str) -> Result<Ctl> {
        let name = CString::new(card)?;
        let mut ctl_ptr = ptr::null_mut();
        // Safe because we provide a valid ctl_ptr to be filled, name is a safe CString
        // and we validate the return code before using ctl_ptr.
        let rc = unsafe { snd_ctl_open(&mut ctl_ptr, name.as_ptr(), 0) };
        if rc < 0 {
            return Err(Error::CtlOpenFailed(
                FFIError::Rc(rc),
                name.to_str()?.to_owned(),
            ));
        }
        let ctl = ptr::NonNull::new(ctl_ptr).ok_or(Error::CtlOpenFailed(
            FFIError::NullPtr,
            name.to_str()?.to_owned(),
        ))?;
        Ok(Ctl(ctl, PhantomData))
    }

    /// Borrows the mutable inner pointer
    pub fn as_mut_ptr(&mut self) -> *mut snd_ctl_t {
        self.0.as_ptr()
    }
}

/// Safe [snd_strerror](https://www.alsa-project.org/alsa-doc/alsa-lib/group___error.html#ga182bbadf2349e11602bc531e8cf22f7e) wrapper.
///
/// # Errors
///
/// * If `snd_strerror` returns invalid UTF-8 data.
pub fn snd_strerror(err_num: i32) -> Result<&'static str> {
    // Safe because we validate the return pointer of snd_strerror()
    // before using it.
    let s_ptr = unsafe { alsa_sys::snd_strerror(err_num) };
    if s_ptr.is_null() {
        return Err(Error::SndStrErrorFailed(err_num));
    }
    // Safe because s_ptr is a non-null *const u8 and its lifetime is static.
    let s = CStr::from_bytes_with_nul(unsafe {
        slice::from_raw_parts(s_ptr as *const u8, strlen(s_ptr) + 1)
    })?;
    Ok(s.to_str()?)
}
