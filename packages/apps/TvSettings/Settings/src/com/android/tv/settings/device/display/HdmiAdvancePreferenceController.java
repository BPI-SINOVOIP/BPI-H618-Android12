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
package com.android.tv.settings.device.display;

import android.content.Context;

import androidx.preference.ListPreference;
import androidx.preference.Preference;
import androidx.preference.PreferenceScreen;
import android.widget.Toast;

import com.android.settingslib.core.AbstractPreferenceController;

import java.io.File;

import vendor.display.DisplayOutputManager;

public class HdmiAdvancePreferenceController extends AbstractPreferenceController implements
        Preference.OnPreferenceChangeListener {

    private static final int DISPLAY_BUILD_IN = 0;
    private static final String KEY_HDMI_ADVANCE_SETTING = "hdmi_advance_setting";
    private static final String KEY_HDMI_COLOR_MODE = "hdmi_color_mode";
    private static final String KEY_HDR_MODE = "hdr_mode";
    private DisplayOutputManager mDisplayManager;
    private ListPreference mHdmiColorMode;
    private ListPreference mHdrMode;

    public HdmiAdvancePreferenceController(Context context) {
        super(context);
        mDisplayManager = DisplayOutputManager.getInstance();
    }

    @Override
    public void displayPreference(PreferenceScreen screen) {
        if (isAvailable()) {
            setVisible(screen, getPreferenceKey(), true);
            mHdmiColorMode = (ListPreference) screen.findPreference(KEY_HDMI_COLOR_MODE);
            mHdmiColorMode.setOnPreferenceChangeListener(this);
            mHdrMode = (ListPreference) screen.findPreference(KEY_HDR_MODE);
            mHdrMode.setOnPreferenceChangeListener(this);
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
        return KEY_HDMI_ADVANCE_SETTING;
    }

    @Override
    public void updateState(Preference preference) {
        if (mHdmiColorMode != null) {
            int value = mDisplayManager.getDisplayOutputPixelFormat(DISPLAY_BUILD_IN);
            mHdmiColorMode.setValue(Integer.toHexString(value));
        }
        if (mHdrMode != null) {
            int value = mDisplayManager.getDisplayOutputDataspaceMode(DISPLAY_BUILD_IN);
            mHdrMode.setValue(Integer.toHexString(value));
        }
    }

    @Override
    public boolean onPreferenceChange(Preference preference, Object newValue) {
        int ret = 0;
        String result = null;
        if (mHdmiColorMode == preference) {
            int value = Integer.parseInt((String) newValue);
            ret = mDisplayManager.setDisplayOutputPixelFormat(DISPLAY_BUILD_IN, value);
            result = "set hdmi color space and deep mode failed";
        } else if (mHdrMode == preference) {
            int value = Integer.parseInt((String) newValue);
            ret = mDisplayManager.setDisplayOutputDataspaceMode(DISPLAY_BUILD_IN, value);
            result = "set hdmi hdr mode failed";
        }
        if (ret < 0) {
            Toast.makeText(mContext, result, Toast.LENGTH_LONG).show();
            return false;
        }
        return true;
    }
}
