// Copyright 2018 The Chromium OS Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

use std::ffi::CStr;

use log::{self, Level, LevelFilter, Metadata, Record, SetLoggerError};

static LOGGER: SyslogLogger = SyslogLogger;

struct SyslogLogger;

impl log::Log for SyslogLogger {
    fn enabled(&self, metadata: &Metadata) -> bool {
        if cfg!(debug_assertions) {
            metadata.level() <= Level::Debug
        } else {
            metadata.level() <= Level::Info
        }
    }

    fn log(&self, record: &Record) {
        if !self.enabled(&record.metadata()) {
            return;
        }

        let level = match record.level() {
            Level::Error => libc::LOG_ERR,
            Level::Warn => libc::LOG_WARNING,
            Level::Info => libc::LOG_INFO,
            Level::Debug => libc::LOG_DEBUG,
            Level::Trace => libc::LOG_DEBUG,
        };

        let msg = format!("{}\0", record.args());
        let cmsg = if let Ok(m) = CStr::from_bytes_with_nul(msg.as_bytes()) {
            m
        } else {
            // For now we just drop messages with interior nuls.
            return;
        };

        // Safe because this doesn't modify any memory.  There's not much use
        // in checking the return value because this _is_ the logging function
        // so there's no way for us to tell anyone about the error.
        unsafe {
            libc::syslog(level, cmsg.as_ptr());
        }
    }
    fn flush(&self) {}
}

/// Initializes the logger to send log messages to syslog.
pub fn init(ident: &'static CStr) -> Result<(), SetLoggerError> {
    // Safe because this only modifies libc's internal state and is safe to call
    // multiple times.
    unsafe {
        libc::openlog(
            ident.as_ptr(),
            libc::LOG_NDELAY | libc::LOG_PID,
            libc::LOG_USER,
        )
    };
    log::set_logger(&LOGGER)?;
    let level = if cfg!(debug_assertions) {
        LevelFilter::Debug
    } else {
        LevelFilter::Info
    };
    log::set_max_level(level);

    Ok(())
}
