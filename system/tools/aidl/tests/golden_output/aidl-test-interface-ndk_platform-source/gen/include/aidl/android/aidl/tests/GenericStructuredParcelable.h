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
template <typename T, typename U, typename B>
class GenericStructuredParcelable {
public:
  typedef std::false_type fixed_size;
  static const char* descriptor;

  int32_t a = 0;
  int32_t b = 0;

  binder_status_t readFromParcel(const AParcel* parcel);
  binder_status_t writeToParcel(AParcel* parcel) const;

  inline bool operator!=(const GenericStructuredParcelable& rhs) const {
    return std::tie(a, b) != std::tie(rhs.a, rhs.b);
  }
  inline bool operator<(const GenericStructuredParcelable& rhs) const {
    return std::tie(a, b) < std::tie(rhs.a, rhs.b);
  }
  inline bool operator<=(const GenericStructuredParcelable& rhs) const {
    return std::tie(a, b) <= std::tie(rhs.a, rhs.b);
  }
  inline bool operator==(const GenericStructuredParcelable& rhs) const {
    return std::tie(a, b) == std::tie(rhs.a, rhs.b);
  }
  inline bool operator>(const GenericStructuredParcelable& rhs) const {
    return std::tie(a, b) > std::tie(rhs.a, rhs.b);
  }
  inline bool operator>=(const GenericStructuredParcelable& rhs) const {
    return std::tie(a, b) >= std::tie(rhs.a, rhs.b);
  }

  static const ::ndk::parcelable_stability_t _aidl_stability = ::ndk::STABILITY_LOCAL;
  inline std::string toString() const {
    std::ostringstream os;
    os << "GenericStructuredParcelable{";
    os << "a: " << ::android::internal::ToString(a);
    os << ", b: " << ::android::internal::ToString(b);
    os << "}";
    return os.str();
  }
};
}  // namespace tests
}  // namespace aidl
}  // namespace android
}  // namespace aidl
#include "aidl/android/aidl/tests/GenericStructuredParcelable.h"

#include <android/binder_parcel_utils.h>

namespace aidl {
namespace android {
namespace aidl {
namespace tests {
template <typename T, typename U, typename B>
const char* GenericStructuredParcelable<T, U, B>::descriptor = "android.aidl.tests.GenericStructuredParcelable";

template <typename T, typename U, typename B>
binder_status_t GenericStructuredParcelable<T, U, B>::readFromParcel(const AParcel* parcel) {
  int32_t _aidl_parcelable_size;
  int32_t _aidl_start_pos = AParcel_getDataPosition(parcel);
  binder_status_t _aidl_ret_status = AParcel_readInt32(parcel, &_aidl_parcelable_size);
  if (_aidl_start_pos > INT32_MAX - _aidl_parcelable_size) return STATUS_BAD_VALUE;
  if (_aidl_parcelable_size < 0) return STATUS_BAD_VALUE;
  if (_aidl_ret_status != STATUS_OK) return _aidl_ret_status;

  if (AParcel_getDataPosition(parcel) - _aidl_start_pos >= _aidl_parcelable_size) {
    AParcel_setDataPosition(parcel, _aidl_start_pos + _aidl_parcelable_size);
    return _aidl_ret_status;
  }
  _aidl_ret_status = AParcel_readInt32(parcel, &a);
  if (_aidl_ret_status != STATUS_OK) return _aidl_ret_status;

  if (AParcel_getDataPosition(parcel) - _aidl_start_pos >= _aidl_parcelable_size) {
    AParcel_setDataPosition(parcel, _aidl_start_pos + _aidl_parcelable_size);
    return _aidl_ret_status;
  }
  _aidl_ret_status = AParcel_readInt32(parcel, &b);
  if (_aidl_ret_status != STATUS_OK) return _aidl_ret_status;

  AParcel_setDataPosition(parcel, _aidl_start_pos + _aidl_parcelable_size);
  return _aidl_ret_status;
}
template <typename T, typename U, typename B>
binder_status_t GenericStructuredParcelable<T, U, B>::writeToParcel(AParcel* parcel) const {
  binder_status_t _aidl_ret_status;
  size_t _aidl_start_pos = AParcel_getDataPosition(parcel);
  _aidl_ret_status = AParcel_writeInt32(parcel, 0);
  if (_aidl_ret_status != STATUS_OK) return _aidl_ret_status;

  _aidl_ret_status = AParcel_writeInt32(parcel, a);
  if (_aidl_ret_status != STATUS_OK) return _aidl_ret_status;

  _aidl_ret_status = AParcel_writeInt32(parcel, b);
  if (_aidl_ret_status != STATUS_OK) return _aidl_ret_status;

  size_t _aidl_end_pos = AParcel_getDataPosition(parcel);
  AParcel_setDataPosition(parcel, _aidl_start_pos);
  AParcel_writeInt32(parcel, _aidl_end_pos - _aidl_start_pos);
  AParcel_setDataPosition(parcel, _aidl_end_pos);
  return _aidl_ret_status;
}

}  // namespace tests
}  // namespace aidl
}  // namespace android
}  // namespace aidl
