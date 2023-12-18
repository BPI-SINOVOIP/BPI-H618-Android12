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
namespace loggable {
enum class Enum : int8_t {
  FOO = 42,
};

[[nodiscard]] static inline std::string toString(Enum val) {
  switch(val) {
  case Enum::FOO:
    return "FOO";
  default:
    return std::to_string(static_cast<int8_t>(val));
  }
}
}  // namespace loggable
}  // namespace aidl
}  // namespace android
}  // namespace aidl
namespace ndk {
namespace internal {
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wc++17-extensions"
template <>
constexpr inline std::array<aidl::android::aidl::loggable::Enum, 1> enum_values<aidl::android::aidl::loggable::Enum> = {
  aidl::android::aidl::loggable::Enum::FOO,
};
#pragma clang diagnostic pop
}  // namespace internal
}  // namespace android
