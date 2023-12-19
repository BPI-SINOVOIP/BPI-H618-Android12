// Copyright 2019 The Chromium OS Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
use std::time::Duration;
use std::{error, fmt};

use audio_streams::{
    shm_streams::{BufferSet, ServerRequest, ShmStream},
    BoxError, SampleFormat, StreamDirection,
};
use cras_sys::gen::CRAS_AUDIO_MESSAGE_ID;
use sys_util::error;

use crate::audio_socket::{AudioMessage, AudioSocket};
use crate::cras_server_socket::CrasServerSocket;
use crate::cras_shm::{self, CrasAudioHeader, CrasAudioShmHeaderFd};

#[derive(Debug)]
pub enum Error {
    MessageTypeError,
    CaptureBufferTooSmall,
}

impl error::Error for Error {}

impl fmt::Display for Error {
    fn fmt(&self, f: &mut fmt::Formatter) -> fmt::Result {
        match self {
            Error::MessageTypeError => write!(f, "Message type error"),
            Error::CaptureBufferTooSmall => write!(
                f,
                "Capture buffer too small, must have size at least 'used_size'."
            ),
        }
    }
}

/// An object that handles interactions with CRAS for a shm stream.
/// The object implements `ShmStream` and so can be used to wait for
/// `ServerRequest` and `BufferComplete` messages.
pub struct CrasShmStream<'a> {
    stream_id: u32,
    server_socket: CrasServerSocket,
    audio_socket: AudioSocket,
    direction: StreamDirection,
    header: CrasAudioHeader<'a>,
    frame_size: usize,
    num_channels: usize,
    frame_rate: u32,
    // The index of the next buffer within SHM to set the buffer offset for.
    next_buffer_idx: usize,
}

impl<'a> CrasShmStream<'a> {
    /// Attempt to creates a CrasShmStream with the given arguments.
    ///
    /// # Arguments
    ///
    /// * `stream_id` - The server's ID for the stream.
    /// * `server_socket` - The socket that is connected to the server.
    /// * `audio_socket` - The socket for audio request and audio available messages.
    /// * `direction` - The direction of the stream, `Playback` or `Capture`.
    /// * `num_channels` - The number of audio channels for the stream.
    /// * `format` - The format to use for the stream's samples.
    /// * `header_fd` - The file descriptor for the audio header shm area.
    /// * `samples_len` - The size of the audio samples shm area.
    ///
    /// # Returns
    ///
    /// `CrasShmStream` - CRAS client stream.
    ///
    /// # Errors
    ///
    /// * If `header_fd` could not be successfully mmapped.
    #[allow(clippy::too_many_arguments)]
    pub fn try_new(
        stream_id: u32,
        server_socket: CrasServerSocket,
        audio_socket: AudioSocket,
        direction: StreamDirection,
        num_channels: usize,
        frame_rate: u32,
        format: SampleFormat,
        header_fd: CrasAudioShmHeaderFd,
        samples_len: usize,
    ) -> Result<Self, BoxError> {
        let header = cras_shm::create_header(header_fd, samples_len)?;
        Ok(Self {
            stream_id,
            server_socket,
            audio_socket,
            direction,
            header,
            frame_size: format.sample_bytes() * num_channels,
            num_channels,
            frame_rate,
            // We have either sent zero or two offsets to the server, so we will
            // need to update index 0 next.
            next_buffer_idx: 0,
        })
    }
}

impl<'a> Drop for CrasShmStream<'a> {
    /// Send the disconnect stream message and log an error if sending fails.
    fn drop(&mut self) {
        if let Err(e) = self.server_socket.disconnect_stream(self.stream_id) {
            error!("CrasShmStream::drop error: {}", e);
        }
    }
}

impl<'a> ShmStream for CrasShmStream<'a> {
    fn frame_size(&self) -> usize {
        self.frame_size
    }

    fn num_channels(&self) -> usize {
        self.num_channels
    }

    fn frame_rate(&self) -> u32 {
        self.frame_rate
    }

    fn wait_for_next_action_with_timeout(
        &mut self,
        timeout: Duration,
    ) -> Result<Option<ServerRequest>, BoxError> {
        let expected_id = match self.direction {
            StreamDirection::Playback => CRAS_AUDIO_MESSAGE_ID::AUDIO_MESSAGE_REQUEST_DATA,
            StreamDirection::Capture => CRAS_AUDIO_MESSAGE_ID::AUDIO_MESSAGE_DATA_READY,
        };

        match self
            .audio_socket
            .read_audio_message_with_timeout(Some(timeout))?
        {
            Some(AudioMessage::Success { id, frames }) if id == expected_id => {
                Ok(Some(ServerRequest::new(frames as usize, self)))
            }
            None => Ok(None),
            _ => Err(Box::new(Error::MessageTypeError)),
        }
    }
}

impl BufferSet for CrasShmStream<'_> {
    fn callback(&mut self, offset: usize, frames: usize) -> Result<(), BoxError> {
        self.header
            .set_buffer_offset(self.next_buffer_idx, offset)?;
        self.next_buffer_idx ^= 1;
        let frames = frames as u32;

        match self.direction {
            StreamDirection::Playback => {
                self.header.commit_written_frames(frames)?;

                // Notify CRAS that we've made playback data available.
                self.audio_socket.data_ready(frames)?
            }
            StreamDirection::Capture => {
                let used_size = self.header.get_used_size();
                // Because CRAS doesn't know how long our buffer in shm is, we
                // must make sure that there are always at least buffer_size
                // frames available so that it doesn't write outside the buffer.
                if frames < (used_size / self.frame_size) as u32 {
                    return Err(Box::new(Error::CaptureBufferTooSmall));
                }

                self.header.commit_read_frames(frames)?;
                self.audio_socket.capture_ready(frames)?;
            }
        }

        Ok(())
    }

    fn ignore(&mut self) -> Result<(), BoxError> {
        // We send an empty buffer for an ignored playback request since the
        // server will not read from a 0-length buffer. We don't do anything for
        // an ignored capture request, since we don't have a way to communicate
        // buffer length to the server, and we don't want the server writing
        // data to offsets within the SHM area that aren't audio buffers.
        if self.direction == StreamDirection::Playback {
            self.callback(0, 0)?;
        }

        Ok(())
    }
}
