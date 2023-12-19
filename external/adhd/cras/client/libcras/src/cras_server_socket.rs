// Copyright 2019 The Chromium OS Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
use std::os::unix::io::{AsRawFd, RawFd};
use std::{io, mem};

use cras_sys::gen::{cras_disconnect_stream_message, cras_server_message, CRAS_SERVER_MESSAGE_ID};
use sys_util::{net::UnixSeqpacket, ScmSocket};

use data_model::DataInit;

/// Server socket type to connect.
pub enum CrasSocketType {
    /// A server socket type supports only playback function.
    Legacy,
    /// A server socket type supports both playback and capture functions.
    Unified,
}

impl CrasSocketType {
    fn sock_path(&self) -> &str {
        match self {
            Self::Legacy => "/run/cras/.cras_socket",
            Self::Unified => "/run/cras/.cras_unified",
        }
    }
}

/// A socket connecting to the CRAS audio server.
pub struct CrasServerSocket {
    socket: UnixSeqpacket,
}

impl CrasServerSocket {
    pub fn new() -> io::Result<CrasServerSocket> {
        Self::with_type(CrasSocketType::Legacy)
    }

    /// Creates a `CrasServerSocket` with given `CrasSocketType`.
    ///
    /// # Errors
    ///
    /// Returns the `io::Error` generated when connecting to the socket on failure.
    pub fn with_type(socket_type: CrasSocketType) -> io::Result<CrasServerSocket> {
        Ok(CrasServerSocket {
            socket: UnixSeqpacket::connect(socket_type.sock_path())?,
        })
    }

    /// Sends a sized and packed server messge to the server socket. The message
    /// must implement `Sized` and `DataInit`.
    /// # Arguments
    /// * `message` - A sized and packed message.
    /// * `fds` - A slice of fds to send.
    ///
    /// # Returns
    /// * Length of written bytes in `usize`.
    ///
    /// # Errors
    /// Return error if the socket fails to write message to server.
    pub fn send_server_message_with_fds<M: Sized + DataInit>(
        &self,
        message: &M,
        fds: &[RawFd],
    ) -> io::Result<usize> {
        match fds.len() {
            0 => self.socket.send(message.as_slice()),
            _ => {
                let ioslice = io::IoSlice::new(message.as_slice());
                match self.send_with_fds(&[ioslice], fds) {
                    Ok(len) => Ok(len),
                    Err(err) => Err(io::Error::new(io::ErrorKind::Other, format!("{}", err))),
                }
            }
        }
    }

    /// Creates a clone of the underlying socket. The returned clone can also be
    /// used to communicate with the cras server.
    pub fn try_clone(&self) -> io::Result<CrasServerSocket> {
        let new_sock = self.socket.try_clone()?;
        Ok(CrasServerSocket { socket: new_sock })
    }

    /// Send a message to request disconnection of the given stream.
    ///
    /// Builds a `cras_disconnect_stream_message` containing `stream_id` and
    /// sends it to the server.
    /// No response is expected.
    ///
    /// # Arguments
    ///
    /// * `stream_id` - The id of the stream that should be disconnected.
    ///
    /// # Errors
    ///
    /// * If the message was not written to the server socket successfully.
    pub fn disconnect_stream(&self, stream_id: u32) -> io::Result<()> {
        let msg_header = cras_server_message {
            length: mem::size_of::<cras_disconnect_stream_message>() as u32,
            id: CRAS_SERVER_MESSAGE_ID::CRAS_SERVER_DISCONNECT_STREAM,
        };
        let server_cmsg = cras_disconnect_stream_message {
            header: msg_header,
            stream_id,
        };
        self.send_server_message_with_fds(&server_cmsg, &[])
            .map(|_| ())
    }
}

// For using `recv_with_fds` and `send_with_fds`.
impl ScmSocket for CrasServerSocket {
    fn socket_fd(&self) -> RawFd {
        self.socket.as_raw_fd()
    }
}

// For using `PollContex`.
impl AsRawFd for CrasServerSocket {
    fn as_raw_fd(&self) -> RawFd {
        self.socket.as_raw_fd()
    }
}
