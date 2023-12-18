#[derive(Debug, Clone, PartialEq)]
pub enum EnumUnion {
  IntEnum(crate::mangled::_7_android_4_aidl_5_tests_7_IntEnum),
  LongEnum(crate::mangled::_7_android_4_aidl_5_tests_8_LongEnum),
}
pub(crate) mod mangled { pub use super::EnumUnion as _7_android_4_aidl_5_tests_6_unions_9_EnumUnion; }
impl Default for EnumUnion {
  fn default() -> Self {
    Self::IntEnum(crate::mangled::_7_android_4_aidl_5_tests_7_IntEnum::FOO)
  }
}
impl binder::parcel::Serialize for EnumUnion {
  fn serialize(&self, parcel: &mut binder::parcel::Parcel) -> binder::Result<()> {
    <Self as binder::parcel::SerializeOption>::serialize_option(Some(self), parcel)
  }
}
impl binder::parcel::SerializeArray for EnumUnion {}
impl binder::parcel::SerializeOption for EnumUnion {
  fn serialize_option(this: Option<&Self>, parcel: &mut binder::parcel::Parcel) -> binder::Result<()> {
    let this = if let Some(this) = this {
      parcel.write(&1i32)?;
      this
    } else {
      return parcel.write(&0i32);
    };
    match this {
      Self::IntEnum(v) => {
        parcel.write(&0i32)?;
        parcel.write(v)
      }
      Self::LongEnum(v) => {
        parcel.write(&1i32)?;
        parcel.write(v)
      }
    }
  }
}
impl binder::parcel::Deserialize for EnumUnion {
  fn deserialize(parcel: &binder::parcel::Parcel) -> binder::Result<Self> {
    <Self as binder::parcel::DeserializeOption>::deserialize_option(parcel)
       .transpose()
       .unwrap_or(Err(binder::StatusCode::UNEXPECTED_NULL))
  }
}
impl binder::parcel::DeserializeArray for EnumUnion {}
impl binder::parcel::DeserializeOption for EnumUnion {
  fn deserialize_option(parcel: &binder::parcel::Parcel) -> binder::Result<Option<Self>> {
    let status: i32 = parcel.read()?;
    if status == 0 { return Ok(None); }
    let tag: i32 = parcel.read()?;
    match tag {
      0 => {
        let value: crate::mangled::_7_android_4_aidl_5_tests_7_IntEnum = parcel.read()?;
        Ok(Some(Self::IntEnum(value)))
      }
      1 => {
        let value: crate::mangled::_7_android_4_aidl_5_tests_8_LongEnum = parcel.read()?;
        Ok(Some(Self::LongEnum(value)))
      }
      _ => {
        Err(binder::StatusCode::BAD_VALUE)
      }
    }
  }
}
