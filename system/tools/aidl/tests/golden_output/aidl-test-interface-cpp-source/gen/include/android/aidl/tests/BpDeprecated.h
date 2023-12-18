#pragma once

#include <binder/IBinder.h>
#include <binder/IInterface.h>
#include <utils/Errors.h>
#include <android/aidl/tests/IDeprecated.h>

namespace android {

namespace aidl {

namespace tests {

class __attribute__((deprecated("test"))) BpDeprecated : public ::android::BpInterface<IDeprecated> {
public:
  explicit BpDeprecated(const ::android::sp<::android::IBinder>& _aidl_impl);
  virtual ~BpDeprecated() = default;
};  // class BpDeprecated

}  // namespace tests

}  // namespace aidl

}  // namespace android
