/*
 * This file is auto-generated.  DO NOT MODIFY.
 */
package android.aidl.loggable;
public interface ILoggableInterface extends android.os.IInterface
{
  /** Default implementation for ILoggableInterface. */
  public static class Default implements android.aidl.loggable.ILoggableInterface
  {
    @Override public java.lang.String[] LogThis(boolean boolValue, boolean[] boolArray, byte byteValue, byte[] byteArray, char charValue, char[] charArray, int intValue, int[] intArray, long longValue, long[] longArray, float floatValue, float[] floatArray, double doubleValue, double[] doubleArray, java.lang.String stringValue, java.lang.String[] stringArray, java.util.List<java.lang.String> listValue, android.aidl.loggable.Data dataValue, android.os.IBinder binderValue, android.os.ParcelFileDescriptor pfdValue, android.os.ParcelFileDescriptor[] pfdArray) throws android.os.RemoteException
    {
      return null;
    }
    @Override
    public android.os.IBinder asBinder() {
      return null;
    }
  }
  /** Local-side IPC implementation stub class. */
  public static abstract class Stub extends android.os.Binder implements android.aidl.loggable.ILoggableInterface
  {
    /** Construct the stub at attach it to the interface. */
    public Stub()
    {
      this.attachInterface(this, DESCRIPTOR);
    }
    /**
     * Cast an IBinder object into an android.aidl.loggable.ILoggableInterface interface,
     * generating a proxy if needed.
     */
    public static android.aidl.loggable.ILoggableInterface asInterface(android.os.IBinder obj)
    {
      if ((obj==null)) {
        return null;
      }
      android.os.IInterface iin = obj.queryLocalInterface(DESCRIPTOR);
      if (((iin!=null)&&(iin instanceof android.aidl.loggable.ILoggableInterface))) {
        return ((android.aidl.loggable.ILoggableInterface)iin);
      }
      return new android.aidl.loggable.ILoggableInterface.Stub.Proxy(obj);
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
        case TRANSACTION_LogThis:
        {
          try {
            android.os.Trace.traceBegin(android.os.Trace.TRACE_TAG_AIDL, "AIDL::java::ILoggableInterface::LogThis::server");
            data.enforceInterface(descriptor);
            boolean _arg0;
            _arg0 = (0!=data.readInt());
            boolean[] _arg1;
            _arg1 = data.createBooleanArray();
            byte _arg2;
            _arg2 = data.readByte();
            byte[] _arg3;
            _arg3 = data.createByteArray();
            char _arg4;
            _arg4 = (char)data.readInt();
            char[] _arg5;
            _arg5 = data.createCharArray();
            int _arg6;
            _arg6 = data.readInt();
            int[] _arg7;
            _arg7 = data.createIntArray();
            long _arg8;
            _arg8 = data.readLong();
            long[] _arg9;
            _arg9 = data.createLongArray();
            float _arg10;
            _arg10 = data.readFloat();
            float[] _arg11;
            _arg11 = data.createFloatArray();
            double _arg12;
            _arg12 = data.readDouble();
            double[] _arg13;
            _arg13 = data.createDoubleArray();
            java.lang.String _arg14;
            _arg14 = data.readString();
            java.lang.String[] _arg15;
            _arg15 = data.createStringArray();
            java.util.List<java.lang.String> _arg16;
            _arg16 = data.createStringArrayList();
            android.aidl.loggable.Data _arg17;
            if ((0!=data.readInt())) {
              _arg17 = android.aidl.loggable.Data.CREATOR.createFromParcel(data);
            }
            else {
              _arg17 = null;
            }
            android.os.IBinder _arg18;
            _arg18 = data.readStrongBinder();
            android.os.ParcelFileDescriptor _arg19;
            if ((0!=data.readInt())) {
              _arg19 = android.os.ParcelFileDescriptor.CREATOR.createFromParcel(data);
            }
            else {
              _arg19 = null;
            }
            android.os.ParcelFileDescriptor[] _arg20;
            _arg20 = data.createTypedArray(android.os.ParcelFileDescriptor.CREATOR);
            java.lang.String[] _result = this.LogThis(_arg0, _arg1, _arg2, _arg3, _arg4, _arg5, _arg6, _arg7, _arg8, _arg9, _arg10, _arg11, _arg12, _arg13, _arg14, _arg15, _arg16, _arg17, _arg18, _arg19, _arg20);
            reply.writeNoException();
            reply.writeStringArray(_result);
            reply.writeBooleanArray(_arg1);
            reply.writeByteArray(_arg3);
            reply.writeCharArray(_arg5);
            reply.writeIntArray(_arg7);
            reply.writeLongArray(_arg9);
            reply.writeFloatArray(_arg11);
            reply.writeDoubleArray(_arg13);
            reply.writeStringArray(_arg15);
            reply.writeStringList(_arg16);
            if ((_arg19!=null)) {
              reply.writeInt(1);
              _arg19.writeToParcel(reply, android.os.Parcelable.PARCELABLE_WRITE_RETURN_VALUE);
            }
            else {
              reply.writeInt(0);
            }
            reply.writeTypedArray(_arg20, android.os.Parcelable.PARCELABLE_WRITE_RETURN_VALUE);
            return true;
          }
          finally {
            android.os.Trace.traceEnd(android.os.Trace.TRACE_TAG_AIDL);
          }
        }
        default:
        {
          return super.onTransact(code, data, reply, flags);
        }
      }
    }
    private static class Proxy implements android.aidl.loggable.ILoggableInterface
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
      @Override public java.lang.String[] LogThis(boolean boolValue, boolean[] boolArray, byte byteValue, byte[] byteArray, char charValue, char[] charArray, int intValue, int[] intArray, long longValue, long[] longArray, float floatValue, float[] floatArray, double doubleValue, double[] doubleArray, java.lang.String stringValue, java.lang.String[] stringArray, java.util.List<java.lang.String> listValue, android.aidl.loggable.Data dataValue, android.os.IBinder binderValue, android.os.ParcelFileDescriptor pfdValue, android.os.ParcelFileDescriptor[] pfdArray) throws android.os.RemoteException
      {
        android.os.Parcel _data = android.os.Parcel.obtain();
        android.os.Parcel _reply = android.os.Parcel.obtain();
        java.lang.String[] _result;
        try {
          android.os.Trace.traceBegin(android.os.Trace.TRACE_TAG_AIDL, "AIDL::java::ILoggableInterface::LogThis::client");
          _data.writeInterfaceToken(DESCRIPTOR);
          _data.writeInt(((boolValue)?(1):(0)));
          _data.writeBooleanArray(boolArray);
          _data.writeByte(byteValue);
          _data.writeByteArray(byteArray);
          _data.writeInt(((int)charValue));
          _data.writeCharArray(charArray);
          _data.writeInt(intValue);
          _data.writeIntArray(intArray);
          _data.writeLong(longValue);
          _data.writeLongArray(longArray);
          _data.writeFloat(floatValue);
          _data.writeFloatArray(floatArray);
          _data.writeDouble(doubleValue);
          _data.writeDoubleArray(doubleArray);
          _data.writeString(stringValue);
          _data.writeStringArray(stringArray);
          _data.writeStringList(listValue);
          if ((dataValue!=null)) {
            _data.writeInt(1);
            dataValue.writeToParcel(_data, 0);
          }
          else {
            _data.writeInt(0);
          }
          _data.writeStrongBinder(binderValue);
          if ((pfdValue!=null)) {
            _data.writeInt(1);
            pfdValue.writeToParcel(_data, 0);
          }
          else {
            _data.writeInt(0);
          }
          _data.writeTypedArray(pfdArray, 0);
          boolean _status = mRemote.transact(Stub.TRANSACTION_LogThis, _data, _reply, 0);
          if (!_status) {
            if (getDefaultImpl() != null) {
              return getDefaultImpl().LogThis(boolValue, boolArray, byteValue, byteArray, charValue, charArray, intValue, intArray, longValue, longArray, floatValue, floatArray, doubleValue, doubleArray, stringValue, stringArray, listValue, dataValue, binderValue, pfdValue, pfdArray);
            }
          }
          _reply.readException();
          _result = _reply.createStringArray();
          _reply.readBooleanArray(boolArray);
          _reply.readByteArray(byteArray);
          _reply.readCharArray(charArray);
          _reply.readIntArray(intArray);
          _reply.readLongArray(longArray);
          _reply.readFloatArray(floatArray);
          _reply.readDoubleArray(doubleArray);
          _reply.readStringArray(stringArray);
          _reply.readStringList(listValue);
          if ((0!=_reply.readInt())) {
            pfdValue = android.os.ParcelFileDescriptor.CREATOR.createFromParcel(_reply);
          }
          _reply.readTypedArray(pfdArray, android.os.ParcelFileDescriptor.CREATOR);
        }
        finally {
          _reply.recycle();
          _data.recycle();
          android.os.Trace.traceEnd(android.os.Trace.TRACE_TAG_AIDL);
        }
        return _result;
      }
      public static android.aidl.loggable.ILoggableInterface sDefaultImpl;
    }
    static final int TRANSACTION_LogThis = (android.os.IBinder.FIRST_CALL_TRANSACTION + 0);
    public static boolean setDefaultImpl(android.aidl.loggable.ILoggableInterface impl) {
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
    public static android.aidl.loggable.ILoggableInterface getDefaultImpl() {
      return Stub.Proxy.sDefaultImpl;
    }
  }
  public static final java.lang.String DESCRIPTOR = "android$aidl$loggable$ILoggableInterface".replace('$', '.');
  public java.lang.String[] LogThis(boolean boolValue, boolean[] boolArray, byte byteValue, byte[] byteArray, char charValue, char[] charArray, int intValue, int[] intArray, long longValue, long[] longArray, float floatValue, float[] floatArray, double doubleValue, double[] doubleArray, java.lang.String stringValue, java.lang.String[] stringArray, java.util.List<java.lang.String> listValue, android.aidl.loggable.Data dataValue, android.os.IBinder binderValue, android.os.ParcelFileDescriptor pfdValue, android.os.ParcelFileDescriptor[] pfdArray) throws android.os.RemoteException;
}
