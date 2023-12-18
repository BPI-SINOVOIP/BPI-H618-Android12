package com.softwinner.agingdragonbox;

import android.app.Application;

import com.softwinner.agingdragonbox.engine.DFEngine;
import com.softwinner.agingdragonbox.engine.LogcatHelper;

public class DragonBoxApp extends Application {

    public DFEngine mEngine;

    @Override
    public void onCreate() {
        super.onCreate();
        LogcatHelper.getInstance(this).start();
    }

    public void setEngine(DFEngine engine) {
        mEngine = engine;
    }

    public DFEngine getEngine() {
        return mEngine;
    }
}
