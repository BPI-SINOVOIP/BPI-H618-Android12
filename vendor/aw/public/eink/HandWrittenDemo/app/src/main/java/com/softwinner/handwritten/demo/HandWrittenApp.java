package com.softwinner.handwritten.demo;

import android.app.Application;

import com.softwinner.handwritten.demo.utils.Utils;

public class HandWrittenApp extends Application {

    private final static String TAG = HandWrittenApp.class.getSimpleName();

    private static HandWrittenApp sApp;

    @Override
    public void onCreate() {
        super.onCreate();

        sApp = this;

        if (Utils.isMainProcess(this)) {
            initConfig();
        }
    }

    private void initConfig() {
    }

    public static HandWrittenApp get() {
        return sApp;
    }

}
