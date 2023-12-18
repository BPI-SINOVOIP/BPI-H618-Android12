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
import androidx.preference.ListPreference;
import androidx.preference.PreferenceScreen;
import androidx.preference.Preference;
import androidx.preference.PreferenceCategory;

import com.android.settings.core.PreferenceControllerMixin;
import com.android.settings.widget.SeekBarPreference;
import com.android.settingslib.core.AbstractPreferenceController;

import java.io.File;

import vendor.display.DisplayOutputManager;
import com.android.settings.R;

public class AwDisplayPreferenceController extends AbstractPreferenceController implements
        PreferenceControllerMixin, Preference.OnPreferenceChangeListener {

    private static final String HDMI_STATE = "/sys/class/extcon/hdmi/state";
    private static final String CVBS_STATE = "/sys/class/extcon/cvbs/state";
    private static final String KEY_HDMI_SETTING = "second_screen_setting";
    private static final String KEY_HDMI_OUTPUT_MODE = "second_screen_output_mode";
    private static final String KEY_HDMI_FULLSCREEN = "hdmi_fullscreen";
    private static final String KEY_HDMI_WIDTH_SCALE = "second_screen_width_scale";
    private static final String KEY_HDMI_HEIGHT_SCALE = "second_screen_height_scale";
    private static final int HDMI_SCALE_MIN = 80;
    private static final int HDMI_SCALE_MAX = 100;
    private static final int NONE_DISPLAY = 0;
    private static final int HDMI_PLUG_IN = 1;
    private static final int CVBS_PLUG_IN = 2;
    private int display_type = NONE_DISPLAY;
    private ListPreference mOutputMode;
    private SwitchPreference mFullscreen;
    private SeekBarPreference mWidthScale;
    private SeekBarPreference mHeightScale;
    private Context mContext;
    private DisplayOutputManager mManager;
    private static final int DISPLAY1 = 1;
    private int mMarginWidthValue;
    private int mMarginHeightValue;

    public AwDisplayPreferenceController(Context context) {
        super(context);
        mContext = context;
        mManager = DisplayOutputManager.getInstance();
    }

    @Override
    public void displayPreference(PreferenceScreen screen) {
        if (isAvailable()) {
            setVisible(screen, getPreferenceKey(), true);
            mOutputMode = (ListPreference) screen.findPreference(KEY_HDMI_OUTPUT_MODE);
            mOutputMode.setOnPreferenceChangeListener(this);
            mFullscreen = (SwitchPreference) screen.findPreference(KEY_HDMI_FULLSCREEN);
            mFullscreen.setOnPreferenceChangeListener(this);
            mWidthScale = (SeekBarPreference) screen.findPreference(KEY_HDMI_WIDTH_SCALE);
            mWidthScale.setOnPreferenceChangeListener(this);
            mHeightScale = (SeekBarPreference) screen.findPreference(KEY_HDMI_HEIGHT_SCALE);
            mHeightScale.setOnPreferenceChangeListener(this);
            mWidthScale.setMax(HDMI_SCALE_MAX - HDMI_SCALE_MIN);
            mHeightScale.setMax(HDMI_SCALE_MAX - HDMI_SCALE_MIN);
            if (display_type == CVBS_PLUG_IN) {
                //CVBS dont need full screen mode
                PreferenceCategory screen_settings = (PreferenceCategory)(screen.findPreference(KEY_HDMI_SETTING));
                screen_settings.removePreference(mFullscreen);
                //OutputMode default ues hdmi mode entries and values, so if display type is cvbs, should change entries and values
                mOutputMode.setEntries(R.array.cvbs_output_mode_entries);
                mOutputMode.setEntryValues(R.array.cvbs_output_mode_values);
            }
        } else {
            setVisible(screen, getPreferenceKey(), false);
        }
    }

    //support HDMI and CVBS
    private boolean checkHDMIState() {
        if (new File(HDMI_STATE).exists()) {
            display_type = HDMI_PLUG_IN;
            return true;
        }
        return false;
    }

    private boolean checkCVBSState() {
        if (new File(CVBS_STATE).exists()) {
            display_type = CVBS_PLUG_IN;
            return true;
        }
        return false;
    }

    @Override
    public boolean isAvailable() {
        //second display only support HDMI or CVBS, can not support them as the same time
        return checkHDMIState() || checkCVBSState();
    }

    @Override
    public String getPreferenceKey() {
        return KEY_HDMI_SETTING;
    }

    @Override
    public void updateState(Preference preference) {
        if (mOutputMode != null) {
            int mode = mManager.getDisplayOutputMode(DISPLAY1);
            mOutputMode.setValue("" + mode);
        }
        if (mFullscreen != null) {
            boolean full = mManager.getHdmiFullscreen(DISPLAY1);
            mFullscreen.setChecked(full);
        }
        if (mWidthScale != null || mHeightScale != null) {
            int[] value = mManager.getDisplayMargin(DISPLAY1);
            if (mWidthScale != null) {
                mWidthScale.setProgress(value[0] - HDMI_SCALE_MIN);
            }
            if (mHeightScale != null) {
                mHeightScale.setProgress(value[1] - HDMI_SCALE_MIN);
            }
        }
    }

    @Override
    public boolean onPreferenceChange(Preference preference, Object newValue) {
        if (mOutputMode == preference) {
            int mode = Integer.parseInt((String) newValue);
            mManager.setDisplayOutputMode(DISPLAY1, mode);
        } else if (mFullscreen == preference) {
            boolean full = (Boolean) newValue;
            mManager.setHdmiFullscreen(DISPLAY1, full);
        } else if (mWidthScale == preference) {
            mMarginWidthValue = (Integer) newValue;
            mManager.setDisplayMargin(DISPLAY1, mMarginWidthValue + HDMI_SCALE_MIN,
                    mMarginHeightValue + HDMI_SCALE_MIN);
        } else if (mHeightScale == preference) {
            mMarginHeightValue = (Integer) newValue;
            mManager.setDisplayMargin(DISPLAY1, mMarginWidthValue + HDMI_SCALE_MIN,
                    mMarginHeightValue + HDMI_SCALE_MIN);
        }
        return true;
    }
}
