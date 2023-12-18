#pragma once

#include <android/binder_to_string.h>
#include <binder/Parcel.h>
#include <binder/Status.h>
#include <cstdint>
#include <tuple>
#include <utils/String16.h>

namespace android {

namespace aidl {

namespace tests {

template <typename T, typename U, typename B>
class GenericStructuredParcelable : public ::android::Parcelable {
public:
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

  int32_t a = int32_t(0);
  int32_t b = int32_t(0);
  ::android::status_t readFromParcel(const ::android::Parcel* _aidl_parcel) final;
  ::android::status_t writeToParcel(::android::Parcel* _aidl_parcel) const final;
  static const ::android::String16& getParcelableDescriptor() {
    static const ::android::StaticString16 DESCIPTOR (u"android.aidl.tests.GenericStructuredParcelable");
    return DESCIPTOR;
  }
  inline std::string toString() const {
    std::ostringstream os;
    os << "GenericStructuredParcelable{";
    os << "a: " << ::android::internal::ToString(a);
    os << ", b: " << ::android::internal::ToString(b);
    os << "}";
    return os.str();
  }
};  // class GenericStructuredParcelable

}  // namespace tests

}  // namespace aidl

}  // namespace android
#include <android/aidl/tests/GenericStructuredParcelable.h>

namespace android {

namespace aidl {

namespace tests {

template <typename T, typename U, typename B>
::android::status_t GenericStructuredParcelable<T,U,B>::readFromParcel(const ::android::Parcel* _aidl_parcel) {
  ::android::status_t _aidl_ret_status = ::android::OK;
  [[maybe_unused]] size_t _aidl_start_pos = _aidl_parcel->dataPosition();
  int32_t _aidl_parcelable_raw_size = _aidl_parcel->readInt32();
  if (_aidl_parcelable_raw_size < 0) return ::android::BAD_VALUE;
  [[maybe_unused]] size_t _aidl_parcelable_size = static_cast<size_t>(_aidl_parcelable_raw_size);
  if (_aidl_start_pos > SIZE_MAX - _aidl_parcelable_size) return ::android::BAD_VALUE;
  if (_aidl_parcel->dataPosition() - _aidl_start_pos >= _aidl_parcelable_size) {
    _aidl_parcel->setDataPosition(_aidl_start_pos + _aidl_parcelable_size);
    return _aidl_ret_status;
  }
  _aidl_ret_status = _aidl_parcel->readInt32(&a);
  if (((_aidl_ret_status) != (::android::OK))) {
    return _aidl_ret_status;
  }
  if (_aidl_parcel->dataPosition() - _aidl_start_pos >= _aidl_parcelable_size) {
    _aidl_parcel->setDataPosition(_aidl_start_pos + _aidl_parcelable_size);
    return _aidl_ret_status;
  }
  _aidl_ret_status = _aidl_parcel->readInt32(&b);
  if (((_aidl_ret_status) != (::android::OK))) {
    return _aidl_ret_status;
  }
  _aidl_parcel->setDataPosition(_aidl_start_pos + _aidl_parcelable_size);
  return _aidl_ret_status;
}

template <typename T, typename U, typename B>
::android::status_t GenericStructuredParcelable<T,U,B>::writeToParcel(::android::Parcel* _aidl_parcel) const {
  ::android::status_t _aidl_ret_status = ::android::OK;
  auto _aidl_start_pos = _aidl_parcel->dataPosition();
  _aidl_parcel->writeInt32(0);
  _aidl_ret_status = _aidl_parcel->writeInt32(a);
  if (((_aidl_ret_status) != (::android::OK))) {
    return _aidl_ret_status;
  }
  _aidl_ret_status = _aidl_parcel->writeInt32(b);
  if (((_aidl_ret_status) != (::android::OK))) {
    return _aidl_ret_status;
  }
  auto _aidl_end_pos = _aidl_parcel->dataPosition();
  _aidl_parcel->setDataPosition(_aidl_start_pos);
  _aidl_parcel->writeInt32(_aidl_end_pos - _aidl_start_pos);
  _aidl_parcel->setDataPosition(_aidl_end_pos);
  return _aidl_ret_status;
}

}  // namespace tests

}  // namespace aidl

}  // namespace android
