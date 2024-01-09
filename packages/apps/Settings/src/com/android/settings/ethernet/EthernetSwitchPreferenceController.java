/*
 * Copyright (C) 2020 The Android Open Source Project
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

package com.android.settings.ethernet;

import android.content.Context;
import android.content.SharedPreferences;
import android.net.EthernetManager;
import android.util.Log;
import android.widget.Toast;

import androidx.preference.Preference;
import androidx.preference.PreferenceScreen;
import androidx.preference.SwitchPreference;

import com.android.settings.R;
import com.android.settingslib.core.AbstractPreferenceController;

import java.io.IOException;
/**
 * This controller helps to manage the state of wifi switch preference.
 */
public class EthernetSwitchPreferenceController extends AbstractPreferenceController
        implements Preference.OnPreferenceChangeListener {

    private static final String TAG = "EthernetSwitchPreferenceController";
    public static final String KEY = "main_toggle_ethernet";

    private SwitchPreference mPreference = null;;
    private EthernetManager mEthManager;
    private Context mContext;
    private SharedPreferences mPrefs = null;
    private Preference mEthernetSettings = null;
    private PreferenceScreen mPreferenceScreen = null;

    public EthernetSwitchPreferenceController(Context context) {
        super(context);
        mContext = context;
        mEthManager = (EthernetManager) mContext.getSystemService(Context.ETHERNET_SERVICE);
        mPrefs = mContext.getSharedPreferences(EthernetSettings.SHARED_PREFERENCES_NAME, Context.MODE_PRIVATE);
    }

    @Override
    public String getPreferenceKey() {
        return KEY;
    }

    @Override
    public void displayPreference(PreferenceScreen screen) {
        super.displayPreference(screen);
        mPreferenceScreen = screen;
        mPreference = screen.findPreference(getPreferenceKey());
        mEthernetSettings = mPreferenceScreen.findPreference(EthernetSettings.ETHERNET_SETTINGS_CONFIG);
        Log.w(TAG, "mPreference is null: " + (mPreference == null));
        mPreference.setChecked(mEthManager.isInterfaceup("eth0"));
    }

    private void showEthernetSettings(boolean show) {
        if (mPreferenceScreen == null) return;
        if (!show) {
            mPreferenceScreen.removePreference(mEthernetSettings);
        } else {
            mPreferenceScreen.addPreference(mEthernetSettings);
        }
    }

    @Override
    public boolean isAvailable() {
        return true;
    }

    private boolean tryEthNode() {
        try {
            Process p = Runtime.getRuntime().exec("ls sys/class/net/eth0");
            int tmp = p.waitFor();
            Log.w(TAG, "tryEthNode tmp: " + tmp);
            return tmp == 0;
        } catch (IOException | InterruptedException e) {
            Log.w(TAG, "access /proc/net/dev failed!");
        }
        return false;
    }

    @Override
    public boolean onPreferenceChange(Preference preference, Object newValue) {
        boolean isChecked = (Boolean) newValue;
        Log.w(TAG, "onPreferenceChange: " + isChecked);
        if (!tryEthNode()) {
            Toast.makeText(mContext, mContext.getString(R.string.not_support_eth), Toast.LENGTH_LONG).show();
            return false;
        }
        Log.w(TAG, "mPrefs is null: " + (mPrefs == null));

        if (mPrefs != null) {
            mPrefs.edit()
                    .putBoolean(KEY, isChecked)
                    .apply();
        }
        mEthManager.updateIfaceState("eth0", isChecked);
        return true;
    }
}
