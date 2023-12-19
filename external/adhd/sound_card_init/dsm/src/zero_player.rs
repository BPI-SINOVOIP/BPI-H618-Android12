// Copyright 2020 The Chromium OS Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
use std::io::Write;
use std::sync::atomic::{AtomicBool, Ordering};
use std::sync::{Arc, Condvar, Mutex};
use std::thread;
use std::thread::JoinHandle;
use std::time::Duration;

use audio_streams::SampleFormat;
use libcras::{CrasClient, CrasNodeType};
use sys_util::error;

use crate::error::{Error, Result};

/// `ZeroPlayer` provides the functionality to play zeros sample in the background thread.
#[derive(Default)]
pub struct ZeroPlayer {
    thread_info: Option<PlayZeroWorkerInfo>,
}

impl Drop for ZeroPlayer {
    fn drop(&mut self) {
        if self.thread_info.is_some() {
            if let Err(e) = self.stop() {
                error!("{}", e);
            }
        }
    }
}

impl ZeroPlayer {
    /// It takes about 400 ms to get CRAS_NODE_TYPE_INTERNAL_SPEAKER during the boot time.
    const TIMEOUT: Duration = Duration::from_millis(1000);

    /// Returns whether the ZeroPlayer is running.
    pub fn running(&self) -> bool {
        self.thread_info.is_some()
    }

    /// Starts to play zeros for at most `max_playback_time`.
    /// This function blocks and returns until playback has started for `min_playback_time`.
    /// This function must be called when self.running() returns false.
    ///
    /// # Arguments
    ///
    /// * `min_playback_time` - It blocks and returns until playback has started for
    ///                         `min_playback_time`.
    ///
    /// # Errors
    ///
    /// * If it's called when the `ZeroPlayer` is already running.
    /// * Failed to find internal speakers.
    /// * Failed to start the background thread.
    pub fn start(&mut self, min_playback_time: Duration) -> Result<()> {
        if self.running() {
            return Err(Error::ZeroPlayerIsRunning);
        }
        self.thread_info = Some(PlayZeroWorkerInfo::new(min_playback_time));
        if let Some(thread_info) = &mut self.thread_info {
            // Block until playback of zeros has started for min_playback_time or timeout.
            let (lock, cvar) = &*(thread_info.ready);
            let result = cvar.wait_timeout_while(
                lock.lock()?,
                min_playback_time + ZeroPlayer::TIMEOUT,
                |&mut is_ready| !is_ready,
            )?;
            if result.1.timed_out() {
                return Err(Error::StartPlaybackTimeout);
            }
        }
        Ok(())
    }

    /// Stops playing zeros in the background thread.
    /// This function must be called when self.running() returns true.
    ///
    /// # Errors
    ///
    /// * If it's called again when the `ZeroPlayer` is not running.
    /// * Failed to play zeros to internal speakers via CRAS client.
    /// * Failed to join the background thread.
    pub fn stop(&mut self) -> Result<()> {
        match self.thread_info.take() {
            Some(mut thread_info) => Ok(thread_info.destroy()?),
            None => Err(Error::ZeroPlayerIsNotRunning),
        }
    }
}

// Audio thread book-keeping data
struct PlayZeroWorkerInfo {
    thread: Option<JoinHandle<Result<()>>>,
    // Uses `thread_run` to notify the background thread to stop.
    thread_run: Arc<AtomicBool>,
    // The background thread uses `ready` to notify the main thread that playback
    // of zeros has started for min_playback_time.
    ready: Arc<(Mutex<bool>, Condvar)>,
}

impl Drop for PlayZeroWorkerInfo {
    fn drop(&mut self) {
        if let Err(e) = self.destroy() {
            error!("{}", e);
        }
    }
}

impl PlayZeroWorkerInfo {
    // Spawns the PlayZeroWorker.
    fn new(min_playback_time: Duration) -> Self {
        let thread_run = Arc::new(AtomicBool::new(false));
        let ready = Arc::new((Mutex::new(false), Condvar::new()));
        let mut worker = PlayZeroWorker::new(min_playback_time, thread_run.clone(), ready.clone());
        Self {
            thread: Some(thread::spawn(move || -> Result<()> {
                worker.run()?;
                Ok(())
            })),
            thread_run,
            ready,
        }
    }

    // Joins the PlayZeroWorker.
    fn destroy(&mut self) -> Result<()> {
        self.thread_run.store(false, Ordering::Relaxed);
        if let Some(handle) = self.thread.take() {
            let res = handle.join().map_err(Error::WorkerPanics)?;
            return match res {
                Err(e) => Err(e),
                Ok(_) => Ok(()),
            };
        }
        Ok(())
    }
}

struct PlayZeroWorker {
    min_playback_time: Duration,
    // Uses `thread_run` to notify the background thread to stop.
    thread_run: Arc<AtomicBool>,
    // The background thread uses `ready` to notify the main thread that playback
    // of zeros has started for min_playback_time.
    ready: Arc<(Mutex<bool>, Condvar)>,
}

impl PlayZeroWorker {
    const FRAMES_PER_BUFFER: usize = 256;
    const FRAME_RATE: u32 = 48000;
    const NUM_CHANNELS: usize = 2;
    const FORMAT: SampleFormat = SampleFormat::S16LE;

    fn new(
        min_playback_time: Duration,
        thread_run: Arc<AtomicBool>,
        ready: Arc<(Mutex<bool>, Condvar)>,
    ) -> Self {
        Self {
            min_playback_time,
            thread_run,
            ready,
        }
    }

    fn run(&mut self) -> Result<()> {
        let mut cras_client = CrasClient::new().map_err(Error::CrasClientFailed)?;
        // TODO(b/155007305): Implement cras_client.wait_node_change and use it here.
        let node = cras_client
            .output_nodes()
            .find(|node| node.node_type == CrasNodeType::CRAS_NODE_TYPE_INTERNAL_SPEAKER)
            .ok_or(Error::InternalSpeakerNotFound)?;
        let local_buffer =
            vec![0u8; Self::FRAMES_PER_BUFFER * Self::NUM_CHANNELS * Self::FORMAT.sample_bytes()];
        let min_playback_iterations = (Self::FRAME_RATE
            * self.min_playback_time.as_millis() as u32)
            / Self::FRAMES_PER_BUFFER as u32
            / 1000;
        let (_control, mut stream) = cras_client
            .new_pinned_playback_stream(
                node.iodev_index,
                Self::NUM_CHANNELS,
                Self::FORMAT,
                Self::FRAME_RATE,
                Self::FRAMES_PER_BUFFER,
            )
            .map_err(|e| Error::NewPlayStreamFailed(e))?;

        let mut iter = 0;
        self.thread_run.store(true, Ordering::Relaxed);
        while self.thread_run.load(Ordering::Relaxed) {
            let mut buffer = stream
                .next_playback_buffer()
                .map_err(|e| Error::NextPlaybackBufferFailed(e))?;
            let _write_frames = buffer.write(&local_buffer).map_err(Error::PlaybackFailed)?;

            // Notifies the main thread that playback of zeros has started for min_playback_time.
            if iter == min_playback_iterations {
                let (lock, cvar) = &*self.ready;
                let mut is_ready = lock.lock()?;
                *is_ready = true;
                cvar.notify_one();
            }
            iter += 1;
        }
        Ok(())
    }
}
