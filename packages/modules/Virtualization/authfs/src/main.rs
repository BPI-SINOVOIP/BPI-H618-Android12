/*
 * Copyright (C) 2020 The Android Open Source Project
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

//! This crate implements AuthFS, a FUSE-based, non-generic filesystem where file access is
//! authenticated. This filesystem assumes the underlying layer is not trusted, e.g. file may be
//! provided by an untrusted host/VM, so that the content can't be simply trusted. However, with a
//! public key from a trusted party, this filesystem can still verify a (read-only) file signed by
//! the trusted party even if the host/VM as the blob provider is malicious. With the Merkle tree,
//! each read of file block can be verified individually only when needed.
//!
//! AuthFS only serve files that are specifically configured. A file configuration may include the
//! source (e.g. local file or remote file server), verification method (e.g. certificate for
//! fs-verity verification, or no verification if expected to mount over dm-verity), and file ID.
//! Regardless of the actual file name, the exposed file names through AuthFS are currently integer,
//! e.g. /mountpoint/42.

use anyhow::{bail, Context, Result};
use std::collections::BTreeMap;
use std::fs::File;
use std::io::Read;
use std::path::{Path, PathBuf};
use std::sync::{Arc, Mutex};
use structopt::StructOpt;

mod auth;
mod common;
mod crypto;
mod file;
mod fsverity;
mod fusefs;

use auth::FakeAuthenticator;
use file::{LocalFileReader, RemoteFileEditor, RemoteFileReader, RemoteMerkleTreeReader};
use fsverity::{VerifiedFileEditor, VerifiedFileReader};
use fusefs::{FileConfig, Inode};

#[derive(StructOpt)]
struct Args {
    /// Mount point of AuthFS.
    #[structopt(parse(from_os_str))]
    mount_point: PathBuf,

    /// A read-only remote file with integrity check. Can be multiple.
    ///
    /// For example, `--remote-verified-file 5:10:1234:/path/to/cert` tells the filesystem to
    /// associate entry 5 with a remote file 10 of size 1234 bytes, and need to be verified against
    /// the /path/to/cert.
    #[structopt(long, parse(try_from_str = parse_remote_ro_file_option))]
    remote_ro_file: Vec<OptionRemoteRoFile>,

    /// A read-only remote file without integrity check. Can be multiple.
    ///
    /// For example, `--remote-unverified-file 5:10:1234` tells the filesystem to associate entry 5
    /// with a remote file 10 of size 1234 bytes.
    #[structopt(long, parse(try_from_str = parse_remote_ro_file_unverified_option))]
    remote_ro_file_unverified: Vec<OptionRemoteRoFileUnverified>,

    /// A new read-writable remote file with integrity check. Can be multiple.
    ///
    /// For example, `--remote-new-verified-file 12:34` tells the filesystem to associate entry 12
    /// with a remote file 34.
    #[structopt(long, parse(try_from_str = parse_remote_new_rw_file_option))]
    remote_new_rw_file: Vec<OptionRemoteRwFile>,

    /// Debug only. A read-only local file with integrity check. Can be multiple.
    #[structopt(long, parse(try_from_str = parse_local_file_ro_option))]
    local_ro_file: Vec<OptionLocalFileRo>,

    /// Debug only. A read-only local file without integrity check. Can be multiple.
    #[structopt(long, parse(try_from_str = parse_local_ro_file_unverified_ro_option))]
    local_ro_file_unverified: Vec<OptionLocalRoFileUnverified>,

    /// Enable debugging features.
    #[structopt(long)]
    debug: bool,
}

struct OptionRemoteRoFile {
    ino: Inode,

    /// ID to refer to the remote file.
    remote_id: i32,

    /// Expected size of the remote file. Necessary for signature check and Merkle tree
    /// verification.
    file_size: u64,

    /// Certificate to verify the authenticity of the file's fs-verity signature.
    /// TODO(170494765): Implement PKCS#7 signature verification.
    _certificate_path: PathBuf,
}

struct OptionRemoteRoFileUnverified {
    ino: Inode,

    /// ID to refer to the remote file.
    remote_id: i32,

    /// Expected size of the remote file.
    file_size: u64,
}

struct OptionRemoteRwFile {
    ino: Inode,

    /// ID to refer to the remote file.
    remote_id: i32,
}

struct OptionLocalFileRo {
    ino: Inode,

    /// Local path of the backing file.
    file_path: PathBuf,

    /// Local path of the backing file's fs-verity Merkle tree dump.
    merkle_tree_dump_path: PathBuf,

    /// Local path of fs-verity signature for the backing file.
    signature_path: PathBuf,

    /// Certificate to verify the authenticity of the file's fs-verity signature.
    /// TODO(170494765): Implement PKCS#7 signature verification.
    _certificate_path: PathBuf,
}

struct OptionLocalRoFileUnverified {
    ino: Inode,

    /// Local path of the backing file.
    file_path: PathBuf,
}

fn parse_remote_ro_file_option(option: &str) -> Result<OptionRemoteRoFile> {
    let strs: Vec<&str> = option.split(':').collect();
    if strs.len() != 4 {
        bail!("Invalid option: {}", option);
    }
    Ok(OptionRemoteRoFile {
        ino: strs[0].parse::<Inode>()?,
        remote_id: strs[1].parse::<i32>()?,
        file_size: strs[2].parse::<u64>()?,
        _certificate_path: PathBuf::from(strs[3]),
    })
}

fn parse_remote_ro_file_unverified_option(option: &str) -> Result<OptionRemoteRoFileUnverified> {
    let strs: Vec<&str> = option.split(':').collect();
    if strs.len() != 3 {
        bail!("Invalid option: {}", option);
    }
    Ok(OptionRemoteRoFileUnverified {
        ino: strs[0].parse::<Inode>()?,
        remote_id: strs[1].parse::<i32>()?,
        file_size: strs[2].parse::<u64>()?,
    })
}

fn parse_remote_new_rw_file_option(option: &str) -> Result<OptionRemoteRwFile> {
    let strs: Vec<&str> = option.split(':').collect();
    if strs.len() != 2 {
        bail!("Invalid option: {}", option);
    }
    Ok(OptionRemoteRwFile {
        ino: strs[0].parse::<Inode>().unwrap(),
        remote_id: strs[1].parse::<i32>().unwrap(),
    })
}

fn parse_local_file_ro_option(option: &str) -> Result<OptionLocalFileRo> {
    let strs: Vec<&str> = option.split(':').collect();
    if strs.len() != 5 {
        bail!("Invalid option: {}", option);
    }
    Ok(OptionLocalFileRo {
        ino: strs[0].parse::<Inode>()?,
        file_path: PathBuf::from(strs[1]),
        merkle_tree_dump_path: PathBuf::from(strs[2]),
        signature_path: PathBuf::from(strs[3]),
        _certificate_path: PathBuf::from(strs[4]),
    })
}

fn parse_local_ro_file_unverified_ro_option(option: &str) -> Result<OptionLocalRoFileUnverified> {
    let strs: Vec<&str> = option.split(':').collect();
    if strs.len() != 2 {
        bail!("Invalid option: {}", option);
    }
    Ok(OptionLocalRoFileUnverified {
        ino: strs[0].parse::<Inode>()?,
        file_path: PathBuf::from(strs[1]),
    })
}

fn new_config_remote_verified_file(remote_id: i32, file_size: u64) -> Result<FileConfig> {
    let service = file::get_local_binder();
    let signature = service.readFsveritySignature(remote_id).context("Failed to read signature")?;

    let service = Arc::new(Mutex::new(service));
    let authenticator = FakeAuthenticator::always_succeed();
    Ok(FileConfig::RemoteVerifiedReadonlyFile {
        reader: VerifiedFileReader::new(
            &authenticator,
            RemoteFileReader::new(Arc::clone(&service), remote_id),
            file_size,
            signature,
            RemoteMerkleTreeReader::new(Arc::clone(&service), remote_id),
        )?,
        file_size,
    })
}

fn new_config_remote_unverified_file(remote_id: i32, file_size: u64) -> Result<FileConfig> {
    let reader = RemoteFileReader::new(Arc::new(Mutex::new(file::get_local_binder())), remote_id);
    Ok(FileConfig::RemoteUnverifiedReadonlyFile { reader, file_size })
}

fn new_config_local_ro_file(
    protected_file: &Path,
    merkle_tree_dump: &Path,
    signature: &Path,
) -> Result<FileConfig> {
    let file = File::open(&protected_file)?;
    let file_size = file.metadata()?.len();
    let file_reader = LocalFileReader::new(file)?;
    let merkle_tree_reader = LocalFileReader::new(File::open(merkle_tree_dump)?)?;
    let authenticator = FakeAuthenticator::always_succeed();
    let mut sig = Vec::new();
    let _ = File::open(signature)?.read_to_end(&mut sig)?;
    let reader =
        VerifiedFileReader::new(&authenticator, file_reader, file_size, sig, merkle_tree_reader)?;
    Ok(FileConfig::LocalVerifiedReadonlyFile { reader, file_size })
}

fn new_config_local_ro_file_unverified(file_path: &Path) -> Result<FileConfig> {
    let reader = LocalFileReader::new(File::open(file_path)?)?;
    let file_size = reader.len();
    Ok(FileConfig::LocalUnverifiedReadonlyFile { reader, file_size })
}

fn new_config_remote_new_verified_file(remote_id: i32) -> Result<FileConfig> {
    let remote_file =
        RemoteFileEditor::new(Arc::new(Mutex::new(file::get_local_binder())), remote_id);
    Ok(FileConfig::RemoteVerifiedNewFile { editor: VerifiedFileEditor::new(remote_file) })
}

fn prepare_file_pool(args: &Args) -> Result<BTreeMap<Inode, FileConfig>> {
    let mut file_pool = BTreeMap::new();

    for config in &args.remote_ro_file {
        file_pool.insert(
            config.ino,
            new_config_remote_verified_file(config.remote_id, config.file_size)?,
        );
    }

    for config in &args.remote_ro_file_unverified {
        file_pool.insert(
            config.ino,
            new_config_remote_unverified_file(config.remote_id, config.file_size)?,
        );
    }

    for config in &args.remote_new_rw_file {
        file_pool.insert(config.ino, new_config_remote_new_verified_file(config.remote_id)?);
    }

    for config in &args.local_ro_file {
        file_pool.insert(
            config.ino,
            new_config_local_ro_file(
                &config.file_path,
                &config.merkle_tree_dump_path,
                &config.signature_path,
            )?,
        );
    }

    for config in &args.local_ro_file_unverified {
        file_pool.insert(config.ino, new_config_local_ro_file_unverified(&config.file_path)?);
    }

    Ok(file_pool)
}

fn main() -> Result<()> {
    let args = Args::from_args();

    let log_level = if args.debug { log::Level::Debug } else { log::Level::Info };
    android_logger::init_once(
        android_logger::Config::default().with_tag("authfs").with_min_level(log_level),
    );

    let file_pool = prepare_file_pool(&args)?;
    fusefs::loop_forever(file_pool, &args.mount_point)?;
    bail!("Unexpected exit after the handler loop")
}
