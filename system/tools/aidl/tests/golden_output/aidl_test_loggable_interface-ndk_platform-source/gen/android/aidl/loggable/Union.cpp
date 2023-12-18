#include "aidl/android/aidl/loggable/Union.h"

#include <android/binder_parcel_utils.h>

namespace aidl {
namespace android {
namespace aidl {
namespace loggable {
const char* Union::descriptor = "android.aidl.loggable.Union";

binder_status_t Union::readFromParcel(const AParcel* _parcel) {
  binder_status_t _aidl_ret_status;
  int32_t _aidl_tag;
  if ((_aidl_ret_status = AParcel_readInt32(_parcel, &_aidl_tag)) != STATUS_OK) return _aidl_ret_status;
  switch (_aidl_tag) {
  case num: {
    int32_t _aidl_value;
    if ((_aidl_ret_status = AParcel_readInt32(_parcel, &_aidl_value)) != STATUS_OK) return _aidl_ret_status;
    if constexpr (std::is_trivially_copyable_v<int32_t>) {
      set<num>(_aidl_value);
    } else {
      // NOLINTNEXTLINE(performance-move-const-arg)
      set<num>(std::move(_aidl_value));
    }
    return STATUS_OK; }
  case str: {
    std::string _aidl_value;
    if ((_aidl_ret_status = ::ndk::AParcel_readString(_parcel, &_aidl_value)) != STATUS_OK) return _aidl_ret_status;
    if constexpr (std::is_trivially_copyable_v<std::string>) {
      set<str>(_aidl_value);
    } else {
      // NOLINTNEXTLINE(performance-move-const-arg)
      set<str>(std::move(_aidl_value));
    }
    return STATUS_OK; }
  }
  return STATUS_BAD_VALUE;
}
binder_status_t Union::writeToParcel(AParcel* _parcel) const {
  binder_status_t _aidl_ret_status = AParcel_writeInt32(_parcel, getTag());
  if (_aidl_ret_status != STATUS_OK) return _aidl_ret_status;
  switch (getTag()) {
  case num: return AParcel_writeInt32(_parcel, get<num>());
  case str: return ::ndk::AParcel_writeString(_parcel, get<str>());
  }
  __assert2(__FILE__, __LINE__, __PRETTY_FUNCTION__, "can't reach here");
}

}  // namespace loggable
}  // namespace aidl
}  // namespace android
}  // namespace aidl
