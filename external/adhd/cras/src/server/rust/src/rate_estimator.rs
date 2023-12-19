// Copyright 2019 The Chromium OS Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

pub mod rate_estimator_bindings;

use std::error;
use std::fmt;
use std::time::Duration;

#[derive(Debug)]
pub enum Error {
    InvalidSmoothFactor(f64),
}

impl error::Error for Error {}

impl fmt::Display for Error {
    fn fmt(&self, f: &mut fmt::Formatter) -> fmt::Result {
        use Error::*;
        match self {
            InvalidSmoothFactor(sf) => write!(f, "Smooth factor {} is not between 0.0 and 1.0", sf),
        }
    }
}

type Result<T> = std::result::Result<T, Error>;

const MAX_RATE_SKEW: f64 = 100.0;

/// Hold information to calculate linear least square from
/// several (x, y) samples.
#[derive(Debug, Default)]
struct LeastSquares {
    sum_x: f64,
    sum_y: f64,
    sum_xy: f64,
    sum_x2: f64,
    num_samples: u32,
}

impl LeastSquares {
    fn new() -> Self {
        Self::default()
    }

    fn add_sample(&mut self, x: f64, y: f64) {
        self.sum_x += x;
        self.sum_y += y;
        self.sum_xy += x * y;
        self.sum_x2 += x * x;
        self.num_samples += 1;
    }

    fn best_fit_slope(&self) -> f64 {
        let num = self.num_samples as f64 * self.sum_xy - self.sum_x * self.sum_y;
        let den = self.num_samples as f64 * self.sum_x2 - self.sum_x * self.sum_x;
        num / den
    }
}

/// An estimator holding the required information to determine the actual frame
/// rate of an audio device.
///
/// # Members
///    * `last_level` - Buffer level of the audio device at last check time.
///    * `level_diff` - Number of frames written to or read from audio device
///                     since the last check time. Rate estimator will use this
///                     change plus the difference of buffer level to derive the
///                     number of frames audio device has actually processed.
///    * `window_start` - The start time of the current window.
///    * `window_size` - The size of the window.
///    * `window_frames` - The number of frames accumulated in current window.
///    * `lsq` - The helper used to estimate sample rate.
///    * `smooth_factor` - A scaling factor used to average the previous and new
///                        rate estimates to ensure that estimates do not change
///                        too quickly.
///    * `estimated_rate` - The estimated rate at which samples are consumed.
pub struct RateEstimator {
    last_level: i32,
    level_diff: i32,
    window_start: Option<Duration>,
    window_size: Duration,
    window_frames: u32,
    lsq: LeastSquares,
    smooth_factor: f64,
    estimated_rate: f64,
}

impl RateEstimator {
    /// Creates a rate estimator.
    ///
    /// # Arguments
    ///    * `rate` - The initial value to estimate rate from.
    ///    * `window_size` - The window size of the rate estimator.
    ///    * `smooth_factor` - The coefficient used to calculate moving average
    ///                        from old estimated rate values. Must be between
    ///                        0.0 and 1.0
    ///
    /// # Errors
    ///    * If `smooth_factor` is not between 0.0 and 1.0
    pub fn try_new(rate: u32, window_size: Duration, smooth_factor: f64) -> Result<Self> {
        if smooth_factor < 0.0 || smooth_factor > 1.0 {
            return Err(Error::InvalidSmoothFactor(smooth_factor));
        }

        Ok(RateEstimator {
            last_level: 0,
            level_diff: 0,
            window_start: None,
            window_size,
            window_frames: 0,
            lsq: LeastSquares::new(),
            smooth_factor,
            estimated_rate: rate as f64,
        })
    }

    /// Resets the estimated rate
    ///
    /// Reset the estimated rate to `rate`, and erase all collected data.
    pub fn reset_rate(&mut self, rate: u32) {
        self.last_level = 0;
        self.level_diff = 0;
        self.window_start = None;
        self.window_frames = 0;
        self.lsq = LeastSquares::new();
        self.estimated_rate = rate as f64;
    }

    /// Adds additional frames transmitted to/from audio device.
    ///
    /// # Arguments
    ///    * `frames` - The number of frames written to the device.  For input,
    ///                 this should be negative to indicate how many samples
    ///                 were read.
    pub fn add_frames(&mut self, frames: i32) {
        self.level_diff += frames;
    }

    /// Gets the estimated rate.
    pub fn get_estimated_rate(&self) -> f64 {
        self.estimated_rate
    }

    /// Check the timestamp and buffer level difference since last check time,
    /// and use them as a new sample to update the estimated rate.
    ///
    /// # Arguments
    ///    * `level` - The current buffer level of audio device.
    ///    * `now` - The time at which this function is called.
    ///
    /// # Returns
    ///    True if the estimated rate is updated and window is reset,
    ///    otherwise false.
    pub fn update_estimated_rate(&mut self, level: i32, now: Duration) -> bool {
        let start = match self.window_start {
            None => {
                self.window_start = Some(now);
                return false;
            }
            Some(t) => t,
        };

        let delta = match now.checked_sub(start) {
            Some(d) => d,
            None => return false,
        };
        self.window_frames += (self.last_level - level + self.level_diff).abs() as u32;
        self.level_diff = 0;
        self.last_level = level;

        let secs = (delta.as_secs() as f64) + delta.subsec_nanos() as f64 / 1_000_000_000.0;
        self.lsq.add_sample(secs, self.window_frames as f64);
        if delta > self.window_size && self.lsq.num_samples > 1 {
            let rate = self.lsq.best_fit_slope();
            if (self.estimated_rate - rate).abs() < MAX_RATE_SKEW {
                self.estimated_rate =
                    rate * (1.0 - self.smooth_factor) + self.estimated_rate * self.smooth_factor;
            }
            self.lsq = LeastSquares::new();
            self.window_start = Some(now);
            self.window_frames = 0;
            return true;
        }
        false
    }
}
