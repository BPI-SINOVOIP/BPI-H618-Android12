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
class INamedCallback : public ::ndk::ICInterface {
public:
  static const char* descriptor;
  INamedCallback();
  virtual ~INamedCallback();

  static constexpr uint32_t TRANSACTION_GetName = FIRST_CALL_TRANSACTION + 0;

  static std::shared_ptr<INamedCallback> fromBinder(const ::ndk::SpAIBinder& binder);
  static binder_status_t writeToParcel(AParcel* parcel, const std::shared_ptr<INamedCallback>& instance);
  static binder_status_t readFromParcel(const AParcel* parcel, std::shared_ptr<INamedCallback>* instance);
  static bool setDefaultImpl(const std::shared_ptr<INamedCallback>& impl);
  static const std::shared_ptr<INamedCallback>& getDefaultImpl();
  virtual ::ndk::ScopedAStatus GetName(std::string* _aidl_return) = 0;
private:
  static std::shared_ptr<INamedCallback> default_impl;
};
class INamedCallbackDefault : public INamedCallback {
public:
  ::ndk::ScopedAStatus GetName(std::string* _aidl_return) override;
  ::ndk::SpAIBinder asBinder() override;
  bool isRemote() override;
};
}  // namespace tests
}  // namespace aidl
}  // namespace android
}  // namespace aidl
