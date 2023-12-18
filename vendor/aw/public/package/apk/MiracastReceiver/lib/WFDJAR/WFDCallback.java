package com.softwinner.wfdsink;

import android.view.Surface;

public interface WFDCallback {
    public int handleRTSPStatus(int status);
    public int handleVideoCrop(int cropx, int croph);
    public int feedData(byte[] buffer, int len);
    public void handleException();
    public Surface requestSurface();
}
