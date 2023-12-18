#[derive(Debug)]
pub enum UnionWithFd {
  Num(i32),
  Pfd(Option<binder::parcel::ParcelFileDescriptor>),
}
pub(crate) mod mangled { pub use super::UnionWithFd as _7_android_4_aidl_5_tests_11_UnionWithFd; }
impl Default for UnionWithFd {
  fn default() -> Self {
    Self::Num(0)
  }
}
impl binder::parcel::Serialize for UnionWithFd {
  fn serialize(&self, parcel: &mut binder::parcel::Parcel) -> binder::Result<()> {
    <Self as binder::parcel::SerializeOption>::serialize_option(Some(self), parcel)
  }
}
impl binder::parcel::SerializeArray for UnionWithFd {}
impl binder::parcel::SerializeOption for UnionWithFd {
  fn serialize_option(this: Option<&Self>, parcel: &mut binder::parcel::Parcel) -> binder::Result<()> {
    let this = if let Some(this) = this {
      parcel.write(&1i32)?;
      this
    } else {
      return parcel.write(&0i32);
    };
    match this {
      Self::Num(v) => {
        parcel.write(&0i32)?;
        parcel.write(v)
      }
      Self::Pfd(v) => {
        parcel.write(&1i32)?;
        let __field_ref = v.as_ref().ok_or(binder::StatusCode::UNEXPECTED_NULL)?;
        parcel.write(__field_ref)
      }
    }
  }
}
impl binder::parcel::Deserialize for UnionWithFd {
  fn deserialize(parcel: &binder::parcel::Parcel) -> binder::Result<Self> {
    <Self as binder::parcel::DeserializeOption>::deserialize_option(parcel)
       .transpose()
       .unwrap_or(Err(binder::StatusCode::UNEXPECTED_NULL))
  }
}
impl binder::parcel::DeserializeArray for UnionWithFd {}
impl binder::parcel::DeserializeOption for UnionWithFd {
  fn deserialize_option(parcel: &binder::parcel::Parcel) -> binder::Result<Option<Self>> {
    let status: i32 = parcel.read()?;
    if status == 0 { return Ok(None); }
    let tag: i32 = parcel.read()?;
    match tag {
      0 => {
        let value: i32 = parcel.read()?;
        Ok(Some(Self::Num(value)))
      }
      1 => {
        let value: Option<binder::parcel::ParcelFileDescriptor> = Some(parcel.read()?);
        Ok(Some(Self::Pfd(value)))
      }
      _ => {
        Err(binder::StatusCode::BAD_VALUE)
      }
    }
  }
}
