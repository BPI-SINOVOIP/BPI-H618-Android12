package com.allwinner.camera.cameracontrol;

import android.content.Context;
import android.graphics.SurfaceTexture;
import android.net.Uri;
import android.view.Surface;


import com.allwinner.camera.program.FilterCaptureEngine;

import java.lang.reflect.Array;
import java.util.List;

public interface CameraStatusListener {
    SurfaceTexture getSurfaceTexture();
    List<Surface> getSurface();
    Surface getVideoSurface();

    void onCameraOpen();
    void onCameraOpenFail(int error);
    void onCameraClose();
    void onFlashStatus(boolean isFlashAvaliable);
    void onStartPreviewDone();
    void onStartPreviewFail();
    void onPreviewStop();
    void onCaptureStarted();
    void onCatpureCompleted();
    void onAutoFocusSuccess();
    void onAutoFocusFail();
    void onAutoFocusing();
    Context getContext();
    FilterCaptureEngine getFilterCaptureEngine();
}
