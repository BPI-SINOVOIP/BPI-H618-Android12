#pragma once

#include <binder/IBinder.h>
#include <binder/IInterface.h>
#include <utils/Errors.h>
#include <android/aidl/tests/INewName.h>

namespace android {

namespace aidl {

namespace tests {

class BpNewName : public ::android::BpInterface<INewName> {
public:
  explicit BpNewName(const ::android::sp<::android::IBinder>& _aidl_impl);
  virtual ~BpNewName() = default;
  ::android::binder::Status RealName(::android::String16* _aidl_return) override;
};  // class BpNewName

}  // namespace tests

}  // namespace aidl

}  // namespace android
