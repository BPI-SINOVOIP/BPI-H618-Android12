/*
 * Copyright (C) 2013 The Android Open Source Project
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
import android.net.ConnectivityManager;
import android.net.NetworkInfo;
import android.net.EthernetManager;
import android.net.InetAddresses;
import android.net.IpConfiguration;
import android.net.IpConfiguration.IpAssignment;
import android.net.LinkAddress;
import android.net.LinkProperties;
import android.net.Network;
import android.net.StaticIpConfiguration;
import android.text.TextUtils;
import android.util.Log;

import java.net.Inet4Address;
import java.net.InetAddress;
import java.util.ArrayList;

public class EthernetSettingsManager {
    private final static String TAG = "EthernetSettingsManager";
    private Context mContext = null;

    private ConnectivityManager mConnectivityManager;
    private IpConfiguration mIpConfiguration;
    private EthernetManager mEthManager;

    EthernetSettingsManager(Context context) {
        mContext = context;
        mConnectivityManager = (ConnectivityManager) mContext.getSystemService(Context.CONNECTIVITY_SERVICE);
        mEthManager = (EthernetManager) mContext.getSystemService(Context.ETHERNET_SERVICE);
        mIpConfiguration = mEthManager.getConfiguration("eth0");
    }

    public IpConfiguration getIpConfiguration() {
        return mIpConfiguration;
    }

    public LinkProperties getLinkProperties(Network network) {
        return mConnectivityManager.getLinkProperties(network);
    }

    public void setIpAssignment(boolean useStaticIp) {
        mIpConfiguration.setIpAssignment(useStaticIp ? IpAssignment.STATIC : IpAssignment.DHCP);
    }

    private boolean isEthConnected() {
        return  mConnectivityManager != null &&
                mConnectivityManager.getNetworkInfo(ConnectivityManager.TYPE_ETHERNET).isConnected();
    }

    public void processIpSettings(EthernetConfig ethernetConfig) {
        if (mIpConfiguration.getIpAssignment() == IpAssignment.STATIC) {
            StaticIpConfiguration staticConfig = getStaticIpSettings(ethernetConfig);
            mIpConfiguration.setStaticIpConfiguration(staticConfig);
        } else {
            mIpConfiguration.setStaticIpConfiguration(null);
        }
    }

    public void setConfiguration() {
        mEthManager.setConfiguration("eth0", mIpConfiguration);
    }

    public Network getFirstNetwork() {
        final Network[] networks = mConnectivityManager.getAllNetworks();
        for (final Network network : networks) {
            NetworkInfo networkInfo = mConnectivityManager.getNetworkInfo(network);
            Log.w(TAG,"getFirstNetwork, networkInfo:" + networkInfo);
            if (networkInfo != null
                    && (networkInfo.getType() == ConnectivityManager.TYPE_ETHERNET)) {
                Log.w(TAG,"getFirstNetwork, type is ethernet:" + network);
                return network;
            }
        }
        return null;
    }

    public StaticIpConfiguration getStaticIpSettings(EthernetConfig ethernetConfig) {
        StaticIpConfiguration.Builder staticConfig = new StaticIpConfiguration.Builder();
        String ipAddr = ethernetConfig.getIp();
        if (TextUtils.isEmpty(ipAddr)) return null;

        Inet4Address inetAddr = null;
        try {
            inetAddr = (Inet4Address) InetAddresses.parseNumericAddress(ipAddr);
        } catch (IllegalArgumentException | ClassCastException e) {
            return null;
        }

        int networkPrefixLength = -1;
        try {
            networkPrefixLength = Integer.parseInt(ethernetConfig.getMask());
            if (networkPrefixLength < 0 || networkPrefixLength > 32) {
                return null;
            }
            staticConfig.setIpAddress(new LinkAddress(inetAddr, networkPrefixLength));
        } catch (NumberFormatException e) {
            return null;
        }

        String gateway = ethernetConfig.getGw();
        String dns1 = ethernetConfig.getDns1();
        String dns2 = ethernetConfig.getDns2();
        if (!TextUtils.isEmpty(gateway)
        || !TextUtils.isEmpty(dns1)
        || !TextUtils.isEmpty(dns2)) {
        try {
            ArrayList<InetAddress> dnsServers = new ArrayList<>();
            dnsServers.add(
                    (Inet4Address) InetAddresses.parseNumericAddress(dns1));
            dnsServers.add(
                    (Inet4Address) InetAddresses.parseNumericAddress(dns2));
            staticConfig.setGateway((Inet4Address) InetAddresses.parseNumericAddress(gateway))
                    .setDnsServers(dnsServers);

            } catch (IllegalArgumentException | ClassCastException e) {
                return null;
            }
        }
        return staticConfig.build();
	}

    public static String getMaskFromIp(String ip) {
        String ary0 = ip.substring(0, ip.indexOf("."));
        Integer itg = Integer.valueOf(ary0);
        int i = itg.intValue();
        if (i < 128 && i > 0) {
            return "255.0.0.0";
        } else if (i < 192) {
            return "255.255.0.0";
        } else if (i < 224) {
            return "255.255.255.0";
        } else {
            return "255.255.255.255";
        }
    }

}
