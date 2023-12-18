#pragma once

#include "aidl/android/aidl/tests/ITestService.h"

#include <android/binder_ibinder.h>

namespace aidl {
namespace android {
namespace aidl {
namespace tests {
class BnTestService : public ::ndk::BnCInterface<ITestService> {
public:
  BnTestService();
  virtual ~BnTestService();
protected:
  ::ndk::SpAIBinder createBinder() override;
private:
};
}  // namespace tests
}  // namespace aidl
}  // namespace android
}  // namespace aidl
