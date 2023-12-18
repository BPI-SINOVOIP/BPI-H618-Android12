#[derive(Debug, Clone, PartialEq)]
pub enum Union {
  Ns(Vec<i32>),
  N(i32),
  M(i32),
  S(String),
  Ibinder(Option<binder::SpIBinder>),
  Ss(Vec<String>),
  Be(crate::mangled::_7_android_4_aidl_5_tests_8_ByteEnum),
}
pub const S1: &str = "a string constant in union";
pub(crate) mod mangled { pub use super::Union as _7_android_4_aidl_5_tests_5_Union; }
impl Default for Union {
  fn default() -> Self {
    Self::Ns(vec!{})
  }
}
impl binder::parcel::Serialize for Union {
  fn serialize(&self, parcel: &mut binder::parcel::Parcel) -> binder::Result<()> {
    <Self as binder::parcel::SerializeOption>::serialize_option(Some(self), parcel)
  }
}
impl binder::parcel::SerializeArray for Union {}
impl binder::parcel::SerializeOption for Union {
  fn serialize_option(this: Option<&Self>, parcel: &mut binder::parcel::Parcel) -> binder::Result<()> {
    let this = if let Some(this) = this {
      parcel.write(&1i32)?;
      this
    } else {
      return parcel.write(&0i32);
    };
    match this {
      Self::Ns(v) => {
        parcel.write(&0i32)?;
        parcel.write(v)
      }
      Self::N(v) => {
        parcel.write(&1i32)?;
        parcel.write(v)
      }
      Self::M(v) => {
        parcel.write(&2i32)?;
        parcel.write(v)
      }
      Self::S(v) => {
        parcel.write(&3i32)?;
        parcel.write(v)
      }
      Self::Ibinder(v) => {
        parcel.write(&4i32)?;
        parcel.write(v)
      }
      Self::Ss(v) => {
        parcel.write(&5i32)?;
        parcel.write(v)
      }
      Self::Be(v) => {
        parcel.write(&6i32)?;
        parcel.write(v)
      }
    }
  }
}
impl binder::parcel::Deserialize for Union {
  fn deserialize(parcel: &binder::parcel::Parcel) -> binder::Result<Self> {
    <Self as binder::parcel::DeserializeOption>::deserialize_option(parcel)
       .transpose()
       .unwrap_or(Err(binder::StatusCode::UNEXPECTED_NULL))
  }
}
impl binder::parcel::DeserializeArray for Union {}
impl binder::parcel::DeserializeOption for Union {
  fn deserialize_option(parcel: &binder::parcel::Parcel) -> binder::Result<Option<Self>> {
    let status: i32 = parcel.read()?;
    if status == 0 { return Ok(None); }
    let tag: i32 = parcel.read()?;
    match tag {
      0 => {
        let value: Vec<i32> = parcel.read()?;
        Ok(Some(Self::Ns(value)))
      }
      1 => {
        let value: i32 = parcel.read()?;
        Ok(Some(Self::N(value)))
      }
      2 => {
        let value: i32 = parcel.read()?;
        Ok(Some(Self::M(value)))
      }
      3 => {
        let value: String = parcel.read()?;
        Ok(Some(Self::S(value)))
      }
      4 => {
        let value: Option<binder::SpIBinder> = parcel.read()?;
        Ok(Some(Self::Ibinder(value)))
      }
      5 => {
        let value: Vec<String> = parcel.read()?;
        Ok(Some(Self::Ss(value)))
      }
      6 => {
        let value: crate::mangled::_7_android_4_aidl_5_tests_8_ByteEnum = parcel.read()?;
        Ok(Some(Self::Be(value)))
      }
      _ => {
        Err(binder::StatusCode::BAD_VALUE)
      }
    }
  }
}
