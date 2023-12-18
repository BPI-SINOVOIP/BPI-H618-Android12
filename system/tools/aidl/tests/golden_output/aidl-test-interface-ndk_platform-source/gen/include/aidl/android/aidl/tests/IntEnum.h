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
enum class IntEnum : int32_t {
  FOO = 1000,
  BAR = 2000,
  BAZ = 2001,
};

[[nodiscard]] static inline std::string toString(IntEnum val) {
  switch(val) {
  case IntEnum::FOO:
    return "FOO";
  case IntEnum::BAR:
    return "BAR";
  case IntEnum::BAZ:
    return "BAZ";
  default:
    return std::to_string(static_cast<int32_t>(val));
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
constexpr inline std::array<aidl::android::aidl::tests::IntEnum, 3> enum_values<aidl::android::aidl::tests::IntEnum> = {
  aidl::android::aidl::tests::IntEnum::FOO,
  aidl::android::aidl::tests::IntEnum::BAR,
  aidl::android::aidl::tests::IntEnum::BAZ,
};
#pragma clang diagnostic pop
}  // namespace internal
}  // namespace android
