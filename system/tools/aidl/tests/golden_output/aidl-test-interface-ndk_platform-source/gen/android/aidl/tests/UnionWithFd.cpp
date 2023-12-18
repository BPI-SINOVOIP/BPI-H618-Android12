#include "aidl/android/aidl/tests/UnionWithFd.h"

#include <android/binder_parcel_utils.h>

namespace aidl {
namespace android {
namespace aidl {
namespace tests {
const char* UnionWithFd::descriptor = "android.aidl.tests.UnionWithFd";

binder_status_t UnionWithFd::readFromParcel(const AParcel* _parcel) {
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
  case pfd: {
    ::ndk::ScopedFileDescriptor _aidl_value;
    if ((_aidl_ret_status = ::ndk::AParcel_readRequiredParcelFileDescriptor(_parcel, &_aidl_value)) != STATUS_OK) return _aidl_ret_status;
    if constexpr (std::is_trivially_copyable_v<::ndk::ScopedFileDescriptor>) {
      set<pfd>(_aidl_value);
    } else {
      // NOLINTNEXTLINE(performance-move-const-arg)
      set<pfd>(std::move(_aidl_value));
    }
    return STATUS_OK; }
  }
  return STATUS_BAD_VALUE;
}
binder_status_t UnionWithFd::writeToParcel(AParcel* _parcel) const {
  binder_status_t _aidl_ret_status = AParcel_writeInt32(_parcel, getTag());
  if (_aidl_ret_status != STATUS_OK) return _aidl_ret_status;
  switch (getTag()) {
  case num: return AParcel_writeInt32(_parcel, get<num>());
  case pfd: return ::ndk::AParcel_writeRequiredParcelFileDescriptor(_parcel, get<pfd>());
  }
  __assert2(__FILE__, __LINE__, __PRETTY_FUNCTION__, "can't reach here");
}

}  // namespace tests
}  // namespace aidl
}  // namespace android
}  // namespace aidl
