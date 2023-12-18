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
import android.content.ComponentName;
import android.content.Intent;
import androidx.preference.ListPreference;
import androidx.preference.Preference;
import androidx.preference.SeekBarPreference;
import androidx.preference.PreferenceCategory;
import androidx.preference.PreferenceScreen;

import com.android.settingslib.core.AbstractPreferenceController;

import java.io.File;
import android.util.Log;
import android.os.Bundle;

public class HdmiPreferenceController extends AbstractPreferenceController implements
        Preference.OnPreferenceChangeListener, Preference.OnPreferenceClickListener{

    private static final String KEY_HDMI_SETTING = "hdmi_setting";
    private static final String KEY_HDMI_OUTPUT_MODE = "hdmi_output_mode";
    private static final String KEY_HDMI_PERCENTAGE = "hdmi_percentage";
    private static final String KEY_HDMI_PERCENTAGE_ENTRY = "hdmi_percentage_entry";
    private static final int DISPLAY_PERCENTAGE_MIN = 80;
    private static final int DISPLAY_PERCENTAGE_MAX = 100;
    private DisplayOutputImpl mDisplayOutputImpl;
    private ListPreference mHdmiOutputMode;
    private SeekBarPreference mHdmiPercentage;
    private Preference mHdmiPercentagePref;
    private Context mContext;
    private static final String TAG = "HdmiPreferenceController";
    private static final String KEY_TYPE = "DISPLAY_TYPE";
    private PreferenceCategory mCategory;

    public HdmiPreferenceController(Context context) {
        super(context);
        mContext = context;
        mDisplayOutputImpl = new DisplayOutputImpl();
    }

    @Override
    public void displayPreference(PreferenceScreen screen) {
        mCategory = (PreferenceCategory) screen.findPreference(getPreferenceKey());
        mHdmiOutputMode = (ListPreference) mCategory.findPreference(KEY_HDMI_OUTPUT_MODE);
        mHdmiOutputMode.setOnPreferenceChangeListener(this);
        mHdmiPercentage = (SeekBarPreference) mCategory.findPreference(KEY_HDMI_PERCENTAGE);
        mHdmiPercentage.setOnPreferenceChangeListener(this);
        mHdmiPercentage.setMin(DISPLAY_PERCENTAGE_MIN);
        mHdmiPercentage.setMax(DISPLAY_PERCENTAGE_MAX);
        setVisible(mCategory, KEY_HDMI_PERCENTAGE, false);
        mHdmiPercentagePref = (Preference) mCategory.findPreference(KEY_HDMI_PERCENTAGE_ENTRY);
        mHdmiPercentagePref.setOnPreferenceClickListener(this);
        mDisplayOutputImpl.updateSupportModes(mHdmiOutputMode, DisplayOutputImpl.TYPE_HDMI);
    }

    @Override
    public boolean isAvailable() {
        return true;
    }

    @Override
    public String getPreferenceKey() {
        return KEY_HDMI_SETTING;
    }

    @Override
    public void updateState(Preference preference) {
        if (mHdmiOutputMode != null) {
            int mode = mDisplayOutputImpl.getDisplayOutputModeHdmi();
            mHdmiOutputMode.setValue("" + mode);
        }
        if (mHdmiPercentage != null) {
            mHdmiPercentage.setValue(mDisplayOutputImpl.getDisplayPercent());
        }
    }

    @Override
    public boolean onPreferenceChange(Preference preference, Object newValue) {
        if (mHdmiOutputMode == preference) {
            int mode = Integer.parseInt((String) newValue);
            mDisplayOutputImpl.setDisplayOutputMode(mode);
            mDisplayOutputImpl.updateSupportModes(mHdmiOutputMode, DisplayOutputImpl.TYPE_HDMI);
        } else if (mHdmiPercentage == preference) {
            int value = (int) newValue;
            mDisplayOutputImpl.setDisplayPercent(value);
        }
        return true;
    }

    @Override
    public boolean onPreferenceClick(Preference preference){
        if (mHdmiPercentagePref == preference) {
            ComponentName mComponentName = new ComponentName(mContext,DisplayPercentageActivity.class);
            Bundle mBundle = new Bundle();
            mBundle.putInt(KEY_TYPE,DisplayOutputImpl.TYPE_HDMI);
            Intent mIntent = new Intent();
            mIntent.setComponent(mComponentName);
            mIntent.putExtras(mBundle);
            mContext.startActivity(mIntent);
            return true;
        }
        return false;
    }
}
