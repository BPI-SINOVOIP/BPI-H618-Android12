#pragma once

#include <binder/IBinder.h>
#include <binder/IInterface.h>
#include <binder/Status.h>
#include <utils/String16.h>
#include <utils/StrongPointer.h>

namespace android {

namespace aidl {

namespace tests {

class INamedCallback : public ::android::IInterface {
public:
  DECLARE_META_INTERFACE(NamedCallback)
  virtual ::android::binder::Status GetName(::android::String16* _aidl_return) = 0;
};  // class INamedCallback

class INamedCallbackDefault : public INamedCallback {
public:
  ::android::IBinder* onAsBinder() override {
    return nullptr;
  }
  ::android::binder::Status GetName(::android::String16*) override {
    return ::android::binder::Status::fromStatusT(::android::UNKNOWN_TRANSACTION);
  }
};  // class INamedCallbackDefault

}  // namespace tests

}  // namespace aidl

}  // namespace android
