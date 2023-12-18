/*
 * Copyright (C) 2017 The Android Open Source Project
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

package com.android.settings.deviceinfo;

import android.content.Context;

import com.android.settings.R;
import com.android.settings.core.PreferenceControllerMixin;
import java.lang.ref.WeakReference;
import android.content.Context;
import android.os.Handler;
import android.os.Message;
import android.os.SystemClock;
import android.os.SystemProperties;
import android.text.format.DateUtils;

import androidx.annotation.VisibleForTesting;
import androidx.preference.Preference;
import androidx.preference.PreferenceScreen;

import com.android.settingslib.core.AbstractPreferenceController;
import com.android.settingslib.core.lifecycle.Lifecycle;
import com.android.settingslib.core.lifecycle.LifecycleObserver;
import com.android.settingslib.core.lifecycle.events.OnStart;
import com.android.settingslib.core.lifecycle.events.OnStop;

/**
 * Concrete subclass of IP address preference controller
 */
public class VersionNumberPreferenceController extends AbstractPreferenceController implements
        LifecycleObserver, OnStart, OnStop,PreferenceControllerMixin {

    private Preference mVersionNumber;
    static final String VERSION_NUMBER = "version_number";
    private static final int EVENT_UPDATE_STATS = 500;
    //static final String VERSION_NUMBER_VALUE = SystemProperties.get("ro.build.Bigdroid.version");
    static final String VERSION_NUMBER_VALUE = "BigdroidOS 2.0.0";

    public VersionNumberPreferenceController(Context context, Lifecycle lifecycle) {
        super(context);
        if (lifecycle != null) {
            lifecycle.addObserver(this);
        }
    }

    @Override
    public void onStart() {
    }

    @Override
    public void onStop() {
    }

    @Override
    public boolean isAvailable() {
        return true;
    }


    @Override
    public String getPreferenceKey() {
        return VERSION_NUMBER;
    }


    @Override
    public void displayPreference(PreferenceScreen screen) {
        super.displayPreference(screen);
        mVersionNumber = screen.findPreference(VERSION_NUMBER);
        updateVersionNumber();
    }

    private void updateVersionNumber() {
        mVersionNumber.setSummary(VERSION_NUMBER_VALUE);
    }
    // This space intentionally left blank
}
