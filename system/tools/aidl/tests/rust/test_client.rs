/*
 * Copyright (C) 2020, The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

//! Test Rust client for the AIDL compiler.

use aidl_test_interface::aidl::android::aidl::tests::INewName::{self, BpNewName};
use aidl_test_interface::aidl::android::aidl::tests::IOldName::{self, BpOldName};
use aidl_test_interface::aidl::android::aidl::tests::ITestService::{
    self, BpTestService, ITestServiceDefault, ITestServiceDefaultRef,
};
use aidl_test_interface::aidl::android::aidl::tests::{
    BackendType::BackendType, ByteEnum::ByteEnum, IntEnum::IntEnum, LongEnum::LongEnum, StructuredParcelable, Union,
};
use aidl_test_interface::aidl::android::aidl::tests::unions::{
    EnumUnion::EnumUnion,
};
use aidl_test_interface::binder;
use aidl_test_versioned_interface::aidl::android::aidl::versioned::tests::{
    IFooInterface, IFooInterface::BpFooInterface, BazUnion::BazUnion,
};
use std::fs::File;
use std::io::{Read, Write};
use std::os::unix::io::FromRawFd;
use std::sync::Arc;

fn get_test_service() -> binder::Strong<dyn ITestService::ITestService> {
    binder::get_interface(<BpTestService as ITestService::ITestService>::get_descriptor())
        .expect("did not get binder service")
}

#[test]
fn test_constants() {
    assert_eq!(ITestService::A1, 1);
    assert_eq!(ITestService::A2, 1);
    assert_eq!(ITestService::A3, 1);
    assert_eq!(ITestService::A4, 1);
    assert_eq!(ITestService::A5, 1);
    assert_eq!(ITestService::A6, 1);
    assert_eq!(ITestService::A7, 1);
    assert_eq!(ITestService::A8, 1);
    assert_eq!(ITestService::A9, 1);
    assert_eq!(ITestService::A10, 1);
    assert_eq!(ITestService::A11, 1);
    assert_eq!(ITestService::A12, 1);
    assert_eq!(ITestService::A13, 1);
    assert_eq!(ITestService::A14, 1);
    assert_eq!(ITestService::A15, 1);
    assert_eq!(ITestService::A16, 1);
    assert_eq!(ITestService::A17, 1);
    assert_eq!(ITestService::A18, 1);
    assert_eq!(ITestService::A19, 1);
    assert_eq!(ITestService::A20, 1);
    assert_eq!(ITestService::A21, 1);
    assert_eq!(ITestService::A22, 1);
    assert_eq!(ITestService::A23, 1);
    assert_eq!(ITestService::A24, 1);
    assert_eq!(ITestService::A25, 1);
    assert_eq!(ITestService::A26, 1);
    assert_eq!(ITestService::A27, 1);
    assert_eq!(ITestService::A28, 1);
    assert_eq!(ITestService::A29, 1);
    assert_eq!(ITestService::A30, 1);
    assert_eq!(ITestService::A31, 1);
    assert_eq!(ITestService::A32, 1);
    assert_eq!(ITestService::A33, 1);
    assert_eq!(ITestService::A34, 1);
    assert_eq!(ITestService::A35, 1);
    assert_eq!(ITestService::A36, 1);
    assert_eq!(ITestService::A37, 1);
    assert_eq!(ITestService::A38, 1);
    assert_eq!(ITestService::A39, 1);
    assert_eq!(ITestService::A40, 1);
    assert_eq!(ITestService::A41, 1);
    assert_eq!(ITestService::A42, 1);
    assert_eq!(ITestService::A43, 1);
    assert_eq!(ITestService::A44, 1);
    assert_eq!(ITestService::A45, 1);
    assert_eq!(ITestService::A46, 1);
    assert_eq!(ITestService::A47, 1);
    assert_eq!(ITestService::A48, 1);
    assert_eq!(ITestService::A49, 1);
    assert_eq!(ITestService::A50, 1);
    assert_eq!(ITestService::A51, 1);
    assert_eq!(ITestService::A52, 1);
    assert_eq!(ITestService::A53, 1);
    assert_eq!(ITestService::A54, 1);
    assert_eq!(ITestService::A55, 1);
    assert_eq!(ITestService::A56, 1);
    assert_eq!(ITestService::A57, 1);
}

#[test]
fn test_oneway() {
    let result = get_test_service().TestOneway();
    assert_eq!(result, Ok(()));
}

macro_rules! test_primitive {
    ($test:ident, $func:ident, $value:expr) => {
        #[test]
        fn $test() {
            let value = $value;
            let result = get_test_service().$func(value);
            assert_eq!(result, Ok(value));
        }
    };
}

test_primitive! {test_primitive_bool_false, RepeatBoolean, false}
test_primitive! {test_primitive_bool_true, RepeatBoolean, true}
test_primitive! {test_primitive_byte, RepeatByte, -128i8}
test_primitive! {test_primitive_char, RepeatChar, 'A' as u16}
test_primitive! {test_primitive_int, RepeatInt, 1i32 << 30}
test_primitive! {test_primitive_long, RepeatLong, 1i64 << 60}
test_primitive! {test_primitive_float, RepeatFloat, 1.0f32 / 3.0f32}
test_primitive! {test_primitive_double, RepeatDouble, 1.0f64 / 3.0f64}
test_primitive! {test_primitive_byte_constant, RepeatByte, ITestService::BYTE_TEST_CONSTANT}
test_primitive! {test_primitive_constant1, RepeatInt, ITestService::TEST_CONSTANT}
test_primitive! {test_primitive_constant2, RepeatInt, ITestService::TEST_CONSTANT2}
test_primitive! {test_primitive_constant3, RepeatInt, ITestService::TEST_CONSTANT3}
test_primitive! {test_primitive_constant4, RepeatInt, ITestService::TEST_CONSTANT4}
test_primitive! {test_primitive_constant5, RepeatInt, ITestService::TEST_CONSTANT5}
test_primitive! {test_primitive_constant6, RepeatInt, ITestService::TEST_CONSTANT6}
test_primitive! {test_primitive_constant7, RepeatInt, ITestService::TEST_CONSTANT7}
test_primitive! {test_primitive_constant8, RepeatInt, ITestService::TEST_CONSTANT8}
test_primitive! {test_primitive_constant9, RepeatInt, ITestService::TEST_CONSTANT9}
test_primitive! {test_primitive_constant10, RepeatInt, ITestService::TEST_CONSTANT10}
test_primitive! {test_primitive_constant11, RepeatInt, ITestService::TEST_CONSTANT11}
test_primitive! {test_primitive_constant12, RepeatInt, ITestService::TEST_CONSTANT12}
test_primitive! {test_primitive_long_constant, RepeatLong, ITestService::LONG_TEST_CONSTANT}
test_primitive! {test_primitive_byte_enum, RepeatByteEnum, ByteEnum::FOO}
test_primitive! {test_primitive_int_enum, RepeatIntEnum, IntEnum::BAR}
test_primitive! {test_primitive_long_enum, RepeatLongEnum, LongEnum::FOO}

#[test]
fn test_repeat_string() {
    let service = get_test_service();
    let inputs = [
        "typical string".into(),
        String::new(),
        "\0\0".into(),
        // This is actually two unicode code points:
        //   U+10437: The 'small letter yee' character in the deseret alphabet
        //   U+20AC: A euro sign
        String::from_utf16(&[0xD801, 0xDC37, 0x20AC]).expect("error converting string"),
        ITestService::STRING_TEST_CONSTANT.into(),
        ITestService::STRING_TEST_CONSTANT2.into(),
    ];
    for input in &inputs {
        let result = service.RepeatString(&input);
        assert_eq!(result.as_ref(), Ok(input));
    }
}

macro_rules! test_reverse_array {
    ($test:ident, $func:ident, $array:expr) => {
        #[test]
        fn $test() {
            let mut array = $array;

            // Java needs initial values here (can't resize arrays)
            let mut repeated = vec![Default::default(); array.len()];

            let result = get_test_service().$func(&array, &mut repeated);
            assert_eq!(repeated, array);
            array.reverse();
            assert_eq!(result, Ok(array));
        }
    };
}

test_reverse_array! {test_array_boolean, ReverseBoolean, vec![true, false, false]}
test_reverse_array! {test_array_byte, ReverseByte, vec![255u8, 0u8, 127u8]}
test_reverse_array! {
    service,
    ReverseChar,
    vec!['A' as u16, 'B' as u16, 'C' as u16]
}
test_reverse_array! {test_array_int, ReverseInt, vec![1, 2, 3]}
test_reverse_array! {test_array_long, ReverseLong, vec![-1i64, 0i64, 1i64 << 60]}
test_reverse_array! {test_array_float, ReverseFloat, vec![-0.3f32, -0.7f32, 8.0f32]}
test_reverse_array! {
    test_array_double,
    ReverseDouble,
    vec![1.0f64 / 3.0f64, 1.0f64 / 7.0f64, 42.0f64]
}
test_reverse_array! {
    test_array_string,
    ReverseString,
    vec!["f".into(), "a".into(), "b".into()]
}
test_reverse_array! {
    test_array_byte_enum,
    ReverseByteEnum,
    vec![ByteEnum::FOO, ByteEnum::BAR, ByteEnum::BAR]
}
test_reverse_array! {
    test_array_byte_enum_v2,
    ReverseByteEnum,
    vec![ByteEnum::FOO, ByteEnum::BAR, ByteEnum::BAZ]
}
test_reverse_array! {
    test_array_int_enum,
    ReverseIntEnum,
    vec![IntEnum::FOO, IntEnum::BAR, IntEnum::BAR]
}
test_reverse_array! {
    test_array_long_enum,
    ReverseLongEnum,
    vec![LongEnum::FOO, LongEnum::BAR, LongEnum::BAR]
}
test_reverse_array! {
    test_array_string_list,
    ReverseStringList,
    vec!["f".into(), "a".into(), "b".into()]
}
test_reverse_array! {
    test_array_utf8_string,
    ReverseUtf8CppString,
    vec![
        "a".into(),
        String::new(),
        std::str::from_utf8(&[0xC3, 0xB8])
            .expect("error converting string")
            .into(),
    ]
}

#[test]
fn test_binder_exchange() {
    const NAME: &str = "Smythe";
    let service = get_test_service();
    let got = service
        .GetOtherTestService(NAME)
        .expect("error calling GetOtherTestService");
    assert_eq!(got.GetName().as_ref().map(String::as_ref), Ok(NAME));
    assert_eq!(service.VerifyName(&got, NAME), Ok(true));
}

fn build_pipe() -> (File, File) {
    // Safety: we get two file descriptors from pipe()
    // and pass them after checking if the function returned
    // without an error, so the descriptors should be valid
    // by that point
    unsafe {
        let mut fds = [0, 0];
        if libc::pipe(fds.as_mut_ptr()) != 0 {
            panic!("pipe() error");
        }
        (File::from_raw_fd(fds[0]), File::from_raw_fd(fds[1]))
    }
}

#[test]
fn test_parcel_file_descriptor() {
    let service = get_test_service();
    let (mut read_file, write_file) = build_pipe();

    let write_pfd = binder::ParcelFileDescriptor::new(write_file);
    let result_pfd = service
        .RepeatParcelFileDescriptor(&write_pfd)
        .expect("error calling RepeatParcelFileDescriptor");

    const TEST_DATA: &[u8] = b"FrazzleSnazzleFlimFlamFlibbityGumboChops";
    result_pfd
        .as_ref()
        .write_all(TEST_DATA)
        .expect("error writing to pipe");

    let mut buf = [0u8; TEST_DATA.len()];
    read_file
        .read_exact(&mut buf)
        .expect("error reading from pipe");
    assert_eq!(&buf[..], TEST_DATA);
}

#[test]
fn test_parcel_file_descriptor_array() {
    let service = get_test_service();

    let (read_file, write_file) = build_pipe();
    let input = vec![
        binder::ParcelFileDescriptor::new(read_file),
        binder::ParcelFileDescriptor::new(write_file),
    ];

    let mut repeated = vec![];

    let backend = service.getBackendType().expect("error getting backend type");
    if backend == BackendType::JAVA {
        // Java needs initial values here (can't resize arrays)
        // Other backends can't accept 'None', but we can use it in Java for convenience, rather
        // than creating file descriptors.
        repeated = vec![None, None];
    }

    let result = service
        .ReverseParcelFileDescriptorArray(&input[..], &mut repeated)
        .expect("error calling ReverseParcelFileDescriptorArray");

    input[1]
        .as_ref()
        .write_all(b"First")
        .expect("error writing to pipe");
    repeated[1]
        .as_mut()
        .expect("received None for ParcelFileDescriptor")
        .as_ref()
        .write_all(b"Second")
        .expect("error writing to pipe");
    result[0]
        .as_ref()
        .write_all(b"Third")
        .expect("error writing to pipe");

    const TEST_DATA: &[u8] = b"FirstSecondThird";
    let mut buf = [0u8; TEST_DATA.len()];
    input[0]
        .as_ref()
        .read_exact(&mut buf)
        .expect("error reading from pipe");
    assert_eq!(&buf[..], TEST_DATA);
}

#[test]
fn test_service_specific_exception() {
    let service = get_test_service();

    let backend = service.getBackendType().expect("error getting backend type");
    if backend == BackendType::JAVA {
        // TODO(b/178861468): not correctly thrown from Java
        return;
    }

    for i in -1..2 {
        let result = service.ThrowServiceException(i);
        assert!(result.is_err());

        let status = result.unwrap_err();
        assert_eq!(
            status.exception_code(),
            binder::ExceptionCode::SERVICE_SPECIFIC
        );
        assert_eq!(status.service_specific_error(), i);
    }
}

macro_rules! test_nullable {
    ($test:ident, $func:ident, $value:expr) => {
        #[test]
        fn $test() {
            let service = get_test_service();
            let value = Some($value);
            let result = service.$func(value.as_deref());
            assert_eq!(result, Ok(value));

            let result = service.$func(None);
            assert_eq!(result, Ok(None));
        }
    };
}

test_nullable! {test_nullable_array_int, RepeatNullableIntArray, vec![1, 2, 3]}
test_nullable! {
    test_nullable_array_byte_enum,
    RepeatNullableByteEnumArray,
    vec![ByteEnum::FOO, ByteEnum::BAR]
}
test_nullable! {
    test_nullable_array_int_enum,
    RepeatNullableIntEnumArray,
    vec![IntEnum::FOO, IntEnum::BAR]
}
test_nullable! {
    test_nullable_array_long_enum,
    RepeatNullableLongEnumArray,
    vec![LongEnum::FOO, LongEnum::BAR]
}
test_nullable! {test_nullable_string, RepeatNullableString, "Blooob".into()}
test_nullable! {
    test_nullable_string_list,
    RepeatNullableStringList,
    vec![
        Some("Wat".into()),
        Some("Blooob".into()),
        Some("Wat".into()),
        None,
        Some("YEAH".into()),
        Some("OKAAAAY".into()),
    ]
}

#[test]
fn test_nullable_parcelable() {
    let value = StructuredParcelable::StructuredParcelable{
        f: 42,
        ..Default::default()
    };

    let service = get_test_service();
    let value = Some(value);
    let result = service.RepeatNullableParcelable(value.as_ref());
    assert_eq!(result, Ok(value));

    let result = service.RepeatNullableParcelable(None);
    assert_eq!(result, Ok(None));
}

#[test]
fn test_binder() {
    let service = get_test_service();
    assert!(service
        .GetCallback(true)
        .expect("error calling GetCallback")
        .is_none());
    let callback = service
        .GetCallback(false)
        .expect("error calling GetCallback")
        .expect("expected Some from GetCallback");

    // We don't have any place to get a fresh `SpIBinder`, so we
    // reuse the interface for the binder tests
    let binder = callback.as_binder();
    assert_eq!(service.TakesAnIBinder(&binder), Ok(()));
    assert_eq!(service.TakesANullableIBinder(None), Ok(()));
    assert_eq!(service.TakesANullableIBinder(Some(&binder)), Ok(()));
}

macro_rules! test_reverse_null_array {
    ($service:expr, $func:ident, $expect_repeated:expr) => {{
        let mut repeated = None;
        let result = $service.$func(None, &mut repeated);
        assert_eq!(repeated, $expect_repeated);
        assert_eq!(result, Ok(None));
    }};
}

macro_rules! test_reverse_nullable_array {
    ($service:expr, $func:ident, $array:expr) => {{
        let mut array = $array;
        // Java needs initial values here (can't resize arrays)
        let mut repeated = Some(vec![Default::default(); array.len()]);
        let result = $service.$func(Some(&array[..]), &mut repeated);
        assert_eq!(repeated.as_ref(), Some(&array));
        array.reverse();
        assert_eq!(result, Ok(Some(array)));
    }};
}

#[test]
fn test_utf8_string() {
    let service = get_test_service();
    let inputs = [
        "typical string",
        "",
        "\0\0",
        std::str::from_utf8(&[0xF0, 0x90, 0x90, 0xB7, 0xE2, 0x82, 0xAC])
            .expect("error converting string"),
        ITestService::STRING_TEST_CONSTANT_UTF8,
    ];
    for input in &inputs {
        let result = service.RepeatUtf8CppString(input);
        assert_eq!(result.as_ref().map(String::as_str), Ok(*input));

        let result = service.RepeatNullableUtf8CppString(Some(input));
        assert_eq!(result.as_ref().map(Option::as_deref), Ok(Some(*input)));
    }

    let result = service.RepeatNullableUtf8CppString(None);
    assert_eq!(result, Ok(None));

    let inputs = vec![
        Some("typical string".into()),
        Some(String::new()),
        None,
        Some(
            std::str::from_utf8(&[0xF0, 0x90, 0x90, 0xB7, 0xE2, 0x82, 0xAC])
                .expect("error converting string")
                .into(),
        ),
        Some(ITestService::STRING_TEST_CONSTANT_UTF8.into()),
    ];

    // Java can't return a null list as a parameter
    let backend = service.getBackendType().expect("error getting backend type");
    let null_output = if backend == BackendType::JAVA { Some(vec![]) } else { None };
    test_reverse_null_array!(service, ReverseUtf8CppStringList, null_output);

    test_reverse_null_array!(service, ReverseNullableUtf8CppString, None);

    test_reverse_nullable_array!(service, ReverseUtf8CppStringList, inputs.clone());
    test_reverse_nullable_array!(service, ReverseNullableUtf8CppString, inputs);
}

#[allow(clippy::approx_constant)]
#[allow(clippy::float_cmp)]
#[test]
fn test_parcelable() {
    let service = get_test_service();
    let mut parcelable = StructuredParcelable::StructuredParcelable::default();

    const DESIRED_VALUE: i32 = 23;
    parcelable.f = DESIRED_VALUE;

    assert_eq!(parcelable.stringDefaultsToFoo, "foo");
    assert_eq!(parcelable.byteDefaultsToFour, 4);
    assert_eq!(parcelable.intDefaultsToFive, 5);
    assert_eq!(parcelable.longDefaultsToNegativeSeven, -7);
    assert_eq!(parcelable.booleanDefaultsToTrue, true);
    assert_eq!(parcelable.charDefaultsToC, 'C' as u16);
    assert_eq!(parcelable.floatDefaultsToPi, 3.14f32);
    assert_eq!(parcelable.doubleWithDefault, -3.14e17f64);
    assert_eq!(parcelable.boolDefault, false);
    assert_eq!(parcelable.byteDefault, 0);
    assert_eq!(parcelable.intDefault, 0);
    assert_eq!(parcelable.longDefault, 0);
    assert_eq!(parcelable.floatDefault, 0.0f32);
    assert_eq!(parcelable.doubleDefault, 0.0f64);
    assert_eq!(parcelable.arrayDefaultsTo123, &[1, 2, 3]);
    assert!(parcelable.arrayDefaultsToEmpty.is_empty());

    let result = service.FillOutStructuredParcelable(&mut parcelable);
    assert_eq!(result, Ok(()));

    assert_eq!(
        parcelable.shouldContainThreeFs,
        [DESIRED_VALUE, DESIRED_VALUE, DESIRED_VALUE]
    );
    assert_eq!(parcelable.shouldBeJerry, "Jerry");
    assert_eq!(parcelable.int32_min, i32::MIN);
    assert_eq!(parcelable.int32_max, i32::MAX);
    assert_eq!(parcelable.int64_max, i64::MAX);
    assert_eq!(parcelable.hexInt32_neg_1, -1);
    for i in parcelable.int32_1 {
        assert_eq!(i, 1);
    }
    for i in parcelable.int64_1 {
        assert_eq!(i, 1);
    }
    assert_eq!(parcelable.hexInt32_pos_1, 1);
    assert_eq!(parcelable.hexInt64_pos_1, 1);
    assert_eq!(parcelable.const_exprs_1.0, 1);
    assert_eq!(parcelable.const_exprs_2.0, 1);
    assert_eq!(parcelable.const_exprs_3.0, 1);
    assert_eq!(parcelable.const_exprs_4.0, 1);
    assert_eq!(parcelable.const_exprs_5.0, 1);
    assert_eq!(parcelable.const_exprs_6.0, 1);
    assert_eq!(parcelable.const_exprs_7.0, 1);
    assert_eq!(parcelable.const_exprs_8.0, 1);
    assert_eq!(parcelable.const_exprs_9.0, 1);
    assert_eq!(parcelable.const_exprs_10.0, 1);
    assert_eq!(parcelable.addString1, "hello world!");
    assert_eq!(
        parcelable.addString2,
        "The quick brown fox jumps over the lazy dog."
    );

    assert_eq!(parcelable.shouldSetBit0AndBit2, StructuredParcelable::BIT0 | StructuredParcelable::BIT2);

    assert_eq!(parcelable.u, Some(Union::Union::Ns(vec![1, 2, 3])));
    assert_eq!(parcelable.shouldBeConstS1, Some(Union::Union::S(Union::S1.to_string())))
}

#[test]
fn test_unions() {
    assert_eq!(Union::Union::default(), Union::Union::Ns(vec![]));
    assert_eq!(EnumUnion::default(), EnumUnion::IntEnum(IntEnum::FOO));
}

const EXPECTED_ARG_VALUE: i32 = 100;
const EXPECTED_RETURN_VALUE: i32 = 200;

struct TestDefaultImpl;

impl binder::Interface for TestDefaultImpl {}

impl ITestServiceDefault for TestDefaultImpl {
    fn UnimplementedMethod(&self, arg: i32) -> binder::Result<i32> {
        assert_eq!(arg, EXPECTED_ARG_VALUE);
        Ok(EXPECTED_RETURN_VALUE)
    }
}

#[test]
fn test_default_impl() {
    let service = get_test_service();
    let di: ITestServiceDefaultRef = Some(Arc::new(TestDefaultImpl));
    <BpTestService as ITestService::ITestService>::setDefaultImpl(di);

    let result = service.UnimplementedMethod(EXPECTED_ARG_VALUE);
    assert_eq!(result, Ok(EXPECTED_RETURN_VALUE));
}

#[test]
fn test_versioned_interface_version() {
    let service: binder::Strong<dyn IFooInterface::IFooInterface> =
        binder::get_interface(<BpFooInterface as IFooInterface::IFooInterface>::get_descriptor())
            .expect("did not get binder service");

    let version = service.getInterfaceVersion();
    assert_eq!(version, Ok(1));
}

#[test]
fn test_versioned_interface_hash() {
    let service: binder::Strong<dyn IFooInterface::IFooInterface> =
        binder::get_interface(<BpFooInterface as IFooInterface::IFooInterface>::get_descriptor())
            .expect("did not get binder service");

    let hash = service.getInterfaceHash();
    assert_eq!(
        hash.as_ref().map(String::as_str),
        Ok("9e7be1859820c59d9d55dd133e71a3687b5d2e5b")
    );
}

#[test]
fn test_versioned_known_union_field_is_ok() {
    let service: binder::Strong<dyn IFooInterface::IFooInterface> =
        binder::get_interface(<BpFooInterface as IFooInterface::IFooInterface>::get_descriptor())
            .expect("did not get binder service");

    assert_eq!(service.acceptUnionAndReturnString(&BazUnion::IntNum(42)), Ok(String::from("42")));
}

#[test]
fn test_versioned_unknown_union_field_triggers_error() {
    let service: binder::Strong<dyn IFooInterface::IFooInterface> =
        binder::get_interface(<BpFooInterface as IFooInterface::IFooInterface>::get_descriptor())
            .expect("did not get binder service");

    let ret = service.acceptUnionAndReturnString(&BazUnion::LongNum(42));
    assert!(!ret.is_ok());

    let main_service = get_test_service();
    let backend = main_service.getBackendType().expect("error getting backend type");

    // b/173458620 - for investigation of fixing difference
    if backend == BackendType::JAVA {
        assert_eq!(ret.unwrap_err().exception_code(), binder::ExceptionCode::ILLEGAL_ARGUMENT);
    } else {
        assert_eq!(ret.unwrap_err().transaction_error(), binder::StatusCode::BAD_VALUE);
    }
}

#[test]
fn test_array_of_parcelable_with_new_field() {
    let service: binder::Strong<dyn IFooInterface::IFooInterface> =
        binder::get_interface(<BpFooInterface as IFooInterface::IFooInterface>::get_descriptor())
            .expect("did not get binder service");

    let foos = [Default::default(), Default::default(), Default::default()];
    let ret = service.returnsLengthOfFooArray(&foos);
    assert_eq!(ret, Ok(foos.len() as i32));
}

#[test]
fn test_read_data_correctly_after_parcelable_with_new_field() {
    let service: binder::Strong<dyn IFooInterface::IFooInterface> =
        binder::get_interface(<BpFooInterface as IFooInterface::IFooInterface>::get_descriptor())
            .expect("did not get binder service");

    let in_foo = Default::default();
    let mut inout_foo = Default::default();
    let mut out_foo = Default::default();
    let ret = service.ignoreParcelablesAndRepeatInt(&in_foo, &mut inout_foo, &mut out_foo, 43);
    assert_eq!(ret, Ok(43));
}

fn test_renamed_interface<F>(f: F)
where
    F: FnOnce(binder::Strong<dyn IOldName::IOldName>, binder::Strong<dyn INewName::INewName>),
{
    let service = get_test_service();
    let old_name = service.GetOldNameInterface();
    assert!(old_name.is_ok());

    let new_name = service.GetNewNameInterface();
    assert!(new_name.is_ok());

    f(old_name.unwrap(), new_name.unwrap());
}

#[test]
fn test_renamed_interface_old_as_old() {
    test_renamed_interface(|old_name, _| {
        assert_eq!(
            <BpOldName as IOldName::IOldName>::get_descriptor(),
            "android.aidl.tests.IOldName"
        );

        let real_name = old_name.RealName();
        assert_eq!(real_name.as_ref().map(String::as_str), Ok("OldName"));
    });
}

#[test]
fn test_renamed_interface_new_as_new() {
    test_renamed_interface(|_, new_name| {
        assert_eq!(
            <BpNewName as INewName::INewName>::get_descriptor(),
            "android.aidl.tests.IOldName"
        );

        let real_name = new_name.RealName();
        assert_eq!(real_name.as_ref().map(String::as_str), Ok("NewName"));
    });
}

#[test]
fn test_renamed_interface_old_as_new() {
    test_renamed_interface(|old_name, _| {
        let new_name = old_name
            .as_binder()
            .into_interface::<dyn INewName::INewName>();
        assert!(new_name.is_ok());

        let real_name = new_name.unwrap().RealName();
        assert_eq!(real_name.as_ref().map(String::as_str), Ok("OldName"));
    });
}

#[test]
fn test_renamed_interface_new_as_old() {
    test_renamed_interface(|_, new_name| {
        let old_name = new_name
            .as_binder()
            .into_interface::<dyn IOldName::IOldName>();
        assert!(old_name.is_ok());

        let real_name = old_name.unwrap().RealName();
        assert_eq!(real_name.as_ref().map(String::as_str), Ok("NewName"));
    });
}
