/*
 * Copyright (C) 2015 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */
package com.android.compatibility.common.deviceinfo;

import android.app.Activity;
import android.content.Context;
import android.content.res.Configuration;
import android.os.Bundle;
import android.os.IBinder;
import android.server.wm.jetpack.utils.ExtensionUtils;
import android.server.wm.jetpack.utils.wrapper.TestDisplayFeature;
import android.server.wm.jetpack.utils.wrapper.TestInterfaceCompat;
import android.server.wm.jetpack.utils.wrapper.TestWindowLayoutInfo;
import android.text.TextUtils;
import android.util.DisplayMetrics;
import android.view.Display;
import android.view.WindowManager;

import com.android.compatibility.common.util.DeviceInfoStore;
import com.android.compatibility.common.util.DummyActivity;

import java.util.List;

/**
 * Screen device info collector.
 */
public final class ScreenDeviceInfo extends DeviceInfo {

    @Override
    protected void collectDeviceInfo(DeviceInfoStore store) throws Exception {
        DisplayMetrics metrics = new DisplayMetrics();
        WindowManager windowManager =
                (WindowManager) getContext().getSystemService(Context.WINDOW_SERVICE);
        Display display = windowManager.getDefaultDisplay();
        display.getRealMetrics(metrics);

        store.addResult("width_pixels", metrics.widthPixels);
        store.addResult("height_pixels", metrics.heightPixels);
        store.addResult("x_dpi", metrics.xdpi);
        store.addResult("y_dpi", metrics.ydpi);
        store.addResult("density", metrics.density);
        store.addResult("density_dpi", metrics.densityDpi);

        Configuration configuration = getContext().getResources().getConfiguration();
        store.addResult("screen_size", getScreenSize(configuration));
        store.addResult("smallest_screen_width_dp", configuration.smallestScreenWidthDp);

        // WindowManager Jetpack Library version and available display features.
        String wmJetpackVersion = ExtensionUtils.getVersion();
        if (!TextUtils.isEmpty(wmJetpackVersion)) {
            int[] displayFeatures = getDisplayFeatures();
            store.addResult("wm_jetpack_version", wmJetpackVersion);
            store.addArrayResult("display_features", displayFeatures);
        }
    }

    private int[] getDisplayFeatures() {
        final Activity activity = ScreenDeviceInfo.this.launchActivity(
                "com.android.compatibility.common.deviceinfo",
                DummyActivity.class,
                new Bundle());
        final IBinder windowToken = activity.getWindow().getAttributes().token;
        final TestInterfaceCompat extension = ExtensionUtils.getInterfaceCompat(activity);
        if (extension == null) {
            return new int[0];
        }

        final TestWindowLayoutInfo windowLayoutInfo = extension.getWindowLayoutInfo(windowToken);
        if (windowLayoutInfo == null) {
            return new int[0];
        }

        List<TestDisplayFeature> displayFeatureList = windowLayoutInfo.getDisplayFeatures();
        final int[] displayFeatures = new int[displayFeatureList.size()];
        for (int i = 0; i < displayFeatureList.size(); i++) {
            displayFeatures[i] = displayFeatureList.get(i).getType();
        }
        return displayFeatures;
    }

    private static String getScreenSize(Configuration configuration) {
        int screenLayout = configuration.screenLayout & Configuration.SCREENLAYOUT_SIZE_MASK;
        String screenSize = String.format("0x%x", screenLayout);
        switch (screenLayout) {
            case Configuration.SCREENLAYOUT_SIZE_SMALL:
                screenSize = "small";
                break;

            case Configuration.SCREENLAYOUT_SIZE_NORMAL:
                screenSize = "normal";
                break;

            case Configuration.SCREENLAYOUT_SIZE_LARGE:
                screenSize = "large";
                break;

            case Configuration.SCREENLAYOUT_SIZE_XLARGE:
                screenSize = "xlarge";
                break;

            case Configuration.SCREENLAYOUT_SIZE_UNDEFINED:
                screenSize = "undefined";
                break;
        }
        return screenSize;
    }
}
