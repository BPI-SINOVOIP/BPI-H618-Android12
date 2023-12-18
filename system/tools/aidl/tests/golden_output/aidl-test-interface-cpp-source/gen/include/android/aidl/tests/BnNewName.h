#pragma once

#include <binder/IInterface.h>
#include <android/aidl/tests/INewName.h>

namespace android {

namespace aidl {

namespace tests {

class BnNewName : public ::android::BnInterface<INewName> {
public:
  static constexpr uint32_t TRANSACTION_RealName = ::android::IBinder::FIRST_CALL_TRANSACTION + 0;
  explicit BnNewName();
  ::android::status_t onTransact(uint32_t _aidl_code, const ::android::Parcel& _aidl_data, ::android::Parcel* _aidl_reply, uint32_t _aidl_flags) override;
};  // class BnNewName

}  // namespace tests

}  // namespace aidl

}  // namespace android
