// Copyright 2019 The Chromium OS Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

pub mod linux;
pub mod net;
mod read_dir;
pub mod scoped_path;
pub mod sync;
pub mod syslog;
pub mod vsock;

pub use read_dir::*;

#[macro_export]
macro_rules! syscall {
    ($e:expr) => {{
        let res = $e;
        if res < 0 {
            Err(::std::io::Error::last_os_error())
        } else {
            Ok(res)
        }
    }};
}
