/*
 * This file is auto-generated.  DO NOT MODIFY.
 */
package android.aidl.tests;
public class GenericStructuredParcelable<T,U,B> implements android.os.Parcelable
{
  public int a = 0;
  public int b = 0;
  public static final android.os.Parcelable.Creator<GenericStructuredParcelable> CREATOR = new android.os.Parcelable.Creator<GenericStructuredParcelable>() {
    @Override
    public GenericStructuredParcelable createFromParcel(android.os.Parcel _aidl_source) {
      GenericStructuredParcelable _aidl_out = new GenericStructuredParcelable();
      _aidl_out.readFromParcel(_aidl_source);
      return _aidl_out;
    }
    @Override
    public GenericStructuredParcelable[] newArray(int _aidl_size) {
      return new GenericStructuredParcelable[_aidl_size];
    }
  };
  @Override public final void writeToParcel(android.os.Parcel _aidl_parcel, int _aidl_flag)
  {
    int _aidl_start_pos = _aidl_parcel.dataPosition();
    _aidl_parcel.writeInt(0);
    _aidl_parcel.writeInt(a);
    _aidl_parcel.writeInt(b);
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
      if (_aidl_parcel.dataPosition() - _aidl_start_pos >= _aidl_parcelable_size) return;
      a = _aidl_parcel.readInt();
      if (_aidl_parcel.dataPosition() - _aidl_start_pos >= _aidl_parcelable_size) return;
      b = _aidl_parcel.readInt();
    } finally {
      if (_aidl_start_pos > (Integer.MAX_VALUE - _aidl_parcelable_size)) {
        throw new android.os.BadParcelableException("Overflow in the size of parcelable");
      }
      _aidl_parcel.setDataPosition(_aidl_start_pos + _aidl_parcelable_size);
    }
  }
  @Override
  public String toString() {
    java.util.StringJoiner _aidl_sj = new java.util.StringJoiner(", ", "{", "}");
    _aidl_sj.add("a: " + (a));
    _aidl_sj.add("b: " + (b));
    return "android.aidl.tests.GenericStructuredParcelable" + _aidl_sj.toString()  ;
  }
  @Override
  public int describeContents() {
    int _mask = 0;
    return _mask;
  }
}
