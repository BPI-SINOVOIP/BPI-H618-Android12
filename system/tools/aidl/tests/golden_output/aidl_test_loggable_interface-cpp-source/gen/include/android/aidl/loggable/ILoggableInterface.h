#pragma once

#include <android/aidl/loggable/Data.h>
#include <binder/IBinder.h>
#include <binder/IInterface.h>
#include <binder/ParcelFileDescriptor.h>
#include <binder/Status.h>
#include <cstdint>
#include <optional>
#include <utils/String16.h>
#include <utils/StrongPointer.h>
#include <utils/Trace.h>
#include <vector>

namespace android {

namespace aidl {

namespace loggable {

class ILoggableInterface : public ::android::IInterface {
public:
  DECLARE_META_INTERFACE(LoggableInterface)
  virtual ::android::binder::Status LogThis(bool boolValue, ::std::vector<bool>* boolArray, int8_t byteValue, ::std::vector<uint8_t>* byteArray, char16_t charValue, ::std::vector<char16_t>* charArray, int32_t intValue, ::std::vector<int32_t>* intArray, int64_t longValue, ::std::vector<int64_t>* longArray, float floatValue, ::std::vector<float>* floatArray, double doubleValue, ::std::vector<double>* doubleArray, const ::android::String16& stringValue, ::std::vector<::android::String16>* stringArray, ::std::vector<::android::String16>* listValue, const ::android::aidl::loggable::Data& dataValue, const ::android::sp<::android::IBinder>& binderValue, ::std::optional<::android::os::ParcelFileDescriptor>* pfdValue, ::std::vector<::android::os::ParcelFileDescriptor>* pfdArray, ::std::vector<::android::String16>* _aidl_return) = 0;
};  // class ILoggableInterface

class ILoggableInterfaceDefault : public ILoggableInterface {
public:
  ::android::IBinder* onAsBinder() override {
    return nullptr;
  }
  ::android::binder::Status LogThis(bool, ::std::vector<bool>*, int8_t, ::std::vector<uint8_t>*, char16_t, ::std::vector<char16_t>*, int32_t, ::std::vector<int32_t>*, int64_t, ::std::vector<int64_t>*, float, ::std::vector<float>*, double, ::std::vector<double>*, const ::android::String16&, ::std::vector<::android::String16>*, ::std::vector<::android::String16>*, const ::android::aidl::loggable::Data&, const ::android::sp<::android::IBinder>&, ::std::optional<::android::os::ParcelFileDescriptor>*, ::std::vector<::android::os::ParcelFileDescriptor>*, ::std::vector<::android::String16>*) override {
    return ::android::binder::Status::fromStatusT(::android::UNKNOWN_TRANSACTION);
  }
};  // class ILoggableInterfaceDefault

}  // namespace loggable

}  // namespace aidl

}  // namespace android
