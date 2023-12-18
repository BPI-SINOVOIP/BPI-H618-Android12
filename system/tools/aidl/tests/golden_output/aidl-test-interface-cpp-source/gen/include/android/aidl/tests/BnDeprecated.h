#pragma once

#include <binder/IInterface.h>
#include <android/aidl/tests/IDeprecated.h>

namespace android {

namespace aidl {

namespace tests {

class __attribute__((deprecated("test"))) BnDeprecated : public ::android::BnInterface<IDeprecated> {
public:
  explicit BnDeprecated();
  ::android::status_t onTransact(uint32_t _aidl_code, const ::android::Parcel& _aidl_data, ::android::Parcel* _aidl_reply, uint32_t _aidl_flags) override;
};  // class BnDeprecated

}  // namespace tests

}  // namespace aidl

}  // namespace android
