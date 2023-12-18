#pragma once

#include <android/binder_interface_utils.h>
#include <functional>
#include <chrono>
#include <sstream>

#include <cstdint>
#include <memory>
#include <optional>
#include <string>
#include <vector>
#ifdef BINDER_STABILITY_SUPPORT
#include <android/binder_stability.h>
#endif  // BINDER_STABILITY_SUPPORT
#include <aidl/android/aidl/loggable/Data.h>

namespace aidl {
namespace android {
namespace aidl {
namespace loggable {
class ILoggableInterface : public ::ndk::ICInterface {
public:
  static const char* descriptor;
  ILoggableInterface();
  virtual ~ILoggableInterface();

  static constexpr uint32_t TRANSACTION_LogThis = FIRST_CALL_TRANSACTION + 0;

  static std::shared_ptr<ILoggableInterface> fromBinder(const ::ndk::SpAIBinder& binder);
  static binder_status_t writeToParcel(AParcel* parcel, const std::shared_ptr<ILoggableInterface>& instance);
  static binder_status_t readFromParcel(const AParcel* parcel, std::shared_ptr<ILoggableInterface>* instance);
  static bool setDefaultImpl(const std::shared_ptr<ILoggableInterface>& impl);
  static const std::shared_ptr<ILoggableInterface>& getDefaultImpl();
  virtual ::ndk::ScopedAStatus LogThis(bool in_boolValue, std::vector<bool>* in_boolArray, int8_t in_byteValue, std::vector<uint8_t>* in_byteArray, char16_t in_charValue, std::vector<char16_t>* in_charArray, int32_t in_intValue, std::vector<int32_t>* in_intArray, int64_t in_longValue, std::vector<int64_t>* in_longArray, float in_floatValue, std::vector<float>* in_floatArray, double in_doubleValue, std::vector<double>* in_doubleArray, const std::string& in_stringValue, std::vector<std::string>* in_stringArray, std::vector<std::string>* in_listValue, const ::aidl::android::aidl::loggable::Data& in_dataValue, const ::ndk::SpAIBinder& in_binderValue, ::ndk::ScopedFileDescriptor* in_pfdValue, std::vector<::ndk::ScopedFileDescriptor>* in_pfdArray, std::vector<std::string>* _aidl_return) = 0;
private:
  static std::shared_ptr<ILoggableInterface> default_impl;
};
class ILoggableInterfaceDefault : public ILoggableInterface {
public:
  ::ndk::ScopedAStatus LogThis(bool in_boolValue, std::vector<bool>* in_boolArray, int8_t in_byteValue, std::vector<uint8_t>* in_byteArray, char16_t in_charValue, std::vector<char16_t>* in_charArray, int32_t in_intValue, std::vector<int32_t>* in_intArray, int64_t in_longValue, std::vector<int64_t>* in_longArray, float in_floatValue, std::vector<float>* in_floatArray, double in_doubleValue, std::vector<double>* in_doubleArray, const std::string& in_stringValue, std::vector<std::string>* in_stringArray, std::vector<std::string>* in_listValue, const ::aidl::android::aidl::loggable::Data& in_dataValue, const ::ndk::SpAIBinder& in_binderValue, ::ndk::ScopedFileDescriptor* in_pfdValue, std::vector<::ndk::ScopedFileDescriptor>* in_pfdArray, std::vector<std::string>* _aidl_return) override;
  ::ndk::SpAIBinder asBinder() override;
  bool isRemote() override;
};
}  // namespace loggable
}  // namespace aidl
}  // namespace android
}  // namespace aidl
