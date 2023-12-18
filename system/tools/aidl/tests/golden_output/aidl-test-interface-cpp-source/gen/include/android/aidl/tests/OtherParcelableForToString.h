#pragma once

#include <android/binder_to_string.h>
#include <binder/Parcel.h>
#include <binder/Status.h>
#include <tuple>
#include <utils/String16.h>

namespace android {

namespace aidl {

namespace tests {

class OtherParcelableForToString : public ::android::Parcelable {
public:
  inline bool operator!=(const OtherParcelableForToString& rhs) const {
    return std::tie(field) != std::tie(rhs.field);
  }
  inline bool operator<(const OtherParcelableForToString& rhs) const {
    return std::tie(field) < std::tie(rhs.field);
  }
  inline bool operator<=(const OtherParcelableForToString& rhs) const {
    return std::tie(field) <= std::tie(rhs.field);
  }
  inline bool operator==(const OtherParcelableForToString& rhs) const {
    return std::tie(field) == std::tie(rhs.field);
  }
  inline bool operator>(const OtherParcelableForToString& rhs) const {
    return std::tie(field) > std::tie(rhs.field);
  }
  inline bool operator>=(const OtherParcelableForToString& rhs) const {
    return std::tie(field) >= std::tie(rhs.field);
  }

  ::android::String16 field;
  ::android::status_t readFromParcel(const ::android::Parcel* _aidl_parcel) final;
  ::android::status_t writeToParcel(::android::Parcel* _aidl_parcel) const final;
  static const ::android::String16& getParcelableDescriptor() {
    static const ::android::StaticString16 DESCIPTOR (u"android.aidl.tests.OtherParcelableForToString");
    return DESCIPTOR;
  }
  inline std::string toString() const {
    std::ostringstream os;
    os << "OtherParcelableForToString{";
    os << "field: " << ::android::internal::ToString(field);
    os << "}";
    return os.str();
  }
};  // class OtherParcelableForToString

}  // namespace tests

}  // namespace aidl

}  // namespace android
