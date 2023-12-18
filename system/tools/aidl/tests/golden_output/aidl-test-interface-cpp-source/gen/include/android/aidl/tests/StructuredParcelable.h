#pragma once

#include <android/aidl/tests/ByteEnum.h>
#include <android/aidl/tests/ConstantExpressionEnum.h>
#include <android/aidl/tests/IntEnum.h>
#include <android/aidl/tests/LongEnum.h>
#include <android/aidl/tests/Union.h>
#include <android/binder_to_string.h>
#include <binder/IBinder.h>
#include <binder/Parcel.h>
#include <binder/Status.h>
#include <cstdint>
#include <optional>
#include <string>
#include <tuple>
#include <utils/String16.h>
#include <vector>

namespace android {

namespace aidl {

namespace tests {

class StructuredParcelable : public ::android::Parcelable {
public:
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

  ::std::vector<int32_t> shouldContainThreeFs;
  int32_t f = int32_t(0);
  ::std::string shouldBeJerry;
  ::android::aidl::tests::ByteEnum shouldBeByteBar = ::android::aidl::tests::ByteEnum(0);
  ::android::aidl::tests::IntEnum shouldBeIntBar = ::android::aidl::tests::IntEnum(0);
  ::android::aidl::tests::LongEnum shouldBeLongBar = ::android::aidl::tests::LongEnum(0);
  ::std::vector<::android::aidl::tests::ByteEnum> shouldContainTwoByteFoos;
  ::std::vector<::android::aidl::tests::IntEnum> shouldContainTwoIntFoos;
  ::std::vector<::android::aidl::tests::LongEnum> shouldContainTwoLongFoos;
  ::android::String16 stringDefaultsToFoo = ::android::String16(::android::String16("foo"));
  int8_t byteDefaultsToFour = int8_t(4);
  int32_t intDefaultsToFive = int32_t(5);
  int64_t longDefaultsToNegativeSeven = int64_t(-7L);
  bool booleanDefaultsToTrue = bool(true);
  char16_t charDefaultsToC = char16_t('C');
  float floatDefaultsToPi = float(3.140000f);
  double doubleWithDefault = double(-314000000000000000.000000);
  ::std::vector<int32_t> arrayDefaultsTo123 = ::std::vector<int32_t>({1, 2, 3});
  ::std::vector<int32_t> arrayDefaultsToEmpty = ::std::vector<int32_t>({});
  bool boolDefault = bool(false);
  int8_t byteDefault = int8_t(0);
  int32_t intDefault = int32_t(0);
  int64_t longDefault = int64_t(0L);
  float floatDefault = float(0.000000f);
  double doubleDefault = double(0.000000);
  double checkDoubleFromFloat = double(3.140000);
  ::std::vector<::android::String16> checkStringArray1 = ::std::vector<::android::String16>({::android::String16("a"), ::android::String16("b")});
  ::std::vector<::std::string> checkStringArray2 = ::std::vector<::std::string>({"a", "b"});
  int32_t int32_min = int32_t(-2147483648);
  int32_t int32_max = int32_t(2147483647);
  int64_t int64_max = int64_t(9223372036854775807L);
  int32_t hexInt32_neg_1 = int32_t(-1);
  ::android::sp<::android::IBinder> ibinder;
  ::std::vector<int32_t> int32_1 = ::std::vector<int32_t>({1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1});
  ::std::vector<int64_t> int64_1 = ::std::vector<int64_t>({1L, 1L, 1L, 1L, 1L, 1L, 1L, 1L, 1L, 1L});
  int32_t hexInt32_pos_1 = int32_t(1);
  int32_t hexInt64_pos_1 = int32_t(1);
  ::android::aidl::tests::ConstantExpressionEnum const_exprs_1 = ::android::aidl::tests::ConstantExpressionEnum(0);
  ::android::aidl::tests::ConstantExpressionEnum const_exprs_2 = ::android::aidl::tests::ConstantExpressionEnum(0);
  ::android::aidl::tests::ConstantExpressionEnum const_exprs_3 = ::android::aidl::tests::ConstantExpressionEnum(0);
  ::android::aidl::tests::ConstantExpressionEnum const_exprs_4 = ::android::aidl::tests::ConstantExpressionEnum(0);
  ::android::aidl::tests::ConstantExpressionEnum const_exprs_5 = ::android::aidl::tests::ConstantExpressionEnum(0);
  ::android::aidl::tests::ConstantExpressionEnum const_exprs_6 = ::android::aidl::tests::ConstantExpressionEnum(0);
  ::android::aidl::tests::ConstantExpressionEnum const_exprs_7 = ::android::aidl::tests::ConstantExpressionEnum(0);
  ::android::aidl::tests::ConstantExpressionEnum const_exprs_8 = ::android::aidl::tests::ConstantExpressionEnum(0);
  ::android::aidl::tests::ConstantExpressionEnum const_exprs_9 = ::android::aidl::tests::ConstantExpressionEnum(0);
  ::android::aidl::tests::ConstantExpressionEnum const_exprs_10 = ::android::aidl::tests::ConstantExpressionEnum(0);
  ::std::string addString1 = ::std::string("hello world!");
  ::std::string addString2 = ::std::string("The quick brown fox jumps over the lazy dog.");
  int32_t shouldSetBit0AndBit2 = int32_t(0);
  ::std::optional<::android::aidl::tests::Union> u;
  ::std::optional<::android::aidl::tests::Union> shouldBeConstS1;
  ::android::aidl::tests::IntEnum defaultWithFoo = ::android::aidl::tests::IntEnum(::android::aidl::tests::IntEnum::FOO);
  enum  : int32_t {
    BIT0 = 1,
    BIT1 = 2,
    BIT2 = 4,
  };
  ::android::status_t readFromParcel(const ::android::Parcel* _aidl_parcel) final;
  ::android::status_t writeToParcel(::android::Parcel* _aidl_parcel) const final;
  static const ::android::String16& getParcelableDescriptor() {
    static const ::android::StaticString16 DESCIPTOR (u"android.aidl.tests.StructuredParcelable");
    return DESCIPTOR;
  }
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
};  // class StructuredParcelable

}  // namespace tests

}  // namespace aidl

}  // namespace android
