#pragma once

#include <android/aidl/tests/GenericStructuredParcelable.h>
#include <android/aidl/tests/IntEnum.h>
#include <android/aidl/tests/OtherParcelableForToString.h>
#include <android/aidl/tests/StructuredParcelable.h>
#include <android/aidl/tests/Union.h>
#include <android/binder_to_string.h>
#include <binder/Parcel.h>
#include <binder/Status.h>
#include <cstdint>
#include <tuple>
#include <utils/String16.h>
#include <vector>

namespace android {

namespace aidl {

namespace tests {

class ParcelableForToString : public ::android::Parcelable {
public:
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

  int32_t intValue = int32_t(0);
  ::std::vector<int32_t> intArray;
  int64_t longValue = int64_t(0L);
  ::std::vector<int64_t> longArray;
  double doubleValue = double(0.000000);
  ::std::vector<double> doubleArray;
  float floatValue = float(0.000000f);
  ::std::vector<float> floatArray;
  int8_t byteValue = int8_t(0);
  ::std::vector<uint8_t> byteArray;
  bool booleanValue = bool(false);
  ::std::vector<bool> booleanArray;
  ::android::String16 stringValue;
  ::std::vector<::android::String16> stringArray;
  ::std::vector<::android::String16> stringList;
  ::android::aidl::tests::OtherParcelableForToString parcelableValue;
  ::std::vector<::android::aidl::tests::OtherParcelableForToString> parcelableArray;
  ::android::aidl::tests::IntEnum enumValue = ::android::aidl::tests::IntEnum(::android::aidl::tests::IntEnum::FOO);
  ::std::vector<::android::aidl::tests::IntEnum> enumArray;
  ::std::vector<::android::String16> nullArray;
  ::std::vector<::android::String16> nullList;
  ::android::aidl::tests::GenericStructuredParcelable<int32_t, ::android::aidl::tests::StructuredParcelable, ::android::aidl::tests::IntEnum> parcelableGeneric;
  ::android::aidl::tests::Union unionValue;
  ::android::status_t readFromParcel(const ::android::Parcel* _aidl_parcel) final;
  ::android::status_t writeToParcel(::android::Parcel* _aidl_parcel) const final;
  static const ::android::String16& getParcelableDescriptor() {
    static const ::android::StaticString16 DESCIPTOR (u"android.aidl.tests.ParcelableForToString");
    return DESCIPTOR;
  }
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
};  // class ParcelableForToString

}  // namespace tests

}  // namespace aidl

}  // namespace android
