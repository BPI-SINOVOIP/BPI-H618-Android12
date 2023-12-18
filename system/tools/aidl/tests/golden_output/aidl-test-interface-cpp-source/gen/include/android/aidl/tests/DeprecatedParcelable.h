#pragma once

#include <android/binder_to_string.h>
#include <binder/Parcel.h>
#include <binder/Status.h>
#include <tuple>
#include <utils/String16.h>

namespace android {

namespace aidl {

namespace tests {

class __attribute__((deprecated("test"))) DeprecatedParcelable : public ::android::Parcelable {
public:
  inline bool operator!=(const DeprecatedParcelable&) const {
    return std::tie() != std::tie();
  }
  inline bool operator<(const DeprecatedParcelable&) const {
    return std::tie() < std::tie();
  }
  inline bool operator<=(const DeprecatedParcelable&) const {
    return std::tie() <= std::tie();
  }
  inline bool operator==(const DeprecatedParcelable&) const {
    return std::tie() == std::tie();
  }
  inline bool operator>(const DeprecatedParcelable&) const {
    return std::tie() > std::tie();
  }
  inline bool operator>=(const DeprecatedParcelable&) const {
    return std::tie() >= std::tie();
  }

  ::android::status_t readFromParcel(const ::android::Parcel* _aidl_parcel) final;
  ::android::status_t writeToParcel(::android::Parcel* _aidl_parcel) const final;
  static const ::android::String16& getParcelableDescriptor() {
    static const ::android::StaticString16 DESCIPTOR (u"android.aidl.tests.DeprecatedParcelable");
    return DESCIPTOR;
  }
  inline std::string toString() const {
    std::ostringstream os;
    os << "DeprecatedParcelable{";
    os << "}";
    return os.str();
  }
};  // class DeprecatedParcelable

}  // namespace tests

}  // namespace aidl

}  // namespace android
