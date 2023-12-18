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
import android.content.res.Resources.NotFoundException;
import androidx.preference.SwitchPreference;
import androidx.preference.PreferenceScreen;
import com.android.settings.widget.SeekBarPreference;
import androidx.preference.Preference;
import androidx.preference.PreferenceCategory;

import com.android.settings.R;
import com.android.settings.core.PreferenceControllerMixin;
import com.android.settingslib.core.AbstractPreferenceController;

import vendor.display.DisplayOutputManager;
public class AwEnhanceModePreferenceController extends AbstractPreferenceController implements
        PreferenceControllerMixin, Preference.OnPreferenceChangeListener {

    private static final String KEY_ENHANCE_MODE_SETTING = "enhance_mode_setting";
    private static final String KEY_ENHANCE_MODE = "enhance_mode";
    private static final String KEY_ENHANCE_MODE_DEMO = "enhance_mode_demo";
    private static final String KEY_COLOR_BRIGHT_SETTING = "color_bright_setting";
    private static final String KEY_CONTRAST_SETTING = "contrast_setting";
    private static final String KEY_SATURATION_SETTING = "saturation_setting";
    private static final String KEY_BLACK_WHITE_MODE_DEMO = "black_white_mode";
    private Context mContext;
    private SwitchPreference mEnhanceMode;
    private SwitchPreference mDemo;
    private SeekBarPreference mBright;
    private SeekBarPreference mContrast;
    private SeekBarPreference mSaturation;
    private SwitchPreference mBlackWhiteMode;
    private int maxRange;
    private boolean mEnhanceModeValue;
    private boolean mEnhanceDemoValue;
    private static final int DISPLAY0 = 0;

    private DisplayOutputManager mManager;
    public AwEnhanceModePreferenceController(Context context) {
        super(context);
        mContext = context;
        mManager = DisplayOutputManager.getInstance();
    }

    @Override
    public void displayPreference(PreferenceScreen screen) {
        if (isAvailable()) {
            setVisible(screen, getPreferenceKey(), true);
            mEnhanceMode = (SwitchPreference) screen.findPreference(KEY_ENHANCE_MODE);
            mEnhanceMode.setOnPreferenceChangeListener(this);
            mDemo = (SwitchPreference) screen.findPreference(KEY_ENHANCE_MODE_DEMO);
            mDemo.setOnPreferenceChangeListener(this);
            mBlackWhiteMode = (SwitchPreference) screen.findPreference(KEY_BLACK_WHITE_MODE_DEMO);
            if (isHasHSL()) {
                try {
                    maxRange = mContext.getResources().getInteger(R.integer.config_HSL_max_range);
                } catch (NotFoundException e) {
                    maxRange = 10;       // default max range is 10 for pad.
                }
                mBright = (SeekBarPreference) screen.findPreference(KEY_COLOR_BRIGHT_SETTING);
                mBright.setOnPreferenceChangeListener(this);
                mBright.setMax(maxRange);
                mContrast = (SeekBarPreference) screen.findPreference(KEY_CONTRAST_SETTING);
                mContrast.setOnPreferenceChangeListener(this);
                mContrast.setMax(maxRange);
                mSaturation = (SeekBarPreference) screen.findPreference(KEY_SATURATION_SETTING);
                mSaturation.setOnPreferenceChangeListener(this);
                mSaturation.setMax(maxRange);
                mBlackWhiteMode.setOnPreferenceChangeListener(this);
            } else {
                PreferenceCategory screen_enhance = (PreferenceCategory)(screen.findPreference(KEY_ENHANCE_MODE_SETTING));
                mBright = (SeekBarPreference) screen.findPreference(KEY_COLOR_BRIGHT_SETTING);
                mContrast = (SeekBarPreference) screen.findPreference(KEY_CONTRAST_SETTING);
                mSaturation = (SeekBarPreference) screen.findPreference(KEY_SATURATION_SETTING);
                screen_enhance.removePreference(mBright);
                screen_enhance.removePreference(mContrast);
                screen_enhance.removePreference(mSaturation);
                screen_enhance.removePreference(mBlackWhiteMode);
            }
        } else {
            setVisible(screen, getPreferenceKey(), false);
        }
    }

    @Override
    public boolean isAvailable() {
        return true;
    }

    private boolean isHasHSL() {
        try {
            return mContext.getResources().getBoolean(R.bool.config_has_HSL);
        } catch (NotFoundException e) {
            return false;     // default do not show HSL for pad.
        }
    }

    @Override
    public String getPreferenceKey() {
        return KEY_ENHANCE_MODE_SETTING;
    }

    @Override
    public void updateState(Preference preference) {
        int mode = mManager.getDisplayEnhanceMode(DISPLAY0);
        boolean enabled = (mode == DisplayOutputManager.MODE_DEMO)
                        || (mode == DisplayOutputManager.MODE_ENABLE);
        boolean demo = mode == DisplayOutputManager.MODE_DEMO;
        if (mEnhanceMode != null)
            mEnhanceMode.setChecked(enabled);
        if (mDemo != null) {
            mDemo.setEnabled(enabled);
            mDemo.setChecked(demo);
        }
        if (mBright != null) {
            int bright = mManager.getDisplayBright(DISPLAY0);
            if (bright < 0 || bright > maxRange)
                bright = maxRange / 2;
            mBright.setProgress(bright);
            mBright.setEnabled(enabled);
        }
        if (mContrast != null) {
            int contrast = mManager.getDisplayContrast(DISPLAY0);
            if (contrast < 0 || contrast > maxRange)
                contrast = maxRange / 2;
            mContrast.setProgress(contrast);
            mContrast.setEnabled(enabled);
        }
        if (mSaturation != null) {
            int saturation = mManager.getDisplaySaturation(DISPLAY0);
            if (saturation < 0 || saturation > maxRange)
                saturation = maxRange / 2;
            mSaturation.setProgress(saturation);
            mSaturation.setEnabled(enabled);
        }
        if (mBlackWhiteMode != null) {
            boolean bwMode = mManager.getBlackWhiteMode(DISPLAY0);
            mBlackWhiteMode.setEnabled(enabled);
            mBlackWhiteMode.setChecked(bwMode);
        }
    }

    @Override
    public boolean onPreferenceChange(Preference preference, Object newValue) {
        if (mEnhanceMode == preference) {
            boolean auto = (Boolean) newValue;
            mEnhanceModeValue = auto;
            if (mEnhanceModeValue && mEnhanceDemoValue) {
                mManager.setDisplayEnhanceMode(DISPLAY0, DisplayOutputManager.MODE_DEMO);
            } else if (mEnhanceModeValue) {
                mManager.setDisplayEnhanceMode(DISPLAY0, DisplayOutputManager.MODE_ENABLE);
            } else {
                mManager.setDisplayEnhanceMode(DISPLAY0, DisplayOutputManager.MODE_DISABLE);
            }
            if (mDemo != null) mDemo.setEnabled(auto);
            if (mBright != null) mBright.setEnabled(auto);
            if (mContrast != null) mContrast.setEnabled(auto);
            if (mSaturation	!= null) mSaturation.setEnabled(auto);
            if (mBlackWhiteMode != null) mBlackWhiteMode.setEnabled(auto);
        } else if (mDemo == preference) {
            boolean auto = (Boolean) newValue;
            mEnhanceDemoValue = auto;
            if (mEnhanceModeValue && mEnhanceDemoValue) {
                mManager.setDisplayEnhanceMode(DISPLAY0, DisplayOutputManager.MODE_DEMO);
            } else if (mEnhanceModeValue) {
                mManager.setDisplayEnhanceMode(DISPLAY0, DisplayOutputManager.MODE_ENABLE);
            } else {
                mManager.setDisplayEnhanceMode(DISPLAY0, DisplayOutputManager.MODE_DISABLE);
            }
        } else if (mBright == preference) {
            int auto = (Integer) newValue;
            mManager.setDisplayBright(DISPLAY0, auto);
        } else if (mContrast == preference) {
            int auto = (Integer) newValue;
            mManager.setDisplayContrast(DISPLAY0, auto);
        } else if (mSaturation == preference) {
            int auto = (Integer) newValue;
            mManager.setDisplaySaturation(DISPLAY0, auto);
        } else if (mBlackWhiteMode == preference) {
            boolean auto = (Boolean) newValue;
            mManager.setBlackWhiteMode(DISPLAY0, auto);
            int saturation = mManager.getDisplaySaturation(DISPLAY0);
            if (saturation < 0 || saturation > maxRange)
                    saturation = maxRange / 2;
            mSaturation.setProgress(saturation);
            mSaturation.setEnabled(!auto);
        }
        return true;
    }
}
