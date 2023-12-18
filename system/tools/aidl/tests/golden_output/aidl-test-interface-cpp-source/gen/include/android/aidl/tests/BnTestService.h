#pragma once

#include <binder/IInterface.h>
#include <android/aidl/tests/ITestService.h>

namespace android {

namespace aidl {

namespace tests {

class BnTestService : public ::android::BnInterface<ITestService> {
public:
  static constexpr uint32_t TRANSACTION_UnimplementedMethod = ::android::IBinder::FIRST_CALL_TRANSACTION + 0;
  static constexpr uint32_t TRANSACTION_Deprecated = ::android::IBinder::FIRST_CALL_TRANSACTION + 1;
  static constexpr uint32_t TRANSACTION_TestOneway = ::android::IBinder::FIRST_CALL_TRANSACTION + 2;
  static constexpr uint32_t TRANSACTION_RepeatBoolean = ::android::IBinder::FIRST_CALL_TRANSACTION + 3;
  static constexpr uint32_t TRANSACTION_RepeatByte = ::android::IBinder::FIRST_CALL_TRANSACTION + 4;
  static constexpr uint32_t TRANSACTION_RepeatChar = ::android::IBinder::FIRST_CALL_TRANSACTION + 5;
  static constexpr uint32_t TRANSACTION_RepeatInt = ::android::IBinder::FIRST_CALL_TRANSACTION + 6;
  static constexpr uint32_t TRANSACTION_RepeatLong = ::android::IBinder::FIRST_CALL_TRANSACTION + 7;
  static constexpr uint32_t TRANSACTION_RepeatFloat = ::android::IBinder::FIRST_CALL_TRANSACTION + 8;
  static constexpr uint32_t TRANSACTION_RepeatDouble = ::android::IBinder::FIRST_CALL_TRANSACTION + 9;
  static constexpr uint32_t TRANSACTION_RepeatString = ::android::IBinder::FIRST_CALL_TRANSACTION + 10;
  static constexpr uint32_t TRANSACTION_RepeatByteEnum = ::android::IBinder::FIRST_CALL_TRANSACTION + 11;
  static constexpr uint32_t TRANSACTION_RepeatIntEnum = ::android::IBinder::FIRST_CALL_TRANSACTION + 12;
  static constexpr uint32_t TRANSACTION_RepeatLongEnum = ::android::IBinder::FIRST_CALL_TRANSACTION + 13;
  static constexpr uint32_t TRANSACTION_ReverseBoolean = ::android::IBinder::FIRST_CALL_TRANSACTION + 14;
  static constexpr uint32_t TRANSACTION_ReverseByte = ::android::IBinder::FIRST_CALL_TRANSACTION + 15;
  static constexpr uint32_t TRANSACTION_ReverseChar = ::android::IBinder::FIRST_CALL_TRANSACTION + 16;
  static constexpr uint32_t TRANSACTION_ReverseInt = ::android::IBinder::FIRST_CALL_TRANSACTION + 17;
  static constexpr uint32_t TRANSACTION_ReverseLong = ::android::IBinder::FIRST_CALL_TRANSACTION + 18;
  static constexpr uint32_t TRANSACTION_ReverseFloat = ::android::IBinder::FIRST_CALL_TRANSACTION + 19;
  static constexpr uint32_t TRANSACTION_ReverseDouble = ::android::IBinder::FIRST_CALL_TRANSACTION + 20;
  static constexpr uint32_t TRANSACTION_ReverseString = ::android::IBinder::FIRST_CALL_TRANSACTION + 21;
  static constexpr uint32_t TRANSACTION_ReverseByteEnum = ::android::IBinder::FIRST_CALL_TRANSACTION + 22;
  static constexpr uint32_t TRANSACTION_ReverseIntEnum = ::android::IBinder::FIRST_CALL_TRANSACTION + 23;
  static constexpr uint32_t TRANSACTION_ReverseLongEnum = ::android::IBinder::FIRST_CALL_TRANSACTION + 24;
  static constexpr uint32_t TRANSACTION_GetOtherTestService = ::android::IBinder::FIRST_CALL_TRANSACTION + 25;
  static constexpr uint32_t TRANSACTION_VerifyName = ::android::IBinder::FIRST_CALL_TRANSACTION + 26;
  static constexpr uint32_t TRANSACTION_ReverseStringList = ::android::IBinder::FIRST_CALL_TRANSACTION + 27;
  static constexpr uint32_t TRANSACTION_RepeatParcelFileDescriptor = ::android::IBinder::FIRST_CALL_TRANSACTION + 28;
  static constexpr uint32_t TRANSACTION_ReverseParcelFileDescriptorArray = ::android::IBinder::FIRST_CALL_TRANSACTION + 29;
  static constexpr uint32_t TRANSACTION_ThrowServiceException = ::android::IBinder::FIRST_CALL_TRANSACTION + 30;
  static constexpr uint32_t TRANSACTION_RepeatNullableIntArray = ::android::IBinder::FIRST_CALL_TRANSACTION + 31;
  static constexpr uint32_t TRANSACTION_RepeatNullableByteEnumArray = ::android::IBinder::FIRST_CALL_TRANSACTION + 32;
  static constexpr uint32_t TRANSACTION_RepeatNullableIntEnumArray = ::android::IBinder::FIRST_CALL_TRANSACTION + 33;
  static constexpr uint32_t TRANSACTION_RepeatNullableLongEnumArray = ::android::IBinder::FIRST_CALL_TRANSACTION + 34;
  static constexpr uint32_t TRANSACTION_RepeatNullableString = ::android::IBinder::FIRST_CALL_TRANSACTION + 35;
  static constexpr uint32_t TRANSACTION_RepeatNullableStringList = ::android::IBinder::FIRST_CALL_TRANSACTION + 36;
  static constexpr uint32_t TRANSACTION_RepeatNullableParcelable = ::android::IBinder::FIRST_CALL_TRANSACTION + 37;
  static constexpr uint32_t TRANSACTION_TakesAnIBinder = ::android::IBinder::FIRST_CALL_TRANSACTION + 38;
  static constexpr uint32_t TRANSACTION_TakesANullableIBinder = ::android::IBinder::FIRST_CALL_TRANSACTION + 39;
  static constexpr uint32_t TRANSACTION_RepeatUtf8CppString = ::android::IBinder::FIRST_CALL_TRANSACTION + 40;
  static constexpr uint32_t TRANSACTION_RepeatNullableUtf8CppString = ::android::IBinder::FIRST_CALL_TRANSACTION + 41;
  static constexpr uint32_t TRANSACTION_ReverseUtf8CppString = ::android::IBinder::FIRST_CALL_TRANSACTION + 42;
  static constexpr uint32_t TRANSACTION_ReverseNullableUtf8CppString = ::android::IBinder::FIRST_CALL_TRANSACTION + 43;
  static constexpr uint32_t TRANSACTION_ReverseUtf8CppStringList = ::android::IBinder::FIRST_CALL_TRANSACTION + 44;
  static constexpr uint32_t TRANSACTION_GetCallback = ::android::IBinder::FIRST_CALL_TRANSACTION + 45;
  static constexpr uint32_t TRANSACTION_FillOutStructuredParcelable = ::android::IBinder::FIRST_CALL_TRANSACTION + 46;
  static constexpr uint32_t TRANSACTION_GetOldNameInterface = ::android::IBinder::FIRST_CALL_TRANSACTION + 47;
  static constexpr uint32_t TRANSACTION_GetNewNameInterface = ::android::IBinder::FIRST_CALL_TRANSACTION + 48;
  static constexpr uint32_t TRANSACTION_GetCppJavaTests = ::android::IBinder::FIRST_CALL_TRANSACTION + 49;
  static constexpr uint32_t TRANSACTION_getBackendType = ::android::IBinder::FIRST_CALL_TRANSACTION + 50;
  explicit BnTestService();
  ::android::status_t onTransact(uint32_t _aidl_code, const ::android::Parcel& _aidl_data, ::android::Parcel* _aidl_reply, uint32_t _aidl_flags) override;
};  // class BnTestService

}  // namespace tests

}  // namespace aidl

}  // namespace android
