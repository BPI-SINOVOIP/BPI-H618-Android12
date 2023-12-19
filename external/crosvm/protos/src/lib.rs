// Copyright 2019 The Chromium OS Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#[cfg(feature = "plugin")]
pub use crosvm_plugin_proto::plugin;

#[cfg(feature = "trunks")]
pub mod trunks;

#[cfg(feature = "composite-disk")]
pub use cdisk_spec_proto::cdisk_spec;
