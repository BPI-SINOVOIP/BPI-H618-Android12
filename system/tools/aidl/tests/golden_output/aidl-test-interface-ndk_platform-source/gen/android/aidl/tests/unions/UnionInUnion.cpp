#include "aidl/android/aidl/tests/unions/UnionInUnion.h"

#include <android/binder_parcel_utils.h>

namespace aidl {
namespace android {
namespace aidl {
namespace tests {
namespace unions {
const char* UnionInUnion::descriptor = "android.aidl.tests.unions.UnionInUnion";

binder_status_t UnionInUnion::readFromParcel(const AParcel* _parcel) {
  binder_status_t _aidl_ret_status;
  int32_t _aidl_tag;
  if ((_aidl_ret_status = AParcel_readInt32(_parcel, &_aidl_tag)) != STATUS_OK) return _aidl_ret_status;
  switch (_aidl_tag) {
  case first: {
    ::aidl::android::aidl::tests::unions::EnumUnion _aidl_value;
    if ((_aidl_ret_status = ::ndk::AParcel_readParcelable(_parcel, &_aidl_value)) != STATUS_OK) return _aidl_ret_status;
    if constexpr (std::is_trivially_copyable_v<::aidl::android::aidl::tests::unions::EnumUnion>) {
      set<first>(_aidl_value);
    } else {
      // NOLINTNEXTLINE(performance-move-const-arg)
      set<first>(std::move(_aidl_value));
    }
    return STATUS_OK; }
  case second: {
    int32_t _aidl_value;
    if ((_aidl_ret_status = AParcel_readInt32(_parcel, &_aidl_value)) != STATUS_OK) return _aidl_ret_status;
    if constexpr (std::is_trivially_copyable_v<int32_t>) {
      set<second>(_aidl_value);
    } else {
      // NOLINTNEXTLINE(performance-move-const-arg)
      set<second>(std::move(_aidl_value));
    }
    return STATUS_OK; }
  }
  return STATUS_BAD_VALUE;
}
binder_status_t UnionInUnion::writeToParcel(AParcel* _parcel) const {
  binder_status_t _aidl_ret_status = AParcel_writeInt32(_parcel, getTag());
  if (_aidl_ret_status != STATUS_OK) return _aidl_ret_status;
  switch (getTag()) {
  case first: return ::ndk::AParcel_writeParcelable(_parcel, get<first>());
  case second: return AParcel_writeInt32(_parcel, get<second>());
  }
  __assert2(__FILE__, __LINE__, __PRETTY_FUNCTION__, "can't reach here");
}

}  // namespace unions
}  // namespace tests
}  // namespace aidl
}  // namespace android
}  // namespace aidl
