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
import android.os.Bundle;

import com.android.settingslib.core.AbstractPreferenceController;

import java.io.File;

public class CvbsPreferenceController extends AbstractPreferenceController implements
        Preference.OnPreferenceChangeListener, Preference.OnPreferenceClickListener{

    private static final String KEY_CVBS_SETTING = "cvbs_setting";
    private static final String KEY_CVBS_OUTPUT_MODE = "cvbs_output_mode";
    private static final String KEY_CVBS_PERCENTAGE = "cvbs_percentage";
    private static final String KEY_CVBS_PERCENTAGE_ENTRY = "cvbs_percentage_entry";
    private static final int DISPLAY_PERCENTAGE_MIN = 80;
    private static final int DISPLAY_PERCENTAGE_MAX = 100;
    private DisplayOutputImpl mDisplayOutputImpl;
    private ListPreference mCvbsOutputMode;
    private SeekBarPreference mCvbsPercentage;
    private Preference mCvbsPercentagePref;
    private static final String KEY_TYPE = "DISPLAY_TYPE";
    private PreferenceCategory mCategory;

    public CvbsPreferenceController(Context context) {
        super(context);
        mDisplayOutputImpl = new DisplayOutputImpl();
    }

    @Override
    public void displayPreference(PreferenceScreen screen) {
        mCategory = (PreferenceCategory) screen.findPreference(getPreferenceKey());
        mCvbsOutputMode = (ListPreference) mCategory.findPreference(KEY_CVBS_OUTPUT_MODE);
        mCvbsOutputMode.setOnPreferenceChangeListener(this);
        mCvbsPercentage = (SeekBarPreference) mCategory.findPreference(KEY_CVBS_PERCENTAGE);
        mCvbsPercentage.setOnPreferenceChangeListener(this);
        mCvbsPercentage.setMin(DISPLAY_PERCENTAGE_MIN);
        mCvbsPercentage.setMax(DISPLAY_PERCENTAGE_MAX);
        setVisible(mCategory, KEY_CVBS_PERCENTAGE, false);
        mCvbsPercentagePref = (Preference) mCategory.findPreference(KEY_CVBS_PERCENTAGE_ENTRY);
        mCvbsPercentagePref.setOnPreferenceClickListener(this);
        mDisplayOutputImpl.updateSupportModes(mCvbsOutputMode, DisplayOutputImpl.TYPE_CVBS);
    }

    @Override
    public boolean isAvailable() {
        return true;
    }

    @Override
    public String getPreferenceKey() {
        return KEY_CVBS_SETTING;
    }

    @Override
    public void updateState(Preference preference) {
        if (mCvbsOutputMode != null) {
            int mode = mDisplayOutputImpl.getCvbsOutputMode();
            mCvbsOutputMode.setValue("" + mode);
        }
        if (mCvbsPercentage != null) {
            mCvbsPercentage.setValue(mDisplayOutputImpl.getDisplayPercent());
        }
    }

    @Override
    public boolean onPreferenceChange(Preference preference, Object newValue) {
        if (mCvbsOutputMode == preference) {
            int mode = Integer.parseInt((String) newValue);
            mDisplayOutputImpl.setDisplayOutputMode(mode);
        } else if (mCvbsPercentage == preference) {
            int value = (int) newValue;
            mDisplayOutputImpl.setDisplayPercent(value);
        }
        return true;
    }

    @Override
    public boolean onPreferenceClick(Preference preference){
        if (mCvbsPercentagePref == preference) {
            ComponentName mComponentName = new ComponentName(mContext,DisplayPercentageActivity.class);
            Intent mIntent = new Intent();
            Bundle mBundle = new Bundle();
            mBundle.putInt(KEY_TYPE, DisplayOutputImpl.TYPE_CVBS);
            mIntent.setComponent(mComponentName);
            mIntent.putExtras(mBundle);
            mContext.startActivity(mIntent);
            return true;
        }
        return false;
    }
}
