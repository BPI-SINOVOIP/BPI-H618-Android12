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

package com.android.settings.ethernet;

import android.app.Activity;
import android.app.AlertDialog;
import android.content.Context;
import android.content.SharedPreferences;
import android.net.IpConfiguration;
import android.net.IpConfiguration.IpAssignment;
import android.net.LinkAddress;
import android.net.LinkProperties;
import android.net.Network;
import android.net.RouteInfo;
import android.os.Bundle;
import android.util.Log;
import android.util.Slog;
import android.view.Menu;
import android.view.MenuInflater;
import android.view.MenuItem;
import android.view.View;
import android.widget.EditText;
import android.widget.Toast;

import androidx.preference.Preference;

import com.android.settings.R;
import com.android.settings.SettingsPreferenceFragment;
import com.android.settingslib.RestrictedSwitchPreference;

import java.net.Inet4Address;
import java.util.HashMap;
import java.util.Map;
import java.util.regex.Pattern;
public class EthernetSettingsConfig extends SettingsPreferenceFragment implements
        Preference.OnPreferenceChangeListener {
    private static final String TAG = "EthernetSettingsConfig";
    private static final int MENU_SAVE = 0;
    private static boolean DBG = true;

    //*******************************************************//
    private static final String KEY_TOGGLE = "eth_toggle";
    private static final String KEY_DEVICES_TITLE = "eth_device_title";
    private static final String KEY_ETH_CONFIG = "eth_configure";
    private static final String KEY_IP_PRE = "current_ip_address";
    private static final String KEY_MAC_PRE = "mac_address";
    private static final String KEY_GW_PRE = "gw_address";
    private static final String KEY_DNS1_PRE = "dns1_address";
    private static final String KEY_DNS2_PRE = "dns2_address";
    private static final String KEY_MASK_PRE = "mask_address";
    private static final String KEY_DHCP_MODE = "dhcp_mode";
    private static final String KEY_STATIC_MODE = "static_mode";
    private static final String KEY_CONNECT_MODE = "connect_mode";
    private static final String KEY_ETH_INFO = "eth_info";
    private static final String KEY_TOGGLE_ETHERNET_STATIC = "main_toggle_ethernet_static";
    private static final String DHCP_MODE = "dhcp";
    private static final String STATIC_MODE = "static";
    private static final String DEFAULT_IP = "0.0.0.0";
    private static final String DEFAULT_MASK = "255.255.255.255";

    private Preference mMacPreference = null;
    private Preference mIpPreference = null;
    private Preference mGwPreference = null;
    private Preference mDns1Preference = null;
    private Preference mDns2Preference = null;
    private Preference mMaskPreference = null;
    private RestrictedSwitchPreference mEthernetStatic = null;

    private boolean mUseStaticIp = false;
    private Map<String, Preference> ETH_ITEM_MAP = new HashMap<String, Preference>();
    private EthernetSettingsManager mEthernetSettingsManager = null;
    private SharedPreferences mPrefs = null;
    private Activity mActivity = null;

    @Override
    public void onCreate(Bundle icicle) {
        super.onCreate(icicle);
        addPreferencesFromResource(R.xml.ethernet_settings_config);
        mActivity = getActivity();
        mIpPreference = findPreference(KEY_IP_PRE);
        mMacPreference = findPreference(KEY_MAC_PRE);
        mGwPreference = findPreference(KEY_GW_PRE);
        mDns1Preference = findPreference(KEY_DNS1_PRE);
        mDns2Preference = findPreference(KEY_DNS2_PRE);
        mMaskPreference = findPreference(KEY_MASK_PRE);
        mEthernetStatic = findPreference(KEY_TOGGLE_ETHERNET_STATIC);

        ETH_ITEM_MAP.put(KEY_IP_PRE, mIpPreference);
        ETH_ITEM_MAP.put(KEY_GW_PRE, mGwPreference);
        ETH_ITEM_MAP.put(KEY_DNS1_PRE, mDns1Preference);
        ETH_ITEM_MAP.put(KEY_DNS2_PRE, mDns2Preference);
        ETH_ITEM_MAP.put(KEY_MASK_PRE, mMaskPreference);

        mEthernetStatic.setOnPreferenceChangeListener(this);
        mEthernetSettingsManager = new EthernetSettingsManager(mActivity);
        mPrefs = mActivity.getSharedPreferences(EthernetSettings.SHARED_PREFERENCES_NAME, Context.MODE_PRIVATE);

        /* first, we must get the Service. */
        if(DBG) Slog.d(TAG, "onCreate.");
        /* Get the SaveConfig and update for Dialog. */
    }

    @Override
    public boolean onPreferenceTreeClick(Preference preference) {
        final String key = preference.getKey();
        Log.w(TAG, "onPreferenceTreeClick: " + key);
        if (!preference.equals(mEthernetStatic) && mUseStaticIp) {
            showDialog(key);
            return mUseStaticIp;
        }
        return super.onPreferenceTreeClick(preference);
    }

    @Override
    public boolean onPreferenceChange(Preference preference, Object newValue) {
        final String key = preference.getKey();
        Log.w(TAG, "onPreferenceChange: key: " + key + " newValue: " + newValue);
        if (preference.equals(mEthernetStatic)) {
            mUseStaticIp = (Boolean) newValue;
            mEthernetStatic.setChecked(mUseStaticIp);
            mPrefs.edit().putBoolean(KEY_TOGGLE_ETHERNET_STATIC, mUseStaticIp).apply();
        }
        return true;
    }

    private boolean checkEthFormat(String data) {
        return Pattern.compile("\\d{1,3}\\.\\d{1,3}\\.\\d{1,3}\\.\\d{1,3}").matcher(data).matches();
    }

    private void showDialog(String key) {
        View view = getActivity().getLayoutInflater().inflate(R.layout.eth_config_input, null);
        EditText editText = (EditText) view.findViewById(R.id.config_content);
        editText.setText(ETH_ITEM_MAP.get(key).getSummary());
        new AlertDialog.Builder(getActivity())
                .setCancelable(true)
                .setTitle(ETH_ITEM_MAP.get(key).getTitle())
                .setView(view)
                .setPositiveButton(R.string.eth_ok, (dialog, which) -> {
                    String tmpEthData = editText.getText().toString();
                    if (checkEthFormat(tmpEthData)) ETH_ITEM_MAP.get(key).setSummary(tmpEthData);
                    else showToast(getActivity().getString(R.string.error_data_format));
                })
                .setNegativeButton(R.string.eth_cancel, (dialog, which) -> {
                    dialog.dismiss();
                })
                .create()
                .show();
    }

    private void showToast(String msg) {
        Toast.makeText(getActivity(), msg, Toast.LENGTH_LONG).show();
    }

    private void savePrefs() {
        ETH_ITEM_MAP.forEach((key, value) -> {
            String val = value.getSummary().toString();
            Log.w(TAG, "value.getSummary().toString(): " + val);
            mPrefs.edit().putString(key, val).apply();
        });
    }

    private void setPrefs() {
        ETH_ITEM_MAP.forEach((key, value) -> {
            Log.w(TAG, "value.getSummary().toString(): " + value.getSummary().toString());
            String sum = mPrefs.getString(key, KEY_MASK_PRE.equals(key) ? DEFAULT_MASK : DEFAULT_IP);
            value.setSummary(sum);
        });
    }

    @Override
    public void onCreateOptionsMenu(Menu menu, MenuInflater inflater) {
        MenuItem item = menu.add(0, MENU_SAVE, 0, "save");
        item.setShowAsAction(MenuItem.SHOW_AS_ACTION_ALWAYS);
        super.onCreateOptionsMenu(menu, inflater);
    }

    @Override
    public boolean onOptionsItemSelected(MenuItem menuItem) {
        if (menuItem.getItemId() == MENU_SAVE) {
            saveConfig();
            finishSave();
            return true;
        }
        return super.onOptionsItemSelected(menuItem);
    }

    private void finishSave() {
        getActivity().finish();
    }

    @Override
    public void onSaveInstanceState(Bundle outState) {
        super.onSaveInstanceState(outState);
    }

    @Override
    public void onViewCreated(View view, Bundle savedInstanceState) {
        super.onViewCreated(view, savedInstanceState);
    }

    @Override
    public void onAttach(Context context) {
        super.onAttach(context);
    }

    @Override
    public int getMetricsCategory() {
        return 0;
    }

    @Override
    public void onPause() {
        super.onPause();
    }

    @Override
    public void onResume() {
        super.onResume();
        mUseStaticIp = mPrefs.getBoolean(KEY_TOGGLE_ETHERNET_STATIC, false);
        mEthernetStatic.setChecked(mUseStaticIp);
        loadEthInfo();
        Log.w(TAG, "mUseStaticIp: " + mUseStaticIp);
    }

    private void saveConfig() {
        savePrefs();
        setEthInfo();
    }

    private void loadEthInfo() {
        IpConfiguration ipConfiguration = mEthernetSettingsManager.getIpConfiguration();
        mEthernetSettingsManager.processIpSettings(getEthernetConfig());
        updateEthernetInfo(mUseStaticIp);
    }

    private void setEthInfo() {
        IpConfiguration ipConfiguration = mEthernetSettingsManager.getIpConfiguration();
        mEthernetSettingsManager.setIpAssignment(mUseStaticIp);
        mEthernetSettingsManager.processIpSettings(getEthernetConfig());
        mEthernetSettingsManager.setConfiguration();
    }

    private EthernetConfig getEthernetConfig() {
        EthernetConfig ethernetConfig = new EthernetConfig(mIpPreference.getSummary().toString());
        ethernetConfig.setGw(mGwPreference.getSummary().toString());
        ethernetConfig.setDns1(mDns1Preference.getSummary().toString());
        ethernetConfig.setDns2(mDns2Preference.getSummary().toString());
        ethernetConfig.setMask(mMaskPreference.getSummary().toString());
        return ethernetConfig;
    }

    private void setConfiguration(IpConfiguration config) {
        if ((config.getIpAssignment() == IpAssignment.DHCP
                || config.getIpAssignment() == IpAssignment.STATIC)) {
            updateEthernetInfo(mUseStaticIp);
            mEthernetSettingsManager.setConfiguration();
        }
    }

    private void updateEthernetInfo(boolean isStatic) {
        // rewrite to mask
        setPrefs();
        if (isStatic) {
            return;
        }
        Network network = mEthernetSettingsManager.getFirstNetwork();
        if (network == null) {
            Log.w(TAG,"network is null");
            return;
        }
        final LinkProperties linkProperties = mEthernetSettingsManager.getLinkProperties(network);
        for (LinkAddress linkAddress : linkProperties.getLinkAddresses()) { //set ipaddress.
            if (linkAddress.getAddress() instanceof Inet4Address) {
                mIpPreference.setSummary(linkAddress.getAddress().getHostAddress());
                mMaskPreference.setSummary(String.valueOf(linkAddress.getPrefixLength()));
                break;
            }
        }

        int dnsCount = linkProperties.getDnsServers().size();
        String dns1 = null;
        String dns2 = null;

        switch (dnsCount) {
            case 2:
                dns2 = linkProperties.getDnsServers().get(1).getHostAddress().toString();
            case 1:
                dns1 = linkProperties.getDnsServers().get(0).getHostAddress().toString();
        }
        mDns1Preference.setSummary(dns1);
        mDns2Preference.setSummary(dnsCount > 1 ? dns2 : "0.0.0.0");
        String route = null;
        for (RouteInfo routeInfo : linkProperties.getRoutes()){
            if (isIPv4Default(routeInfo)) {
                route = routeInfo.getGateway().toString();
                break;
            }
        }
        String[] routeStr = route.split("/");
        mGwPreference.setSummary(routeStr[1]);
    }

    private boolean isIPv4Default(RouteInfo routeInfo) {
        return routeInfo.isDefaultRoute() && routeInfo.getDestination().getAddress() instanceof Inet4Address;
    }

    private void addAllPreference() {
        getPreferenceScreen().addPreference(mIpPreference);
        getPreferenceScreen().addPreference(mGwPreference);
        getPreferenceScreen().addPreference(mDns1Preference);
        getPreferenceScreen().addPreference(mDns2Preference);
        getPreferenceScreen().addPreference(mMaskPreference);
    }
}
