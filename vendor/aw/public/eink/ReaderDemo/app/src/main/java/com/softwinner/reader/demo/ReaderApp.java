package com.softwinner.reader.demo;

import android.app.Application;

import com.softwinner.reader.demo.utils.Utils;

public class ReaderApp extends Application {

    private final static String TAG = ReaderApp.class.getSimpleName();

    private static ReaderApp sApp;

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

    public static ReaderApp get() {
        return sApp;
    }

}
