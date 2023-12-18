#include "aidl/android/aidl/tests/ParcelableForToString.h"

#include <android/binder_parcel_utils.h>

namespace aidl {
namespace android {
namespace aidl {
namespace tests {
const char* ParcelableForToString::descriptor = "android.aidl.tests.ParcelableForToString";

binder_status_t ParcelableForToString::readFromParcel(const AParcel* parcel) {
  int32_t _aidl_parcelable_size;
  int32_t _aidl_start_pos = AParcel_getDataPosition(parcel);
  binder_status_t _aidl_ret_status = AParcel_readInt32(parcel, &_aidl_parcelable_size);
  if (_aidl_start_pos > INT32_MAX - _aidl_parcelable_size) return STATUS_BAD_VALUE;
  if (_aidl_parcelable_size < 0) return STATUS_BAD_VALUE;
  if (_aidl_ret_status != STATUS_OK) return _aidl_ret_status;

  if (AParcel_getDataPosition(parcel) - _aidl_start_pos >= _aidl_parcelable_size) {
    AParcel_setDataPosition(parcel, _aidl_start_pos + _aidl_parcelable_size);
    return _aidl_ret_status;
  }
  _aidl_ret_status = AParcel_readInt32(parcel, &intValue);
  if (_aidl_ret_status != STATUS_OK) return _aidl_ret_status;

  if (AParcel_getDataPosition(parcel) - _aidl_start_pos >= _aidl_parcelable_size) {
    AParcel_setDataPosition(parcel, _aidl_start_pos + _aidl_parcelable_size);
    return _aidl_ret_status;
  }
  _aidl_ret_status = ::ndk::AParcel_readVector(parcel, &intArray);
  if (_aidl_ret_status != STATUS_OK) return _aidl_ret_status;

  if (AParcel_getDataPosition(parcel) - _aidl_start_pos >= _aidl_parcelable_size) {
    AParcel_setDataPosition(parcel, _aidl_start_pos + _aidl_parcelable_size);
    return _aidl_ret_status;
  }
  _aidl_ret_status = AParcel_readInt64(parcel, &longValue);
  if (_aidl_ret_status != STATUS_OK) return _aidl_ret_status;

  if (AParcel_getDataPosition(parcel) - _aidl_start_pos >= _aidl_parcelable_size) {
    AParcel_setDataPosition(parcel, _aidl_start_pos + _aidl_parcelable_size);
    return _aidl_ret_status;
  }
  _aidl_ret_status = ::ndk::AParcel_readVector(parcel, &longArray);
  if (_aidl_ret_status != STATUS_OK) return _aidl_ret_status;

  if (AParcel_getDataPosition(parcel) - _aidl_start_pos >= _aidl_parcelable_size) {
    AParcel_setDataPosition(parcel, _aidl_start_pos + _aidl_parcelable_size);
    return _aidl_ret_status;
  }
  _aidl_ret_status = AParcel_readDouble(parcel, &doubleValue);
  if (_aidl_ret_status != STATUS_OK) return _aidl_ret_status;

  if (AParcel_getDataPosition(parcel) - _aidl_start_pos >= _aidl_parcelable_size) {
    AParcel_setDataPosition(parcel, _aidl_start_pos + _aidl_parcelable_size);
    return _aidl_ret_status;
  }
  _aidl_ret_status = ::ndk::AParcel_readVector(parcel, &doubleArray);
  if (_aidl_ret_status != STATUS_OK) return _aidl_ret_status;

  if (AParcel_getDataPosition(parcel) - _aidl_start_pos >= _aidl_parcelable_size) {
    AParcel_setDataPosition(parcel, _aidl_start_pos + _aidl_parcelable_size);
    return _aidl_ret_status;
  }
  _aidl_ret_status = AParcel_readFloat(parcel, &floatValue);
  if (_aidl_ret_status != STATUS_OK) return _aidl_ret_status;

  if (AParcel_getDataPosition(parcel) - _aidl_start_pos >= _aidl_parcelable_size) {
    AParcel_setDataPosition(parcel, _aidl_start_pos + _aidl_parcelable_size);
    return _aidl_ret_status;
  }
  _aidl_ret_status = ::ndk::AParcel_readVector(parcel, &floatArray);
  if (_aidl_ret_status != STATUS_OK) return _aidl_ret_status;

  if (AParcel_getDataPosition(parcel) - _aidl_start_pos >= _aidl_parcelable_size) {
    AParcel_setDataPosition(parcel, _aidl_start_pos + _aidl_parcelable_size);
    return _aidl_ret_status;
  }
  _aidl_ret_status = AParcel_readByte(parcel, &byteValue);
  if (_aidl_ret_status != STATUS_OK) return _aidl_ret_status;

  if (AParcel_getDataPosition(parcel) - _aidl_start_pos >= _aidl_parcelable_size) {
    AParcel_setDataPosition(parcel, _aidl_start_pos + _aidl_parcelable_size);
    return _aidl_ret_status;
  }
  _aidl_ret_status = ::ndk::AParcel_readVector(parcel, &byteArray);
  if (_aidl_ret_status != STATUS_OK) return _aidl_ret_status;

  if (AParcel_getDataPosition(parcel) - _aidl_start_pos >= _aidl_parcelable_size) {
    AParcel_setDataPosition(parcel, _aidl_start_pos + _aidl_parcelable_size);
    return _aidl_ret_status;
  }
  _aidl_ret_status = AParcel_readBool(parcel, &booleanValue);
  if (_aidl_ret_status != STATUS_OK) return _aidl_ret_status;

  if (AParcel_getDataPosition(parcel) - _aidl_start_pos >= _aidl_parcelable_size) {
    AParcel_setDataPosition(parcel, _aidl_start_pos + _aidl_parcelable_size);
    return _aidl_ret_status;
  }
  _aidl_ret_status = ::ndk::AParcel_readVector(parcel, &booleanArray);
  if (_aidl_ret_status != STATUS_OK) return _aidl_ret_status;

  if (AParcel_getDataPosition(parcel) - _aidl_start_pos >= _aidl_parcelable_size) {
    AParcel_setDataPosition(parcel, _aidl_start_pos + _aidl_parcelable_size);
    return _aidl_ret_status;
  }
  _aidl_ret_status = ::ndk::AParcel_readString(parcel, &stringValue);
  if (_aidl_ret_status != STATUS_OK) return _aidl_ret_status;

  if (AParcel_getDataPosition(parcel) - _aidl_start_pos >= _aidl_parcelable_size) {
    AParcel_setDataPosition(parcel, _aidl_start_pos + _aidl_parcelable_size);
    return _aidl_ret_status;
  }
  _aidl_ret_status = ::ndk::AParcel_readVector(parcel, &stringArray);
  if (_aidl_ret_status != STATUS_OK) return _aidl_ret_status;

  if (AParcel_getDataPosition(parcel) - _aidl_start_pos >= _aidl_parcelable_size) {
    AParcel_setDataPosition(parcel, _aidl_start_pos + _aidl_parcelable_size);
    return _aidl_ret_status;
  }
  _aidl_ret_status = ::ndk::AParcel_readVector(parcel, &stringList);
  if (_aidl_ret_status != STATUS_OK) return _aidl_ret_status;

  if (AParcel_getDataPosition(parcel) - _aidl_start_pos >= _aidl_parcelable_size) {
    AParcel_setDataPosition(parcel, _aidl_start_pos + _aidl_parcelable_size);
    return _aidl_ret_status;
  }
  _aidl_ret_status = ::ndk::AParcel_readParcelable(parcel, &parcelableValue);
  if (_aidl_ret_status != STATUS_OK) return _aidl_ret_status;

  if (AParcel_getDataPosition(parcel) - _aidl_start_pos >= _aidl_parcelable_size) {
    AParcel_setDataPosition(parcel, _aidl_start_pos + _aidl_parcelable_size);
    return _aidl_ret_status;
  }
  _aidl_ret_status = ::ndk::AParcel_readVector(parcel, &parcelableArray);
  if (_aidl_ret_status != STATUS_OK) return _aidl_ret_status;

  if (AParcel_getDataPosition(parcel) - _aidl_start_pos >= _aidl_parcelable_size) {
    AParcel_setDataPosition(parcel, _aidl_start_pos + _aidl_parcelable_size);
    return _aidl_ret_status;
  }
  _aidl_ret_status = AParcel_readInt32(parcel, reinterpret_cast<int32_t*>(&enumValue));
  if (_aidl_ret_status != STATUS_OK) return _aidl_ret_status;

  if (AParcel_getDataPosition(parcel) - _aidl_start_pos >= _aidl_parcelable_size) {
    AParcel_setDataPosition(parcel, _aidl_start_pos + _aidl_parcelable_size);
    return _aidl_ret_status;
  }
  _aidl_ret_status = AParcel_readInt32Array(parcel, static_cast<void*>(&enumArray), ndk::AParcel_stdVectorAllocator<int32_t>);
  if (_aidl_ret_status != STATUS_OK) return _aidl_ret_status;

  if (AParcel_getDataPosition(parcel) - _aidl_start_pos >= _aidl_parcelable_size) {
    AParcel_setDataPosition(parcel, _aidl_start_pos + _aidl_parcelable_size);
    return _aidl_ret_status;
  }
  _aidl_ret_status = ::ndk::AParcel_readVector(parcel, &nullArray);
  if (_aidl_ret_status != STATUS_OK) return _aidl_ret_status;

  if (AParcel_getDataPosition(parcel) - _aidl_start_pos >= _aidl_parcelable_size) {
    AParcel_setDataPosition(parcel, _aidl_start_pos + _aidl_parcelable_size);
    return _aidl_ret_status;
  }
  _aidl_ret_status = ::ndk::AParcel_readVector(parcel, &nullList);
  if (_aidl_ret_status != STATUS_OK) return _aidl_ret_status;

  if (AParcel_getDataPosition(parcel) - _aidl_start_pos >= _aidl_parcelable_size) {
    AParcel_setDataPosition(parcel, _aidl_start_pos + _aidl_parcelable_size);
    return _aidl_ret_status;
  }
  _aidl_ret_status = ::ndk::AParcel_readParcelable(parcel, &parcelableGeneric);
  if (_aidl_ret_status != STATUS_OK) return _aidl_ret_status;

  if (AParcel_getDataPosition(parcel) - _aidl_start_pos >= _aidl_parcelable_size) {
    AParcel_setDataPosition(parcel, _aidl_start_pos + _aidl_parcelable_size);
    return _aidl_ret_status;
  }
  _aidl_ret_status = ::ndk::AParcel_readParcelable(parcel, &unionValue);
  if (_aidl_ret_status != STATUS_OK) return _aidl_ret_status;

  AParcel_setDataPosition(parcel, _aidl_start_pos + _aidl_parcelable_size);
  return _aidl_ret_status;
}
binder_status_t ParcelableForToString::writeToParcel(AParcel* parcel) const {
  binder_status_t _aidl_ret_status;
  size_t _aidl_start_pos = AParcel_getDataPosition(parcel);
  _aidl_ret_status = AParcel_writeInt32(parcel, 0);
  if (_aidl_ret_status != STATUS_OK) return _aidl_ret_status;

  _aidl_ret_status = AParcel_writeInt32(parcel, intValue);
  if (_aidl_ret_status != STATUS_OK) return _aidl_ret_status;

  _aidl_ret_status = ::ndk::AParcel_writeVector(parcel, intArray);
  if (_aidl_ret_status != STATUS_OK) return _aidl_ret_status;

  _aidl_ret_status = AParcel_writeInt64(parcel, longValue);
  if (_aidl_ret_status != STATUS_OK) return _aidl_ret_status;

  _aidl_ret_status = ::ndk::AParcel_writeVector(parcel, longArray);
  if (_aidl_ret_status != STATUS_OK) return _aidl_ret_status;

  _aidl_ret_status = AParcel_writeDouble(parcel, doubleValue);
  if (_aidl_ret_status != STATUS_OK) return _aidl_ret_status;

  _aidl_ret_status = ::ndk::AParcel_writeVector(parcel, doubleArray);
  if (_aidl_ret_status != STATUS_OK) return _aidl_ret_status;

  _aidl_ret_status = AParcel_writeFloat(parcel, floatValue);
  if (_aidl_ret_status != STATUS_OK) return _aidl_ret_status;

  _aidl_ret_status = ::ndk::AParcel_writeVector(parcel, floatArray);
  if (_aidl_ret_status != STATUS_OK) return _aidl_ret_status;

  _aidl_ret_status = AParcel_writeByte(parcel, byteValue);
  if (_aidl_ret_status != STATUS_OK) return _aidl_ret_status;

  _aidl_ret_status = ::ndk::AParcel_writeVector(parcel, byteArray);
  if (_aidl_ret_status != STATUS_OK) return _aidl_ret_status;

  _aidl_ret_status = AParcel_writeBool(parcel, booleanValue);
  if (_aidl_ret_status != STATUS_OK) return _aidl_ret_status;

  _aidl_ret_status = ::ndk::AParcel_writeVector(parcel, booleanArray);
  if (_aidl_ret_status != STATUS_OK) return _aidl_ret_status;

  _aidl_ret_status = ::ndk::AParcel_writeString(parcel, stringValue);
  if (_aidl_ret_status != STATUS_OK) return _aidl_ret_status;

  _aidl_ret_status = ::ndk::AParcel_writeVector(parcel, stringArray);
  if (_aidl_ret_status != STATUS_OK) return _aidl_ret_status;

  _aidl_ret_status = ::ndk::AParcel_writeVector(parcel, stringList);
  if (_aidl_ret_status != STATUS_OK) return _aidl_ret_status;

  _aidl_ret_status = ::ndk::AParcel_writeParcelable(parcel, parcelableValue);
  if (_aidl_ret_status != STATUS_OK) return _aidl_ret_status;

  _aidl_ret_status = ::ndk::AParcel_writeVector(parcel, parcelableArray);
  if (_aidl_ret_status != STATUS_OK) return _aidl_ret_status;

  _aidl_ret_status = AParcel_writeInt32(parcel, static_cast<int32_t>(enumValue));
  if (_aidl_ret_status != STATUS_OK) return _aidl_ret_status;

  _aidl_ret_status = AParcel_writeInt32Array(parcel, reinterpret_cast<const int32_t*>(enumArray.data()), enumArray.size());
  if (_aidl_ret_status != STATUS_OK) return _aidl_ret_status;

  _aidl_ret_status = ::ndk::AParcel_writeVector(parcel, nullArray);
  if (_aidl_ret_status != STATUS_OK) return _aidl_ret_status;

  _aidl_ret_status = ::ndk::AParcel_writeVector(parcel, nullList);
  if (_aidl_ret_status != STATUS_OK) return _aidl_ret_status;

  _aidl_ret_status = ::ndk::AParcel_writeParcelable(parcel, parcelableGeneric);
  if (_aidl_ret_status != STATUS_OK) return _aidl_ret_status;

  _aidl_ret_status = ::ndk::AParcel_writeParcelable(parcel, unionValue);
  if (_aidl_ret_status != STATUS_OK) return _aidl_ret_status;

  size_t _aidl_end_pos = AParcel_getDataPosition(parcel);
  AParcel_setDataPosition(parcel, _aidl_start_pos);
  AParcel_writeInt32(parcel, _aidl_end_pos - _aidl_start_pos);
  AParcel_setDataPosition(parcel, _aidl_end_pos);
  return _aidl_ret_status;
}

}  // namespace tests
}  // namespace aidl
}  // namespace android
}  // namespace aidl
