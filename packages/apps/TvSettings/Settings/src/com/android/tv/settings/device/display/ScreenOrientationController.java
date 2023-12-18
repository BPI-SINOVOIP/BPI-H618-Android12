
/*
* Copyright (C) 2016 The Android Open Source Project+ *
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
import android.provider.Settings;
import android.os.SystemProperties;

import com.android.tv.settings.R;

import androidx.preference.ListPreference;
import androidx.preference.Preference;
import androidx.preference.PreferenceScreen;


import com.android.settingslib.core.AbstractPreferenceController;

public class ScreenOrientationController extends AbstractPreferenceController implements
	Preference.OnPreferenceChangeListener {

	private static final String KEY_SCREEN_ORIENTATION_SETTINGS = "screen_orientation_settings";
	private static final String KEY_SCREEN_ROTATION = "screen_rotation";
	private ListPreference mScreenRotation;
	private Context mContext;

	public ScreenOrientationController(Context context) {
		super(context);
		mContext = context;
		}

	@Override
	public void displayPreference(PreferenceScreen screen) {
		if (isAvailable()) {
			setVisible(screen, getPreferenceKey(), true);
			mScreenRotation = (ListPreference) screen.findPreference(KEY_SCREEN_ROTATION);
			mScreenRotation.setOnPreferenceChangeListener(this);
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
		return KEY_SCREEN_ROTATION;
	}

	@Override
	public void updateState(Preference preference) {
		int rotationDefault = SystemProperties.getInt("ro.default.rotation",0)/90;
		int rotation = Settings.System.getInt(mContext.getContentResolver(), Settings.System.USER_ROTATION, rotationDefault);
		preference.setSummary(mContext.getString(R.string.screen_rotation_summary, "" + rotation * 90 + ""));
		mScreenRotation.setValue(""+rotation);
	}

	@Override
	public boolean onPreferenceChange(Preference preference, Object newValue) {
		int orientation = Integer.parseInt((String) newValue);
		Settings.System.putInt(mContext.getContentResolver(), Settings.System.USER_ROTATION, orientation);
		return true;
	}
}

