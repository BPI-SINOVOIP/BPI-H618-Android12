/*
 * Copyright (C) 2016 The Android Open Source Project
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
 * limitations under the License
 */

package com.android.tv.settings.connectivity;

import static android.net.ConnectivityManager.TETHERING_WIFI;

import android.content.Context;
import android.net.wifi.WifiConfiguration;
import android.os.Bundle;
import android.os.Handler;
import android.os.Looper;
import android.os.SystemClock;
import android.provider.Settings;
import androidx.annotation.Keep;
import androidx.preference.Preference;
import androidx.preference.PreferenceCategory;
import androidx.preference.PreferenceManager;
import androidx.preference.TwoStatePreference;

import com.android.internal.logging.nano.MetricsProto;
import com.android.settingslib.wifi.AccessPoint;
import com.android.settingslib.wifi.AccessPointPreference;
import com.android.tv.settings.R;
import com.android.tv.settings.SettingsPreferenceFragment;
import com.android.tv.settings.PreferenceControllerFragment;
import com.android.tv.settings.connectivity.util.WifiHotspotParameter;
import android.text.TextUtils;
import android.os.ServiceManager;
import android.app.Activity;
import android.widget.Toast;
import android.content.Intent;
import android.content.IntentFilter;
import android.content.BroadcastReceiver;
import android.os.Bundle;
import android.net.wifi.SoftApConfiguration;
import android.net.wifi.WifiManager;
import android.net.wifi.WifiConfiguration;
import android.net.ConnectivityManager;
import android.provider.Settings;

import java.util.Collection;
import java.util.HashSet;
import java.util.Set;
import java.util.List;

/**
 * Fragment for controlling wifi hotspot
 */
@Keep
public class HotspotFragment extends SettingsPreferenceFragment {
    private static final String KEY_HOTSPOT_ENABLE = "hotspot_enable";
    private static final String KEY_HOTSPOT_NAME = "hotspot_name";
    private static final String KEY_HOTSPOT_SECURITY = "hotspot_security";
    private static final String KEY_HOTSPOT_PASSWORD = "hotspot_password";
    private static final String KEY_HOTSPOT_TURN_OFF_AUTO = "hotspot_turn_off_auto";
    private static final String KEY_AP_BAND = "ap_band";

    private static final String TAG = "HotspotFragment";
    private WifiManager mWifiManager;
    private ConnectivityManager mConnectivityManager;

    private TwoStatePreference mEnableHotspotPref;
    public static Preference mHotspotnamePref;
    public static Preference mHotspotSecurityPref;
    public static Preference mHotspotPasswordPref;
    private TwoStatePreference mTurnoffauto;
    public static Preference mApbandPref;

    final ConnectivityManager.OnStartTetheringCallback mOnStartTetheringCallback =
            new ConnectivityManager.OnStartTetheringCallback() {
                @Override
                public void onTetheringFailed() {
                    super.onTetheringFailed();
                }
            };

    public static HotspotFragment newInstance() {
        return new HotspotFragment();
    }

    @Override
    public void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
    }

    @Override
    public void onStart() {
        super.onStart();
    }

    @Override
    public void onStop() {
        super.onStop();
    }

    @Override
    public void onAttach(Context context) {
        super.onAttach(context);
        mWifiManager = (WifiManager) context.getSystemService(Context.WIFI_SERVICE);
        mConnectivityManager =
                (ConnectivityManager) context.getSystemService(Context.CONNECTIVITY_SERVICE);
    }

    public void onResume() {
        super.onResume();
    }

    @Override
    public void onCreatePreferences(Bundle savedInstanceState, String rootKey) {
        getPreferenceManager().setPreferenceComparisonCallback(
                new PreferenceManager.SimplePreferenceComparisonCallback());
        setPreferencesFromResource(R.xml.wifi_hotspot, null);

        mEnableHotspotPref = (TwoStatePreference) findPreference(KEY_HOTSPOT_ENABLE);
        mHotspotnamePref = findPreference(KEY_HOTSPOT_NAME);
        mHotspotSecurityPref = findPreference(KEY_HOTSPOT_SECURITY);
        mHotspotPasswordPref = findPreference(KEY_HOTSPOT_PASSWORD);
        mApbandPref = findPreference(KEY_AP_BAND);
        mTurnoffauto = (TwoStatePreference) findPreference(KEY_HOTSPOT_TURN_OFF_AUTO);
        final boolean settingsOn = Settings.Global.getInt(getContext().getContentResolver(),
                Settings.Global.SOFT_AP_TIMEOUT_ENABLED, 1) != 0;
        mTurnoffauto.setChecked(settingsOn);
        if (mWifiManager.getWifiApState() == WifiManager.WIFI_AP_STATE_ENABLED) {
            mEnableHotspotPref.setChecked(true);
            mHotspotnamePref.setEnabled(false);
            mHotspotSecurityPref.setEnabled(false);
            mHotspotPasswordPref.setEnabled(false);
            mApbandPref.setEnabled(false);
            mTurnoffauto.setEnabled(false);
        } else {
            mEnableHotspotPref.setChecked(false);
        }
        showConfigSummary();
    }

    @Override
    public boolean onPreferenceTreeClick(Preference preference) {
        if (preference.getKey() == null) {
            return super.onPreferenceTreeClick(preference);
        }
        switch (preference.getKey()) {
            case KEY_HOTSPOT_ENABLE:
                updateHotspotConfig();
                setWifiApEnabled(mEnableHotspotPref.isChecked());
                return true;
            case KEY_HOTSPOT_TURN_OFF_AUTO:
                Settings.Global.putInt(getContext().getContentResolver(),
                         Settings.Global.SOFT_AP_TIMEOUT_ENABLED,
                         mTurnoffauto.isChecked() ? 1 : 0);
                break;
        }
        return super.onPreferenceTreeClick(preference);
    }

    private SoftApConfiguration buildNewConfig() {
        final SoftApConfiguration nowconfig = mWifiManager.getSoftApConfiguration();
        String ssid;
        int securityType;
        String preSharedKey;
        int apBand;
        if (WifiHotspotParameter.getNewHotspotName() == "") {
            ssid = nowconfig.getSsid();
        } else {
            ssid = WifiHotspotParameter.getNewHotspotName();
        }
        if (WifiHotspotParameter.getSecurityType() == -1) {
            securityType = nowconfig.getSecurityType();
        } else {
            securityType = WifiHotspotParameter.getSecurityType();
        }
        if (WifiHotspotParameter.getNewHotspotPassword() == "") {
            preSharedKey = nowconfig.getPassphrase();
        } else {
            preSharedKey = WifiHotspotParameter.getNewHotspotPassword();
        }
        if (securityType == WifiConfiguration.KeyMgmt.NONE) {
            preSharedKey = "";
        }
        if (!is5GhzBandSupported()) {
            mApbandPref.setEnabled(false);
        }
        if (WifiHotspotParameter.getApbandType() == -1) {
            apBand = nowconfig.getBand();
        } else {
            apBand = WifiHotspotParameter.getApbandType();
        }
        final SoftApConfiguration config = new SoftApConfiguration.Builder()
            .setSsid(ssid)
            .setPassphrase(preSharedKey, securityType)
            .setBand(apBand)
            .build();
        return config;
    }

    public void stopTether() {
        mConnectivityManager.stopTethering(TETHERING_WIFI);
    }

    public void startTether() {
        mConnectivityManager.startTethering(TETHERING_WIFI, true /* showProvisioningUi */,
                mOnStartTetheringCallback, new Handler(Looper.getMainLooper()));
    }

    private void setWifiApEnabled(boolean enable) {
        if (enable) {
            startTether();
            mHotspotnamePref.setEnabled(false);
            mHotspotSecurityPref.setEnabled(false);
            mHotspotPasswordPref.setEnabled(false);
            if (is5GhzBandSupported()) {
                mApbandPref.setEnabled(false);
            }
            mTurnoffauto.setEnabled(false);
        } else {
            stopTether();
            mHotspotnamePref.setEnabled(true);
            mHotspotSecurityPref.setEnabled(true);
            mHotspotPasswordPref.setEnabled(true);
            if (is5GhzBandSupported()) {
                mApbandPref.setEnabled(true);
            }
            mTurnoffauto.setEnabled(true);
        }
    }

    public static void setHotspotNameSummary(String name) {
        mHotspotnamePref.setSummary(name);
    }

    public static void setHotspotPasswordSummary(String password) {
        mHotspotPasswordPref.setSummary(password);
    }

    public static void setHotspotSecuritySummary(int typevalue) {
        if (typevalue == WifiConfiguration.KeyMgmt.NONE ) {
            mHotspotSecurityPref.setSummary(R.string.hotspot_security_type_none);
            mHotspotPasswordPref.setVisible(false);
        } else {
            mHotspotSecurityPref.setSummary(R.string.hotspot_security_type_wpa2_psk);
            mHotspotPasswordPref.setVisible(true);
        }
    }

    public static void setHotspotApbandSummary(int typevalue) {
        if (typevalue == SoftApConfiguration.BAND_5GHZ ) {
            mApbandPref.setSummary(R.string.hotspot_apband_5G);
        } else {
            mApbandPref.setSummary(R.string.hotspot_apband_2G);
        }
    }

    public void updateHotspotConfig() {
        final SoftApConfiguration config = buildNewConfig();
        mWifiManager.setSoftApConfiguration(config);
    }

    private void showConfigSummary() {
        final WifiConfiguration config = mWifiManager.getWifiApConfiguration();
        final SoftApConfiguration softApConfig = mWifiManager.getSoftApConfiguration();
            if (WifiHotspotParameter.hotspotname != "") {
                mHotspotnamePref.setSummary(WifiHotspotParameter.hotspotname);
            } else {
                mHotspotnamePref.setSummary(config.SSID);
            }

            if (config.getAuthType() == WifiConfiguration.KeyMgmt.NONE) {
                mHotspotSecurityPref.setSummary(R.string.hotspot_security_type_none);
                mHotspotPasswordPref.setVisible(false);
            } else {
                mHotspotSecurityPref.setSummary(R.string.hotspot_security_type_wpa2_psk);
                mHotspotPasswordPref.setVisible(true);
            }

            if (WifiHotspotParameter.hotspotpassword != "") {
                mHotspotPasswordPref.setSummary(WifiHotspotParameter.hotspotpassword);
            } else {
                mHotspotPasswordPref.setSummary(config.preSharedKey);
            }

            if (!is5GhzBandSupported()) {
                mApbandPref.setEnabled(false);
            }

            if (softApConfig.getBand() == SoftApConfiguration.BAND_5GHZ) {
                mApbandPref.setSummary(R.string.hotspot_apband_5G);
            } else {
                mApbandPref.setSummary(R.string.hotspot_apband_2G);
            }
    }

    private boolean is5GhzBandSupported() {
        final String countryCode = mWifiManager.getCountryCode();
        if (!mWifiManager.is5GHzBandSupported() || countryCode == null) {
            return false;
        }
        return true;
    }

    @Deprecated
    public int getMetricsCategory() {
        return MetricsProto.MetricsEvent.WIFI_TETHER_SETTINGS;
    }
}
