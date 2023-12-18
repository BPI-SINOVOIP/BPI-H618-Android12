#![allow(non_upper_case_globals)]
#![allow(non_snake_case)]
#[allow(unused_imports)] use binder::IBinderInternal;
use binder::declare_binder_interface;
declare_binder_interface! {
  IDeprecated["android.aidl.tests.IDeprecated"] {
    native: BnDeprecated(on_transact),
    proxy: BpDeprecated {
    },
  }
}
#[deprecated = "test"]
pub trait IDeprecated: binder::Interface + Send {
  fn get_descriptor() -> &'static str where Self: Sized { "android.aidl.tests.IDeprecated" }
  fn getDefaultImpl() -> IDeprecatedDefaultRef where Self: Sized {
    DEFAULT_IMPL.lock().unwrap().clone()
  }
  fn setDefaultImpl(d: IDeprecatedDefaultRef) -> IDeprecatedDefaultRef where Self: Sized {
    std::mem::replace(&mut *DEFAULT_IMPL.lock().unwrap(), d)
  }
}
pub trait IDeprecatedDefault: Send + Sync {
}
pub mod transactions {
}
pub type IDeprecatedDefaultRef = Option<std::sync::Arc<dyn IDeprecatedDefault>>;
use lazy_static::lazy_static;
lazy_static! {
  static ref DEFAULT_IMPL: std::sync::Mutex<IDeprecatedDefaultRef> = std::sync::Mutex::new(None);
}
pub(crate) mod mangled { pub use super::IDeprecated as _7_android_4_aidl_5_tests_11_IDeprecated; }
impl IDeprecated for BpDeprecated {
}
impl IDeprecated for binder::Binder<BnDeprecated> {
}
fn on_transact(_aidl_service: &dyn IDeprecated, _aidl_code: binder::TransactionCode, _aidl_data: &binder::parcel::Parcel, _aidl_reply: &mut binder::parcel::Parcel) -> binder::Result<()> {
  match _aidl_code {
    _ => Err(binder::StatusCode::UNKNOWN_TRANSACTION)
  }
}
