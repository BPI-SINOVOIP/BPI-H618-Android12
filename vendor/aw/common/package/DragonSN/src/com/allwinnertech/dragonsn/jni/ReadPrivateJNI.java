
package com.allwinnertech.dragonsn.jni;

public class ReadPrivateJNI {

    static {
        System.loadLibrary("allwinnertech_read_private");
        nativeInit();
    }

    public static native boolean nativeInit();

    public native String nativeGetParameter(String name);

    public native boolean nativeSetParameter(String name, String value);

    public native void nativeRelease();

}
