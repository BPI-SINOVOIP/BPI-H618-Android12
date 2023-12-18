#pragma once

#include <binder/IBinder.h>
#include <binder/IInterface.h>
#include <binder/Status.h>
#include <utils/StrongPointer.h>

namespace android {

namespace aidl {

namespace tests {

class __attribute__((deprecated("test"))) IDeprecated : public ::android::IInterface {
public:
  DECLARE_META_INTERFACE(Deprecated)
};  // class IDeprecated

class __attribute__((deprecated("test"))) IDeprecatedDefault : public IDeprecated {
public:
  ::android::IBinder* onAsBinder() override {
    return nullptr;
  }
};  // class IDeprecatedDefault

}  // namespace tests

}  // namespace aidl

}  // namespace android
