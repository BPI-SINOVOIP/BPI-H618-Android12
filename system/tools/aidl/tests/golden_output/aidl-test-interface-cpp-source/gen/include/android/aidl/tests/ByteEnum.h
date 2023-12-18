#pragma once

#include <array>
#include <binder/Enums.h>
#include <cstdint>
#include <string>

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
namespace android {

namespace internal {

#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wc++17-extensions"
template <>
constexpr inline std::array<::android::aidl::tests::ByteEnum, 3> enum_values<::android::aidl::tests::ByteEnum> = {
  ::android::aidl::tests::ByteEnum::FOO,
  ::android::aidl::tests::ByteEnum::BAR,
  ::android::aidl::tests::ByteEnum::BAZ,
};
#pragma clang diagnostic pop

}  // namespace internal

}  // namespace android
