#pragma once

#include <array>
#include <binder/Enums.h>
#include <cstdint>
#include <string>

namespace android {

namespace aidl {

namespace tests {

enum class __attribute__((deprecated("test"))) DeprecatedEnum : int32_t {
  A = 0,
  B = 1,
  C = 2,
};

[[nodiscard]] __attribute__((deprecated("test"))) static inline std::string toString(DeprecatedEnum val) {
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
namespace android {

namespace internal {

#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wc++17-extensions"
template <>
constexpr inline std::array<::android::aidl::tests::DeprecatedEnum, 3> __attribute__((deprecated("test"))) enum_values<::android::aidl::tests::DeprecatedEnum> = {
  ::android::aidl::tests::DeprecatedEnum::A,
  ::android::aidl::tests::DeprecatedEnum::B,
  ::android::aidl::tests::DeprecatedEnum::C,
};
#pragma clang diagnostic pop

}  // namespace internal

}  // namespace android
