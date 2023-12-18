#pragma once

#include <array>
#include <binder/Enums.h>
#include <cstdint>
#include <string>

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
namespace android {

namespace internal {

#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wc++17-extensions"
template <>
constexpr inline std::array<::android::aidl::loggable::Enum, 1> enum_values<::android::aidl::loggable::Enum> = {
  ::android::aidl::loggable::Enum::FOO,
};
#pragma clang diagnostic pop

}  // namespace internal

}  // namespace android
