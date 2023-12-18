#pragma once

#include <binder/IBinder.h>
#include <binder/IInterface.h>
#include <binder/Status.h>
#include <utils/String16.h>
#include <utils/StrongPointer.h>

namespace android {

namespace aidl {

namespace tests {

class IOldName : public ::android::IInterface {
public:
  DECLARE_META_INTERFACE(OldName)
  virtual ::android::binder::Status RealName(::android::String16* _aidl_return) = 0;
};  // class IOldName

class IOldNameDefault : public IOldName {
public:
  ::android::IBinder* onAsBinder() override {
    return nullptr;
  }
  ::android::binder::Status RealName(::android::String16*) override {
    return ::android::binder::Status::fromStatusT(::android::UNKNOWN_TRANSACTION);
  }
};  // class IOldNameDefault

}  // namespace tests

}  // namespace aidl

}  // namespace android
