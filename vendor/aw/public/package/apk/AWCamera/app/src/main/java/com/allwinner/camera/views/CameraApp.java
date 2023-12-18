package com.allwinner.camera.views;

import android.app.Application;
import android.content.Context;
import android.support.v4.view.ViewPager;
import android.util.AttributeSet;
import android.util.Log;


import com.allwinner.camera.cameracontrol.CameraManager;
import com.allwinner.camera.utils.PlatFormsUtils;

import org.greenrobot.eventbus.EventBus;

public class CameraApp extends Application {
    @Override
    public void onCreate() {
        super.onCreate();
        CameraManager.getInstance().setContext(this);
        PlatFormsUtils.init(this);
        // EventBus.builder().throwSubscriberException(BuildConfig.DEBUG).addIndex(new MyEventBusIndex()).installDefaultEventBus();
    }
}
