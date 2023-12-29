
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
import android.os.ServiceManager;
import android.os.SystemProperties;
import android.os.UserHandle;
import android.util.Log;
import android.view.IWindowManager;

import com.android.tv.settings.R;

import androidx.preference.ListPreference;
import androidx.preference.Preference;
import androidx.preference.PreferenceScreen;

import com.android.settingslib.core.AbstractPreferenceController;

public class ScreenDensityController extends AbstractPreferenceController implements
	Preference.OnPreferenceChangeListener {

	private static final String TAG = "ScreenDensityController";
 
	private static final String KEY_SCREEN_DENSITY_SETTINGS = "screen_density_settings";
	private static final String KEY_SCREEN_DENSITY = "screen_density";
	private ListPreference mScreenDensity;
	private Context mContext;

	private static final int DEFAULT_DISPLAY = 0;
	private static final int DEFAULT_DENSITY = 213;
	private IWindowManager wm;

	public ScreenDensityController(Context context) {
		super(context);
		mContext = context;
		wm = IWindowManager.Stub.asInterface(ServiceManager.getService(Context.WINDOW_SERVICE));
		}

	@Override
	public void displayPreference(PreferenceScreen screen) {
		if (isAvailable()) {
			setVisible(screen, getPreferenceKey(), true);
			mScreenDensity = (ListPreference) screen.findPreference(KEY_SCREEN_DENSITY);
			mScreenDensity.setOnPreferenceChangeListener(this);
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
		return KEY_SCREEN_DENSITY;
	}

	@Override
	public void updateState(Preference preference) {
		int currentDensity = getDensity();
		preference.setSummary(mContext.getString(R.string.screen_density_summary, "" + currentDensity + ""));
		mScreenDensity.setValue(String.valueOf(currentDensity));
	}

	public void setDensity(int density) {
		Log.d(TAG, "setDensity, density = " + density);
		try {
			wm.setForcedDisplayDensityForUser(DEFAULT_DISPLAY, density, UserHandle.USER_CURRENT);
		} catch (Exception e) {
			Log.d(TAG, "setDensity, density = " + density);
		}
	}

	public int getDensity() {
		try {
			int initialDensity = wm.getInitialDisplayDensity(DEFAULT_DISPLAY);
			int baseDensity = wm.getBaseDisplayDensity(DEFAULT_DISPLAY);
			Log.d(TAG, "getDensity,initialDensity=" + initialDensity + " baseDensity=" + baseDensity);

			if (initialDensity != baseDensity) {
				return baseDensity;
			} else {
				return initialDensity;
			}
		} catch (Exception e) {
			Log.e(TAG, "getDensity failed");
		}

		return DEFAULT_DENSITY;
	}

	@Override
	public boolean onPreferenceChange(Preference preference, Object newValue) {
		int density = Integer.parseInt((String) newValue);
		setDensity(density);
		return true;
	}
}

