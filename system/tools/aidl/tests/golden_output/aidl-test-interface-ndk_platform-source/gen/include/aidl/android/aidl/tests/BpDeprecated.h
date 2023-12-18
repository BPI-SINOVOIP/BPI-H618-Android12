#pragma once

#include "aidl/android/aidl/tests/IDeprecated.h"

#include <android/binder_ibinder.h>

namespace aidl {
namespace android {
namespace aidl {
namespace tests {
class __attribute__((deprecated("test"))) BpDeprecated : public ::ndk::BpCInterface<IDeprecated> {
public:
  explicit BpDeprecated(const ::ndk::SpAIBinder& binder);
  virtual ~BpDeprecated();

};
}  // namespace tests
}  // namespace aidl
}  // namespace android
}  // namespace aidl
