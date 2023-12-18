#pragma once

#include <android/binder_interface_utils.h>

#include <cstdint>
#include <memory>
#include <optional>
#include <string>
#include <vector>
#ifdef BINDER_STABILITY_SUPPORT
#include <android/binder_stability.h>
#endif  // BINDER_STABILITY_SUPPORT
#include <android/binder_parcel_platform.h>
#include <android/binder_ibinder_platform.h>
#include <aidl/android/aidl/tests/BackendType.h>
#include <aidl/android/aidl/tests/ByteEnum.h>
#include <aidl/android/aidl/tests/INamedCallback.h>
#include <aidl/android/aidl/tests/INewName.h>
#include <aidl/android/aidl/tests/IOldName.h>
#include <aidl/android/aidl/tests/IntEnum.h>
#include <aidl/android/aidl/tests/LongEnum.h>
#include <aidl/android/aidl/tests/StructuredParcelable.h>

namespace aidl {
namespace android {
namespace aidl {
namespace tests {
class ITestService : public ::ndk::ICInterface {
public:
  static const char* descriptor;
  ITestService();
  virtual ~ITestService();

  enum : int32_t { TEST_CONSTANT = 42 };
  enum : int32_t { TEST_CONSTANT2 = -42 };
  enum : int32_t { TEST_CONSTANT3 = 42 };
  enum : int32_t { TEST_CONSTANT4 = 4 };
  enum : int32_t { TEST_CONSTANT5 = -4 };
  enum : int32_t { TEST_CONSTANT6 = 0 };
  enum : int32_t { TEST_CONSTANT7 = 0 };
  enum : int32_t { TEST_CONSTANT8 = 0 };
  enum : int32_t { TEST_CONSTANT9 = 86 };
  enum : int32_t { TEST_CONSTANT10 = 165 };
  enum : int32_t { TEST_CONSTANT11 = 250 };
  enum : int32_t { TEST_CONSTANT12 = -1 };
  enum : int8_t { BYTE_TEST_CONSTANT = 17 };
  enum : int64_t { LONG_TEST_CONSTANT = 1099511627776L };
  static const char* STRING_TEST_CONSTANT;
  static const char* STRING_TEST_CONSTANT2;
  static const char* STRING_TEST_CONSTANT_UTF8;
  enum : int32_t { A1 = 1 };
  enum : int32_t { A2 = 1 };
  enum : int32_t { A3 = 1 };
  enum : int32_t { A4 = 1 };
  enum : int32_t { A5 = 1 };
  enum : int32_t { A6 = 1 };
  enum : int32_t { A7 = 1 };
  enum : int32_t { A8 = 1 };
  enum : int32_t { A9 = 1 };
  enum : int32_t { A10 = 1 };
  enum : int32_t { A11 = 1 };
  enum : int32_t { A12 = 1 };
  enum : int32_t { A13 = 1 };
  enum : int32_t { A14 = 1 };
  enum : int32_t { A15 = 1 };
  enum : int32_t { A16 = 1 };
  enum : int32_t { A17 = 1 };
  enum : int32_t { A18 = 1 };
  enum : int32_t { A19 = 1 };
  enum : int32_t { A20 = 1 };
  enum : int32_t { A21 = 1 };
  enum : int32_t { A22 = 1 };
  enum : int32_t { A23 = 1 };
  enum : int32_t { A24 = 1 };
  enum : int32_t { A25 = 1 };
  enum : int32_t { A26 = 1 };
  enum : int32_t { A27 = 1 };
  enum : int32_t { A28 = 1 };
  enum : int32_t { A29 = 1 };
  enum : int32_t { A30 = 1 };
  enum : int32_t { A31 = 1 };
  enum : int32_t { A32 = 1 };
  enum : int32_t { A33 = 1 };
  enum : int32_t { A34 = 1 };
  enum : int32_t { A35 = 1 };
  enum : int32_t { A36 = 1 };
  enum : int32_t { A37 = 1 };
  enum : int32_t { A38 = 1 };
  enum : int32_t { A39 = 1 };
  enum : int32_t { A40 = 1 };
  enum : int32_t { A41 = 1 };
  enum : int32_t { A42 = 1 };
  enum : int32_t { A43 = 1 };
  enum : int32_t { A44 = 1 };
  enum : int32_t { A45 = 1 };
  enum : int32_t { A46 = 1 };
  enum : int32_t { A47 = 1 };
  enum : int32_t { A48 = 1 };
  enum : int32_t { A49 = 1 };
  enum : int32_t { A50 = 1 };
  enum : int32_t { A51 = 1 };
  enum : int32_t { A52 = 1 };
  enum : int32_t { A53 = 1 };
  enum : int32_t { A54 = 1 };
  enum : int32_t { A55 = 1 };
  enum : int32_t { A56 = 1 };
  enum : int32_t { A57 = 1 };
  static constexpr uint32_t TRANSACTION_UnimplementedMethod = FIRST_CALL_TRANSACTION + 0;
  static constexpr uint32_t TRANSACTION_Deprecated = FIRST_CALL_TRANSACTION + 1;
  static constexpr uint32_t TRANSACTION_TestOneway = FIRST_CALL_TRANSACTION + 2;
  static constexpr uint32_t TRANSACTION_RepeatBoolean = FIRST_CALL_TRANSACTION + 3;
  static constexpr uint32_t TRANSACTION_RepeatByte = FIRST_CALL_TRANSACTION + 4;
  static constexpr uint32_t TRANSACTION_RepeatChar = FIRST_CALL_TRANSACTION + 5;
  static constexpr uint32_t TRANSACTION_RepeatInt = FIRST_CALL_TRANSACTION + 6;
  static constexpr uint32_t TRANSACTION_RepeatLong = FIRST_CALL_TRANSACTION + 7;
  static constexpr uint32_t TRANSACTION_RepeatFloat = FIRST_CALL_TRANSACTION + 8;
  static constexpr uint32_t TRANSACTION_RepeatDouble = FIRST_CALL_TRANSACTION + 9;
  static constexpr uint32_t TRANSACTION_RepeatString = FIRST_CALL_TRANSACTION + 10;
  static constexpr uint32_t TRANSACTION_RepeatByteEnum = FIRST_CALL_TRANSACTION + 11;
  static constexpr uint32_t TRANSACTION_RepeatIntEnum = FIRST_CALL_TRANSACTION + 12;
  static constexpr uint32_t TRANSACTION_RepeatLongEnum = FIRST_CALL_TRANSACTION + 13;
  static constexpr uint32_t TRANSACTION_ReverseBoolean = FIRST_CALL_TRANSACTION + 14;
  static constexpr uint32_t TRANSACTION_ReverseByte = FIRST_CALL_TRANSACTION + 15;
  static constexpr uint32_t TRANSACTION_ReverseChar = FIRST_CALL_TRANSACTION + 16;
  static constexpr uint32_t TRANSACTION_ReverseInt = FIRST_CALL_TRANSACTION + 17;
  static constexpr uint32_t TRANSACTION_ReverseLong = FIRST_CALL_TRANSACTION + 18;
  static constexpr uint32_t TRANSACTION_ReverseFloat = FIRST_CALL_TRANSACTION + 19;
  static constexpr uint32_t TRANSACTION_ReverseDouble = FIRST_CALL_TRANSACTION + 20;
  static constexpr uint32_t TRANSACTION_ReverseString = FIRST_CALL_TRANSACTION + 21;
  static constexpr uint32_t TRANSACTION_ReverseByteEnum = FIRST_CALL_TRANSACTION + 22;
  static constexpr uint32_t TRANSACTION_ReverseIntEnum = FIRST_CALL_TRANSACTION + 23;
  static constexpr uint32_t TRANSACTION_ReverseLongEnum = FIRST_CALL_TRANSACTION + 24;
  static constexpr uint32_t TRANSACTION_GetOtherTestService = FIRST_CALL_TRANSACTION + 25;
  static constexpr uint32_t TRANSACTION_VerifyName = FIRST_CALL_TRANSACTION + 26;
  static constexpr uint32_t TRANSACTION_ReverseStringList = FIRST_CALL_TRANSACTION + 27;
  static constexpr uint32_t TRANSACTION_RepeatParcelFileDescriptor = FIRST_CALL_TRANSACTION + 28;
  static constexpr uint32_t TRANSACTION_ReverseParcelFileDescriptorArray = FIRST_CALL_TRANSACTION + 29;
  static constexpr uint32_t TRANSACTION_ThrowServiceException = FIRST_CALL_TRANSACTION + 30;
  static constexpr uint32_t TRANSACTION_RepeatNullableIntArray = FIRST_CALL_TRANSACTION + 31;
  static constexpr uint32_t TRANSACTION_RepeatNullableByteEnumArray = FIRST_CALL_TRANSACTION + 32;
  static constexpr uint32_t TRANSACTION_RepeatNullableIntEnumArray = FIRST_CALL_TRANSACTION + 33;
  static constexpr uint32_t TRANSACTION_RepeatNullableLongEnumArray = FIRST_CALL_TRANSACTION + 34;
  static constexpr uint32_t TRANSACTION_RepeatNullableString = FIRST_CALL_TRANSACTION + 35;
  static constexpr uint32_t TRANSACTION_RepeatNullableStringList = FIRST_CALL_TRANSACTION + 36;
  static constexpr uint32_t TRANSACTION_RepeatNullableParcelable = FIRST_CALL_TRANSACTION + 37;
  static constexpr uint32_t TRANSACTION_TakesAnIBinder = FIRST_CALL_TRANSACTION + 38;
  static constexpr uint32_t TRANSACTION_TakesANullableIBinder = FIRST_CALL_TRANSACTION + 39;
  static constexpr uint32_t TRANSACTION_RepeatUtf8CppString = FIRST_CALL_TRANSACTION + 40;
  static constexpr uint32_t TRANSACTION_RepeatNullableUtf8CppString = FIRST_CALL_TRANSACTION + 41;
  static constexpr uint32_t TRANSACTION_ReverseUtf8CppString = FIRST_CALL_TRANSACTION + 42;
  static constexpr uint32_t TRANSACTION_ReverseNullableUtf8CppString = FIRST_CALL_TRANSACTION + 43;
  static constexpr uint32_t TRANSACTION_ReverseUtf8CppStringList = FIRST_CALL_TRANSACTION + 44;
  static constexpr uint32_t TRANSACTION_GetCallback = FIRST_CALL_TRANSACTION + 45;
  static constexpr uint32_t TRANSACTION_FillOutStructuredParcelable = FIRST_CALL_TRANSACTION + 46;
  static constexpr uint32_t TRANSACTION_GetOldNameInterface = FIRST_CALL_TRANSACTION + 47;
  static constexpr uint32_t TRANSACTION_GetNewNameInterface = FIRST_CALL_TRANSACTION + 48;
  static constexpr uint32_t TRANSACTION_GetCppJavaTests = FIRST_CALL_TRANSACTION + 49;
  static constexpr uint32_t TRANSACTION_getBackendType = FIRST_CALL_TRANSACTION + 50;

  static std::shared_ptr<ITestService> fromBinder(const ::ndk::SpAIBinder& binder);
  static binder_status_t writeToParcel(AParcel* parcel, const std::shared_ptr<ITestService>& instance);
  static binder_status_t readFromParcel(const AParcel* parcel, std::shared_ptr<ITestService>* instance);
  static bool setDefaultImpl(const std::shared_ptr<ITestService>& impl);
  static const std::shared_ptr<ITestService>& getDefaultImpl();
  virtual ::ndk::ScopedAStatus UnimplementedMethod(int32_t in_arg, int32_t* _aidl_return) = 0;
  virtual ::ndk::ScopedAStatus Deprecated() __attribute__((deprecated("to make sure we have something in system/tools/aidl which does a compile check of deprecated and make sure this is reflected in goldens"))) = 0;
  virtual ::ndk::ScopedAStatus TestOneway() = 0;
  virtual ::ndk::ScopedAStatus RepeatBoolean(bool in_token, bool* _aidl_return) = 0;
  virtual ::ndk::ScopedAStatus RepeatByte(int8_t in_token, int8_t* _aidl_return) = 0;
  virtual ::ndk::ScopedAStatus RepeatChar(char16_t in_token, char16_t* _aidl_return) = 0;
  virtual ::ndk::ScopedAStatus RepeatInt(int32_t in_token, int32_t* _aidl_return) = 0;
  virtual ::ndk::ScopedAStatus RepeatLong(int64_t in_token, int64_t* _aidl_return) = 0;
  virtual ::ndk::ScopedAStatus RepeatFloat(float in_token, float* _aidl_return) = 0;
  virtual ::ndk::ScopedAStatus RepeatDouble(double in_token, double* _aidl_return) = 0;
  virtual ::ndk::ScopedAStatus RepeatString(const std::string& in_token, std::string* _aidl_return) = 0;
  virtual ::ndk::ScopedAStatus RepeatByteEnum(::aidl::android::aidl::tests::ByteEnum in_token, ::aidl::android::aidl::tests::ByteEnum* _aidl_return) = 0;
  virtual ::ndk::ScopedAStatus RepeatIntEnum(::aidl::android::aidl::tests::IntEnum in_token, ::aidl::android::aidl::tests::IntEnum* _aidl_return) = 0;
  virtual ::ndk::ScopedAStatus RepeatLongEnum(::aidl::android::aidl::tests::LongEnum in_token, ::aidl::android::aidl::tests::LongEnum* _aidl_return) = 0;
  virtual ::ndk::ScopedAStatus ReverseBoolean(const std::vector<bool>& in_input, std::vector<bool>* out_repeated, std::vector<bool>* _aidl_return) = 0;
  virtual ::ndk::ScopedAStatus ReverseByte(const std::vector<uint8_t>& in_input, std::vector<uint8_t>* out_repeated, std::vector<uint8_t>* _aidl_return) = 0;
  virtual ::ndk::ScopedAStatus ReverseChar(const std::vector<char16_t>& in_input, std::vector<char16_t>* out_repeated, std::vector<char16_t>* _aidl_return) = 0;
  virtual ::ndk::ScopedAStatus ReverseInt(const std::vector<int32_t>& in_input, std::vector<int32_t>* out_repeated, std::vector<int32_t>* _aidl_return) = 0;
  virtual ::ndk::ScopedAStatus ReverseLong(const std::vector<int64_t>& in_input, std::vector<int64_t>* out_repeated, std::vector<int64_t>* _aidl_return) = 0;
  virtual ::ndk::ScopedAStatus ReverseFloat(const std::vector<float>& in_input, std::vector<float>* out_repeated, std::vector<float>* _aidl_return) = 0;
  virtual ::ndk::ScopedAStatus ReverseDouble(const std::vector<double>& in_input, std::vector<double>* out_repeated, std::vector<double>* _aidl_return) = 0;
  virtual ::ndk::ScopedAStatus ReverseString(const std::vector<std::string>& in_input, std::vector<std::string>* out_repeated, std::vector<std::string>* _aidl_return) = 0;
  virtual ::ndk::ScopedAStatus ReverseByteEnum(const std::vector<::aidl::android::aidl::tests::ByteEnum>& in_input, std::vector<::aidl::android::aidl::tests::ByteEnum>* out_repeated, std::vector<::aidl::android::aidl::tests::ByteEnum>* _aidl_return) = 0;
  virtual ::ndk::ScopedAStatus ReverseIntEnum(const std::vector<::aidl::android::aidl::tests::IntEnum>& in_input, std::vector<::aidl::android::aidl::tests::IntEnum>* out_repeated, std::vector<::aidl::android::aidl::tests::IntEnum>* _aidl_return) = 0;
  virtual ::ndk::ScopedAStatus ReverseLongEnum(const std::vector<::aidl::android::aidl::tests::LongEnum>& in_input, std::vector<::aidl::android::aidl::tests::LongEnum>* out_repeated, std::vector<::aidl::android::aidl::tests::LongEnum>* _aidl_return) = 0;
  virtual ::ndk::ScopedAStatus GetOtherTestService(const std::string& in_name, std::shared_ptr<::aidl::android::aidl::tests::INamedCallback>* _aidl_return) = 0;
  virtual ::ndk::ScopedAStatus VerifyName(const std::shared_ptr<::aidl::android::aidl::tests::INamedCallback>& in_service, const std::string& in_name, bool* _aidl_return) = 0;
  virtual ::ndk::ScopedAStatus ReverseStringList(const std::vector<std::string>& in_input, std::vector<std::string>* out_repeated, std::vector<std::string>* _aidl_return) = 0;
  virtual ::ndk::ScopedAStatus RepeatParcelFileDescriptor(const ::ndk::ScopedFileDescriptor& in_read, ::ndk::ScopedFileDescriptor* _aidl_return) = 0;
  virtual ::ndk::ScopedAStatus ReverseParcelFileDescriptorArray(const std::vector<::ndk::ScopedFileDescriptor>& in_input, std::vector<::ndk::ScopedFileDescriptor>* out_repeated, std::vector<::ndk::ScopedFileDescriptor>* _aidl_return) = 0;
  virtual ::ndk::ScopedAStatus ThrowServiceException(int32_t in_code) = 0;
  virtual ::ndk::ScopedAStatus RepeatNullableIntArray(const std::optional<std::vector<int32_t>>& in_input, std::optional<std::vector<int32_t>>* _aidl_return) = 0;
  virtual ::ndk::ScopedAStatus RepeatNullableByteEnumArray(const std::optional<std::vector<::aidl::android::aidl::tests::ByteEnum>>& in_input, std::optional<std::vector<::aidl::android::aidl::tests::ByteEnum>>* _aidl_return) = 0;
  virtual ::ndk::ScopedAStatus RepeatNullableIntEnumArray(const std::optional<std::vector<::aidl::android::aidl::tests::IntEnum>>& in_input, std::optional<std::vector<::aidl::android::aidl::tests::IntEnum>>* _aidl_return) = 0;
  virtual ::ndk::ScopedAStatus RepeatNullableLongEnumArray(const std::optional<std::vector<::aidl::android::aidl::tests::LongEnum>>& in_input, std::optional<std::vector<::aidl::android::aidl::tests::LongEnum>>* _aidl_return) = 0;
  virtual ::ndk::ScopedAStatus RepeatNullableString(const std::optional<std::string>& in_input, std::optional<std::string>* _aidl_return) = 0;
  virtual ::ndk::ScopedAStatus RepeatNullableStringList(const std::vector<std::string>& in_input, std::vector<std::string>* _aidl_return) = 0;
  virtual ::ndk::ScopedAStatus RepeatNullableParcelable(const std::optional<::aidl::android::aidl::tests::StructuredParcelable>& in_input, std::optional<::aidl::android::aidl::tests::StructuredParcelable>* _aidl_return) = 0;
  virtual ::ndk::ScopedAStatus TakesAnIBinder(const ::ndk::SpAIBinder& in_input) = 0;
  virtual ::ndk::ScopedAStatus TakesANullableIBinder(const ::ndk::SpAIBinder& in_input) = 0;
  virtual ::ndk::ScopedAStatus RepeatUtf8CppString(const std::string& in_token, std::string* _aidl_return) = 0;
  virtual ::ndk::ScopedAStatus RepeatNullableUtf8CppString(const std::optional<std::string>& in_token, std::optional<std::string>* _aidl_return) = 0;
  virtual ::ndk::ScopedAStatus ReverseUtf8CppString(const std::vector<std::string>& in_input, std::vector<std::string>* out_repeated, std::vector<std::string>* _aidl_return) = 0;
  virtual ::ndk::ScopedAStatus ReverseNullableUtf8CppString(const std::optional<std::vector<std::optional<std::string>>>& in_input, std::optional<std::vector<std::optional<std::string>>>* out_repeated, std::optional<std::vector<std::optional<std::string>>>* _aidl_return) = 0;
  virtual ::ndk::ScopedAStatus ReverseUtf8CppStringList(const std::vector<std::string>& in_input, std::vector<std::string>* out_repeated, std::vector<std::string>* _aidl_return) = 0;
  virtual ::ndk::ScopedAStatus GetCallback(bool in_return_null, std::shared_ptr<::aidl::android::aidl::tests::INamedCallback>* _aidl_return) = 0;
  virtual ::ndk::ScopedAStatus FillOutStructuredParcelable(::aidl::android::aidl::tests::StructuredParcelable* in_parcel) = 0;
  virtual ::ndk::ScopedAStatus GetOldNameInterface(std::shared_ptr<::aidl::android::aidl::tests::IOldName>* _aidl_return) = 0;
  virtual ::ndk::ScopedAStatus GetNewNameInterface(std::shared_ptr<::aidl::android::aidl::tests::INewName>* _aidl_return) = 0;
  virtual ::ndk::ScopedAStatus GetCppJavaTests(::ndk::SpAIBinder* _aidl_return) = 0;
  virtual ::ndk::ScopedAStatus getBackendType(::aidl::android::aidl::tests::BackendType* _aidl_return) = 0;
private:
  static std::shared_ptr<ITestService> default_impl;
};
class ITestServiceDefault : public ITestService {
public:
  ::ndk::ScopedAStatus UnimplementedMethod(int32_t in_arg, int32_t* _aidl_return) override;
  ::ndk::ScopedAStatus Deprecated() override __attribute__((deprecated("to make sure we have something in system/tools/aidl which does a compile check of deprecated and make sure this is reflected in goldens")));
  ::ndk::ScopedAStatus TestOneway() override;
  ::ndk::ScopedAStatus RepeatBoolean(bool in_token, bool* _aidl_return) override;
  ::ndk::ScopedAStatus RepeatByte(int8_t in_token, int8_t* _aidl_return) override;
  ::ndk::ScopedAStatus RepeatChar(char16_t in_token, char16_t* _aidl_return) override;
  ::ndk::ScopedAStatus RepeatInt(int32_t in_token, int32_t* _aidl_return) override;
  ::ndk::ScopedAStatus RepeatLong(int64_t in_token, int64_t* _aidl_return) override;
  ::ndk::ScopedAStatus RepeatFloat(float in_token, float* _aidl_return) override;
  ::ndk::ScopedAStatus RepeatDouble(double in_token, double* _aidl_return) override;
  ::ndk::ScopedAStatus RepeatString(const std::string& in_token, std::string* _aidl_return) override;
  ::ndk::ScopedAStatus RepeatByteEnum(::aidl::android::aidl::tests::ByteEnum in_token, ::aidl::android::aidl::tests::ByteEnum* _aidl_return) override;
  ::ndk::ScopedAStatus RepeatIntEnum(::aidl::android::aidl::tests::IntEnum in_token, ::aidl::android::aidl::tests::IntEnum* _aidl_return) override;
  ::ndk::ScopedAStatus RepeatLongEnum(::aidl::android::aidl::tests::LongEnum in_token, ::aidl::android::aidl::tests::LongEnum* _aidl_return) override;
  ::ndk::ScopedAStatus ReverseBoolean(const std::vector<bool>& in_input, std::vector<bool>* out_repeated, std::vector<bool>* _aidl_return) override;
  ::ndk::ScopedAStatus ReverseByte(const std::vector<uint8_t>& in_input, std::vector<uint8_t>* out_repeated, std::vector<uint8_t>* _aidl_return) override;
  ::ndk::ScopedAStatus ReverseChar(const std::vector<char16_t>& in_input, std::vector<char16_t>* out_repeated, std::vector<char16_t>* _aidl_return) override;
  ::ndk::ScopedAStatus ReverseInt(const std::vector<int32_t>& in_input, std::vector<int32_t>* out_repeated, std::vector<int32_t>* _aidl_return) override;
  ::ndk::ScopedAStatus ReverseLong(const std::vector<int64_t>& in_input, std::vector<int64_t>* out_repeated, std::vector<int64_t>* _aidl_return) override;
  ::ndk::ScopedAStatus ReverseFloat(const std::vector<float>& in_input, std::vector<float>* out_repeated, std::vector<float>* _aidl_return) override;
  ::ndk::ScopedAStatus ReverseDouble(const std::vector<double>& in_input, std::vector<double>* out_repeated, std::vector<double>* _aidl_return) override;
  ::ndk::ScopedAStatus ReverseString(const std::vector<std::string>& in_input, std::vector<std::string>* out_repeated, std::vector<std::string>* _aidl_return) override;
  ::ndk::ScopedAStatus ReverseByteEnum(const std::vector<::aidl::android::aidl::tests::ByteEnum>& in_input, std::vector<::aidl::android::aidl::tests::ByteEnum>* out_repeated, std::vector<::aidl::android::aidl::tests::ByteEnum>* _aidl_return) override;
  ::ndk::ScopedAStatus ReverseIntEnum(const std::vector<::aidl::android::aidl::tests::IntEnum>& in_input, std::vector<::aidl::android::aidl::tests::IntEnum>* out_repeated, std::vector<::aidl::android::aidl::tests::IntEnum>* _aidl_return) override;
  ::ndk::ScopedAStatus ReverseLongEnum(const std::vector<::aidl::android::aidl::tests::LongEnum>& in_input, std::vector<::aidl::android::aidl::tests::LongEnum>* out_repeated, std::vector<::aidl::android::aidl::tests::LongEnum>* _aidl_return) override;
  ::ndk::ScopedAStatus GetOtherTestService(const std::string& in_name, std::shared_ptr<::aidl::android::aidl::tests::INamedCallback>* _aidl_return) override;
  ::ndk::ScopedAStatus VerifyName(const std::shared_ptr<::aidl::android::aidl::tests::INamedCallback>& in_service, const std::string& in_name, bool* _aidl_return) override;
  ::ndk::ScopedAStatus ReverseStringList(const std::vector<std::string>& in_input, std::vector<std::string>* out_repeated, std::vector<std::string>* _aidl_return) override;
  ::ndk::ScopedAStatus RepeatParcelFileDescriptor(const ::ndk::ScopedFileDescriptor& in_read, ::ndk::ScopedFileDescriptor* _aidl_return) override;
  ::ndk::ScopedAStatus ReverseParcelFileDescriptorArray(const std::vector<::ndk::ScopedFileDescriptor>& in_input, std::vector<::ndk::ScopedFileDescriptor>* out_repeated, std::vector<::ndk::ScopedFileDescriptor>* _aidl_return) override;
  ::ndk::ScopedAStatus ThrowServiceException(int32_t in_code) override;
  ::ndk::ScopedAStatus RepeatNullableIntArray(const std::optional<std::vector<int32_t>>& in_input, std::optional<std::vector<int32_t>>* _aidl_return) override;
  ::ndk::ScopedAStatus RepeatNullableByteEnumArray(const std::optional<std::vector<::aidl::android::aidl::tests::ByteEnum>>& in_input, std::optional<std::vector<::aidl::android::aidl::tests::ByteEnum>>* _aidl_return) override;
  ::ndk::ScopedAStatus RepeatNullableIntEnumArray(const std::optional<std::vector<::aidl::android::aidl::tests::IntEnum>>& in_input, std::optional<std::vector<::aidl::android::aidl::tests::IntEnum>>* _aidl_return) override;
  ::ndk::ScopedAStatus RepeatNullableLongEnumArray(const std::optional<std::vector<::aidl::android::aidl::tests::LongEnum>>& in_input, std::optional<std::vector<::aidl::android::aidl::tests::LongEnum>>* _aidl_return) override;
  ::ndk::ScopedAStatus RepeatNullableString(const std::optional<std::string>& in_input, std::optional<std::string>* _aidl_return) override;
  ::ndk::ScopedAStatus RepeatNullableStringList(const std::vector<std::string>& in_input, std::vector<std::string>* _aidl_return) override;
  ::ndk::ScopedAStatus RepeatNullableParcelable(const std::optional<::aidl::android::aidl::tests::StructuredParcelable>& in_input, std::optional<::aidl::android::aidl::tests::StructuredParcelable>* _aidl_return) override;
  ::ndk::ScopedAStatus TakesAnIBinder(const ::ndk::SpAIBinder& in_input) override;
  ::ndk::ScopedAStatus TakesANullableIBinder(const ::ndk::SpAIBinder& in_input) override;
  ::ndk::ScopedAStatus RepeatUtf8CppString(const std::string& in_token, std::string* _aidl_return) override;
  ::ndk::ScopedAStatus RepeatNullableUtf8CppString(const std::optional<std::string>& in_token, std::optional<std::string>* _aidl_return) override;
  ::ndk::ScopedAStatus ReverseUtf8CppString(const std::vector<std::string>& in_input, std::vector<std::string>* out_repeated, std::vector<std::string>* _aidl_return) override;
  ::ndk::ScopedAStatus ReverseNullableUtf8CppString(const std::optional<std::vector<std::optional<std::string>>>& in_input, std::optional<std::vector<std::optional<std::string>>>* out_repeated, std::optional<std::vector<std::optional<std::string>>>* _aidl_return) override;
  ::ndk::ScopedAStatus ReverseUtf8CppStringList(const std::vector<std::string>& in_input, std::vector<std::string>* out_repeated, std::vector<std::string>* _aidl_return) override;
  ::ndk::ScopedAStatus GetCallback(bool in_return_null, std::shared_ptr<::aidl::android::aidl::tests::INamedCallback>* _aidl_return) override;
  ::ndk::ScopedAStatus FillOutStructuredParcelable(::aidl::android::aidl::tests::StructuredParcelable* in_parcel) override;
  ::ndk::ScopedAStatus GetOldNameInterface(std::shared_ptr<::aidl::android::aidl::tests::IOldName>* _aidl_return) override;
  ::ndk::ScopedAStatus GetNewNameInterface(std::shared_ptr<::aidl::android::aidl::tests::INewName>* _aidl_return) override;
  ::ndk::ScopedAStatus GetCppJavaTests(::ndk::SpAIBinder* _aidl_return) override;
  ::ndk::ScopedAStatus getBackendType(::aidl::android::aidl::tests::BackendType* _aidl_return) override;
  ::ndk::SpAIBinder asBinder() override;
  bool isRemote() override;
};
}  // namespace tests
}  // namespace aidl
}  // namespace android
}  // namespace aidl
