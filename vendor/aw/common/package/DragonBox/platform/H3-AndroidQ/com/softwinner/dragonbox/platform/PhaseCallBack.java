package com.softwinner.dragonbox.platform;

import com.softwinner.dragonbox.ui.DragonBoxMain;
import com.softwinner.dragonbox.utils.Utils;

import android.content.Context;
import android.content.Intent;
import android.util.Log;

public class PhaseCallBack implements IPhaseCallback {
    private static final String TAG = "DragonBox-H3-AndroidQ-PhaseCallBack";
    private static final String ACTION = "com.lhxk.voice_recognition_end";
    public void allResultTestPassCallback(Context context) {
        Log.w(TAG,"trigger allResultTestPassCallback!");
        //Utils.setPropertySecure(DragonBoxMain.PROPERTY_DRAGONAGING_NEXTBOOT,"true");
        //Utils.setPropertySecure("persist.sys.dragonaging_time","0");
    }
    public void startAppCallback(Context context) {
        Log.w(TAG,"trigger startAppCallback");
    }
    public void allTestOverCallback(Context context,String...params) {
        Log.w(TAG,"trigger allTestOverCallback");
        Utils.setPropertySecure(DragonBoxMain.PROPERTY_SMT_DRAGONBOX_TESTCASE,params[0]);
        Utils.setPropertySecure(DragonBoxMain.PROPERTY_SMT_DRAGONBOX_TESTRESULT,params[1]);
    }
    public void allResultTestPassCallbackAfterResultDialog(Context context) {
        Log.w(TAG,"trigger allResultTestPassCallbackAfterResultDialog");
    }
}
