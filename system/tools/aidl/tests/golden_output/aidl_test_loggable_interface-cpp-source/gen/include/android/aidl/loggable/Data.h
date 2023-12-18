#pragma once

#include <android/aidl/loggable/Enum.h>
#include <android/aidl/loggable/Union.h>
#include <android/binder_to_string.h>
#include <binder/Parcel.h>
#include <binder/Status.h>
#include <cstdint>
#include <string>
#include <tuple>
#include <utils/String16.h>

namespace android {

namespace aidl {

namespace loggable {

class Data : public ::android::Parcelable {
public:
  inline bool operator!=(const Data& rhs) const {
    return std::tie(num, str, nestedUnion, nestedEnum) != std::tie(rhs.num, rhs.str, rhs.nestedUnion, rhs.nestedEnum);
  }
  inline bool operator<(const Data& rhs) const {
    return std::tie(num, str, nestedUnion, nestedEnum) < std::tie(rhs.num, rhs.str, rhs.nestedUnion, rhs.nestedEnum);
  }
  inline bool operator<=(const Data& rhs) const {
    return std::tie(num, str, nestedUnion, nestedEnum) <= std::tie(rhs.num, rhs.str, rhs.nestedUnion, rhs.nestedEnum);
  }
  inline bool operator==(const Data& rhs) const {
    return std::tie(num, str, nestedUnion, nestedEnum) == std::tie(rhs.num, rhs.str, rhs.nestedUnion, rhs.nestedEnum);
  }
  inline bool operator>(const Data& rhs) const {
    return std::tie(num, str, nestedUnion, nestedEnum) > std::tie(rhs.num, rhs.str, rhs.nestedUnion, rhs.nestedEnum);
  }
  inline bool operator>=(const Data& rhs) const {
    return std::tie(num, str, nestedUnion, nestedEnum) >= std::tie(rhs.num, rhs.str, rhs.nestedUnion, rhs.nestedEnum);
  }

  int32_t num = int32_t(0);
  ::std::string str;
  ::android::aidl::loggable::Union nestedUnion;
  ::android::aidl::loggable::Enum nestedEnum = ::android::aidl::loggable::Enum(::android::aidl::loggable::Enum::FOO);
  ::android::status_t readFromParcel(const ::android::Parcel* _aidl_parcel) final;
  ::android::status_t writeToParcel(::android::Parcel* _aidl_parcel) const final;
  static const ::android::String16& getParcelableDescriptor() {
    static const ::android::StaticString16 DESCIPTOR (u"android.aidl.loggable.Data");
    return DESCIPTOR;
  }
  inline std::string toString() const {
    std::ostringstream os;
    os << "Data{";
    os << "num: " << ::android::internal::ToString(num);
    os << ", str: " << ::android::internal::ToString(str);
    os << ", nestedUnion: " << ::android::internal::ToString(nestedUnion);
    os << ", nestedEnum: " << ::android::internal::ToString(nestedEnum);
    os << "}";
    return os.str();
  }
};  // class Data

}  // namespace loggable

}  // namespace aidl

}  // namespace android
