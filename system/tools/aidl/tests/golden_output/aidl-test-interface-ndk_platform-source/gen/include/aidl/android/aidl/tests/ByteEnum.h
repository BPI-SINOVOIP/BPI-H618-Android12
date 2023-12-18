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
enum class ByteEnum : int8_t {
  FOO = 1,
  BAR = 2,
  BAZ = 3,
};

[[nodiscard]] static inline std::string toString(ByteEnum val) {
  switch(val) {
  case ByteEnum::FOO:
    return "FOO";
  case ByteEnum::BAR:
    return "BAR";
  case ByteEnum::BAZ:
    return "BAZ";
  default:
    return std::to_string(static_cast<int8_t>(val));
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
constexpr inline std::array<aidl::android::aidl::tests::ByteEnum, 3> enum_values<aidl::android::aidl::tests::ByteEnum> = {
  aidl::android::aidl::tests::ByteEnum::FOO,
  aidl::android::aidl::tests::ByteEnum::BAR,
  aidl::android::aidl::tests::ByteEnum::BAZ,
};
#pragma clang diagnostic pop
}  // namespace internal
}  // namespace android
