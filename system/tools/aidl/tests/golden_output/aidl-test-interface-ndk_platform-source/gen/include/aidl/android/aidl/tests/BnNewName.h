#pragma once

#include "aidl/android/aidl/tests/INewName.h"

#include <android/binder_ibinder.h>

namespace aidl {
namespace android {
namespace aidl {
namespace tests {
class BnNewName : public ::ndk::BnCInterface<INewName> {
public:
  BnNewName();
  virtual ~BnNewName();
protected:
  ::ndk::SpAIBinder createBinder() override;
private:
};
}  // namespace tests
}  // namespace aidl
}  // namespace android
}  // namespace aidl
