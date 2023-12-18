#include "aidl/android/aidl/tests/unions/EnumUnion.h"

#include <android/binder_parcel_utils.h>

namespace aidl {
namespace android {
namespace aidl {
namespace tests {
namespace unions {
const char* EnumUnion::descriptor = "android.aidl.tests.unions.EnumUnion";

binder_status_t EnumUnion::readFromParcel(const AParcel* _parcel) {
  binder_status_t _aidl_ret_status;
  int32_t _aidl_tag;
  if ((_aidl_ret_status = AParcel_readInt32(_parcel, &_aidl_tag)) != STATUS_OK) return _aidl_ret_status;
  switch (_aidl_tag) {
  case intEnum: {
    ::aidl::android::aidl::tests::IntEnum _aidl_value;
    if ((_aidl_ret_status = AParcel_readInt32(_parcel, reinterpret_cast<int32_t*>(&_aidl_value))) != STATUS_OK) return _aidl_ret_status;
    if constexpr (std::is_trivially_copyable_v<::aidl::android::aidl::tests::IntEnum>) {
      set<intEnum>(_aidl_value);
    } else {
      // NOLINTNEXTLINE(performance-move-const-arg)
      set<intEnum>(std::move(_aidl_value));
    }
    return STATUS_OK; }
  case longEnum: {
    ::aidl::android::aidl::tests::LongEnum _aidl_value;
    if ((_aidl_ret_status = AParcel_readInt64(_parcel, reinterpret_cast<int64_t*>(&_aidl_value))) != STATUS_OK) return _aidl_ret_status;
    if constexpr (std::is_trivially_copyable_v<::aidl::android::aidl::tests::LongEnum>) {
      set<longEnum>(_aidl_value);
    } else {
      // NOLINTNEXTLINE(performance-move-const-arg)
      set<longEnum>(std::move(_aidl_value));
    }
    return STATUS_OK; }
  }
  return STATUS_BAD_VALUE;
}
binder_status_t EnumUnion::writeToParcel(AParcel* _parcel) const {
  binder_status_t _aidl_ret_status = AParcel_writeInt32(_parcel, getTag());
  if (_aidl_ret_status != STATUS_OK) return _aidl_ret_status;
  switch (getTag()) {
  case intEnum: return AParcel_writeInt32(_parcel, static_cast<int32_t>(get<intEnum>()));
  case longEnum: return AParcel_writeInt64(_parcel, static_cast<int64_t>(get<longEnum>()));
  }
  __assert2(__FILE__, __LINE__, __PRETTY_FUNCTION__, "can't reach here");
}

}  // namespace unions
}  // namespace tests
}  // namespace aidl
}  // namespace android
}  // namespace aidl
