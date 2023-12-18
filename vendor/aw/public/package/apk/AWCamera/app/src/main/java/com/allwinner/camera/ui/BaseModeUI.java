package com.allwinner.camera.ui;

import android.content.Context;
import android.graphics.Bitmap;
import android.view.Surface;
import android.view.View;

import com.allwinner.camera.utils.SoundPlayerUtil;

import static com.allwinner.camera.data.Contants.CameraCommand.TAKEPICTURE;

public class BaseModeUI {
    UIManager mUiManager;
    public BaseModeUI(View rootView,UIManager uiManager){
        mUiManager = uiManager;
    }

    public UIManager getUiManager() {
        return mUiManager;
    }
    public Context getContext(){
        return mUiManager.getContext();
    }
    public void release() {
    }

    public void onResume() {

    }

    public void updateThumbnail(Bitmap bitmap) {

    }

    public void startCountDown(int time) {
        getUiManager().startCountDown(time);
    }

    public void cancelCountDown() {
        getUiManager().cancelCountDown();
    }


    public void onCountDownStart() {

    }


    public void onCountDownEnd() {
        mUiManager.startCoverAnimation();
        SoundPlayerUtil.playSound(SoundPlayerUtil.TYPE_IMAGE_SOUND);
        getUiManager().sendCommand(TAKEPICTURE, null);
    }


    public void onCountDownCancel() {

    }

    public void onOrientationChanged(int orientationCompensation) {

    }
    public void setCaptureEnable(){
    }

    public Surface getPreviewSurface(){
        return null;
    }

    public void enter() {
    }

    public void onFlashStatus(boolean isFlashAvaliable) {
    }

    public void onModeChanged() {
    }
}
