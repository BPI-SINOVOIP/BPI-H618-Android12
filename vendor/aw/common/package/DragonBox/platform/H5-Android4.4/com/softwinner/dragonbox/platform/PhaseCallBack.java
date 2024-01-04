package com.softwinner.dragonbox.platform;

import android.content.Context;
import android.content.Intent;
import android.util.Log;

public class PhaseCallBack implements IPhaseCallback {
    private static final String TAG = "DragonBox-Android4.4-PhaseCallBack";
    private static final String ACTION = "com.lhxk.voice_recognition_end";
    public void allResultTestPassCallback(Context context) {
        Log.w(TAG,"trigger allResultTestPassCallback!");
        Intent intent = new Intent();
        //intent.setClassName("com.allwinnertech.dragonenter", "com.allwinnertech.dragonenter.EnterActivity");
        intent.setClassName("com.softwinner.TvdFileManager", "com.softwinner.TvdFileManager.MainUI");
        context.startActivity(intent);
    }
    public void startAppCallback(Context context) {
        Log.w(TAG,"trigger startAppCallback");
        Intent intent = new Intent();
        intent.setAction(ACTION);
        context.sendBroadcast(intent);
    }
    public void allResultTestPassCallbackAfterResultDialog(Context context) {
        Log.w(TAG,"trigger allResultTestPassCallbackAfterResultDialog");
    }
}
