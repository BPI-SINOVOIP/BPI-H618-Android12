// Copyright 2020 The Chromium OS Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
extern crate bindgen;

use std::fs::File;
use std::io::Write;
use std::path::PathBuf;

fn main() {
    let bindings = bindgen::Builder::default()
        .header("wrapper.h")
        .derive_debug(false)
        .clang_arg("-I../../../sound-open-firmware-private/src/include")
        .whitelist_type("sof_abi_hdr")
        .whitelist_type("sof_ipc_ctrl_cmd")
        .generate()
        .expect("Unable to generate bindings");

    let header = b"// Copyright 2020 The Chromium OS Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
/*
 * generated from files in sound-open-firmware-private/src/include:
 * kernel/header.h
 * ipc/control.h
 */

";

    // Write the bindings to the $OUT_DIR/bindings.rs file.
    let out_path = PathBuf::from("../src").join("bindings.rs");

    let mut output_file =
        File::create(&out_path).expect(&format!("Couldn't create {:?}", out_path));
    output_file
        .write_all(header)
        .expect("Couldn't write header");
    output_file
        .write_all(bindings.to_string().as_bytes())
        .expect("Couldn't write bindings");
}
