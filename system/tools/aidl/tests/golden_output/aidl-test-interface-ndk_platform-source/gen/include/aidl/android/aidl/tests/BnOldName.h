#pragma once

#include "aidl/android/aidl/tests/IOldName.h"

#include <android/binder_ibinder.h>

namespace aidl {
namespace android {
namespace aidl {
namespace tests {
class BnOldName : public ::ndk::BnCInterface<IOldName> {
public:
  BnOldName();
  virtual ~BnOldName();
protected:
  ::ndk::SpAIBinder createBinder() override;
private:
};
}  // namespace tests
}  // namespace aidl
}  // namespace android
}  // namespace aidl
