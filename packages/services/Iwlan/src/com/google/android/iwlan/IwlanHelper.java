/*
 * Copyright 2020 The Android Open Source Project
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

package com.google.android.iwlan;

import android.content.Context;
import android.net.ConnectivityManager;
import android.net.LinkAddress;
import android.net.LinkProperties;
import android.net.Network;
import android.os.PersistableBundle;
import android.telephony.CarrierConfigManager;
import android.telephony.SubscriptionInfo;
import android.telephony.SubscriptionManager;
import android.telephony.TelephonyManager;
import android.telephony.ims.ImsManager;
import android.telephony.ims.ImsMmTelManager;

import java.net.Inet4Address;
import java.net.Inet6Address;
import java.net.InetAddress;
import java.util.ArrayList;
import java.util.List;

public class IwlanHelper {

    private static final String TAG = IwlanHelper.class.getSimpleName();

    public static String getNai(Context context, int slotId) {
        StringBuilder naiBuilder = new StringBuilder();
        TelephonyManager tm = context.getSystemService(TelephonyManager.class);
        SubscriptionInfo subInfo = null;
        tm = tm.createForSubscriptionId(getSubId(context, slotId));

        try {
            subInfo = getSubInfo(context, slotId);
        } catch (IllegalStateException e) {
            return null;
        }

        String mnc = subInfo.getMncString();
        mnc = (mnc.length() == 2) ? '0' + mnc : mnc;

        naiBuilder.append('0').append(tm.getSubscriberId()).append('@');

        return naiBuilder
                .append("nai.epc.mnc")
                .append(mnc)
                .append(".mcc")
                .append(subInfo.getMccString())
                .append(".3gppnetwork.org")
                .toString();
    }

    public static int getSubId(Context context, int slotId) {
        int subid = SubscriptionManager.INVALID_SUBSCRIPTION_ID;

        try {
            subid = getSubInfo(context, slotId).getSubscriptionId();
        } catch (IllegalStateException e) {
            subid = SubscriptionManager.INVALID_SUBSCRIPTION_ID;
        }
        return subid;
    }

    public static int getCarrierId(Context context, int slotId) {
        TelephonyManager tm = context.getSystemService(TelephonyManager.class);
        tm = tm.createForSubscriptionId(IwlanHelper.getSubId(context, slotId));
        return tm.getSimCarrierId();
    }

    private static SubscriptionInfo getSubInfo(Context context, int slotId)
            throws IllegalStateException {
        SubscriptionManager sm = context.getSystemService(SubscriptionManager.class);
        SubscriptionInfo info = sm.getActiveSubscriptionInfoForSimSlotIndex(slotId);

        if (info == null) {
            throw new IllegalStateException("Subscription info is null.");
        }

        return info;
    }

    public static List<InetAddress> getAddressesForNetwork(Network network, Context context) {
        ConnectivityManager connectivityManager =
                context.getSystemService(ConnectivityManager.class);
        List<InetAddress> gatewayList = new ArrayList<InetAddress>();
        if (network != null) {
            LinkProperties linkProperties = connectivityManager.getLinkProperties(network);
            if (linkProperties != null) {
                for (LinkAddress laddr : linkProperties.getLinkAddresses()) {
                    InetAddress inetaddr = laddr.getAddress();
                    // skip linklocal and loopback addresses
                    if (!inetaddr.isLoopbackAddress() && !inetaddr.isLinkLocalAddress()) {
                        gatewayList.add(inetaddr);
                    }
                }
            }
        }
        return gatewayList;
    }

    public static boolean hasIpv6Address(List<InetAddress> localAddresses) {
        for (InetAddress address : localAddresses) {
            if (address instanceof Inet6Address) {
                return true;
            }
        }

        return false;
    }

    public static boolean hasIpv4Address(List<InetAddress> localAddresses) {
        for (InetAddress address : localAddresses) {
            if (address instanceof Inet4Address) {
                return true;
            }
        }

        return false;
    }

    public static InetAddress getIpv4Address(List<InetAddress> localAddresses) {
        for (InetAddress address : localAddresses) {
            if (address instanceof Inet4Address) {
                return address;
            }
        }

        throw new IllegalStateException("Local address should not be null.");
    }

    public static InetAddress getIpv6Address(List<InetAddress> localAddresses) {
        for (InetAddress address : localAddresses) {
            if (address instanceof Inet6Address) {
                return address;
            }
        }

        throw new IllegalStateException("Local address should not be null.");
    }

    public static <T> T getConfig(String key, Context context, int slotId) {
        CarrierConfigManager carrierConfigManager =
                context.getSystemService(CarrierConfigManager.class);
        if (carrierConfigManager == null) {
            throw new IllegalStateException("Carrier config manager is null.");
        }

        PersistableBundle bundle =
                carrierConfigManager.getConfigForSubId(getSubId(context, slotId));

        if (bundle == null || bundle.get(key) == null) {
            return getDefaultConfig(key);
        } else {
            return (T) bundle.get(key);
        }
    }

    public static <T> T getDefaultConfig(String key) {
        PersistableBundle bundle = CarrierConfigManager.getDefaultConfig();
        if (bundle == null) {
            throw new IllegalStateException("Default config is null for: " + key);
        }
        return (T) bundle.get(key);
    }

    public static boolean isDefaultDataSlot(Context context, int slotId) {
        SubscriptionManager sm = context.getSystemService(SubscriptionManager.class);
        int ddsSlotId = sm.getSlotIndex(sm.getDefaultDataSubscriptionId());
        if (ddsSlotId != sm.INVALID_SIM_SLOT_INDEX) {
            if (ddsSlotId == slotId) {
                return true;
            }
        }
        return false;
    }

    public static boolean isCrossSimCallingEnabled(Context context, int slotId) {
        boolean isCstEnabled = false;
        int subid = getSubId(context, slotId);

        if (subid == SubscriptionManager.INVALID_SUBSCRIPTION_ID) {
            // Fail to query subscription id, just return false.
            return false;
        }

        ImsManager imsManager = context.getSystemService(ImsManager.class);
        if (imsManager != null) {
            ImsMmTelManager imsMmTelManager = imsManager.getImsMmTelManager(subid);
            if (imsMmTelManager != null) {
                try {
                    isCstEnabled = imsMmTelManager.isCrossSimCallingEnabled();
                } catch (Exception e) {
                    // Fail to query Cross-SIM calling setting, just return false to avoid an
                    // exception.
                }
            }
        }
        return isCstEnabled;
    }
}
