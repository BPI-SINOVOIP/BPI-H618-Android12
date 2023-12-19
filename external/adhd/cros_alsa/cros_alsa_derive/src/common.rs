// Copyright 2020 The Chromium OS Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

//! This mod provides common constants, structs and functions used across cros_alsa_derive.
use std::convert::TryFrom;

use syn::{Lit, Meta, NestedMeta, Result};

/// Attribute name of cros_alsa.
const CROS_ALSA: &str = "cros_alsa";

/// const for `#[cros_alsa(path = "...")]`
const PATH: &str = "path";

/// Possible `#[cros_alsa("...")]` derive macro helper attributes.
pub enum CrosAlsaAttr {
    /// Use `#[cros_alsa(path = "crate")]` to replace the crate path of cros_alsa in derive macros.
    Path(syn::Path),
}

impl CrosAlsaAttr {
    /// Return true if the value is a `Path` variant.
    pub fn is_path(&self) -> bool {
        use CrosAlsaAttr::*;
        match self {
            Path(_) => true,
            // suppress unreachable_patterns warning because there is only one CrosAlsaAttr variant.
            #[allow(unreachable_patterns)]
            _ => false,
        }
    }
}

impl TryFrom<syn::NestedMeta> for CrosAlsaAttr {
    type Error = syn::Error;
    fn try_from(meta_item: NestedMeta) -> Result<CrosAlsaAttr> {
        match meta_item {
            // Parse `#[cros_alsa(path = "crate")]`
            NestedMeta::Meta(Meta::NameValue(m)) if m.path.is_ident(PATH) => {
                if let Lit::Str(lit_str) = &m.lit {
                    return Ok(CrosAlsaAttr::Path(lit_str.parse()?));
                }
                Err(syn::Error::new_spanned(
                    &m.lit,
                    "expected a valid path for cros_alsa_derive::CrosAlsaAttr::Path",
                ))
            }
            _ => Err(syn::Error::new_spanned(
                meta_item,
                "unrecognized cros_alsa_derive::CrosAlsaAttr",
            )),
        }
    }
}

/// Parses `#[cros_alsa(path = "...")]` into a list of `CrosAlsaAttr`.
/// It's used to replace the crate path of cros_alsa in derive macros.
pub fn parse_cros_alsa_attr(attrs: &[syn::Attribute]) -> Result<Vec<CrosAlsaAttr>> {
    attrs
        .iter()
        .flat_map(|attr| get_cros_alsa_meta_items(attr))
        .flatten()
        .map(CrosAlsaAttr::try_from)
        .collect()
}

/// Parses and collects `NestedMeta` under `cros_alsa` attribute.
fn get_cros_alsa_meta_items(attr: &syn::Attribute) -> Result<Vec<syn::NestedMeta>> {
    if !attr.path.is_ident(CROS_ALSA) {
        return Ok(Vec::new());
    }
    match attr.parse_meta() {
        Ok(Meta::List(meta)) => Ok(meta.nested.into_iter().collect()),
        _ => Err(syn::Error::new_spanned(attr, "expected #[cros_alsa(...)]")),
    }
}
