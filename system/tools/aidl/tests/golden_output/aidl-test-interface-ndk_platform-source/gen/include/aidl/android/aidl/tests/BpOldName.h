#pragma once

#include "aidl/android/aidl/tests/IOldName.h"

#include <android/binder_ibinder.h>

namespace aidl {
namespace android {
namespace aidl {
namespace tests {
class BpOldName : public ::ndk::BpCInterface<IOldName> {
public:
  explicit BpOldName(const ::ndk::SpAIBinder& binder);
  virtual ~BpOldName();

  ::ndk::ScopedAStatus RealName(std::string* _aidl_return) override;
};
}  // namespace tests
}  // namespace aidl
}  // namespace android
}  // namespace aidl
