/*
 * Copyright (C) 2021 The Android Open Source Project
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

package com.android.tv.settings.service.network;

import static com.android.tv.settings.util.InstrumentationUtils.logEntrySelected;

import android.app.tvsettings.TvSettingsEnums;
import android.content.Context;
import android.net.IpConfiguration;
import android.net.wifi.WifiConfiguration;
import android.net.wifi.WifiInfo;
import android.os.Bundle;
import android.os.RemoteException;
import android.text.TextUtils;
import android.util.Log;

import com.android.settingslib.wifi.AccessPoint;
import com.android.tv.settings.R;
import com.android.tv.settings.connectivity.ConnectivityListener;
import com.android.tv.settings.connectivity.EditIpSettingsActivity;
import com.android.tv.settings.connectivity.EditProxySettingsActivity;
import com.android.tv.settings.service.ISettingsServiceListener;
import com.android.tv.settings.service.PreferenceParcelable;
import com.android.tv.settings.service.ServiceUtil;
import com.android.tv.settings.service.data.PreferenceParcelableManager;
import com.android.tv.settings.service.data.State;

import java.util.ArrayList;
import java.util.List;

public class WifiDetailsState implements State,
        ConnectivityListener.Listener, ConnectivityListener.WifiNetworkListener {
    private static final String TAG = "WifiDetailsState";
    private final Context mContext;
    private final ISettingsServiceListener mServiceListener;
    private NetworkModule mNetworkModule;
    private AccessPoint mAccessPoint;
    PreferenceParcelableManager mPreferenceParcelableManager;

    private static final String KEY_CONNECTION_STATUS = "connection_status";
    private static final String KEY_IP_ADDRESS = "ip_address";
    private static final String KEY_MAC_ADDRESS = "mac_address";
    private static final String KEY_SIGNAL_STRENGTH = "signal_strength";
    private static final String KEY_RANDOM_MAC = "random_mac";
    private static final String KEY_PROXY_SETTINGS = "proxy_settings";
    private static final String KEY_IP_SETTINGS = "ip_settings";
    private static final String KEY_FORGET_NETWORK = "forget_network";

    private static final String VALUE_MAC_RANDOM = "random";
    private static final String VALUE_MAC_DEVICE = "device";


    private PreferenceParcelable mConnectionStatusPref;
    private PreferenceParcelable mIpAddressPref;
    private PreferenceParcelable mMacAddressPref;
    private PreferenceParcelable mSignalStrengthPref;
    // TODO: add support for list preference.
//    private ListPreference mRandomMacPref;
    private PreferenceParcelable mProxySettingsPref;
    private PreferenceParcelable mIpSettingsPref;
    private PreferenceParcelable mForgetNetworkPref;

    public WifiDetailsState(Context context, ISettingsServiceListener listener) {
        mServiceListener = listener;
        mContext = context;
        mNetworkModule = NetworkModule.getInstance(mContext);
    }

    @Override
    public void onCreate(Bundle extras) {
        mNetworkModule = NetworkModule.getInstance(mContext);
        mPreferenceParcelableManager = new PreferenceParcelableManager();
        mAccessPoint = mNetworkModule.getAccessPoint(extras);
        try {
            mServiceListener.notifyUpdateScreenTitle(getStateIdentifier(),
                    String.valueOf(mAccessPoint.getSsid()));
        } catch (RemoteException e) {
            Log.e(TAG, "Cannot set title for screen " + e);
        }
        mConnectionStatusPref = mPreferenceParcelableManager.getOrCreatePrefParcelable(
                KEY_CONNECTION_STATUS);
        mIpAddressPref = mPreferenceParcelableManager.getOrCreatePrefParcelable(KEY_IP_ADDRESS);
        mMacAddressPref = mPreferenceParcelableManager.getOrCreatePrefParcelable(KEY_MAC_ADDRESS);
        mSignalStrengthPref = mPreferenceParcelableManager.getOrCreatePrefParcelable(
                KEY_SIGNAL_STRENGTH);
//        mRandomMacPref = (ListPreference) findPreference(KEY_RANDOM_MAC);
        mProxySettingsPref = mPreferenceParcelableManager.getOrCreatePrefParcelable(
                KEY_PROXY_SETTINGS);
        mIpSettingsPref = mPreferenceParcelableManager.getOrCreatePrefParcelable(KEY_IP_SETTINGS);
        mForgetNetworkPref = mPreferenceParcelableManager.getOrCreatePrefParcelable(
                KEY_FORGET_NETWORK);
    }

    @Override
    public void onStart() {
        mNetworkModule.addState(this);
        mNetworkModule.getConnectivityListener().setWifiListener(this);
        mNetworkModule.getConnectivityListener().start();
    }

    @Override
    public void onResume() {
        update();
    }

    @Override
    public void onPause() {

    }

    @Override
    public void onStop() {
        mNetworkModule.getConnectivityListener().stop();
        mNetworkModule.removeState(this);
    }

    @Override
    public void onDestroy() {

    }

    @Override
    public void onPreferenceTreeClick(String key, boolean status) {
        switch (key) {
            case KEY_PROXY_SETTINGS:
                logEntrySelected(TvSettingsEnums.NETWORK_AP_INFO_PROXY_SETTINGS);
                break;
            case KEY_IP_SETTINGS:
                logEntrySelected(TvSettingsEnums.NETWORK_AP_INFO_IP_SETTINGS);
                break;
            case KEY_FORGET_NETWORK:
                logEntrySelected(TvSettingsEnums.NETWORK_AP_INFO_FORGET_NETWORK);
                break;
            default:
                // no-op
        }
    }

    @Override
    public int getStateIdentifier() {
        return ServiceUtil.STATE_WIFI_DETAILS;
    }

    @Override
    public void onConnectivityChange() {
        update();
    }

    @Override
    public void onWifiListChanged() {
        final List<AccessPoint> accessPoints = mNetworkModule
                .getConnectivityListener().getAvailableNetworks();
        for (final AccessPoint accessPoint : accessPoints) {
            if (TextUtils.equals(mAccessPoint.getSsidStr(), accessPoint.getSsidStr())
                    && mAccessPoint.getSecurity() == accessPoint.getSecurity()) {
                // Make sure we're not holding on to the one we inflated from the bundle, because
                // it won't be updated
                mAccessPoint = accessPoint;
                break;
            }
        }
        update();
    }

    private void update() {
        List<PreferenceParcelable> preferenceParcelables = new ArrayList<>();
        if (mAccessPoint == null) {
            return;
        }
        final boolean active = mAccessPoint.isActive();

        mConnectionStatusPref.setSummary(active
                ? mContext.getString(R.string.connected)
                : mContext.getString(R.string.not_connected));
        mIpAddressPref.setVisible(active);
        mSignalStrengthPref.setVisible(active);
        preferenceParcelables.add(mConnectionStatusPref);
        preferenceParcelables.add(mIpAddressPref);
        preferenceParcelables.add(mSignalStrengthPref);
        preferenceParcelables.add(mMacAddressPref);
        preferenceParcelables.add(mProxySettingsPref);
        preferenceParcelables.add(mIpSettingsPref);
        preferenceParcelables.add(mForgetNetworkPref);

        if (active) {
            mIpAddressPref.setSummary(mNetworkModule.getConnectivityListener().getWifiIpAddress());
            mSignalStrengthPref.setSummary(getSignalStrength());
        }

        // Mac address related Preferences (info entry and random mac setting entry)
        String macAddress = mNetworkModule.getConnectivityListener()
                .getWifiMacAddress(mAccessPoint);
        if (active && !TextUtils.isEmpty(macAddress)) {
            mMacAddressPref.setVisible(true);
            updateMacAddressPref(macAddress);
//            updateRandomMacPref();
        } else {
            mMacAddressPref.setVisible(false);
//            mRandomMacPref.setVisible(false);
        }

        WifiConfiguration wifiConfiguration = mAccessPoint.getConfig();
        if (wifiConfiguration != null) {
            final int networkId = wifiConfiguration.networkId;
            IpConfiguration.ProxySettings proxySettings =
                    wifiConfiguration.getIpConfiguration().getProxySettings();
            mProxySettingsPref.setSummary(proxySettings == IpConfiguration.ProxySettings.NONE
                    ? mContext.getString(R.string.wifi_action_proxy_none)
                    : mContext.getString(R.string.wifi_action_proxy_manual));
            mProxySettingsPref.setIntent(EditProxySettingsActivity.createIntent(mContext,
                    networkId));

            IpConfiguration.IpAssignment ipAssignment =
                    wifiConfiguration.getIpConfiguration().getIpAssignment();
            mIpSettingsPref.setSummary(ipAssignment == IpConfiguration.IpAssignment.STATIC
                    ? mContext.getString(R.string.wifi_action_static)
                    : mContext.getString(R.string.wifi_action_dhcp));
            mIpSettingsPref.setIntent(EditIpSettingsActivity.createIntent(mContext, networkId));

            // TODO : use full activity to replace this
//            mForgetNetworkPref.setFragment(WifiDetailsFragment.ForgetNetworkConfirmFragment
//            .class.getName());
//            WifiDetailsFragment.ForgetNetworkConfirmFragment.prepareArgs(mForgetNetworkPref
//            .getExtras(), mAccessPoint);
        }

        mProxySettingsPref.setVisible(wifiConfiguration != null);
        mIpSettingsPref.setVisible(wifiConfiguration != null);
        mForgetNetworkPref.setVisible(wifiConfiguration != null);
        try {
            mServiceListener.notifyUpdateAll(getStateIdentifier(),
                    PreferenceParcelableManager.prefParcelablesCopy(preferenceParcelables));
        } catch (RemoteException e) {
            Log.e(TAG, "remote failed: " + e);
        }
    }


    private void updateMacAddressPref(String macAddress) {
        if (WifiInfo.DEFAULT_MAC_ADDRESS.equals(macAddress)) {
            mMacAddressPref.setSummary(mContext.getString(R.string.mac_address_not_available));
        } else {
            mMacAddressPref.setSummary(macAddress);
        }
        if (mAccessPoint == null || mAccessPoint.getConfig() == null) {
            return;
        }
        // For saved Passpoint network, framework doesn't have the field to keep the MAC choice
        // persistently, so Passpoint network will always use the default value so far, which is
        // randomized MAC address, so don't need to modify title.
        if (mAccessPoint.isPasspoint() || mAccessPoint.isPasspointConfig()) {
            return;
        }
        mMacAddressPref.setTitle(
                (mAccessPoint.getConfig().macRandomizationSetting
                        == WifiConfiguration.RANDOMIZATION_PERSISTENT)
                        ? mContext.getString(R.string.title_randomized_mac_address)
                        : mContext.getString(R.string.title_mac_address));
    }


    private String getSignalStrength() {
        String[] signalLevels = mContext.getResources()
                .getStringArray(R.array.wifi_signal_strength);
        int strength = mNetworkModule.getConnectivityListener()
                .getWifiSignalStrength(signalLevels.length);
        return signalLevels[strength];
    }

}
