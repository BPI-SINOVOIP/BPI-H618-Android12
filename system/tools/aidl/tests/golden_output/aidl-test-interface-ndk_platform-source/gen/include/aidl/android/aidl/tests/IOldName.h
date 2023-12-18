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
class IOldName : public ::ndk::ICInterface {
public:
  static const char* descriptor;
  IOldName();
  virtual ~IOldName();

  static constexpr uint32_t TRANSACTION_RealName = FIRST_CALL_TRANSACTION + 0;

  static std::shared_ptr<IOldName> fromBinder(const ::ndk::SpAIBinder& binder);
  static binder_status_t writeToParcel(AParcel* parcel, const std::shared_ptr<IOldName>& instance);
  static binder_status_t readFromParcel(const AParcel* parcel, std::shared_ptr<IOldName>* instance);
  static bool setDefaultImpl(const std::shared_ptr<IOldName>& impl);
  static const std::shared_ptr<IOldName>& getDefaultImpl();
  virtual ::ndk::ScopedAStatus RealName(std::string* _aidl_return) = 0;
private:
  static std::shared_ptr<IOldName> default_impl;
};
class IOldNameDefault : public IOldName {
public:
  ::ndk::ScopedAStatus RealName(std::string* _aidl_return) override;
  ::ndk::SpAIBinder asBinder() override;
  bool isRemote() override;
};
}  // namespace tests
}  // namespace aidl
}  // namespace android
}  // namespace aidl
