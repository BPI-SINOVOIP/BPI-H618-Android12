/*
 * Copyright (C) 2016 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License"); you may not use this file
 * except in compliance with the License. You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software distributed under the
 * License is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
 * KIND, either express or implied. See the License for the specific language governing
 * permissions and limitations under the License.
 */
package com.android.settings.display;

import android.content.Context;
import androidx.preference.SwitchPreference;
import androidx.preference.PreferenceScreen;
import androidx.preference.Preference;

import com.android.settings.core.PreferenceControllerMixin;
import com.android.settingslib.core.AbstractPreferenceController;

import vendor.display.DisplayOutputManager;

public class AwSmartBacklightPreferenceController extends AbstractPreferenceController implements
        PreferenceControllerMixin, Preference.OnPreferenceChangeListener {

    private static final String KEY_SMART_BACKLIGHT_SETTING = "smart_backlight_setting";
    private static final String KEY_SMART_BACKLIGHT = "smart_backlight";
    private static final String KEY_SMART_BACKLIGHT_DEMO = "smart_backlight_demo";
    private SwitchPreference mSmartBacklight;
    private SwitchPreference mDemo;

    private boolean mSmartBacklightValue;
    private boolean mSmartBacklightDemoValue;
    private static final int DISPLAY0 = 0;
    private DisplayOutputManager mManager;
    public AwSmartBacklightPreferenceController(Context context) {
        super(context);
        mManager = DisplayOutputManager.getInstance();
    }

    @Override
    public void displayPreference(PreferenceScreen screen) {
        if (isAvailable()) {
            setVisible(screen, getPreferenceKey(), true);
            mSmartBacklight = (SwitchPreference) screen.findPreference(KEY_SMART_BACKLIGHT);
            mSmartBacklight.setOnPreferenceChangeListener(this);
            mDemo = (SwitchPreference) screen.findPreference(KEY_SMART_BACKLIGHT_DEMO);
            mDemo.setOnPreferenceChangeListener(this);
        } else {
            setVisible(screen, getPreferenceKey(), false);
        }
    }

    @Override
    public boolean isAvailable() {
        return true;
    }

    @Override
    public String getPreferenceKey() {
        return KEY_SMART_BACKLIGHT_SETTING;
    }

    @Override
    public void updateState(Preference preference) {
        int mode = mManager.getSmartBacklight(DISPLAY0);
        boolean enabled = (mode == DisplayOutputManager.MODE_DEMO)
                    || (mode == DisplayOutputManager.MODE_ENABLE);
        boolean demo = mode == DisplayOutputManager.MODE_DEMO;
        if (mSmartBacklight != null)
            mSmartBacklight.setChecked(enabled);
        if (mDemo != null) {
            mDemo.setEnabled(enabled);
            mDemo.setChecked(demo);
        }
    }

    @Override
    public boolean onPreferenceChange(Preference preference, Object newValue) {
        boolean auto = (Boolean) newValue;
        if (mSmartBacklight == preference) {
            mSmartBacklightValue = auto;
            if(mDemo != null) mDemo.setEnabled(auto);
        } else if (mDemo == preference) {
            mSmartBacklightDemoValue = auto;
        }
        if (mSmartBacklightValue && mSmartBacklightDemoValue) {
            mManager.setSmartBacklight(DISPLAY0, DisplayOutputManager.MODE_DEMO);
        } else if (mSmartBacklightValue) {
            mManager.setSmartBacklight(DISPLAY0, DisplayOutputManager.MODE_ENABLE);
        } else {
            mManager.setSmartBacklight(DISPLAY0, DisplayOutputManager.MODE_DISABLE);
        }
        return true;
    }
}
