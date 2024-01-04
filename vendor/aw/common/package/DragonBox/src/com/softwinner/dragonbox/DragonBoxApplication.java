package com.softwinner.dragonbox;

import android.content.Context;
import android.app.Application;
import com.softwinner.dragonbox.jni.ReadPrivateJNI;

public class DragonBoxApplication extends Application {

    public static ReadPrivateJNI mReadPrivateJNI = null;
    private static DragonBoxApplication instance = null;
    private static Context mContext;

    @Override
    public void onCreate() {
        super.onCreate();
        instance = this;
        mContext = getApplicationContext();
    }

    public static DragonBoxApplication getApplication() {
        return instance;
    }

    public static Context getAppContext() {
        return mContext;
    }

    public static ReadPrivateJNI getReadPrivateJNI() {
        if(mReadPrivateJNI==null) {
            mReadPrivateJNI = new ReadPrivateJNI();
        }
        return mReadPrivateJNI;
    }
}