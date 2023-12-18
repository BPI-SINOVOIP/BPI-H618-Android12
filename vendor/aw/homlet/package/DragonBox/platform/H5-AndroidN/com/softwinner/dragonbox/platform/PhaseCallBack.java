package com.softwinner.dragonbox.platform;

import android.content.Context;
import android.content.Intent;
import android.util.Log;

public class PhaseCallBack implements IPhaseCallback {
    private static final String TAG = "DragonBox-AndroidN-PhaseCallBack";
    private static final String ACTION = "com.lhxk.voice_recognition_end";
    public void allResultTestPassCallback(Context context) {
        Log.w(TAG,"trigger allResultTestPassCallback!");
    }
    public void startAppCallback(Context context) {
        Log.w(TAG,"trigger startAppCallback");
    }
    public void allResultTestPassCallbackAfterResultDialog(Context context) {
        Log.w(TAG,"trigger allResultTestPassCallbackAfterResultDialog");
    }
}
