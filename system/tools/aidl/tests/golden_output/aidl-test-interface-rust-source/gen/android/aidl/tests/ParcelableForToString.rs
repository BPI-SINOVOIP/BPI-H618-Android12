#[derive(Debug)]
pub struct ParcelableForToString {
  pub intValue: i32,
  pub intArray: Vec<i32>,
  pub longValue: i64,
  pub longArray: Vec<i64>,
  pub doubleValue: f64,
  pub doubleArray: Vec<f64>,
  pub floatValue: f32,
  pub floatArray: Vec<f32>,
  pub byteValue: i8,
  pub byteArray: Vec<u8>,
  pub booleanValue: bool,
  pub booleanArray: Vec<bool>,
  pub stringValue: String,
  pub stringArray: Vec<String>,
  pub stringList: Vec<String>,
  pub parcelableValue: crate::mangled::_7_android_4_aidl_5_tests_26_OtherParcelableForToString,
  pub parcelableArray: Vec<crate::mangled::_7_android_4_aidl_5_tests_26_OtherParcelableForToString>,
  pub enumValue: crate::mangled::_7_android_4_aidl_5_tests_7_IntEnum,
  pub enumArray: Vec<crate::mangled::_7_android_4_aidl_5_tests_7_IntEnum>,
  pub nullArray: Vec<String>,
  pub nullList: Vec<String>,
  pub parcelableGeneric: i32,
  pub unionValue: crate::mangled::_7_android_4_aidl_5_tests_5_Union,
}
pub(crate) mod mangled { pub use super::ParcelableForToString as _7_android_4_aidl_5_tests_21_ParcelableForToString; }
impl Default for ParcelableForToString {
  fn default() -> Self {
    Self {
      intValue: 0,
      intArray: Default::default(),
      longValue: 0,
      longArray: Default::default(),
      doubleValue: 0.000000f64,
      doubleArray: Default::default(),
      floatValue: 0.000000f32,
      floatArray: Default::default(),
      byteValue: 0,
      byteArray: Default::default(),
      booleanValue: false,
      booleanArray: Default::default(),
      stringValue: Default::default(),
      stringArray: Default::default(),
      stringList: Default::default(),
      parcelableValue: Default::default(),
      parcelableArray: Default::default(),
      enumValue: crate::mangled::_7_android_4_aidl_5_tests_7_IntEnum::FOO,
      enumArray: Default::default(),
      nullArray: Default::default(),
      nullList: Default::default(),
      parcelableGeneric: Default::default(),
      unionValue: Default::default(),
    }
  }
}
impl binder::parcel::Serialize for ParcelableForToString {
  fn serialize(&self, parcel: &mut binder::parcel::Parcel) -> binder::Result<()> {
    <Self as binder::parcel::SerializeOption>::serialize_option(Some(self), parcel)
  }
}
impl binder::parcel::SerializeArray for ParcelableForToString {}
impl binder::parcel::SerializeOption for ParcelableForToString {
  fn serialize_option(this: Option<&Self>, parcel: &mut binder::parcel::Parcel) -> binder::Result<()> {
    let this = if let Some(this) = this {
      parcel.write(&1i32)?;
      this
    } else {
      return parcel.write(&0i32);
    };
    parcel.sized_write(|subparcel| {
      subparcel.write(&this.intValue)?;
      subparcel.write(&this.intArray)?;
      subparcel.write(&this.longValue)?;
      subparcel.write(&this.longArray)?;
      subparcel.write(&this.doubleValue)?;
      subparcel.write(&this.doubleArray)?;
      subparcel.write(&this.floatValue)?;
      subparcel.write(&this.floatArray)?;
      subparcel.write(&this.byteValue)?;
      subparcel.write(&this.byteArray)?;
      subparcel.write(&this.booleanValue)?;
      subparcel.write(&this.booleanArray)?;
      subparcel.write(&this.stringValue)?;
      subparcel.write(&this.stringArray)?;
      subparcel.write(&this.stringList)?;
      subparcel.write(&this.parcelableValue)?;
      subparcel.write(&this.parcelableArray)?;
      subparcel.write(&this.enumValue)?;
      subparcel.write(&this.enumArray)?;
      subparcel.write(&this.nullArray)?;
      subparcel.write(&this.nullList)?;
      subparcel.write(&this.parcelableGeneric)?;
      subparcel.write(&this.unionValue)?;
      Ok(())
    })
  }
}
impl binder::parcel::Deserialize for ParcelableForToString {
  fn deserialize(parcel: &binder::parcel::Parcel) -> binder::Result<Self> {
    <Self as binder::parcel::DeserializeOption>::deserialize_option(parcel)
       .transpose()
       .unwrap_or(Err(binder::StatusCode::UNEXPECTED_NULL))
  }
}
impl binder::parcel::DeserializeArray for ParcelableForToString {}
impl binder::parcel::DeserializeOption for ParcelableForToString {
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
    if (parcel.get_data_position() - start_pos) == parcelable_size {
      return Ok(Some(result));
    }
    result.intValue = parcel.read()?;
    if (parcel.get_data_position() - start_pos) == parcelable_size {
      return Ok(Some(result));
    }
    result.intArray = parcel.read()?;
    if (parcel.get_data_position() - start_pos) == parcelable_size {
      return Ok(Some(result));
    }
    result.longValue = parcel.read()?;
    if (parcel.get_data_position() - start_pos) == parcelable_size {
      return Ok(Some(result));
    }
    result.longArray = parcel.read()?;
    if (parcel.get_data_position() - start_pos) == parcelable_size {
      return Ok(Some(result));
    }
    result.doubleValue = parcel.read()?;
    if (parcel.get_data_position() - start_pos) == parcelable_size {
      return Ok(Some(result));
    }
    result.doubleArray = parcel.read()?;
    if (parcel.get_data_position() - start_pos) == parcelable_size {
      return Ok(Some(result));
    }
    result.floatValue = parcel.read()?;
    if (parcel.get_data_position() - start_pos) == parcelable_size {
      return Ok(Some(result));
    }
    result.floatArray = parcel.read()?;
    if (parcel.get_data_position() - start_pos) == parcelable_size {
      return Ok(Some(result));
    }
    result.byteValue = parcel.read()?;
    if (parcel.get_data_position() - start_pos) == parcelable_size {
      return Ok(Some(result));
    }
    result.byteArray = parcel.read()?;
    if (parcel.get_data_position() - start_pos) == parcelable_size {
      return Ok(Some(result));
    }
    result.booleanValue = parcel.read()?;
    if (parcel.get_data_position() - start_pos) == parcelable_size {
      return Ok(Some(result));
    }
    result.booleanArray = parcel.read()?;
    if (parcel.get_data_position() - start_pos) == parcelable_size {
      return Ok(Some(result));
    }
    result.stringValue = parcel.read()?;
    if (parcel.get_data_position() - start_pos) == parcelable_size {
      return Ok(Some(result));
    }
    result.stringArray = parcel.read()?;
    if (parcel.get_data_position() - start_pos) == parcelable_size {
      return Ok(Some(result));
    }
    result.stringList = parcel.read()?;
    if (parcel.get_data_position() - start_pos) == parcelable_size {
      return Ok(Some(result));
    }
    result.parcelableValue = parcel.read()?;
    if (parcel.get_data_position() - start_pos) == parcelable_size {
      return Ok(Some(result));
    }
    result.parcelableArray = parcel.read()?;
    if (parcel.get_data_position() - start_pos) == parcelable_size {
      return Ok(Some(result));
    }
    result.enumValue = parcel.read()?;
    if (parcel.get_data_position() - start_pos) == parcelable_size {
      return Ok(Some(result));
    }
    result.enumArray = parcel.read()?;
    if (parcel.get_data_position() - start_pos) == parcelable_size {
      return Ok(Some(result));
    }
    result.nullArray = parcel.read()?;
    if (parcel.get_data_position() - start_pos) == parcelable_size {
      return Ok(Some(result));
    }
    result.nullList = parcel.read()?;
    if (parcel.get_data_position() - start_pos) == parcelable_size {
      return Ok(Some(result));
    }
    result.parcelableGeneric = parcel.read()?;
    if (parcel.get_data_position() - start_pos) == parcelable_size {
      return Ok(Some(result));
    }
    result.unionValue = parcel.read()?;
    unsafe {
      parcel.set_data_position(start_pos + parcelable_size)?;
    }
    Ok(Some(result))
  }
}
