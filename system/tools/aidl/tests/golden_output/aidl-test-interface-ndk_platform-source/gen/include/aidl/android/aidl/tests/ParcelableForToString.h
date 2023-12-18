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
#include <aidl/android/aidl/tests/GenericStructuredParcelable.h>
#include <aidl/android/aidl/tests/IntEnum.h>
#include <aidl/android/aidl/tests/OtherParcelableForToString.h>
#include <aidl/android/aidl/tests/StructuredParcelable.h>
#include <aidl/android/aidl/tests/Union.h>
namespace aidl {
namespace android {
namespace aidl {
namespace tests {
class ParcelableForToString {
public:
  typedef std::false_type fixed_size;
  static const char* descriptor;

  int32_t intValue = 0;
  std::vector<int32_t> intArray;
  int64_t longValue = 0L;
  std::vector<int64_t> longArray;
  double doubleValue = 0.000000;
  std::vector<double> doubleArray;
  float floatValue = 0.000000f;
  std::vector<float> floatArray;
  int8_t byteValue = 0;
  std::vector<uint8_t> byteArray;
  bool booleanValue = false;
  std::vector<bool> booleanArray;
  std::string stringValue;
  std::vector<std::string> stringArray;
  std::vector<std::string> stringList;
  ::aidl::android::aidl::tests::OtherParcelableForToString parcelableValue;
  std::vector<::aidl::android::aidl::tests::OtherParcelableForToString> parcelableArray;
  ::aidl::android::aidl::tests::IntEnum enumValue = ::aidl::android::aidl::tests::IntEnum::FOO;
  std::vector<::aidl::android::aidl::tests::IntEnum> enumArray;
  std::vector<std::string> nullArray;
  std::vector<std::string> nullList;
  ::aidl::android::aidl::tests::GenericStructuredParcelable<int32_t, ::aidl::android::aidl::tests::StructuredParcelable, ::aidl::android::aidl::tests::IntEnum> parcelableGeneric;
  ::aidl::android::aidl::tests::Union unionValue;

  binder_status_t readFromParcel(const AParcel* parcel);
  binder_status_t writeToParcel(AParcel* parcel) const;

  inline bool operator!=(const ParcelableForToString& rhs) const {
    return std::tie(intValue, intArray, longValue, longArray, doubleValue, doubleArray, floatValue, floatArray, byteValue, byteArray, booleanValue, booleanArray, stringValue, stringArray, stringList, parcelableValue, parcelableArray, enumValue, enumArray, nullArray, nullList, parcelableGeneric, unionValue) != std::tie(rhs.intValue, rhs.intArray, rhs.longValue, rhs.longArray, rhs.doubleValue, rhs.doubleArray, rhs.floatValue, rhs.floatArray, rhs.byteValue, rhs.byteArray, rhs.booleanValue, rhs.booleanArray, rhs.stringValue, rhs.stringArray, rhs.stringList, rhs.parcelableValue, rhs.parcelableArray, rhs.enumValue, rhs.enumArray, rhs.nullArray, rhs.nullList, rhs.parcelableGeneric, rhs.unionValue);
  }
  inline bool operator<(const ParcelableForToString& rhs) const {
    return std::tie(intValue, intArray, longValue, longArray, doubleValue, doubleArray, floatValue, floatArray, byteValue, byteArray, booleanValue, booleanArray, stringValue, stringArray, stringList, parcelableValue, parcelableArray, enumValue, enumArray, nullArray, nullList, parcelableGeneric, unionValue) < std::tie(rhs.intValue, rhs.intArray, rhs.longValue, rhs.longArray, rhs.doubleValue, rhs.doubleArray, rhs.floatValue, rhs.floatArray, rhs.byteValue, rhs.byteArray, rhs.booleanValue, rhs.booleanArray, rhs.stringValue, rhs.stringArray, rhs.stringList, rhs.parcelableValue, rhs.parcelableArray, rhs.enumValue, rhs.enumArray, rhs.nullArray, rhs.nullList, rhs.parcelableGeneric, rhs.unionValue);
  }
  inline bool operator<=(const ParcelableForToString& rhs) const {
    return std::tie(intValue, intArray, longValue, longArray, doubleValue, doubleArray, floatValue, floatArray, byteValue, byteArray, booleanValue, booleanArray, stringValue, stringArray, stringList, parcelableValue, parcelableArray, enumValue, enumArray, nullArray, nullList, parcelableGeneric, unionValue) <= std::tie(rhs.intValue, rhs.intArray, rhs.longValue, rhs.longArray, rhs.doubleValue, rhs.doubleArray, rhs.floatValue, rhs.floatArray, rhs.byteValue, rhs.byteArray, rhs.booleanValue, rhs.booleanArray, rhs.stringValue, rhs.stringArray, rhs.stringList, rhs.parcelableValue, rhs.parcelableArray, rhs.enumValue, rhs.enumArray, rhs.nullArray, rhs.nullList, rhs.parcelableGeneric, rhs.unionValue);
  }
  inline bool operator==(const ParcelableForToString& rhs) const {
    return std::tie(intValue, intArray, longValue, longArray, doubleValue, doubleArray, floatValue, floatArray, byteValue, byteArray, booleanValue, booleanArray, stringValue, stringArray, stringList, parcelableValue, parcelableArray, enumValue, enumArray, nullArray, nullList, parcelableGeneric, unionValue) == std::tie(rhs.intValue, rhs.intArray, rhs.longValue, rhs.longArray, rhs.doubleValue, rhs.doubleArray, rhs.floatValue, rhs.floatArray, rhs.byteValue, rhs.byteArray, rhs.booleanValue, rhs.booleanArray, rhs.stringValue, rhs.stringArray, rhs.stringList, rhs.parcelableValue, rhs.parcelableArray, rhs.enumValue, rhs.enumArray, rhs.nullArray, rhs.nullList, rhs.parcelableGeneric, rhs.unionValue);
  }
  inline bool operator>(const ParcelableForToString& rhs) const {
    return std::tie(intValue, intArray, longValue, longArray, doubleValue, doubleArray, floatValue, floatArray, byteValue, byteArray, booleanValue, booleanArray, stringValue, stringArray, stringList, parcelableValue, parcelableArray, enumValue, enumArray, nullArray, nullList, parcelableGeneric, unionValue) > std::tie(rhs.intValue, rhs.intArray, rhs.longValue, rhs.longArray, rhs.doubleValue, rhs.doubleArray, rhs.floatValue, rhs.floatArray, rhs.byteValue, rhs.byteArray, rhs.booleanValue, rhs.booleanArray, rhs.stringValue, rhs.stringArray, rhs.stringList, rhs.parcelableValue, rhs.parcelableArray, rhs.enumValue, rhs.enumArray, rhs.nullArray, rhs.nullList, rhs.parcelableGeneric, rhs.unionValue);
  }
  inline bool operator>=(const ParcelableForToString& rhs) const {
    return std::tie(intValue, intArray, longValue, longArray, doubleValue, doubleArray, floatValue, floatArray, byteValue, byteArray, booleanValue, booleanArray, stringValue, stringArray, stringList, parcelableValue, parcelableArray, enumValue, enumArray, nullArray, nullList, parcelableGeneric, unionValue) >= std::tie(rhs.intValue, rhs.intArray, rhs.longValue, rhs.longArray, rhs.doubleValue, rhs.doubleArray, rhs.floatValue, rhs.floatArray, rhs.byteValue, rhs.byteArray, rhs.booleanValue, rhs.booleanArray, rhs.stringValue, rhs.stringArray, rhs.stringList, rhs.parcelableValue, rhs.parcelableArray, rhs.enumValue, rhs.enumArray, rhs.nullArray, rhs.nullList, rhs.parcelableGeneric, rhs.unionValue);
  }

  static const ::ndk::parcelable_stability_t _aidl_stability = ::ndk::STABILITY_LOCAL;
  inline std::string toString() const {
    std::ostringstream os;
    os << "ParcelableForToString{";
    os << "intValue: " << ::android::internal::ToString(intValue);
    os << ", intArray: " << ::android::internal::ToString(intArray);
    os << ", longValue: " << ::android::internal::ToString(longValue);
    os << ", longArray: " << ::android::internal::ToString(longArray);
    os << ", doubleValue: " << ::android::internal::ToString(doubleValue);
    os << ", doubleArray: " << ::android::internal::ToString(doubleArray);
    os << ", floatValue: " << ::android::internal::ToString(floatValue);
    os << ", floatArray: " << ::android::internal::ToString(floatArray);
    os << ", byteValue: " << ::android::internal::ToString(byteValue);
    os << ", byteArray: " << ::android::internal::ToString(byteArray);
    os << ", booleanValue: " << ::android::internal::ToString(booleanValue);
    os << ", booleanArray: " << ::android::internal::ToString(booleanArray);
    os << ", stringValue: " << ::android::internal::ToString(stringValue);
    os << ", stringArray: " << ::android::internal::ToString(stringArray);
    os << ", stringList: " << ::android::internal::ToString(stringList);
    os << ", parcelableValue: " << ::android::internal::ToString(parcelableValue);
    os << ", parcelableArray: " << ::android::internal::ToString(parcelableArray);
    os << ", enumValue: " << ::android::internal::ToString(enumValue);
    os << ", enumArray: " << ::android::internal::ToString(enumArray);
    os << ", nullArray: " << ::android::internal::ToString(nullArray);
    os << ", nullList: " << ::android::internal::ToString(nullList);
    os << ", parcelableGeneric: " << ::android::internal::ToString(parcelableGeneric);
    os << ", unionValue: " << ::android::internal::ToString(unionValue);
    os << "}";
    return os.str();
  }
};
}  // namespace tests
}  // namespace aidl
}  // namespace android
}  // namespace aidl
