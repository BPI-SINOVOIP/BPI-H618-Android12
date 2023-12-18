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
enum class __attribute__((deprecated("test"))) DeprecatedEnum : int32_t {
  A = 0,
  B = 1,
  C = 2,
};

[[nodiscard]] static inline std::string toString(DeprecatedEnum val) __attribute__((deprecated("test")));
[[nodiscard]] static inline std::string toString(DeprecatedEnum val) {
  switch(val) {
  case DeprecatedEnum::A:
    return "A";
  case DeprecatedEnum::B:
    return "B";
  case DeprecatedEnum::C:
    return "C";
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
constexpr inline std::array<aidl::android::aidl::tests::DeprecatedEnum, 3> __attribute__((deprecated("test"))) enum_values<aidl::android::aidl::tests::DeprecatedEnum> = {
  aidl::android::aidl::tests::DeprecatedEnum::A,
  aidl::android::aidl::tests::DeprecatedEnum::B,
  aidl::android::aidl::tests::DeprecatedEnum::C,
};
#pragma clang diagnostic pop
}  // namespace internal
}  // namespace android
