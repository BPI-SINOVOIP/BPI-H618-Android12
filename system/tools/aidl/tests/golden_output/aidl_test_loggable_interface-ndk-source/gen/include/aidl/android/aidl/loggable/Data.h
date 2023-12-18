#pragma once
#include <android/binder_interface_utils.h>
#include <android/binder_parcelable_utils.h>
#include <android/binder_to_string.h>
#include <cstdint>
#include <memory>
#include <optional>
#include <string>
#include <vector>
#ifdef BINDER_STABILITY_SUPPORT
#include <android/binder_stability.h>
#endif  // BINDER_STABILITY_SUPPORT
#include <aidl/android/aidl/loggable/Enum.h>
#include <aidl/android/aidl/loggable/Union.h>
namespace aidl {
namespace android {
namespace aidl {
namespace loggable {
class Data {
public:
  typedef std::false_type fixed_size;
  static const char* descriptor;

  int32_t num = 0;
  std::string str;
  ::aidl::android::aidl::loggable::Union nestedUnion;
  ::aidl::android::aidl::loggable::Enum nestedEnum = ::aidl::android::aidl::loggable::Enum::FOO;

  binder_status_t readFromParcel(const AParcel* parcel);
  binder_status_t writeToParcel(AParcel* parcel) const;

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

  static const ::ndk::parcelable_stability_t _aidl_stability = ::ndk::STABILITY_LOCAL;
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
};
}  // namespace loggable
}  // namespace aidl
}  // namespace android
}  // namespace aidl
