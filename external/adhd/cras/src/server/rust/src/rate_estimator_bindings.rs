// Copyright 2019 The Chromium OS Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

use std::time::Duration;

use libc;

use crate::RateEstimator;

/// # Safety
///
/// To use this function safely, `window_size` must be a valid pointer to a
/// timespec.
#[no_mangle]
pub unsafe extern "C" fn rate_estimator_create(
    rate: libc::c_uint,
    window_size: *const libc::timespec,
    smooth_factor: libc::c_double,
) -> *mut RateEstimator {
    if window_size.is_null() {
        return std::ptr::null_mut::<RateEstimator>();
    }

    let ts = &*window_size;
    let window = Duration::new(ts.tv_sec as u64, ts.tv_nsec as u32);

    match RateEstimator::try_new(rate, window, smooth_factor) {
        Ok(re) => Box::into_raw(Box::new(re)),
        Err(_) => std::ptr::null_mut::<RateEstimator>(),
    }
}

/// # Safety
///
/// To use this function safely, `re` must be a pointer returned from
/// rate_estimator_create, or null.
#[no_mangle]
pub unsafe extern "C" fn rate_estimator_destroy(re: *mut RateEstimator) {
    if re.is_null() {
        return;
    }

    drop(Box::from_raw(re));
}

/// # Safety
///
/// To use this function safely, `re` must be a pointer returned from
/// rate_estimator_create, or null.
#[no_mangle]
pub unsafe extern "C" fn rate_estimator_add_frames(re: *mut RateEstimator, frames: libc::c_int) {
    if re.is_null() {
        return;
    }

    (*re).add_frames(frames)
}

/// # Safety
///
/// To use this function safely, `re` must be a pointer returned from
/// rate_estimator_create, or null, and `now` must be a valid pointer to a
/// timespec.
#[no_mangle]
pub unsafe extern "C" fn rate_estimator_check(
    re: *mut RateEstimator,
    level: libc::c_int,
    now: *const libc::timespec,
) -> i32 {
    if re.is_null() || now.is_null() {
        return 0;
    }

    let ts = &*now;
    if ts.tv_sec < 0 || ts.tv_nsec < 0 {
        return 0;
    }
    let secs = ts.tv_sec as u64 + (ts.tv_nsec / 1_000_000_000) as u64;
    let nsecs = (ts.tv_nsec % 1_000_000_000) as u32;
    let now = Duration::new(secs, nsecs);

    (*re).update_estimated_rate(level, now) as i32
}

/// # Safety
///
/// To use this function safely, `re` must be a pointer returned from
/// rate_estimator_create, or null.
#[no_mangle]
pub unsafe extern "C" fn rate_estimator_get_rate(re: *const RateEstimator) -> libc::c_double {
    if re.is_null() {
        return 0.0;
    }

    (*re).get_estimated_rate()
}

/// # Safety
///
/// To use this function safely, `re` must be a pointer returned from
/// rate_estimator_create, or null.
#[no_mangle]
pub unsafe extern "C" fn rate_estimator_reset_rate(re: *mut RateEstimator, rate: libc::c_uint) {
    if re.is_null() {
        return;
    }

    (*re).reset_rate(rate)
}
