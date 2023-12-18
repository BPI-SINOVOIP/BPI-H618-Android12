#pragma once

#include <binder/IBinder.h>
#include <binder/IInterface.h>
#include <utils/Errors.h>
#include <android/aidl/tests/INamedCallback.h>

namespace android {

namespace aidl {

namespace tests {

class BpNamedCallback : public ::android::BpInterface<INamedCallback> {
public:
  explicit BpNamedCallback(const ::android::sp<::android::IBinder>& _aidl_impl);
  virtual ~BpNamedCallback() = default;
  ::android::binder::Status GetName(::android::String16* _aidl_return) override;
};  // class BpNamedCallback

}  // namespace tests

}  // namespace aidl

}  // namespace android
