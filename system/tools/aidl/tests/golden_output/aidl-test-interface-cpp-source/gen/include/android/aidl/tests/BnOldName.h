#pragma once

#include <binder/IInterface.h>
#include <android/aidl/tests/IOldName.h>

namespace android {

namespace aidl {

namespace tests {

class BnOldName : public ::android::BnInterface<IOldName> {
public:
  static constexpr uint32_t TRANSACTION_RealName = ::android::IBinder::FIRST_CALL_TRANSACTION + 0;
  explicit BnOldName();
  ::android::status_t onTransact(uint32_t _aidl_code, const ::android::Parcel& _aidl_data, ::android::Parcel* _aidl_reply, uint32_t _aidl_flags) override;
};  // class BnOldName

}  // namespace tests

}  // namespace aidl

}  // namespace android
