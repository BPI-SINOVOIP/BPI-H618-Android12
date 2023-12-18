#![allow(non_upper_case_globals)]
#![allow(non_snake_case)]
#[allow(unused_imports)] use binder::IBinderInternal;
use binder::declare_binder_interface;
declare_binder_interface! {
  INewName["android.aidl.tests.IOldName"] {
    native: BnNewName(on_transact),
    proxy: BpNewName {
    },
  }
}
pub trait INewName: binder::Interface + Send {
  fn get_descriptor() -> &'static str where Self: Sized { "android.aidl.tests.IOldName" }
  fn RealName(&self) -> binder::public_api::Result<String>;
  fn getDefaultImpl() -> INewNameDefaultRef where Self: Sized {
    DEFAULT_IMPL.lock().unwrap().clone()
  }
  fn setDefaultImpl(d: INewNameDefaultRef) -> INewNameDefaultRef where Self: Sized {
    std::mem::replace(&mut *DEFAULT_IMPL.lock().unwrap(), d)
  }
}
pub trait INewNameDefault: Send + Sync {
  fn RealName(&self) -> binder::public_api::Result<String> {
    Err(binder::StatusCode::UNKNOWN_TRANSACTION.into())
  }
}
pub mod transactions {
  pub const RealName: binder::TransactionCode = binder::FIRST_CALL_TRANSACTION + 0;
}
pub type INewNameDefaultRef = Option<std::sync::Arc<dyn INewNameDefault>>;
use lazy_static::lazy_static;
lazy_static! {
  static ref DEFAULT_IMPL: std::sync::Mutex<INewNameDefaultRef> = std::sync::Mutex::new(None);
}
pub(crate) mod mangled { pub use super::INewName as _7_android_4_aidl_5_tests_8_INewName; }
impl INewName for BpNewName {
  fn RealName(&self) -> binder::public_api::Result<String> {
    let _aidl_reply = self.binder.transact(transactions::RealName, binder::FLAG_PRIVATE_LOCAL, |_aidl_data| {
      Ok(())
    });
    if let Err(binder::StatusCode::UNKNOWN_TRANSACTION) = _aidl_reply {
      if let Some(_aidl_default_impl) = <Self as INewName>::getDefaultImpl() {
        return _aidl_default_impl.RealName();
      }
    }
    let _aidl_reply = _aidl_reply?;
    let _aidl_status: binder::Status = _aidl_reply.read()?;
    if !_aidl_status.is_ok() { return Err(_aidl_status); }
    let _aidl_return: String = _aidl_reply.read()?;
    Ok(_aidl_return)
  }
}
impl INewName for binder::Binder<BnNewName> {
  fn RealName(&self) -> binder::public_api::Result<String> { self.0.RealName() }
}
fn on_transact(_aidl_service: &dyn INewName, _aidl_code: binder::TransactionCode, _aidl_data: &binder::parcel::Parcel, _aidl_reply: &mut binder::parcel::Parcel) -> binder::Result<()> {
  match _aidl_code {
    transactions::RealName => {
      let _aidl_return = _aidl_service.RealName();
      match &_aidl_return {
        Ok(_aidl_return) => {
          _aidl_reply.write(&binder::Status::from(binder::StatusCode::OK))?;
          _aidl_reply.write(_aidl_return)?;
        }
        Err(_aidl_status) => _aidl_reply.write(_aidl_status)?
      }
      Ok(())
    }
    _ => Err(binder::StatusCode::UNKNOWN_TRANSACTION)
  }
}
