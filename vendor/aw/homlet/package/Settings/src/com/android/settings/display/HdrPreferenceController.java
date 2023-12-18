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
import android.support.v14.preference.SwitchPreference;
import android.support.v7.preference.ListPreference;
import android.support.v7.preference.PreferenceScreen;
import android.support.v7.preference.Preference;

import com.android.settings.core.PreferenceControllerMixin;
import com.android.settings.widget.SeekBarPreference;
import com.android.settingslib.core.AbstractPreferenceController;

import android.app.Fragment;
import android.widget.Toast;
import com.softwinner.display.DisplaydClient;

public class HdrPreferenceController extends AbstractPreferenceController implements
        PreferenceControllerMixin, Preference.OnPreferenceChangeListener {

    private static final String KEY_OUTPUT_SETTING   = "output_setting";
    private static final String KEY_HDMI_COLOR_MODE = "hdmi_color_mode";
    private static final String KEY_HDR_MODE        = "hdr_mode";

    private final Fragment mFragment;
    private final DisplaydClient mProxy;

    private ListPreference mHdmiColorMode;
    private ListPreference mHdrMode;

    public HdrPreferenceController(Context context, Fragment fragment) {
        super(context);
        mProxy = new DisplaydClient(context);
        mFragment = fragment;
    }

    @Override
    public void displayPreference(PreferenceScreen screen) {
        mHdmiColorMode = (ListPreference)screen.findPreference(KEY_HDMI_COLOR_MODE);
        mHdmiColorMode.setOnPreferenceChangeListener(this);

        mHdrMode = (ListPreference)screen.findPreference(KEY_HDR_MODE);
        mHdrMode.setOnPreferenceChangeListener(this);
    }

    @Override
    public boolean isAvailable() {
        return true;
    }

    @Override
    public String getPreferenceKey() {
        return KEY_OUTPUT_SETTING;
    }

    @Override
    public void updateState(Preference preference) {
        if (mHdmiColorMode != null) {
            Integer pixelformat = mProxy.getPixelFormat(0);
            mHdmiColorMode.setValue(pixelformat.toString());
        }
        if (mHdrMode != null) {
            Integer hdrmode = mProxy.getDataspaceMode(0);
            mHdrMode.setValue(hdrmode.toString());
        }
    }

    @Override
    public boolean onPreferenceChange(Preference preference, Object newValue) {
        boolean ret = false;
        if (mHdmiColorMode == preference) {
            int pixelformat = Integer.parseInt((String)newValue);
            if (mProxy.setPixelFormat(0, pixelformat) == 0)
                ret = true;
        }
        if (mHdrMode == preference) {
            int hdrmode = Integer.parseInt((String)newValue);
            if (mProxy.setDataspaceMode(0, hdrmode) == 0)
                ret = true;
        }

        if (!ret) {
            Toast.makeText(mFragment.getActivity(),
                    "Unsupported",Toast.LENGTH_LONG).show();
        }
        return ret;
    }
}
