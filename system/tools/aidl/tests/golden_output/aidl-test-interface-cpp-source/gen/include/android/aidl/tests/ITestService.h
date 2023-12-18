#pragma once

#include <android/aidl/tests/BackendType.h>
#include <android/aidl/tests/ByteEnum.h>
#include <android/aidl/tests/INamedCallback.h>
#include <android/aidl/tests/INewName.h>
#include <android/aidl/tests/IOldName.h>
#include <android/aidl/tests/IntEnum.h>
#include <android/aidl/tests/LongEnum.h>
#include <android/aidl/tests/StructuredParcelable.h>
#include <binder/IBinder.h>
#include <binder/IInterface.h>
#include <binder/ParcelFileDescriptor.h>
#include <binder/Status.h>
#include <cstdint>
#include <optional>
#include <string>
#include <utils/String16.h>
#include <utils/StrongPointer.h>
#include <vector>

namespace android {

namespace aidl {

namespace tests {

class ITestService : public ::android::IInterface {
public:
  DECLARE_META_INTERFACE(TestService)
  enum  : int8_t {
    BYTE_TEST_CONSTANT = 17,
  };
  enum  : int32_t {
    TEST_CONSTANT = 42,
    TEST_CONSTANT2 = -42,
    TEST_CONSTANT3 = 42,
    TEST_CONSTANT4 = 4,
    TEST_CONSTANT5 = -4,
    TEST_CONSTANT6 = 0,
    TEST_CONSTANT7 = 0,
    TEST_CONSTANT8 = 0,
    TEST_CONSTANT9 = 86,
    TEST_CONSTANT10 = 165,
    TEST_CONSTANT11 = 250,
    TEST_CONSTANT12 = -1,
    A1 = 1,
    A2 = 1,
    A3 = 1,
    A4 = 1,
    A5 = 1,
    A6 = 1,
    A7 = 1,
    A8 = 1,
    A9 = 1,
    A10 = 1,
    A11 = 1,
    A12 = 1,
    A13 = 1,
    A14 = 1,
    A15 = 1,
    A16 = 1,
    A17 = 1,
    A18 = 1,
    A19 = 1,
    A20 = 1,
    A21 = 1,
    A22 = 1,
    A23 = 1,
    A24 = 1,
    A25 = 1,
    A26 = 1,
    A27 = 1,
    A28 = 1,
    A29 = 1,
    A30 = 1,
    A31 = 1,
    A32 = 1,
    A33 = 1,
    A34 = 1,
    A35 = 1,
    A36 = 1,
    A37 = 1,
    A38 = 1,
    A39 = 1,
    A40 = 1,
    A41 = 1,
    A42 = 1,
    A43 = 1,
    A44 = 1,
    A45 = 1,
    A46 = 1,
    A47 = 1,
    A48 = 1,
    A49 = 1,
    A50 = 1,
    A51 = 1,
    A52 = 1,
    A53 = 1,
    A54 = 1,
    A55 = 1,
    A56 = 1,
    A57 = 1,
  };
  enum  : int64_t {
    LONG_TEST_CONSTANT = 1099511627776L,
  };
  static const ::android::String16& STRING_TEST_CONSTANT();
  static const ::android::String16& STRING_TEST_CONSTANT2();
  static const ::std::string& STRING_TEST_CONSTANT_UTF8();
  virtual ::android::binder::Status UnimplementedMethod(int32_t arg, int32_t* _aidl_return) = 0;
  virtual ::android::binder::Status Deprecated() __attribute__((deprecated("to make sure we have something in system/tools/aidl which does a compile check of deprecated and make sure this is reflected in goldens"))) = 0;
  virtual ::android::binder::Status TestOneway() = 0;
  virtual ::android::binder::Status RepeatBoolean(bool token, bool* _aidl_return) = 0;
  virtual ::android::binder::Status RepeatByte(int8_t token, int8_t* _aidl_return) = 0;
  virtual ::android::binder::Status RepeatChar(char16_t token, char16_t* _aidl_return) = 0;
  virtual ::android::binder::Status RepeatInt(int32_t token, int32_t* _aidl_return) = 0;
  virtual ::android::binder::Status RepeatLong(int64_t token, int64_t* _aidl_return) = 0;
  virtual ::android::binder::Status RepeatFloat(float token, float* _aidl_return) = 0;
  virtual ::android::binder::Status RepeatDouble(double token, double* _aidl_return) = 0;
  virtual ::android::binder::Status RepeatString(const ::android::String16& token, ::android::String16* _aidl_return) = 0;
  virtual ::android::binder::Status RepeatByteEnum(::android::aidl::tests::ByteEnum token, ::android::aidl::tests::ByteEnum* _aidl_return) = 0;
  virtual ::android::binder::Status RepeatIntEnum(::android::aidl::tests::IntEnum token, ::android::aidl::tests::IntEnum* _aidl_return) = 0;
  virtual ::android::binder::Status RepeatLongEnum(::android::aidl::tests::LongEnum token, ::android::aidl::tests::LongEnum* _aidl_return) = 0;
  virtual ::android::binder::Status ReverseBoolean(const ::std::vector<bool>& input, ::std::vector<bool>* repeated, ::std::vector<bool>* _aidl_return) = 0;
  virtual ::android::binder::Status ReverseByte(const ::std::vector<uint8_t>& input, ::std::vector<uint8_t>* repeated, ::std::vector<uint8_t>* _aidl_return) = 0;
  virtual ::android::binder::Status ReverseChar(const ::std::vector<char16_t>& input, ::std::vector<char16_t>* repeated, ::std::vector<char16_t>* _aidl_return) = 0;
  virtual ::android::binder::Status ReverseInt(const ::std::vector<int32_t>& input, ::std::vector<int32_t>* repeated, ::std::vector<int32_t>* _aidl_return) = 0;
  virtual ::android::binder::Status ReverseLong(const ::std::vector<int64_t>& input, ::std::vector<int64_t>* repeated, ::std::vector<int64_t>* _aidl_return) = 0;
  virtual ::android::binder::Status ReverseFloat(const ::std::vector<float>& input, ::std::vector<float>* repeated, ::std::vector<float>* _aidl_return) = 0;
  virtual ::android::binder::Status ReverseDouble(const ::std::vector<double>& input, ::std::vector<double>* repeated, ::std::vector<double>* _aidl_return) = 0;
  virtual ::android::binder::Status ReverseString(const ::std::vector<::android::String16>& input, ::std::vector<::android::String16>* repeated, ::std::vector<::android::String16>* _aidl_return) = 0;
  virtual ::android::binder::Status ReverseByteEnum(const ::std::vector<::android::aidl::tests::ByteEnum>& input, ::std::vector<::android::aidl::tests::ByteEnum>* repeated, ::std::vector<::android::aidl::tests::ByteEnum>* _aidl_return) = 0;
  virtual ::android::binder::Status ReverseIntEnum(const ::std::vector<::android::aidl::tests::IntEnum>& input, ::std::vector<::android::aidl::tests::IntEnum>* repeated, ::std::vector<::android::aidl::tests::IntEnum>* _aidl_return) = 0;
  virtual ::android::binder::Status ReverseLongEnum(const ::std::vector<::android::aidl::tests::LongEnum>& input, ::std::vector<::android::aidl::tests::LongEnum>* repeated, ::std::vector<::android::aidl::tests::LongEnum>* _aidl_return) = 0;
  virtual ::android::binder::Status GetOtherTestService(const ::android::String16& name, ::android::sp<::android::aidl::tests::INamedCallback>* _aidl_return) = 0;
  virtual ::android::binder::Status VerifyName(const ::android::sp<::android::aidl::tests::INamedCallback>& service, const ::android::String16& name, bool* _aidl_return) = 0;
  virtual ::android::binder::Status ReverseStringList(const ::std::vector<::android::String16>& input, ::std::vector<::android::String16>* repeated, ::std::vector<::android::String16>* _aidl_return) = 0;
  virtual ::android::binder::Status RepeatParcelFileDescriptor(const ::android::os::ParcelFileDescriptor& read, ::android::os::ParcelFileDescriptor* _aidl_return) = 0;
  virtual ::android::binder::Status ReverseParcelFileDescriptorArray(const ::std::vector<::android::os::ParcelFileDescriptor>& input, ::std::vector<::android::os::ParcelFileDescriptor>* repeated, ::std::vector<::android::os::ParcelFileDescriptor>* _aidl_return) = 0;
  virtual ::android::binder::Status ThrowServiceException(int32_t code) = 0;
  virtual ::android::binder::Status RepeatNullableIntArray(const ::std::optional<::std::vector<int32_t>>& input, ::std::optional<::std::vector<int32_t>>* _aidl_return) = 0;
  virtual ::android::binder::Status RepeatNullableByteEnumArray(const ::std::optional<::std::vector<::android::aidl::tests::ByteEnum>>& input, ::std::optional<::std::vector<::android::aidl::tests::ByteEnum>>* _aidl_return) = 0;
  virtual ::android::binder::Status RepeatNullableIntEnumArray(const ::std::optional<::std::vector<::android::aidl::tests::IntEnum>>& input, ::std::optional<::std::vector<::android::aidl::tests::IntEnum>>* _aidl_return) = 0;
  virtual ::android::binder::Status RepeatNullableLongEnumArray(const ::std::optional<::std::vector<::android::aidl::tests::LongEnum>>& input, ::std::optional<::std::vector<::android::aidl::tests::LongEnum>>* _aidl_return) = 0;
  virtual ::android::binder::Status RepeatNullableString(const ::std::optional<::android::String16>& input, ::std::optional<::android::String16>* _aidl_return) = 0;
  virtual ::android::binder::Status RepeatNullableStringList(const ::std::optional<::std::vector<::std::optional<::android::String16>>>& input, ::std::optional<::std::vector<::std::optional<::android::String16>>>* _aidl_return) = 0;
  virtual ::android::binder::Status RepeatNullableParcelable(const ::std::optional<::android::aidl::tests::StructuredParcelable>& input, ::std::optional<::android::aidl::tests::StructuredParcelable>* _aidl_return) = 0;
  virtual ::android::binder::Status TakesAnIBinder(const ::android::sp<::android::IBinder>& input) = 0;
  virtual ::android::binder::Status TakesANullableIBinder(const ::android::sp<::android::IBinder>& input) = 0;
  virtual ::android::binder::Status RepeatUtf8CppString(const ::std::string& token, ::std::string* _aidl_return) = 0;
  virtual ::android::binder::Status RepeatNullableUtf8CppString(const ::std::optional<::std::string>& token, ::std::optional<::std::string>* _aidl_return) = 0;
  virtual ::android::binder::Status ReverseUtf8CppString(const ::std::vector<::std::string>& input, ::std::vector<::std::string>* repeated, ::std::vector<::std::string>* _aidl_return) = 0;
  virtual ::android::binder::Status ReverseNullableUtf8CppString(const ::std::optional<::std::vector<::std::optional<::std::string>>>& input, ::std::optional<::std::vector<::std::optional<::std::string>>>* repeated, ::std::optional<::std::vector<::std::optional<::std::string>>>* _aidl_return) = 0;
  virtual ::android::binder::Status ReverseUtf8CppStringList(const ::std::optional<::std::vector<::std::optional<::std::string>>>& input, ::std::optional<::std::vector<::std::optional<::std::string>>>* repeated, ::std::optional<::std::vector<::std::optional<::std::string>>>* _aidl_return) = 0;
  virtual ::android::binder::Status GetCallback(bool return_null, ::android::sp<::android::aidl::tests::INamedCallback>* _aidl_return) = 0;
  virtual ::android::binder::Status FillOutStructuredParcelable(::android::aidl::tests::StructuredParcelable* parcel) = 0;
  virtual ::android::binder::Status GetOldNameInterface(::android::sp<::android::aidl::tests::IOldName>* _aidl_return) = 0;
  virtual ::android::binder::Status GetNewNameInterface(::android::sp<::android::aidl::tests::INewName>* _aidl_return) = 0;
  virtual ::android::binder::Status GetCppJavaTests(::android::sp<::android::IBinder>* _aidl_return) = 0;
  virtual ::android::binder::Status getBackendType(::android::aidl::tests::BackendType* _aidl_return) = 0;
};  // class ITestService

class ITestServiceDefault : public ITestService {
public:
  ::android::IBinder* onAsBinder() override {
    return nullptr;
  }
  ::android::binder::Status UnimplementedMethod(int32_t, int32_t*) override {
    return ::android::binder::Status::fromStatusT(::android::UNKNOWN_TRANSACTION);
  }
  ::android::binder::Status Deprecated() override __attribute__((deprecated("to make sure we have something in system/tools/aidl which does a compile check of deprecated and make sure this is reflected in goldens"))) {
    return ::android::binder::Status::fromStatusT(::android::UNKNOWN_TRANSACTION);
  }
  ::android::binder::Status TestOneway() override {
    return ::android::binder::Status::fromStatusT(::android::UNKNOWN_TRANSACTION);
  }
  ::android::binder::Status RepeatBoolean(bool, bool*) override {
    return ::android::binder::Status::fromStatusT(::android::UNKNOWN_TRANSACTION);
  }
  ::android::binder::Status RepeatByte(int8_t, int8_t*) override {
    return ::android::binder::Status::fromStatusT(::android::UNKNOWN_TRANSACTION);
  }
  ::android::binder::Status RepeatChar(char16_t, char16_t*) override {
    return ::android::binder::Status::fromStatusT(::android::UNKNOWN_TRANSACTION);
  }
  ::android::binder::Status RepeatInt(int32_t, int32_t*) override {
    return ::android::binder::Status::fromStatusT(::android::UNKNOWN_TRANSACTION);
  }
  ::android::binder::Status RepeatLong(int64_t, int64_t*) override {
    return ::android::binder::Status::fromStatusT(::android::UNKNOWN_TRANSACTION);
  }
  ::android::binder::Status RepeatFloat(float, float*) override {
    return ::android::binder::Status::fromStatusT(::android::UNKNOWN_TRANSACTION);
  }
  ::android::binder::Status RepeatDouble(double, double*) override {
    return ::android::binder::Status::fromStatusT(::android::UNKNOWN_TRANSACTION);
  }
  ::android::binder::Status RepeatString(const ::android::String16&, ::android::String16*) override {
    return ::android::binder::Status::fromStatusT(::android::UNKNOWN_TRANSACTION);
  }
  ::android::binder::Status RepeatByteEnum(::android::aidl::tests::ByteEnum, ::android::aidl::tests::ByteEnum*) override {
    return ::android::binder::Status::fromStatusT(::android::UNKNOWN_TRANSACTION);
  }
  ::android::binder::Status RepeatIntEnum(::android::aidl::tests::IntEnum, ::android::aidl::tests::IntEnum*) override {
    return ::android::binder::Status::fromStatusT(::android::UNKNOWN_TRANSACTION);
  }
  ::android::binder::Status RepeatLongEnum(::android::aidl::tests::LongEnum, ::android::aidl::tests::LongEnum*) override {
    return ::android::binder::Status::fromStatusT(::android::UNKNOWN_TRANSACTION);
  }
  ::android::binder::Status ReverseBoolean(const ::std::vector<bool>&, ::std::vector<bool>*, ::std::vector<bool>*) override {
    return ::android::binder::Status::fromStatusT(::android::UNKNOWN_TRANSACTION);
  }
  ::android::binder::Status ReverseByte(const ::std::vector<uint8_t>&, ::std::vector<uint8_t>*, ::std::vector<uint8_t>*) override {
    return ::android::binder::Status::fromStatusT(::android::UNKNOWN_TRANSACTION);
  }
  ::android::binder::Status ReverseChar(const ::std::vector<char16_t>&, ::std::vector<char16_t>*, ::std::vector<char16_t>*) override {
    return ::android::binder::Status::fromStatusT(::android::UNKNOWN_TRANSACTION);
  }
  ::android::binder::Status ReverseInt(const ::std::vector<int32_t>&, ::std::vector<int32_t>*, ::std::vector<int32_t>*) override {
    return ::android::binder::Status::fromStatusT(::android::UNKNOWN_TRANSACTION);
  }
  ::android::binder::Status ReverseLong(const ::std::vector<int64_t>&, ::std::vector<int64_t>*, ::std::vector<int64_t>*) override {
    return ::android::binder::Status::fromStatusT(::android::UNKNOWN_TRANSACTION);
  }
  ::android::binder::Status ReverseFloat(const ::std::vector<float>&, ::std::vector<float>*, ::std::vector<float>*) override {
    return ::android::binder::Status::fromStatusT(::android::UNKNOWN_TRANSACTION);
  }
  ::android::binder::Status ReverseDouble(const ::std::vector<double>&, ::std::vector<double>*, ::std::vector<double>*) override {
    return ::android::binder::Status::fromStatusT(::android::UNKNOWN_TRANSACTION);
  }
  ::android::binder::Status ReverseString(const ::std::vector<::android::String16>&, ::std::vector<::android::String16>*, ::std::vector<::android::String16>*) override {
    return ::android::binder::Status::fromStatusT(::android::UNKNOWN_TRANSACTION);
  }
  ::android::binder::Status ReverseByteEnum(const ::std::vector<::android::aidl::tests::ByteEnum>&, ::std::vector<::android::aidl::tests::ByteEnum>*, ::std::vector<::android::aidl::tests::ByteEnum>*) override {
    return ::android::binder::Status::fromStatusT(::android::UNKNOWN_TRANSACTION);
  }
  ::android::binder::Status ReverseIntEnum(const ::std::vector<::android::aidl::tests::IntEnum>&, ::std::vector<::android::aidl::tests::IntEnum>*, ::std::vector<::android::aidl::tests::IntEnum>*) override {
    return ::android::binder::Status::fromStatusT(::android::UNKNOWN_TRANSACTION);
  }
  ::android::binder::Status ReverseLongEnum(const ::std::vector<::android::aidl::tests::LongEnum>&, ::std::vector<::android::aidl::tests::LongEnum>*, ::std::vector<::android::aidl::tests::LongEnum>*) override {
    return ::android::binder::Status::fromStatusT(::android::UNKNOWN_TRANSACTION);
  }
  ::android::binder::Status GetOtherTestService(const ::android::String16&, ::android::sp<::android::aidl::tests::INamedCallback>*) override {
    return ::android::binder::Status::fromStatusT(::android::UNKNOWN_TRANSACTION);
  }
  ::android::binder::Status VerifyName(const ::android::sp<::android::aidl::tests::INamedCallback>&, const ::android::String16&, bool*) override {
    return ::android::binder::Status::fromStatusT(::android::UNKNOWN_TRANSACTION);
  }
  ::android::binder::Status ReverseStringList(const ::std::vector<::android::String16>&, ::std::vector<::android::String16>*, ::std::vector<::android::String16>*) override {
    return ::android::binder::Status::fromStatusT(::android::UNKNOWN_TRANSACTION);
  }
  ::android::binder::Status RepeatParcelFileDescriptor(const ::android::os::ParcelFileDescriptor&, ::android::os::ParcelFileDescriptor*) override {
    return ::android::binder::Status::fromStatusT(::android::UNKNOWN_TRANSACTION);
  }
  ::android::binder::Status ReverseParcelFileDescriptorArray(const ::std::vector<::android::os::ParcelFileDescriptor>&, ::std::vector<::android::os::ParcelFileDescriptor>*, ::std::vector<::android::os::ParcelFileDescriptor>*) override {
    return ::android::binder::Status::fromStatusT(::android::UNKNOWN_TRANSACTION);
  }
  ::android::binder::Status ThrowServiceException(int32_t) override {
    return ::android::binder::Status::fromStatusT(::android::UNKNOWN_TRANSACTION);
  }
  ::android::binder::Status RepeatNullableIntArray(const ::std::optional<::std::vector<int32_t>>&, ::std::optional<::std::vector<int32_t>>*) override {
    return ::android::binder::Status::fromStatusT(::android::UNKNOWN_TRANSACTION);
  }
  ::android::binder::Status RepeatNullableByteEnumArray(const ::std::optional<::std::vector<::android::aidl::tests::ByteEnum>>&, ::std::optional<::std::vector<::android::aidl::tests::ByteEnum>>*) override {
    return ::android::binder::Status::fromStatusT(::android::UNKNOWN_TRANSACTION);
  }
  ::android::binder::Status RepeatNullableIntEnumArray(const ::std::optional<::std::vector<::android::aidl::tests::IntEnum>>&, ::std::optional<::std::vector<::android::aidl::tests::IntEnum>>*) override {
    return ::android::binder::Status::fromStatusT(::android::UNKNOWN_TRANSACTION);
  }
  ::android::binder::Status RepeatNullableLongEnumArray(const ::std::optional<::std::vector<::android::aidl::tests::LongEnum>>&, ::std::optional<::std::vector<::android::aidl::tests::LongEnum>>*) override {
    return ::android::binder::Status::fromStatusT(::android::UNKNOWN_TRANSACTION);
  }
  ::android::binder::Status RepeatNullableString(const ::std::optional<::android::String16>&, ::std::optional<::android::String16>*) override {
    return ::android::binder::Status::fromStatusT(::android::UNKNOWN_TRANSACTION);
  }
  ::android::binder::Status RepeatNullableStringList(const ::std::optional<::std::vector<::std::optional<::android::String16>>>&, ::std::optional<::std::vector<::std::optional<::android::String16>>>*) override {
    return ::android::binder::Status::fromStatusT(::android::UNKNOWN_TRANSACTION);
  }
  ::android::binder::Status RepeatNullableParcelable(const ::std::optional<::android::aidl::tests::StructuredParcelable>&, ::std::optional<::android::aidl::tests::StructuredParcelable>*) override {
    return ::android::binder::Status::fromStatusT(::android::UNKNOWN_TRANSACTION);
  }
  ::android::binder::Status TakesAnIBinder(const ::android::sp<::android::IBinder>&) override {
    return ::android::binder::Status::fromStatusT(::android::UNKNOWN_TRANSACTION);
  }
  ::android::binder::Status TakesANullableIBinder(const ::android::sp<::android::IBinder>&) override {
    return ::android::binder::Status::fromStatusT(::android::UNKNOWN_TRANSACTION);
  }
  ::android::binder::Status RepeatUtf8CppString(const ::std::string&, ::std::string*) override {
    return ::android::binder::Status::fromStatusT(::android::UNKNOWN_TRANSACTION);
  }
  ::android::binder::Status RepeatNullableUtf8CppString(const ::std::optional<::std::string>&, ::std::optional<::std::string>*) override {
    return ::android::binder::Status::fromStatusT(::android::UNKNOWN_TRANSACTION);
  }
  ::android::binder::Status ReverseUtf8CppString(const ::std::vector<::std::string>&, ::std::vector<::std::string>*, ::std::vector<::std::string>*) override {
    return ::android::binder::Status::fromStatusT(::android::UNKNOWN_TRANSACTION);
  }
  ::android::binder::Status ReverseNullableUtf8CppString(const ::std::optional<::std::vector<::std::optional<::std::string>>>&, ::std::optional<::std::vector<::std::optional<::std::string>>>*, ::std::optional<::std::vector<::std::optional<::std::string>>>*) override {
    return ::android::binder::Status::fromStatusT(::android::UNKNOWN_TRANSACTION);
  }
  ::android::binder::Status ReverseUtf8CppStringList(const ::std::optional<::std::vector<::std::optional<::std::string>>>&, ::std::optional<::std::vector<::std::optional<::std::string>>>*, ::std::optional<::std::vector<::std::optional<::std::string>>>*) override {
    return ::android::binder::Status::fromStatusT(::android::UNKNOWN_TRANSACTION);
  }
  ::android::binder::Status GetCallback(bool, ::android::sp<::android::aidl::tests::INamedCallback>*) override {
    return ::android::binder::Status::fromStatusT(::android::UNKNOWN_TRANSACTION);
  }
  ::android::binder::Status FillOutStructuredParcelable(::android::aidl::tests::StructuredParcelable*) override {
    return ::android::binder::Status::fromStatusT(::android::UNKNOWN_TRANSACTION);
  }
  ::android::binder::Status GetOldNameInterface(::android::sp<::android::aidl::tests::IOldName>*) override {
    return ::android::binder::Status::fromStatusT(::android::UNKNOWN_TRANSACTION);
  }
  ::android::binder::Status GetNewNameInterface(::android::sp<::android::aidl::tests::INewName>*) override {
    return ::android::binder::Status::fromStatusT(::android::UNKNOWN_TRANSACTION);
  }
  ::android::binder::Status GetCppJavaTests(::android::sp<::android::IBinder>*) override {
    return ::android::binder::Status::fromStatusT(::android::UNKNOWN_TRANSACTION);
  }
  ::android::binder::Status getBackendType(::android::aidl::tests::BackendType*) override {
    return ::android::binder::Status::fromStatusT(::android::UNKNOWN_TRANSACTION);
  }
};  // class ITestServiceDefault

}  // namespace tests

}  // namespace aidl

}  // namespace android
