#pragma once

#include <android/binder_interface_utils.h>

#include <cstdint>
#include <memory>
#include <optional>
#include <string>
#include <vector>
#ifdef BINDER_STABILITY_SUPPORT
#include <android/binder_stability.h>
#endif  // BINDER_STABILITY_SUPPORT

namespace aidl {
namespace android {
namespace aidl {
namespace tests {
class __attribute__((deprecated("test"))) IDeprecated : public ::ndk::ICInterface {
public:
  static const char* descriptor;
  IDeprecated();
  virtual ~IDeprecated();


  static std::shared_ptr<IDeprecated> fromBinder(const ::ndk::SpAIBinder& binder);
  static binder_status_t writeToParcel(AParcel* parcel, const std::shared_ptr<IDeprecated>& instance);
  static binder_status_t readFromParcel(const AParcel* parcel, std::shared_ptr<IDeprecated>* instance);
  static bool setDefaultImpl(const std::shared_ptr<IDeprecated>& impl);
  static const std::shared_ptr<IDeprecated>& getDefaultImpl();
private:
  static std::shared_ptr<IDeprecated> default_impl;
};
class __attribute__((deprecated("test"))) IDeprecatedDefault : public IDeprecated {
public:
  ::ndk::SpAIBinder asBinder() override;
  bool isRemote() override;
};
}  // namespace tests
}  // namespace aidl
}  // namespace android
}  // namespace aidl
