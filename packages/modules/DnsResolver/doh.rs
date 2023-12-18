/*
 * Copyright (C) 2021 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

//! DoH backend for the Android DnsResolver module.

use anyhow::{anyhow, Context, Result};
use lazy_static::lazy_static;
use libc::{c_char, size_t, ssize_t};
use log::{debug, error, info, warn};
use quiche::h3;
use ring::rand::SecureRandom;
use std::collections::HashMap;
use std::net::{IpAddr, SocketAddr};
use std::os::unix::io::{AsRawFd, RawFd};
use std::str::FromStr;
use std::sync::Arc;
use std::{ptr, slice};
use tokio::net::UdpSocket;
use tokio::runtime::{Builder, Runtime};
use tokio::sync::{mpsc, oneshot};
use tokio::task;
use tokio::time::Duration;
use url::Url;

lazy_static! {
    /// Tokio runtime used to perform doh-handler tasks.
    static ref RUNTIME_STATIC: Arc<Runtime> = Arc::new(
        Builder::new_multi_thread()
            .worker_threads(2)
            .max_blocking_threads(1)
            .enable_all()
            .thread_name("doh-handler")
            .build()
            .expect("Failed to create tokio runtime")
    );
}

const MAX_BUFFERED_CMD_SIZE: usize = 400;
const MAX_INCOMING_BUFFER_SIZE_WHOLE: u64 = 10000000;
const MAX_INCOMING_BUFFER_SIZE_EACH: u64 = 1000000;
const MAX_CONCURRENT_STREAM_SIZE: u64 = 100;
const MAX_DATAGRAM_SIZE: usize = 1350;
const MAX_DATAGRAM_SIZE_U64: u64 = 1350;
const DOH_PORT: u16 = 443;
const QUICHE_IDLE_TIMEOUT_MS: u64 = 180000;
const SYSTEM_CERT_PATH: &str = "/system/etc/security/cacerts";

type SCID = [u8; quiche::MAX_CONN_ID_LEN];
type Query = Vec<u8>;
type Response = Vec<u8>;
type CmdSender = mpsc::Sender<Command>;
type CmdReceiver = mpsc::Receiver<Command>;
type QueryResponder = oneshot::Sender<Option<Response>>;

#[derive(Debug)]
enum Command {
    DohQuery { query: Query, resp: QueryResponder },
}

/// Context for a running DoH engine.
pub struct DohDispatcher {
    /// Used to submit queries to the I/O thread.
    query_sender: CmdSender,

    join_handle: task::JoinHandle<Result<()>>,
}

fn make_doh_udp_socket(ip_addr: &str, mark: u32) -> Result<std::net::UdpSocket> {
    let sock_addr = SocketAddr::new(IpAddr::from_str(&ip_addr)?, DOH_PORT);
    let bind_addr = match sock_addr {
        std::net::SocketAddr::V4(_) => "0.0.0.0:0",
        std::net::SocketAddr::V6(_) => "[::]:0",
    };
    let udp_sk = std::net::UdpSocket::bind(bind_addr)?;
    udp_sk.set_nonblocking(true)?;
    mark_socket(udp_sk.as_raw_fd(), mark)?;
    udp_sk.connect(sock_addr)?;

    debug!("connecting to {:} from {:}", sock_addr, udp_sk.local_addr()?);
    Ok(udp_sk)
}

// DoH dispatcher
impl DohDispatcher {
    fn new(
        url: &str,
        ip_addr: &str,
        mark: u32,
        cert_path: Option<&str>,
    ) -> Result<Box<DohDispatcher>> {
        // Setup socket
        let udp_sk = make_doh_udp_socket(&ip_addr, mark)?;
        DohDispatcher::new_with_socket(url, ip_addr, mark, cert_path, udp_sk)
    }

    fn new_with_socket(
        url: &str,
        ip_addr: &str,
        mark: u32,
        cert_path: Option<&str>,
        udp_sk: std::net::UdpSocket,
    ) -> Result<Box<DohDispatcher>> {
        let url = Url::parse(&url.to_string())?;
        if url.domain().is_none() {
            return Err(anyhow!("no domain"));
        }
        // Setup quiche config
        let config = create_quiche_config(cert_path)?;
        let h3_config = h3::Config::new()?;
        let mut scid = [0; quiche::MAX_CONN_ID_LEN];
        ring::rand::SystemRandom::new().fill(&mut scid[..]).context("failed to generate scid")?;

        let (cmd_sender, cmd_receiver) = mpsc::channel::<Command>(MAX_BUFFERED_CMD_SIZE);
        debug!(
            "Creating a doh handler task: url={}, ip_addr={}, mark={:#x}, scid {:x?}",
            url, ip_addr, mark, &scid
        );
        let join_handle =
            RUNTIME_STATIC.spawn(doh_handler(url, udp_sk, config, h3_config, scid, cmd_receiver));
        Ok(Box::new(DohDispatcher { query_sender: cmd_sender, join_handle }))
    }

    fn query(&self, cmd: Command) -> Result<()> {
        self.query_sender.blocking_send(cmd)?;
        Ok(())
    }

    fn abort_handler(&self) {
        self.join_handle.abort();
    }
}

async fn doh_handler(
    url: url::Url,
    udp_sk: std::net::UdpSocket,
    mut config: quiche::Config,
    h3_config: h3::Config,
    scid: SCID,
    mut rx: CmdReceiver,
) -> Result<()> {
    debug!("doh_handler: url={:?}", url);

    let sk = UdpSocket::from_std(udp_sk)?;
    let mut conn = quiche::connect(url.domain(), &scid, &mut config)?;
    let mut quic_conn_start = std::time::Instant::now();
    let mut h3_conn: Option<h3::Connection> = None;
    let mut is_idle = false;
    let mut buf = [0; 65535];

    let mut query_map = HashMap::<u64, QueryResponder>::new();
    let mut pending_cmds: Vec<Command> = Vec::new();

    let mut ts = Duration::from_millis(QUICHE_IDLE_TIMEOUT_MS);
    loop {
        tokio::select! {
            size = sk.recv(&mut buf) => {
                debug!("recv {:?} ", size);
                match size {
                    Ok(size) => {
                        let processed = match conn.recv(&mut buf[..size]) {
                            Ok(l) => l,
                            Err(e) => {
                                error!("quic recv failed: {:?}", e);
                                continue;
                            }
                        };
                        debug!("processed {} bytes", processed);
                    },
                    Err(e) => {
                        error!("socket recv failed: {:?}", e);
                        continue;
                    },
                };
            }
            Some(cmd) = rx.recv() => {
                debug!("recv {:?}", cmd);
                pending_cmds.push(cmd);
            }
            _ = tokio::time::sleep(ts) => {
                conn.on_timeout();
                debug!("quic connection timeout");
            }
        }
        if conn.is_closed() {
            // Show connection statistics after it's closed
            if !is_idle {
                info!("connection closed, {:?}, {:?}", quic_conn_start.elapsed(), conn.stats());
                is_idle = true;
                if !conn.is_established() {
                    error!("connection handshake timed out after {:?}", quic_conn_start.elapsed());
                }
            }

            // If there is any pending query, resume the quic connection.
            if !pending_cmds.is_empty() {
                info!("still some pending queries but connection is not avaiable, resume it");
                conn = quiche::connect(url.domain(), &scid, &mut config)?;
                quic_conn_start = std::time::Instant::now();
                h3_conn = None;
                is_idle = false;
            }
        }

        // Create a new HTTP/3 connection once the QUIC connection is established.
        if conn.is_established() && h3_conn.is_none() {
            info!("quic ready, creating h3 conn");
            h3_conn = Some(quiche::h3::Connection::with_transport(&mut conn, &h3_config)?);
        }
        // Try to receive query answers from h3 connection.
        if let Some(h3) = h3_conn.as_mut() {
            recv_query(h3, &mut conn, &mut query_map).await;
        }

        // Update the next timeout of quic connection.
        ts = conn.timeout().unwrap_or_else(|| Duration::from_millis(QUICHE_IDLE_TIMEOUT_MS));
        info!("next connection timouts  {:?}", ts);

        // Process the pending queries
        while !pending_cmds.is_empty() && conn.is_established() {
            if let Some(cmd) = pending_cmds.pop() {
                match cmd {
                    Command::DohQuery { query, resp } => {
                        match send_dns_query(&query, &url, &mut h3_conn, &mut conn) {
                            Ok(stream_id) => {
                                query_map.insert(stream_id, resp);
                            }
                            Err(e) => {
                                info!("failed to send query {}", e);
                                pending_cmds.push(Command::DohQuery { query, resp });
                            }
                        }
                    }
                }
            }
        }
        flush_tx(&sk, &mut conn).await.unwrap_or_else(|e| {
            error!("flush error {:?} ", e);
        });
    }
}

fn send_dns_query(
    query: &[u8],
    url: &url::Url,
    h3_conn: &mut Option<quiche::h3::Connection>,
    mut conn: &mut quiche::Connection,
) -> Result<u64> {
    let h3_conn = h3_conn.as_mut().ok_or_else(|| anyhow!("h3 conn isn't available"))?;

    let mut path = String::from(url.path());
    path.push_str("?dns=");
    path.push_str(std::str::from_utf8(&query)?);
    let _req = vec![
        quiche::h3::Header::new(":method", "GET"),
        quiche::h3::Header::new(":scheme", "https"),
        quiche::h3::Header::new(
            ":authority",
            url.host_str().ok_or_else(|| anyhow!("failed to get host"))?,
        ),
        quiche::h3::Header::new(":path", &path),
        quiche::h3::Header::new("user-agent", "quiche"),
        quiche::h3::Header::new("accept", "application/dns-message"),
        // TODO: is content-length required?
    ];

    Ok(h3_conn.send_request(&mut conn, &_req, false /*fin*/)?)
}

async fn recv_query(
    h3_conn: &mut h3::Connection,
    mut conn: &mut quiche::Connection,
    map: &mut HashMap<u64, QueryResponder>,
) {
    // Process HTTP/3 events.
    let mut buf = [0; MAX_DATAGRAM_SIZE];
    loop {
        match h3_conn.poll(&mut conn) {
            Ok((stream_id, quiche::h3::Event::Headers { list, has_body })) => {
                info!(
                    "got response headers {:?} on stream id {} has_body {}",
                    list, stream_id, has_body
                );
            }
            Ok((stream_id, quiche::h3::Event::Data)) => {
                debug!("quiche::h3::Event::Data");
                if let Ok(read) = h3_conn.recv_body(&mut conn, stream_id, &mut buf) {
                    info!(
                        "got {} bytes of response data on stream {}: {:x?}",
                        read,
                        stream_id,
                        &buf[..read]
                    );
                    if let Some(resp) = map.remove(&stream_id) {
                        resp.send(Some(buf[..read].to_vec())).unwrap_or_else(|e| {
                            warn!("the receiver dropped {:?}", e);
                        });
                    }
                }
            }
            Ok((_stream_id, quiche::h3::Event::Finished)) => {
                debug!("quiche::h3::Event::Finished");
            }
            Ok((_stream_id, quiche::h3::Event::Datagram)) => {
                debug!("quiche::h3::Event::Datagram");
            }
            Ok((_stream_id, quiche::h3::Event::GoAway)) => {
                debug!("quiche::h3::Event::GoAway");
            }
            Err(quiche::h3::Error::Done) => {
                debug!("quiche::h3::Error::Done");
                break;
            }
            Err(e) => {
                error!("HTTP/3 processing failed: {:?}", e);
                break;
            }
        }
    }
}

async fn flush_tx(sk: &UdpSocket, conn: &mut quiche::Connection) -> Result<()> {
    let mut out = [0; MAX_DATAGRAM_SIZE];
    loop {
        let write = match conn.send(&mut out) {
            Ok(v) => v,
            Err(quiche::Error::Done) => {
                debug!("done writing");
                break;
            }
            Err(e) => {
                conn.close(false, 0x1, b"fail").ok();
                return Err(anyhow::Error::new(e));
            }
        };
        sk.send(&out[..write]).await?;
        debug!("written {}", write);
    }
    Ok(())
}

fn create_quiche_config(cert_path: Option<&str>) -> Result<quiche::Config> {
    let mut config = quiche::Config::new(quiche::PROTOCOL_VERSION)?;
    config.set_application_protos(h3::APPLICATION_PROTOCOL)?;
    config.verify_peer(true);
    config.load_verify_locations_from_directory(cert_path.unwrap_or(SYSTEM_CERT_PATH))?;
    // Some of these configs are necessary, or the server can't respond the HTTP/3 request.
    config.set_max_idle_timeout(QUICHE_IDLE_TIMEOUT_MS);
    config.set_max_udp_payload_size(MAX_DATAGRAM_SIZE_U64);
    config.set_initial_max_data(MAX_INCOMING_BUFFER_SIZE_WHOLE);
    config.set_initial_max_stream_data_bidi_local(MAX_INCOMING_BUFFER_SIZE_EACH);
    config.set_initial_max_stream_data_bidi_remote(MAX_INCOMING_BUFFER_SIZE_EACH);
    config.set_initial_max_stream_data_uni(MAX_INCOMING_BUFFER_SIZE_EACH);
    config.set_initial_max_streams_bidi(MAX_CONCURRENT_STREAM_SIZE);
    config.set_initial_max_streams_uni(MAX_CONCURRENT_STREAM_SIZE);
    config.set_disable_active_migration(true);
    Ok(config)
}

fn mark_socket(fd: RawFd, mark: u32) -> Result<()> {
    // libc::setsockopt is a wrapper function calling into bionic setsockopt.
    // Both fd and mark are valid, which makes the function call mostly safe.
    if unsafe {
        libc::setsockopt(
            fd,
            libc::SOL_SOCKET,
            libc::SO_MARK,
            &mark as *const _ as *const libc::c_void,
            std::mem::size_of::<u32>() as libc::socklen_t,
        )
    } == 0
    {
        Ok(())
    } else {
        Err(anyhow::Error::new(std::io::Error::last_os_error()))
    }
}

/// Performs static initialization fo the DoH engine.
#[no_mangle]
pub extern "C" fn doh_init() -> *const c_char {
    android_logger::init_once(android_logger::Config::default().with_min_level(log::Level::Trace));
    static VERSION: &str = "1.0\0";
    VERSION.as_ptr() as *const c_char
}

/// Creates and returns a DoH engine instance.
/// The returned object must be freed with doh_delete().
/// # Safety
/// All the pointer args are null terminated strings.
#[no_mangle]
pub unsafe extern "C" fn doh_new(
    url: *const c_char,
    ip_addr: *const c_char,
    mark: libc::uint32_t,
    cert_path: *const c_char,
) -> *mut DohDispatcher {
    let (url, ip_addr, cert_path) = match (
        std::ffi::CStr::from_ptr(url).to_str(),
        std::ffi::CStr::from_ptr(ip_addr).to_str(),
        std::ffi::CStr::from_ptr(cert_path).to_str(),
    ) {
        (Ok(url), Ok(ip_addr), Ok(cert_path)) => {
            if !cert_path.is_empty() {
                (url, ip_addr, Some(cert_path))
            } else {
                (url, ip_addr, None)
            }
        }
        _ => {
            error!("bad input");
            return ptr::null_mut();
        }
    };
    match DohDispatcher::new(url, ip_addr, mark, cert_path) {
        Ok(c) => Box::into_raw(c),
        Err(e) => {
            error!("doh_new: failed: {:?}", e);
            ptr::null_mut()
        }
    }
}

/// Deletes a DoH engine created by doh_new().
/// # Safety
/// `doh` must be a non-null pointer previously created by `doh_new()`
/// and not yet deleted by `doh_delete()`.
#[no_mangle]
pub unsafe extern "C" fn doh_delete(doh: *mut DohDispatcher) {
    Box::from_raw(doh).abort_handler()
}

/// Sends a DNS query and waits for the response.
/// # Safety
/// `doh` must be a non-null pointer previously created by `doh_new()`
/// and not yet deleted by `doh_delete()`.
/// `query` must point to a buffer at least `query_len` in size.
/// `response` must point to a buffer at least `response_len` in size.
#[no_mangle]
pub unsafe extern "C" fn doh_query(
    doh: &mut DohDispatcher,
    query: *mut u8,
    query_len: size_t,
    response: *mut u8,
    response_len: size_t,
) -> ssize_t {
    let q = slice::from_raw_parts_mut(query, query_len);
    let (resp_tx, resp_rx) = oneshot::channel();
    let cmd = Command::DohQuery { query: q.to_vec(), resp: resp_tx };
    if let Err(e) = doh.query(cmd) {
        error!("Failed to send the query: {:?}", e);
        return -1;
    }
    match RUNTIME_STATIC.block_on(resp_rx) {
        Ok(value) => {
            if let Some(resp) = value {
                if resp.len() > response_len || resp.len() > isize::MAX as usize {
                    return -1;
                }
                let response = slice::from_raw_parts_mut(response, resp.len());
                response.copy_from_slice(&resp);
                return resp.len() as ssize_t;
            }
            -1
        }
        Err(e) => {
            error!("no result {}", e);
            -1
        }
    }
}

#[cfg(test)]
mod tests {
    use super::*;
    use std::net::{Ipv4Addr, SocketAddr, SocketAddrV4};

    const TEST_MARK: u32 = 0xD0033;
    const LOOPBACK_ADDR: &str = "127.0.0.1";

    #[test]
    fn dohdispatcher_invalid_args() {
        let test_args = [
            // Bad url
            ("foo", "bar"),
            ("https://1", "bar"),
            ("https:/", "bar"),
            // Bad ip
            ("https://dns.google", "bar"),
            ("https://dns.google", "256.256.256.256"),
        ];
        for args in &test_args {
            assert!(
                DohDispatcher::new(args.0, args.1, 0, None).is_err(),
                "doh dispatcher should not be created"
            )
        }
    }

    #[test]
    fn make_doh_udp_socket() {
        // Bad ip
        for ip in &["foo", "1", "333.333.333.333"] {
            assert!(super::make_doh_udp_socket(ip, 0).is_err(), "udp socket should not be created");
        }
        // Make a socket connecting to loopback with a test mark.
        let sk = super::make_doh_udp_socket(LOOPBACK_ADDR, TEST_MARK).unwrap();
        // Check if the socket is connected to loopback.
        assert_eq!(
            sk.peer_addr().unwrap(),
            SocketAddr::V4(SocketAddrV4::new(Ipv4Addr::new(127, 0, 0, 1), DOH_PORT))
        );

        // Check if the socket mark is correct.
        let fd: RawFd = sk.as_raw_fd();

        let mut mark: u32 = 50;
        let mut size = std::mem::size_of::<u32>() as libc::socklen_t;
        unsafe {
            // Safety: fd must be valid.
            assert_eq!(
                libc::getsockopt(
                    fd,
                    libc::SOL_SOCKET,
                    libc::SO_MARK,
                    &mut mark as *mut _ as *mut libc::c_void,
                    &mut size as *mut _ as *mut libc::socklen_t,
                ),
                0
            );
        }
        assert_eq!(mark, TEST_MARK);

        // Check if the socket is non-blocking.
        unsafe {
            // Safety: fd must be valid.
            assert_eq!(libc::fcntl(fd, libc::F_GETFL, 0) & libc::O_NONBLOCK, libc::O_NONBLOCK);
        }
    }

    #[test]
    fn create_quiche_config() {
        assert!(
            super::create_quiche_config(None).is_ok(),
            "quiche config without cert creating failed"
        );
        assert!(
            super::create_quiche_config(Some("data/local/tmp/")).is_ok(),
            "quiche config with cert creating failed"
        );
    }

    const GOOGLE_DNS_URL: &str = "https://dns.google/dns-query";
    const GOOGLE_DNS_IP: &str = "8.8.8.8";
    // qtype: A, qname: www.example.com
    const SAMPLE_QUERY: &str = "q80BAAABAAAAAAAAA3d3dwdleGFtcGxlA2NvbQAAAQAB";
    #[test]
    fn close_doh() {
        let udp_sk = super::make_doh_udp_socket(LOOPBACK_ADDR, TEST_MARK).unwrap();
        let doh =
            DohDispatcher::new_with_socket(GOOGLE_DNS_URL, GOOGLE_DNS_IP, 0, None, udp_sk).unwrap();
        let (resp_tx, resp_rx) = oneshot::channel();
        let cmd = Command::DohQuery { query: SAMPLE_QUERY.as_bytes().to_vec(), resp: resp_tx };
        assert!(doh.query(cmd).is_ok(), "Send query failed");
        doh.abort_handler();
        assert!(RUNTIME_STATIC.block_on(resp_rx).is_err(), "channel should already be closed");
    }

    #[test]
    fn doh_init() {
        unsafe {
            // Safety: the returned pointer of doh_init() must be a null terminated string.
            assert_eq!(std::ffi::CStr::from_ptr(super::doh_init()).to_str().unwrap(), "1.0");
        }
    }
}
