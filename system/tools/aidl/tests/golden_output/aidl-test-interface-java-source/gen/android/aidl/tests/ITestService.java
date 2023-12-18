/*
 * This file is auto-generated.  DO NOT MODIFY.
 */
package android.aidl.tests;
public interface ITestService extends android.os.IInterface
{
  /** Default implementation for ITestService. */
  public static class Default implements android.aidl.tests.ITestService
  {
    // This is to emulate a method that is added after the service is implemented.
    // So the client cannot assume that a call to this method will be successful
    // or not. However, inside the test environment, we can't build client and
    // the server with different version of this AIDL file. So, we let the server
    // actually implement this and intercept the dispatch to the method
    // inside onTransact().
    // WARNING: Must be first method.
    // This requires hard coding the transaction number. As long as this method is
    // the first in this interface, it can keep the
    // "::android::IBinder::FIRST_CALL_TRANSACTION + 0" value and allow
    // methods to be added and removed.
    @Override public int UnimplementedMethod(int arg) throws android.os.RemoteException
    {
      return 0;
    }
    /**
         * @deprecated to make sure we have something in system/tools/aidl which does a compile check
         *     of deprecated and make sure this is reflected in goldens
         */
    @Override public void Deprecated() throws android.os.RemoteException
    {
    }
    @Override public void TestOneway() throws android.os.RemoteException
    {
    }
    // Test that primitives work as parameters and return types.
    @Override public boolean RepeatBoolean(boolean token) throws android.os.RemoteException
    {
      return false;
    }
    @Override public byte RepeatByte(byte token) throws android.os.RemoteException
    {
      return 0;
    }
    @Override public char RepeatChar(char token) throws android.os.RemoteException
    {
      return '\u0000';
    }
    @Override public int RepeatInt(int token) throws android.os.RemoteException
    {
      return 0;
    }
    @Override public long RepeatLong(long token) throws android.os.RemoteException
    {
      return 0L;
    }
    @Override public float RepeatFloat(float token) throws android.os.RemoteException
    {
      return 0.0f;
    }
    @Override public double RepeatDouble(double token) throws android.os.RemoteException
    {
      return 0.0d;
    }
    @Override public java.lang.String RepeatString(java.lang.String token) throws android.os.RemoteException
    {
      return null;
    }
    @Override public byte RepeatByteEnum(byte token) throws android.os.RemoteException
    {
      return 0;
    }
    @Override public int RepeatIntEnum(int token) throws android.os.RemoteException
    {
      return 0;
    }
    @Override public long RepeatLongEnum(long token) throws android.os.RemoteException
    {
      return 0L;
    }
    // Test that arrays work as parameters and return types.
    @Override public boolean[] ReverseBoolean(boolean[] input, boolean[] repeated) throws android.os.RemoteException
    {
      return null;
    }
    @Override public byte[] ReverseByte(byte[] input, byte[] repeated) throws android.os.RemoteException
    {
      return null;
    }
    @Override public char[] ReverseChar(char[] input, char[] repeated) throws android.os.RemoteException
    {
      return null;
    }
    @Override public int[] ReverseInt(int[] input, int[] repeated) throws android.os.RemoteException
    {
      return null;
    }
    @Override public long[] ReverseLong(long[] input, long[] repeated) throws android.os.RemoteException
    {
      return null;
    }
    @Override public float[] ReverseFloat(float[] input, float[] repeated) throws android.os.RemoteException
    {
      return null;
    }
    @Override public double[] ReverseDouble(double[] input, double[] repeated) throws android.os.RemoteException
    {
      return null;
    }
    @Override public java.lang.String[] ReverseString(java.lang.String[] input, java.lang.String[] repeated) throws android.os.RemoteException
    {
      return null;
    }
    @Override public byte[] ReverseByteEnum(byte[] input, byte[] repeated) throws android.os.RemoteException
    {
      return null;
    }
    @Override public int[] ReverseIntEnum(int[] input, int[] repeated) throws android.os.RemoteException
    {
      return null;
    }
    @Override public long[] ReverseLongEnum(long[] input, long[] repeated) throws android.os.RemoteException
    {
      return null;
    }
    // Test that clients can send and receive Binders.
    @Override public android.aidl.tests.INamedCallback GetOtherTestService(java.lang.String name) throws android.os.RemoteException
    {
      return null;
    }
    @Override public boolean VerifyName(android.aidl.tests.INamedCallback service, java.lang.String name) throws android.os.RemoteException
    {
      return false;
    }
    // Test that List<T> types work correctly.
    @Override public java.util.List<java.lang.String> ReverseStringList(java.util.List<java.lang.String> input, java.util.List<java.lang.String> repeated) throws android.os.RemoteException
    {
      return null;
    }
    @Override public android.os.ParcelFileDescriptor RepeatParcelFileDescriptor(android.os.ParcelFileDescriptor read) throws android.os.RemoteException
    {
      return null;
    }
    @Override public android.os.ParcelFileDescriptor[] ReverseParcelFileDescriptorArray(android.os.ParcelFileDescriptor[] input, android.os.ParcelFileDescriptor[] repeated) throws android.os.RemoteException
    {
      return null;
    }
    // Test that service specific exceptions work correctly.
    @Override public void ThrowServiceException(int code) throws android.os.RemoteException
    {
    }
    // Test nullability
    @Override public int[] RepeatNullableIntArray(int[] input) throws android.os.RemoteException
    {
      return null;
    }
    @Override public byte[] RepeatNullableByteEnumArray(byte[] input) throws android.os.RemoteException
    {
      return null;
    }
    @Override public int[] RepeatNullableIntEnumArray(int[] input) throws android.os.RemoteException
    {
      return null;
    }
    @Override public long[] RepeatNullableLongEnumArray(long[] input) throws android.os.RemoteException
    {
      return null;
    }
    @Override public java.lang.String RepeatNullableString(java.lang.String input) throws android.os.RemoteException
    {
      return null;
    }
    @Override public java.util.List<java.lang.String> RepeatNullableStringList(java.util.List<java.lang.String> input) throws android.os.RemoteException
    {
      return null;
    }
    @Override public android.aidl.tests.StructuredParcelable RepeatNullableParcelable(android.aidl.tests.StructuredParcelable input) throws android.os.RemoteException
    {
      return null;
    }
    @Override public void TakesAnIBinder(android.os.IBinder input) throws android.os.RemoteException
    {
    }
    @Override public void TakesANullableIBinder(android.os.IBinder input) throws android.os.RemoteException
    {
    }
    // Test utf8 decoding from utf16 wire format
    @Override public java.lang.String RepeatUtf8CppString(java.lang.String token) throws android.os.RemoteException
    {
      return null;
    }
    @Override public java.lang.String RepeatNullableUtf8CppString(java.lang.String token) throws android.os.RemoteException
    {
      return null;
    }
    @Override public java.lang.String[] ReverseUtf8CppString(java.lang.String[] input, java.lang.String[] repeated) throws android.os.RemoteException
    {
      return null;
    }
    @Override public java.lang.String[] ReverseNullableUtf8CppString(java.lang.String[] input, java.lang.String[] repeated) throws android.os.RemoteException
    {
      return null;
    }
    @Override public java.util.List<java.lang.String> ReverseUtf8CppStringList(java.util.List<java.lang.String> input, java.util.List<java.lang.String> repeated) throws android.os.RemoteException
    {
      return null;
    }
    @Override public android.aidl.tests.INamedCallback GetCallback(boolean return_null) throws android.os.RemoteException
    {
      return null;
    }
    // Since this paracelable has clearly defined default values, it would be
    // inefficient to use an IPC to fill it out in practice.
    @Override public void FillOutStructuredParcelable(android.aidl.tests.StructuredParcelable parcel) throws android.os.RemoteException
    {
    }
    @Override public android.aidl.tests.IOldName GetOldNameInterface() throws android.os.RemoteException
    {
      return null;
    }
    @Override public android.aidl.tests.INewName GetNewNameInterface() throws android.os.RemoteException
    {
      return null;
    }
    // Retrieve the ICppJavaTests if the server supports it
    @Override public android.os.IBinder GetCppJavaTests() throws android.os.RemoteException
    {
      return null;
    }
    @Override public byte getBackendType() throws android.os.RemoteException
    {
      return 0;
    }
    @Override
    public android.os.IBinder asBinder() {
      return null;
    }
  }
  /** Local-side IPC implementation stub class. */
  public static abstract class Stub extends android.os.Binder implements android.aidl.tests.ITestService
  {
    /** Construct the stub at attach it to the interface. */
    public Stub()
    {
      this.attachInterface(this, DESCRIPTOR);
    }
    /**
     * Cast an IBinder object into an android.aidl.tests.ITestService interface,
     * generating a proxy if needed.
     */
    public static android.aidl.tests.ITestService asInterface(android.os.IBinder obj)
    {
      if ((obj==null)) {
        return null;
      }
      android.os.IInterface iin = obj.queryLocalInterface(DESCRIPTOR);
      if (((iin!=null)&&(iin instanceof android.aidl.tests.ITestService))) {
        return ((android.aidl.tests.ITestService)iin);
      }
      return new android.aidl.tests.ITestService.Stub.Proxy(obj);
    }
    @Override public android.os.IBinder asBinder()
    {
      return this;
    }
    @Override public boolean onTransact(int code, android.os.Parcel data, android.os.Parcel reply, int flags) throws android.os.RemoteException
    {
      java.lang.String descriptor = DESCRIPTOR;
      switch (code)
      {
        case INTERFACE_TRANSACTION:
        {
          reply.writeString(descriptor);
          return true;
        }
      }
      switch (code)
      {
        case TRANSACTION_UnimplementedMethod:
        {
          data.enforceInterface(descriptor);
          int _arg0;
          _arg0 = data.readInt();
          int _result = this.UnimplementedMethod(_arg0);
          reply.writeNoException();
          reply.writeInt(_result);
          return true;
        }
        case TRANSACTION_Deprecated:
        {
          data.enforceInterface(descriptor);
          this.Deprecated();
          reply.writeNoException();
          return true;
        }
        case TRANSACTION_TestOneway:
        {
          data.enforceInterface(descriptor);
          this.TestOneway();
          return true;
        }
        case TRANSACTION_RepeatBoolean:
        {
          data.enforceInterface(descriptor);
          boolean _arg0;
          _arg0 = (0!=data.readInt());
          boolean _result = this.RepeatBoolean(_arg0);
          reply.writeNoException();
          reply.writeInt(((_result)?(1):(0)));
          return true;
        }
        case TRANSACTION_RepeatByte:
        {
          data.enforceInterface(descriptor);
          byte _arg0;
          _arg0 = data.readByte();
          byte _result = this.RepeatByte(_arg0);
          reply.writeNoException();
          reply.writeByte(_result);
          return true;
        }
        case TRANSACTION_RepeatChar:
        {
          data.enforceInterface(descriptor);
          char _arg0;
          _arg0 = (char)data.readInt();
          char _result = this.RepeatChar(_arg0);
          reply.writeNoException();
          reply.writeInt(((int)_result));
          return true;
        }
        case TRANSACTION_RepeatInt:
        {
          data.enforceInterface(descriptor);
          int _arg0;
          _arg0 = data.readInt();
          int _result = this.RepeatInt(_arg0);
          reply.writeNoException();
          reply.writeInt(_result);
          return true;
        }
        case TRANSACTION_RepeatLong:
        {
          data.enforceInterface(descriptor);
          long _arg0;
          _arg0 = data.readLong();
          long _result = this.RepeatLong(_arg0);
          reply.writeNoException();
          reply.writeLong(_result);
          return true;
        }
        case TRANSACTION_RepeatFloat:
        {
          data.enforceInterface(descriptor);
          float _arg0;
          _arg0 = data.readFloat();
          float _result = this.RepeatFloat(_arg0);
          reply.writeNoException();
          reply.writeFloat(_result);
          return true;
        }
        case TRANSACTION_RepeatDouble:
        {
          data.enforceInterface(descriptor);
          double _arg0;
          _arg0 = data.readDouble();
          double _result = this.RepeatDouble(_arg0);
          reply.writeNoException();
          reply.writeDouble(_result);
          return true;
        }
        case TRANSACTION_RepeatString:
        {
          data.enforceInterface(descriptor);
          java.lang.String _arg0;
          _arg0 = data.readString();
          java.lang.String _result = this.RepeatString(_arg0);
          reply.writeNoException();
          reply.writeString(_result);
          return true;
        }
        case TRANSACTION_RepeatByteEnum:
        {
          data.enforceInterface(descriptor);
          byte _arg0;
          _arg0 = data.readByte();
          byte _result = this.RepeatByteEnum(_arg0);
          reply.writeNoException();
          reply.writeByte(_result);
          return true;
        }
        case TRANSACTION_RepeatIntEnum:
        {
          data.enforceInterface(descriptor);
          int _arg0;
          _arg0 = data.readInt();
          int _result = this.RepeatIntEnum(_arg0);
          reply.writeNoException();
          reply.writeInt(_result);
          return true;
        }
        case TRANSACTION_RepeatLongEnum:
        {
          data.enforceInterface(descriptor);
          long _arg0;
          _arg0 = data.readLong();
          long _result = this.RepeatLongEnum(_arg0);
          reply.writeNoException();
          reply.writeLong(_result);
          return true;
        }
        case TRANSACTION_ReverseBoolean:
        {
          data.enforceInterface(descriptor);
          boolean[] _arg0;
          _arg0 = data.createBooleanArray();
          boolean[] _arg1;
          int _arg1_length = data.readInt();
          if ((_arg1_length<0)) {
            _arg1 = null;
          }
          else {
            _arg1 = new boolean[_arg1_length];
          }
          boolean[] _result = this.ReverseBoolean(_arg0, _arg1);
          reply.writeNoException();
          reply.writeBooleanArray(_result);
          reply.writeBooleanArray(_arg1);
          return true;
        }
        case TRANSACTION_ReverseByte:
        {
          data.enforceInterface(descriptor);
          byte[] _arg0;
          _arg0 = data.createByteArray();
          byte[] _arg1;
          int _arg1_length = data.readInt();
          if ((_arg1_length<0)) {
            _arg1 = null;
          }
          else {
            _arg1 = new byte[_arg1_length];
          }
          byte[] _result = this.ReverseByte(_arg0, _arg1);
          reply.writeNoException();
          reply.writeByteArray(_result);
          reply.writeByteArray(_arg1);
          return true;
        }
        case TRANSACTION_ReverseChar:
        {
          data.enforceInterface(descriptor);
          char[] _arg0;
          _arg0 = data.createCharArray();
          char[] _arg1;
          int _arg1_length = data.readInt();
          if ((_arg1_length<0)) {
            _arg1 = null;
          }
          else {
            _arg1 = new char[_arg1_length];
          }
          char[] _result = this.ReverseChar(_arg0, _arg1);
          reply.writeNoException();
          reply.writeCharArray(_result);
          reply.writeCharArray(_arg1);
          return true;
        }
        case TRANSACTION_ReverseInt:
        {
          data.enforceInterface(descriptor);
          int[] _arg0;
          _arg0 = data.createIntArray();
          int[] _arg1;
          int _arg1_length = data.readInt();
          if ((_arg1_length<0)) {
            _arg1 = null;
          }
          else {
            _arg1 = new int[_arg1_length];
          }
          int[] _result = this.ReverseInt(_arg0, _arg1);
          reply.writeNoException();
          reply.writeIntArray(_result);
          reply.writeIntArray(_arg1);
          return true;
        }
        case TRANSACTION_ReverseLong:
        {
          data.enforceInterface(descriptor);
          long[] _arg0;
          _arg0 = data.createLongArray();
          long[] _arg1;
          int _arg1_length = data.readInt();
          if ((_arg1_length<0)) {
            _arg1 = null;
          }
          else {
            _arg1 = new long[_arg1_length];
          }
          long[] _result = this.ReverseLong(_arg0, _arg1);
          reply.writeNoException();
          reply.writeLongArray(_result);
          reply.writeLongArray(_arg1);
          return true;
        }
        case TRANSACTION_ReverseFloat:
        {
          data.enforceInterface(descriptor);
          float[] _arg0;
          _arg0 = data.createFloatArray();
          float[] _arg1;
          int _arg1_length = data.readInt();
          if ((_arg1_length<0)) {
            _arg1 = null;
          }
          else {
            _arg1 = new float[_arg1_length];
          }
          float[] _result = this.ReverseFloat(_arg0, _arg1);
          reply.writeNoException();
          reply.writeFloatArray(_result);
          reply.writeFloatArray(_arg1);
          return true;
        }
        case TRANSACTION_ReverseDouble:
        {
          data.enforceInterface(descriptor);
          double[] _arg0;
          _arg0 = data.createDoubleArray();
          double[] _arg1;
          int _arg1_length = data.readInt();
          if ((_arg1_length<0)) {
            _arg1 = null;
          }
          else {
            _arg1 = new double[_arg1_length];
          }
          double[] _result = this.ReverseDouble(_arg0, _arg1);
          reply.writeNoException();
          reply.writeDoubleArray(_result);
          reply.writeDoubleArray(_arg1);
          return true;
        }
        case TRANSACTION_ReverseString:
        {
          data.enforceInterface(descriptor);
          java.lang.String[] _arg0;
          _arg0 = data.createStringArray();
          java.lang.String[] _arg1;
          int _arg1_length = data.readInt();
          if ((_arg1_length<0)) {
            _arg1 = null;
          }
          else {
            _arg1 = new java.lang.String[_arg1_length];
          }
          java.lang.String[] _result = this.ReverseString(_arg0, _arg1);
          reply.writeNoException();
          reply.writeStringArray(_result);
          reply.writeStringArray(_arg1);
          return true;
        }
        case TRANSACTION_ReverseByteEnum:
        {
          data.enforceInterface(descriptor);
          byte[] _arg0;
          _arg0 = data.createByteArray();
          byte[] _arg1;
          int _arg1_length = data.readInt();
          if ((_arg1_length<0)) {
            _arg1 = null;
          }
          else {
            _arg1 = new byte[_arg1_length];
          }
          byte[] _result = this.ReverseByteEnum(_arg0, _arg1);
          reply.writeNoException();
          reply.writeByteArray(_result);
          reply.writeByteArray(_arg1);
          return true;
        }
        case TRANSACTION_ReverseIntEnum:
        {
          data.enforceInterface(descriptor);
          int[] _arg0;
          _arg0 = data.createIntArray();
          int[] _arg1;
          int _arg1_length = data.readInt();
          if ((_arg1_length<0)) {
            _arg1 = null;
          }
          else {
            _arg1 = new int[_arg1_length];
          }
          int[] _result = this.ReverseIntEnum(_arg0, _arg1);
          reply.writeNoException();
          reply.writeIntArray(_result);
          reply.writeIntArray(_arg1);
          return true;
        }
        case TRANSACTION_ReverseLongEnum:
        {
          data.enforceInterface(descriptor);
          long[] _arg0;
          _arg0 = data.createLongArray();
          long[] _arg1;
          int _arg1_length = data.readInt();
          if ((_arg1_length<0)) {
            _arg1 = null;
          }
          else {
            _arg1 = new long[_arg1_length];
          }
          long[] _result = this.ReverseLongEnum(_arg0, _arg1);
          reply.writeNoException();
          reply.writeLongArray(_result);
          reply.writeLongArray(_arg1);
          return true;
        }
        case TRANSACTION_GetOtherTestService:
        {
          data.enforceInterface(descriptor);
          java.lang.String _arg0;
          _arg0 = data.readString();
          android.aidl.tests.INamedCallback _result = this.GetOtherTestService(_arg0);
          reply.writeNoException();
          reply.writeStrongBinder((((_result!=null))?(_result.asBinder()):(null)));
          return true;
        }
        case TRANSACTION_VerifyName:
        {
          data.enforceInterface(descriptor);
          android.aidl.tests.INamedCallback _arg0;
          _arg0 = android.aidl.tests.INamedCallback.Stub.asInterface(data.readStrongBinder());
          java.lang.String _arg1;
          _arg1 = data.readString();
          boolean _result = this.VerifyName(_arg0, _arg1);
          reply.writeNoException();
          reply.writeInt(((_result)?(1):(0)));
          return true;
        }
        case TRANSACTION_ReverseStringList:
        {
          data.enforceInterface(descriptor);
          java.util.List<java.lang.String> _arg0;
          _arg0 = data.createStringArrayList();
          java.util.List<java.lang.String> _arg1;
          _arg1 = new java.util.ArrayList<java.lang.String>();
          java.util.List<java.lang.String> _result = this.ReverseStringList(_arg0, _arg1);
          reply.writeNoException();
          reply.writeStringList(_result);
          reply.writeStringList(_arg1);
          return true;
        }
        case TRANSACTION_RepeatParcelFileDescriptor:
        {
          data.enforceInterface(descriptor);
          android.os.ParcelFileDescriptor _arg0;
          if ((0!=data.readInt())) {
            _arg0 = android.os.ParcelFileDescriptor.CREATOR.createFromParcel(data);
          }
          else {
            _arg0 = null;
          }
          android.os.ParcelFileDescriptor _result = this.RepeatParcelFileDescriptor(_arg0);
          reply.writeNoException();
          if ((_result!=null)) {
            reply.writeInt(1);
            _result.writeToParcel(reply, android.os.Parcelable.PARCELABLE_WRITE_RETURN_VALUE);
          }
          else {
            reply.writeInt(0);
          }
          return true;
        }
        case TRANSACTION_ReverseParcelFileDescriptorArray:
        {
          data.enforceInterface(descriptor);
          android.os.ParcelFileDescriptor[] _arg0;
          _arg0 = data.createTypedArray(android.os.ParcelFileDescriptor.CREATOR);
          android.os.ParcelFileDescriptor[] _arg1;
          int _arg1_length = data.readInt();
          if ((_arg1_length<0)) {
            _arg1 = null;
          }
          else {
            _arg1 = new android.os.ParcelFileDescriptor[_arg1_length];
          }
          android.os.ParcelFileDescriptor[] _result = this.ReverseParcelFileDescriptorArray(_arg0, _arg1);
          reply.writeNoException();
          reply.writeTypedArray(_result, android.os.Parcelable.PARCELABLE_WRITE_RETURN_VALUE);
          reply.writeTypedArray(_arg1, android.os.Parcelable.PARCELABLE_WRITE_RETURN_VALUE);
          return true;
        }
        case TRANSACTION_ThrowServiceException:
        {
          data.enforceInterface(descriptor);
          int _arg0;
          _arg0 = data.readInt();
          this.ThrowServiceException(_arg0);
          reply.writeNoException();
          return true;
        }
        case TRANSACTION_RepeatNullableIntArray:
        {
          data.enforceInterface(descriptor);
          int[] _arg0;
          _arg0 = data.createIntArray();
          int[] _result = this.RepeatNullableIntArray(_arg0);
          reply.writeNoException();
          reply.writeIntArray(_result);
          return true;
        }
        case TRANSACTION_RepeatNullableByteEnumArray:
        {
          data.enforceInterface(descriptor);
          byte[] _arg0;
          _arg0 = data.createByteArray();
          byte[] _result = this.RepeatNullableByteEnumArray(_arg0);
          reply.writeNoException();
          reply.writeByteArray(_result);
          return true;
        }
        case TRANSACTION_RepeatNullableIntEnumArray:
        {
          data.enforceInterface(descriptor);
          int[] _arg0;
          _arg0 = data.createIntArray();
          int[] _result = this.RepeatNullableIntEnumArray(_arg0);
          reply.writeNoException();
          reply.writeIntArray(_result);
          return true;
        }
        case TRANSACTION_RepeatNullableLongEnumArray:
        {
          data.enforceInterface(descriptor);
          long[] _arg0;
          _arg0 = data.createLongArray();
          long[] _result = this.RepeatNullableLongEnumArray(_arg0);
          reply.writeNoException();
          reply.writeLongArray(_result);
          return true;
        }
        case TRANSACTION_RepeatNullableString:
        {
          data.enforceInterface(descriptor);
          java.lang.String _arg0;
          _arg0 = data.readString();
          java.lang.String _result = this.RepeatNullableString(_arg0);
          reply.writeNoException();
          reply.writeString(_result);
          return true;
        }
        case TRANSACTION_RepeatNullableStringList:
        {
          data.enforceInterface(descriptor);
          java.util.List<java.lang.String> _arg0;
          _arg0 = data.createStringArrayList();
          java.util.List<java.lang.String> _result = this.RepeatNullableStringList(_arg0);
          reply.writeNoException();
          reply.writeStringList(_result);
          return true;
        }
        case TRANSACTION_RepeatNullableParcelable:
        {
          data.enforceInterface(descriptor);
          android.aidl.tests.StructuredParcelable _arg0;
          if ((0!=data.readInt())) {
            _arg0 = android.aidl.tests.StructuredParcelable.CREATOR.createFromParcel(data);
          }
          else {
            _arg0 = null;
          }
          android.aidl.tests.StructuredParcelable _result = this.RepeatNullableParcelable(_arg0);
          reply.writeNoException();
          if ((_result!=null)) {
            reply.writeInt(1);
            _result.writeToParcel(reply, android.os.Parcelable.PARCELABLE_WRITE_RETURN_VALUE);
          }
          else {
            reply.writeInt(0);
          }
          return true;
        }
        case TRANSACTION_TakesAnIBinder:
        {
          data.enforceInterface(descriptor);
          android.os.IBinder _arg0;
          _arg0 = data.readStrongBinder();
          this.TakesAnIBinder(_arg0);
          reply.writeNoException();
          return true;
        }
        case TRANSACTION_TakesANullableIBinder:
        {
          data.enforceInterface(descriptor);
          android.os.IBinder _arg0;
          _arg0 = data.readStrongBinder();
          this.TakesANullableIBinder(_arg0);
          reply.writeNoException();
          return true;
        }
        case TRANSACTION_RepeatUtf8CppString:
        {
          data.enforceInterface(descriptor);
          java.lang.String _arg0;
          _arg0 = data.readString();
          java.lang.String _result = this.RepeatUtf8CppString(_arg0);
          reply.writeNoException();
          reply.writeString(_result);
          return true;
        }
        case TRANSACTION_RepeatNullableUtf8CppString:
        {
          data.enforceInterface(descriptor);
          java.lang.String _arg0;
          _arg0 = data.readString();
          java.lang.String _result = this.RepeatNullableUtf8CppString(_arg0);
          reply.writeNoException();
          reply.writeString(_result);
          return true;
        }
        case TRANSACTION_ReverseUtf8CppString:
        {
          data.enforceInterface(descriptor);
          java.lang.String[] _arg0;
          _arg0 = data.createStringArray();
          java.lang.String[] _arg1;
          int _arg1_length = data.readInt();
          if ((_arg1_length<0)) {
            _arg1 = null;
          }
          else {
            _arg1 = new java.lang.String[_arg1_length];
          }
          java.lang.String[] _result = this.ReverseUtf8CppString(_arg0, _arg1);
          reply.writeNoException();
          reply.writeStringArray(_result);
          reply.writeStringArray(_arg1);
          return true;
        }
        case TRANSACTION_ReverseNullableUtf8CppString:
        {
          data.enforceInterface(descriptor);
          java.lang.String[] _arg0;
          _arg0 = data.createStringArray();
          java.lang.String[] _arg1;
          int _arg1_length = data.readInt();
          if ((_arg1_length<0)) {
            _arg1 = null;
          }
          else {
            _arg1 = new java.lang.String[_arg1_length];
          }
          java.lang.String[] _result = this.ReverseNullableUtf8CppString(_arg0, _arg1);
          reply.writeNoException();
          reply.writeStringArray(_result);
          reply.writeStringArray(_arg1);
          return true;
        }
        case TRANSACTION_ReverseUtf8CppStringList:
        {
          data.enforceInterface(descriptor);
          java.util.List<java.lang.String> _arg0;
          _arg0 = data.createStringArrayList();
          java.util.List<java.lang.String> _arg1;
          _arg1 = new java.util.ArrayList<java.lang.String>();
          java.util.List<java.lang.String> _result = this.ReverseUtf8CppStringList(_arg0, _arg1);
          reply.writeNoException();
          reply.writeStringList(_result);
          reply.writeStringList(_arg1);
          return true;
        }
        case TRANSACTION_GetCallback:
        {
          data.enforceInterface(descriptor);
          boolean _arg0;
          _arg0 = (0!=data.readInt());
          android.aidl.tests.INamedCallback _result = this.GetCallback(_arg0);
          reply.writeNoException();
          reply.writeStrongBinder((((_result!=null))?(_result.asBinder()):(null)));
          return true;
        }
        case TRANSACTION_FillOutStructuredParcelable:
        {
          data.enforceInterface(descriptor);
          android.aidl.tests.StructuredParcelable _arg0;
          if ((0!=data.readInt())) {
            _arg0 = android.aidl.tests.StructuredParcelable.CREATOR.createFromParcel(data);
          }
          else {
            _arg0 = null;
          }
          this.FillOutStructuredParcelable(_arg0);
          reply.writeNoException();
          if ((_arg0!=null)) {
            reply.writeInt(1);
            _arg0.writeToParcel(reply, android.os.Parcelable.PARCELABLE_WRITE_RETURN_VALUE);
          }
          else {
            reply.writeInt(0);
          }
          return true;
        }
        case TRANSACTION_GetOldNameInterface:
        {
          data.enforceInterface(descriptor);
          android.aidl.tests.IOldName _result = this.GetOldNameInterface();
          reply.writeNoException();
          reply.writeStrongBinder((((_result!=null))?(_result.asBinder()):(null)));
          return true;
        }
        case TRANSACTION_GetNewNameInterface:
        {
          data.enforceInterface(descriptor);
          android.aidl.tests.INewName _result = this.GetNewNameInterface();
          reply.writeNoException();
          reply.writeStrongBinder((((_result!=null))?(_result.asBinder()):(null)));
          return true;
        }
        case TRANSACTION_GetCppJavaTests:
        {
          data.enforceInterface(descriptor);
          android.os.IBinder _result = this.GetCppJavaTests();
          reply.writeNoException();
          reply.writeStrongBinder(_result);
          return true;
        }
        case TRANSACTION_getBackendType:
        {
          data.enforceInterface(descriptor);
          byte _result = this.getBackendType();
          reply.writeNoException();
          reply.writeByte(_result);
          return true;
        }
        default:
        {
          return super.onTransact(code, data, reply, flags);
        }
      }
    }
    private static class Proxy implements android.aidl.tests.ITestService
    {
      private android.os.IBinder mRemote;
      Proxy(android.os.IBinder remote)
      {
        mRemote = remote;
      }
      @Override public android.os.IBinder asBinder()
      {
        return mRemote;
      }
      public java.lang.String getInterfaceDescriptor()
      {
        return DESCRIPTOR;
      }
      // This is to emulate a method that is added after the service is implemented.
      // So the client cannot assume that a call to this method will be successful
      // or not. However, inside the test environment, we can't build client and
      // the server with different version of this AIDL file. So, we let the server
      // actually implement this and intercept the dispatch to the method
      // inside onTransact().
      // WARNING: Must be first method.
      // This requires hard coding the transaction number. As long as this method is
      // the first in this interface, it can keep the
      // "::android::IBinder::FIRST_CALL_TRANSACTION + 0" value and allow
      // methods to be added and removed.
      @Override public int UnimplementedMethod(int arg) throws android.os.RemoteException
      {
        android.os.Parcel _data = android.os.Parcel.obtain();
        _data.markSensitive();android.os.Parcel _reply = android.os.Parcel.obtain();
        int _result;
        try {
          _data.writeInterfaceToken(DESCRIPTOR);
          _data.writeInt(arg);
          boolean _status = mRemote.transact(Stub.TRANSACTION_UnimplementedMethod, _data, _reply, android.os.IBinder.FLAG_CLEAR_BUF);
          if (!_status) {
            if (getDefaultImpl() != null) {
              return getDefaultImpl().UnimplementedMethod(arg);
            }
          }
          _reply.readException();
          _result = _reply.readInt();
        }
        finally {
          _reply.recycle();
          _data.recycle();
        }
        return _result;
      }
      /**
           * @deprecated to make sure we have something in system/tools/aidl which does a compile check
           *     of deprecated and make sure this is reflected in goldens
           */
      @Override public void Deprecated() throws android.os.RemoteException
      {
        android.os.Parcel _data = android.os.Parcel.obtain();
        _data.markSensitive();android.os.Parcel _reply = android.os.Parcel.obtain();
        try {
          _data.writeInterfaceToken(DESCRIPTOR);
          boolean _status = mRemote.transact(Stub.TRANSACTION_Deprecated, _data, _reply, android.os.IBinder.FLAG_CLEAR_BUF);
          if (!_status) {
            if (getDefaultImpl() != null) {
              getDefaultImpl().Deprecated();
              return;
            }
          }
          _reply.readException();
        }
        finally {
          _reply.recycle();
          _data.recycle();
        }
      }
      @Override public void TestOneway() throws android.os.RemoteException
      {
        android.os.Parcel _data = android.os.Parcel.obtain();
        _data.markSensitive();try {
          _data.writeInterfaceToken(DESCRIPTOR);
          boolean _status = mRemote.transact(Stub.TRANSACTION_TestOneway, _data, null, android.os.IBinder.FLAG_ONEWAY | android.os.IBinder.FLAG_CLEAR_BUF);
          if (!_status) {
            if (getDefaultImpl() != null) {
              getDefaultImpl().TestOneway();
              return;
            }
          }
        }
        finally {
          _data.recycle();
        }
      }
      // Test that primitives work as parameters and return types.
      @Override public boolean RepeatBoolean(boolean token) throws android.os.RemoteException
      {
        android.os.Parcel _data = android.os.Parcel.obtain();
        _data.markSensitive();android.os.Parcel _reply = android.os.Parcel.obtain();
        boolean _result;
        try {
          _data.writeInterfaceToken(DESCRIPTOR);
          _data.writeInt(((token)?(1):(0)));
          boolean _status = mRemote.transact(Stub.TRANSACTION_RepeatBoolean, _data, _reply, android.os.IBinder.FLAG_CLEAR_BUF);
          if (!_status) {
            if (getDefaultImpl() != null) {
              return getDefaultImpl().RepeatBoolean(token);
            }
          }
          _reply.readException();
          _result = (0!=_reply.readInt());
        }
        finally {
          _reply.recycle();
          _data.recycle();
        }
        return _result;
      }
      @Override public byte RepeatByte(byte token) throws android.os.RemoteException
      {
        android.os.Parcel _data = android.os.Parcel.obtain();
        _data.markSensitive();android.os.Parcel _reply = android.os.Parcel.obtain();
        byte _result;
        try {
          _data.writeInterfaceToken(DESCRIPTOR);
          _data.writeByte(token);
          boolean _status = mRemote.transact(Stub.TRANSACTION_RepeatByte, _data, _reply, android.os.IBinder.FLAG_CLEAR_BUF);
          if (!_status) {
            if (getDefaultImpl() != null) {
              return getDefaultImpl().RepeatByte(token);
            }
          }
          _reply.readException();
          _result = _reply.readByte();
        }
        finally {
          _reply.recycle();
          _data.recycle();
        }
        return _result;
      }
      @Override public char RepeatChar(char token) throws android.os.RemoteException
      {
        android.os.Parcel _data = android.os.Parcel.obtain();
        _data.markSensitive();android.os.Parcel _reply = android.os.Parcel.obtain();
        char _result;
        try {
          _data.writeInterfaceToken(DESCRIPTOR);
          _data.writeInt(((int)token));
          boolean _status = mRemote.transact(Stub.TRANSACTION_RepeatChar, _data, _reply, android.os.IBinder.FLAG_CLEAR_BUF);
          if (!_status) {
            if (getDefaultImpl() != null) {
              return getDefaultImpl().RepeatChar(token);
            }
          }
          _reply.readException();
          _result = (char)_reply.readInt();
        }
        finally {
          _reply.recycle();
          _data.recycle();
        }
        return _result;
      }
      @Override public int RepeatInt(int token) throws android.os.RemoteException
      {
        android.os.Parcel _data = android.os.Parcel.obtain();
        _data.markSensitive();android.os.Parcel _reply = android.os.Parcel.obtain();
        int _result;
        try {
          _data.writeInterfaceToken(DESCRIPTOR);
          _data.writeInt(token);
          boolean _status = mRemote.transact(Stub.TRANSACTION_RepeatInt, _data, _reply, android.os.IBinder.FLAG_CLEAR_BUF);
          if (!_status) {
            if (getDefaultImpl() != null) {
              return getDefaultImpl().RepeatInt(token);
            }
          }
          _reply.readException();
          _result = _reply.readInt();
        }
        finally {
          _reply.recycle();
          _data.recycle();
        }
        return _result;
      }
      @Override public long RepeatLong(long token) throws android.os.RemoteException
      {
        android.os.Parcel _data = android.os.Parcel.obtain();
        _data.markSensitive();android.os.Parcel _reply = android.os.Parcel.obtain();
        long _result;
        try {
          _data.writeInterfaceToken(DESCRIPTOR);
          _data.writeLong(token);
          boolean _status = mRemote.transact(Stub.TRANSACTION_RepeatLong, _data, _reply, android.os.IBinder.FLAG_CLEAR_BUF);
          if (!_status) {
            if (getDefaultImpl() != null) {
              return getDefaultImpl().RepeatLong(token);
            }
          }
          _reply.readException();
          _result = _reply.readLong();
        }
        finally {
          _reply.recycle();
          _data.recycle();
        }
        return _result;
      }
      @Override public float RepeatFloat(float token) throws android.os.RemoteException
      {
        android.os.Parcel _data = android.os.Parcel.obtain();
        _data.markSensitive();android.os.Parcel _reply = android.os.Parcel.obtain();
        float _result;
        try {
          _data.writeInterfaceToken(DESCRIPTOR);
          _data.writeFloat(token);
          boolean _status = mRemote.transact(Stub.TRANSACTION_RepeatFloat, _data, _reply, android.os.IBinder.FLAG_CLEAR_BUF);
          if (!_status) {
            if (getDefaultImpl() != null) {
              return getDefaultImpl().RepeatFloat(token);
            }
          }
          _reply.readException();
          _result = _reply.readFloat();
        }
        finally {
          _reply.recycle();
          _data.recycle();
        }
        return _result;
      }
      @Override public double RepeatDouble(double token) throws android.os.RemoteException
      {
        android.os.Parcel _data = android.os.Parcel.obtain();
        _data.markSensitive();android.os.Parcel _reply = android.os.Parcel.obtain();
        double _result;
        try {
          _data.writeInterfaceToken(DESCRIPTOR);
          _data.writeDouble(token);
          boolean _status = mRemote.transact(Stub.TRANSACTION_RepeatDouble, _data, _reply, android.os.IBinder.FLAG_CLEAR_BUF);
          if (!_status) {
            if (getDefaultImpl() != null) {
              return getDefaultImpl().RepeatDouble(token);
            }
          }
          _reply.readException();
          _result = _reply.readDouble();
        }
        finally {
          _reply.recycle();
          _data.recycle();
        }
        return _result;
      }
      @Override public java.lang.String RepeatString(java.lang.String token) throws android.os.RemoteException
      {
        android.os.Parcel _data = android.os.Parcel.obtain();
        _data.markSensitive();android.os.Parcel _reply = android.os.Parcel.obtain();
        java.lang.String _result;
        try {
          _data.writeInterfaceToken(DESCRIPTOR);
          _data.writeString(token);
          boolean _status = mRemote.transact(Stub.TRANSACTION_RepeatString, _data, _reply, android.os.IBinder.FLAG_CLEAR_BUF);
          if (!_status) {
            if (getDefaultImpl() != null) {
              return getDefaultImpl().RepeatString(token);
            }
          }
          _reply.readException();
          _result = _reply.readString();
        }
        finally {
          _reply.recycle();
          _data.recycle();
        }
        return _result;
      }
      @Override public byte RepeatByteEnum(byte token) throws android.os.RemoteException
      {
        android.os.Parcel _data = android.os.Parcel.obtain();
        _data.markSensitive();android.os.Parcel _reply = android.os.Parcel.obtain();
        byte _result;
        try {
          _data.writeInterfaceToken(DESCRIPTOR);
          _data.writeByte(token);
          boolean _status = mRemote.transact(Stub.TRANSACTION_RepeatByteEnum, _data, _reply, android.os.IBinder.FLAG_CLEAR_BUF);
          if (!_status) {
            if (getDefaultImpl() != null) {
              return getDefaultImpl().RepeatByteEnum(token);
            }
          }
          _reply.readException();
          _result = _reply.readByte();
        }
        finally {
          _reply.recycle();
          _data.recycle();
        }
        return _result;
      }
      @Override public int RepeatIntEnum(int token) throws android.os.RemoteException
      {
        android.os.Parcel _data = android.os.Parcel.obtain();
        _data.markSensitive();android.os.Parcel _reply = android.os.Parcel.obtain();
        int _result;
        try {
          _data.writeInterfaceToken(DESCRIPTOR);
          _data.writeInt(token);
          boolean _status = mRemote.transact(Stub.TRANSACTION_RepeatIntEnum, _data, _reply, android.os.IBinder.FLAG_CLEAR_BUF);
          if (!_status) {
            if (getDefaultImpl() != null) {
              return getDefaultImpl().RepeatIntEnum(token);
            }
          }
          _reply.readException();
          _result = _reply.readInt();
        }
        finally {
          _reply.recycle();
          _data.recycle();
        }
        return _result;
      }
      @Override public long RepeatLongEnum(long token) throws android.os.RemoteException
      {
        android.os.Parcel _data = android.os.Parcel.obtain();
        _data.markSensitive();android.os.Parcel _reply = android.os.Parcel.obtain();
        long _result;
        try {
          _data.writeInterfaceToken(DESCRIPTOR);
          _data.writeLong(token);
          boolean _status = mRemote.transact(Stub.TRANSACTION_RepeatLongEnum, _data, _reply, android.os.IBinder.FLAG_CLEAR_BUF);
          if (!_status) {
            if (getDefaultImpl() != null) {
              return getDefaultImpl().RepeatLongEnum(token);
            }
          }
          _reply.readException();
          _result = _reply.readLong();
        }
        finally {
          _reply.recycle();
          _data.recycle();
        }
        return _result;
      }
      // Test that arrays work as parameters and return types.
      @Override public boolean[] ReverseBoolean(boolean[] input, boolean[] repeated) throws android.os.RemoteException
      {
        android.os.Parcel _data = android.os.Parcel.obtain();
        _data.markSensitive();android.os.Parcel _reply = android.os.Parcel.obtain();
        boolean[] _result;
        try {
          _data.writeInterfaceToken(DESCRIPTOR);
          _data.writeBooleanArray(input);
          if ((repeated==null)) {
            _data.writeInt(-1);
          }
          else {
            _data.writeInt(repeated.length);
          }
          boolean _status = mRemote.transact(Stub.TRANSACTION_ReverseBoolean, _data, _reply, android.os.IBinder.FLAG_CLEAR_BUF);
          if (!_status) {
            if (getDefaultImpl() != null) {
              return getDefaultImpl().ReverseBoolean(input, repeated);
            }
          }
          _reply.readException();
          _result = _reply.createBooleanArray();
          _reply.readBooleanArray(repeated);
        }
        finally {
          _reply.recycle();
          _data.recycle();
        }
        return _result;
      }
      @Override public byte[] ReverseByte(byte[] input, byte[] repeated) throws android.os.RemoteException
      {
        android.os.Parcel _data = android.os.Parcel.obtain();
        _data.markSensitive();android.os.Parcel _reply = android.os.Parcel.obtain();
        byte[] _result;
        try {
          _data.writeInterfaceToken(DESCRIPTOR);
          _data.writeByteArray(input);
          if ((repeated==null)) {
            _data.writeInt(-1);
          }
          else {
            _data.writeInt(repeated.length);
          }
          boolean _status = mRemote.transact(Stub.TRANSACTION_ReverseByte, _data, _reply, android.os.IBinder.FLAG_CLEAR_BUF);
          if (!_status) {
            if (getDefaultImpl() != null) {
              return getDefaultImpl().ReverseByte(input, repeated);
            }
          }
          _reply.readException();
          _result = _reply.createByteArray();
          _reply.readByteArray(repeated);
        }
        finally {
          _reply.recycle();
          _data.recycle();
        }
        return _result;
      }
      @Override public char[] ReverseChar(char[] input, char[] repeated) throws android.os.RemoteException
      {
        android.os.Parcel _data = android.os.Parcel.obtain();
        _data.markSensitive();android.os.Parcel _reply = android.os.Parcel.obtain();
        char[] _result;
        try {
          _data.writeInterfaceToken(DESCRIPTOR);
          _data.writeCharArray(input);
          if ((repeated==null)) {
            _data.writeInt(-1);
          }
          else {
            _data.writeInt(repeated.length);
          }
          boolean _status = mRemote.transact(Stub.TRANSACTION_ReverseChar, _data, _reply, android.os.IBinder.FLAG_CLEAR_BUF);
          if (!_status) {
            if (getDefaultImpl() != null) {
              return getDefaultImpl().ReverseChar(input, repeated);
            }
          }
          _reply.readException();
          _result = _reply.createCharArray();
          _reply.readCharArray(repeated);
        }
        finally {
          _reply.recycle();
          _data.recycle();
        }
        return _result;
      }
      @Override public int[] ReverseInt(int[] input, int[] repeated) throws android.os.RemoteException
      {
        android.os.Parcel _data = android.os.Parcel.obtain();
        _data.markSensitive();android.os.Parcel _reply = android.os.Parcel.obtain();
        int[] _result;
        try {
          _data.writeInterfaceToken(DESCRIPTOR);
          _data.writeIntArray(input);
          if ((repeated==null)) {
            _data.writeInt(-1);
          }
          else {
            _data.writeInt(repeated.length);
          }
          boolean _status = mRemote.transact(Stub.TRANSACTION_ReverseInt, _data, _reply, android.os.IBinder.FLAG_CLEAR_BUF);
          if (!_status) {
            if (getDefaultImpl() != null) {
              return getDefaultImpl().ReverseInt(input, repeated);
            }
          }
          _reply.readException();
          _result = _reply.createIntArray();
          _reply.readIntArray(repeated);
        }
        finally {
          _reply.recycle();
          _data.recycle();
        }
        return _result;
      }
      @Override public long[] ReverseLong(long[] input, long[] repeated) throws android.os.RemoteException
      {
        android.os.Parcel _data = android.os.Parcel.obtain();
        _data.markSensitive();android.os.Parcel _reply = android.os.Parcel.obtain();
        long[] _result;
        try {
          _data.writeInterfaceToken(DESCRIPTOR);
          _data.writeLongArray(input);
          if ((repeated==null)) {
            _data.writeInt(-1);
          }
          else {
            _data.writeInt(repeated.length);
          }
          boolean _status = mRemote.transact(Stub.TRANSACTION_ReverseLong, _data, _reply, android.os.IBinder.FLAG_CLEAR_BUF);
          if (!_status) {
            if (getDefaultImpl() != null) {
              return getDefaultImpl().ReverseLong(input, repeated);
            }
          }
          _reply.readException();
          _result = _reply.createLongArray();
          _reply.readLongArray(repeated);
        }
        finally {
          _reply.recycle();
          _data.recycle();
        }
        return _result;
      }
      @Override public float[] ReverseFloat(float[] input, float[] repeated) throws android.os.RemoteException
      {
        android.os.Parcel _data = android.os.Parcel.obtain();
        _data.markSensitive();android.os.Parcel _reply = android.os.Parcel.obtain();
        float[] _result;
        try {
          _data.writeInterfaceToken(DESCRIPTOR);
          _data.writeFloatArray(input);
          if ((repeated==null)) {
            _data.writeInt(-1);
          }
          else {
            _data.writeInt(repeated.length);
          }
          boolean _status = mRemote.transact(Stub.TRANSACTION_ReverseFloat, _data, _reply, android.os.IBinder.FLAG_CLEAR_BUF);
          if (!_status) {
            if (getDefaultImpl() != null) {
              return getDefaultImpl().ReverseFloat(input, repeated);
            }
          }
          _reply.readException();
          _result = _reply.createFloatArray();
          _reply.readFloatArray(repeated);
        }
        finally {
          _reply.recycle();
          _data.recycle();
        }
        return _result;
      }
      @Override public double[] ReverseDouble(double[] input, double[] repeated) throws android.os.RemoteException
      {
        android.os.Parcel _data = android.os.Parcel.obtain();
        _data.markSensitive();android.os.Parcel _reply = android.os.Parcel.obtain();
        double[] _result;
        try {
          _data.writeInterfaceToken(DESCRIPTOR);
          _data.writeDoubleArray(input);
          if ((repeated==null)) {
            _data.writeInt(-1);
          }
          else {
            _data.writeInt(repeated.length);
          }
          boolean _status = mRemote.transact(Stub.TRANSACTION_ReverseDouble, _data, _reply, android.os.IBinder.FLAG_CLEAR_BUF);
          if (!_status) {
            if (getDefaultImpl() != null) {
              return getDefaultImpl().ReverseDouble(input, repeated);
            }
          }
          _reply.readException();
          _result = _reply.createDoubleArray();
          _reply.readDoubleArray(repeated);
        }
        finally {
          _reply.recycle();
          _data.recycle();
        }
        return _result;
      }
      @Override public java.lang.String[] ReverseString(java.lang.String[] input, java.lang.String[] repeated) throws android.os.RemoteException
      {
        android.os.Parcel _data = android.os.Parcel.obtain();
        _data.markSensitive();android.os.Parcel _reply = android.os.Parcel.obtain();
        java.lang.String[] _result;
        try {
          _data.writeInterfaceToken(DESCRIPTOR);
          _data.writeStringArray(input);
          if ((repeated==null)) {
            _data.writeInt(-1);
          }
          else {
            _data.writeInt(repeated.length);
          }
          boolean _status = mRemote.transact(Stub.TRANSACTION_ReverseString, _data, _reply, android.os.IBinder.FLAG_CLEAR_BUF);
          if (!_status) {
            if (getDefaultImpl() != null) {
              return getDefaultImpl().ReverseString(input, repeated);
            }
          }
          _reply.readException();
          _result = _reply.createStringArray();
          _reply.readStringArray(repeated);
        }
        finally {
          _reply.recycle();
          _data.recycle();
        }
        return _result;
      }
      @Override public byte[] ReverseByteEnum(byte[] input, byte[] repeated) throws android.os.RemoteException
      {
        android.os.Parcel _data = android.os.Parcel.obtain();
        _data.markSensitive();android.os.Parcel _reply = android.os.Parcel.obtain();
        byte[] _result;
        try {
          _data.writeInterfaceToken(DESCRIPTOR);
          _data.writeByteArray(input);
          if ((repeated==null)) {
            _data.writeInt(-1);
          }
          else {
            _data.writeInt(repeated.length);
          }
          boolean _status = mRemote.transact(Stub.TRANSACTION_ReverseByteEnum, _data, _reply, android.os.IBinder.FLAG_CLEAR_BUF);
          if (!_status) {
            if (getDefaultImpl() != null) {
              return getDefaultImpl().ReverseByteEnum(input, repeated);
            }
          }
          _reply.readException();
          _result = _reply.createByteArray();
          _reply.readByteArray(repeated);
        }
        finally {
          _reply.recycle();
          _data.recycle();
        }
        return _result;
      }
      @Override public int[] ReverseIntEnum(int[] input, int[] repeated) throws android.os.RemoteException
      {
        android.os.Parcel _data = android.os.Parcel.obtain();
        _data.markSensitive();android.os.Parcel _reply = android.os.Parcel.obtain();
        int[] _result;
        try {
          _data.writeInterfaceToken(DESCRIPTOR);
          _data.writeIntArray(input);
          if ((repeated==null)) {
            _data.writeInt(-1);
          }
          else {
            _data.writeInt(repeated.length);
          }
          boolean _status = mRemote.transact(Stub.TRANSACTION_ReverseIntEnum, _data, _reply, android.os.IBinder.FLAG_CLEAR_BUF);
          if (!_status) {
            if (getDefaultImpl() != null) {
              return getDefaultImpl().ReverseIntEnum(input, repeated);
            }
          }
          _reply.readException();
          _result = _reply.createIntArray();
          _reply.readIntArray(repeated);
        }
        finally {
          _reply.recycle();
          _data.recycle();
        }
        return _result;
      }
      @Override public long[] ReverseLongEnum(long[] input, long[] repeated) throws android.os.RemoteException
      {
        android.os.Parcel _data = android.os.Parcel.obtain();
        _data.markSensitive();android.os.Parcel _reply = android.os.Parcel.obtain();
        long[] _result;
        try {
          _data.writeInterfaceToken(DESCRIPTOR);
          _data.writeLongArray(input);
          if ((repeated==null)) {
            _data.writeInt(-1);
          }
          else {
            _data.writeInt(repeated.length);
          }
          boolean _status = mRemote.transact(Stub.TRANSACTION_ReverseLongEnum, _data, _reply, android.os.IBinder.FLAG_CLEAR_BUF);
          if (!_status) {
            if (getDefaultImpl() != null) {
              return getDefaultImpl().ReverseLongEnum(input, repeated);
            }
          }
          _reply.readException();
          _result = _reply.createLongArray();
          _reply.readLongArray(repeated);
        }
        finally {
          _reply.recycle();
          _data.recycle();
        }
        return _result;
      }
      // Test that clients can send and receive Binders.
      @Override public android.aidl.tests.INamedCallback GetOtherTestService(java.lang.String name) throws android.os.RemoteException
      {
        android.os.Parcel _data = android.os.Parcel.obtain();
        _data.markSensitive();android.os.Parcel _reply = android.os.Parcel.obtain();
        android.aidl.tests.INamedCallback _result;
        try {
          _data.writeInterfaceToken(DESCRIPTOR);
          _data.writeString(name);
          boolean _status = mRemote.transact(Stub.TRANSACTION_GetOtherTestService, _data, _reply, android.os.IBinder.FLAG_CLEAR_BUF);
          if (!_status) {
            if (getDefaultImpl() != null) {
              return getDefaultImpl().GetOtherTestService(name);
            }
          }
          _reply.readException();
          _result = android.aidl.tests.INamedCallback.Stub.asInterface(_reply.readStrongBinder());
        }
        finally {
          _reply.recycle();
          _data.recycle();
        }
        return _result;
      }
      @Override public boolean VerifyName(android.aidl.tests.INamedCallback service, java.lang.String name) throws android.os.RemoteException
      {
        android.os.Parcel _data = android.os.Parcel.obtain();
        _data.markSensitive();android.os.Parcel _reply = android.os.Parcel.obtain();
        boolean _result;
        try {
          _data.writeInterfaceToken(DESCRIPTOR);
          _data.writeStrongBinder((((service!=null))?(service.asBinder()):(null)));
          _data.writeString(name);
          boolean _status = mRemote.transact(Stub.TRANSACTION_VerifyName, _data, _reply, android.os.IBinder.FLAG_CLEAR_BUF);
          if (!_status) {
            if (getDefaultImpl() != null) {
              return getDefaultImpl().VerifyName(service, name);
            }
          }
          _reply.readException();
          _result = (0!=_reply.readInt());
        }
        finally {
          _reply.recycle();
          _data.recycle();
        }
        return _result;
      }
      // Test that List<T> types work correctly.
      @Override public java.util.List<java.lang.String> ReverseStringList(java.util.List<java.lang.String> input, java.util.List<java.lang.String> repeated) throws android.os.RemoteException
      {
        android.os.Parcel _data = android.os.Parcel.obtain();
        _data.markSensitive();android.os.Parcel _reply = android.os.Parcel.obtain();
        java.util.List<java.lang.String> _result;
        try {
          _data.writeInterfaceToken(DESCRIPTOR);
          _data.writeStringList(input);
          boolean _status = mRemote.transact(Stub.TRANSACTION_ReverseStringList, _data, _reply, android.os.IBinder.FLAG_CLEAR_BUF);
          if (!_status) {
            if (getDefaultImpl() != null) {
              return getDefaultImpl().ReverseStringList(input, repeated);
            }
          }
          _reply.readException();
          _result = _reply.createStringArrayList();
          _reply.readStringList(repeated);
        }
        finally {
          _reply.recycle();
          _data.recycle();
        }
        return _result;
      }
      @Override public android.os.ParcelFileDescriptor RepeatParcelFileDescriptor(android.os.ParcelFileDescriptor read) throws android.os.RemoteException
      {
        android.os.Parcel _data = android.os.Parcel.obtain();
        _data.markSensitive();android.os.Parcel _reply = android.os.Parcel.obtain();
        android.os.ParcelFileDescriptor _result;
        try {
          _data.writeInterfaceToken(DESCRIPTOR);
          if ((read!=null)) {
            _data.writeInt(1);
            read.writeToParcel(_data, 0);
          }
          else {
            _data.writeInt(0);
          }
          boolean _status = mRemote.transact(Stub.TRANSACTION_RepeatParcelFileDescriptor, _data, _reply, android.os.IBinder.FLAG_CLEAR_BUF);
          if (!_status) {
            if (getDefaultImpl() != null) {
              return getDefaultImpl().RepeatParcelFileDescriptor(read);
            }
          }
          _reply.readException();
          if ((0!=_reply.readInt())) {
            _result = android.os.ParcelFileDescriptor.CREATOR.createFromParcel(_reply);
          }
          else {
            _result = null;
          }
        }
        finally {
          _reply.recycle();
          _data.recycle();
        }
        return _result;
      }
      @Override public android.os.ParcelFileDescriptor[] ReverseParcelFileDescriptorArray(android.os.ParcelFileDescriptor[] input, android.os.ParcelFileDescriptor[] repeated) throws android.os.RemoteException
      {
        android.os.Parcel _data = android.os.Parcel.obtain();
        _data.markSensitive();android.os.Parcel _reply = android.os.Parcel.obtain();
        android.os.ParcelFileDescriptor[] _result;
        try {
          _data.writeInterfaceToken(DESCRIPTOR);
          _data.writeTypedArray(input, 0);
          if ((repeated==null)) {
            _data.writeInt(-1);
          }
          else {
            _data.writeInt(repeated.length);
          }
          boolean _status = mRemote.transact(Stub.TRANSACTION_ReverseParcelFileDescriptorArray, _data, _reply, android.os.IBinder.FLAG_CLEAR_BUF);
          if (!_status) {
            if (getDefaultImpl() != null) {
              return getDefaultImpl().ReverseParcelFileDescriptorArray(input, repeated);
            }
          }
          _reply.readException();
          _result = _reply.createTypedArray(android.os.ParcelFileDescriptor.CREATOR);
          _reply.readTypedArray(repeated, android.os.ParcelFileDescriptor.CREATOR);
        }
        finally {
          _reply.recycle();
          _data.recycle();
        }
        return _result;
      }
      // Test that service specific exceptions work correctly.
      @Override public void ThrowServiceException(int code) throws android.os.RemoteException
      {
        android.os.Parcel _data = android.os.Parcel.obtain();
        _data.markSensitive();android.os.Parcel _reply = android.os.Parcel.obtain();
        try {
          _data.writeInterfaceToken(DESCRIPTOR);
          _data.writeInt(code);
          boolean _status = mRemote.transact(Stub.TRANSACTION_ThrowServiceException, _data, _reply, android.os.IBinder.FLAG_CLEAR_BUF);
          if (!_status) {
            if (getDefaultImpl() != null) {
              getDefaultImpl().ThrowServiceException(code);
              return;
            }
          }
          _reply.readException();
        }
        finally {
          _reply.recycle();
          _data.recycle();
        }
      }
      // Test nullability
      @Override public int[] RepeatNullableIntArray(int[] input) throws android.os.RemoteException
      {
        android.os.Parcel _data = android.os.Parcel.obtain();
        _data.markSensitive();android.os.Parcel _reply = android.os.Parcel.obtain();
        int[] _result;
        try {
          _data.writeInterfaceToken(DESCRIPTOR);
          _data.writeIntArray(input);
          boolean _status = mRemote.transact(Stub.TRANSACTION_RepeatNullableIntArray, _data, _reply, android.os.IBinder.FLAG_CLEAR_BUF);
          if (!_status) {
            if (getDefaultImpl() != null) {
              return getDefaultImpl().RepeatNullableIntArray(input);
            }
          }
          _reply.readException();
          _result = _reply.createIntArray();
        }
        finally {
          _reply.recycle();
          _data.recycle();
        }
        return _result;
      }
      @Override public byte[] RepeatNullableByteEnumArray(byte[] input) throws android.os.RemoteException
      {
        android.os.Parcel _data = android.os.Parcel.obtain();
        _data.markSensitive();android.os.Parcel _reply = android.os.Parcel.obtain();
        byte[] _result;
        try {
          _data.writeInterfaceToken(DESCRIPTOR);
          _data.writeByteArray(input);
          boolean _status = mRemote.transact(Stub.TRANSACTION_RepeatNullableByteEnumArray, _data, _reply, android.os.IBinder.FLAG_CLEAR_BUF);
          if (!_status) {
            if (getDefaultImpl() != null) {
              return getDefaultImpl().RepeatNullableByteEnumArray(input);
            }
          }
          _reply.readException();
          _result = _reply.createByteArray();
        }
        finally {
          _reply.recycle();
          _data.recycle();
        }
        return _result;
      }
      @Override public int[] RepeatNullableIntEnumArray(int[] input) throws android.os.RemoteException
      {
        android.os.Parcel _data = android.os.Parcel.obtain();
        _data.markSensitive();android.os.Parcel _reply = android.os.Parcel.obtain();
        int[] _result;
        try {
          _data.writeInterfaceToken(DESCRIPTOR);
          _data.writeIntArray(input);
          boolean _status = mRemote.transact(Stub.TRANSACTION_RepeatNullableIntEnumArray, _data, _reply, android.os.IBinder.FLAG_CLEAR_BUF);
          if (!_status) {
            if (getDefaultImpl() != null) {
              return getDefaultImpl().RepeatNullableIntEnumArray(input);
            }
          }
          _reply.readException();
          _result = _reply.createIntArray();
        }
        finally {
          _reply.recycle();
          _data.recycle();
        }
        return _result;
      }
      @Override public long[] RepeatNullableLongEnumArray(long[] input) throws android.os.RemoteException
      {
        android.os.Parcel _data = android.os.Parcel.obtain();
        _data.markSensitive();android.os.Parcel _reply = android.os.Parcel.obtain();
        long[] _result;
        try {
          _data.writeInterfaceToken(DESCRIPTOR);
          _data.writeLongArray(input);
          boolean _status = mRemote.transact(Stub.TRANSACTION_RepeatNullableLongEnumArray, _data, _reply, android.os.IBinder.FLAG_CLEAR_BUF);
          if (!_status) {
            if (getDefaultImpl() != null) {
              return getDefaultImpl().RepeatNullableLongEnumArray(input);
            }
          }
          _reply.readException();
          _result = _reply.createLongArray();
        }
        finally {
          _reply.recycle();
          _data.recycle();
        }
        return _result;
      }
      @Override public java.lang.String RepeatNullableString(java.lang.String input) throws android.os.RemoteException
      {
        android.os.Parcel _data = android.os.Parcel.obtain();
        _data.markSensitive();android.os.Parcel _reply = android.os.Parcel.obtain();
        java.lang.String _result;
        try {
          _data.writeInterfaceToken(DESCRIPTOR);
          _data.writeString(input);
          boolean _status = mRemote.transact(Stub.TRANSACTION_RepeatNullableString, _data, _reply, android.os.IBinder.FLAG_CLEAR_BUF);
          if (!_status) {
            if (getDefaultImpl() != null) {
              return getDefaultImpl().RepeatNullableString(input);
            }
          }
          _reply.readException();
          _result = _reply.readString();
        }
        finally {
          _reply.recycle();
          _data.recycle();
        }
        return _result;
      }
      @Override public java.util.List<java.lang.String> RepeatNullableStringList(java.util.List<java.lang.String> input) throws android.os.RemoteException
      {
        android.os.Parcel _data = android.os.Parcel.obtain();
        _data.markSensitive();android.os.Parcel _reply = android.os.Parcel.obtain();
        java.util.List<java.lang.String> _result;
        try {
          _data.writeInterfaceToken(DESCRIPTOR);
          _data.writeStringList(input);
          boolean _status = mRemote.transact(Stub.TRANSACTION_RepeatNullableStringList, _data, _reply, android.os.IBinder.FLAG_CLEAR_BUF);
          if (!_status) {
            if (getDefaultImpl() != null) {
              return getDefaultImpl().RepeatNullableStringList(input);
            }
          }
          _reply.readException();
          _result = _reply.createStringArrayList();
        }
        finally {
          _reply.recycle();
          _data.recycle();
        }
        return _result;
      }
      @Override public android.aidl.tests.StructuredParcelable RepeatNullableParcelable(android.aidl.tests.StructuredParcelable input) throws android.os.RemoteException
      {
        android.os.Parcel _data = android.os.Parcel.obtain();
        _data.markSensitive();android.os.Parcel _reply = android.os.Parcel.obtain();
        android.aidl.tests.StructuredParcelable _result;
        try {
          _data.writeInterfaceToken(DESCRIPTOR);
          if ((input!=null)) {
            _data.writeInt(1);
            input.writeToParcel(_data, 0);
          }
          else {
            _data.writeInt(0);
          }
          boolean _status = mRemote.transact(Stub.TRANSACTION_RepeatNullableParcelable, _data, _reply, android.os.IBinder.FLAG_CLEAR_BUF);
          if (!_status) {
            if (getDefaultImpl() != null) {
              return getDefaultImpl().RepeatNullableParcelable(input);
            }
          }
          _reply.readException();
          if ((0!=_reply.readInt())) {
            _result = android.aidl.tests.StructuredParcelable.CREATOR.createFromParcel(_reply);
          }
          else {
            _result = null;
          }
        }
        finally {
          _reply.recycle();
          _data.recycle();
        }
        return _result;
      }
      @Override public void TakesAnIBinder(android.os.IBinder input) throws android.os.RemoteException
      {
        android.os.Parcel _data = android.os.Parcel.obtain();
        _data.markSensitive();android.os.Parcel _reply = android.os.Parcel.obtain();
        try {
          _data.writeInterfaceToken(DESCRIPTOR);
          _data.writeStrongBinder(input);
          boolean _status = mRemote.transact(Stub.TRANSACTION_TakesAnIBinder, _data, _reply, android.os.IBinder.FLAG_CLEAR_BUF);
          if (!_status) {
            if (getDefaultImpl() != null) {
              getDefaultImpl().TakesAnIBinder(input);
              return;
            }
          }
          _reply.readException();
        }
        finally {
          _reply.recycle();
          _data.recycle();
        }
      }
      @Override public void TakesANullableIBinder(android.os.IBinder input) throws android.os.RemoteException
      {
        android.os.Parcel _data = android.os.Parcel.obtain();
        _data.markSensitive();android.os.Parcel _reply = android.os.Parcel.obtain();
        try {
          _data.writeInterfaceToken(DESCRIPTOR);
          _data.writeStrongBinder(input);
          boolean _status = mRemote.transact(Stub.TRANSACTION_TakesANullableIBinder, _data, _reply, android.os.IBinder.FLAG_CLEAR_BUF);
          if (!_status) {
            if (getDefaultImpl() != null) {
              getDefaultImpl().TakesANullableIBinder(input);
              return;
            }
          }
          _reply.readException();
        }
        finally {
          _reply.recycle();
          _data.recycle();
        }
      }
      // Test utf8 decoding from utf16 wire format
      @Override public java.lang.String RepeatUtf8CppString(java.lang.String token) throws android.os.RemoteException
      {
        android.os.Parcel _data = android.os.Parcel.obtain();
        _data.markSensitive();android.os.Parcel _reply = android.os.Parcel.obtain();
        java.lang.String _result;
        try {
          _data.writeInterfaceToken(DESCRIPTOR);
          _data.writeString(token);
          boolean _status = mRemote.transact(Stub.TRANSACTION_RepeatUtf8CppString, _data, _reply, android.os.IBinder.FLAG_CLEAR_BUF);
          if (!_status) {
            if (getDefaultImpl() != null) {
              return getDefaultImpl().RepeatUtf8CppString(token);
            }
          }
          _reply.readException();
          _result = _reply.readString();
        }
        finally {
          _reply.recycle();
          _data.recycle();
        }
        return _result;
      }
      @Override public java.lang.String RepeatNullableUtf8CppString(java.lang.String token) throws android.os.RemoteException
      {
        android.os.Parcel _data = android.os.Parcel.obtain();
        _data.markSensitive();android.os.Parcel _reply = android.os.Parcel.obtain();
        java.lang.String _result;
        try {
          _data.writeInterfaceToken(DESCRIPTOR);
          _data.writeString(token);
          boolean _status = mRemote.transact(Stub.TRANSACTION_RepeatNullableUtf8CppString, _data, _reply, android.os.IBinder.FLAG_CLEAR_BUF);
          if (!_status) {
            if (getDefaultImpl() != null) {
              return getDefaultImpl().RepeatNullableUtf8CppString(token);
            }
          }
          _reply.readException();
          _result = _reply.readString();
        }
        finally {
          _reply.recycle();
          _data.recycle();
        }
        return _result;
      }
      @Override public java.lang.String[] ReverseUtf8CppString(java.lang.String[] input, java.lang.String[] repeated) throws android.os.RemoteException
      {
        android.os.Parcel _data = android.os.Parcel.obtain();
        _data.markSensitive();android.os.Parcel _reply = android.os.Parcel.obtain();
        java.lang.String[] _result;
        try {
          _data.writeInterfaceToken(DESCRIPTOR);
          _data.writeStringArray(input);
          if ((repeated==null)) {
            _data.writeInt(-1);
          }
          else {
            _data.writeInt(repeated.length);
          }
          boolean _status = mRemote.transact(Stub.TRANSACTION_ReverseUtf8CppString, _data, _reply, android.os.IBinder.FLAG_CLEAR_BUF);
          if (!_status) {
            if (getDefaultImpl() != null) {
              return getDefaultImpl().ReverseUtf8CppString(input, repeated);
            }
          }
          _reply.readException();
          _result = _reply.createStringArray();
          _reply.readStringArray(repeated);
        }
        finally {
          _reply.recycle();
          _data.recycle();
        }
        return _result;
      }
      @Override public java.lang.String[] ReverseNullableUtf8CppString(java.lang.String[] input, java.lang.String[] repeated) throws android.os.RemoteException
      {
        android.os.Parcel _data = android.os.Parcel.obtain();
        _data.markSensitive();android.os.Parcel _reply = android.os.Parcel.obtain();
        java.lang.String[] _result;
        try {
          _data.writeInterfaceToken(DESCRIPTOR);
          _data.writeStringArray(input);
          if ((repeated==null)) {
            _data.writeInt(-1);
          }
          else {
            _data.writeInt(repeated.length);
          }
          boolean _status = mRemote.transact(Stub.TRANSACTION_ReverseNullableUtf8CppString, _data, _reply, android.os.IBinder.FLAG_CLEAR_BUF);
          if (!_status) {
            if (getDefaultImpl() != null) {
              return getDefaultImpl().ReverseNullableUtf8CppString(input, repeated);
            }
          }
          _reply.readException();
          _result = _reply.createStringArray();
          _reply.readStringArray(repeated);
        }
        finally {
          _reply.recycle();
          _data.recycle();
        }
        return _result;
      }
      @Override public java.util.List<java.lang.String> ReverseUtf8CppStringList(java.util.List<java.lang.String> input, java.util.List<java.lang.String> repeated) throws android.os.RemoteException
      {
        android.os.Parcel _data = android.os.Parcel.obtain();
        _data.markSensitive();android.os.Parcel _reply = android.os.Parcel.obtain();
        java.util.List<java.lang.String> _result;
        try {
          _data.writeInterfaceToken(DESCRIPTOR);
          _data.writeStringList(input);
          boolean _status = mRemote.transact(Stub.TRANSACTION_ReverseUtf8CppStringList, _data, _reply, android.os.IBinder.FLAG_CLEAR_BUF);
          if (!_status) {
            if (getDefaultImpl() != null) {
              return getDefaultImpl().ReverseUtf8CppStringList(input, repeated);
            }
          }
          _reply.readException();
          _result = _reply.createStringArrayList();
          _reply.readStringList(repeated);
        }
        finally {
          _reply.recycle();
          _data.recycle();
        }
        return _result;
      }
      @Override public android.aidl.tests.INamedCallback GetCallback(boolean return_null) throws android.os.RemoteException
      {
        android.os.Parcel _data = android.os.Parcel.obtain();
        _data.markSensitive();android.os.Parcel _reply = android.os.Parcel.obtain();
        android.aidl.tests.INamedCallback _result;
        try {
          _data.writeInterfaceToken(DESCRIPTOR);
          _data.writeInt(((return_null)?(1):(0)));
          boolean _status = mRemote.transact(Stub.TRANSACTION_GetCallback, _data, _reply, android.os.IBinder.FLAG_CLEAR_BUF);
          if (!_status) {
            if (getDefaultImpl() != null) {
              return getDefaultImpl().GetCallback(return_null);
            }
          }
          _reply.readException();
          _result = android.aidl.tests.INamedCallback.Stub.asInterface(_reply.readStrongBinder());
        }
        finally {
          _reply.recycle();
          _data.recycle();
        }
        return _result;
      }
      // Since this paracelable has clearly defined default values, it would be
      // inefficient to use an IPC to fill it out in practice.
      @Override public void FillOutStructuredParcelable(android.aidl.tests.StructuredParcelable parcel) throws android.os.RemoteException
      {
        android.os.Parcel _data = android.os.Parcel.obtain();
        _data.markSensitive();android.os.Parcel _reply = android.os.Parcel.obtain();
        try {
          _data.writeInterfaceToken(DESCRIPTOR);
          if ((parcel!=null)) {
            _data.writeInt(1);
            parcel.writeToParcel(_data, 0);
          }
          else {
            _data.writeInt(0);
          }
          boolean _status = mRemote.transact(Stub.TRANSACTION_FillOutStructuredParcelable, _data, _reply, android.os.IBinder.FLAG_CLEAR_BUF);
          if (!_status) {
            if (getDefaultImpl() != null) {
              getDefaultImpl().FillOutStructuredParcelable(parcel);
              return;
            }
          }
          _reply.readException();
          if ((0!=_reply.readInt())) {
            parcel.readFromParcel(_reply);
          }
        }
        finally {
          _reply.recycle();
          _data.recycle();
        }
      }
      @Override public android.aidl.tests.IOldName GetOldNameInterface() throws android.os.RemoteException
      {
        android.os.Parcel _data = android.os.Parcel.obtain();
        _data.markSensitive();android.os.Parcel _reply = android.os.Parcel.obtain();
        android.aidl.tests.IOldName _result;
        try {
          _data.writeInterfaceToken(DESCRIPTOR);
          boolean _status = mRemote.transact(Stub.TRANSACTION_GetOldNameInterface, _data, _reply, android.os.IBinder.FLAG_CLEAR_BUF);
          if (!_status) {
            if (getDefaultImpl() != null) {
              return getDefaultImpl().GetOldNameInterface();
            }
          }
          _reply.readException();
          _result = android.aidl.tests.IOldName.Stub.asInterface(_reply.readStrongBinder());
        }
        finally {
          _reply.recycle();
          _data.recycle();
        }
        return _result;
      }
      @Override public android.aidl.tests.INewName GetNewNameInterface() throws android.os.RemoteException
      {
        android.os.Parcel _data = android.os.Parcel.obtain();
        _data.markSensitive();android.os.Parcel _reply = android.os.Parcel.obtain();
        android.aidl.tests.INewName _result;
        try {
          _data.writeInterfaceToken(DESCRIPTOR);
          boolean _status = mRemote.transact(Stub.TRANSACTION_GetNewNameInterface, _data, _reply, android.os.IBinder.FLAG_CLEAR_BUF);
          if (!_status) {
            if (getDefaultImpl() != null) {
              return getDefaultImpl().GetNewNameInterface();
            }
          }
          _reply.readException();
          _result = android.aidl.tests.INewName.Stub.asInterface(_reply.readStrongBinder());
        }
        finally {
          _reply.recycle();
          _data.recycle();
        }
        return _result;
      }
      // Retrieve the ICppJavaTests if the server supports it
      @Override public android.os.IBinder GetCppJavaTests() throws android.os.RemoteException
      {
        android.os.Parcel _data = android.os.Parcel.obtain();
        _data.markSensitive();android.os.Parcel _reply = android.os.Parcel.obtain();
        android.os.IBinder _result;
        try {
          _data.writeInterfaceToken(DESCRIPTOR);
          boolean _status = mRemote.transact(Stub.TRANSACTION_GetCppJavaTests, _data, _reply, android.os.IBinder.FLAG_CLEAR_BUF);
          if (!_status) {
            if (getDefaultImpl() != null) {
              return getDefaultImpl().GetCppJavaTests();
            }
          }
          _reply.readException();
          _result = _reply.readStrongBinder();
        }
        finally {
          _reply.recycle();
          _data.recycle();
        }
        return _result;
      }
      @Override public byte getBackendType() throws android.os.RemoteException
      {
        android.os.Parcel _data = android.os.Parcel.obtain();
        _data.markSensitive();android.os.Parcel _reply = android.os.Parcel.obtain();
        byte _result;
        try {
          _data.writeInterfaceToken(DESCRIPTOR);
          boolean _status = mRemote.transact(Stub.TRANSACTION_getBackendType, _data, _reply, android.os.IBinder.FLAG_CLEAR_BUF);
          if (!_status) {
            if (getDefaultImpl() != null) {
              return getDefaultImpl().getBackendType();
            }
          }
          _reply.readException();
          _result = _reply.readByte();
        }
        finally {
          _reply.recycle();
          _data.recycle();
        }
        return _result;
      }
      public static android.aidl.tests.ITestService sDefaultImpl;
    }
    static final int TRANSACTION_UnimplementedMethod = (android.os.IBinder.FIRST_CALL_TRANSACTION + 0);
    static final int TRANSACTION_Deprecated = (android.os.IBinder.FIRST_CALL_TRANSACTION + 1);
    static final int TRANSACTION_TestOneway = (android.os.IBinder.FIRST_CALL_TRANSACTION + 2);
    static final int TRANSACTION_RepeatBoolean = (android.os.IBinder.FIRST_CALL_TRANSACTION + 3);
    static final int TRANSACTION_RepeatByte = (android.os.IBinder.FIRST_CALL_TRANSACTION + 4);
    static final int TRANSACTION_RepeatChar = (android.os.IBinder.FIRST_CALL_TRANSACTION + 5);
    static final int TRANSACTION_RepeatInt = (android.os.IBinder.FIRST_CALL_TRANSACTION + 6);
    static final int TRANSACTION_RepeatLong = (android.os.IBinder.FIRST_CALL_TRANSACTION + 7);
    static final int TRANSACTION_RepeatFloat = (android.os.IBinder.FIRST_CALL_TRANSACTION + 8);
    static final int TRANSACTION_RepeatDouble = (android.os.IBinder.FIRST_CALL_TRANSACTION + 9);
    static final int TRANSACTION_RepeatString = (android.os.IBinder.FIRST_CALL_TRANSACTION + 10);
    static final int TRANSACTION_RepeatByteEnum = (android.os.IBinder.FIRST_CALL_TRANSACTION + 11);
    static final int TRANSACTION_RepeatIntEnum = (android.os.IBinder.FIRST_CALL_TRANSACTION + 12);
    static final int TRANSACTION_RepeatLongEnum = (android.os.IBinder.FIRST_CALL_TRANSACTION + 13);
    static final int TRANSACTION_ReverseBoolean = (android.os.IBinder.FIRST_CALL_TRANSACTION + 14);
    static final int TRANSACTION_ReverseByte = (android.os.IBinder.FIRST_CALL_TRANSACTION + 15);
    static final int TRANSACTION_ReverseChar = (android.os.IBinder.FIRST_CALL_TRANSACTION + 16);
    static final int TRANSACTION_ReverseInt = (android.os.IBinder.FIRST_CALL_TRANSACTION + 17);
    static final int TRANSACTION_ReverseLong = (android.os.IBinder.FIRST_CALL_TRANSACTION + 18);
    static final int TRANSACTION_ReverseFloat = (android.os.IBinder.FIRST_CALL_TRANSACTION + 19);
    static final int TRANSACTION_ReverseDouble = (android.os.IBinder.FIRST_CALL_TRANSACTION + 20);
    static final int TRANSACTION_ReverseString = (android.os.IBinder.FIRST_CALL_TRANSACTION + 21);
    static final int TRANSACTION_ReverseByteEnum = (android.os.IBinder.FIRST_CALL_TRANSACTION + 22);
    static final int TRANSACTION_ReverseIntEnum = (android.os.IBinder.FIRST_CALL_TRANSACTION + 23);
    static final int TRANSACTION_ReverseLongEnum = (android.os.IBinder.FIRST_CALL_TRANSACTION + 24);
    static final int TRANSACTION_GetOtherTestService = (android.os.IBinder.FIRST_CALL_TRANSACTION + 25);
    static final int TRANSACTION_VerifyName = (android.os.IBinder.FIRST_CALL_TRANSACTION + 26);
    static final int TRANSACTION_ReverseStringList = (android.os.IBinder.FIRST_CALL_TRANSACTION + 27);
    static final int TRANSACTION_RepeatParcelFileDescriptor = (android.os.IBinder.FIRST_CALL_TRANSACTION + 28);
    static final int TRANSACTION_ReverseParcelFileDescriptorArray = (android.os.IBinder.FIRST_CALL_TRANSACTION + 29);
    static final int TRANSACTION_ThrowServiceException = (android.os.IBinder.FIRST_CALL_TRANSACTION + 30);
    static final int TRANSACTION_RepeatNullableIntArray = (android.os.IBinder.FIRST_CALL_TRANSACTION + 31);
    static final int TRANSACTION_RepeatNullableByteEnumArray = (android.os.IBinder.FIRST_CALL_TRANSACTION + 32);
    static final int TRANSACTION_RepeatNullableIntEnumArray = (android.os.IBinder.FIRST_CALL_TRANSACTION + 33);
    static final int TRANSACTION_RepeatNullableLongEnumArray = (android.os.IBinder.FIRST_CALL_TRANSACTION + 34);
    static final int TRANSACTION_RepeatNullableString = (android.os.IBinder.FIRST_CALL_TRANSACTION + 35);
    static final int TRANSACTION_RepeatNullableStringList = (android.os.IBinder.FIRST_CALL_TRANSACTION + 36);
    static final int TRANSACTION_RepeatNullableParcelable = (android.os.IBinder.FIRST_CALL_TRANSACTION + 37);
    static final int TRANSACTION_TakesAnIBinder = (android.os.IBinder.FIRST_CALL_TRANSACTION + 38);
    static final int TRANSACTION_TakesANullableIBinder = (android.os.IBinder.FIRST_CALL_TRANSACTION + 39);
    static final int TRANSACTION_RepeatUtf8CppString = (android.os.IBinder.FIRST_CALL_TRANSACTION + 40);
    static final int TRANSACTION_RepeatNullableUtf8CppString = (android.os.IBinder.FIRST_CALL_TRANSACTION + 41);
    static final int TRANSACTION_ReverseUtf8CppString = (android.os.IBinder.FIRST_CALL_TRANSACTION + 42);
    static final int TRANSACTION_ReverseNullableUtf8CppString = (android.os.IBinder.FIRST_CALL_TRANSACTION + 43);
    static final int TRANSACTION_ReverseUtf8CppStringList = (android.os.IBinder.FIRST_CALL_TRANSACTION + 44);
    static final int TRANSACTION_GetCallback = (android.os.IBinder.FIRST_CALL_TRANSACTION + 45);
    static final int TRANSACTION_FillOutStructuredParcelable = (android.os.IBinder.FIRST_CALL_TRANSACTION + 46);
    static final int TRANSACTION_GetOldNameInterface = (android.os.IBinder.FIRST_CALL_TRANSACTION + 47);
    static final int TRANSACTION_GetNewNameInterface = (android.os.IBinder.FIRST_CALL_TRANSACTION + 48);
    static final int TRANSACTION_GetCppJavaTests = (android.os.IBinder.FIRST_CALL_TRANSACTION + 49);
    static final int TRANSACTION_getBackendType = (android.os.IBinder.FIRST_CALL_TRANSACTION + 50);
    public static boolean setDefaultImpl(android.aidl.tests.ITestService impl) {
      // Only one user of this interface can use this function
      // at a time. This is a heuristic to detect if two different
      // users in the same process use this function.
      if (Stub.Proxy.sDefaultImpl != null) {
        throw new IllegalStateException("setDefaultImpl() called twice");
      }
      if (impl != null) {
        Stub.Proxy.sDefaultImpl = impl;
        return true;
      }
      return false;
    }
    public static android.aidl.tests.ITestService getDefaultImpl() {
      return Stub.Proxy.sDefaultImpl;
    }
  }
  public static final java.lang.String DESCRIPTOR = "android$aidl$tests$ITestService".replace('$', '.');
  // Test that constants are accessible
  public static final int TEST_CONSTANT = 42;
  public static final int TEST_CONSTANT2 = -42;
  public static final int TEST_CONSTANT3 = 42;
  public static final int TEST_CONSTANT4 = 4;
  public static final int TEST_CONSTANT5 = -4;
  public static final int TEST_CONSTANT6 = 0;
  public static final int TEST_CONSTANT7 = 0;
  public static final int TEST_CONSTANT8 = 0;
  public static final int TEST_CONSTANT9 = 86;
  public static final int TEST_CONSTANT10 = 165;
  public static final int TEST_CONSTANT11 = 250;
  public static final int TEST_CONSTANT12 = -1;
  public static final byte BYTE_TEST_CONSTANT = 17;
  public static final long LONG_TEST_CONSTANT = 1099511627776L;
  public static final String STRING_TEST_CONSTANT = "foo";
  public static final String STRING_TEST_CONSTANT2 = "bar";
  public static final String STRING_TEST_CONSTANT_UTF8 = "baz";
  // All these constant expressions should be equal to 1
  public static final int A1 = 1;
  public static final int A2 = 1;
  public static final int A3 = 1;
  public static final int A4 = 1;
  public static final int A5 = 1;
  public static final int A6 = 1;
  public static final int A7 = 1;
  public static final int A8 = 1;
  public static final int A9 = 1;
  public static final int A10 = 1;
  public static final int A11 = 1;
  public static final int A12 = 1;
  public static final int A13 = 1;
  public static final int A14 = 1;
  public static final int A15 = 1;
  public static final int A16 = 1;
  public static final int A17 = 1;
  public static final int A18 = 1;
  public static final int A19 = 1;
  public static final int A20 = 1;
  public static final int A21 = 1;
  public static final int A22 = 1;
  public static final int A23 = 1;
  public static final int A24 = 1;
  public static final int A25 = 1;
  public static final int A26 = 1;
  public static final int A27 = 1;
  public static final int A28 = 1;
  public static final int A29 = 1;
  public static final int A30 = 1;
  public static final int A31 = 1;
  public static final int A32 = 1;
  public static final int A33 = 1;
  public static final int A34 = 1;
  public static final int A35 = 1;
  public static final int A36 = 1;
  public static final int A37 = 1;
  public static final int A38 = 1;
  public static final int A39 = 1;
  public static final int A40 = 1;
  public static final int A41 = 1;
  public static final int A42 = 1;
  public static final int A43 = 1;
  public static final int A44 = 1;
  // && higher than ||
  public static final int A45 = 1;
  public static final int A46 = 1;
  public static final int A47 = 1;
  public static final int A48 = 1;
  public static final int A49 = 1;
  public static final int A50 = 1;
  public static final int A51 = 1;
  public static final int A52 = 1;
  public static final int A53 = 1;
  // & higher than ^
  public static final int A54 = 1;
  public static final int A55 = 1;
  public static final int A56 = 1;
  public static final int A57 = 1;
  // This is to emulate a method that is added after the service is implemented.
  // So the client cannot assume that a call to this method will be successful
  // or not. However, inside the test environment, we can't build client and
  // the server with different version of this AIDL file. So, we let the server
  // actually implement this and intercept the dispatch to the method
  // inside onTransact().
  // WARNING: Must be first method.
  // This requires hard coding the transaction number. As long as this method is
  // the first in this interface, it can keep the
  // "::android::IBinder::FIRST_CALL_TRANSACTION + 0" value and allow
  // methods to be added and removed.
  public int UnimplementedMethod(int arg) throws android.os.RemoteException;
  /**
       * @deprecated to make sure we have something in system/tools/aidl which does a compile check
       *     of deprecated and make sure this is reflected in goldens
       */
  @Deprecated
  public void Deprecated() throws android.os.RemoteException;
  public void TestOneway() throws android.os.RemoteException;
  // Test that primitives work as parameters and return types.
  public boolean RepeatBoolean(boolean token) throws android.os.RemoteException;
  public byte RepeatByte(byte token) throws android.os.RemoteException;
  public char RepeatChar(char token) throws android.os.RemoteException;
  public int RepeatInt(int token) throws android.os.RemoteException;
  public long RepeatLong(long token) throws android.os.RemoteException;
  public float RepeatFloat(float token) throws android.os.RemoteException;
  public double RepeatDouble(double token) throws android.os.RemoteException;
  public java.lang.String RepeatString(java.lang.String token) throws android.os.RemoteException;
  public byte RepeatByteEnum(byte token) throws android.os.RemoteException;
  public int RepeatIntEnum(int token) throws android.os.RemoteException;
  public long RepeatLongEnum(long token) throws android.os.RemoteException;
  // Test that arrays work as parameters and return types.
  public boolean[] ReverseBoolean(boolean[] input, boolean[] repeated) throws android.os.RemoteException;
  public byte[] ReverseByte(byte[] input, byte[] repeated) throws android.os.RemoteException;
  public char[] ReverseChar(char[] input, char[] repeated) throws android.os.RemoteException;
  public int[] ReverseInt(int[] input, int[] repeated) throws android.os.RemoteException;
  public long[] ReverseLong(long[] input, long[] repeated) throws android.os.RemoteException;
  public float[] ReverseFloat(float[] input, float[] repeated) throws android.os.RemoteException;
  public double[] ReverseDouble(double[] input, double[] repeated) throws android.os.RemoteException;
  public java.lang.String[] ReverseString(java.lang.String[] input, java.lang.String[] repeated) throws android.os.RemoteException;
  public byte[] ReverseByteEnum(byte[] input, byte[] repeated) throws android.os.RemoteException;
  public int[] ReverseIntEnum(int[] input, int[] repeated) throws android.os.RemoteException;
  public long[] ReverseLongEnum(long[] input, long[] repeated) throws android.os.RemoteException;
  // Test that clients can send and receive Binders.
  public android.aidl.tests.INamedCallback GetOtherTestService(java.lang.String name) throws android.os.RemoteException;
  public boolean VerifyName(android.aidl.tests.INamedCallback service, java.lang.String name) throws android.os.RemoteException;
  // Test that List<T> types work correctly.
  public java.util.List<java.lang.String> ReverseStringList(java.util.List<java.lang.String> input, java.util.List<java.lang.String> repeated) throws android.os.RemoteException;
  public android.os.ParcelFileDescriptor RepeatParcelFileDescriptor(android.os.ParcelFileDescriptor read) throws android.os.RemoteException;
  public android.os.ParcelFileDescriptor[] ReverseParcelFileDescriptorArray(android.os.ParcelFileDescriptor[] input, android.os.ParcelFileDescriptor[] repeated) throws android.os.RemoteException;
  // Test that service specific exceptions work correctly.
  public void ThrowServiceException(int code) throws android.os.RemoteException;
  // Test nullability
  public int[] RepeatNullableIntArray(int[] input) throws android.os.RemoteException;
  public byte[] RepeatNullableByteEnumArray(byte[] input) throws android.os.RemoteException;
  public int[] RepeatNullableIntEnumArray(int[] input) throws android.os.RemoteException;
  public long[] RepeatNullableLongEnumArray(long[] input) throws android.os.RemoteException;
  public java.lang.String RepeatNullableString(java.lang.String input) throws android.os.RemoteException;
  public java.util.List<java.lang.String> RepeatNullableStringList(java.util.List<java.lang.String> input) throws android.os.RemoteException;
  public android.aidl.tests.StructuredParcelable RepeatNullableParcelable(android.aidl.tests.StructuredParcelable input) throws android.os.RemoteException;
  public void TakesAnIBinder(android.os.IBinder input) throws android.os.RemoteException;
  public void TakesANullableIBinder(android.os.IBinder input) throws android.os.RemoteException;
  // Test utf8 decoding from utf16 wire format
  public java.lang.String RepeatUtf8CppString(java.lang.String token) throws android.os.RemoteException;
  public java.lang.String RepeatNullableUtf8CppString(java.lang.String token) throws android.os.RemoteException;
  public java.lang.String[] ReverseUtf8CppString(java.lang.String[] input, java.lang.String[] repeated) throws android.os.RemoteException;
  public java.lang.String[] ReverseNullableUtf8CppString(java.lang.String[] input, java.lang.String[] repeated) throws android.os.RemoteException;
  public java.util.List<java.lang.String> ReverseUtf8CppStringList(java.util.List<java.lang.String> input, java.util.List<java.lang.String> repeated) throws android.os.RemoteException;
  public android.aidl.tests.INamedCallback GetCallback(boolean return_null) throws android.os.RemoteException;
  // Since this paracelable has clearly defined default values, it would be
  // inefficient to use an IPC to fill it out in practice.
  public void FillOutStructuredParcelable(android.aidl.tests.StructuredParcelable parcel) throws android.os.RemoteException;
  public android.aidl.tests.IOldName GetOldNameInterface() throws android.os.RemoteException;
  public android.aidl.tests.INewName GetNewNameInterface() throws android.os.RemoteException;
  // Retrieve the ICppJavaTests if the server supports it
  public android.os.IBinder GetCppJavaTests() throws android.os.RemoteException;
  public byte getBackendType() throws android.os.RemoteException;
}
