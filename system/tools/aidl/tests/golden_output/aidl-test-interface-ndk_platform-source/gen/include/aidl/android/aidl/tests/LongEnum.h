#pragma once

#include <cstdint>
#include <memory>
#include <optional>
#include <string>
#include <vector>
#ifdef BINDER_STABILITY_SUPPORT
#include <android/binder_stability.h>
#endif  // BINDER_STABILITY_SUPPORT
#include <array>
#include <android/binder_enums.h>
namespace aidl {
namespace android {
namespace aidl {
namespace tests {
enum class LongEnum : int64_t {
  FOO = 100000000000L,
  BAR = 200000000000L,
  BAZ = 200000000001L,
};

[[nodiscard]] static inline std::string toString(LongEnum val) {
  switch(val) {
  case LongEnum::FOO:
    return "FOO";
  case LongEnum::BAR:
    return "BAR";
  case LongEnum::BAZ:
    return "BAZ";
  default:
    return std::to_string(static_cast<int64_t>(val));
  }
}
}  // namespace tests
}  // namespace aidl
}  // namespace android
}  // namespace aidl
namespace ndk {
namespace internal {
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wc++17-extensions"
template <>
constexpr inline std::array<aidl::android::aidl::tests::LongEnum, 3> enum_values<aidl::android::aidl::tests::LongEnum> = {
  aidl::android::aidl::tests::LongEnum::FOO,
  aidl::android::aidl::tests::LongEnum::BAR,
  aidl::android::aidl::tests::LongEnum::BAZ,
};
#pragma clang diagnostic pop
}  // namespace internal
}  // namespace android
