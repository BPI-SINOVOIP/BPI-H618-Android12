#pragma once

#include "aidl/android/aidl/tests/IDeprecated.h"

#include <android/binder_ibinder.h>

namespace aidl {
namespace android {
namespace aidl {
namespace tests {
class __attribute__((deprecated("test"))) BnDeprecated : public ::ndk::BnCInterface<IDeprecated> {
public:
  BnDeprecated();
  virtual ~BnDeprecated();
protected:
  ::ndk::SpAIBinder createBinder() override;
private:
};
}  // namespace tests
}  // namespace aidl
}  // namespace android
}  // namespace aidl
