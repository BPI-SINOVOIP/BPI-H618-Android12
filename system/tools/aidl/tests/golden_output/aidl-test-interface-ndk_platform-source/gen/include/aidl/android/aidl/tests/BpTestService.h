#pragma once

#include "aidl/android/aidl/tests/ITestService.h"

#include <android/binder_ibinder.h>

namespace aidl {
namespace android {
namespace aidl {
namespace tests {
class BpTestService : public ::ndk::BpCInterface<ITestService> {
public:
  explicit BpTestService(const ::ndk::SpAIBinder& binder);
  virtual ~BpTestService();

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
};
}  // namespace tests
}  // namespace aidl
}  // namespace android
}  // namespace aidl
