#pragma once
#include <android/binder_interface_utils.h>
#include <android/binder_parcelable_utils.h>
#include <android/binder_to_string.h>
#include <cstdint>
#include <memory>
#include <optional>
#include <string>
#include <vector>
#ifdef BINDER_STABILITY_SUPPORT
#include <android/binder_stability.h>
#endif  // BINDER_STABILITY_SUPPORT
#include <aidl/android/aidl/tests/ByteEnum.h>
#include <aidl/android/aidl/tests/ConstantExpressionEnum.h>
#include <aidl/android/aidl/tests/IntEnum.h>
#include <aidl/android/aidl/tests/LongEnum.h>
#include <aidl/android/aidl/tests/Union.h>
namespace aidl {
namespace android {
namespace aidl {
namespace tests {
class StructuredParcelable {
public:
  typedef std::false_type fixed_size;
  static const char* descriptor;

  std::vector<int32_t> shouldContainThreeFs;
  int32_t f = 0;
  std::string shouldBeJerry;
  ::aidl::android::aidl::tests::ByteEnum shouldBeByteBar = ::aidl::android::aidl::tests::ByteEnum(0);
  ::aidl::android::aidl::tests::IntEnum shouldBeIntBar = ::aidl::android::aidl::tests::IntEnum(0);
  ::aidl::android::aidl::tests::LongEnum shouldBeLongBar = ::aidl::android::aidl::tests::LongEnum(0);
  std::vector<::aidl::android::aidl::tests::ByteEnum> shouldContainTwoByteFoos;
  std::vector<::aidl::android::aidl::tests::IntEnum> shouldContainTwoIntFoos;
  std::vector<::aidl::android::aidl::tests::LongEnum> shouldContainTwoLongFoos;
  std::string stringDefaultsToFoo = "foo";
  int8_t byteDefaultsToFour = 4;
  int32_t intDefaultsToFive = 5;
  int64_t longDefaultsToNegativeSeven = -7L;
  bool booleanDefaultsToTrue = true;
  char16_t charDefaultsToC = 'C';
  float floatDefaultsToPi = 3.140000f;
  double doubleWithDefault = -314000000000000000.000000;
  std::vector<int32_t> arrayDefaultsTo123 = {1, 2, 3};
  std::vector<int32_t> arrayDefaultsToEmpty = {};
  bool boolDefault = false;
  int8_t byteDefault = 0;
  int32_t intDefault = 0;
  int64_t longDefault = 0L;
  float floatDefault = 0.000000f;
  double doubleDefault = 0.000000;
  double checkDoubleFromFloat = 3.140000;
  std::vector<std::string> checkStringArray1 = {"a", "b"};
  std::vector<std::string> checkStringArray2 = {"a", "b"};
  int32_t int32_min = -2147483648;
  int32_t int32_max = 2147483647;
  int64_t int64_max = 9223372036854775807L;
  int32_t hexInt32_neg_1 = -1;
  ::ndk::SpAIBinder ibinder;
  std::vector<int32_t> int32_1 = {1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1};
  std::vector<int64_t> int64_1 = {1L, 1L, 1L, 1L, 1L, 1L, 1L, 1L, 1L, 1L};
  int32_t hexInt32_pos_1 = 1;
  int32_t hexInt64_pos_1 = 1;
  ::aidl::android::aidl::tests::ConstantExpressionEnum const_exprs_1 = ::aidl::android::aidl::tests::ConstantExpressionEnum(0);
  ::aidl::android::aidl::tests::ConstantExpressionEnum const_exprs_2 = ::aidl::android::aidl::tests::ConstantExpressionEnum(0);
  ::aidl::android::aidl::tests::ConstantExpressionEnum const_exprs_3 = ::aidl::android::aidl::tests::ConstantExpressionEnum(0);
  ::aidl::android::aidl::tests::ConstantExpressionEnum const_exprs_4 = ::aidl::android::aidl::tests::ConstantExpressionEnum(0);
  ::aidl::android::aidl::tests::ConstantExpressionEnum const_exprs_5 = ::aidl::android::aidl::tests::ConstantExpressionEnum(0);
  ::aidl::android::aidl::tests::ConstantExpressionEnum const_exprs_6 = ::aidl::android::aidl::tests::ConstantExpressionEnum(0);
  ::aidl::android::aidl::tests::ConstantExpressionEnum const_exprs_7 = ::aidl::android::aidl::tests::ConstantExpressionEnum(0);
  ::aidl::android::aidl::tests::ConstantExpressionEnum const_exprs_8 = ::aidl::android::aidl::tests::ConstantExpressionEnum(0);
  ::aidl::android::aidl::tests::ConstantExpressionEnum const_exprs_9 = ::aidl::android::aidl::tests::ConstantExpressionEnum(0);
  ::aidl::android::aidl::tests::ConstantExpressionEnum const_exprs_10 = ::aidl::android::aidl::tests::ConstantExpressionEnum(0);
  std::string addString1 = "hello world!";
  std::string addString2 = "The quick brown fox jumps over the lazy dog.";
  int32_t shouldSetBit0AndBit2 = 0;
  std::optional<::aidl::android::aidl::tests::Union> u;
  std::optional<::aidl::android::aidl::tests::Union> shouldBeConstS1;
  ::aidl::android::aidl::tests::IntEnum defaultWithFoo = ::aidl::android::aidl::tests::IntEnum::FOO;

  binder_status_t readFromParcel(const AParcel* parcel);
  binder_status_t writeToParcel(AParcel* parcel) const;

  inline bool operator!=(const StructuredParcelable& rhs) const {
    return std::tie(shouldContainThreeFs, f, shouldBeJerry, shouldBeByteBar, shouldBeIntBar, shouldBeLongBar, shouldContainTwoByteFoos, shouldContainTwoIntFoos, shouldContainTwoLongFoos, stringDefaultsToFoo, byteDefaultsToFour, intDefaultsToFive, longDefaultsToNegativeSeven, booleanDefaultsToTrue, charDefaultsToC, floatDefaultsToPi, doubleWithDefault, arrayDefaultsTo123, arrayDefaultsToEmpty, boolDefault, byteDefault, intDefault, longDefault, floatDefault, doubleDefault, checkDoubleFromFloat, checkStringArray1, checkStringArray2, int32_min, int32_max, int64_max, hexInt32_neg_1, ibinder, int32_1, int64_1, hexInt32_pos_1, hexInt64_pos_1, const_exprs_1, const_exprs_2, const_exprs_3, const_exprs_4, const_exprs_5, const_exprs_6, const_exprs_7, const_exprs_8, const_exprs_9, const_exprs_10, addString1, addString2, shouldSetBit0AndBit2, u, shouldBeConstS1, defaultWithFoo) != std::tie(rhs.shouldContainThreeFs, rhs.f, rhs.shouldBeJerry, rhs.shouldBeByteBar, rhs.shouldBeIntBar, rhs.shouldBeLongBar, rhs.shouldContainTwoByteFoos, rhs.shouldContainTwoIntFoos, rhs.shouldContainTwoLongFoos, rhs.stringDefaultsToFoo, rhs.byteDefaultsToFour, rhs.intDefaultsToFive, rhs.longDefaultsToNegativeSeven, rhs.booleanDefaultsToTrue, rhs.charDefaultsToC, rhs.floatDefaultsToPi, rhs.doubleWithDefault, rhs.arrayDefaultsTo123, rhs.arrayDefaultsToEmpty, rhs.boolDefault, rhs.byteDefault, rhs.intDefault, rhs.longDefault, rhs.floatDefault, rhs.doubleDefault, rhs.checkDoubleFromFloat, rhs.checkStringArray1, rhs.checkStringArray2, rhs.int32_min, rhs.int32_max, rhs.int64_max, rhs.hexInt32_neg_1, rhs.ibinder, rhs.int32_1, rhs.int64_1, rhs.hexInt32_pos_1, rhs.hexInt64_pos_1, rhs.const_exprs_1, rhs.const_exprs_2, rhs.const_exprs_3, rhs.const_exprs_4, rhs.const_exprs_5, rhs.const_exprs_6, rhs.const_exprs_7, rhs.const_exprs_8, rhs.const_exprs_9, rhs.const_exprs_10, rhs.addString1, rhs.addString2, rhs.shouldSetBit0AndBit2, rhs.u, rhs.shouldBeConstS1, rhs.defaultWithFoo);
  }
  inline bool operator<(const StructuredParcelable& rhs) const {
    return std::tie(shouldContainThreeFs, f, shouldBeJerry, shouldBeByteBar, shouldBeIntBar, shouldBeLongBar, shouldContainTwoByteFoos, shouldContainTwoIntFoos, shouldContainTwoLongFoos, stringDefaultsToFoo, byteDefaultsToFour, intDefaultsToFive, longDefaultsToNegativeSeven, booleanDefaultsToTrue, charDefaultsToC, floatDefaultsToPi, doubleWithDefault, arrayDefaultsTo123, arrayDefaultsToEmpty, boolDefault, byteDefault, intDefault, longDefault, floatDefault, doubleDefault, checkDoubleFromFloat, checkStringArray1, checkStringArray2, int32_min, int32_max, int64_max, hexInt32_neg_1, ibinder, int32_1, int64_1, hexInt32_pos_1, hexInt64_pos_1, const_exprs_1, const_exprs_2, const_exprs_3, const_exprs_4, const_exprs_5, const_exprs_6, const_exprs_7, const_exprs_8, const_exprs_9, const_exprs_10, addString1, addString2, shouldSetBit0AndBit2, u, shouldBeConstS1, defaultWithFoo) < std::tie(rhs.shouldContainThreeFs, rhs.f, rhs.shouldBeJerry, rhs.shouldBeByteBar, rhs.shouldBeIntBar, rhs.shouldBeLongBar, rhs.shouldContainTwoByteFoos, rhs.shouldContainTwoIntFoos, rhs.shouldContainTwoLongFoos, rhs.stringDefaultsToFoo, rhs.byteDefaultsToFour, rhs.intDefaultsToFive, rhs.longDefaultsToNegativeSeven, rhs.booleanDefaultsToTrue, rhs.charDefaultsToC, rhs.floatDefaultsToPi, rhs.doubleWithDefault, rhs.arrayDefaultsTo123, rhs.arrayDefaultsToEmpty, rhs.boolDefault, rhs.byteDefault, rhs.intDefault, rhs.longDefault, rhs.floatDefault, rhs.doubleDefault, rhs.checkDoubleFromFloat, rhs.checkStringArray1, rhs.checkStringArray2, rhs.int32_min, rhs.int32_max, rhs.int64_max, rhs.hexInt32_neg_1, rhs.ibinder, rhs.int32_1, rhs.int64_1, rhs.hexInt32_pos_1, rhs.hexInt64_pos_1, rhs.const_exprs_1, rhs.const_exprs_2, rhs.const_exprs_3, rhs.const_exprs_4, rhs.const_exprs_5, rhs.const_exprs_6, rhs.const_exprs_7, rhs.const_exprs_8, rhs.const_exprs_9, rhs.const_exprs_10, rhs.addString1, rhs.addString2, rhs.shouldSetBit0AndBit2, rhs.u, rhs.shouldBeConstS1, rhs.defaultWithFoo);
  }
  inline bool operator<=(const StructuredParcelable& rhs) const {
    return std::tie(shouldContainThreeFs, f, shouldBeJerry, shouldBeByteBar, shouldBeIntBar, shouldBeLongBar, shouldContainTwoByteFoos, shouldContainTwoIntFoos, shouldContainTwoLongFoos, stringDefaultsToFoo, byteDefaultsToFour, intDefaultsToFive, longDefaultsToNegativeSeven, booleanDefaultsToTrue, charDefaultsToC, floatDefaultsToPi, doubleWithDefault, arrayDefaultsTo123, arrayDefaultsToEmpty, boolDefault, byteDefault, intDefault, longDefault, floatDefault, doubleDefault, checkDoubleFromFloat, checkStringArray1, checkStringArray2, int32_min, int32_max, int64_max, hexInt32_neg_1, ibinder, int32_1, int64_1, hexInt32_pos_1, hexInt64_pos_1, const_exprs_1, const_exprs_2, const_exprs_3, const_exprs_4, const_exprs_5, const_exprs_6, const_exprs_7, const_exprs_8, const_exprs_9, const_exprs_10, addString1, addString2, shouldSetBit0AndBit2, u, shouldBeConstS1, defaultWithFoo) <= std::tie(rhs.shouldContainThreeFs, rhs.f, rhs.shouldBeJerry, rhs.shouldBeByteBar, rhs.shouldBeIntBar, rhs.shouldBeLongBar, rhs.shouldContainTwoByteFoos, rhs.shouldContainTwoIntFoos, rhs.shouldContainTwoLongFoos, rhs.stringDefaultsToFoo, rhs.byteDefaultsToFour, rhs.intDefaultsToFive, rhs.longDefaultsToNegativeSeven, rhs.booleanDefaultsToTrue, rhs.charDefaultsToC, rhs.floatDefaultsToPi, rhs.doubleWithDefault, rhs.arrayDefaultsTo123, rhs.arrayDefaultsToEmpty, rhs.boolDefault, rhs.byteDefault, rhs.intDefault, rhs.longDefault, rhs.floatDefault, rhs.doubleDefault, rhs.checkDoubleFromFloat, rhs.checkStringArray1, rhs.checkStringArray2, rhs.int32_min, rhs.int32_max, rhs.int64_max, rhs.hexInt32_neg_1, rhs.ibinder, rhs.int32_1, rhs.int64_1, rhs.hexInt32_pos_1, rhs.hexInt64_pos_1, rhs.const_exprs_1, rhs.const_exprs_2, rhs.const_exprs_3, rhs.const_exprs_4, rhs.const_exprs_5, rhs.const_exprs_6, rhs.const_exprs_7, rhs.const_exprs_8, rhs.const_exprs_9, rhs.const_exprs_10, rhs.addString1, rhs.addString2, rhs.shouldSetBit0AndBit2, rhs.u, rhs.shouldBeConstS1, rhs.defaultWithFoo);
  }
  inline bool operator==(const StructuredParcelable& rhs) const {
    return std::tie(shouldContainThreeFs, f, shouldBeJerry, shouldBeByteBar, shouldBeIntBar, shouldBeLongBar, shouldContainTwoByteFoos, shouldContainTwoIntFoos, shouldContainTwoLongFoos, stringDefaultsToFoo, byteDefaultsToFour, intDefaultsToFive, longDefaultsToNegativeSeven, booleanDefaultsToTrue, charDefaultsToC, floatDefaultsToPi, doubleWithDefault, arrayDefaultsTo123, arrayDefaultsToEmpty, boolDefault, byteDefault, intDefault, longDefault, floatDefault, doubleDefault, checkDoubleFromFloat, checkStringArray1, checkStringArray2, int32_min, int32_max, int64_max, hexInt32_neg_1, ibinder, int32_1, int64_1, hexInt32_pos_1, hexInt64_pos_1, const_exprs_1, const_exprs_2, const_exprs_3, const_exprs_4, const_exprs_5, const_exprs_6, const_exprs_7, const_exprs_8, const_exprs_9, const_exprs_10, addString1, addString2, shouldSetBit0AndBit2, u, shouldBeConstS1, defaultWithFoo) == std::tie(rhs.shouldContainThreeFs, rhs.f, rhs.shouldBeJerry, rhs.shouldBeByteBar, rhs.shouldBeIntBar, rhs.shouldBeLongBar, rhs.shouldContainTwoByteFoos, rhs.shouldContainTwoIntFoos, rhs.shouldContainTwoLongFoos, rhs.stringDefaultsToFoo, rhs.byteDefaultsToFour, rhs.intDefaultsToFive, rhs.longDefaultsToNegativeSeven, rhs.booleanDefaultsToTrue, rhs.charDefaultsToC, rhs.floatDefaultsToPi, rhs.doubleWithDefault, rhs.arrayDefaultsTo123, rhs.arrayDefaultsToEmpty, rhs.boolDefault, rhs.byteDefault, rhs.intDefault, rhs.longDefault, rhs.floatDefault, rhs.doubleDefault, rhs.checkDoubleFromFloat, rhs.checkStringArray1, rhs.checkStringArray2, rhs.int32_min, rhs.int32_max, rhs.int64_max, rhs.hexInt32_neg_1, rhs.ibinder, rhs.int32_1, rhs.int64_1, rhs.hexInt32_pos_1, rhs.hexInt64_pos_1, rhs.const_exprs_1, rhs.const_exprs_2, rhs.const_exprs_3, rhs.const_exprs_4, rhs.const_exprs_5, rhs.const_exprs_6, rhs.const_exprs_7, rhs.const_exprs_8, rhs.const_exprs_9, rhs.const_exprs_10, rhs.addString1, rhs.addString2, rhs.shouldSetBit0AndBit2, rhs.u, rhs.shouldBeConstS1, rhs.defaultWithFoo);
  }
  inline bool operator>(const StructuredParcelable& rhs) const {
    return std::tie(shouldContainThreeFs, f, shouldBeJerry, shouldBeByteBar, shouldBeIntBar, shouldBeLongBar, shouldContainTwoByteFoos, shouldContainTwoIntFoos, shouldContainTwoLongFoos, stringDefaultsToFoo, byteDefaultsToFour, intDefaultsToFive, longDefaultsToNegativeSeven, booleanDefaultsToTrue, charDefaultsToC, floatDefaultsToPi, doubleWithDefault, arrayDefaultsTo123, arrayDefaultsToEmpty, boolDefault, byteDefault, intDefault, longDefault, floatDefault, doubleDefault, checkDoubleFromFloat, checkStringArray1, checkStringArray2, int32_min, int32_max, int64_max, hexInt32_neg_1, ibinder, int32_1, int64_1, hexInt32_pos_1, hexInt64_pos_1, const_exprs_1, const_exprs_2, const_exprs_3, const_exprs_4, const_exprs_5, const_exprs_6, const_exprs_7, const_exprs_8, const_exprs_9, const_exprs_10, addString1, addString2, shouldSetBit0AndBit2, u, shouldBeConstS1, defaultWithFoo) > std::tie(rhs.shouldContainThreeFs, rhs.f, rhs.shouldBeJerry, rhs.shouldBeByteBar, rhs.shouldBeIntBar, rhs.shouldBeLongBar, rhs.shouldContainTwoByteFoos, rhs.shouldContainTwoIntFoos, rhs.shouldContainTwoLongFoos, rhs.stringDefaultsToFoo, rhs.byteDefaultsToFour, rhs.intDefaultsToFive, rhs.longDefaultsToNegativeSeven, rhs.booleanDefaultsToTrue, rhs.charDefaultsToC, rhs.floatDefaultsToPi, rhs.doubleWithDefault, rhs.arrayDefaultsTo123, rhs.arrayDefaultsToEmpty, rhs.boolDefault, rhs.byteDefault, rhs.intDefault, rhs.longDefault, rhs.floatDefault, rhs.doubleDefault, rhs.checkDoubleFromFloat, rhs.checkStringArray1, rhs.checkStringArray2, rhs.int32_min, rhs.int32_max, rhs.int64_max, rhs.hexInt32_neg_1, rhs.ibinder, rhs.int32_1, rhs.int64_1, rhs.hexInt32_pos_1, rhs.hexInt64_pos_1, rhs.const_exprs_1, rhs.const_exprs_2, rhs.const_exprs_3, rhs.const_exprs_4, rhs.const_exprs_5, rhs.const_exprs_6, rhs.const_exprs_7, rhs.const_exprs_8, rhs.const_exprs_9, rhs.const_exprs_10, rhs.addString1, rhs.addString2, rhs.shouldSetBit0AndBit2, rhs.u, rhs.shouldBeConstS1, rhs.defaultWithFoo);
  }
  inline bool operator>=(const StructuredParcelable& rhs) const {
    return std::tie(shouldContainThreeFs, f, shouldBeJerry, shouldBeByteBar, shouldBeIntBar, shouldBeLongBar, shouldContainTwoByteFoos, shouldContainTwoIntFoos, shouldContainTwoLongFoos, stringDefaultsToFoo, byteDefaultsToFour, intDefaultsToFive, longDefaultsToNegativeSeven, booleanDefaultsToTrue, charDefaultsToC, floatDefaultsToPi, doubleWithDefault, arrayDefaultsTo123, arrayDefaultsToEmpty, boolDefault, byteDefault, intDefault, longDefault, floatDefault, doubleDefault, checkDoubleFromFloat, checkStringArray1, checkStringArray2, int32_min, int32_max, int64_max, hexInt32_neg_1, ibinder, int32_1, int64_1, hexInt32_pos_1, hexInt64_pos_1, const_exprs_1, const_exprs_2, const_exprs_3, const_exprs_4, const_exprs_5, const_exprs_6, const_exprs_7, const_exprs_8, const_exprs_9, const_exprs_10, addString1, addString2, shouldSetBit0AndBit2, u, shouldBeConstS1, defaultWithFoo) >= std::tie(rhs.shouldContainThreeFs, rhs.f, rhs.shouldBeJerry, rhs.shouldBeByteBar, rhs.shouldBeIntBar, rhs.shouldBeLongBar, rhs.shouldContainTwoByteFoos, rhs.shouldContainTwoIntFoos, rhs.shouldContainTwoLongFoos, rhs.stringDefaultsToFoo, rhs.byteDefaultsToFour, rhs.intDefaultsToFive, rhs.longDefaultsToNegativeSeven, rhs.booleanDefaultsToTrue, rhs.charDefaultsToC, rhs.floatDefaultsToPi, rhs.doubleWithDefault, rhs.arrayDefaultsTo123, rhs.arrayDefaultsToEmpty, rhs.boolDefault, rhs.byteDefault, rhs.intDefault, rhs.longDefault, rhs.floatDefault, rhs.doubleDefault, rhs.checkDoubleFromFloat, rhs.checkStringArray1, rhs.checkStringArray2, rhs.int32_min, rhs.int32_max, rhs.int64_max, rhs.hexInt32_neg_1, rhs.ibinder, rhs.int32_1, rhs.int64_1, rhs.hexInt32_pos_1, rhs.hexInt64_pos_1, rhs.const_exprs_1, rhs.const_exprs_2, rhs.const_exprs_3, rhs.const_exprs_4, rhs.const_exprs_5, rhs.const_exprs_6, rhs.const_exprs_7, rhs.const_exprs_8, rhs.const_exprs_9, rhs.const_exprs_10, rhs.addString1, rhs.addString2, rhs.shouldSetBit0AndBit2, rhs.u, rhs.shouldBeConstS1, rhs.defaultWithFoo);
  }

  static const ::ndk::parcelable_stability_t _aidl_stability = ::ndk::STABILITY_LOCAL;
  enum : int32_t { BIT0 = 1 };
  enum : int32_t { BIT1 = 2 };
  enum : int32_t { BIT2 = 4 };
  inline std::string toString() const {
    std::ostringstream os;
    os << "StructuredParcelable{";
    os << "shouldContainThreeFs: " << ::android::internal::ToString(shouldContainThreeFs);
    os << ", f: " << ::android::internal::ToString(f);
    os << ", shouldBeJerry: " << ::android::internal::ToString(shouldBeJerry);
    os << ", shouldBeByteBar: " << ::android::internal::ToString(shouldBeByteBar);
    os << ", shouldBeIntBar: " << ::android::internal::ToString(shouldBeIntBar);
    os << ", shouldBeLongBar: " << ::android::internal::ToString(shouldBeLongBar);
    os << ", shouldContainTwoByteFoos: " << ::android::internal::ToString(shouldContainTwoByteFoos);
    os << ", shouldContainTwoIntFoos: " << ::android::internal::ToString(shouldContainTwoIntFoos);
    os << ", shouldContainTwoLongFoos: " << ::android::internal::ToString(shouldContainTwoLongFoos);
    os << ", stringDefaultsToFoo: " << ::android::internal::ToString(stringDefaultsToFoo);
    os << ", byteDefaultsToFour: " << ::android::internal::ToString(byteDefaultsToFour);
    os << ", intDefaultsToFive: " << ::android::internal::ToString(intDefaultsToFive);
    os << ", longDefaultsToNegativeSeven: " << ::android::internal::ToString(longDefaultsToNegativeSeven);
    os << ", booleanDefaultsToTrue: " << ::android::internal::ToString(booleanDefaultsToTrue);
    os << ", charDefaultsToC: " << ::android::internal::ToString(charDefaultsToC);
    os << ", floatDefaultsToPi: " << ::android::internal::ToString(floatDefaultsToPi);
    os << ", doubleWithDefault: " << ::android::internal::ToString(doubleWithDefault);
    os << ", arrayDefaultsTo123: " << ::android::internal::ToString(arrayDefaultsTo123);
    os << ", arrayDefaultsToEmpty: " << ::android::internal::ToString(arrayDefaultsToEmpty);
    os << ", boolDefault: " << ::android::internal::ToString(boolDefault);
    os << ", byteDefault: " << ::android::internal::ToString(byteDefault);
    os << ", intDefault: " << ::android::internal::ToString(intDefault);
    os << ", longDefault: " << ::android::internal::ToString(longDefault);
    os << ", floatDefault: " << ::android::internal::ToString(floatDefault);
    os << ", doubleDefault: " << ::android::internal::ToString(doubleDefault);
    os << ", checkDoubleFromFloat: " << ::android::internal::ToString(checkDoubleFromFloat);
    os << ", checkStringArray1: " << ::android::internal::ToString(checkStringArray1);
    os << ", checkStringArray2: " << ::android::internal::ToString(checkStringArray2);
    os << ", int32_min: " << ::android::internal::ToString(int32_min);
    os << ", int32_max: " << ::android::internal::ToString(int32_max);
    os << ", int64_max: " << ::android::internal::ToString(int64_max);
    os << ", hexInt32_neg_1: " << ::android::internal::ToString(hexInt32_neg_1);
    os << ", ibinder: " << ::android::internal::ToString(ibinder);
    os << ", int32_1: " << ::android::internal::ToString(int32_1);
    os << ", int64_1: " << ::android::internal::ToString(int64_1);
    os << ", hexInt32_pos_1: " << ::android::internal::ToString(hexInt32_pos_1);
    os << ", hexInt64_pos_1: " << ::android::internal::ToString(hexInt64_pos_1);
    os << ", const_exprs_1: " << ::android::internal::ToString(const_exprs_1);
    os << ", const_exprs_2: " << ::android::internal::ToString(const_exprs_2);
    os << ", const_exprs_3: " << ::android::internal::ToString(const_exprs_3);
    os << ", const_exprs_4: " << ::android::internal::ToString(const_exprs_4);
    os << ", const_exprs_5: " << ::android::internal::ToString(const_exprs_5);
    os << ", const_exprs_6: " << ::android::internal::ToString(const_exprs_6);
    os << ", const_exprs_7: " << ::android::internal::ToString(const_exprs_7);
    os << ", const_exprs_8: " << ::android::internal::ToString(const_exprs_8);
    os << ", const_exprs_9: " << ::android::internal::ToString(const_exprs_9);
    os << ", const_exprs_10: " << ::android::internal::ToString(const_exprs_10);
    os << ", addString1: " << ::android::internal::ToString(addString1);
    os << ", addString2: " << ::android::internal::ToString(addString2);
    os << ", shouldSetBit0AndBit2: " << ::android::internal::ToString(shouldSetBit0AndBit2);
    os << ", u: " << ::android::internal::ToString(u);
    os << ", shouldBeConstS1: " << ::android::internal::ToString(shouldBeConstS1);
    os << ", defaultWithFoo: " << ::android::internal::ToString(defaultWithFoo);
    os << "}";
    return os.str();
  }
};
}  // namespace tests
}  // namespace aidl
}  // namespace android
}  // namespace aidl
