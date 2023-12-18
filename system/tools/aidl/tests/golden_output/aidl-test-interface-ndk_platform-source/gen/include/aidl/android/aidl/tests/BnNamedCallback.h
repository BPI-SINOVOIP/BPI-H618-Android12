#pragma once

#include "aidl/android/aidl/tests/INamedCallback.h"

#include <android/binder_ibinder.h>

namespace aidl {
namespace android {
namespace aidl {
namespace tests {
class BnNamedCallback : public ::ndk::BnCInterface<INamedCallback> {
public:
  BnNamedCallback();
  virtual ~BnNamedCallback();
protected:
  ::ndk::SpAIBinder createBinder() override;
private:
};
}  // namespace tests
}  // namespace aidl
}  // namespace android
}  // namespace aidl
