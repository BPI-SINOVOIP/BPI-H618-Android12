#pragma once

#include <binder/IBinder.h>
#include <binder/IInterface.h>
#include <utils/Errors.h>
#include <android/aidl/tests/IOldName.h>

namespace android {

namespace aidl {

namespace tests {

class BpOldName : public ::android::BpInterface<IOldName> {
public:
  explicit BpOldName(const ::android::sp<::android::IBinder>& _aidl_impl);
  virtual ~BpOldName() = default;
  ::android::binder::Status RealName(::android::String16* _aidl_return) override;
};  // class BpOldName

}  // namespace tests

}  // namespace aidl

}  // namespace android
