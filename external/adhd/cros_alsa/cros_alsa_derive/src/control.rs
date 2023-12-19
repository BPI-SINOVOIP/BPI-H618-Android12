// Copyright 2020 The Chromium OS Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

//! This mod provides derive macros for cros_alsa::control.

use proc_macro::TokenStream;
use proc_macro2::Span;
use quote::quote;

use crate::common::{parse_cros_alsa_attr, CrosAlsaAttr};

/// The provide default implementation for `ControlOps`.
/// Users could hold `Ctl` and `ElemID` as `handle` and `id` in their control structure and use
/// `#[derive(ControlOps)]` macro to generate default load / save implementations.
pub fn impl_control_ops(ast: &syn::DeriveInput) -> TokenStream {
    let name = &ast.ident;
    let (impl_generics, ty_generics, where_clause) = ast.generics.split_for_impl();
    let attrs = match parse_cros_alsa_attr(&ast.attrs) {
        Ok(attrs) => attrs,
        Err(e) => return e.to_compile_error().into(),
    };
    let path = match attrs.iter().find(|x| x.is_path()) {
        Some(CrosAlsaAttr::Path(path)) => path.clone(),
        None => syn::LitStr::new("cros_alsa", Span::call_site())
            .parse()
            .expect("failed to create a default path for derive macro: ControlOps"),
    };
    let gen = quote! {
        impl #impl_generics #path::ControlOps #impl_generics for #name #ty_generics #where_clause {
            fn load(&mut self) -> ::std::result::Result<<Self as #path::Control #impl_generics>::Item, #path::ControlError> {
                Ok(<Self as #path::Control>::Item::load(self.handle, &self.id)?)
            }
            fn save(&mut self, val: <Self as #path::Control #impl_generics>::Item) -> ::std::result::Result<bool, #path::ControlError> {
                Ok(<Self as #path::Control>::Item::save(self.handle, &self.id, val)?)
            }
        }
    };
    gen.into()
}
