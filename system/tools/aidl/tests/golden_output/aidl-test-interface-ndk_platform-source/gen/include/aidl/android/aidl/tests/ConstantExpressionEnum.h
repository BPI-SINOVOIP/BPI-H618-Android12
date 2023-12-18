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
}  // namespace aidl
namespace ndk {
namespace internal {
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wc++17-extensions"
template <>
constexpr inline std::array<aidl::android::aidl::tests::ConstantExpressionEnum, 10> enum_values<aidl::android::aidl::tests::ConstantExpressionEnum> = {
  aidl::android::aidl::tests::ConstantExpressionEnum::decInt32_1,
  aidl::android::aidl::tests::ConstantExpressionEnum::decInt32_2,
  aidl::android::aidl::tests::ConstantExpressionEnum::decInt64_1,
  aidl::android::aidl::tests::ConstantExpressionEnum::decInt64_2,
  aidl::android::aidl::tests::ConstantExpressionEnum::decInt64_3,
  aidl::android::aidl::tests::ConstantExpressionEnum::decInt64_4,
  aidl::android::aidl::tests::ConstantExpressionEnum::hexInt32_1,
  aidl::android::aidl::tests::ConstantExpressionEnum::hexInt32_2,
  aidl::android::aidl::tests::ConstantExpressionEnum::hexInt32_3,
  aidl::android::aidl::tests::ConstantExpressionEnum::hexInt64_1,
};
#pragma clang diagnostic pop
}  // namespace internal
}  // namespace android
