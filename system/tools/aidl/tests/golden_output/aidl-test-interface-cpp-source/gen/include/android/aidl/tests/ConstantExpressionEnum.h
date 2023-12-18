#pragma once

#include <array>
#include <binder/Enums.h>
#include <cstdint>
#include <string>

namespace android {

namespace aidl {

namespace tests {

enum class ConstantExpressionEnum : int32_t {
  decInt32_1 = 1,
  decInt32_2 = 1,
  decInt64_1 = 1,
  decInt64_2 = 1,
  decInt64_3 = 1,
  decInt64_4 = 1,
  hexInt32_1 = 1,
  hexInt32_2 = 1,
  hexInt32_3 = 1,
  hexInt64_1 = 1,
};

[[nodiscard]] static inline std::string toString(ConstantExpressionEnum val) {
  switch(val) {
  case ConstantExpressionEnum::decInt32_1:
    return "decInt32_1";
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
constexpr inline std::array<::android::aidl::tests::ConstantExpressionEnum, 10> enum_values<::android::aidl::tests::ConstantExpressionEnum> = {
  ::android::aidl::tests::ConstantExpressionEnum::decInt32_1,
  ::android::aidl::tests::ConstantExpressionEnum::decInt32_2,
  ::android::aidl::tests::ConstantExpressionEnum::decInt64_1,
  ::android::aidl::tests::ConstantExpressionEnum::decInt64_2,
  ::android::aidl::tests::ConstantExpressionEnum::decInt64_3,
  ::android::aidl::tests::ConstantExpressionEnum::decInt64_4,
  ::android::aidl::tests::ConstantExpressionEnum::hexInt32_1,
  ::android::aidl::tests::ConstantExpressionEnum::hexInt32_2,
  ::android::aidl::tests::ConstantExpressionEnum::hexInt32_3,
  ::android::aidl::tests::ConstantExpressionEnum::hexInt64_1,
};
#pragma clang diagnostic pop

}  // namespace internal

}  // namespace android
