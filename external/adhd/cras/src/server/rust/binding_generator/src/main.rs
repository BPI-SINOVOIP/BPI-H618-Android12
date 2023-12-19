// Copyright 2019 The Chromium OS Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
extern crate cbindgen;

use cbindgen::Builder;

const HEADER: &str = "// Copyright 2019 The Chromium OS Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// Generated from files in cras/src/server/rust/src in adhd.";

fn main() {
    Builder::new()
        .with_src("../src/rate_estimator.rs")
        .rename_item("RateEstimator", "rate_estimator")
        .rename_item("timespec", "struct timespec")
        .with_no_includes()
        .with_sys_include("time.h")
        .with_include_guard("RATE_ESTIMATOR_H_")
        .with_language(cbindgen::Language::C)
        .with_header(HEADER)
        .generate()
        .expect("Unable to generate bindings")
        .write_to_file("../src/headers/rate_estimator.h");
}
