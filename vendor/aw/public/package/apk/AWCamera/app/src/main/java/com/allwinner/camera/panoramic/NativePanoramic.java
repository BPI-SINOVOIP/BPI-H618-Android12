package com.allwinner.camera.panoramic;

import android.graphics.Bitmap;

public class NativePanoramic {
    public native static int processPanorama(long[] imageAddressArray, long outputAddress, String[] stringArray);

    public native static void cropPanorama(long imageAddress, long outputAddress);

    public native static int getProgress();
    public native static int getBitmap(Bitmap bitmap);
}
