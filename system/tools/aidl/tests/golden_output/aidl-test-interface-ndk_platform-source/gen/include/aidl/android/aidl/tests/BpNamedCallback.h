#pragma once

#include "aidl/android/aidl/tests/INamedCallback.h"

#include <android/binder_ibinder.h>

namespace aidl {
namespace android {
namespace aidl {
namespace tests {
class BpNamedCallback : public ::ndk::BpCInterface<INamedCallback> {
public:
  explicit BpNamedCallback(const ::ndk::SpAIBinder& binder);
  virtual ~BpNamedCallback();

  ::ndk::ScopedAStatus GetName(std::string* _aidl_return) override;
};
}  // namespace tests
}  // namespace aidl
}  // namespace android
}  // namespace aidl
