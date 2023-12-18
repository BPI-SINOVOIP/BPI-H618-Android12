#[derive(Debug, Clone, PartialEq)]
pub struct StructuredParcelable {
  pub shouldContainThreeFs: Vec<i32>,
  pub f: i32,
  pub shouldBeJerry: String,
  pub shouldBeByteBar: crate::mangled::_7_android_4_aidl_5_tests_8_ByteEnum,
  pub shouldBeIntBar: crate::mangled::_7_android_4_aidl_5_tests_7_IntEnum,
  pub shouldBeLongBar: crate::mangled::_7_android_4_aidl_5_tests_8_LongEnum,
  pub shouldContainTwoByteFoos: Vec<crate::mangled::_7_android_4_aidl_5_tests_8_ByteEnum>,
  pub shouldContainTwoIntFoos: Vec<crate::mangled::_7_android_4_aidl_5_tests_7_IntEnum>,
  pub shouldContainTwoLongFoos: Vec<crate::mangled::_7_android_4_aidl_5_tests_8_LongEnum>,
  pub stringDefaultsToFoo: String,
  pub byteDefaultsToFour: i8,
  pub intDefaultsToFive: i32,
  pub longDefaultsToNegativeSeven: i64,
  pub booleanDefaultsToTrue: bool,
  pub charDefaultsToC: u16,
  pub floatDefaultsToPi: f32,
  pub doubleWithDefault: f64,
  pub arrayDefaultsTo123: Vec<i32>,
  pub arrayDefaultsToEmpty: Vec<i32>,
  pub boolDefault: bool,
  pub byteDefault: i8,
  pub intDefault: i32,
  pub longDefault: i64,
  pub floatDefault: f32,
  pub doubleDefault: f64,
  pub checkDoubleFromFloat: f64,
  pub checkStringArray1: Vec<String>,
  pub checkStringArray2: Vec<String>,
  pub int32_min: i32,
  pub int32_max: i32,
  pub int64_max: i64,
  pub hexInt32_neg_1: i32,
  pub ibinder: Option<binder::SpIBinder>,
  pub int32_1: Vec<i32>,
  pub int64_1: Vec<i64>,
  pub hexInt32_pos_1: i32,
  pub hexInt64_pos_1: i32,
  pub const_exprs_1: crate::mangled::_7_android_4_aidl_5_tests_22_ConstantExpressionEnum,
  pub const_exprs_2: crate::mangled::_7_android_4_aidl_5_tests_22_ConstantExpressionEnum,
  pub const_exprs_3: crate::mangled::_7_android_4_aidl_5_tests_22_ConstantExpressionEnum,
  pub const_exprs_4: crate::mangled::_7_android_4_aidl_5_tests_22_ConstantExpressionEnum,
  pub const_exprs_5: crate::mangled::_7_android_4_aidl_5_tests_22_ConstantExpressionEnum,
  pub const_exprs_6: crate::mangled::_7_android_4_aidl_5_tests_22_ConstantExpressionEnum,
  pub const_exprs_7: crate::mangled::_7_android_4_aidl_5_tests_22_ConstantExpressionEnum,
  pub const_exprs_8: crate::mangled::_7_android_4_aidl_5_tests_22_ConstantExpressionEnum,
  pub const_exprs_9: crate::mangled::_7_android_4_aidl_5_tests_22_ConstantExpressionEnum,
  pub const_exprs_10: crate::mangled::_7_android_4_aidl_5_tests_22_ConstantExpressionEnum,
  pub addString1: String,
  pub addString2: String,
  pub shouldSetBit0AndBit2: i32,
  pub u: Option<crate::mangled::_7_android_4_aidl_5_tests_5_Union>,
  pub shouldBeConstS1: Option<crate::mangled::_7_android_4_aidl_5_tests_5_Union>,
  pub defaultWithFoo: crate::mangled::_7_android_4_aidl_5_tests_7_IntEnum,
}
pub const BIT0: i32 = 1;
pub const BIT1: i32 = 2;
pub const BIT2: i32 = 4;
pub(crate) mod mangled { pub use super::StructuredParcelable as _7_android_4_aidl_5_tests_20_StructuredParcelable; }
impl Default for StructuredParcelable {
  fn default() -> Self {
    Self {
      shouldContainThreeFs: Default::default(),
      f: 0,
      shouldBeJerry: Default::default(),
      shouldBeByteBar: Default::default(),
      shouldBeIntBar: Default::default(),
      shouldBeLongBar: Default::default(),
      shouldContainTwoByteFoos: Default::default(),
      shouldContainTwoIntFoos: Default::default(),
      shouldContainTwoLongFoos: Default::default(),
      stringDefaultsToFoo: "foo".into(),
      byteDefaultsToFour: 4,
      intDefaultsToFive: 5,
      longDefaultsToNegativeSeven: -7,
      booleanDefaultsToTrue: true,
      charDefaultsToC: 'C' as u16,
      floatDefaultsToPi: 3.140000f32,
      doubleWithDefault: -314000000000000000.000000f64,
      arrayDefaultsTo123: vec!{1, 2, 3},
      arrayDefaultsToEmpty: vec!{},
      boolDefault: false,
      byteDefault: 0,
      intDefault: 0,
      longDefault: 0,
      floatDefault: 0.000000f32,
      doubleDefault: 0.000000f64,
      checkDoubleFromFloat: 3.140000f64,
      checkStringArray1: vec!{"a".into(), "b".into()},
      checkStringArray2: vec!{"a".into(), "b".into()},
      int32_min: -2147483648,
      int32_max: 2147483647,
      int64_max: 9223372036854775807,
      hexInt32_neg_1: -1,
      ibinder: Default::default(),
      int32_1: vec!{1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1},
      int64_1: vec!{1, 1, 1, 1, 1, 1, 1, 1, 1, 1},
      hexInt32_pos_1: 1,
      hexInt64_pos_1: 1,
      const_exprs_1: Default::default(),
      const_exprs_2: Default::default(),
      const_exprs_3: Default::default(),
      const_exprs_4: Default::default(),
      const_exprs_5: Default::default(),
      const_exprs_6: Default::default(),
      const_exprs_7: Default::default(),
      const_exprs_8: Default::default(),
      const_exprs_9: Default::default(),
      const_exprs_10: Default::default(),
      addString1: "hello world!".into(),
      addString2: "The quick brown fox jumps over the lazy dog.".into(),
      shouldSetBit0AndBit2: 0,
      u: Default::default(),
      shouldBeConstS1: Default::default(),
      defaultWithFoo: crate::mangled::_7_android_4_aidl_5_tests_7_IntEnum::FOO,
    }
  }
}
impl binder::parcel::Serialize for StructuredParcelable {
  fn serialize(&self, parcel: &mut binder::parcel::Parcel) -> binder::Result<()> {
    <Self as binder::parcel::SerializeOption>::serialize_option(Some(self), parcel)
  }
}
impl binder::parcel::SerializeArray for StructuredParcelable {}
impl binder::parcel::SerializeOption for StructuredParcelable {
  fn serialize_option(this: Option<&Self>, parcel: &mut binder::parcel::Parcel) -> binder::Result<()> {
    let this = if let Some(this) = this {
      parcel.write(&1i32)?;
      this
    } else {
      return parcel.write(&0i32);
    };
    parcel.sized_write(|subparcel| {
      subparcel.write(&this.shouldContainThreeFs)?;
      subparcel.write(&this.f)?;
      subparcel.write(&this.shouldBeJerry)?;
      subparcel.write(&this.shouldBeByteBar)?;
      subparcel.write(&this.shouldBeIntBar)?;
      subparcel.write(&this.shouldBeLongBar)?;
      subparcel.write(&this.shouldContainTwoByteFoos)?;
      subparcel.write(&this.shouldContainTwoIntFoos)?;
      subparcel.write(&this.shouldContainTwoLongFoos)?;
      subparcel.write(&this.stringDefaultsToFoo)?;
      subparcel.write(&this.byteDefaultsToFour)?;
      subparcel.write(&this.intDefaultsToFive)?;
      subparcel.write(&this.longDefaultsToNegativeSeven)?;
      subparcel.write(&this.booleanDefaultsToTrue)?;
      subparcel.write(&this.charDefaultsToC)?;
      subparcel.write(&this.floatDefaultsToPi)?;
      subparcel.write(&this.doubleWithDefault)?;
      subparcel.write(&this.arrayDefaultsTo123)?;
      subparcel.write(&this.arrayDefaultsToEmpty)?;
      subparcel.write(&this.boolDefault)?;
      subparcel.write(&this.byteDefault)?;
      subparcel.write(&this.intDefault)?;
      subparcel.write(&this.longDefault)?;
      subparcel.write(&this.floatDefault)?;
      subparcel.write(&this.doubleDefault)?;
      subparcel.write(&this.checkDoubleFromFloat)?;
      subparcel.write(&this.checkStringArray1)?;
      subparcel.write(&this.checkStringArray2)?;
      subparcel.write(&this.int32_min)?;
      subparcel.write(&this.int32_max)?;
      subparcel.write(&this.int64_max)?;
      subparcel.write(&this.hexInt32_neg_1)?;
      subparcel.write(&this.ibinder)?;
      subparcel.write(&this.int32_1)?;
      subparcel.write(&this.int64_1)?;
      subparcel.write(&this.hexInt32_pos_1)?;
      subparcel.write(&this.hexInt64_pos_1)?;
      subparcel.write(&this.const_exprs_1)?;
      subparcel.write(&this.const_exprs_2)?;
      subparcel.write(&this.const_exprs_3)?;
      subparcel.write(&this.const_exprs_4)?;
      subparcel.write(&this.const_exprs_5)?;
      subparcel.write(&this.const_exprs_6)?;
      subparcel.write(&this.const_exprs_7)?;
      subparcel.write(&this.const_exprs_8)?;
      subparcel.write(&this.const_exprs_9)?;
      subparcel.write(&this.const_exprs_10)?;
      subparcel.write(&this.addString1)?;
      subparcel.write(&this.addString2)?;
      subparcel.write(&this.shouldSetBit0AndBit2)?;
      subparcel.write(&this.u)?;
      subparcel.write(&this.shouldBeConstS1)?;
      subparcel.write(&this.defaultWithFoo)?;
      Ok(())
    })
  }
}
impl binder::parcel::Deserialize for StructuredParcelable {
  fn deserialize(parcel: &binder::parcel::Parcel) -> binder::Result<Self> {
    <Self as binder::parcel::DeserializeOption>::deserialize_option(parcel)
       .transpose()
       .unwrap_or(Err(binder::StatusCode::UNEXPECTED_NULL))
  }
}
impl binder::parcel::DeserializeArray for StructuredParcelable {}
impl binder::parcel::DeserializeOption for StructuredParcelable {
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
    result.shouldContainThreeFs = parcel.read()?;
    if (parcel.get_data_position() - start_pos) == parcelable_size {
      return Ok(Some(result));
    }
    result.f = parcel.read()?;
    if (parcel.get_data_position() - start_pos) == parcelable_size {
      return Ok(Some(result));
    }
    result.shouldBeJerry = parcel.read()?;
    if (parcel.get_data_position() - start_pos) == parcelable_size {
      return Ok(Some(result));
    }
    result.shouldBeByteBar = parcel.read()?;
    if (parcel.get_data_position() - start_pos) == parcelable_size {
      return Ok(Some(result));
    }
    result.shouldBeIntBar = parcel.read()?;
    if (parcel.get_data_position() - start_pos) == parcelable_size {
      return Ok(Some(result));
    }
    result.shouldBeLongBar = parcel.read()?;
    if (parcel.get_data_position() - start_pos) == parcelable_size {
      return Ok(Some(result));
    }
    result.shouldContainTwoByteFoos = parcel.read()?;
    if (parcel.get_data_position() - start_pos) == parcelable_size {
      return Ok(Some(result));
    }
    result.shouldContainTwoIntFoos = parcel.read()?;
    if (parcel.get_data_position() - start_pos) == parcelable_size {
      return Ok(Some(result));
    }
    result.shouldContainTwoLongFoos = parcel.read()?;
    if (parcel.get_data_position() - start_pos) == parcelable_size {
      return Ok(Some(result));
    }
    result.stringDefaultsToFoo = parcel.read()?;
    if (parcel.get_data_position() - start_pos) == parcelable_size {
      return Ok(Some(result));
    }
    result.byteDefaultsToFour = parcel.read()?;
    if (parcel.get_data_position() - start_pos) == parcelable_size {
      return Ok(Some(result));
    }
    result.intDefaultsToFive = parcel.read()?;
    if (parcel.get_data_position() - start_pos) == parcelable_size {
      return Ok(Some(result));
    }
    result.longDefaultsToNegativeSeven = parcel.read()?;
    if (parcel.get_data_position() - start_pos) == parcelable_size {
      return Ok(Some(result));
    }
    result.booleanDefaultsToTrue = parcel.read()?;
    if (parcel.get_data_position() - start_pos) == parcelable_size {
      return Ok(Some(result));
    }
    result.charDefaultsToC = parcel.read()?;
    if (parcel.get_data_position() - start_pos) == parcelable_size {
      return Ok(Some(result));
    }
    result.floatDefaultsToPi = parcel.read()?;
    if (parcel.get_data_position() - start_pos) == parcelable_size {
      return Ok(Some(result));
    }
    result.doubleWithDefault = parcel.read()?;
    if (parcel.get_data_position() - start_pos) == parcelable_size {
      return Ok(Some(result));
    }
    result.arrayDefaultsTo123 = parcel.read()?;
    if (parcel.get_data_position() - start_pos) == parcelable_size {
      return Ok(Some(result));
    }
    result.arrayDefaultsToEmpty = parcel.read()?;
    if (parcel.get_data_position() - start_pos) == parcelable_size {
      return Ok(Some(result));
    }
    result.boolDefault = parcel.read()?;
    if (parcel.get_data_position() - start_pos) == parcelable_size {
      return Ok(Some(result));
    }
    result.byteDefault = parcel.read()?;
    if (parcel.get_data_position() - start_pos) == parcelable_size {
      return Ok(Some(result));
    }
    result.intDefault = parcel.read()?;
    if (parcel.get_data_position() - start_pos) == parcelable_size {
      return Ok(Some(result));
    }
    result.longDefault = parcel.read()?;
    if (parcel.get_data_position() - start_pos) == parcelable_size {
      return Ok(Some(result));
    }
    result.floatDefault = parcel.read()?;
    if (parcel.get_data_position() - start_pos) == parcelable_size {
      return Ok(Some(result));
    }
    result.doubleDefault = parcel.read()?;
    if (parcel.get_data_position() - start_pos) == parcelable_size {
      return Ok(Some(result));
    }
    result.checkDoubleFromFloat = parcel.read()?;
    if (parcel.get_data_position() - start_pos) == parcelable_size {
      return Ok(Some(result));
    }
    result.checkStringArray1 = parcel.read()?;
    if (parcel.get_data_position() - start_pos) == parcelable_size {
      return Ok(Some(result));
    }
    result.checkStringArray2 = parcel.read()?;
    if (parcel.get_data_position() - start_pos) == parcelable_size {
      return Ok(Some(result));
    }
    result.int32_min = parcel.read()?;
    if (parcel.get_data_position() - start_pos) == parcelable_size {
      return Ok(Some(result));
    }
    result.int32_max = parcel.read()?;
    if (parcel.get_data_position() - start_pos) == parcelable_size {
      return Ok(Some(result));
    }
    result.int64_max = parcel.read()?;
    if (parcel.get_data_position() - start_pos) == parcelable_size {
      return Ok(Some(result));
    }
    result.hexInt32_neg_1 = parcel.read()?;
    if (parcel.get_data_position() - start_pos) == parcelable_size {
      return Ok(Some(result));
    }
    result.ibinder = parcel.read()?;
    if (parcel.get_data_position() - start_pos) == parcelable_size {
      return Ok(Some(result));
    }
    result.int32_1 = parcel.read()?;
    if (parcel.get_data_position() - start_pos) == parcelable_size {
      return Ok(Some(result));
    }
    result.int64_1 = parcel.read()?;
    if (parcel.get_data_position() - start_pos) == parcelable_size {
      return Ok(Some(result));
    }
    result.hexInt32_pos_1 = parcel.read()?;
    if (parcel.get_data_position() - start_pos) == parcelable_size {
      return Ok(Some(result));
    }
    result.hexInt64_pos_1 = parcel.read()?;
    if (parcel.get_data_position() - start_pos) == parcelable_size {
      return Ok(Some(result));
    }
    result.const_exprs_1 = parcel.read()?;
    if (parcel.get_data_position() - start_pos) == parcelable_size {
      return Ok(Some(result));
    }
    result.const_exprs_2 = parcel.read()?;
    if (parcel.get_data_position() - start_pos) == parcelable_size {
      return Ok(Some(result));
    }
    result.const_exprs_3 = parcel.read()?;
    if (parcel.get_data_position() - start_pos) == parcelable_size {
      return Ok(Some(result));
    }
    result.const_exprs_4 = parcel.read()?;
    if (parcel.get_data_position() - start_pos) == parcelable_size {
      return Ok(Some(result));
    }
    result.const_exprs_5 = parcel.read()?;
    if (parcel.get_data_position() - start_pos) == parcelable_size {
      return Ok(Some(result));
    }
    result.const_exprs_6 = parcel.read()?;
    if (parcel.get_data_position() - start_pos) == parcelable_size {
      return Ok(Some(result));
    }
    result.const_exprs_7 = parcel.read()?;
    if (parcel.get_data_position() - start_pos) == parcelable_size {
      return Ok(Some(result));
    }
    result.const_exprs_8 = parcel.read()?;
    if (parcel.get_data_position() - start_pos) == parcelable_size {
      return Ok(Some(result));
    }
    result.const_exprs_9 = parcel.read()?;
    if (parcel.get_data_position() - start_pos) == parcelable_size {
      return Ok(Some(result));
    }
    result.const_exprs_10 = parcel.read()?;
    if (parcel.get_data_position() - start_pos) == parcelable_size {
      return Ok(Some(result));
    }
    result.addString1 = parcel.read()?;
    if (parcel.get_data_position() - start_pos) == parcelable_size {
      return Ok(Some(result));
    }
    result.addString2 = parcel.read()?;
    if (parcel.get_data_position() - start_pos) == parcelable_size {
      return Ok(Some(result));
    }
    result.shouldSetBit0AndBit2 = parcel.read()?;
    if (parcel.get_data_position() - start_pos) == parcelable_size {
      return Ok(Some(result));
    }
    result.u = parcel.read()?;
    if (parcel.get_data_position() - start_pos) == parcelable_size {
      return Ok(Some(result));
    }
    result.shouldBeConstS1 = parcel.read()?;
    if (parcel.get_data_position() - start_pos) == parcelable_size {
      return Ok(Some(result));
    }
    result.defaultWithFoo = parcel.read()?;
    unsafe {
      parcel.set_data_position(start_pos + parcelable_size)?;
    }
    Ok(Some(result))
  }
}
