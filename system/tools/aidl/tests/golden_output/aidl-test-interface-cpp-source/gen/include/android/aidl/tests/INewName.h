#pragma once

#include <binder/IBinder.h>
#include <binder/IInterface.h>
#include <binder/Status.h>
#include <utils/String16.h>
#include <utils/StrongPointer.h>

namespace android {

namespace aidl {

namespace tests {

class INewName : public ::android::IInterface {
public:
  DECLARE_META_INTERFACE(NewName)
  virtual ::android::binder::Status RealName(::android::String16* _aidl_return) = 0;
};  // class INewName

class INewNameDefault : public INewName {
public:
  ::android::IBinder* onAsBinder() override {
    return nullptr;
  }
  ::android::binder::Status RealName(::android::String16*) override {
    return ::android::binder::Status::fromStatusT(::android::UNKNOWN_TRANSACTION);
  }
};  // class INewNameDefault

}  // namespace tests

}  // namespace aidl

}  // namespace android
