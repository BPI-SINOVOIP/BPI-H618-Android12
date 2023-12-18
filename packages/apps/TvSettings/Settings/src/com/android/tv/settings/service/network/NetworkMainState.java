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

import static com.android.tv.settings.service.ServiceUtil.INFO_COLLAPSE;
import static com.android.tv.settings.util.InstrumentationUtils.logEntrySelected;
import static com.android.tv.settings.util.InstrumentationUtils.logToggleInteracted;

import android.app.tvsettings.TvSettingsEnums;
import android.content.Context;
import android.net.NetworkCapabilities;
import android.net.NetworkInfo;
import android.os.Bundle;
import android.os.Handler;
import android.os.RemoteException;
import android.os.SystemClock;
import android.provider.Settings;
import android.util.Log;

import com.android.settingslib.wifi.AccessPoint;
import com.android.tv.settings.R;
import com.android.tv.settings.connectivity.ConnectivityListener;
import com.android.tv.settings.connectivity.WifiConnectionActivity;
import com.android.tv.settings.service.ISettingsServiceListener;
import com.android.tv.settings.service.PreferenceParcelable;
import com.android.tv.settings.service.ServiceUtil;
import com.android.tv.settings.service.data.PreferenceParcelableManager;
import com.android.tv.settings.service.data.State;

import java.util.ArrayList;
import java.util.Collection;
import java.util.List;

/** State to provide data for rendering NetworkFragment. */
public class NetworkMainState implements State, AccessPoint.AccessPointListener,
        ConnectivityListener.WifiNetworkListener, ConnectivityListener.Listener {
    private static final String TAG = "NetworkMainState";
    private static final boolean DEBUG = true;
    private static final String KEY_WIFI_ENABLE = "wifi_enable";
    private static final String KEY_WIFI_LIST = "wifi_list";
    private static final String KEY_WIFI_COLLAPSE = "wifi_collapse";
    private static final String KEY_WIFI_OTHER = "wifi_other";
    private static final String KEY_WIFI_ADD = "wifi_add";
    private static final String KEY_WIFI_ALWAYS_SCAN = "wifi_always_scan";
    private static final String KEY_ETHERNET = "ethernet";
    private static final String KEY_ETHERNET_STATUS = "ethernet_status";
    private static final String KEY_ETHERNET_PROXY = "ethernet_proxy";
    private static final String KEY_ETHERNET_DHCP = "ethernet_dhcp";
    private static final String KEY_DATA_SAVER_SLICE = "data_saver_slice";
    private static final String KEY_DATA_ALERT_SLICE = "data_alert_slice";
    private static final String KEY_NETWORK_DIAGNOSTICS = "network_diagnostics";
    private static final int INITIAL_UPDATE_DELAY = 500;

    PreferenceParcelable mEnableWifiPref;
    PreferenceParcelable mCollapsePref;
    PreferenceParcelable mAddPref;
    PreferenceParcelable mEthernetCategory;
    PreferenceParcelable mEthernetStatusPref;
    PreferenceParcelable mEthernetProxyPref;
    PreferenceParcelable mAlwaysScan;
    PreferenceParcelable mWifiNetworkCategoryPref;
    PreferenceParcelableManager mPreferenceParcelableManager;
    private NetworkModule mNetworkModule;
    private final Context mContext;
    private final ISettingsServiceListener mServiceListener;
    private final Handler mHandler = new Handler();
    private long mNoWifiUpdateBeforeMillis;
    private final Runnable mInitialUpdateWifiListRunnable = new Runnable() {
        @Override
        public void run() {
            mNoWifiUpdateBeforeMillis = 0;
            updateWifiList();
        }
    };

    public NetworkMainState(Context context, ISettingsServiceListener listener) {
        mServiceListener = listener;
        mContext = context;
    }

    @Override
    public void onCreate(Bundle extras) {
        mNetworkModule = NetworkModule.getInstance(mContext);
        mPreferenceParcelableManager = new PreferenceParcelableManager();
        mEnableWifiPref = mPreferenceParcelableManager.getOrCreatePrefParcelable(KEY_WIFI_ENABLE);
        mAlwaysScan = mPreferenceParcelableManager.getOrCreatePrefParcelable(KEY_WIFI_ALWAYS_SCAN);
        mCollapsePref = mPreferenceParcelableManager.getOrCreatePrefParcelable(KEY_WIFI_COLLAPSE);
        mCollapsePref.addInfo(INFO_COLLAPSE, "true");
        mAddPref = mPreferenceParcelableManager.getOrCreatePrefParcelable(KEY_WIFI_ADD);
        mEthernetCategory = mPreferenceParcelableManager.getOrCreatePrefParcelable(KEY_ETHERNET);
        mEthernetStatusPref = mPreferenceParcelableManager.getOrCreatePrefParcelable(
                KEY_ETHERNET_STATUS);
        mEthernetProxyPref = mPreferenceParcelableManager.getOrCreatePrefParcelable(
                KEY_ETHERNET_PROXY);
        mWifiNetworkCategoryPref = mPreferenceParcelableManager.getOrCreatePrefParcelable(
                KEY_WIFI_LIST);
        mWifiNetworkCategoryPref.addInfo(INFO_COLLAPSE, "true");
        mWifiNetworkCategoryPref.setType(
                PreferenceParcelable.TYPE_PREFERENCE_WIFI_COLLAPSE_CATEGORY);
    }

    @Override
    public void onStart() {
        mNetworkModule.addState(this);
        mNetworkModule.getConnectivityListener().setWifiListener(this);
        mNoWifiUpdateBeforeMillis = SystemClock.elapsedRealtime() + INITIAL_UPDATE_DELAY;
        mNetworkModule.getConnectivityListener().start();
        updateWifiList();
    }

    @Override
    public void onResume() {
        updateConnectivity();
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

    private void updateWifiList() {
        if (!mNetworkModule.isWifiHardwarePresent() ||
                !mNetworkModule.getConnectivityListener().isWifiEnabledOrEnabling()) {
            mNoWifiUpdateBeforeMillis = 0;
            return;
        }

        final long now = SystemClock.elapsedRealtime();
        if (mNoWifiUpdateBeforeMillis > now) {
            mHandler.removeCallbacks(mInitialUpdateWifiListRunnable);
            mHandler.postDelayed(mInitialUpdateWifiListRunnable,
                    mNoWifiUpdateBeforeMillis - now);
            return;
        }

        final Collection<AccessPoint> accessPoints =
                mNetworkModule.getConnectivityListener().getAvailableNetworks();
        mWifiNetworkCategoryPref.initChildPreferences();
        for (final AccessPoint accessPoint : accessPoints) {
            accessPoint.setListener(this);
            PreferenceParcelable accessPointPref = new PreferenceParcelable(
                    new String[]{KEY_WIFI_LIST, accessPoint.getKey()});
            accessPointPref.setTitle(accessPoint.getTitle());
            accessPointPref.setType(PreferenceParcelable.TYPE_PREFERENCE_ACCESS_POINT);
            accessPointPref.addInfo(ServiceUtil.INFO_WIFI_SIGNAL_LEVEL,
                    String.valueOf(accessPoint.getLevel()));
            if (accessPoint.isActive() && !isCaptivePortal(accessPoint)) {
                Bundle apBundle = new Bundle();
                accessPoint.saveWifiState(apBundle);
                accessPointPref.setExtras(apBundle);
                accessPointPref.addInfo(ServiceUtil.INFO_NEXT_STATE, String.valueOf(
                        ServiceUtil.STATE_WIFI_DETAILS));
                accessPointPref.setIntent(null);
            } else {
                accessPointPref.setIntent(
                        WifiConnectionActivity.createIntent(mContext, accessPoint));
            }
            mWifiNetworkCategoryPref.addChildPrefParcelable(accessPointPref);
        }
        mNetworkModule.setAccessPoints(mWifiNetworkCategoryPref.getChildPrefParcelables());
        try {
            mServiceListener.notifyUpdate(getStateIdentifier(),
                    PreferenceParcelableManager.prefParcelableCopy(mWifiNetworkCategoryPref));
        } catch (RemoteException e) {
            Log.e(TAG, "remote failed: " + e);
        }
    }

    @Override
    public void onPreferenceTreeClick(String key, boolean status) {
        switch (key) {
            case KEY_WIFI_ENABLE:
                mNetworkModule.getConnectivityListener().setWifiEnabled(status);
                mEnableWifiPref.setChecked(status);
                break;
            case KEY_WIFI_COLLAPSE:
                boolean collapse = !("true".equals(
                        mWifiNetworkCategoryPref.getInfo(ServiceUtil.INFO_COLLAPSE)));
                mWifiNetworkCategoryPref.addInfo(ServiceUtil.INFO_COLLAPSE,
                        String.valueOf(collapse));
                mCollapsePref.addInfo(ServiceUtil.INFO_COLLAPSE, String.valueOf(collapse));
                logEntrySelected(
                        collapse
                                ? TvSettingsEnums.NETWORK_SEE_FEWER
                                : TvSettingsEnums.NETWORK_SEE_ALL);
                try {
                    mServiceListener.notifyUpdate(getStateIdentifier(),
                            PreferenceParcelableManager.prefParcelableCopy(
                                    mWifiNetworkCategoryPref));
                } catch (RemoteException e) {
                    Log.e(TAG, "remote failed: " + e);
                }
                break;
            case KEY_WIFI_ALWAYS_SCAN:
                mAlwaysScan.setChecked(status);
                Settings.Global.putInt(mContext.getContentResolver(),
                        Settings.Global.WIFI_SCAN_ALWAYS_AVAILABLE,
                        status ? 1 : 0);
                logToggleInteracted(
                        TvSettingsEnums.NETWORK_ALWAYS_SCANNING_NETWORKS, status);
                break;
            case KEY_ETHERNET_STATUS:
                break;
            case KEY_WIFI_ADD:
                logEntrySelected(TvSettingsEnums.NETWORK_ADD_NEW_NETWORK);
                break;
            case KEY_ETHERNET_DHCP:
                logEntrySelected(TvSettingsEnums.NETWORK_ETHERNET_IP_SETTINGS);
                break;
            case KEY_ETHERNET_PROXY:
                logEntrySelected(TvSettingsEnums.NETWORK_ETHERNET_PROXY_SETTINGS);
                break;
            default:
                // no-op
        }
        try {
            mServiceListener.notifyUpdate(getStateIdentifier(),
                    mPreferenceParcelableManager.prefParcelableCopy(key));
        } catch (RemoteException e) {
            Log.e(TAG, "remote failed: " + e);
        }
    }


    private void updateConnectivity() {
        List<PreferenceParcelable> preferenceParcelables = new ArrayList<>();
        final boolean wifiEnabled = mNetworkModule.isWifiHardwarePresent()
                && mNetworkModule.getConnectivityListener().isWifiEnabledOrEnabling();
        mEnableWifiPref.setChecked(wifiEnabled);
        preferenceParcelables.add(mEnableWifiPref);

        mWifiNetworkCategoryPref.setVisible(wifiEnabled);
        preferenceParcelables.add(mWifiNetworkCategoryPref);

        mCollapsePref.setVisible(wifiEnabled);
        preferenceParcelables.add(mCollapsePref);

        mAddPref.setVisible(wifiEnabled);
        preferenceParcelables.add(mAddPref);


        if (!wifiEnabled) {
            updateWifiList();
        }

        int scanAlwaysAvailable = 0;
        try {
            scanAlwaysAvailable = Settings.Global.getInt(mContext.getContentResolver(),
                    Settings.Global.WIFI_SCAN_ALWAYS_AVAILABLE);
        } catch (Settings.SettingNotFoundException e) {
            // Ignore
        }

        mAlwaysScan.setChecked(scanAlwaysAvailable == 1);
        mAlwaysScan.setContentDescription(
                mContext.getString(R.string.wifi_setting_always_scan_content_description));

        final boolean ethernetAvailable =
                mNetworkModule.getConnectivityListener().isEthernetAvailable();
        mEthernetCategory.setVisible(ethernetAvailable);
        mEthernetStatusPref.setVisible(ethernetAvailable);
        mEthernetProxyPref.setVisible(ethernetAvailable);
        preferenceParcelables.add(mEthernetCategory);
        preferenceParcelables.add(mEthernetStatusPref);
        preferenceParcelables.add(mEthernetProxyPref);
        if (ethernetAvailable) {
            final boolean ethernetConnected =
                    mNetworkModule.getConnectivityListener().isEthernetConnected();
            mEthernetStatusPref.setTitle(ethernetConnected
                    ? mContext.getString(R.string.connected)
                    : mContext.getString(R.string.not_connected));
            mEthernetStatusPref.setSummary(
                    mNetworkModule.getConnectivityListener().getEthernetIpAddress());
        }

        try {
            mServiceListener.notifyUpdateAll(getStateIdentifier(),
                    PreferenceParcelableManager.prefParcelablesCopy(preferenceParcelables));
        } catch (RemoteException e) {
            Log.e(TAG, "remote failed: " + e);
        }
    }

    @Override
    public void onAccessPointChanged(AccessPoint accessPoint) {
        PreferenceParcelable accessPointPref = new PreferenceParcelable(
                new String[]{KEY_WIFI_LIST, accessPoint.getKey()});
        try {
            mServiceListener.notifyUpdate(getStateIdentifier(),
                    PreferenceParcelableManager.prefParcelableCopy(accessPointPref));
        } catch (RemoteException e) {
            Log.e(TAG, "remote failed: " + e);
        }
    }

    @Override
    public void onLevelChanged(AccessPoint accessPoint) {
        PreferenceParcelable accessPointPref = new PreferenceParcelable(
                new String[]{KEY_WIFI_LIST, accessPoint.getKey()});
        try {
            mServiceListener.notifyUpdate(getStateIdentifier(),
                    PreferenceParcelableManager.prefParcelableCopy(accessPointPref));
        } catch (RemoteException e) {
            Log.e(TAG, "remote failed: " + e);
        }
    }

    @Override
    public void onWifiListChanged() {
        updateWifiList();
    }


    @Override
    public int getStateIdentifier() {
        return ServiceUtil.STATE_NETWORK_MAIN;
    }

    private boolean isCaptivePortal(AccessPoint accessPoint) {
        if (accessPoint.getDetailedState() != NetworkInfo.DetailedState.CONNECTED) {
            return false;
        }
        NetworkCapabilities nc = mNetworkModule.getConnectivityManager().getNetworkCapabilities(
                mNetworkModule.getWifiManager().getCurrentNetwork());
        return nc != null && nc.hasCapability(NetworkCapabilities.NET_CAPABILITY_CAPTIVE_PORTAL);
    }

    @Override
    public void onConnectivityChange() {
        updateConnectivity();
    }
}
