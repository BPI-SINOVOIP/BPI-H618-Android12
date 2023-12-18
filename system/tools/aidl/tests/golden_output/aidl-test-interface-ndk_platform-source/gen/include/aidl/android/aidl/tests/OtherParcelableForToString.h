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
namespace aidl {
namespace android {
namespace aidl {
namespace tests {
class OtherParcelableForToString {
public:
  typedef std::false_type fixed_size;
  static const char* descriptor;

  std::string field;

  binder_status_t readFromParcel(const AParcel* parcel);
  binder_status_t writeToParcel(AParcel* parcel) const;

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

  static const ::ndk::parcelable_stability_t _aidl_stability = ::ndk::STABILITY_LOCAL;
  inline std::string toString() const {
    std::ostringstream os;
    os << "OtherParcelableForToString{";
    os << "field: " << ::android::internal::ToString(field);
    os << "}";
    return os.str();
  }
};
}  // namespace tests
}  // namespace aidl
}  // namespace android
}  // namespace aidl
