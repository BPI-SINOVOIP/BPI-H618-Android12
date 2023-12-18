#[derive(Debug)]
#[deprecated = "test"]
pub struct DeprecatedParcelable {
}
pub(crate) mod mangled { pub use super::DeprecatedParcelable as _7_android_4_aidl_5_tests_20_DeprecatedParcelable; }
impl Default for DeprecatedParcelable {
  fn default() -> Self {
    Self {
    }
  }
}
impl binder::parcel::Serialize for DeprecatedParcelable {
  fn serialize(&self, parcel: &mut binder::parcel::Parcel) -> binder::Result<()> {
    <Self as binder::parcel::SerializeOption>::serialize_option(Some(self), parcel)
  }
}
impl binder::parcel::SerializeArray for DeprecatedParcelable {}
impl binder::parcel::SerializeOption for DeprecatedParcelable {
  fn serialize_option(this: Option<&Self>, parcel: &mut binder::parcel::Parcel) -> binder::Result<()> {
    let this = if let Some(this) = this {
      parcel.write(&1i32)?;
      this
    } else {
      return parcel.write(&0i32);
    };
    parcel.sized_write(|subparcel| {
      Ok(())
    })
  }
}
impl binder::parcel::Deserialize for DeprecatedParcelable {
  fn deserialize(parcel: &binder::parcel::Parcel) -> binder::Result<Self> {
    <Self as binder::parcel::DeserializeOption>::deserialize_option(parcel)
       .transpose()
       .unwrap_or(Err(binder::StatusCode::UNEXPECTED_NULL))
  }
}
impl binder::parcel::DeserializeArray for DeprecatedParcelable {}
impl binder::parcel::DeserializeOption for DeprecatedParcelable {
  fn deserialize_option(parcel: &binder::parcel::Parcel) -> binder::Result<Option<Self>> {
    let status: i32 = parcel.read()?;
    if status == 0 { return Ok(None); }
    let start_pos = parcel.get_data_position();
    let parcelable_size: i32 = parcel.read()?;
    if parcelable_size < 0 { return Err(binder::StatusCode::BAD_VALUE); }
    if start_pos.checked_add(parcelable_size).is_none() {
      return Err(binder::StatusCode::BAD_VALUE);
    }
    let mut result = Self::default();
    unsafe {
      parcel.set_data_position(start_pos + parcelable_size)?;
    }
    Ok(Some(result))
  }
}
