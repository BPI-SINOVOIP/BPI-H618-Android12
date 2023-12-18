#pragma once

#include "aidl/android/aidl/tests/INewName.h"

#include <android/binder_ibinder.h>

namespace aidl {
namespace android {
namespace aidl {
namespace tests {
class BpNewName : public ::ndk::BpCInterface<INewName> {
public:
  explicit BpNewName(const ::ndk::SpAIBinder& binder);
  virtual ~BpNewName();

  ::ndk::ScopedAStatus RealName(std::string* _aidl_return) override;
};
}  // namespace tests
}  // namespace aidl
}  // namespace android
}  // namespace aidl
