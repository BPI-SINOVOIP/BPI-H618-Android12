#![allow(non_upper_case_globals)]
#![allow(non_snake_case)]
#[allow(unused_imports)] use binder::IBinderInternal;
use binder::declare_binder_interface;
declare_binder_interface! {
  ITestService["android.aidl.tests.ITestService"] {
    native: BnTestService(on_transact),
    proxy: BpTestService {
    },
  }
}
pub trait ITestService: binder::Interface + Send {
  fn get_descriptor() -> &'static str where Self: Sized { "android.aidl.tests.ITestService" }
  fn UnimplementedMethod(&self, _arg_arg: i32) -> binder::public_api::Result<i32>;
  #[deprecated = "to make sure we have something in system/tools/aidl which does a compile check of deprecated and make sure this is reflected in goldens"]
  fn Deprecated(&self) -> binder::public_api::Result<()>;
  fn TestOneway(&self) -> binder::public_api::Result<()>;
  fn RepeatBoolean(&self, _arg_token: bool) -> binder::public_api::Result<bool>;
  fn RepeatByte(&self, _arg_token: i8) -> binder::public_api::Result<i8>;
  fn RepeatChar(&self, _arg_token: u16) -> binder::public_api::Result<u16>;
  fn RepeatInt(&self, _arg_token: i32) -> binder::public_api::Result<i32>;
  fn RepeatLong(&self, _arg_token: i64) -> binder::public_api::Result<i64>;
  fn RepeatFloat(&self, _arg_token: f32) -> binder::public_api::Result<f32>;
  fn RepeatDouble(&self, _arg_token: f64) -> binder::public_api::Result<f64>;
  fn RepeatString(&self, _arg_token: &str) -> binder::public_api::Result<String>;
  fn RepeatByteEnum(&self, _arg_token: crate::mangled::_7_android_4_aidl_5_tests_8_ByteEnum) -> binder::public_api::Result<crate::mangled::_7_android_4_aidl_5_tests_8_ByteEnum>;
  fn RepeatIntEnum(&self, _arg_token: crate::mangled::_7_android_4_aidl_5_tests_7_IntEnum) -> binder::public_api::Result<crate::mangled::_7_android_4_aidl_5_tests_7_IntEnum>;
  fn RepeatLongEnum(&self, _arg_token: crate::mangled::_7_android_4_aidl_5_tests_8_LongEnum) -> binder::public_api::Result<crate::mangled::_7_android_4_aidl_5_tests_8_LongEnum>;
  fn ReverseBoolean(&self, _arg_input: &[bool], _arg_repeated: &mut Vec<bool>) -> binder::public_api::Result<Vec<bool>>;
  fn ReverseByte(&self, _arg_input: &[u8], _arg_repeated: &mut Vec<u8>) -> binder::public_api::Result<Vec<u8>>;
  fn ReverseChar(&self, _arg_input: &[u16], _arg_repeated: &mut Vec<u16>) -> binder::public_api::Result<Vec<u16>>;
  fn ReverseInt(&self, _arg_input: &[i32], _arg_repeated: &mut Vec<i32>) -> binder::public_api::Result<Vec<i32>>;
  fn ReverseLong(&self, _arg_input: &[i64], _arg_repeated: &mut Vec<i64>) -> binder::public_api::Result<Vec<i64>>;
  fn ReverseFloat(&self, _arg_input: &[f32], _arg_repeated: &mut Vec<f32>) -> binder::public_api::Result<Vec<f32>>;
  fn ReverseDouble(&self, _arg_input: &[f64], _arg_repeated: &mut Vec<f64>) -> binder::public_api::Result<Vec<f64>>;
  fn ReverseString(&self, _arg_input: &[String], _arg_repeated: &mut Vec<String>) -> binder::public_api::Result<Vec<String>>;
  fn ReverseByteEnum(&self, _arg_input: &[crate::mangled::_7_android_4_aidl_5_tests_8_ByteEnum], _arg_repeated: &mut Vec<crate::mangled::_7_android_4_aidl_5_tests_8_ByteEnum>) -> binder::public_api::Result<Vec<crate::mangled::_7_android_4_aidl_5_tests_8_ByteEnum>>;
  fn ReverseIntEnum(&self, _arg_input: &[crate::mangled::_7_android_4_aidl_5_tests_7_IntEnum], _arg_repeated: &mut Vec<crate::mangled::_7_android_4_aidl_5_tests_7_IntEnum>) -> binder::public_api::Result<Vec<crate::mangled::_7_android_4_aidl_5_tests_7_IntEnum>>;
  fn ReverseLongEnum(&self, _arg_input: &[crate::mangled::_7_android_4_aidl_5_tests_8_LongEnum], _arg_repeated: &mut Vec<crate::mangled::_7_android_4_aidl_5_tests_8_LongEnum>) -> binder::public_api::Result<Vec<crate::mangled::_7_android_4_aidl_5_tests_8_LongEnum>>;
  fn GetOtherTestService(&self, _arg_name: &str) -> binder::public_api::Result<binder::Strong<dyn crate::mangled::_7_android_4_aidl_5_tests_14_INamedCallback>>;
  fn VerifyName(&self, _arg_service: &binder::Strong<dyn crate::mangled::_7_android_4_aidl_5_tests_14_INamedCallback>, _arg_name: &str) -> binder::public_api::Result<bool>;
  fn ReverseStringList(&self, _arg_input: &[String], _arg_repeated: &mut Vec<String>) -> binder::public_api::Result<Vec<String>>;
  fn RepeatParcelFileDescriptor(&self, _arg_read: &binder::parcel::ParcelFileDescriptor) -> binder::public_api::Result<binder::parcel::ParcelFileDescriptor>;
  fn ReverseParcelFileDescriptorArray(&self, _arg_input: &[binder::parcel::ParcelFileDescriptor], _arg_repeated: &mut Vec<Option<binder::parcel::ParcelFileDescriptor>>) -> binder::public_api::Result<Vec<binder::parcel::ParcelFileDescriptor>>;
  fn ThrowServiceException(&self, _arg_code: i32) -> binder::public_api::Result<()>;
  fn RepeatNullableIntArray(&self, _arg_input: Option<&[i32]>) -> binder::public_api::Result<Option<Vec<i32>>>;
  fn RepeatNullableByteEnumArray(&self, _arg_input: Option<&[crate::mangled::_7_android_4_aidl_5_tests_8_ByteEnum]>) -> binder::public_api::Result<Option<Vec<crate::mangled::_7_android_4_aidl_5_tests_8_ByteEnum>>>;
  fn RepeatNullableIntEnumArray(&self, _arg_input: Option<&[crate::mangled::_7_android_4_aidl_5_tests_7_IntEnum]>) -> binder::public_api::Result<Option<Vec<crate::mangled::_7_android_4_aidl_5_tests_7_IntEnum>>>;
  fn RepeatNullableLongEnumArray(&self, _arg_input: Option<&[crate::mangled::_7_android_4_aidl_5_tests_8_LongEnum]>) -> binder::public_api::Result<Option<Vec<crate::mangled::_7_android_4_aidl_5_tests_8_LongEnum>>>;
  fn RepeatNullableString(&self, _arg_input: Option<&str>) -> binder::public_api::Result<Option<String>>;
  fn RepeatNullableStringList(&self, _arg_input: Option<&[Option<String>]>) -> binder::public_api::Result<Option<Vec<Option<String>>>>;
  fn RepeatNullableParcelable(&self, _arg_input: Option<&crate::mangled::_7_android_4_aidl_5_tests_20_StructuredParcelable>) -> binder::public_api::Result<Option<crate::mangled::_7_android_4_aidl_5_tests_20_StructuredParcelable>>;
  fn TakesAnIBinder(&self, _arg_input: &binder::SpIBinder) -> binder::public_api::Result<()>;
  fn TakesANullableIBinder(&self, _arg_input: Option<&binder::SpIBinder>) -> binder::public_api::Result<()>;
  fn RepeatUtf8CppString(&self, _arg_token: &str) -> binder::public_api::Result<String>;
  fn RepeatNullableUtf8CppString(&self, _arg_token: Option<&str>) -> binder::public_api::Result<Option<String>>;
  fn ReverseUtf8CppString(&self, _arg_input: &[String], _arg_repeated: &mut Vec<String>) -> binder::public_api::Result<Vec<String>>;
  fn ReverseNullableUtf8CppString(&self, _arg_input: Option<&[Option<String>]>, _arg_repeated: &mut Option<Vec<Option<String>>>) -> binder::public_api::Result<Option<Vec<Option<String>>>>;
  fn ReverseUtf8CppStringList(&self, _arg_input: Option<&[Option<String>]>, _arg_repeated: &mut Option<Vec<Option<String>>>) -> binder::public_api::Result<Option<Vec<Option<String>>>>;
  fn GetCallback(&self, _arg_return_null: bool) -> binder::public_api::Result<Option<binder::Strong<dyn crate::mangled::_7_android_4_aidl_5_tests_14_INamedCallback>>>;
  fn FillOutStructuredParcelable(&self, _arg_parcel: &mut crate::mangled::_7_android_4_aidl_5_tests_20_StructuredParcelable) -> binder::public_api::Result<()>;
  fn GetOldNameInterface(&self) -> binder::public_api::Result<binder::Strong<dyn crate::mangled::_7_android_4_aidl_5_tests_8_IOldName>>;
  fn GetNewNameInterface(&self) -> binder::public_api::Result<binder::Strong<dyn crate::mangled::_7_android_4_aidl_5_tests_8_INewName>>;
  fn GetCppJavaTests(&self) -> binder::public_api::Result<Option<binder::SpIBinder>>;
  fn getBackendType(&self) -> binder::public_api::Result<crate::mangled::_7_android_4_aidl_5_tests_11_BackendType>;
  fn getDefaultImpl() -> ITestServiceDefaultRef where Self: Sized {
    DEFAULT_IMPL.lock().unwrap().clone()
  }
  fn setDefaultImpl(d: ITestServiceDefaultRef) -> ITestServiceDefaultRef where Self: Sized {
    std::mem::replace(&mut *DEFAULT_IMPL.lock().unwrap(), d)
  }
}
pub trait ITestServiceDefault: Send + Sync {
  fn UnimplementedMethod(&self, _arg_arg: i32) -> binder::public_api::Result<i32> {
    Err(binder::StatusCode::UNKNOWN_TRANSACTION.into())
  }
  fn Deprecated(&self) -> binder::public_api::Result<()> {
    Err(binder::StatusCode::UNKNOWN_TRANSACTION.into())
  }
  fn TestOneway(&self) -> binder::public_api::Result<()> {
    Err(binder::StatusCode::UNKNOWN_TRANSACTION.into())
  }
  fn RepeatBoolean(&self, _arg_token: bool) -> binder::public_api::Result<bool> {
    Err(binder::StatusCode::UNKNOWN_TRANSACTION.into())
  }
  fn RepeatByte(&self, _arg_token: i8) -> binder::public_api::Result<i8> {
    Err(binder::StatusCode::UNKNOWN_TRANSACTION.into())
  }
  fn RepeatChar(&self, _arg_token: u16) -> binder::public_api::Result<u16> {
    Err(binder::StatusCode::UNKNOWN_TRANSACTION.into())
  }
  fn RepeatInt(&self, _arg_token: i32) -> binder::public_api::Result<i32> {
    Err(binder::StatusCode::UNKNOWN_TRANSACTION.into())
  }
  fn RepeatLong(&self, _arg_token: i64) -> binder::public_api::Result<i64> {
    Err(binder::StatusCode::UNKNOWN_TRANSACTION.into())
  }
  fn RepeatFloat(&self, _arg_token: f32) -> binder::public_api::Result<f32> {
    Err(binder::StatusCode::UNKNOWN_TRANSACTION.into())
  }
  fn RepeatDouble(&self, _arg_token: f64) -> binder::public_api::Result<f64> {
    Err(binder::StatusCode::UNKNOWN_TRANSACTION.into())
  }
  fn RepeatString(&self, _arg_token: &str) -> binder::public_api::Result<String> {
    Err(binder::StatusCode::UNKNOWN_TRANSACTION.into())
  }
  fn RepeatByteEnum(&self, _arg_token: crate::mangled::_7_android_4_aidl_5_tests_8_ByteEnum) -> binder::public_api::Result<crate::mangled::_7_android_4_aidl_5_tests_8_ByteEnum> {
    Err(binder::StatusCode::UNKNOWN_TRANSACTION.into())
  }
  fn RepeatIntEnum(&self, _arg_token: crate::mangled::_7_android_4_aidl_5_tests_7_IntEnum) -> binder::public_api::Result<crate::mangled::_7_android_4_aidl_5_tests_7_IntEnum> {
    Err(binder::StatusCode::UNKNOWN_TRANSACTION.into())
  }
  fn RepeatLongEnum(&self, _arg_token: crate::mangled::_7_android_4_aidl_5_tests_8_LongEnum) -> binder::public_api::Result<crate::mangled::_7_android_4_aidl_5_tests_8_LongEnum> {
    Err(binder::StatusCode::UNKNOWN_TRANSACTION.into())
  }
  fn ReverseBoolean(&self, _arg_input: &[bool], _arg_repeated: &mut Vec<bool>) -> binder::public_api::Result<Vec<bool>> {
    Err(binder::StatusCode::UNKNOWN_TRANSACTION.into())
  }
  fn ReverseByte(&self, _arg_input: &[u8], _arg_repeated: &mut Vec<u8>) -> binder::public_api::Result<Vec<u8>> {
    Err(binder::StatusCode::UNKNOWN_TRANSACTION.into())
  }
  fn ReverseChar(&self, _arg_input: &[u16], _arg_repeated: &mut Vec<u16>) -> binder::public_api::Result<Vec<u16>> {
    Err(binder::StatusCode::UNKNOWN_TRANSACTION.into())
  }
  fn ReverseInt(&self, _arg_input: &[i32], _arg_repeated: &mut Vec<i32>) -> binder::public_api::Result<Vec<i32>> {
    Err(binder::StatusCode::UNKNOWN_TRANSACTION.into())
  }
  fn ReverseLong(&self, _arg_input: &[i64], _arg_repeated: &mut Vec<i64>) -> binder::public_api::Result<Vec<i64>> {
    Err(binder::StatusCode::UNKNOWN_TRANSACTION.into())
  }
  fn ReverseFloat(&self, _arg_input: &[f32], _arg_repeated: &mut Vec<f32>) -> binder::public_api::Result<Vec<f32>> {
    Err(binder::StatusCode::UNKNOWN_TRANSACTION.into())
  }
  fn ReverseDouble(&self, _arg_input: &[f64], _arg_repeated: &mut Vec<f64>) -> binder::public_api::Result<Vec<f64>> {
    Err(binder::StatusCode::UNKNOWN_TRANSACTION.into())
  }
  fn ReverseString(&self, _arg_input: &[String], _arg_repeated: &mut Vec<String>) -> binder::public_api::Result<Vec<String>> {
    Err(binder::StatusCode::UNKNOWN_TRANSACTION.into())
  }
  fn ReverseByteEnum(&self, _arg_input: &[crate::mangled::_7_android_4_aidl_5_tests_8_ByteEnum], _arg_repeated: &mut Vec<crate::mangled::_7_android_4_aidl_5_tests_8_ByteEnum>) -> binder::public_api::Result<Vec<crate::mangled::_7_android_4_aidl_5_tests_8_ByteEnum>> {
    Err(binder::StatusCode::UNKNOWN_TRANSACTION.into())
  }
  fn ReverseIntEnum(&self, _arg_input: &[crate::mangled::_7_android_4_aidl_5_tests_7_IntEnum], _arg_repeated: &mut Vec<crate::mangled::_7_android_4_aidl_5_tests_7_IntEnum>) -> binder::public_api::Result<Vec<crate::mangled::_7_android_4_aidl_5_tests_7_IntEnum>> {
    Err(binder::StatusCode::UNKNOWN_TRANSACTION.into())
  }
  fn ReverseLongEnum(&self, _arg_input: &[crate::mangled::_7_android_4_aidl_5_tests_8_LongEnum], _arg_repeated: &mut Vec<crate::mangled::_7_android_4_aidl_5_tests_8_LongEnum>) -> binder::public_api::Result<Vec<crate::mangled::_7_android_4_aidl_5_tests_8_LongEnum>> {
    Err(binder::StatusCode::UNKNOWN_TRANSACTION.into())
  }
  fn GetOtherTestService(&self, _arg_name: &str) -> binder::public_api::Result<binder::Strong<dyn crate::mangled::_7_android_4_aidl_5_tests_14_INamedCallback>> {
    Err(binder::StatusCode::UNKNOWN_TRANSACTION.into())
  }
  fn VerifyName(&self, _arg_service: &binder::Strong<dyn crate::mangled::_7_android_4_aidl_5_tests_14_INamedCallback>, _arg_name: &str) -> binder::public_api::Result<bool> {
    Err(binder::StatusCode::UNKNOWN_TRANSACTION.into())
  }
  fn ReverseStringList(&self, _arg_input: &[String], _arg_repeated: &mut Vec<String>) -> binder::public_api::Result<Vec<String>> {
    Err(binder::StatusCode::UNKNOWN_TRANSACTION.into())
  }
  fn RepeatParcelFileDescriptor(&self, _arg_read: &binder::parcel::ParcelFileDescriptor) -> binder::public_api::Result<binder::parcel::ParcelFileDescriptor> {
    Err(binder::StatusCode::UNKNOWN_TRANSACTION.into())
  }
  fn ReverseParcelFileDescriptorArray(&self, _arg_input: &[binder::parcel::ParcelFileDescriptor], _arg_repeated: &mut Vec<Option<binder::parcel::ParcelFileDescriptor>>) -> binder::public_api::Result<Vec<binder::parcel::ParcelFileDescriptor>> {
    Err(binder::StatusCode::UNKNOWN_TRANSACTION.into())
  }
  fn ThrowServiceException(&self, _arg_code: i32) -> binder::public_api::Result<()> {
    Err(binder::StatusCode::UNKNOWN_TRANSACTION.into())
  }
  fn RepeatNullableIntArray(&self, _arg_input: Option<&[i32]>) -> binder::public_api::Result<Option<Vec<i32>>> {
    Err(binder::StatusCode::UNKNOWN_TRANSACTION.into())
  }
  fn RepeatNullableByteEnumArray(&self, _arg_input: Option<&[crate::mangled::_7_android_4_aidl_5_tests_8_ByteEnum]>) -> binder::public_api::Result<Option<Vec<crate::mangled::_7_android_4_aidl_5_tests_8_ByteEnum>>> {
    Err(binder::StatusCode::UNKNOWN_TRANSACTION.into())
  }
  fn RepeatNullableIntEnumArray(&self, _arg_input: Option<&[crate::mangled::_7_android_4_aidl_5_tests_7_IntEnum]>) -> binder::public_api::Result<Option<Vec<crate::mangled::_7_android_4_aidl_5_tests_7_IntEnum>>> {
    Err(binder::StatusCode::UNKNOWN_TRANSACTION.into())
  }
  fn RepeatNullableLongEnumArray(&self, _arg_input: Option<&[crate::mangled::_7_android_4_aidl_5_tests_8_LongEnum]>) -> binder::public_api::Result<Option<Vec<crate::mangled::_7_android_4_aidl_5_tests_8_LongEnum>>> {
    Err(binder::StatusCode::UNKNOWN_TRANSACTION.into())
  }
  fn RepeatNullableString(&self, _arg_input: Option<&str>) -> binder::public_api::Result<Option<String>> {
    Err(binder::StatusCode::UNKNOWN_TRANSACTION.into())
  }
  fn RepeatNullableStringList(&self, _arg_input: Option<&[Option<String>]>) -> binder::public_api::Result<Option<Vec<Option<String>>>> {
    Err(binder::StatusCode::UNKNOWN_TRANSACTION.into())
  }
  fn RepeatNullableParcelable(&self, _arg_input: Option<&crate::mangled::_7_android_4_aidl_5_tests_20_StructuredParcelable>) -> binder::public_api::Result<Option<crate::mangled::_7_android_4_aidl_5_tests_20_StructuredParcelable>> {
    Err(binder::StatusCode::UNKNOWN_TRANSACTION.into())
  }
  fn TakesAnIBinder(&self, _arg_input: &binder::SpIBinder) -> binder::public_api::Result<()> {
    Err(binder::StatusCode::UNKNOWN_TRANSACTION.into())
  }
  fn TakesANullableIBinder(&self, _arg_input: Option<&binder::SpIBinder>) -> binder::public_api::Result<()> {
    Err(binder::StatusCode::UNKNOWN_TRANSACTION.into())
  }
  fn RepeatUtf8CppString(&self, _arg_token: &str) -> binder::public_api::Result<String> {
    Err(binder::StatusCode::UNKNOWN_TRANSACTION.into())
  }
  fn RepeatNullableUtf8CppString(&self, _arg_token: Option<&str>) -> binder::public_api::Result<Option<String>> {
    Err(binder::StatusCode::UNKNOWN_TRANSACTION.into())
  }
  fn ReverseUtf8CppString(&self, _arg_input: &[String], _arg_repeated: &mut Vec<String>) -> binder::public_api::Result<Vec<String>> {
    Err(binder::StatusCode::UNKNOWN_TRANSACTION.into())
  }
  fn ReverseNullableUtf8CppString(&self, _arg_input: Option<&[Option<String>]>, _arg_repeated: &mut Option<Vec<Option<String>>>) -> binder::public_api::Result<Option<Vec<Option<String>>>> {
    Err(binder::StatusCode::UNKNOWN_TRANSACTION.into())
  }
  fn ReverseUtf8CppStringList(&self, _arg_input: Option<&[Option<String>]>, _arg_repeated: &mut Option<Vec<Option<String>>>) -> binder::public_api::Result<Option<Vec<Option<String>>>> {
    Err(binder::StatusCode::UNKNOWN_TRANSACTION.into())
  }
  fn GetCallback(&self, _arg_return_null: bool) -> binder::public_api::Result<Option<binder::Strong<dyn crate::mangled::_7_android_4_aidl_5_tests_14_INamedCallback>>> {
    Err(binder::StatusCode::UNKNOWN_TRANSACTION.into())
  }
  fn FillOutStructuredParcelable(&self, _arg_parcel: &mut crate::mangled::_7_android_4_aidl_5_tests_20_StructuredParcelable) -> binder::public_api::Result<()> {
    Err(binder::StatusCode::UNKNOWN_TRANSACTION.into())
  }
  fn GetOldNameInterface(&self) -> binder::public_api::Result<binder::Strong<dyn crate::mangled::_7_android_4_aidl_5_tests_8_IOldName>> {
    Err(binder::StatusCode::UNKNOWN_TRANSACTION.into())
  }
  fn GetNewNameInterface(&self) -> binder::public_api::Result<binder::Strong<dyn crate::mangled::_7_android_4_aidl_5_tests_8_INewName>> {
    Err(binder::StatusCode::UNKNOWN_TRANSACTION.into())
  }
  fn GetCppJavaTests(&self) -> binder::public_api::Result<Option<binder::SpIBinder>> {
    Err(binder::StatusCode::UNKNOWN_TRANSACTION.into())
  }
  fn getBackendType(&self) -> binder::public_api::Result<crate::mangled::_7_android_4_aidl_5_tests_11_BackendType> {
    Err(binder::StatusCode::UNKNOWN_TRANSACTION.into())
  }
}
pub mod transactions {
  pub const UnimplementedMethod: binder::TransactionCode = binder::FIRST_CALL_TRANSACTION + 0;
  pub const Deprecated: binder::TransactionCode = binder::FIRST_CALL_TRANSACTION + 1;
  pub const TestOneway: binder::TransactionCode = binder::FIRST_CALL_TRANSACTION + 2;
  pub const RepeatBoolean: binder::TransactionCode = binder::FIRST_CALL_TRANSACTION + 3;
  pub const RepeatByte: binder::TransactionCode = binder::FIRST_CALL_TRANSACTION + 4;
  pub const RepeatChar: binder::TransactionCode = binder::FIRST_CALL_TRANSACTION + 5;
  pub const RepeatInt: binder::TransactionCode = binder::FIRST_CALL_TRANSACTION + 6;
  pub const RepeatLong: binder::TransactionCode = binder::FIRST_CALL_TRANSACTION + 7;
  pub const RepeatFloat: binder::TransactionCode = binder::FIRST_CALL_TRANSACTION + 8;
  pub const RepeatDouble: binder::TransactionCode = binder::FIRST_CALL_TRANSACTION + 9;
  pub const RepeatString: binder::TransactionCode = binder::FIRST_CALL_TRANSACTION + 10;
  pub const RepeatByteEnum: binder::TransactionCode = binder::FIRST_CALL_TRANSACTION + 11;
  pub const RepeatIntEnum: binder::TransactionCode = binder::FIRST_CALL_TRANSACTION + 12;
  pub const RepeatLongEnum: binder::TransactionCode = binder::FIRST_CALL_TRANSACTION + 13;
  pub const ReverseBoolean: binder::TransactionCode = binder::FIRST_CALL_TRANSACTION + 14;
  pub const ReverseByte: binder::TransactionCode = binder::FIRST_CALL_TRANSACTION + 15;
  pub const ReverseChar: binder::TransactionCode = binder::FIRST_CALL_TRANSACTION + 16;
  pub const ReverseInt: binder::TransactionCode = binder::FIRST_CALL_TRANSACTION + 17;
  pub const ReverseLong: binder::TransactionCode = binder::FIRST_CALL_TRANSACTION + 18;
  pub const ReverseFloat: binder::TransactionCode = binder::FIRST_CALL_TRANSACTION + 19;
  pub const ReverseDouble: binder::TransactionCode = binder::FIRST_CALL_TRANSACTION + 20;
  pub const ReverseString: binder::TransactionCode = binder::FIRST_CALL_TRANSACTION + 21;
  pub const ReverseByteEnum: binder::TransactionCode = binder::FIRST_CALL_TRANSACTION + 22;
  pub const ReverseIntEnum: binder::TransactionCode = binder::FIRST_CALL_TRANSACTION + 23;
  pub const ReverseLongEnum: binder::TransactionCode = binder::FIRST_CALL_TRANSACTION + 24;
  pub const GetOtherTestService: binder::TransactionCode = binder::FIRST_CALL_TRANSACTION + 25;
  pub const VerifyName: binder::TransactionCode = binder::FIRST_CALL_TRANSACTION + 26;
  pub const ReverseStringList: binder::TransactionCode = binder::FIRST_CALL_TRANSACTION + 27;
  pub const RepeatParcelFileDescriptor: binder::TransactionCode = binder::FIRST_CALL_TRANSACTION + 28;
  pub const ReverseParcelFileDescriptorArray: binder::TransactionCode = binder::FIRST_CALL_TRANSACTION + 29;
  pub const ThrowServiceException: binder::TransactionCode = binder::FIRST_CALL_TRANSACTION + 30;
  pub const RepeatNullableIntArray: binder::TransactionCode = binder::FIRST_CALL_TRANSACTION + 31;
  pub const RepeatNullableByteEnumArray: binder::TransactionCode = binder::FIRST_CALL_TRANSACTION + 32;
  pub const RepeatNullableIntEnumArray: binder::TransactionCode = binder::FIRST_CALL_TRANSACTION + 33;
  pub const RepeatNullableLongEnumArray: binder::TransactionCode = binder::FIRST_CALL_TRANSACTION + 34;
  pub const RepeatNullableString: binder::TransactionCode = binder::FIRST_CALL_TRANSACTION + 35;
  pub const RepeatNullableStringList: binder::TransactionCode = binder::FIRST_CALL_TRANSACTION + 36;
  pub const RepeatNullableParcelable: binder::TransactionCode = binder::FIRST_CALL_TRANSACTION + 37;
  pub const TakesAnIBinder: binder::TransactionCode = binder::FIRST_CALL_TRANSACTION + 38;
  pub const TakesANullableIBinder: binder::TransactionCode = binder::FIRST_CALL_TRANSACTION + 39;
  pub const RepeatUtf8CppString: binder::TransactionCode = binder::FIRST_CALL_TRANSACTION + 40;
  pub const RepeatNullableUtf8CppString: binder::TransactionCode = binder::FIRST_CALL_TRANSACTION + 41;
  pub const ReverseUtf8CppString: binder::TransactionCode = binder::FIRST_CALL_TRANSACTION + 42;
  pub const ReverseNullableUtf8CppString: binder::TransactionCode = binder::FIRST_CALL_TRANSACTION + 43;
  pub const ReverseUtf8CppStringList: binder::TransactionCode = binder::FIRST_CALL_TRANSACTION + 44;
  pub const GetCallback: binder::TransactionCode = binder::FIRST_CALL_TRANSACTION + 45;
  pub const FillOutStructuredParcelable: binder::TransactionCode = binder::FIRST_CALL_TRANSACTION + 46;
  pub const GetOldNameInterface: binder::TransactionCode = binder::FIRST_CALL_TRANSACTION + 47;
  pub const GetNewNameInterface: binder::TransactionCode = binder::FIRST_CALL_TRANSACTION + 48;
  pub const GetCppJavaTests: binder::TransactionCode = binder::FIRST_CALL_TRANSACTION + 49;
  pub const getBackendType: binder::TransactionCode = binder::FIRST_CALL_TRANSACTION + 50;
}
pub type ITestServiceDefaultRef = Option<std::sync::Arc<dyn ITestServiceDefault>>;
use lazy_static::lazy_static;
lazy_static! {
  static ref DEFAULT_IMPL: std::sync::Mutex<ITestServiceDefaultRef> = std::sync::Mutex::new(None);
}
pub const TEST_CONSTANT: i32 = 42;
pub const TEST_CONSTANT2: i32 = -42;
pub const TEST_CONSTANT3: i32 = 42;
pub const TEST_CONSTANT4: i32 = 4;
pub const TEST_CONSTANT5: i32 = -4;
pub const TEST_CONSTANT6: i32 = 0;
pub const TEST_CONSTANT7: i32 = 0;
pub const TEST_CONSTANT8: i32 = 0;
pub const TEST_CONSTANT9: i32 = 86;
pub const TEST_CONSTANT10: i32 = 165;
pub const TEST_CONSTANT11: i32 = 250;
pub const TEST_CONSTANT12: i32 = -1;
pub const BYTE_TEST_CONSTANT: i8 = 17;
pub const LONG_TEST_CONSTANT: i64 = 1099511627776;
pub const STRING_TEST_CONSTANT: &str = "foo";
pub const STRING_TEST_CONSTANT2: &str = "bar";
pub const STRING_TEST_CONSTANT_UTF8: &str = "baz";
pub const A1: i32 = 1;
pub const A2: i32 = 1;
pub const A3: i32 = 1;
pub const A4: i32 = 1;
pub const A5: i32 = 1;
pub const A6: i32 = 1;
pub const A7: i32 = 1;
pub const A8: i32 = 1;
pub const A9: i32 = 1;
pub const A10: i32 = 1;
pub const A11: i32 = 1;
pub const A12: i32 = 1;
pub const A13: i32 = 1;
pub const A14: i32 = 1;
pub const A15: i32 = 1;
pub const A16: i32 = 1;
pub const A17: i32 = 1;
pub const A18: i32 = 1;
pub const A19: i32 = 1;
pub const A20: i32 = 1;
pub const A21: i32 = 1;
pub const A22: i32 = 1;
pub const A23: i32 = 1;
pub const A24: i32 = 1;
pub const A25: i32 = 1;
pub const A26: i32 = 1;
pub const A27: i32 = 1;
pub const A28: i32 = 1;
pub const A29: i32 = 1;
pub const A30: i32 = 1;
pub const A31: i32 = 1;
pub const A32: i32 = 1;
pub const A33: i32 = 1;
pub const A34: i32 = 1;
pub const A35: i32 = 1;
pub const A36: i32 = 1;
pub const A37: i32 = 1;
pub const A38: i32 = 1;
pub const A39: i32 = 1;
pub const A40: i32 = 1;
pub const A41: i32 = 1;
pub const A42: i32 = 1;
pub const A43: i32 = 1;
pub const A44: i32 = 1;
pub const A45: i32 = 1;
pub const A46: i32 = 1;
pub const A47: i32 = 1;
pub const A48: i32 = 1;
pub const A49: i32 = 1;
pub const A50: i32 = 1;
pub const A51: i32 = 1;
pub const A52: i32 = 1;
pub const A53: i32 = 1;
pub const A54: i32 = 1;
pub const A55: i32 = 1;
pub const A56: i32 = 1;
pub const A57: i32 = 1;
pub(crate) mod mangled { pub use super::ITestService as _7_android_4_aidl_5_tests_12_ITestService; }
impl ITestService for BpTestService {
  fn UnimplementedMethod(&self, _arg_arg: i32) -> binder::public_api::Result<i32> {
    let _aidl_reply = self.binder.transact(transactions::UnimplementedMethod, binder::FLAG_CLEAR_BUF | binder::FLAG_PRIVATE_LOCAL, |_aidl_data| {
      _aidl_data.mark_sensitive();
      _aidl_data.write(&_arg_arg)?;
      Ok(())
    });
    if let Err(binder::StatusCode::UNKNOWN_TRANSACTION) = _aidl_reply {
      if let Some(_aidl_default_impl) = <Self as ITestService>::getDefaultImpl() {
        return _aidl_default_impl.UnimplementedMethod(_arg_arg);
      }
    }
    let _aidl_reply = _aidl_reply?;
    let _aidl_status: binder::Status = _aidl_reply.read()?;
    if !_aidl_status.is_ok() { return Err(_aidl_status); }
    let _aidl_return: i32 = _aidl_reply.read()?;
    Ok(_aidl_return)
  }
  fn Deprecated(&self) -> binder::public_api::Result<()> {
    let _aidl_reply = self.binder.transact(transactions::Deprecated, binder::FLAG_CLEAR_BUF | binder::FLAG_PRIVATE_LOCAL, |_aidl_data| {
      _aidl_data.mark_sensitive();
      Ok(())
    });
    if let Err(binder::StatusCode::UNKNOWN_TRANSACTION) = _aidl_reply {
      if let Some(_aidl_default_impl) = <Self as ITestService>::getDefaultImpl() {
        return _aidl_default_impl.Deprecated();
      }
    }
    let _aidl_reply = _aidl_reply?;
    let _aidl_status: binder::Status = _aidl_reply.read()?;
    if !_aidl_status.is_ok() { return Err(_aidl_status); }
    Ok(())
  }
  fn TestOneway(&self) -> binder::public_api::Result<()> {
    let _aidl_reply = self.binder.transact(transactions::TestOneway, binder::FLAG_ONEWAY | binder::FLAG_CLEAR_BUF | binder::FLAG_PRIVATE_LOCAL, |_aidl_data| {
      _aidl_data.mark_sensitive();
      Ok(())
    });
    if let Err(binder::StatusCode::UNKNOWN_TRANSACTION) = _aidl_reply {
      if let Some(_aidl_default_impl) = <Self as ITestService>::getDefaultImpl() {
        return _aidl_default_impl.TestOneway();
      }
    }
    let _aidl_reply = _aidl_reply?;
    Ok(())
  }
  fn RepeatBoolean(&self, _arg_token: bool) -> binder::public_api::Result<bool> {
    let _aidl_reply = self.binder.transact(transactions::RepeatBoolean, binder::FLAG_CLEAR_BUF | binder::FLAG_PRIVATE_LOCAL, |_aidl_data| {
      _aidl_data.mark_sensitive();
      _aidl_data.write(&_arg_token)?;
      Ok(())
    });
    if let Err(binder::StatusCode::UNKNOWN_TRANSACTION) = _aidl_reply {
      if let Some(_aidl_default_impl) = <Self as ITestService>::getDefaultImpl() {
        return _aidl_default_impl.RepeatBoolean(_arg_token);
      }
    }
    let _aidl_reply = _aidl_reply?;
    let _aidl_status: binder::Status = _aidl_reply.read()?;
    if !_aidl_status.is_ok() { return Err(_aidl_status); }
    let _aidl_return: bool = _aidl_reply.read()?;
    Ok(_aidl_return)
  }
  fn RepeatByte(&self, _arg_token: i8) -> binder::public_api::Result<i8> {
    let _aidl_reply = self.binder.transact(transactions::RepeatByte, binder::FLAG_CLEAR_BUF | binder::FLAG_PRIVATE_LOCAL, |_aidl_data| {
      _aidl_data.mark_sensitive();
      _aidl_data.write(&_arg_token)?;
      Ok(())
    });
    if let Err(binder::StatusCode::UNKNOWN_TRANSACTION) = _aidl_reply {
      if let Some(_aidl_default_impl) = <Self as ITestService>::getDefaultImpl() {
        return _aidl_default_impl.RepeatByte(_arg_token);
      }
    }
    let _aidl_reply = _aidl_reply?;
    let _aidl_status: binder::Status = _aidl_reply.read()?;
    if !_aidl_status.is_ok() { return Err(_aidl_status); }
    let _aidl_return: i8 = _aidl_reply.read()?;
    Ok(_aidl_return)
  }
  fn RepeatChar(&self, _arg_token: u16) -> binder::public_api::Result<u16> {
    let _aidl_reply = self.binder.transact(transactions::RepeatChar, binder::FLAG_CLEAR_BUF | binder::FLAG_PRIVATE_LOCAL, |_aidl_data| {
      _aidl_data.mark_sensitive();
      _aidl_data.write(&_arg_token)?;
      Ok(())
    });
    if let Err(binder::StatusCode::UNKNOWN_TRANSACTION) = _aidl_reply {
      if let Some(_aidl_default_impl) = <Self as ITestService>::getDefaultImpl() {
        return _aidl_default_impl.RepeatChar(_arg_token);
      }
    }
    let _aidl_reply = _aidl_reply?;
    let _aidl_status: binder::Status = _aidl_reply.read()?;
    if !_aidl_status.is_ok() { return Err(_aidl_status); }
    let _aidl_return: u16 = _aidl_reply.read()?;
    Ok(_aidl_return)
  }
  fn RepeatInt(&self, _arg_token: i32) -> binder::public_api::Result<i32> {
    let _aidl_reply = self.binder.transact(transactions::RepeatInt, binder::FLAG_CLEAR_BUF | binder::FLAG_PRIVATE_LOCAL, |_aidl_data| {
      _aidl_data.mark_sensitive();
      _aidl_data.write(&_arg_token)?;
      Ok(())
    });
    if let Err(binder::StatusCode::UNKNOWN_TRANSACTION) = _aidl_reply {
      if let Some(_aidl_default_impl) = <Self as ITestService>::getDefaultImpl() {
        return _aidl_default_impl.RepeatInt(_arg_token);
      }
    }
    let _aidl_reply = _aidl_reply?;
    let _aidl_status: binder::Status = _aidl_reply.read()?;
    if !_aidl_status.is_ok() { return Err(_aidl_status); }
    let _aidl_return: i32 = _aidl_reply.read()?;
    Ok(_aidl_return)
  }
  fn RepeatLong(&self, _arg_token: i64) -> binder::public_api::Result<i64> {
    let _aidl_reply = self.binder.transact(transactions::RepeatLong, binder::FLAG_CLEAR_BUF | binder::FLAG_PRIVATE_LOCAL, |_aidl_data| {
      _aidl_data.mark_sensitive();
      _aidl_data.write(&_arg_token)?;
      Ok(())
    });
    if let Err(binder::StatusCode::UNKNOWN_TRANSACTION) = _aidl_reply {
      if let Some(_aidl_default_impl) = <Self as ITestService>::getDefaultImpl() {
        return _aidl_default_impl.RepeatLong(_arg_token);
      }
    }
    let _aidl_reply = _aidl_reply?;
    let _aidl_status: binder::Status = _aidl_reply.read()?;
    if !_aidl_status.is_ok() { return Err(_aidl_status); }
    let _aidl_return: i64 = _aidl_reply.read()?;
    Ok(_aidl_return)
  }
  fn RepeatFloat(&self, _arg_token: f32) -> binder::public_api::Result<f32> {
    let _aidl_reply = self.binder.transact(transactions::RepeatFloat, binder::FLAG_CLEAR_BUF | binder::FLAG_PRIVATE_LOCAL, |_aidl_data| {
      _aidl_data.mark_sensitive();
      _aidl_data.write(&_arg_token)?;
      Ok(())
    });
    if let Err(binder::StatusCode::UNKNOWN_TRANSACTION) = _aidl_reply {
      if let Some(_aidl_default_impl) = <Self as ITestService>::getDefaultImpl() {
        return _aidl_default_impl.RepeatFloat(_arg_token);
      }
    }
    let _aidl_reply = _aidl_reply?;
    let _aidl_status: binder::Status = _aidl_reply.read()?;
    if !_aidl_status.is_ok() { return Err(_aidl_status); }
    let _aidl_return: f32 = _aidl_reply.read()?;
    Ok(_aidl_return)
  }
  fn RepeatDouble(&self, _arg_token: f64) -> binder::public_api::Result<f64> {
    let _aidl_reply = self.binder.transact(transactions::RepeatDouble, binder::FLAG_CLEAR_BUF | binder::FLAG_PRIVATE_LOCAL, |_aidl_data| {
      _aidl_data.mark_sensitive();
      _aidl_data.write(&_arg_token)?;
      Ok(())
    });
    if let Err(binder::StatusCode::UNKNOWN_TRANSACTION) = _aidl_reply {
      if let Some(_aidl_default_impl) = <Self as ITestService>::getDefaultImpl() {
        return _aidl_default_impl.RepeatDouble(_arg_token);
      }
    }
    let _aidl_reply = _aidl_reply?;
    let _aidl_status: binder::Status = _aidl_reply.read()?;
    if !_aidl_status.is_ok() { return Err(_aidl_status); }
    let _aidl_return: f64 = _aidl_reply.read()?;
    Ok(_aidl_return)
  }
  fn RepeatString(&self, _arg_token: &str) -> binder::public_api::Result<String> {
    let _aidl_reply = self.binder.transact(transactions::RepeatString, binder::FLAG_CLEAR_BUF | binder::FLAG_PRIVATE_LOCAL, |_aidl_data| {
      _aidl_data.mark_sensitive();
      _aidl_data.write(_arg_token)?;
      Ok(())
    });
    if let Err(binder::StatusCode::UNKNOWN_TRANSACTION) = _aidl_reply {
      if let Some(_aidl_default_impl) = <Self as ITestService>::getDefaultImpl() {
        return _aidl_default_impl.RepeatString(_arg_token);
      }
    }
    let _aidl_reply = _aidl_reply?;
    let _aidl_status: binder::Status = _aidl_reply.read()?;
    if !_aidl_status.is_ok() { return Err(_aidl_status); }
    let _aidl_return: String = _aidl_reply.read()?;
    Ok(_aidl_return)
  }
  fn RepeatByteEnum(&self, _arg_token: crate::mangled::_7_android_4_aidl_5_tests_8_ByteEnum) -> binder::public_api::Result<crate::mangled::_7_android_4_aidl_5_tests_8_ByteEnum> {
    let _aidl_reply = self.binder.transact(transactions::RepeatByteEnum, binder::FLAG_CLEAR_BUF | binder::FLAG_PRIVATE_LOCAL, |_aidl_data| {
      _aidl_data.mark_sensitive();
      _aidl_data.write(&_arg_token)?;
      Ok(())
    });
    if let Err(binder::StatusCode::UNKNOWN_TRANSACTION) = _aidl_reply {
      if let Some(_aidl_default_impl) = <Self as ITestService>::getDefaultImpl() {
        return _aidl_default_impl.RepeatByteEnum(_arg_token);
      }
    }
    let _aidl_reply = _aidl_reply?;
    let _aidl_status: binder::Status = _aidl_reply.read()?;
    if !_aidl_status.is_ok() { return Err(_aidl_status); }
    let _aidl_return: crate::mangled::_7_android_4_aidl_5_tests_8_ByteEnum = _aidl_reply.read()?;
    Ok(_aidl_return)
  }
  fn RepeatIntEnum(&self, _arg_token: crate::mangled::_7_android_4_aidl_5_tests_7_IntEnum) -> binder::public_api::Result<crate::mangled::_7_android_4_aidl_5_tests_7_IntEnum> {
    let _aidl_reply = self.binder.transact(transactions::RepeatIntEnum, binder::FLAG_CLEAR_BUF | binder::FLAG_PRIVATE_LOCAL, |_aidl_data| {
      _aidl_data.mark_sensitive();
      _aidl_data.write(&_arg_token)?;
      Ok(())
    });
    if let Err(binder::StatusCode::UNKNOWN_TRANSACTION) = _aidl_reply {
      if let Some(_aidl_default_impl) = <Self as ITestService>::getDefaultImpl() {
        return _aidl_default_impl.RepeatIntEnum(_arg_token);
      }
    }
    let _aidl_reply = _aidl_reply?;
    let _aidl_status: binder::Status = _aidl_reply.read()?;
    if !_aidl_status.is_ok() { return Err(_aidl_status); }
    let _aidl_return: crate::mangled::_7_android_4_aidl_5_tests_7_IntEnum = _aidl_reply.read()?;
    Ok(_aidl_return)
  }
  fn RepeatLongEnum(&self, _arg_token: crate::mangled::_7_android_4_aidl_5_tests_8_LongEnum) -> binder::public_api::Result<crate::mangled::_7_android_4_aidl_5_tests_8_LongEnum> {
    let _aidl_reply = self.binder.transact(transactions::RepeatLongEnum, binder::FLAG_CLEAR_BUF | binder::FLAG_PRIVATE_LOCAL, |_aidl_data| {
      _aidl_data.mark_sensitive();
      _aidl_data.write(&_arg_token)?;
      Ok(())
    });
    if let Err(binder::StatusCode::UNKNOWN_TRANSACTION) = _aidl_reply {
      if let Some(_aidl_default_impl) = <Self as ITestService>::getDefaultImpl() {
        return _aidl_default_impl.RepeatLongEnum(_arg_token);
      }
    }
    let _aidl_reply = _aidl_reply?;
    let _aidl_status: binder::Status = _aidl_reply.read()?;
    if !_aidl_status.is_ok() { return Err(_aidl_status); }
    let _aidl_return: crate::mangled::_7_android_4_aidl_5_tests_8_LongEnum = _aidl_reply.read()?;
    Ok(_aidl_return)
  }
  fn ReverseBoolean(&self, _arg_input: &[bool], _arg_repeated: &mut Vec<bool>) -> binder::public_api::Result<Vec<bool>> {
    let _aidl_reply = self.binder.transact(transactions::ReverseBoolean, binder::FLAG_CLEAR_BUF | binder::FLAG_PRIVATE_LOCAL, |_aidl_data| {
      _aidl_data.mark_sensitive();
      _aidl_data.write(_arg_input)?;
      _aidl_data.write_slice_size(Some(_arg_repeated))?;
      Ok(())
    });
    if let Err(binder::StatusCode::UNKNOWN_TRANSACTION) = _aidl_reply {
      if let Some(_aidl_default_impl) = <Self as ITestService>::getDefaultImpl() {
        return _aidl_default_impl.ReverseBoolean(_arg_input, _arg_repeated);
      }
    }
    let _aidl_reply = _aidl_reply?;
    let _aidl_status: binder::Status = _aidl_reply.read()?;
    if !_aidl_status.is_ok() { return Err(_aidl_status); }
    let _aidl_return: Vec<bool> = _aidl_reply.read()?;
    *_arg_repeated = _aidl_reply.read()?;
    Ok(_aidl_return)
  }
  fn ReverseByte(&self, _arg_input: &[u8], _arg_repeated: &mut Vec<u8>) -> binder::public_api::Result<Vec<u8>> {
    let _aidl_reply = self.binder.transact(transactions::ReverseByte, binder::FLAG_CLEAR_BUF | binder::FLAG_PRIVATE_LOCAL, |_aidl_data| {
      _aidl_data.mark_sensitive();
      _aidl_data.write(_arg_input)?;
      _aidl_data.write_slice_size(Some(_arg_repeated))?;
      Ok(())
    });
    if let Err(binder::StatusCode::UNKNOWN_TRANSACTION) = _aidl_reply {
      if let Some(_aidl_default_impl) = <Self as ITestService>::getDefaultImpl() {
        return _aidl_default_impl.ReverseByte(_arg_input, _arg_repeated);
      }
    }
    let _aidl_reply = _aidl_reply?;
    let _aidl_status: binder::Status = _aidl_reply.read()?;
    if !_aidl_status.is_ok() { return Err(_aidl_status); }
    let _aidl_return: Vec<u8> = _aidl_reply.read()?;
    *_arg_repeated = _aidl_reply.read()?;
    Ok(_aidl_return)
  }
  fn ReverseChar(&self, _arg_input: &[u16], _arg_repeated: &mut Vec<u16>) -> binder::public_api::Result<Vec<u16>> {
    let _aidl_reply = self.binder.transact(transactions::ReverseChar, binder::FLAG_CLEAR_BUF | binder::FLAG_PRIVATE_LOCAL, |_aidl_data| {
      _aidl_data.mark_sensitive();
      _aidl_data.write(_arg_input)?;
      _aidl_data.write_slice_size(Some(_arg_repeated))?;
      Ok(())
    });
    if let Err(binder::StatusCode::UNKNOWN_TRANSACTION) = _aidl_reply {
      if let Some(_aidl_default_impl) = <Self as ITestService>::getDefaultImpl() {
        return _aidl_default_impl.ReverseChar(_arg_input, _arg_repeated);
      }
    }
    let _aidl_reply = _aidl_reply?;
    let _aidl_status: binder::Status = _aidl_reply.read()?;
    if !_aidl_status.is_ok() { return Err(_aidl_status); }
    let _aidl_return: Vec<u16> = _aidl_reply.read()?;
    *_arg_repeated = _aidl_reply.read()?;
    Ok(_aidl_return)
  }
  fn ReverseInt(&self, _arg_input: &[i32], _arg_repeated: &mut Vec<i32>) -> binder::public_api::Result<Vec<i32>> {
    let _aidl_reply = self.binder.transact(transactions::ReverseInt, binder::FLAG_CLEAR_BUF | binder::FLAG_PRIVATE_LOCAL, |_aidl_data| {
      _aidl_data.mark_sensitive();
      _aidl_data.write(_arg_input)?;
      _aidl_data.write_slice_size(Some(_arg_repeated))?;
      Ok(())
    });
    if let Err(binder::StatusCode::UNKNOWN_TRANSACTION) = _aidl_reply {
      if let Some(_aidl_default_impl) = <Self as ITestService>::getDefaultImpl() {
        return _aidl_default_impl.ReverseInt(_arg_input, _arg_repeated);
      }
    }
    let _aidl_reply = _aidl_reply?;
    let _aidl_status: binder::Status = _aidl_reply.read()?;
    if !_aidl_status.is_ok() { return Err(_aidl_status); }
    let _aidl_return: Vec<i32> = _aidl_reply.read()?;
    *_arg_repeated = _aidl_reply.read()?;
    Ok(_aidl_return)
  }
  fn ReverseLong(&self, _arg_input: &[i64], _arg_repeated: &mut Vec<i64>) -> binder::public_api::Result<Vec<i64>> {
    let _aidl_reply = self.binder.transact(transactions::ReverseLong, binder::FLAG_CLEAR_BUF | binder::FLAG_PRIVATE_LOCAL, |_aidl_data| {
      _aidl_data.mark_sensitive();
      _aidl_data.write(_arg_input)?;
      _aidl_data.write_slice_size(Some(_arg_repeated))?;
      Ok(())
    });
    if let Err(binder::StatusCode::UNKNOWN_TRANSACTION) = _aidl_reply {
      if let Some(_aidl_default_impl) = <Self as ITestService>::getDefaultImpl() {
        return _aidl_default_impl.ReverseLong(_arg_input, _arg_repeated);
      }
    }
    let _aidl_reply = _aidl_reply?;
    let _aidl_status: binder::Status = _aidl_reply.read()?;
    if !_aidl_status.is_ok() { return Err(_aidl_status); }
    let _aidl_return: Vec<i64> = _aidl_reply.read()?;
    *_arg_repeated = _aidl_reply.read()?;
    Ok(_aidl_return)
  }
  fn ReverseFloat(&self, _arg_input: &[f32], _arg_repeated: &mut Vec<f32>) -> binder::public_api::Result<Vec<f32>> {
    let _aidl_reply = self.binder.transact(transactions::ReverseFloat, binder::FLAG_CLEAR_BUF | binder::FLAG_PRIVATE_LOCAL, |_aidl_data| {
      _aidl_data.mark_sensitive();
      _aidl_data.write(_arg_input)?;
      _aidl_data.write_slice_size(Some(_arg_repeated))?;
      Ok(())
    });
    if let Err(binder::StatusCode::UNKNOWN_TRANSACTION) = _aidl_reply {
      if let Some(_aidl_default_impl) = <Self as ITestService>::getDefaultImpl() {
        return _aidl_default_impl.ReverseFloat(_arg_input, _arg_repeated);
      }
    }
    let _aidl_reply = _aidl_reply?;
    let _aidl_status: binder::Status = _aidl_reply.read()?;
    if !_aidl_status.is_ok() { return Err(_aidl_status); }
    let _aidl_return: Vec<f32> = _aidl_reply.read()?;
    *_arg_repeated = _aidl_reply.read()?;
    Ok(_aidl_return)
  }
  fn ReverseDouble(&self, _arg_input: &[f64], _arg_repeated: &mut Vec<f64>) -> binder::public_api::Result<Vec<f64>> {
    let _aidl_reply = self.binder.transact(transactions::ReverseDouble, binder::FLAG_CLEAR_BUF | binder::FLAG_PRIVATE_LOCAL, |_aidl_data| {
      _aidl_data.mark_sensitive();
      _aidl_data.write(_arg_input)?;
      _aidl_data.write_slice_size(Some(_arg_repeated))?;
      Ok(())
    });
    if let Err(binder::StatusCode::UNKNOWN_TRANSACTION) = _aidl_reply {
      if let Some(_aidl_default_impl) = <Self as ITestService>::getDefaultImpl() {
        return _aidl_default_impl.ReverseDouble(_arg_input, _arg_repeated);
      }
    }
    let _aidl_reply = _aidl_reply?;
    let _aidl_status: binder::Status = _aidl_reply.read()?;
    if !_aidl_status.is_ok() { return Err(_aidl_status); }
    let _aidl_return: Vec<f64> = _aidl_reply.read()?;
    *_arg_repeated = _aidl_reply.read()?;
    Ok(_aidl_return)
  }
  fn ReverseString(&self, _arg_input: &[String], _arg_repeated: &mut Vec<String>) -> binder::public_api::Result<Vec<String>> {
    let _aidl_reply = self.binder.transact(transactions::ReverseString, binder::FLAG_CLEAR_BUF | binder::FLAG_PRIVATE_LOCAL, |_aidl_data| {
      _aidl_data.mark_sensitive();
      _aidl_data.write(_arg_input)?;
      _aidl_data.write_slice_size(Some(_arg_repeated))?;
      Ok(())
    });
    if let Err(binder::StatusCode::UNKNOWN_TRANSACTION) = _aidl_reply {
      if let Some(_aidl_default_impl) = <Self as ITestService>::getDefaultImpl() {
        return _aidl_default_impl.ReverseString(_arg_input, _arg_repeated);
      }
    }
    let _aidl_reply = _aidl_reply?;
    let _aidl_status: binder::Status = _aidl_reply.read()?;
    if !_aidl_status.is_ok() { return Err(_aidl_status); }
    let _aidl_return: Vec<String> = _aidl_reply.read()?;
    *_arg_repeated = _aidl_reply.read()?;
    Ok(_aidl_return)
  }
  fn ReverseByteEnum(&self, _arg_input: &[crate::mangled::_7_android_4_aidl_5_tests_8_ByteEnum], _arg_repeated: &mut Vec<crate::mangled::_7_android_4_aidl_5_tests_8_ByteEnum>) -> binder::public_api::Result<Vec<crate::mangled::_7_android_4_aidl_5_tests_8_ByteEnum>> {
    let _aidl_reply = self.binder.transact(transactions::ReverseByteEnum, binder::FLAG_CLEAR_BUF | binder::FLAG_PRIVATE_LOCAL, |_aidl_data| {
      _aidl_data.mark_sensitive();
      _aidl_data.write(_arg_input)?;
      _aidl_data.write_slice_size(Some(_arg_repeated))?;
      Ok(())
    });
    if let Err(binder::StatusCode::UNKNOWN_TRANSACTION) = _aidl_reply {
      if let Some(_aidl_default_impl) = <Self as ITestService>::getDefaultImpl() {
        return _aidl_default_impl.ReverseByteEnum(_arg_input, _arg_repeated);
      }
    }
    let _aidl_reply = _aidl_reply?;
    let _aidl_status: binder::Status = _aidl_reply.read()?;
    if !_aidl_status.is_ok() { return Err(_aidl_status); }
    let _aidl_return: Vec<crate::mangled::_7_android_4_aidl_5_tests_8_ByteEnum> = _aidl_reply.read()?;
    *_arg_repeated = _aidl_reply.read()?;
    Ok(_aidl_return)
  }
  fn ReverseIntEnum(&self, _arg_input: &[crate::mangled::_7_android_4_aidl_5_tests_7_IntEnum], _arg_repeated: &mut Vec<crate::mangled::_7_android_4_aidl_5_tests_7_IntEnum>) -> binder::public_api::Result<Vec<crate::mangled::_7_android_4_aidl_5_tests_7_IntEnum>> {
    let _aidl_reply = self.binder.transact(transactions::ReverseIntEnum, binder::FLAG_CLEAR_BUF | binder::FLAG_PRIVATE_LOCAL, |_aidl_data| {
      _aidl_data.mark_sensitive();
      _aidl_data.write(_arg_input)?;
      _aidl_data.write_slice_size(Some(_arg_repeated))?;
      Ok(())
    });
    if let Err(binder::StatusCode::UNKNOWN_TRANSACTION) = _aidl_reply {
      if let Some(_aidl_default_impl) = <Self as ITestService>::getDefaultImpl() {
        return _aidl_default_impl.ReverseIntEnum(_arg_input, _arg_repeated);
      }
    }
    let _aidl_reply = _aidl_reply?;
    let _aidl_status: binder::Status = _aidl_reply.read()?;
    if !_aidl_status.is_ok() { return Err(_aidl_status); }
    let _aidl_return: Vec<crate::mangled::_7_android_4_aidl_5_tests_7_IntEnum> = _aidl_reply.read()?;
    *_arg_repeated = _aidl_reply.read()?;
    Ok(_aidl_return)
  }
  fn ReverseLongEnum(&self, _arg_input: &[crate::mangled::_7_android_4_aidl_5_tests_8_LongEnum], _arg_repeated: &mut Vec<crate::mangled::_7_android_4_aidl_5_tests_8_LongEnum>) -> binder::public_api::Result<Vec<crate::mangled::_7_android_4_aidl_5_tests_8_LongEnum>> {
    let _aidl_reply = self.binder.transact(transactions::ReverseLongEnum, binder::FLAG_CLEAR_BUF | binder::FLAG_PRIVATE_LOCAL, |_aidl_data| {
      _aidl_data.mark_sensitive();
      _aidl_data.write(_arg_input)?;
      _aidl_data.write_slice_size(Some(_arg_repeated))?;
      Ok(())
    });
    if let Err(binder::StatusCode::UNKNOWN_TRANSACTION) = _aidl_reply {
      if let Some(_aidl_default_impl) = <Self as ITestService>::getDefaultImpl() {
        return _aidl_default_impl.ReverseLongEnum(_arg_input, _arg_repeated);
      }
    }
    let _aidl_reply = _aidl_reply?;
    let _aidl_status: binder::Status = _aidl_reply.read()?;
    if !_aidl_status.is_ok() { return Err(_aidl_status); }
    let _aidl_return: Vec<crate::mangled::_7_android_4_aidl_5_tests_8_LongEnum> = _aidl_reply.read()?;
    *_arg_repeated = _aidl_reply.read()?;
    Ok(_aidl_return)
  }
  fn GetOtherTestService(&self, _arg_name: &str) -> binder::public_api::Result<binder::Strong<dyn crate::mangled::_7_android_4_aidl_5_tests_14_INamedCallback>> {
    let _aidl_reply = self.binder.transact(transactions::GetOtherTestService, binder::FLAG_CLEAR_BUF | binder::FLAG_PRIVATE_LOCAL, |_aidl_data| {
      _aidl_data.mark_sensitive();
      _aidl_data.write(_arg_name)?;
      Ok(())
    });
    if let Err(binder::StatusCode::UNKNOWN_TRANSACTION) = _aidl_reply {
      if let Some(_aidl_default_impl) = <Self as ITestService>::getDefaultImpl() {
        return _aidl_default_impl.GetOtherTestService(_arg_name);
      }
    }
    let _aidl_reply = _aidl_reply?;
    let _aidl_status: binder::Status = _aidl_reply.read()?;
    if !_aidl_status.is_ok() { return Err(_aidl_status); }
    let _aidl_return: binder::Strong<dyn crate::mangled::_7_android_4_aidl_5_tests_14_INamedCallback> = _aidl_reply.read()?;
    Ok(_aidl_return)
  }
  fn VerifyName(&self, _arg_service: &binder::Strong<dyn crate::mangled::_7_android_4_aidl_5_tests_14_INamedCallback>, _arg_name: &str) -> binder::public_api::Result<bool> {
    let _aidl_reply = self.binder.transact(transactions::VerifyName, binder::FLAG_CLEAR_BUF | binder::FLAG_PRIVATE_LOCAL, |_aidl_data| {
      _aidl_data.mark_sensitive();
      _aidl_data.write(_arg_service)?;
      _aidl_data.write(_arg_name)?;
      Ok(())
    });
    if let Err(binder::StatusCode::UNKNOWN_TRANSACTION) = _aidl_reply {
      if let Some(_aidl_default_impl) = <Self as ITestService>::getDefaultImpl() {
        return _aidl_default_impl.VerifyName(_arg_service, _arg_name);
      }
    }
    let _aidl_reply = _aidl_reply?;
    let _aidl_status: binder::Status = _aidl_reply.read()?;
    if !_aidl_status.is_ok() { return Err(_aidl_status); }
    let _aidl_return: bool = _aidl_reply.read()?;
    Ok(_aidl_return)
  }
  fn ReverseStringList(&self, _arg_input: &[String], _arg_repeated: &mut Vec<String>) -> binder::public_api::Result<Vec<String>> {
    let _aidl_reply = self.binder.transact(transactions::ReverseStringList, binder::FLAG_CLEAR_BUF | binder::FLAG_PRIVATE_LOCAL, |_aidl_data| {
      _aidl_data.mark_sensitive();
      _aidl_data.write(_arg_input)?;
      Ok(())
    });
    if let Err(binder::StatusCode::UNKNOWN_TRANSACTION) = _aidl_reply {
      if let Some(_aidl_default_impl) = <Self as ITestService>::getDefaultImpl() {
        return _aidl_default_impl.ReverseStringList(_arg_input, _arg_repeated);
      }
    }
    let _aidl_reply = _aidl_reply?;
    let _aidl_status: binder::Status = _aidl_reply.read()?;
    if !_aidl_status.is_ok() { return Err(_aidl_status); }
    let _aidl_return: Vec<String> = _aidl_reply.read()?;
    *_arg_repeated = _aidl_reply.read()?;
    Ok(_aidl_return)
  }
  fn RepeatParcelFileDescriptor(&self, _arg_read: &binder::parcel::ParcelFileDescriptor) -> binder::public_api::Result<binder::parcel::ParcelFileDescriptor> {
    let _aidl_reply = self.binder.transact(transactions::RepeatParcelFileDescriptor, binder::FLAG_CLEAR_BUF | binder::FLAG_PRIVATE_LOCAL, |_aidl_data| {
      _aidl_data.mark_sensitive();
      _aidl_data.write(_arg_read)?;
      Ok(())
    });
    if let Err(binder::StatusCode::UNKNOWN_TRANSACTION) = _aidl_reply {
      if let Some(_aidl_default_impl) = <Self as ITestService>::getDefaultImpl() {
        return _aidl_default_impl.RepeatParcelFileDescriptor(_arg_read);
      }
    }
    let _aidl_reply = _aidl_reply?;
    let _aidl_status: binder::Status = _aidl_reply.read()?;
    if !_aidl_status.is_ok() { return Err(_aidl_status); }
    let _aidl_return: binder::parcel::ParcelFileDescriptor = _aidl_reply.read()?;
    Ok(_aidl_return)
  }
  fn ReverseParcelFileDescriptorArray(&self, _arg_input: &[binder::parcel::ParcelFileDescriptor], _arg_repeated: &mut Vec<Option<binder::parcel::ParcelFileDescriptor>>) -> binder::public_api::Result<Vec<binder::parcel::ParcelFileDescriptor>> {
    let _aidl_reply = self.binder.transact(transactions::ReverseParcelFileDescriptorArray, binder::FLAG_CLEAR_BUF | binder::FLAG_PRIVATE_LOCAL, |_aidl_data| {
      _aidl_data.mark_sensitive();
      _aidl_data.write(_arg_input)?;
      _aidl_data.write_slice_size(Some(_arg_repeated))?;
      Ok(())
    });
    if let Err(binder::StatusCode::UNKNOWN_TRANSACTION) = _aidl_reply {
      if let Some(_aidl_default_impl) = <Self as ITestService>::getDefaultImpl() {
        return _aidl_default_impl.ReverseParcelFileDescriptorArray(_arg_input, _arg_repeated);
      }
    }
    let _aidl_reply = _aidl_reply?;
    let _aidl_status: binder::Status = _aidl_reply.read()?;
    if !_aidl_status.is_ok() { return Err(_aidl_status); }
    let _aidl_return: Vec<binder::parcel::ParcelFileDescriptor> = _aidl_reply.read()?;
    *_arg_repeated = _aidl_reply.read()?;
    Ok(_aidl_return)
  }
  fn ThrowServiceException(&self, _arg_code: i32) -> binder::public_api::Result<()> {
    let _aidl_reply = self.binder.transact(transactions::ThrowServiceException, binder::FLAG_CLEAR_BUF | binder::FLAG_PRIVATE_LOCAL, |_aidl_data| {
      _aidl_data.mark_sensitive();
      _aidl_data.write(&_arg_code)?;
      Ok(())
    });
    if let Err(binder::StatusCode::UNKNOWN_TRANSACTION) = _aidl_reply {
      if let Some(_aidl_default_impl) = <Self as ITestService>::getDefaultImpl() {
        return _aidl_default_impl.ThrowServiceException(_arg_code);
      }
    }
    let _aidl_reply = _aidl_reply?;
    let _aidl_status: binder::Status = _aidl_reply.read()?;
    if !_aidl_status.is_ok() { return Err(_aidl_status); }
    Ok(())
  }
  fn RepeatNullableIntArray(&self, _arg_input: Option<&[i32]>) -> binder::public_api::Result<Option<Vec<i32>>> {
    let _aidl_reply = self.binder.transact(transactions::RepeatNullableIntArray, binder::FLAG_CLEAR_BUF | binder::FLAG_PRIVATE_LOCAL, |_aidl_data| {
      _aidl_data.mark_sensitive();
      _aidl_data.write(&_arg_input)?;
      Ok(())
    });
    if let Err(binder::StatusCode::UNKNOWN_TRANSACTION) = _aidl_reply {
      if let Some(_aidl_default_impl) = <Self as ITestService>::getDefaultImpl() {
        return _aidl_default_impl.RepeatNullableIntArray(_arg_input);
      }
    }
    let _aidl_reply = _aidl_reply?;
    let _aidl_status: binder::Status = _aidl_reply.read()?;
    if !_aidl_status.is_ok() { return Err(_aidl_status); }
    let _aidl_return: Option<Vec<i32>> = _aidl_reply.read()?;
    Ok(_aidl_return)
  }
  fn RepeatNullableByteEnumArray(&self, _arg_input: Option<&[crate::mangled::_7_android_4_aidl_5_tests_8_ByteEnum]>) -> binder::public_api::Result<Option<Vec<crate::mangled::_7_android_4_aidl_5_tests_8_ByteEnum>>> {
    let _aidl_reply = self.binder.transact(transactions::RepeatNullableByteEnumArray, binder::FLAG_CLEAR_BUF | binder::FLAG_PRIVATE_LOCAL, |_aidl_data| {
      _aidl_data.mark_sensitive();
      _aidl_data.write(&_arg_input)?;
      Ok(())
    });
    if let Err(binder::StatusCode::UNKNOWN_TRANSACTION) = _aidl_reply {
      if let Some(_aidl_default_impl) = <Self as ITestService>::getDefaultImpl() {
        return _aidl_default_impl.RepeatNullableByteEnumArray(_arg_input);
      }
    }
    let _aidl_reply = _aidl_reply?;
    let _aidl_status: binder::Status = _aidl_reply.read()?;
    if !_aidl_status.is_ok() { return Err(_aidl_status); }
    let _aidl_return: Option<Vec<crate::mangled::_7_android_4_aidl_5_tests_8_ByteEnum>> = _aidl_reply.read()?;
    Ok(_aidl_return)
  }
  fn RepeatNullableIntEnumArray(&self, _arg_input: Option<&[crate::mangled::_7_android_4_aidl_5_tests_7_IntEnum]>) -> binder::public_api::Result<Option<Vec<crate::mangled::_7_android_4_aidl_5_tests_7_IntEnum>>> {
    let _aidl_reply = self.binder.transact(transactions::RepeatNullableIntEnumArray, binder::FLAG_CLEAR_BUF | binder::FLAG_PRIVATE_LOCAL, |_aidl_data| {
      _aidl_data.mark_sensitive();
      _aidl_data.write(&_arg_input)?;
      Ok(())
    });
    if let Err(binder::StatusCode::UNKNOWN_TRANSACTION) = _aidl_reply {
      if let Some(_aidl_default_impl) = <Self as ITestService>::getDefaultImpl() {
        return _aidl_default_impl.RepeatNullableIntEnumArray(_arg_input);
      }
    }
    let _aidl_reply = _aidl_reply?;
    let _aidl_status: binder::Status = _aidl_reply.read()?;
    if !_aidl_status.is_ok() { return Err(_aidl_status); }
    let _aidl_return: Option<Vec<crate::mangled::_7_android_4_aidl_5_tests_7_IntEnum>> = _aidl_reply.read()?;
    Ok(_aidl_return)
  }
  fn RepeatNullableLongEnumArray(&self, _arg_input: Option<&[crate::mangled::_7_android_4_aidl_5_tests_8_LongEnum]>) -> binder::public_api::Result<Option<Vec<crate::mangled::_7_android_4_aidl_5_tests_8_LongEnum>>> {
    let _aidl_reply = self.binder.transact(transactions::RepeatNullableLongEnumArray, binder::FLAG_CLEAR_BUF | binder::FLAG_PRIVATE_LOCAL, |_aidl_data| {
      _aidl_data.mark_sensitive();
      _aidl_data.write(&_arg_input)?;
      Ok(())
    });
    if let Err(binder::StatusCode::UNKNOWN_TRANSACTION) = _aidl_reply {
      if let Some(_aidl_default_impl) = <Self as ITestService>::getDefaultImpl() {
        return _aidl_default_impl.RepeatNullableLongEnumArray(_arg_input);
      }
    }
    let _aidl_reply = _aidl_reply?;
    let _aidl_status: binder::Status = _aidl_reply.read()?;
    if !_aidl_status.is_ok() { return Err(_aidl_status); }
    let _aidl_return: Option<Vec<crate::mangled::_7_android_4_aidl_5_tests_8_LongEnum>> = _aidl_reply.read()?;
    Ok(_aidl_return)
  }
  fn RepeatNullableString(&self, _arg_input: Option<&str>) -> binder::public_api::Result<Option<String>> {
    let _aidl_reply = self.binder.transact(transactions::RepeatNullableString, binder::FLAG_CLEAR_BUF | binder::FLAG_PRIVATE_LOCAL, |_aidl_data| {
      _aidl_data.mark_sensitive();
      _aidl_data.write(&_arg_input)?;
      Ok(())
    });
    if let Err(binder::StatusCode::UNKNOWN_TRANSACTION) = _aidl_reply {
      if let Some(_aidl_default_impl) = <Self as ITestService>::getDefaultImpl() {
        return _aidl_default_impl.RepeatNullableString(_arg_input);
      }
    }
    let _aidl_reply = _aidl_reply?;
    let _aidl_status: binder::Status = _aidl_reply.read()?;
    if !_aidl_status.is_ok() { return Err(_aidl_status); }
    let _aidl_return: Option<String> = _aidl_reply.read()?;
    Ok(_aidl_return)
  }
  fn RepeatNullableStringList(&self, _arg_input: Option<&[Option<String>]>) -> binder::public_api::Result<Option<Vec<Option<String>>>> {
    let _aidl_reply = self.binder.transact(transactions::RepeatNullableStringList, binder::FLAG_CLEAR_BUF | binder::FLAG_PRIVATE_LOCAL, |_aidl_data| {
      _aidl_data.mark_sensitive();
      _aidl_data.write(&_arg_input)?;
      Ok(())
    });
    if let Err(binder::StatusCode::UNKNOWN_TRANSACTION) = _aidl_reply {
      if let Some(_aidl_default_impl) = <Self as ITestService>::getDefaultImpl() {
        return _aidl_default_impl.RepeatNullableStringList(_arg_input);
      }
    }
    let _aidl_reply = _aidl_reply?;
    let _aidl_status: binder::Status = _aidl_reply.read()?;
    if !_aidl_status.is_ok() { return Err(_aidl_status); }
    let _aidl_return: Option<Vec<Option<String>>> = _aidl_reply.read()?;
    Ok(_aidl_return)
  }
  fn RepeatNullableParcelable(&self, _arg_input: Option<&crate::mangled::_7_android_4_aidl_5_tests_20_StructuredParcelable>) -> binder::public_api::Result<Option<crate::mangled::_7_android_4_aidl_5_tests_20_StructuredParcelable>> {
    let _aidl_reply = self.binder.transact(transactions::RepeatNullableParcelable, binder::FLAG_CLEAR_BUF | binder::FLAG_PRIVATE_LOCAL, |_aidl_data| {
      _aidl_data.mark_sensitive();
      _aidl_data.write(&_arg_input)?;
      Ok(())
    });
    if let Err(binder::StatusCode::UNKNOWN_TRANSACTION) = _aidl_reply {
      if let Some(_aidl_default_impl) = <Self as ITestService>::getDefaultImpl() {
        return _aidl_default_impl.RepeatNullableParcelable(_arg_input);
      }
    }
    let _aidl_reply = _aidl_reply?;
    let _aidl_status: binder::Status = _aidl_reply.read()?;
    if !_aidl_status.is_ok() { return Err(_aidl_status); }
    let _aidl_return: Option<crate::mangled::_7_android_4_aidl_5_tests_20_StructuredParcelable> = _aidl_reply.read()?;
    Ok(_aidl_return)
  }
  fn TakesAnIBinder(&self, _arg_input: &binder::SpIBinder) -> binder::public_api::Result<()> {
    let _aidl_reply = self.binder.transact(transactions::TakesAnIBinder, binder::FLAG_CLEAR_BUF | binder::FLAG_PRIVATE_LOCAL, |_aidl_data| {
      _aidl_data.mark_sensitive();
      _aidl_data.write(_arg_input)?;
      Ok(())
    });
    if let Err(binder::StatusCode::UNKNOWN_TRANSACTION) = _aidl_reply {
      if let Some(_aidl_default_impl) = <Self as ITestService>::getDefaultImpl() {
        return _aidl_default_impl.TakesAnIBinder(_arg_input);
      }
    }
    let _aidl_reply = _aidl_reply?;
    let _aidl_status: binder::Status = _aidl_reply.read()?;
    if !_aidl_status.is_ok() { return Err(_aidl_status); }
    Ok(())
  }
  fn TakesANullableIBinder(&self, _arg_input: Option<&binder::SpIBinder>) -> binder::public_api::Result<()> {
    let _aidl_reply = self.binder.transact(transactions::TakesANullableIBinder, binder::FLAG_CLEAR_BUF | binder::FLAG_PRIVATE_LOCAL, |_aidl_data| {
      _aidl_data.mark_sensitive();
      _aidl_data.write(&_arg_input)?;
      Ok(())
    });
    if let Err(binder::StatusCode::UNKNOWN_TRANSACTION) = _aidl_reply {
      if let Some(_aidl_default_impl) = <Self as ITestService>::getDefaultImpl() {
        return _aidl_default_impl.TakesANullableIBinder(_arg_input);
      }
    }
    let _aidl_reply = _aidl_reply?;
    let _aidl_status: binder::Status = _aidl_reply.read()?;
    if !_aidl_status.is_ok() { return Err(_aidl_status); }
    Ok(())
  }
  fn RepeatUtf8CppString(&self, _arg_token: &str) -> binder::public_api::Result<String> {
    let _aidl_reply = self.binder.transact(transactions::RepeatUtf8CppString, binder::FLAG_CLEAR_BUF | binder::FLAG_PRIVATE_LOCAL, |_aidl_data| {
      _aidl_data.mark_sensitive();
      _aidl_data.write(_arg_token)?;
      Ok(())
    });
    if let Err(binder::StatusCode::UNKNOWN_TRANSACTION) = _aidl_reply {
      if let Some(_aidl_default_impl) = <Self as ITestService>::getDefaultImpl() {
        return _aidl_default_impl.RepeatUtf8CppString(_arg_token);
      }
    }
    let _aidl_reply = _aidl_reply?;
    let _aidl_status: binder::Status = _aidl_reply.read()?;
    if !_aidl_status.is_ok() { return Err(_aidl_status); }
    let _aidl_return: String = _aidl_reply.read()?;
    Ok(_aidl_return)
  }
  fn RepeatNullableUtf8CppString(&self, _arg_token: Option<&str>) -> binder::public_api::Result<Option<String>> {
    let _aidl_reply = self.binder.transact(transactions::RepeatNullableUtf8CppString, binder::FLAG_CLEAR_BUF | binder::FLAG_PRIVATE_LOCAL, |_aidl_data| {
      _aidl_data.mark_sensitive();
      _aidl_data.write(&_arg_token)?;
      Ok(())
    });
    if let Err(binder::StatusCode::UNKNOWN_TRANSACTION) = _aidl_reply {
      if let Some(_aidl_default_impl) = <Self as ITestService>::getDefaultImpl() {
        return _aidl_default_impl.RepeatNullableUtf8CppString(_arg_token);
      }
    }
    let _aidl_reply = _aidl_reply?;
    let _aidl_status: binder::Status = _aidl_reply.read()?;
    if !_aidl_status.is_ok() { return Err(_aidl_status); }
    let _aidl_return: Option<String> = _aidl_reply.read()?;
    Ok(_aidl_return)
  }
  fn ReverseUtf8CppString(&self, _arg_input: &[String], _arg_repeated: &mut Vec<String>) -> binder::public_api::Result<Vec<String>> {
    let _aidl_reply = self.binder.transact(transactions::ReverseUtf8CppString, binder::FLAG_CLEAR_BUF | binder::FLAG_PRIVATE_LOCAL, |_aidl_data| {
      _aidl_data.mark_sensitive();
      _aidl_data.write(_arg_input)?;
      _aidl_data.write_slice_size(Some(_arg_repeated))?;
      Ok(())
    });
    if let Err(binder::StatusCode::UNKNOWN_TRANSACTION) = _aidl_reply {
      if let Some(_aidl_default_impl) = <Self as ITestService>::getDefaultImpl() {
        return _aidl_default_impl.ReverseUtf8CppString(_arg_input, _arg_repeated);
      }
    }
    let _aidl_reply = _aidl_reply?;
    let _aidl_status: binder::Status = _aidl_reply.read()?;
    if !_aidl_status.is_ok() { return Err(_aidl_status); }
    let _aidl_return: Vec<String> = _aidl_reply.read()?;
    *_arg_repeated = _aidl_reply.read()?;
    Ok(_aidl_return)
  }
  fn ReverseNullableUtf8CppString(&self, _arg_input: Option<&[Option<String>]>, _arg_repeated: &mut Option<Vec<Option<String>>>) -> binder::public_api::Result<Option<Vec<Option<String>>>> {
    let _aidl_reply = self.binder.transact(transactions::ReverseNullableUtf8CppString, binder::FLAG_CLEAR_BUF | binder::FLAG_PRIVATE_LOCAL, |_aidl_data| {
      _aidl_data.mark_sensitive();
      _aidl_data.write(&_arg_input)?;
      _aidl_data.write_slice_size(_arg_repeated.as_deref())?;
      Ok(())
    });
    if let Err(binder::StatusCode::UNKNOWN_TRANSACTION) = _aidl_reply {
      if let Some(_aidl_default_impl) = <Self as ITestService>::getDefaultImpl() {
        return _aidl_default_impl.ReverseNullableUtf8CppString(_arg_input, _arg_repeated);
      }
    }
    let _aidl_reply = _aidl_reply?;
    let _aidl_status: binder::Status = _aidl_reply.read()?;
    if !_aidl_status.is_ok() { return Err(_aidl_status); }
    let _aidl_return: Option<Vec<Option<String>>> = _aidl_reply.read()?;
    *_arg_repeated = _aidl_reply.read()?;
    Ok(_aidl_return)
  }
  fn ReverseUtf8CppStringList(&self, _arg_input: Option<&[Option<String>]>, _arg_repeated: &mut Option<Vec<Option<String>>>) -> binder::public_api::Result<Option<Vec<Option<String>>>> {
    let _aidl_reply = self.binder.transact(transactions::ReverseUtf8CppStringList, binder::FLAG_CLEAR_BUF | binder::FLAG_PRIVATE_LOCAL, |_aidl_data| {
      _aidl_data.mark_sensitive();
      _aidl_data.write(&_arg_input)?;
      Ok(())
    });
    if let Err(binder::StatusCode::UNKNOWN_TRANSACTION) = _aidl_reply {
      if let Some(_aidl_default_impl) = <Self as ITestService>::getDefaultImpl() {
        return _aidl_default_impl.ReverseUtf8CppStringList(_arg_input, _arg_repeated);
      }
    }
    let _aidl_reply = _aidl_reply?;
    let _aidl_status: binder::Status = _aidl_reply.read()?;
    if !_aidl_status.is_ok() { return Err(_aidl_status); }
    let _aidl_return: Option<Vec<Option<String>>> = _aidl_reply.read()?;
    *_arg_repeated = _aidl_reply.read()?;
    Ok(_aidl_return)
  }
  fn GetCallback(&self, _arg_return_null: bool) -> binder::public_api::Result<Option<binder::Strong<dyn crate::mangled::_7_android_4_aidl_5_tests_14_INamedCallback>>> {
    let _aidl_reply = self.binder.transact(transactions::GetCallback, binder::FLAG_CLEAR_BUF | binder::FLAG_PRIVATE_LOCAL, |_aidl_data| {
      _aidl_data.mark_sensitive();
      _aidl_data.write(&_arg_return_null)?;
      Ok(())
    });
    if let Err(binder::StatusCode::UNKNOWN_TRANSACTION) = _aidl_reply {
      if let Some(_aidl_default_impl) = <Self as ITestService>::getDefaultImpl() {
        return _aidl_default_impl.GetCallback(_arg_return_null);
      }
    }
    let _aidl_reply = _aidl_reply?;
    let _aidl_status: binder::Status = _aidl_reply.read()?;
    if !_aidl_status.is_ok() { return Err(_aidl_status); }
    let _aidl_return: Option<binder::Strong<dyn crate::mangled::_7_android_4_aidl_5_tests_14_INamedCallback>> = _aidl_reply.read()?;
    Ok(_aidl_return)
  }
  fn FillOutStructuredParcelable(&self, _arg_parcel: &mut crate::mangled::_7_android_4_aidl_5_tests_20_StructuredParcelable) -> binder::public_api::Result<()> {
    let _aidl_reply = self.binder.transact(transactions::FillOutStructuredParcelable, binder::FLAG_CLEAR_BUF | binder::FLAG_PRIVATE_LOCAL, |_aidl_data| {
      _aidl_data.mark_sensitive();
      _aidl_data.write(_arg_parcel)?;
      Ok(())
    });
    if let Err(binder::StatusCode::UNKNOWN_TRANSACTION) = _aidl_reply {
      if let Some(_aidl_default_impl) = <Self as ITestService>::getDefaultImpl() {
        return _aidl_default_impl.FillOutStructuredParcelable(_arg_parcel);
      }
    }
    let _aidl_reply = _aidl_reply?;
    let _aidl_status: binder::Status = _aidl_reply.read()?;
    if !_aidl_status.is_ok() { return Err(_aidl_status); }
    *_arg_parcel = _aidl_reply.read()?;
    Ok(())
  }
  fn GetOldNameInterface(&self) -> binder::public_api::Result<binder::Strong<dyn crate::mangled::_7_android_4_aidl_5_tests_8_IOldName>> {
    let _aidl_reply = self.binder.transact(transactions::GetOldNameInterface, binder::FLAG_CLEAR_BUF | binder::FLAG_PRIVATE_LOCAL, |_aidl_data| {
      _aidl_data.mark_sensitive();
      Ok(())
    });
    if let Err(binder::StatusCode::UNKNOWN_TRANSACTION) = _aidl_reply {
      if let Some(_aidl_default_impl) = <Self as ITestService>::getDefaultImpl() {
        return _aidl_default_impl.GetOldNameInterface();
      }
    }
    let _aidl_reply = _aidl_reply?;
    let _aidl_status: binder::Status = _aidl_reply.read()?;
    if !_aidl_status.is_ok() { return Err(_aidl_status); }
    let _aidl_return: binder::Strong<dyn crate::mangled::_7_android_4_aidl_5_tests_8_IOldName> = _aidl_reply.read()?;
    Ok(_aidl_return)
  }
  fn GetNewNameInterface(&self) -> binder::public_api::Result<binder::Strong<dyn crate::mangled::_7_android_4_aidl_5_tests_8_INewName>> {
    let _aidl_reply = self.binder.transact(transactions::GetNewNameInterface, binder::FLAG_CLEAR_BUF | binder::FLAG_PRIVATE_LOCAL, |_aidl_data| {
      _aidl_data.mark_sensitive();
      Ok(())
    });
    if let Err(binder::StatusCode::UNKNOWN_TRANSACTION) = _aidl_reply {
      if let Some(_aidl_default_impl) = <Self as ITestService>::getDefaultImpl() {
        return _aidl_default_impl.GetNewNameInterface();
      }
    }
    let _aidl_reply = _aidl_reply?;
    let _aidl_status: binder::Status = _aidl_reply.read()?;
    if !_aidl_status.is_ok() { return Err(_aidl_status); }
    let _aidl_return: binder::Strong<dyn crate::mangled::_7_android_4_aidl_5_tests_8_INewName> = _aidl_reply.read()?;
    Ok(_aidl_return)
  }
  fn GetCppJavaTests(&self) -> binder::public_api::Result<Option<binder::SpIBinder>> {
    let _aidl_reply = self.binder.transact(transactions::GetCppJavaTests, binder::FLAG_CLEAR_BUF | binder::FLAG_PRIVATE_LOCAL, |_aidl_data| {
      _aidl_data.mark_sensitive();
      Ok(())
    });
    if let Err(binder::StatusCode::UNKNOWN_TRANSACTION) = _aidl_reply {
      if let Some(_aidl_default_impl) = <Self as ITestService>::getDefaultImpl() {
        return _aidl_default_impl.GetCppJavaTests();
      }
    }
    let _aidl_reply = _aidl_reply?;
    let _aidl_status: binder::Status = _aidl_reply.read()?;
    if !_aidl_status.is_ok() { return Err(_aidl_status); }
    let _aidl_return: Option<binder::SpIBinder> = _aidl_reply.read()?;
    Ok(_aidl_return)
  }
  fn getBackendType(&self) -> binder::public_api::Result<crate::mangled::_7_android_4_aidl_5_tests_11_BackendType> {
    let _aidl_reply = self.binder.transact(transactions::getBackendType, binder::FLAG_CLEAR_BUF | binder::FLAG_PRIVATE_LOCAL, |_aidl_data| {
      _aidl_data.mark_sensitive();
      Ok(())
    });
    if let Err(binder::StatusCode::UNKNOWN_TRANSACTION) = _aidl_reply {
      if let Some(_aidl_default_impl) = <Self as ITestService>::getDefaultImpl() {
        return _aidl_default_impl.getBackendType();
      }
    }
    let _aidl_reply = _aidl_reply?;
    let _aidl_status: binder::Status = _aidl_reply.read()?;
    if !_aidl_status.is_ok() { return Err(_aidl_status); }
    let _aidl_return: crate::mangled::_7_android_4_aidl_5_tests_11_BackendType = _aidl_reply.read()?;
    Ok(_aidl_return)
  }
}
impl ITestService for binder::Binder<BnTestService> {
  fn UnimplementedMethod(&self, _arg_arg: i32) -> binder::public_api::Result<i32> { self.0.UnimplementedMethod(_arg_arg) }
  fn Deprecated(&self) -> binder::public_api::Result<()> { self.0.Deprecated() }
  fn TestOneway(&self) -> binder::public_api::Result<()> { self.0.TestOneway() }
  fn RepeatBoolean(&self, _arg_token: bool) -> binder::public_api::Result<bool> { self.0.RepeatBoolean(_arg_token) }
  fn RepeatByte(&self, _arg_token: i8) -> binder::public_api::Result<i8> { self.0.RepeatByte(_arg_token) }
  fn RepeatChar(&self, _arg_token: u16) -> binder::public_api::Result<u16> { self.0.RepeatChar(_arg_token) }
  fn RepeatInt(&self, _arg_token: i32) -> binder::public_api::Result<i32> { self.0.RepeatInt(_arg_token) }
  fn RepeatLong(&self, _arg_token: i64) -> binder::public_api::Result<i64> { self.0.RepeatLong(_arg_token) }
  fn RepeatFloat(&self, _arg_token: f32) -> binder::public_api::Result<f32> { self.0.RepeatFloat(_arg_token) }
  fn RepeatDouble(&self, _arg_token: f64) -> binder::public_api::Result<f64> { self.0.RepeatDouble(_arg_token) }
  fn RepeatString(&self, _arg_token: &str) -> binder::public_api::Result<String> { self.0.RepeatString(_arg_token) }
  fn RepeatByteEnum(&self, _arg_token: crate::mangled::_7_android_4_aidl_5_tests_8_ByteEnum) -> binder::public_api::Result<crate::mangled::_7_android_4_aidl_5_tests_8_ByteEnum> { self.0.RepeatByteEnum(_arg_token) }
  fn RepeatIntEnum(&self, _arg_token: crate::mangled::_7_android_4_aidl_5_tests_7_IntEnum) -> binder::public_api::Result<crate::mangled::_7_android_4_aidl_5_tests_7_IntEnum> { self.0.RepeatIntEnum(_arg_token) }
  fn RepeatLongEnum(&self, _arg_token: crate::mangled::_7_android_4_aidl_5_tests_8_LongEnum) -> binder::public_api::Result<crate::mangled::_7_android_4_aidl_5_tests_8_LongEnum> { self.0.RepeatLongEnum(_arg_token) }
  fn ReverseBoolean(&self, _arg_input: &[bool], _arg_repeated: &mut Vec<bool>) -> binder::public_api::Result<Vec<bool>> { self.0.ReverseBoolean(_arg_input, _arg_repeated) }
  fn ReverseByte(&self, _arg_input: &[u8], _arg_repeated: &mut Vec<u8>) -> binder::public_api::Result<Vec<u8>> { self.0.ReverseByte(_arg_input, _arg_repeated) }
  fn ReverseChar(&self, _arg_input: &[u16], _arg_repeated: &mut Vec<u16>) -> binder::public_api::Result<Vec<u16>> { self.0.ReverseChar(_arg_input, _arg_repeated) }
  fn ReverseInt(&self, _arg_input: &[i32], _arg_repeated: &mut Vec<i32>) -> binder::public_api::Result<Vec<i32>> { self.0.ReverseInt(_arg_input, _arg_repeated) }
  fn ReverseLong(&self, _arg_input: &[i64], _arg_repeated: &mut Vec<i64>) -> binder::public_api::Result<Vec<i64>> { self.0.ReverseLong(_arg_input, _arg_repeated) }
  fn ReverseFloat(&self, _arg_input: &[f32], _arg_repeated: &mut Vec<f32>) -> binder::public_api::Result<Vec<f32>> { self.0.ReverseFloat(_arg_input, _arg_repeated) }
  fn ReverseDouble(&self, _arg_input: &[f64], _arg_repeated: &mut Vec<f64>) -> binder::public_api::Result<Vec<f64>> { self.0.ReverseDouble(_arg_input, _arg_repeated) }
  fn ReverseString(&self, _arg_input: &[String], _arg_repeated: &mut Vec<String>) -> binder::public_api::Result<Vec<String>> { self.0.ReverseString(_arg_input, _arg_repeated) }
  fn ReverseByteEnum(&self, _arg_input: &[crate::mangled::_7_android_4_aidl_5_tests_8_ByteEnum], _arg_repeated: &mut Vec<crate::mangled::_7_android_4_aidl_5_tests_8_ByteEnum>) -> binder::public_api::Result<Vec<crate::mangled::_7_android_4_aidl_5_tests_8_ByteEnum>> { self.0.ReverseByteEnum(_arg_input, _arg_repeated) }
  fn ReverseIntEnum(&self, _arg_input: &[crate::mangled::_7_android_4_aidl_5_tests_7_IntEnum], _arg_repeated: &mut Vec<crate::mangled::_7_android_4_aidl_5_tests_7_IntEnum>) -> binder::public_api::Result<Vec<crate::mangled::_7_android_4_aidl_5_tests_7_IntEnum>> { self.0.ReverseIntEnum(_arg_input, _arg_repeated) }
  fn ReverseLongEnum(&self, _arg_input: &[crate::mangled::_7_android_4_aidl_5_tests_8_LongEnum], _arg_repeated: &mut Vec<crate::mangled::_7_android_4_aidl_5_tests_8_LongEnum>) -> binder::public_api::Result<Vec<crate::mangled::_7_android_4_aidl_5_tests_8_LongEnum>> { self.0.ReverseLongEnum(_arg_input, _arg_repeated) }
  fn GetOtherTestService(&self, _arg_name: &str) -> binder::public_api::Result<binder::Strong<dyn crate::mangled::_7_android_4_aidl_5_tests_14_INamedCallback>> { self.0.GetOtherTestService(_arg_name) }
  fn VerifyName(&self, _arg_service: &binder::Strong<dyn crate::mangled::_7_android_4_aidl_5_tests_14_INamedCallback>, _arg_name: &str) -> binder::public_api::Result<bool> { self.0.VerifyName(_arg_service, _arg_name) }
  fn ReverseStringList(&self, _arg_input: &[String], _arg_repeated: &mut Vec<String>) -> binder::public_api::Result<Vec<String>> { self.0.ReverseStringList(_arg_input, _arg_repeated) }
  fn RepeatParcelFileDescriptor(&self, _arg_read: &binder::parcel::ParcelFileDescriptor) -> binder::public_api::Result<binder::parcel::ParcelFileDescriptor> { self.0.RepeatParcelFileDescriptor(_arg_read) }
  fn ReverseParcelFileDescriptorArray(&self, _arg_input: &[binder::parcel::ParcelFileDescriptor], _arg_repeated: &mut Vec<Option<binder::parcel::ParcelFileDescriptor>>) -> binder::public_api::Result<Vec<binder::parcel::ParcelFileDescriptor>> { self.0.ReverseParcelFileDescriptorArray(_arg_input, _arg_repeated) }
  fn ThrowServiceException(&self, _arg_code: i32) -> binder::public_api::Result<()> { self.0.ThrowServiceException(_arg_code) }
  fn RepeatNullableIntArray(&self, _arg_input: Option<&[i32]>) -> binder::public_api::Result<Option<Vec<i32>>> { self.0.RepeatNullableIntArray(_arg_input) }
  fn RepeatNullableByteEnumArray(&self, _arg_input: Option<&[crate::mangled::_7_android_4_aidl_5_tests_8_ByteEnum]>) -> binder::public_api::Result<Option<Vec<crate::mangled::_7_android_4_aidl_5_tests_8_ByteEnum>>> { self.0.RepeatNullableByteEnumArray(_arg_input) }
  fn RepeatNullableIntEnumArray(&self, _arg_input: Option<&[crate::mangled::_7_android_4_aidl_5_tests_7_IntEnum]>) -> binder::public_api::Result<Option<Vec<crate::mangled::_7_android_4_aidl_5_tests_7_IntEnum>>> { self.0.RepeatNullableIntEnumArray(_arg_input) }
  fn RepeatNullableLongEnumArray(&self, _arg_input: Option<&[crate::mangled::_7_android_4_aidl_5_tests_8_LongEnum]>) -> binder::public_api::Result<Option<Vec<crate::mangled::_7_android_4_aidl_5_tests_8_LongEnum>>> { self.0.RepeatNullableLongEnumArray(_arg_input) }
  fn RepeatNullableString(&self, _arg_input: Option<&str>) -> binder::public_api::Result<Option<String>> { self.0.RepeatNullableString(_arg_input) }
  fn RepeatNullableStringList(&self, _arg_input: Option<&[Option<String>]>) -> binder::public_api::Result<Option<Vec<Option<String>>>> { self.0.RepeatNullableStringList(_arg_input) }
  fn RepeatNullableParcelable(&self, _arg_input: Option<&crate::mangled::_7_android_4_aidl_5_tests_20_StructuredParcelable>) -> binder::public_api::Result<Option<crate::mangled::_7_android_4_aidl_5_tests_20_StructuredParcelable>> { self.0.RepeatNullableParcelable(_arg_input) }
  fn TakesAnIBinder(&self, _arg_input: &binder::SpIBinder) -> binder::public_api::Result<()> { self.0.TakesAnIBinder(_arg_input) }
  fn TakesANullableIBinder(&self, _arg_input: Option<&binder::SpIBinder>) -> binder::public_api::Result<()> { self.0.TakesANullableIBinder(_arg_input) }
  fn RepeatUtf8CppString(&self, _arg_token: &str) -> binder::public_api::Result<String> { self.0.RepeatUtf8CppString(_arg_token) }
  fn RepeatNullableUtf8CppString(&self, _arg_token: Option<&str>) -> binder::public_api::Result<Option<String>> { self.0.RepeatNullableUtf8CppString(_arg_token) }
  fn ReverseUtf8CppString(&self, _arg_input: &[String], _arg_repeated: &mut Vec<String>) -> binder::public_api::Result<Vec<String>> { self.0.ReverseUtf8CppString(_arg_input, _arg_repeated) }
  fn ReverseNullableUtf8CppString(&self, _arg_input: Option<&[Option<String>]>, _arg_repeated: &mut Option<Vec<Option<String>>>) -> binder::public_api::Result<Option<Vec<Option<String>>>> { self.0.ReverseNullableUtf8CppString(_arg_input, _arg_repeated) }
  fn ReverseUtf8CppStringList(&self, _arg_input: Option<&[Option<String>]>, _arg_repeated: &mut Option<Vec<Option<String>>>) -> binder::public_api::Result<Option<Vec<Option<String>>>> { self.0.ReverseUtf8CppStringList(_arg_input, _arg_repeated) }
  fn GetCallback(&self, _arg_return_null: bool) -> binder::public_api::Result<Option<binder::Strong<dyn crate::mangled::_7_android_4_aidl_5_tests_14_INamedCallback>>> { self.0.GetCallback(_arg_return_null) }
  fn FillOutStructuredParcelable(&self, _arg_parcel: &mut crate::mangled::_7_android_4_aidl_5_tests_20_StructuredParcelable) -> binder::public_api::Result<()> { self.0.FillOutStructuredParcelable(_arg_parcel) }
  fn GetOldNameInterface(&self) -> binder::public_api::Result<binder::Strong<dyn crate::mangled::_7_android_4_aidl_5_tests_8_IOldName>> { self.0.GetOldNameInterface() }
  fn GetNewNameInterface(&self) -> binder::public_api::Result<binder::Strong<dyn crate::mangled::_7_android_4_aidl_5_tests_8_INewName>> { self.0.GetNewNameInterface() }
  fn GetCppJavaTests(&self) -> binder::public_api::Result<Option<binder::SpIBinder>> { self.0.GetCppJavaTests() }
  fn getBackendType(&self) -> binder::public_api::Result<crate::mangled::_7_android_4_aidl_5_tests_11_BackendType> { self.0.getBackendType() }
}
fn on_transact(_aidl_service: &dyn ITestService, _aidl_code: binder::TransactionCode, _aidl_data: &binder::parcel::Parcel, _aidl_reply: &mut binder::parcel::Parcel) -> binder::Result<()> {
  match _aidl_code {
    transactions::UnimplementedMethod => {
      let _arg_arg: i32 = _aidl_data.read()?;
      let _aidl_return = _aidl_service.UnimplementedMethod(_arg_arg);
      match &_aidl_return {
        Ok(_aidl_return) => {
          _aidl_reply.write(&binder::Status::from(binder::StatusCode::OK))?;
          _aidl_reply.write(_aidl_return)?;
        }
        Err(_aidl_status) => _aidl_reply.write(_aidl_status)?
      }
      Ok(())
    }
    transactions::Deprecated => {
      let _aidl_return = _aidl_service.Deprecated();
      match &_aidl_return {
        Ok(_aidl_return) => {
          _aidl_reply.write(&binder::Status::from(binder::StatusCode::OK))?;
        }
        Err(_aidl_status) => _aidl_reply.write(_aidl_status)?
      }
      Ok(())
    }
    transactions::TestOneway => {
      let _aidl_return = _aidl_service.TestOneway();
      Ok(())
    }
    transactions::RepeatBoolean => {
      let _arg_token: bool = _aidl_data.read()?;
      let _aidl_return = _aidl_service.RepeatBoolean(_arg_token);
      match &_aidl_return {
        Ok(_aidl_return) => {
          _aidl_reply.write(&binder::Status::from(binder::StatusCode::OK))?;
          _aidl_reply.write(_aidl_return)?;
        }
        Err(_aidl_status) => _aidl_reply.write(_aidl_status)?
      }
      Ok(())
    }
    transactions::RepeatByte => {
      let _arg_token: i8 = _aidl_data.read()?;
      let _aidl_return = _aidl_service.RepeatByte(_arg_token);
      match &_aidl_return {
        Ok(_aidl_return) => {
          _aidl_reply.write(&binder::Status::from(binder::StatusCode::OK))?;
          _aidl_reply.write(_aidl_return)?;
        }
        Err(_aidl_status) => _aidl_reply.write(_aidl_status)?
      }
      Ok(())
    }
    transactions::RepeatChar => {
      let _arg_token: u16 = _aidl_data.read()?;
      let _aidl_return = _aidl_service.RepeatChar(_arg_token);
      match &_aidl_return {
        Ok(_aidl_return) => {
          _aidl_reply.write(&binder::Status::from(binder::StatusCode::OK))?;
          _aidl_reply.write(_aidl_return)?;
        }
        Err(_aidl_status) => _aidl_reply.write(_aidl_status)?
      }
      Ok(())
    }
    transactions::RepeatInt => {
      let _arg_token: i32 = _aidl_data.read()?;
      let _aidl_return = _aidl_service.RepeatInt(_arg_token);
      match &_aidl_return {
        Ok(_aidl_return) => {
          _aidl_reply.write(&binder::Status::from(binder::StatusCode::OK))?;
          _aidl_reply.write(_aidl_return)?;
        }
        Err(_aidl_status) => _aidl_reply.write(_aidl_status)?
      }
      Ok(())
    }
    transactions::RepeatLong => {
      let _arg_token: i64 = _aidl_data.read()?;
      let _aidl_return = _aidl_service.RepeatLong(_arg_token);
      match &_aidl_return {
        Ok(_aidl_return) => {
          _aidl_reply.write(&binder::Status::from(binder::StatusCode::OK))?;
          _aidl_reply.write(_aidl_return)?;
        }
        Err(_aidl_status) => _aidl_reply.write(_aidl_status)?
      }
      Ok(())
    }
    transactions::RepeatFloat => {
      let _arg_token: f32 = _aidl_data.read()?;
      let _aidl_return = _aidl_service.RepeatFloat(_arg_token);
      match &_aidl_return {
        Ok(_aidl_return) => {
          _aidl_reply.write(&binder::Status::from(binder::StatusCode::OK))?;
          _aidl_reply.write(_aidl_return)?;
        }
        Err(_aidl_status) => _aidl_reply.write(_aidl_status)?
      }
      Ok(())
    }
    transactions::RepeatDouble => {
      let _arg_token: f64 = _aidl_data.read()?;
      let _aidl_return = _aidl_service.RepeatDouble(_arg_token);
      match &_aidl_return {
        Ok(_aidl_return) => {
          _aidl_reply.write(&binder::Status::from(binder::StatusCode::OK))?;
          _aidl_reply.write(_aidl_return)?;
        }
        Err(_aidl_status) => _aidl_reply.write(_aidl_status)?
      }
      Ok(())
    }
    transactions::RepeatString => {
      let _arg_token: String = _aidl_data.read()?;
      let _aidl_return = _aidl_service.RepeatString(&_arg_token);
      match &_aidl_return {
        Ok(_aidl_return) => {
          _aidl_reply.write(&binder::Status::from(binder::StatusCode::OK))?;
          _aidl_reply.write(_aidl_return)?;
        }
        Err(_aidl_status) => _aidl_reply.write(_aidl_status)?
      }
      Ok(())
    }
    transactions::RepeatByteEnum => {
      let _arg_token: crate::mangled::_7_android_4_aidl_5_tests_8_ByteEnum = _aidl_data.read()?;
      let _aidl_return = _aidl_service.RepeatByteEnum(_arg_token);
      match &_aidl_return {
        Ok(_aidl_return) => {
          _aidl_reply.write(&binder::Status::from(binder::StatusCode::OK))?;
          _aidl_reply.write(_aidl_return)?;
        }
        Err(_aidl_status) => _aidl_reply.write(_aidl_status)?
      }
      Ok(())
    }
    transactions::RepeatIntEnum => {
      let _arg_token: crate::mangled::_7_android_4_aidl_5_tests_7_IntEnum = _aidl_data.read()?;
      let _aidl_return = _aidl_service.RepeatIntEnum(_arg_token);
      match &_aidl_return {
        Ok(_aidl_return) => {
          _aidl_reply.write(&binder::Status::from(binder::StatusCode::OK))?;
          _aidl_reply.write(_aidl_return)?;
        }
        Err(_aidl_status) => _aidl_reply.write(_aidl_status)?
      }
      Ok(())
    }
    transactions::RepeatLongEnum => {
      let _arg_token: crate::mangled::_7_android_4_aidl_5_tests_8_LongEnum = _aidl_data.read()?;
      let _aidl_return = _aidl_service.RepeatLongEnum(_arg_token);
      match &_aidl_return {
        Ok(_aidl_return) => {
          _aidl_reply.write(&binder::Status::from(binder::StatusCode::OK))?;
          _aidl_reply.write(_aidl_return)?;
        }
        Err(_aidl_status) => _aidl_reply.write(_aidl_status)?
      }
      Ok(())
    }
    transactions::ReverseBoolean => {
      let _arg_input: Vec<bool> = _aidl_data.read()?;
      let mut _arg_repeated: Vec<bool> = Default::default();
      _aidl_data.resize_out_vec(&mut _arg_repeated)?;
      let _aidl_return = _aidl_service.ReverseBoolean(&_arg_input, &mut _arg_repeated);
      match &_aidl_return {
        Ok(_aidl_return) => {
          _aidl_reply.write(&binder::Status::from(binder::StatusCode::OK))?;
          _aidl_reply.write(_aidl_return)?;
          _aidl_reply.write(&_arg_repeated)?;
        }
        Err(_aidl_status) => _aidl_reply.write(_aidl_status)?
      }
      Ok(())
    }
    transactions::ReverseByte => {
      let _arg_input: Vec<u8> = _aidl_data.read()?;
      let mut _arg_repeated: Vec<u8> = Default::default();
      _aidl_data.resize_out_vec(&mut _arg_repeated)?;
      let _aidl_return = _aidl_service.ReverseByte(&_arg_input, &mut _arg_repeated);
      match &_aidl_return {
        Ok(_aidl_return) => {
          _aidl_reply.write(&binder::Status::from(binder::StatusCode::OK))?;
          _aidl_reply.write(_aidl_return)?;
          _aidl_reply.write(&_arg_repeated)?;
        }
        Err(_aidl_status) => _aidl_reply.write(_aidl_status)?
      }
      Ok(())
    }
    transactions::ReverseChar => {
      let _arg_input: Vec<u16> = _aidl_data.read()?;
      let mut _arg_repeated: Vec<u16> = Default::default();
      _aidl_data.resize_out_vec(&mut _arg_repeated)?;
      let _aidl_return = _aidl_service.ReverseChar(&_arg_input, &mut _arg_repeated);
      match &_aidl_return {
        Ok(_aidl_return) => {
          _aidl_reply.write(&binder::Status::from(binder::StatusCode::OK))?;
          _aidl_reply.write(_aidl_return)?;
          _aidl_reply.write(&_arg_repeated)?;
        }
        Err(_aidl_status) => _aidl_reply.write(_aidl_status)?
      }
      Ok(())
    }
    transactions::ReverseInt => {
      let _arg_input: Vec<i32> = _aidl_data.read()?;
      let mut _arg_repeated: Vec<i32> = Default::default();
      _aidl_data.resize_out_vec(&mut _arg_repeated)?;
      let _aidl_return = _aidl_service.ReverseInt(&_arg_input, &mut _arg_repeated);
      match &_aidl_return {
        Ok(_aidl_return) => {
          _aidl_reply.write(&binder::Status::from(binder::StatusCode::OK))?;
          _aidl_reply.write(_aidl_return)?;
          _aidl_reply.write(&_arg_repeated)?;
        }
        Err(_aidl_status) => _aidl_reply.write(_aidl_status)?
      }
      Ok(())
    }
    transactions::ReverseLong => {
      let _arg_input: Vec<i64> = _aidl_data.read()?;
      let mut _arg_repeated: Vec<i64> = Default::default();
      _aidl_data.resize_out_vec(&mut _arg_repeated)?;
      let _aidl_return = _aidl_service.ReverseLong(&_arg_input, &mut _arg_repeated);
      match &_aidl_return {
        Ok(_aidl_return) => {
          _aidl_reply.write(&binder::Status::from(binder::StatusCode::OK))?;
          _aidl_reply.write(_aidl_return)?;
          _aidl_reply.write(&_arg_repeated)?;
        }
        Err(_aidl_status) => _aidl_reply.write(_aidl_status)?
      }
      Ok(())
    }
    transactions::ReverseFloat => {
      let _arg_input: Vec<f32> = _aidl_data.read()?;
      let mut _arg_repeated: Vec<f32> = Default::default();
      _aidl_data.resize_out_vec(&mut _arg_repeated)?;
      let _aidl_return = _aidl_service.ReverseFloat(&_arg_input, &mut _arg_repeated);
      match &_aidl_return {
        Ok(_aidl_return) => {
          _aidl_reply.write(&binder::Status::from(binder::StatusCode::OK))?;
          _aidl_reply.write(_aidl_return)?;
          _aidl_reply.write(&_arg_repeated)?;
        }
        Err(_aidl_status) => _aidl_reply.write(_aidl_status)?
      }
      Ok(())
    }
    transactions::ReverseDouble => {
      let _arg_input: Vec<f64> = _aidl_data.read()?;
      let mut _arg_repeated: Vec<f64> = Default::default();
      _aidl_data.resize_out_vec(&mut _arg_repeated)?;
      let _aidl_return = _aidl_service.ReverseDouble(&_arg_input, &mut _arg_repeated);
      match &_aidl_return {
        Ok(_aidl_return) => {
          _aidl_reply.write(&binder::Status::from(binder::StatusCode::OK))?;
          _aidl_reply.write(_aidl_return)?;
          _aidl_reply.write(&_arg_repeated)?;
        }
        Err(_aidl_status) => _aidl_reply.write(_aidl_status)?
      }
      Ok(())
    }
    transactions::ReverseString => {
      let _arg_input: Vec<String> = _aidl_data.read()?;
      let mut _arg_repeated: Vec<String> = Default::default();
      _aidl_data.resize_out_vec(&mut _arg_repeated)?;
      let _aidl_return = _aidl_service.ReverseString(&_arg_input, &mut _arg_repeated);
      match &_aidl_return {
        Ok(_aidl_return) => {
          _aidl_reply.write(&binder::Status::from(binder::StatusCode::OK))?;
          _aidl_reply.write(_aidl_return)?;
          _aidl_reply.write(&_arg_repeated)?;
        }
        Err(_aidl_status) => _aidl_reply.write(_aidl_status)?
      }
      Ok(())
    }
    transactions::ReverseByteEnum => {
      let _arg_input: Vec<crate::mangled::_7_android_4_aidl_5_tests_8_ByteEnum> = _aidl_data.read()?;
      let mut _arg_repeated: Vec<crate::mangled::_7_android_4_aidl_5_tests_8_ByteEnum> = Default::default();
      _aidl_data.resize_out_vec(&mut _arg_repeated)?;
      let _aidl_return = _aidl_service.ReverseByteEnum(&_arg_input, &mut _arg_repeated);
      match &_aidl_return {
        Ok(_aidl_return) => {
          _aidl_reply.write(&binder::Status::from(binder::StatusCode::OK))?;
          _aidl_reply.write(_aidl_return)?;
          _aidl_reply.write(&_arg_repeated)?;
        }
        Err(_aidl_status) => _aidl_reply.write(_aidl_status)?
      }
      Ok(())
    }
    transactions::ReverseIntEnum => {
      let _arg_input: Vec<crate::mangled::_7_android_4_aidl_5_tests_7_IntEnum> = _aidl_data.read()?;
      let mut _arg_repeated: Vec<crate::mangled::_7_android_4_aidl_5_tests_7_IntEnum> = Default::default();
      _aidl_data.resize_out_vec(&mut _arg_repeated)?;
      let _aidl_return = _aidl_service.ReverseIntEnum(&_arg_input, &mut _arg_repeated);
      match &_aidl_return {
        Ok(_aidl_return) => {
          _aidl_reply.write(&binder::Status::from(binder::StatusCode::OK))?;
          _aidl_reply.write(_aidl_return)?;
          _aidl_reply.write(&_arg_repeated)?;
        }
        Err(_aidl_status) => _aidl_reply.write(_aidl_status)?
      }
      Ok(())
    }
    transactions::ReverseLongEnum => {
      let _arg_input: Vec<crate::mangled::_7_android_4_aidl_5_tests_8_LongEnum> = _aidl_data.read()?;
      let mut _arg_repeated: Vec<crate::mangled::_7_android_4_aidl_5_tests_8_LongEnum> = Default::default();
      _aidl_data.resize_out_vec(&mut _arg_repeated)?;
      let _aidl_return = _aidl_service.ReverseLongEnum(&_arg_input, &mut _arg_repeated);
      match &_aidl_return {
        Ok(_aidl_return) => {
          _aidl_reply.write(&binder::Status::from(binder::StatusCode::OK))?;
          _aidl_reply.write(_aidl_return)?;
          _aidl_reply.write(&_arg_repeated)?;
        }
        Err(_aidl_status) => _aidl_reply.write(_aidl_status)?
      }
      Ok(())
    }
    transactions::GetOtherTestService => {
      let _arg_name: String = _aidl_data.read()?;
      let _aidl_return = _aidl_service.GetOtherTestService(&_arg_name);
      match &_aidl_return {
        Ok(_aidl_return) => {
          _aidl_reply.write(&binder::Status::from(binder::StatusCode::OK))?;
          _aidl_reply.write(_aidl_return)?;
        }
        Err(_aidl_status) => _aidl_reply.write(_aidl_status)?
      }
      Ok(())
    }
    transactions::VerifyName => {
      let _arg_service: binder::Strong<dyn crate::mangled::_7_android_4_aidl_5_tests_14_INamedCallback> = _aidl_data.read()?;
      let _arg_name: String = _aidl_data.read()?;
      let _aidl_return = _aidl_service.VerifyName(&_arg_service, &_arg_name);
      match &_aidl_return {
        Ok(_aidl_return) => {
          _aidl_reply.write(&binder::Status::from(binder::StatusCode::OK))?;
          _aidl_reply.write(_aidl_return)?;
        }
        Err(_aidl_status) => _aidl_reply.write(_aidl_status)?
      }
      Ok(())
    }
    transactions::ReverseStringList => {
      let _arg_input: Vec<String> = _aidl_data.read()?;
      let mut _arg_repeated: Vec<String> = Default::default();
      let _aidl_return = _aidl_service.ReverseStringList(&_arg_input, &mut _arg_repeated);
      match &_aidl_return {
        Ok(_aidl_return) => {
          _aidl_reply.write(&binder::Status::from(binder::StatusCode::OK))?;
          _aidl_reply.write(_aidl_return)?;
          _aidl_reply.write(&_arg_repeated)?;
        }
        Err(_aidl_status) => _aidl_reply.write(_aidl_status)?
      }
      Ok(())
    }
    transactions::RepeatParcelFileDescriptor => {
      let _arg_read: binder::parcel::ParcelFileDescriptor = _aidl_data.read()?;
      let _aidl_return = _aidl_service.RepeatParcelFileDescriptor(&_arg_read);
      match &_aidl_return {
        Ok(_aidl_return) => {
          _aidl_reply.write(&binder::Status::from(binder::StatusCode::OK))?;
          _aidl_reply.write(_aidl_return)?;
        }
        Err(_aidl_status) => _aidl_reply.write(_aidl_status)?
      }
      Ok(())
    }
    transactions::ReverseParcelFileDescriptorArray => {
      let _arg_input: Vec<binder::parcel::ParcelFileDescriptor> = _aidl_data.read()?;
      let mut _arg_repeated: Vec<Option<binder::parcel::ParcelFileDescriptor>> = Default::default();
      _aidl_data.resize_out_vec(&mut _arg_repeated)?;
      let _aidl_return = _aidl_service.ReverseParcelFileDescriptorArray(&_arg_input, &mut _arg_repeated);
      match &_aidl_return {
        Ok(_aidl_return) => {
          _aidl_reply.write(&binder::Status::from(binder::StatusCode::OK))?;
          _aidl_reply.write(_aidl_return)?;
          if _arg_repeated.iter().any(Option::is_none) { return Err(binder::StatusCode::UNEXPECTED_NULL); }
          _aidl_reply.write(&_arg_repeated)?;
        }
        Err(_aidl_status) => _aidl_reply.write(_aidl_status)?
      }
      Ok(())
    }
    transactions::ThrowServiceException => {
      let _arg_code: i32 = _aidl_data.read()?;
      let _aidl_return = _aidl_service.ThrowServiceException(_arg_code);
      match &_aidl_return {
        Ok(_aidl_return) => {
          _aidl_reply.write(&binder::Status::from(binder::StatusCode::OK))?;
        }
        Err(_aidl_status) => _aidl_reply.write(_aidl_status)?
      }
      Ok(())
    }
    transactions::RepeatNullableIntArray => {
      let _arg_input: Option<Vec<i32>> = _aidl_data.read()?;
      let _aidl_return = _aidl_service.RepeatNullableIntArray(_arg_input.as_deref());
      match &_aidl_return {
        Ok(_aidl_return) => {
          _aidl_reply.write(&binder::Status::from(binder::StatusCode::OK))?;
          _aidl_reply.write(_aidl_return)?;
        }
        Err(_aidl_status) => _aidl_reply.write(_aidl_status)?
      }
      Ok(())
    }
    transactions::RepeatNullableByteEnumArray => {
      let _arg_input: Option<Vec<crate::mangled::_7_android_4_aidl_5_tests_8_ByteEnum>> = _aidl_data.read()?;
      let _aidl_return = _aidl_service.RepeatNullableByteEnumArray(_arg_input.as_deref());
      match &_aidl_return {
        Ok(_aidl_return) => {
          _aidl_reply.write(&binder::Status::from(binder::StatusCode::OK))?;
          _aidl_reply.write(_aidl_return)?;
        }
        Err(_aidl_status) => _aidl_reply.write(_aidl_status)?
      }
      Ok(())
    }
    transactions::RepeatNullableIntEnumArray => {
      let _arg_input: Option<Vec<crate::mangled::_7_android_4_aidl_5_tests_7_IntEnum>> = _aidl_data.read()?;
      let _aidl_return = _aidl_service.RepeatNullableIntEnumArray(_arg_input.as_deref());
      match &_aidl_return {
        Ok(_aidl_return) => {
          _aidl_reply.write(&binder::Status::from(binder::StatusCode::OK))?;
          _aidl_reply.write(_aidl_return)?;
        }
        Err(_aidl_status) => _aidl_reply.write(_aidl_status)?
      }
      Ok(())
    }
    transactions::RepeatNullableLongEnumArray => {
      let _arg_input: Option<Vec<crate::mangled::_7_android_4_aidl_5_tests_8_LongEnum>> = _aidl_data.read()?;
      let _aidl_return = _aidl_service.RepeatNullableLongEnumArray(_arg_input.as_deref());
      match &_aidl_return {
        Ok(_aidl_return) => {
          _aidl_reply.write(&binder::Status::from(binder::StatusCode::OK))?;
          _aidl_reply.write(_aidl_return)?;
        }
        Err(_aidl_status) => _aidl_reply.write(_aidl_status)?
      }
      Ok(())
    }
    transactions::RepeatNullableString => {
      let _arg_input: Option<String> = _aidl_data.read()?;
      let _aidl_return = _aidl_service.RepeatNullableString(_arg_input.as_deref());
      match &_aidl_return {
        Ok(_aidl_return) => {
          _aidl_reply.write(&binder::Status::from(binder::StatusCode::OK))?;
          _aidl_reply.write(_aidl_return)?;
        }
        Err(_aidl_status) => _aidl_reply.write(_aidl_status)?
      }
      Ok(())
    }
    transactions::RepeatNullableStringList => {
      let _arg_input: Option<Vec<Option<String>>> = _aidl_data.read()?;
      let _aidl_return = _aidl_service.RepeatNullableStringList(_arg_input.as_deref());
      match &_aidl_return {
        Ok(_aidl_return) => {
          _aidl_reply.write(&binder::Status::from(binder::StatusCode::OK))?;
          _aidl_reply.write(_aidl_return)?;
        }
        Err(_aidl_status) => _aidl_reply.write(_aidl_status)?
      }
      Ok(())
    }
    transactions::RepeatNullableParcelable => {
      let _arg_input: Option<crate::mangled::_7_android_4_aidl_5_tests_20_StructuredParcelable> = _aidl_data.read()?;
      let _aidl_return = _aidl_service.RepeatNullableParcelable(_arg_input.as_ref());
      match &_aidl_return {
        Ok(_aidl_return) => {
          _aidl_reply.write(&binder::Status::from(binder::StatusCode::OK))?;
          _aidl_reply.write(_aidl_return)?;
        }
        Err(_aidl_status) => _aidl_reply.write(_aidl_status)?
      }
      Ok(())
    }
    transactions::TakesAnIBinder => {
      let _arg_input: binder::SpIBinder = _aidl_data.read()?;
      let _aidl_return = _aidl_service.TakesAnIBinder(&_arg_input);
      match &_aidl_return {
        Ok(_aidl_return) => {
          _aidl_reply.write(&binder::Status::from(binder::StatusCode::OK))?;
        }
        Err(_aidl_status) => _aidl_reply.write(_aidl_status)?
      }
      Ok(())
    }
    transactions::TakesANullableIBinder => {
      let _arg_input: Option<binder::SpIBinder> = _aidl_data.read()?;
      let _aidl_return = _aidl_service.TakesANullableIBinder(_arg_input.as_ref());
      match &_aidl_return {
        Ok(_aidl_return) => {
          _aidl_reply.write(&binder::Status::from(binder::StatusCode::OK))?;
        }
        Err(_aidl_status) => _aidl_reply.write(_aidl_status)?
      }
      Ok(())
    }
    transactions::RepeatUtf8CppString => {
      let _arg_token: String = _aidl_data.read()?;
      let _aidl_return = _aidl_service.RepeatUtf8CppString(&_arg_token);
      match &_aidl_return {
        Ok(_aidl_return) => {
          _aidl_reply.write(&binder::Status::from(binder::StatusCode::OK))?;
          _aidl_reply.write(_aidl_return)?;
        }
        Err(_aidl_status) => _aidl_reply.write(_aidl_status)?
      }
      Ok(())
    }
    transactions::RepeatNullableUtf8CppString => {
      let _arg_token: Option<String> = _aidl_data.read()?;
      let _aidl_return = _aidl_service.RepeatNullableUtf8CppString(_arg_token.as_deref());
      match &_aidl_return {
        Ok(_aidl_return) => {
          _aidl_reply.write(&binder::Status::from(binder::StatusCode::OK))?;
          _aidl_reply.write(_aidl_return)?;
        }
        Err(_aidl_status) => _aidl_reply.write(_aidl_status)?
      }
      Ok(())
    }
    transactions::ReverseUtf8CppString => {
      let _arg_input: Vec<String> = _aidl_data.read()?;
      let mut _arg_repeated: Vec<String> = Default::default();
      _aidl_data.resize_out_vec(&mut _arg_repeated)?;
      let _aidl_return = _aidl_service.ReverseUtf8CppString(&_arg_input, &mut _arg_repeated);
      match &_aidl_return {
        Ok(_aidl_return) => {
          _aidl_reply.write(&binder::Status::from(binder::StatusCode::OK))?;
          _aidl_reply.write(_aidl_return)?;
          _aidl_reply.write(&_arg_repeated)?;
        }
        Err(_aidl_status) => _aidl_reply.write(_aidl_status)?
      }
      Ok(())
    }
    transactions::ReverseNullableUtf8CppString => {
      let _arg_input: Option<Vec<Option<String>>> = _aidl_data.read()?;
      let mut _arg_repeated: Option<Vec<Option<String>>> = Default::default();
      _aidl_data.resize_nullable_out_vec(&mut _arg_repeated)?;
      let _aidl_return = _aidl_service.ReverseNullableUtf8CppString(_arg_input.as_deref(), &mut _arg_repeated);
      match &_aidl_return {
        Ok(_aidl_return) => {
          _aidl_reply.write(&binder::Status::from(binder::StatusCode::OK))?;
          _aidl_reply.write(_aidl_return)?;
          _aidl_reply.write(&_arg_repeated)?;
        }
        Err(_aidl_status) => _aidl_reply.write(_aidl_status)?
      }
      Ok(())
    }
    transactions::ReverseUtf8CppStringList => {
      let _arg_input: Option<Vec<Option<String>>> = _aidl_data.read()?;
      let mut _arg_repeated: Option<Vec<Option<String>>> = Default::default();
      let _aidl_return = _aidl_service.ReverseUtf8CppStringList(_arg_input.as_deref(), &mut _arg_repeated);
      match &_aidl_return {
        Ok(_aidl_return) => {
          _aidl_reply.write(&binder::Status::from(binder::StatusCode::OK))?;
          _aidl_reply.write(_aidl_return)?;
          _aidl_reply.write(&_arg_repeated)?;
        }
        Err(_aidl_status) => _aidl_reply.write(_aidl_status)?
      }
      Ok(())
    }
    transactions::GetCallback => {
      let _arg_return_null: bool = _aidl_data.read()?;
      let _aidl_return = _aidl_service.GetCallback(_arg_return_null);
      match &_aidl_return {
        Ok(_aidl_return) => {
          _aidl_reply.write(&binder::Status::from(binder::StatusCode::OK))?;
          _aidl_reply.write(_aidl_return)?;
        }
        Err(_aidl_status) => _aidl_reply.write(_aidl_status)?
      }
      Ok(())
    }
    transactions::FillOutStructuredParcelable => {
      let mut _arg_parcel: crate::mangled::_7_android_4_aidl_5_tests_20_StructuredParcelable = _aidl_data.read()?;
      let _aidl_return = _aidl_service.FillOutStructuredParcelable(&mut _arg_parcel);
      match &_aidl_return {
        Ok(_aidl_return) => {
          _aidl_reply.write(&binder::Status::from(binder::StatusCode::OK))?;
          _aidl_reply.write(&_arg_parcel)?;
        }
        Err(_aidl_status) => _aidl_reply.write(_aidl_status)?
      }
      Ok(())
    }
    transactions::GetOldNameInterface => {
      let _aidl_return = _aidl_service.GetOldNameInterface();
      match &_aidl_return {
        Ok(_aidl_return) => {
          _aidl_reply.write(&binder::Status::from(binder::StatusCode::OK))?;
          _aidl_reply.write(_aidl_return)?;
        }
        Err(_aidl_status) => _aidl_reply.write(_aidl_status)?
      }
      Ok(())
    }
    transactions::GetNewNameInterface => {
      let _aidl_return = _aidl_service.GetNewNameInterface();
      match &_aidl_return {
        Ok(_aidl_return) => {
          _aidl_reply.write(&binder::Status::from(binder::StatusCode::OK))?;
          _aidl_reply.write(_aidl_return)?;
        }
        Err(_aidl_status) => _aidl_reply.write(_aidl_status)?
      }
      Ok(())
    }
    transactions::GetCppJavaTests => {
      let _aidl_return = _aidl_service.GetCppJavaTests();
      match &_aidl_return {
        Ok(_aidl_return) => {
          _aidl_reply.write(&binder::Status::from(binder::StatusCode::OK))?;
          _aidl_reply.write(_aidl_return)?;
        }
        Err(_aidl_status) => _aidl_reply.write(_aidl_status)?
      }
      Ok(())
    }
    transactions::getBackendType => {
      let _aidl_return = _aidl_service.getBackendType();
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
