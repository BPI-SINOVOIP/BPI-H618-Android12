package com.softwinner.wfdsink;

import com.softwinner.wfdsink.WFDCallback;
import java.lang.ref.WeakReference;
import android.view.Surface;
import android.util.Log;

public class WFDManager {
    private static final String TAG = "WFDManager";

    private static WFDCallback mWFDCallback = null;

    public static final int WFD_SETUP = 1;
    public static final int WFD_PLAY = 2;
    public static final int WFD_PAUSE = 3;
    public static final int WFD_TEARDOWN = 4;

    private static native final int nativeWFDManagerInit(Object wfdManager_this);
    private static native final void nativeWFDManagerDestroy();
    private static native final String nativeWFDManagerGetPeerIP(String MAC);
    private static native final int nativeWFDManagerSetup(String ip, int port);
    private static native final int nativeWFDManagerStop();
    public static native final int nativeWFDManagerSetScreenRotation(int rotation);
/*
    private static native final int nativeWFDManagerPlay();
    private static native final int nativeWFDManagerPause();
*/

    static
    {
        System.loadLibrary("jni_WFDManager");
        handleRTSPStatusFromNative(null,0);
        feedDataFromNative(null,null,0);
        handleExceptionFromNative(null);
        requestSurfaceFromNative(null);
    }

    public WFDManager() {
        nativeWFDManagerInit(new WeakReference<WFDManager>(this));
    }

    public void WFDManagerDestroy() {
        nativeWFDManagerDestroy();
    }

    public static String WFDManagerGetPeerIP(String mac) {
        return nativeWFDManagerGetPeerIP(mac);
    }

    public int WFDManagerSetup(String ip, int port) {
        return nativeWFDManagerSetup(ip, port);
    }

    public int WFDManagerStop() {
        return nativeWFDManagerStop();
    }

/*
    //just for LOCAL_PLAYER
    public int WFDManagerPlay() {
        return nativeWFDManagerPlay();
    }

    //just for LOCAL_PLAYER
    public int WFDManagerPause() {
        return nativeWFDManagerPause();
    }
*/

    public boolean registerCallbacks(WFDCallback callback) {
        mWFDCallback = callback;
        return true;
    }

    public static int handleRTSPStatusFromNative(Object wfdManager_ref, int status) {
        if (wfdManager_ref == null) return -1;
        WFDManager wfdManager = (WFDManager)((WeakReference)wfdManager_ref).get();
        if (wfdManager == null) {
            return -1;
        }

        if (status == WFD_SETUP || status == WFD_PLAY || status == WFD_PAUSE || status == WFD_TEARDOWN) {
            if (wfdManager.mWFDCallback != null) {
                return wfdManager.mWFDCallback.handleRTSPStatus(status);
            }
        }

        return -1;
    }

    public static int handleVideoCropFromNative(Object wfdManager_ref, int cropx, int croph) {
        if (wfdManager_ref == null) return -1;
        WFDManager wfdManager = (WFDManager)((WeakReference)wfdManager_ref).get();
        if (wfdManager == null) {
            return -1;
        }

        if (wfdManager.mWFDCallback != null) {
            return wfdManager.mWFDCallback.handleVideoCrop(cropx, croph);
        }

        return -1;
    }

    public static int feedDataFromNative(Object wfdManager_ref, byte[] buffer, int len) {
        if (wfdManager_ref == null) return -1;
        WFDManager wfdManager = (WFDManager)((WeakReference)wfdManager_ref).get();
        if (wfdManager == null) {
            return -1;
        }

        if (buffer != null && len >= 0 && buffer.length >= len) {
            if (wfdManager.mWFDCallback != null) {
                return wfdManager.mWFDCallback.feedData(buffer, len);
            }
        }

        return -1;
    }

    public static void handleExceptionFromNative(Object wfdManager_ref) {
        if (wfdManager_ref == null) return ;
        WFDManager wfdManager = (WFDManager)((WeakReference)wfdManager_ref).get();
        if (wfdManager == null) {
            return;
        }

        if (wfdManager.mWFDCallback != null) {
            wfdManager.mWFDCallback.handleException();
        }
    }

    public static Surface requestSurfaceFromNative(Object wfdManager_ref) {
        if (wfdManager_ref == null) return null;
        WFDManager wfdManager = (WFDManager)((WeakReference)wfdManager_ref).get();
        if (wfdManager == null) {
            return null;
        }

        if (wfdManager.mWFDCallback != null) {
            return wfdManager.mWFDCallback.requestSurface();
        }

        return null;
    }
}
