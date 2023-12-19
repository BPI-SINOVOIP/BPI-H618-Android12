// Copyright 2020 The Chromium OS Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

/// Provides safe implementations of common low level functions that assume a Linux environment.
use libc::{syscall, SYS_gettid};

pub type Pid = libc::pid_t;

pub fn getpid() -> Pid {
    // Calling getpid() is always safe.
    unsafe { libc::getpid() }
}

pub fn gettid() -> Pid {
    // Calling the gettid() sycall is always safe.
    unsafe { syscall(SYS_gettid) as Pid }
}
