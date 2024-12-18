/*
 * Copyright (C) 2017 The Android Open Source Project
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

package com.android.tv.settings.connectivity.util;

import android.net.InetAddresses;
import android.net.IpConfiguration;
import android.net.LinkAddress;
import android.net.ProxyInfo;
import android.net.PppoeManager;
import android.content.Context;
import android.util.Log;
import android.net.StaticIpConfiguration;
import android.text.TextUtils;

import androidx.fragment.app.FragmentActivity;
import androidx.lifecycle.ViewModelProviders;

import com.android.net.module.util.ProxyUtils;
import com.android.tv.settings.R;
import com.android.tv.settings.connectivity.PppoeFragment;
import com.android.tv.settings.connectivity.WifiConfigHelper;
import com.android.tv.settings.connectivity.setup.AdvancedOptionsFlowInfo;

import java.net.Inet4Address;
import java.net.InetAddress;
import java.util.ArrayList;

/**
 * Class that helps process proxy and IP settings.
 */
public class AdvancedOptionsFlowUtil {
    public static String pppoeinterface;
    public static String pppoeusername;
    public static String pppoepassword;
    public static String preinterface;
    private static PppoeManager mPppoeManager;
    private static final String TAG = "AdvancedOptionsFlowUtil";

    /**
     * Process Proxy Settings.
     *
     * @param activity the activity that calls this method.
     * @return 0 if successful, or other different values based on the result.
     */
    public static int processProxySettings(FragmentActivity activity) {
        AdvancedOptionsFlowInfo flowInfo = ViewModelProviders
                .of(activity)
                .get(AdvancedOptionsFlowInfo.class);
        IpConfiguration mIpConfiguration = flowInfo.getIpConfiguration();
        boolean hasProxySettings =
                (!flowInfo.containsPage(AdvancedOptionsFlowInfo.ADVANCED_OPTIONS)
                        || !flowInfo.choiceChosen(
                                activity.getString(R.string.wifi_action_advanced_no),
                                AdvancedOptionsFlowInfo.ADVANCED_OPTIONS))
                && !flowInfo.choiceChosen(activity.getString(R.string.wifi_action_proxy_none),
                                            AdvancedOptionsFlowInfo.PROXY_SETTINGS);
        mIpConfiguration.setProxySettings(hasProxySettings
                ? IpConfiguration.ProxySettings.STATIC : IpConfiguration.ProxySettings.NONE);
        if (hasProxySettings) {
            String host = flowInfo.get(AdvancedOptionsFlowInfo.PROXY_HOSTNAME);
            String portStr = flowInfo.get(AdvancedOptionsFlowInfo.PROXY_PORT);
            String exclusionList = flowInfo.get(AdvancedOptionsFlowInfo.PROXY_BYPASS);
            int port = 0;
            int result;
            try {
                port = Integer.parseInt(portStr);
                result = WifiConfigHelper.validate(host, portStr, exclusionList);
            } catch (NumberFormatException e) {
                result = R.string.proxy_error_invalid_port;
            }
            if (result == 0) {
                mIpConfiguration.setHttpProxy(ProxyInfo.buildDirectProxy(host, port,
                        ProxyUtils.exclusionStringAsList(exclusionList)));
            } else {
                return result;
            }
        } else {
            mIpConfiguration.setHttpProxy(null);
        }
        return 0;
    }

    /**
     * Process Ip Settings.
     *
     * @param activity the activity that calls this method.
     * @return 0 if successful, or other different values based on the result.
     */
    public static int processIpSettings(FragmentActivity activity) {
        AdvancedOptionsFlowInfo flowInfo = ViewModelProviders
                .of(activity)
                .get(AdvancedOptionsFlowInfo.class);
        IpConfiguration mIpConfiguration = flowInfo.getIpConfiguration();
        boolean hasIpSettings =
                (!flowInfo.containsPage(AdvancedOptionsFlowInfo.ADVANCED_OPTIONS)
                        || !flowInfo.choiceChosen(
                                activity.getString(R.string.wifi_action_advanced_no),
                                AdvancedOptionsFlowInfo.ADVANCED_OPTIONS))
                && !flowInfo.choiceChosen(activity.getString(R.string.wifi_action_dhcp),
                                            AdvancedOptionsFlowInfo.IP_SETTINGS);
        mIpConfiguration.setIpAssignment(hasIpSettings
                        ? IpConfiguration.IpAssignment.STATIC
                        : IpConfiguration.IpAssignment.DHCP);
        if (hasIpSettings) {
            final StaticIpConfiguration.Builder staticIpBuilder =
                    new StaticIpConfiguration.Builder();
            String ipAddr = flowInfo.get(AdvancedOptionsFlowInfo.IP_ADDRESS);
            if (TextUtils.isEmpty(ipAddr)) {
                return R.string.wifi_ip_settings_invalid_ip_address;
            }
            Inet4Address inetAddr;
            try {
                inetAddr = (Inet4Address) InetAddresses.parseNumericAddress(ipAddr);
            } catch (IllegalArgumentException | ClassCastException e) {
                return R.string.wifi_ip_settings_invalid_ip_address;
            }

            try {
                int networkPrefixLength = Integer.parseInt(flowInfo.get(
                        AdvancedOptionsFlowInfo.NETWORK_PREFIX_LENGTH));
                if (networkPrefixLength < 0 || networkPrefixLength > 32) {
                    return R.string.wifi_ip_settings_invalid_network_prefix_length;
                }
                staticIpBuilder.setIpAddress(new LinkAddress(inetAddr, networkPrefixLength));
            } catch (NumberFormatException e) {
                return R.string.wifi_ip_settings_invalid_ip_address;
            }

            String gateway = flowInfo.get(AdvancedOptionsFlowInfo.GATEWAY);
            if (!TextUtils.isEmpty(gateway)) {
                try {
                    staticIpBuilder.setGateway(InetAddresses.parseNumericAddress(gateway));
                } catch (IllegalArgumentException | ClassCastException e) {
                    return R.string.wifi_ip_settings_invalid_gateway;
                }
            }

            final ArrayList<InetAddress> dnsServers = new ArrayList<>();
            String dns1 = flowInfo.get(AdvancedOptionsFlowInfo.DNS1);
            if (!TextUtils.isEmpty(dns1)) {
                try {
                    dnsServers.add(InetAddresses.parseNumericAddress(dns1));
                } catch (IllegalArgumentException | ClassCastException e) {
                    return R.string.wifi_ip_settings_invalid_dns;
                }
            }

            String dns2 = flowInfo.get(AdvancedOptionsFlowInfo.DNS2);
            if (!TextUtils.isEmpty(dns2)) {
                try {
                    dnsServers.add(InetAddresses.parseNumericAddress(dns2));
                } catch (IllegalArgumentException | ClassCastException e) {
                    return R.string.wifi_ip_settings_invalid_dns;
                }
            }

            staticIpBuilder.setDnsServers(dnsServers);
            mIpConfiguration.setStaticIpConfiguration(staticIpBuilder.build());
        } else {
            mIpConfiguration.setStaticIpConfiguration(null);
        }
        return 0;
    }

    /**
     * Process PPPoE Setup.
     *
     * @param activity the activity that calls this method.
     * @return 0 if successful, or other different values based on the result.
     */
    public static int processPppoeSetup(FragmentActivity activity) {
        AdvancedOptionsFlowInfo flowInfo = ViewModelProviders
                .of(activity)
                .get(AdvancedOptionsFlowInfo.class);
        mPppoeManager = (PppoeManager)activity.getSystemService(Context.PPPOE_SERVICE);
        String getinterface  = flowInfo.get(AdvancedOptionsFlowInfo.PPPOE_INTERFACE);
        if (getinterface == null || TextUtils.isEmpty(getinterface)) {
            return -1;
        }
        String username = flowInfo.get(AdvancedOptionsFlowInfo.PPPOE_USERNAME);
        if (username == null || TextUtils.isEmpty(username)) {
            return -1;
        }
        String password = flowInfo.get(AdvancedOptionsFlowInfo.PPPOE_PASSWORD);
        if (password == null || TextUtils.isEmpty(password)) {
            return -1;
        }
        pppoeinterface = getinterface;
        pppoeusername = username;
        pppoepassword = password;
        Log.w(TAG, "interface is : " + pppoeinterface);
        Log.w(TAG, "preinterface is : " + preinterface);
        Log.w(TAG, "username  is : " + pppoeusername);
        if (!pppoeinterface.equals(preinterface)) {
            if (mPppoeManager.isPppoeEnabled()) {
                Log.d(TAG, "Saved,but pppoe already enabled");
                Thread disConnectThread = new Thread(new Runnable() {
                    public void run() {
                        if (mPppoeManager.getPppoeState(preinterface)
                                != PppoeManager.PPPOE_STATE_DISCONNECTED) {
                            Log.d(TAG, "Saved,disconnectPppoe");
                            mPppoeManager.disconnectPppoe(preinterface);
                        }
                    }
                });
                disConnectThread.start();
                PppoeFragment.mEnablePppoePref.setChecked(false);
            }
        }
        return 0;
    }

    public static int processProxyNone(FragmentActivity activity) {
        AdvancedOptionsFlowInfo flowInfo = ViewModelProviders
                .of(activity)
                .get(AdvancedOptionsFlowInfo.class);
        IpConfiguration mIpConfiguration = flowInfo.getIpConfiguration();
        boolean hasProxySettings =
                (!flowInfo.containsPage(AdvancedOptionsFlowInfo.ADVANCED_OPTIONS)
                        || !flowInfo.choiceChosen(
                                activity.getString(R.string.wifi_action_advanced_no),
                                AdvancedOptionsFlowInfo.ADVANCED_OPTIONS))
                && !flowInfo.choiceChosen(activity.getString(R.string.wifi_action_proxy_none),
                                            AdvancedOptionsFlowInfo.PROXY_SETTINGS);
        if (hasProxySettings) {
            mIpConfiguration.setProxySettings(IpConfiguration.ProxySettings.NONE);
            mIpConfiguration.setHttpProxy(null);
        }
        return 0;
    }
}
