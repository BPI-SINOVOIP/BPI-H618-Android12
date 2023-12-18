/*
 * This file is auto-generated.  DO NOT MODIFY.
 */
package android.aidl.tests;
/** @deprecated test */
@Deprecated
public class DeprecatedParcelable implements android.os.Parcelable
{
  public static final android.os.Parcelable.Creator<DeprecatedParcelable> CREATOR = new android.os.Parcelable.Creator<DeprecatedParcelable>() {
    @Override
    public DeprecatedParcelable createFromParcel(android.os.Parcel _aidl_source) {
      DeprecatedParcelable _aidl_out = new DeprecatedParcelable();
      _aidl_out.readFromParcel(_aidl_source);
      return _aidl_out;
    }
    @Override
    public DeprecatedParcelable[] newArray(int _aidl_size) {
      return new DeprecatedParcelable[_aidl_size];
    }
  };
  @Override public final void writeToParcel(android.os.Parcel _aidl_parcel, int _aidl_flag)
  {
    int _aidl_start_pos = _aidl_parcel.dataPosition();
    _aidl_parcel.writeInt(0);
    int _aidl_end_pos = _aidl_parcel.dataPosition();
    _aidl_parcel.setDataPosition(_aidl_start_pos);
    _aidl_parcel.writeInt(_aidl_end_pos - _aidl_start_pos);
    _aidl_parcel.setDataPosition(_aidl_end_pos);
  }
  public final void readFromParcel(android.os.Parcel _aidl_parcel)
  {
    int _aidl_start_pos = _aidl_parcel.dataPosition();
    int _aidl_parcelable_size = _aidl_parcel.readInt();
    try {
      if (_aidl_parcelable_size < 0) return;
    } finally {
      if (_aidl_start_pos > (Integer.MAX_VALUE - _aidl_parcelable_size)) {
        throw new android.os.BadParcelableException("Overflow in the size of parcelable");
      }
      _aidl_parcel.setDataPosition(_aidl_start_pos + _aidl_parcelable_size);
    }
  }
  @Override
  public int describeContents() {
    int _mask = 0;
    return _mask;
  }
}
