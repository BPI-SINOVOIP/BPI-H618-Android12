/*
 * Copyright (C) 2011 The Android Open Source Project
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

import com.android.settings.R;

import android.app.Dialog;
import android.app.AlertDialog;
import android.app.Activity;
import android.content.Context;
import android.content.DialogInterface;
import android.content.BroadcastReceiver;
import android.content.DialogInterface;
import android.content.Intent;
import android.content.IntentFilter;
import android.net.ConnectivityManager;
import android.net.NetworkInfo;
import android.net.Network;
import android.net.NetworkInfo.DetailedState;
import android.net.LinkProperties;
import android.net.LinkAddress;
import android.net.EthernetManager;
import android.net.InterfaceConfiguration;
import android.net.StaticIpConfiguration;
import android.net.IpConfiguration;
import android.net.NetworkUtils;
import android.net.IpConfiguration.IpAssignment;
import android.os.INetworkManagementService;
import android.net.RouteInfo;
import android.os.Bundle;
import android.os.Handler;
import android.os.Message;
import android.os.ServiceManager;
import android.support.v7.preference.Preference;
import android.support.v7.preference.PreferenceScreen;
import android.support.v7.preference.PreferenceCategory;
import android.util.Log;
import android.util.Slog;
import android.view.ContextMenu;
import android.view.ContextMenu.ContextMenuInfo;
import android.view.Menu;
import android.view.MenuItem;
import android.view.View;
import android.text.TextUtils;
import android.widget.AdapterView.AdapterContextMenuInfo;
import android.content.ContentResolver;
import com.android.settings.SettingsPreferenceFragment;
import android.provider.Settings;
import com.android.settings.SettingsActivity;
import com.android.internal.logging.nano.MetricsProto.MetricsEvent;

import java.net.Inet4Address;
import java.util.Arrays;
import java.util.HashMap;
import java.util.List;
import java.util.ArrayList;
import android.os.AsyncTask;
import android.widget.TextView;
import android.widget.Switch;
import android.app.ActionBar;
import android.view.Gravity;
import android.widget.Toast;
import java.net.InetAddress;
import java.lang.reflect.Field;

public class EthernetSettings extends SettingsPreferenceFragment implements
        Preference.OnPreferenceChangeListener, RadioButtonPreference.OnClickListener,
        DialogInterface.OnClickListener, DialogInterface.OnDismissListener {

    private static final String TAG = "EthernetSettings";
    private static boolean DBG = true;

    private ConnectivityManager mConnectivityManager;
    private IpConfiguration mIpConfiguration;
    //private static final String KEY_TOGGLE = "eth_toggle";
    //private static final String KEY_DEVICES_TITLE = "eth_device_title";
    //private static final String KEY_ETH_CONFIG = "eth_configure";
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
    private static final String DHCP_MODE = "dhcp";
    private static final String STATIC_MODE = "static";

    private Preference mMacPreference;
    private Preference mIpPreference;
    private Preference mGwPreference;
    private Preference mDns1Preference;
    private Preference mDns2Preference;
    private Preference mMaskPreference;
    private RadioButtonPreference mDhcpMode;
    private RadioButtonPreference mStaticMode;
    private PreferenceCategory mConnectMode;
    private PreferenceCategory mEthInfo;

    private EthernetManager mEthManager;
    private StaticModeDialog mStaticDialog;
    private DhcpModeDialog mDhcpDialog;
    private TextView mEmptyView;

    private String currentMode;
    private String prevMode;
    private final IntentFilter mFilter;
    private final BroadcastReceiver mEthStateReceiver;
    private boolean waitForConnectResult = false;
    private boolean waitForDisconnectResult = false;
    @Override
    public int getMetricsCategory() {
        return MetricsEvent.DISPLAY;
    }
       /*  New a BroadcastReceiver for EthernetSettings  */
    public EthernetSettings() {
        mFilter = new IntentFilter();
	mFilter.addAction(ConnectivityManager.CONNECTIVITY_ACTION);
        mEthStateReceiver = new BroadcastReceiver() {
            @Override
            public void onReceive(Context context, Intent intent) {
                handleEvent(context, intent);
            }
        };
    }
    @Override
    public void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
            addPreferencesFromResource(R.xml.ethernet_settings);
            mIpPreference = findPreference(KEY_IP_PRE);
            mMacPreference = findPreference(KEY_MAC_PRE);
            mGwPreference = findPreference(KEY_GW_PRE);
            mDns1Preference = findPreference(KEY_DNS1_PRE);
            mDns2Preference = findPreference(KEY_DNS2_PRE);
            mMaskPreference = findPreference(KEY_MASK_PRE);

            mConnectMode = (PreferenceCategory) findPreference(KEY_CONNECT_MODE);
            mEthInfo = (PreferenceCategory) findPreference(KEY_ETH_INFO);

            /* first, we must get the Service. */
            mConnectivityManager = (ConnectivityManager) getSystemService(Context.CONNECTIVITY_SERVICE);
            mEthManager = (EthernetManager)getSystemService(Context.ETHERNET_SERVICE);
            mIpConfiguration = new IpConfiguration();
            if(DBG) Slog.d(TAG, "onCreate.");

            /* Get the SaveConfig and update for Dialog. */
            mDhcpMode = (RadioButtonPreference) findPreference(KEY_DHCP_MODE);
            mDhcpMode.setOnClickListener(this);
            mStaticMode = (RadioButtonPreference) findPreference(KEY_STATIC_MODE);
            mStaticMode.setOnClickListener(this);
            IpConfiguration config = mEthManager.getConfiguration();
            if (config.getIpAssignment() == IpAssignment.STATIC) {
                prevMode = currentMode = STATIC_MODE;
            } else {
                prevMode = currentMode = DHCP_MODE;
            }
            updateRadioButtons(currentMode);
    }

    @Override
    public void onActivityCreated(Bundle savedInstanceState) {
        super.onActivityCreated(savedInstanceState);
        addAllPreference();
    }

    private void updateRadioButtons(String mode) {
        if (mode.equals(DHCP_MODE)) {
            mDhcpMode.setChecked(true);
            mStaticMode.setChecked(false);
        } else if (mode.equals(STATIC_MODE)) {
            mDhcpMode.setChecked(false);
            mStaticMode.setChecked(true);
        }
    }

    @Override
    public void onRadioButtonClicked(RadioButtonPreference emiter) {
        if (emiter == mDhcpMode) {
            currentMode = DHCP_MODE;
            if (!prevMode.equals(currentMode)) {
                prevMode = currentMode;
            }
            mIpConfiguration.setIpAssignment(IpAssignment.DHCP);
            processIpSettings();
            setConfiguration(mIpConfiguration);
        } else {
            currentMode = STATIC_MODE;
            mStaticDialog = new StaticModeDialog(getActivity(), this, null, false);
            mStaticDialog.setOnDismissListener(this);
            mStaticDialog.show();
        }
        updateRadioButtons(currentMode);
    }

    @Override
    public void onSaveInstanceState(Bundle savedState) {
    }

    @Override
    public void onStart() {
        super.onStart();
        final SettingsActivity activity = (SettingsActivity) getActivity();
    }

    @Override
    public void onStop() {
        super.onStop();
    }

    @Override
    public void onResume() {
        super.onResume();
        getActivity().registerReceiver(mEthStateReceiver, mFilter);
    }

    @Override
    public void onPause() {
        super.onPause();
    }

    @Override
    public void onDismiss(DialogInterface dialog) {
        // Here is the exit of a dialog.
        updateRadioButtons(prevMode);
    }

    private void displayToast(String str) {
        Toast.makeText((Context)getActivity(), str, Toast.LENGTH_SHORT).show();
    }
    @Override
    public void onClick(DialogInterface dialog, int whichbutton) {
        /* get the information form dialog. */
        IpConfiguration config = null;
        if (whichbutton == DialogInterface.BUTTON_POSITIVE) {
            Slog.d(TAG, "onClick: BUTTON_POSITIVE");
            if (currentMode.equals(STATIC_MODE)) {
	        mIpConfiguration.setIpAssignment(IpAssignment.STATIC);
                if (processIpSettings()) {
                    setConfiguration(mIpConfiguration);
                } else {
                    Log.w(TAG,"set static ip info error!");
                    return;
                }
            }

            if (!prevMode.equals(currentMode)) {
                prevMode = currentMode;
            }

        } else if (whichbutton == DialogInterface.BUTTON_NEGATIVE) {
            if (currentMode.equals(STATIC_MODE)) {
                try {
                    Field field = mStaticDialog.getClass().getSuperclass().
                    getSuperclass().getDeclaredField("mShowing");
                    field.setAccessible(true);
                    field.set(mStaticDialog, true);
                } catch (NoSuchFieldException ne) {
                } catch (IllegalAccessException ie) {
                }
            }
            Slog.d(TAG, "onClick: BUTTON_NEUTRAL");
        }
    }

    @Override
    public void onCreateContextMenu(ContextMenu menu, View view, ContextMenuInfo info) {
    }

    @Override
    public boolean onContextItemSelected(MenuItem item) {
        return false;
    }

    @Override
    public boolean onPreferenceChange(Preference preference, Object newValue){
        return false;
    }

    private boolean isEthConnected() {
        boolean ethConnected = false;
        ethConnected = mConnectivityManager != null &&
                mConnectivityManager.getNetworkInfo(ConnectivityManager.TYPE_ETHERNET).isConnected();
        return  ethConnected;
    }

    private void handleEvent(Context context, Intent intent) {
        String action = intent.getAction();
	if(ConnectivityManager.CONNECTIVITY_ACTION.equals(action)) {
	    if(isEthConnected()) {
	        Log.w(TAG,"ethernet connected");
                updateEthernetInfo(true);
            }
            if (!isEthConnected()) {
                Log.w(TAG,"ethernet disconnected");
                updateEthernetInfo(false);
            }
        }
    }

    private void addAllPreference() {
            getPreferenceScreen().addPreference(mMacPreference);
            getPreferenceScreen().addPreference(mIpPreference);
            getPreferenceScreen().addPreference(mGwPreference);
            getPreferenceScreen().addPreference(mDns1Preference);
            getPreferenceScreen().addPreference(mDns2Preference);
            getPreferenceScreen().addPreference(mMaskPreference);
            getPreferenceScreen().addPreference(mConnectMode);
            getPreferenceScreen().addPreference(mDhcpMode);
            getPreferenceScreen().addPreference(mStaticMode);
            getPreferenceScreen().addPreference(mEthInfo);
    }

    private void setConfiguration(IpConfiguration config) {
        if (currentMode != prevMode) {
            updateEthernetInfo(false);
        }
        Thread setConfigThread = new Thread(new Runnable() {
            public void run() {
                mEthManager.setConfiguration(config);
            }
        });
        setConfigThread.start();
    }

    private boolean processIpSettings() {
        Context context = getContext();
        if (mIpConfiguration.getIpAssignment() == IpAssignment.STATIC) {
            StaticIpConfiguration staticConfig = mStaticDialog.getStaticIpSettings();
            if (staticConfig != null ) {
                mIpConfiguration.setStaticIpConfiguration(staticConfig);
                return true;
            } else {
                String errTips = context.getString(R.string.eth_errStaticIp);
                displayToast(errTips);
                return false;
            }
        } else {
            mIpConfiguration.setStaticIpConfiguration(null);
            return true;
        }
    }

    private void updateEthernetInfo(boolean status) {
        int network = ConnectivityManager.TYPE_ETHERNET;
        if (status) {
            mMacPreference.setSummary(mConnectivityManager.getNetworkInfo(network).getExtraInfo());
            final LinkProperties linkProperties = mConnectivityManager.getLinkProperties(network);
            for (LinkAddress linkAddress : linkProperties.getLinkAddresses()) { //set ipaddress.
                if (linkAddress.getAddress() instanceof Inet4Address) {
                    mIpPreference.setSummary(linkAddress.getAddress().getHostAddress());
                    String netMask = String.valueOf(linkAddress.getPrefixLength());
                    mMaskPreference.setSummary(netMask);
                    break;
                }
            }
            String dns1 = null;
            String dns2 = null;
            int dnsCount = 0;
            for (InetAddress inetAddress:linkProperties.getDnsServers()) {
                if (0 == dnsCount) {
                    dns1 = inetAddress.getHostAddress().toString();
                    if (inetAddress instanceof Inet4Address) {
                        dns1 = inetAddress.getHostAddress().toString();
                    } else {
                        dns1 = "0.0.0.0";
                    }
                }
                if (1 == dnsCount) {
                    dns2 = inetAddress.getHostAddress().toString();
                    if (inetAddress instanceof Inet4Address) {
                        dns2 = inetAddress.getHostAddress().toString();
                    } else {
                        dns2 = "0.0.0.0";
                    }
                }
                dnsCount++;
            }
            mDns1Preference.setSummary(dns1);
            if (dnsCount >1) {
                mDns2Preference.setSummary(dns2);
            } else {
                mDns2Preference.setSummary("0.0.0.0");
            }
            String Route = null;
            for (RouteInfo route:linkProperties.getRoutes()) {
                if (route.isIPv4Default()) {
                    Route = route.getGateway().toString();
                    break;
                }
            }
            if (Route != null) {
                String[] RouteStr = Route.split("/");
                mGwPreference.setSummary(RouteStr[1]);
            } else {
                mGwPreference.setSummary("0.0.0.0");
            }
        } else {
            if (mIpPreference != null) {
                mIpPreference.setSummary("0.0.0.0");
                mGwPreference.setSummary("0.0.0.0");
                mDns1Preference.setSummary("0.0.0.0");
                mDns2Preference.setSummary("0.0.0.0");
                mMaskPreference.setSummary("0");
                mMacPreference.setSummary("00:00:00:00:00:00");
            }
        }
    }
}
