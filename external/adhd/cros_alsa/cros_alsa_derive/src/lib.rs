// Copyright 2020 The Chromium OS Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

//! `cros_alsa_derive` crate provides derive macros for cros_alsa.
//!

#![deny(missing_docs)]
extern crate proc_macro;

use proc_macro::TokenStream;

mod common;
mod control;
use self::control::impl_control_ops;

#[proc_macro_derive(ControlOps, attributes(cros_alsa))]
/// Derive macro generating an impl of the trait ControlOps.
/// To use this derive macro, users should hold `Ctl` and `ElemID` as `handle`
/// and `id` in their control structure.
pub fn control_ops_derive(input: TokenStream) -> TokenStream {
    match syn::parse(input) {
        Ok(ast) => impl_control_ops(&ast),
        Err(e) => e.to_compile_error().into(),
    }
}
