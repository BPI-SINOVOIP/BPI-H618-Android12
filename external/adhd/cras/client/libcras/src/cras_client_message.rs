// Copyright 2019 The Chromium OS Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
use std::{array::TryFromSliceError, convert::TryInto, error, fmt, io, mem, os::unix::io::RawFd};

use cras_sys::gen::{
    cras_client_connected, cras_client_message, cras_client_stream_connected,
    CRAS_CLIENT_MAX_MSG_SIZE,
    CRAS_CLIENT_MESSAGE_ID::{self, *},
};
use data_model::DataInit;
use sys_util::ScmSocket;

use crate::cras_server_socket::CrasServerSocket;
use crate::cras_shm::*;
use crate::cras_stream;

#[derive(Debug)]
pub enum Error {
    IoError(io::Error),
    SysUtilError(sys_util::Error),
    CrasStreamError(cras_stream::Error),
    ArrayTryFromSliceError(TryFromSliceError),
    InvalidSize,
    MessageTypeError,
    MessageNumFdError,
    MessageTruncated,
    MessageIdError,
    MessageFromSliceError,
}

impl error::Error for Error {}

impl fmt::Display for Error {
    fn fmt(&self, f: &mut fmt::Formatter) -> fmt::Result {
        match self {
            Error::IoError(ref err) => err.fmt(f),
            Error::SysUtilError(ref err) => err.fmt(f),
            Error::MessageTypeError => write!(f, "Message type error"),
            Error::CrasStreamError(ref err) => err.fmt(f),
            Error::ArrayTryFromSliceError(ref err) => err.fmt(f),
            Error::MessageNumFdError => write!(f, "Message the number of fds is not matched"),
            Error::MessageTruncated => write!(f, "Read truncated message"),
            Error::MessageIdError => write!(f, "No such id"),
            Error::MessageFromSliceError => write!(f, "Message from slice error"),
            Error::InvalidSize => write!(f, "Invalid data size"),
        }
    }
}

type Result<T> = std::result::Result<T, Error>;

impl From<io::Error> for Error {
    fn from(io_err: io::Error) -> Self {
        Error::IoError(io_err)
    }
}

impl From<sys_util::Error> for Error {
    fn from(sys_util_err: sys_util::Error) -> Self {
        Error::SysUtilError(sys_util_err)
    }
}

impl From<cras_stream::Error> for Error {
    fn from(err: cras_stream::Error) -> Self {
        Error::CrasStreamError(err)
    }
}

impl From<TryFromSliceError> for Error {
    fn from(err: TryFromSliceError) -> Self {
        Error::ArrayTryFromSliceError(err)
    }
}

/// A handled server result from one message sent from CRAS server.
pub enum ServerResult {
    /// client_id, CrasServerStateShmFd
    Connected(u32, CrasServerStateShmFd),
    /// stream_id, header_fd, samples_fd
    StreamConnected(u32, CrasAudioShmHeaderFd, CrasShmFd),
    DebugInfoReady,
}

impl ServerResult {
    /// Reads and handles one server message and converts `CrasClientMessage` into `ServerResult`
    /// with error handling.
    ///
    /// # Arguments
    /// * `server_socket`: A reference to `CrasServerSocket`.
    pub fn handle_server_message(server_socket: &CrasServerSocket) -> Result<ServerResult> {
        let message = CrasClientMessage::try_new(&server_socket)?;
        match message.get_id()? {
            CRAS_CLIENT_MESSAGE_ID::CRAS_CLIENT_CONNECTED => {
                let cmsg: &cras_client_connected = message.get_message()?;
                // CRAS server should return a shared memory area which contains
                // `cras_server_state`.
                let server_state_fd = unsafe { CrasServerStateShmFd::new(message.fds[0]) };
                Ok(ServerResult::Connected(cmsg.client_id, server_state_fd))
            }
            CRAS_CLIENT_MESSAGE_ID::CRAS_CLIENT_STREAM_CONNECTED => {
                let cmsg: &cras_client_stream_connected = message.get_message()?;
                // CRAS should return two shared memory areas the first which has
                // mem::size_of::<cras_audio_shm_header>() bytes, and the second which has
                // `samples_shm_size` bytes.
                Ok(ServerResult::StreamConnected(
                    cmsg.stream_id,
                    // Safe because CRAS ensures that the first fd contains a cras_audio_shm_header
                    unsafe { CrasAudioShmHeaderFd::new(message.fds[0]) },
                    // Safe because CRAS ensures that the second fd has length 'samples_shm_size'
                    unsafe { CrasShmFd::new(message.fds[1], cmsg.samples_shm_size as usize) },
                ))
            }
            CRAS_CLIENT_MESSAGE_ID::CRAS_CLIENT_AUDIO_DEBUG_INFO_READY => {
                Ok(ServerResult::DebugInfoReady)
            }
            _ => Err(Error::MessageTypeError),
        }
    }
}

// A structure for raw message with fds from CRAS server.
struct CrasClientMessage {
    fds: [RawFd; 2],
    data: [u8; CRAS_CLIENT_MAX_MSG_SIZE as usize],
    len: usize,
}

/// The default constructor won't be used outside of this file and it's an optimization to prevent
/// having to copy the message data from a temp buffer.
impl Default for CrasClientMessage {
    // Initializes fields with default values.
    fn default() -> Self {
        Self {
            fds: [-1; 2],
            data: [0; CRAS_CLIENT_MAX_MSG_SIZE as usize],
            len: 0,
        }
    }
}

impl CrasClientMessage {
    // Reads a message from server_socket and checks validity of the read result
    fn try_new(server_socket: &CrasServerSocket) -> Result<CrasClientMessage> {
        let mut message: Self = Default::default();
        let (len, fd_nums) = server_socket.recv_with_fds(&mut message.data, &mut message.fds)?;

        if len < mem::size_of::<cras_client_message>() {
            Err(Error::MessageTruncated)
        } else {
            message.len = len;
            message.check_fd_nums(fd_nums)?;
            Ok(message)
        }
    }

    // Check if `fd nums` of a read result is valid
    fn check_fd_nums(&self, fd_nums: usize) -> Result<()> {
        match self.get_id()? {
            CRAS_CLIENT_CONNECTED => match fd_nums {
                1 => Ok(()),
                _ => Err(Error::MessageNumFdError),
            },
            CRAS_CLIENT_STREAM_CONNECTED => match fd_nums {
                // CRAS should return two shared memory areas the first which has
                // mem::size_of::<cras_audio_shm_header>() bytes, and the second which has
                // `samples_shm_size` bytes.
                2 => Ok(()),
                _ => Err(Error::MessageNumFdError),
            },
            CRAS_CLIENT_AUDIO_DEBUG_INFO_READY => match fd_nums {
                0 => Ok(()),
                _ => Err(Error::MessageNumFdError),
            },
            _ => Err(Error::MessageTypeError),
        }
    }

    // Gets the message id
    fn get_id(&self) -> Result<CRAS_CLIENT_MESSAGE_ID> {
        let offset = mem::size_of::<u32>();
        match u32::from_le_bytes(self.data[offset..offset + 4].try_into()?) {
            id if id == (CRAS_CLIENT_CONNECTED as u32) => Ok(CRAS_CLIENT_CONNECTED),
            id if id == (CRAS_CLIENT_STREAM_CONNECTED as u32) => Ok(CRAS_CLIENT_STREAM_CONNECTED),
            id if id == (CRAS_CLIENT_AUDIO_DEBUG_INFO_READY as u32) => {
                Ok(CRAS_CLIENT_AUDIO_DEBUG_INFO_READY)
            }
            _ => Err(Error::MessageIdError),
        }
    }

    // Gets a reference to the message content
    fn get_message<T: DataInit>(&self) -> Result<&T> {
        if self.len != mem::size_of::<T>() {
            return Err(Error::InvalidSize);
        }
        T::from_slice(&self.data[..mem::size_of::<T>()]).ok_or(Error::MessageFromSliceError)
    }
}
