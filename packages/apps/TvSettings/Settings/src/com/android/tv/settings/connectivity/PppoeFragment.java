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

import android.content.Context;
import android.net.PppoeManager;
import android.os.Bundle;
import androidx.annotation.Keep;
import androidx.preference.Preference;
import androidx.preference.PreferenceCategory;
import androidx.preference.PreferenceManager;
import androidx.preference.TwoStatePreference;

import com.android.internal.logging.nano.MetricsProto;
import com.android.tv.settings.R;
import com.android.tv.settings.SettingsPreferenceFragment;
import com.android.tv.settings.PreferenceControllerFragment;
import com.android.tv.settings.connectivity.util.AdvancedOptionsFlowUtil;
import android.text.TextUtils;
import android.os.ServiceManager;
import android.app.Activity;
import android.util.Log;
import android.widget.Toast;
import android.content.Intent;
import android.content.IntentFilter;
import android.content.BroadcastReceiver;
import android.net.LinkProperties;
import android.net.LinkAddress;
import android.net.RouteInfo;

import java.util.Collection;
import java.util.HashSet;
import java.util.Set;
import java.net.Inet4Address;
import java.net.InetAddress;
import java.util.List;
import java.util.ArrayList;

/**
 * Fragment for controlling pppoe
 */
@Keep
public class PppoeFragment extends SettingsPreferenceFragment {
    private static final String KEY_PPPOE = "pppoe";
    private static final String KEY_PPPOE_SETUP = "pppoe_setup";
    private static final String KEY_PPPOE_ENABLE = "pppoe_enable";
    private static final String KEY_LOCAL_ADDRESS = "local_address";
    private static final String KEY_REMOTE_ADDRESS = "remote_address";
    private static final String KEY_PPPOE_DNS = "pppoe_dns";

    private static final String TAG = "PppoeFragment";
    public  static String getusername;
    public  static String getpassword;
    private IntentFilter mFilter;
    private BroadcastReceiver mEthStateReceiver;
    private PppoeManager mPppoeManager;

    private PreferenceCategory mPppoeCategory;
    private Preference mPppoeSetupPref;
    public static TwoStatePreference mEnablePppoePref;
    private Preference mLocalAddressPref;
    private Preference mRemoteAddressPref;
    private Preference mPppoeDnsPref;

    @Override
    public void onCreate(Bundle savedInstanceState) {
        mPppoeManager = (PppoeManager)getActivity().getSystemService(Context.PPPOE_SERVICE);
        String currentinterface = mPppoeManager.getPppoeInterfaceName();
        AdvancedOptionsFlowUtil.preinterface = currentinterface;
        AdvancedOptionsFlowUtil.pppoeinterface = currentinterface;
        List<String> login = mPppoeManager.getPppoeUserInfo(currentinterface);
        //List<String> login = new ArrayList<String>();
        if (login != null && (!login.isEmpty())) {
            getusername = login.get(0);
            AdvancedOptionsFlowUtil.pppoeusername = getusername;
            getpassword = login.get(1);
            AdvancedOptionsFlowUtil.pppoepassword = getpassword;
        } else {
            getusername = null;
            getpassword = null;
        }
        mFilter = new IntentFilter();
        mFilter.addAction(PppoeManager.PPPOE_STATE_CHANGED_ACTION);
        mEthStateReceiver = new BroadcastReceiver() {
            @Override
            public void onReceive(Context context, Intent intent) {
                handleEvent(context, intent);
            }
        };
        getActivity().registerReceiver(mEthStateReceiver, mFilter);
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
    }

    public void onResume() {
        super.onResume();
    }

    @Override
    public void onDestroy() {
        super.onDestroy();
        getActivity().unregisterReceiver(mEthStateReceiver);
    }
    @Override
    public void onCreatePreferences(Bundle savedInstanceState, String rootKey) {
        getPreferenceManager().setPreferenceComparisonCallback(
                new PreferenceManager.SimplePreferenceComparisonCallback());
        setPreferencesFromResource(R.xml.pppoe, null);

        mPppoeCategory = (PreferenceCategory) findPreference(KEY_PPPOE);
        mPppoeSetupPref = findPreference(KEY_PPPOE_SETUP);
        mPppoeSetupPref.setIntent(PppoeSetupActivity.createIntent(getContext()));
        mEnablePppoePref = (TwoStatePreference) findPreference(KEY_PPPOE_ENABLE);
        mLocalAddressPref = findPreference(KEY_LOCAL_ADDRESS);
        mRemoteAddressPref = findPreference(KEY_REMOTE_ADDRESS);
        mPppoeDnsPref = findPreference(KEY_PPPOE_DNS);
        if (!mPppoeManager.isPppoeEnabled()) {
            mEnablePppoePref.setChecked(false);
            mPppoeSetupPref.setEnabled(true);
        } else {
            mEnablePppoePref.setChecked(true);
            mPppoeSetupPref.setEnabled(false);
        }
    }

    @Override
    public boolean onPreferenceTreeClick(Preference preference) {
        if (preference.getKey() == null) {
            return super.onPreferenceTreeClick(preference);
        }
        switch (preference.getKey()) {
            case KEY_PPPOE_ENABLE:
                if (mEnablePppoePref.isChecked()) {
                    Log.w(TAG, "start connect pppoe");
                    if ((TextUtils.isEmpty(AdvancedOptionsFlowUtil.pppoeinterface))||
                        (TextUtils.isEmpty(AdvancedOptionsFlowUtil.pppoeusername))||
                        (TextUtils.isEmpty(AdvancedOptionsFlowUtil.pppoepassword))) {
                        String pppoeTips = getString(R.string.pppoe_tips);
                        Toast.makeText((Context)getActivity(), pppoeTips, Toast.LENGTH_SHORT).show();
                        mEnablePppoePref.setChecked(false);
                        return false;
                    }
                    mPppoeManager.setupPppoe(AdvancedOptionsFlowUtil.pppoeinterface,
                            AdvancedOptionsFlowUtil.pppoeusername,AdvancedOptionsFlowUtil.pppoepassword);
                    mEnablePppoePref.setEnabled(true);
                    mEnablePppoePref.setChecked(true);
                    mPppoeSetupPref.setEnabled(false);
                    Thread setConfigThread = new Thread(new Runnable() {
                        public void run() {
                            if (mPppoeManager.getPppoeState(AdvancedOptionsFlowUtil.pppoeinterface)
                                == PppoeManager.PPPOE_STATE_CONNECTED) {
                                mPppoeManager.disconnectPppoe(AdvancedOptionsFlowUtil.pppoeinterface);
                            } else {
                                if (mPppoeManager.getPppoeState(AdvancedOptionsFlowUtil.preinterface)
                                    != PppoeManager.PPPOE_STATE_DISCONNECTED) {
                                    mPppoeManager.disconnectPppoe(AdvancedOptionsFlowUtil.preinterface);
                                }
                                AdvancedOptionsFlowUtil.preinterface = AdvancedOptionsFlowUtil.pppoeinterface;
                                mPppoeManager.connectPppoe(AdvancedOptionsFlowUtil.pppoeinterface);
                            }
                         }
                    });
                    setConfigThread.start();
                } else {
                    Log.w(TAG, "disconnect pppoe");
                    mEnablePppoePref.setChecked(false);
                    mPppoeSetupPref.setEnabled(true);
                    mPppoeManager.disconnectPppoe(AdvancedOptionsFlowUtil.pppoeinterface);
                }
                return true;
        }
        return super.onPreferenceTreeClick(preference);
    }

    private void handleEvent(Context context, Intent intent) {
        String action = intent.getAction();
        if (PppoeManager.PPPOE_STATE_CHANGED_ACTION.equals(action)) {
            final int event = intent.getIntExtra(PppoeManager.EXTRA_PPPOE_STATE,
                     PppoeManager.PPPOE_EVENT_CONNECT_SUCCESSED);
            switch(event) {
                case PppoeManager.PPPOE_EVENT_CONNECT_SUCCESSED :
                    Log.w(TAG, "received pppoe connnected message!");
                    final LinkProperties linkProperties = mPppoeManager.getPppoelinkProperties();
                    for (LinkAddress linkAddress : linkProperties.getLinkAddresses()) { //set ipaddress.
                        if (linkAddress.getAddress() instanceof Inet4Address) {
                            mLocalAddressPref.setSummary(linkAddress.getAddress().getHostAddress());
                            break;
                        }
                    }
                    String Route = null;
                    for (RouteInfo route:linkProperties.getRoutes()){
                        if (route.hasGateway() && route.isDefaultRoute()
                            && route.getDestination().getAddress() instanceof Inet4Address){
                            Route = route.getGateway().toString();
                            break;
                        }
                    }
                    if (Route != null) {
                        String[] RouteStr = Route.split("/");
                        mRemoteAddressPref.setSummary(RouteStr[1]);
                    } else {
                        mRemoteAddressPref.setSummary("0.0.0.0");
                    }
                    String dns = null;
                    int dnsCount = 0;
                    for (InetAddress inetAddress:linkProperties.getDnsServers()) {
                        if (0 == dnsCount) {
                            if (inetAddress instanceof Inet4Address) {
                                dns = inetAddress.getHostAddress().toString();
                                break;
                            }
                        }
                        if (1 == dnsCount) {
                            if (inetAddress instanceof Inet4Address) {
                                dns = inetAddress.getHostAddress().toString();
                                break;
                            }
                        }
                            dnsCount++;
                        }
                    if (dns!=null) {
                        mPppoeDnsPref.setSummary(dns);
                    } else {
                        mPppoeDnsPref.setSummary("0.0.0.0");
                    }

                    break;
                case PppoeManager.PPPOE_EVENT_DISCONNECT_SUCCESSED :
                case PppoeManager.PPPOE_EVENT_CONNECT_FAILED:
                    mLocalAddressPref.setSummary("0.0.0.0");
                    mRemoteAddressPref.setSummary("0.0.0.0");
                    mPppoeDnsPref.setSummary("0.0.0.0");
                    if ((event == PppoeManager.PPPOE_EVENT_CONNECT_FAILED) && (mPppoeManager.isPppoeEnabled())) {
                        String errorReason = intent.getStringExtra(PppoeManager.EXTRA_PPPOE_ERRMSG);
                        String errmsg;
                        int ErrorReason = Integer.parseInt(errorReason);
                        String[] errorMsg = context.getResources().getStringArray(R.array.errorMsg);
                        if (context.getResources().getStringArray(R.array.errorMsg) != null) {
                            errmsg = context.getString(R.string.pppoe_errmsg) + errorMsg[ErrorReason];
                        } else {
                            errmsg = context.getString(R.string.pppoe_errmsg) + "unknow reason";
                        }
                        Toast.makeText((Context)getActivity(), errmsg, Toast.LENGTH_SHORT).show();
                    }
                    break;
                case PppoeManager.PPPOE_EVENT_DISCONNECTING:
                    break;
                default:
                    break;
            }
        }
    }

    @Deprecated
    public int getMetricsCategory() {
        return MetricsProto.MetricsEvent.WIFI_TETHER_SETTINGS;
    }
}
