// Copyright 2020 The Chromium OS Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

/// Structs to supplement std::net.
use std::io;
use std::mem::{self, size_of};
use std::net::{SocketAddr, SocketAddrV4, SocketAddrV6, TcpListener, TcpStream, ToSocketAddrs};
use std::os::unix::io::{AsRawFd, FromRawFd, IntoRawFd, RawFd};

use libc::{
    c_int, in6_addr, in_addr, sa_family_t, sockaddr, sockaddr_in, sockaddr_in6, socklen_t, AF_INET,
    AF_INET6, SOCK_CLOEXEC, SOCK_STREAM,
};

/// Assist in handling both IP version 4 and IP version 6.
#[derive(Debug, Copy, Clone, PartialEq, Eq)]
pub enum InetVersion {
    V4,
    V6,
}

impl InetVersion {
    pub fn from_sockaddr(s: &SocketAddr) -> Self {
        match s {
            SocketAddr::V4(_) => InetVersion::V4,
            SocketAddr::V6(_) => InetVersion::V6,
        }
    }
}

impl Into<sa_family_t> for InetVersion {
    fn into(self) -> sa_family_t {
        match self {
            InetVersion::V4 => AF_INET as sa_family_t,
            InetVersion::V6 => AF_INET6 as sa_family_t,
        }
    }
}

fn sockaddrv4_to_lib_c(s: &SocketAddrV4) -> sockaddr_in {
    sockaddr_in {
        sin_family: AF_INET as sa_family_t,
        sin_port: s.port().to_be(),
        sin_addr: in_addr {
            s_addr: u32::from_ne_bytes(s.ip().octets()),
        },
        sin_zero: [0; 8],
    }
}

fn sockaddrv6_to_lib_c(s: &SocketAddrV6) -> sockaddr_in6 {
    sockaddr_in6 {
        sin6_family: AF_INET6 as sa_family_t,
        sin6_port: s.port().to_be(),
        sin6_flowinfo: 0,
        sin6_addr: in6_addr {
            s6_addr: s.ip().octets(),
        },
        sin6_scope_id: 0,
    }
}

/// A TCP socket.
///
/// Do not use this class unless you need to change socket options or query the
/// state of the socket prior to calling listen or connect. Instead use either TcpStream or
/// TcpListener.
#[derive(Debug)]
pub struct TcpSocket {
    inet_version: InetVersion,
    fd: RawFd,
}

impl TcpSocket {
    pub fn new(inet_version: InetVersion) -> io::Result<Self> {
        let fd = unsafe {
            libc::socket(
                Into::<sa_family_t>::into(inet_version) as c_int,
                SOCK_STREAM | SOCK_CLOEXEC,
                0,
            )
        };
        if fd < 0 {
            Err(io::Error::last_os_error())
        } else {
            Ok(TcpSocket { inet_version, fd })
        }
    }

    pub fn bind<A: ToSocketAddrs>(&mut self, addr: A) -> io::Result<()> {
        let sockaddr = addr
            .to_socket_addrs()
            .map_err(|_| io::Error::from_raw_os_error(libc::EINVAL))?
            .next()
            .unwrap();

        let ret = match sockaddr {
            SocketAddr::V4(a) => {
                let sin = sockaddrv4_to_lib_c(&a);
                // Safe because this doesn't modify any memory and we check the return value.
                unsafe {
                    libc::bind(
                        self.fd,
                        &sin as *const sockaddr_in as *const sockaddr,
                        size_of::<sockaddr_in>() as socklen_t,
                    )
                }
            }
            SocketAddr::V6(a) => {
                let sin6 = sockaddrv6_to_lib_c(&a);
                // Safe because this doesn't modify any memory and we check the return value.
                unsafe {
                    libc::bind(
                        self.fd,
                        &sin6 as *const sockaddr_in6 as *const sockaddr,
                        size_of::<sockaddr_in6>() as socklen_t,
                    )
                }
            }
        };
        if ret < 0 {
            let bind_err = io::Error::last_os_error();
            Err(bind_err)
        } else {
            Ok(())
        }
    }

    pub fn connect<A: ToSocketAddrs>(self, addr: A) -> io::Result<TcpStream> {
        let sockaddr = addr
            .to_socket_addrs()
            .map_err(|_| io::Error::from_raw_os_error(libc::EINVAL))?
            .next()
            .unwrap();

        let ret = match sockaddr {
            SocketAddr::V4(a) => {
                let sin = sockaddrv4_to_lib_c(&a);
                // Safe because this doesn't modify any memory and we check the return value.
                unsafe {
                    libc::connect(
                        self.fd,
                        &sin as *const sockaddr_in as *const sockaddr,
                        size_of::<sockaddr_in>() as socklen_t,
                    )
                }
            }
            SocketAddr::V6(a) => {
                let sin6 = sockaddrv6_to_lib_c(&a);
                // Safe because this doesn't modify any memory and we check the return value.
                unsafe {
                    libc::connect(
                        self.fd,
                        &sin6 as *const sockaddr_in6 as *const sockaddr,
                        size_of::<sockaddr_in>() as socklen_t,
                    )
                }
            }
        };

        if ret < 0 {
            let connect_err = io::Error::last_os_error();
            Err(connect_err)
        } else {
            // Safe because the ownership of the raw fd is released from self and taken over by the
            // new TcpStream.
            Ok(unsafe { TcpStream::from_raw_fd(self.into_raw_fd()) })
        }
    }

    pub fn listen(self) -> io::Result<TcpListener> {
        // Safe because this doesn't modify any memory and we check the return value.
        let ret = unsafe { libc::listen(self.fd, 1) };
        if ret < 0 {
            let listen_err = io::Error::last_os_error();
            Err(listen_err)
        } else {
            // Safe because the ownership of the raw fd is released from self and taken over by the
            // new TcpListener.
            Ok(unsafe { TcpListener::from_raw_fd(self.into_raw_fd()) })
        }
    }

    /// Returns the port that this socket is bound to. This can only succeed after bind is called.
    pub fn local_port(&self) -> io::Result<u16> {
        match self.inet_version {
            InetVersion::V4 => {
                let mut sin = sockaddr_in {
                    sin_family: 0,
                    sin_port: 0,
                    sin_addr: in_addr { s_addr: 0 },
                    sin_zero: [0; 8],
                };

                // Safe because we give a valid pointer for addrlen and check the length.
                let mut addrlen = size_of::<sockaddr_in>() as socklen_t;
                let ret = unsafe {
                    // Get the socket address that was actually bound.
                    libc::getsockname(
                        self.fd,
                        &mut sin as *mut sockaddr_in as *mut sockaddr,
                        &mut addrlen as *mut socklen_t,
                    )
                };
                if ret < 0 {
                    let getsockname_err = io::Error::last_os_error();
                    Err(getsockname_err)
                } else {
                    // If this doesn't match, it's not safe to get the port out of the sockaddr.
                    assert_eq!(addrlen as usize, size_of::<sockaddr_in>());

                    Ok(sin.sin_port)
                }
            }
            InetVersion::V6 => {
                let mut sin6 = sockaddr_in6 {
                    sin6_family: 0,
                    sin6_port: 0,
                    sin6_flowinfo: 0,
                    sin6_addr: in6_addr { s6_addr: [0; 16] },
                    sin6_scope_id: 0,
                };

                // Safe because we give a valid pointer for addrlen and check the length.
                let mut addrlen = size_of::<sockaddr_in6>() as socklen_t;
                let ret = unsafe {
                    // Get the socket address that was actually bound.
                    libc::getsockname(
                        self.fd,
                        &mut sin6 as *mut sockaddr_in6 as *mut sockaddr,
                        &mut addrlen as *mut socklen_t,
                    )
                };
                if ret < 0 {
                    let getsockname_err = io::Error::last_os_error();
                    Err(getsockname_err)
                } else {
                    // If this doesn't match, it's not safe to get the port out of the sockaddr.
                    assert_eq!(addrlen as usize, size_of::<sockaddr_in>());

                    Ok(sin6.sin6_port)
                }
            }
        }
    }
}

impl IntoRawFd for TcpSocket {
    fn into_raw_fd(self) -> RawFd {
        let fd = self.fd;
        mem::forget(self);
        fd
    }
}

impl AsRawFd for TcpSocket {
    fn as_raw_fd(&self) -> RawFd {
        self.fd
    }
}

impl Drop for TcpSocket {
    fn drop(&mut self) {
        // Safe because this doesn't modify any memory and we are the only
        // owner of the file descriptor.
        unsafe { libc::close(self.fd) };
    }
}
