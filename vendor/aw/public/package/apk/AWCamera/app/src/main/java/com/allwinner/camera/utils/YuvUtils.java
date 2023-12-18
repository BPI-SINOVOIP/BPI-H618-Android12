package com.allwinner.camera.utils;

import android.graphics.Bitmap;
import android.util.Log;

import java.nio.ByteBuffer;

public class YuvUtils {
    static {
        System.loadLibrary("yuvutil");
    }

    public static native void changeYuvToNv21(byte[] inputdata, byte[] outyuvBuf, int width, int height, int bits);

    public static native void changeNv21torealyuv(byte[] Nv21Buf, int width, int height, int stride, boolean isBurst);



    public static native byte[] rotateNV21(byte[] input, int width, int height, int rotation, boolean mirror);
    public static native void scaleNV21Data(byte[] inputdata, int inputwidth, int inputheight, byte[] outputdata, int outputwidth, int outputheight);

}
