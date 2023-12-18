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

package com.google.android.iwlan.epdg;

import android.content.Context;
import android.net.Network;
import android.support.annotation.IntDef;
import android.support.annotation.NonNull;
import android.telephony.CarrierConfigManager;
import android.telephony.CellIdentityGsm;
import android.telephony.CellIdentityLte;
import android.telephony.CellIdentityNr;
import android.telephony.CellIdentityWcdma;
import android.telephony.CellInfo;
import android.telephony.CellInfoGsm;
import android.telephony.CellInfoLte;
import android.telephony.CellInfoNr;
import android.telephony.CellInfoWcdma;
import android.telephony.SubscriptionInfo;
import android.telephony.SubscriptionManager;
import android.telephony.TelephonyManager;
import android.util.Log;

import com.android.internal.annotations.VisibleForTesting;

import com.google.android.iwlan.IwlanError;
import com.google.android.iwlan.IwlanHelper;

import java.net.Inet4Address;
import java.net.Inet6Address;
import java.net.InetAddress;
import java.util.*;
import java.util.concurrent.ConcurrentHashMap;

public class EpdgSelector {
    private static final String TAG = "EpdgSelector";
    private Context mContext;
    private int mSlotId;
    private static ConcurrentHashMap<Integer, EpdgSelector> mSelectorInstances =
            new ConcurrentHashMap<>();
    private int mV4PcoId = -1;
    private int mV6PcoId = -1;
    private byte[] mV4PcoData = null;
    private byte[] mV6PcoData = null;

    final Comparator<InetAddress> inetAddressComparator =
            new Comparator<InetAddress>() {
                @Override
                public int compare(InetAddress ip1, InetAddress ip2) {
                    if ((ip1 instanceof Inet4Address) && (ip2 instanceof Inet6Address)) {
                        return -1;
                    } else if ((ip1 instanceof Inet6Address) && (ip2 instanceof Inet4Address)) {
                        return 1;
                    } else {
                        return 0;
                    }
                }
            };

    public static final int PROTO_FILTER_IPV4 = 0;
    public static final int PROTO_FILTER_IPV6 = 1;
    public static final int PROTO_FILTER_IPV4V6 = 2;

    @IntDef({PROTO_FILTER_IPV4, PROTO_FILTER_IPV6, PROTO_FILTER_IPV4V6})
    @interface ProtoFilter {}

    public interface EpdgSelectorCallback {
        /*gives priority ordered list of addresses*/
        void onServerListChanged(int transactionId, ArrayList<InetAddress> validIPList);

        void onError(int transactionId, IwlanError error);
    }

    @VisibleForTesting
    EpdgSelector(Context context, int slotId) {
        mContext = context;
        mSlotId = slotId;
    }

    public static EpdgSelector getSelectorInstance(Context context, int slotId) {
        mSelectorInstances.computeIfAbsent(slotId, k -> new EpdgSelector(context, slotId));
        return mSelectorInstances.get(slotId);
    }

    public boolean setPcoData(int pcoId, byte[] pcoData) {
        Log.d(TAG, "onReceive PcoId:" + String.format("0x%04x", pcoId) + " PcoData:" + pcoData);

        int PCO_ID_IPV6 =
                IwlanHelper.getConfig(
                        CarrierConfigManager.Iwlan.KEY_EPDG_PCO_ID_IPV6_INT, mContext, mSlotId);
        int PCO_ID_IPV4 =
                IwlanHelper.getConfig(
                        CarrierConfigManager.Iwlan.KEY_EPDG_PCO_ID_IPV4_INT, mContext, mSlotId);

        Log.d(
                TAG,
                "PCO_ID_IPV6:"
                        + String.format("0x%04x", PCO_ID_IPV6)
                        + " PCO_ID_IPV4:"
                        + String.format("0x%04x", PCO_ID_IPV4));

        if (pcoId == PCO_ID_IPV4) {
            mV4PcoId = pcoId;
            mV4PcoData = pcoData;
            return true;
        } else if (pcoId == PCO_ID_IPV6) {
            mV6PcoId = pcoId;
            mV6PcoData = pcoData;
            return true;
        }

        return false;
    }

    public void clearPcoData() {
        Log.d(TAG, "Clear PCO data");
        mV4PcoId = -1;
        mV6PcoId = -1;
        mV4PcoData = null;
        mV6PcoData = null;
    }

    private void getIP(
            String domainName, int filter, ArrayList<InetAddress> validIpList, Network network) {
        InetAddress[] ipList;

        // Get All IP for each domain name
        Log.d(TAG, "Input domainName : " + domainName);
        try {
            ipList = network.getAllByName(domainName);
        } catch (Exception e) {
            Log.e(TAG, "Exception when querying IP address : " + e);
            return;
        }

        if (ipList == null) {
            Log.e(TAG, "Get empty IP address list");
            return;
        }

        // Filter the IP list by input ProtoFilter
        for (InetAddress ipAddress : ipList) {
            switch (filter) {
                case PROTO_FILTER_IPV4:
                    if (ipAddress instanceof Inet4Address) {
                        validIpList.add(ipAddress);
                    }
                    break;
                case PROTO_FILTER_IPV6:
                    if (ipAddress instanceof Inet6Address) {
                        validIpList.add(ipAddress);
                    }
                    break;
                case PROTO_FILTER_IPV4V6:
                    validIpList.add(ipAddress);
                    break;
                default:
                    Log.d(TAG, "Invalid ProtoFilter : " + filter);
            }
        }
    }

    private String[] getPlmnList() {
        List<String> plmnsFromSubInfo = new ArrayList<>();

        List<String> plmnsFromCarrierConfig =
                new ArrayList<>(
                        Arrays.asList(
                                IwlanHelper.getConfig(
                                        CarrierConfigManager.Iwlan.KEY_MCC_MNCS_STRING_ARRAY,
                                        mContext,
                                        mSlotId)));
        Log.d(TAG, "plmnsFromCarrierConfig:" + plmnsFromCarrierConfig);

        // Get Ehplmns & mccmnc from SubscriptionManager
        SubscriptionManager subscriptionManager =
                mContext.getSystemService(SubscriptionManager.class);
        if (subscriptionManager == null) {
            Log.e(TAG, "SubscriptionManager is NULL");
            return plmnsFromCarrierConfig.toArray(new String[plmnsFromCarrierConfig.size()]);
        }

        SubscriptionInfo subInfo =
                subscriptionManager.getActiveSubscriptionInfoForSimSlotIndex(mSlotId);
        if (subInfo == null) {
            Log.e(TAG, "SubscriptionInfo is NULL");
            return plmnsFromCarrierConfig.toArray(new String[plmnsFromCarrierConfig.size()]);
        }

        // There are three sources of plmns - sim plmn, plmn list from carrier config and
        // Ehplmn list from subscription info.
        // The plmns are prioritized as follows:
        // 1. Sim plmn
        // 2. Plmns common to both lists.
        // 3. Remaining plmns in the lists.
        List<String> combinedList = new ArrayList<>();
        // Get MCCMNC from IMSI
        String plmnFromImsi =
                new StringBuilder()
                        .append(subInfo.getMccString())
                        .append("-")
                        .append(subInfo.getMncString())
                        .toString();
        combinedList.add(plmnFromImsi);

        // Get Ehplmns from TelephonyManager
        for (String ehplmn : getEhplmns()) {
            if (ehplmn.length() == 5 || ehplmn.length() == 6) {
                StringBuilder str = new StringBuilder(ehplmn);
                str.insert(3, "-");
                plmnsFromSubInfo.add(str.toString());
            }
        }

        Log.d(TAG, "plmnsFromSubInfo:" + plmnsFromSubInfo);

        // To avoid double adding plmn from imsi
        plmnsFromCarrierConfig.removeIf(i -> i.equals(plmnFromImsi));
        plmnsFromSubInfo.removeIf(i -> i.equals(plmnFromImsi));

        for (Iterator<String> iterator = plmnsFromCarrierConfig.iterator(); iterator.hasNext(); ) {
            String plmn = iterator.next();
            if (plmnsFromSubInfo.contains(plmn)) {
                combinedList.add(plmn);
                plmnsFromSubInfo.remove(plmn);
                iterator.remove();
            }
        }

        combinedList.addAll(plmnsFromSubInfo);
        combinedList.addAll(plmnsFromCarrierConfig);

        Log.d(TAG, "Final plmn list:" + combinedList);
        return combinedList.toArray(new String[combinedList.size()]);
    }

    private ArrayList<InetAddress> removeDuplicateIp(ArrayList<InetAddress> validIpList) {
        ArrayList<InetAddress> resultIpList = new ArrayList<InetAddress>();

        for (Iterator<InetAddress> iterator = validIpList.iterator(); iterator.hasNext(); ) {
            InetAddress validIp = iterator.next();

            if (!resultIpList.contains(validIp)) {
                resultIpList.add(validIp);
            }
        }

        return resultIpList;
    }

    private String[] splitMccMnc(String plmn) {
        String[] mccmnc = plmn.split("-");
        mccmnc[1] = String.format("%03d", Integer.parseInt(mccmnc[1]));
        return mccmnc;
    }

    private List<String> getEhplmns() {
        TelephonyManager mTelephonyManager = mContext.getSystemService(TelephonyManager.class);
        mTelephonyManager =
                mTelephonyManager.createForSubscriptionId(IwlanHelper.getSubId(mContext, mSlotId));

        if (mTelephonyManager == null) {
            Log.e(TAG, "TelephonyManager is NULL");
            return new ArrayList<String>();
        } else {
            return mTelephonyManager.getEquivalentHomePlmns();
        }
    }

    private void resolutionMethodStatic(
            int filter, ArrayList<InetAddress> validIpList, boolean isRoaming, Network network) {
        String[] domainNames = null;

        Log.d(TAG, "STATIC Method");

        // Get the static domain names from carrier config
        // Config obtained in form of a list of domain names separated by
        // a delimeter is only used for testing purpose.
        // TODO: need to consider APM on/no cellular condition.
        if (isRoaming && !inSameCountry()) {
            domainNames =
                    getDomainNames(
                            CarrierConfigManager.Iwlan.KEY_EPDG_STATIC_ADDRESS_ROAMING_STRING);
        }
        if (domainNames == null
                && (domainNames =
                                getDomainNames(
                                        CarrierConfigManager.Iwlan.KEY_EPDG_STATIC_ADDRESS_STRING))
                        == null) {
            Log.d(TAG, "Static address string is null");
            return;
        }

        Log.d(TAG, "Static Domain Names: " + Arrays.toString(domainNames));
        for (String domainName : domainNames) {
            getIP(domainName, filter, validIpList, network);
        }
    }

    private String[] getDomainNames(String key) {
        String configValue = (String) IwlanHelper.getConfig(key, mContext, mSlotId);
        if (configValue == null || configValue.isEmpty()) {
            Log.d(TAG, key + " string is null");
            return null;
        }
        return configValue.split(",");
    }

    private boolean inSameCountry() {
        boolean inSameCountry = true;

        TelephonyManager tm = mContext.getSystemService(TelephonyManager.class);
        tm = tm.createForSubscriptionId(IwlanHelper.getSubId(mContext, mSlotId));

        if (tm != null) {
            String simCountry = tm.getSimCountryIso();
            String currentCountry = tm.getNetworkCountryIso();
            if (simCountry != null
                    && !simCountry.isEmpty()
                    && currentCountry != null
                    && !currentCountry.isEmpty()) {
                Log.d(TAG, "simCountry = " + simCountry + ", currentCountry = " + currentCountry);
                inSameCountry = simCountry.equalsIgnoreCase(currentCountry);
            }
        }

        return inSameCountry;
    }

    private void resolutionMethodPlmn(
            int filter, ArrayList<InetAddress> validIpList, boolean isEmergency, Network network) {
        String[] plmnList;
        StringBuilder domainName = new StringBuilder();

        Log.d(TAG, "PLMN Method");

        plmnList = getPlmnList();
        for (String plmn : plmnList) {
            String[] mccmnc = splitMccMnc(plmn);
            /*
             * Operator Identifier based ePDG FQDN format:
             * epdg.epc.mnc<MNC>.mcc<MCC>.pub.3gppnetwork.org
             *
             * Operator Identifier based Emergency ePDG FQDN format:
             * sos.epdg.epc.mnc<MNC>.mcc<MCC>.pub.3gppnetwork.org
             */
            if (isEmergency) {
                domainName.append("sos.");
            }

            domainName
                    .append("epdg.epc.mnc")
                    .append(mccmnc[1])
                    .append(".mcc")
                    .append(mccmnc[0])
                    .append(".pub.3gppnetwork.org");
            getIP(domainName.toString(), filter, validIpList, network);
            domainName.setLength(0);
        }
    }

    private void resolutionMethodCellularLoc(
            int filter, ArrayList<InetAddress> validIpList, boolean isEmergency, Network network) {
        String[] plmnList;
        StringBuilder domainName = new StringBuilder();

        Log.d(TAG, "CELLULAR_LOC Method");

        TelephonyManager mTelephonyManager = mContext.getSystemService(TelephonyManager.class);
        mTelephonyManager =
                mTelephonyManager.createForSubscriptionId(IwlanHelper.getSubId(mContext, mSlotId));

        if (mTelephonyManager == null) {
            Log.e(TAG, "TelephonyManager is NULL");
            return;
        }

        List<CellInfo> cellInfoList = mTelephonyManager.getAllCellInfo();
        if (cellInfoList == null) {
            Log.e(TAG, "cellInfoList is NULL");
            return;
        }

        for (CellInfo cellInfo : cellInfoList) {
            if (!cellInfo.isRegistered()) {
                continue;
            }

            if (cellInfo instanceof CellInfoGsm) {
                CellIdentityGsm gsmCellId = ((CellInfoGsm) cellInfo).getCellIdentity();
                String lacString = String.format("%04x", gsmCellId.getLac());

                lacDomainNameResolution(filter, validIpList, lacString, isEmergency, network);
            } else if (cellInfo instanceof CellInfoWcdma) {
                CellIdentityWcdma wcdmaCellId = ((CellInfoWcdma) cellInfo).getCellIdentity();
                String lacString = String.format("%04x", wcdmaCellId.getLac());

                lacDomainNameResolution(filter, validIpList, lacString, isEmergency, network);
            } else if (cellInfo instanceof CellInfoLte) {
                CellIdentityLte lteCellId = ((CellInfoLte) cellInfo).getCellIdentity();
                String tacString = String.format("%04x", lteCellId.getTac());
                String[] tacSubString = new String[2];
                tacSubString[0] = tacString.substring(0, 2);
                tacSubString[1] = tacString.substring(2);

                plmnList = getPlmnList();
                for (String plmn : plmnList) {
                    String[] mccmnc = splitMccMnc(plmn);
                    /**
                     * Tracking Area Identity based ePDG FQDN format:
                     * tac-lb<TAC-low-byte>.tac-hb<TAC-high-byte>.tac.
                     * epdg.epc.mnc<MNC>.mcc<MCC>.pub.3gppnetwork.org
                     *
                     * <p>Tracking Area Identity based Emergency ePDG FQDN format:
                     * tac-lb<TAC-low-byte>.tac-hb<TAC-highbyte>.tac.
                     * sos.epdg.epc.mnc<MNC>.mcc<MCC>.pub.3gppnetwork.org"
                     */
                    domainName
                            .append("tac-lb")
                            .append(tacSubString[1])
                            .append(".tac-hb")
                            .append(tacSubString[0]);
                    if (isEmergency) {
                        domainName.append(".tac.sos.epdg.epc.mnc");
                    } else {
                        domainName.append(".tac.epdg.epc.mnc");
                    }
                    domainName
                            .append(mccmnc[1])
                            .append(".mcc")
                            .append(mccmnc[0])
                            .append(".pub.3gppnetwork.org");
                    getIP(domainName.toString(), filter, validIpList, network);
                    domainName.setLength(0);
                }
            } else if (cellInfo instanceof CellInfoNr) {
                CellIdentityNr nrCellId =
                        (CellIdentityNr) ((CellInfoNr) cellInfo).getCellIdentity();
                String tacString = String.format("%06x", nrCellId.getTac());
                String[] tacSubString = new String[3];
                tacSubString[0] = tacString.substring(0, 2);
                tacSubString[1] = tacString.substring(2, 4);
                tacSubString[2] = tacString.substring(4);

                plmnList = getPlmnList();
                for (String plmn : plmnList) {
                    String[] mccmnc = splitMccMnc(plmn);
                    /**
                     * 5GS Tracking Area Identity based ePDG FQDN format:
                     * tac-lb<TAC-low-byte>.tac-mb<TAC-middle-byte>.tac-hb<TAC-high-byte>.
                     * 5gstac.epdg.epc.mnc<MNC>.mcc<MCC>.pub.3gppnetwork.org
                     *
                     * <p>5GS Tracking Area Identity based Emergency ePDG FQDN format:
                     * tac-lb<TAC-low-byte>.tac-mb<TAC-middle-byte>.tac-hb<TAC-high-byte>.
                     * 5gstac.sos.epdg.epc.mnc<MNC>.mcc<MCC>.pub.3gppnetwork.org
                     */
                    domainName
                            .append("tac-lb")
                            .append(tacSubString[2])
                            .append(".tac-mb")
                            .append(tacSubString[1])
                            .append(".tac-hb")
                            .append(tacSubString[0]);
                    if (isEmergency) {
                        domainName.append(".5gstac.sos.epdg.epc.mnc");
                    } else {
                        domainName.append(".5gstac.epdg.epc.mnc");
                    }
                    domainName
                            .append(mccmnc[1])
                            .append(".mcc")
                            .append(mccmnc[0])
                            .append(".pub.3gppnetwork.org");
                    getIP(domainName.toString(), filter, validIpList, network);
                    domainName.setLength(0);
                }
            } else {
                Log.d(TAG, "This cell doesn't contain LAC/TAC info");
            }
        }
    }

    private void lacDomainNameResolution(
            int filter,
            ArrayList<InetAddress> validIpList,
            String lacString,
            boolean isEmergency,
            Network network) {
        String[] plmnList;
        StringBuilder domainName = new StringBuilder();

        plmnList = getPlmnList();
        for (String plmn : plmnList) {
            String[] mccmnc = splitMccMnc(plmn);
            /**
             * Location Area Identity based ePDG FQDN format:
             * lac<LAC>.epdg.epc.mnc<MNC>.mcc<MCC>.pub.3gppnetwork.org
             *
             * <p>Location Area Identity based Emergency ePDG FQDN format:
             * lac<LAC>.sos.epdg.epc.mnc<MNC>.mcc<MCC>.pub.3gppnetwork.org
             */
            domainName.append("lac").append(lacString);
            if (isEmergency) {
                domainName.append(".sos.epdg.epc.mnc");
            } else {
                domainName.append(".epdg.epc.mnc");
            }
            domainName
                    .append(mccmnc[1])
                    .append(".mcc")
                    .append(mccmnc[0])
                    .append(".pub.3gppnetwork.org");

            getIP(domainName.toString(), filter, validIpList, network);
            domainName.setLength(0);
        }
    }

    private void resolutionMethodPco(int filter, ArrayList<InetAddress> validIpList) {
        Log.d(TAG, "PCO Method");

        int PCO_ID_IPV6 =
                IwlanHelper.getConfig(
                        CarrierConfigManager.Iwlan.KEY_EPDG_PCO_ID_IPV6_INT, mContext, mSlotId);
        int PCO_ID_IPV4 =
                IwlanHelper.getConfig(
                        CarrierConfigManager.Iwlan.KEY_EPDG_PCO_ID_IPV4_INT, mContext, mSlotId);

        switch (filter) {
            case PROTO_FILTER_IPV4:
                if (mV4PcoId != PCO_ID_IPV4) {
                    clearPcoData();
                } else {
                    getInetAddressWithPcoData(mV4PcoData, validIpList);
                }
                break;
            case PROTO_FILTER_IPV6:
                if (mV6PcoId != PCO_ID_IPV6) {
                    clearPcoData();
                } else {
                    getInetAddressWithPcoData(mV6PcoData, validIpList);
                }
                break;
            case PROTO_FILTER_IPV4V6:
                if ((mV4PcoId != PCO_ID_IPV4) || (mV6PcoId != PCO_ID_IPV6)) {
                    clearPcoData();
                } else {
                    getInetAddressWithPcoData(mV4PcoData, validIpList);
                    getInetAddressWithPcoData(mV6PcoData, validIpList);
                }
                break;
            default:
                Log.d(TAG, "Invalid ProtoFilter : " + filter);
        }
    }

    private void getInetAddressWithPcoData(byte[] pcoData, ArrayList<InetAddress> validIpList) {
        InetAddress ipAddress;
        if (pcoData != null && pcoData.length > 0) {
            try {
                ipAddress = InetAddress.getByAddress(pcoData);
                validIpList.add(ipAddress);
            } catch (Exception e) {
                Log.e(TAG, "Exception when querying IP address : " + e);
            }
        } else {
            Log.d(TAG, "Empty PCO data");
        }
    }

    public IwlanError getValidatedServerList(
            int transactionId,
            @ProtoFilter int filter,
            boolean isRoaming,
            boolean isEmergency,
            @NonNull Network network,
            EpdgSelectorCallback selectorCallback) {
        ArrayList<InetAddress> validIpList = new ArrayList<InetAddress>();
        StringBuilder domainName = new StringBuilder();

        Runnable doValidation =
                () -> {
                    Log.d(TAG, "Processing request with transactionId: " + transactionId);
                    String[] plmnList;

                    int[] addrResolutionMethods =
                            IwlanHelper.getConfig(
                                    CarrierConfigManager.Iwlan.KEY_EPDG_ADDRESS_PRIORITY_INT_ARRAY,
                                    mContext,
                                    mSlotId);

                    for (int addrResolutionMethod : addrResolutionMethods) {
                        switch (addrResolutionMethod) {
                            case CarrierConfigManager.Iwlan.EPDG_ADDRESS_STATIC:
                                resolutionMethodStatic(filter, validIpList, isRoaming, network);
                                break;

                            case CarrierConfigManager.Iwlan.EPDG_ADDRESS_PLMN:
                                resolutionMethodPlmn(filter, validIpList, isEmergency, network);
                                break;

                            case CarrierConfigManager.Iwlan.EPDG_ADDRESS_PCO:
                                resolutionMethodPco(filter, validIpList);
                                break;

                            case CarrierConfigManager.Iwlan.EPDG_ADDRESS_CELLULAR_LOC:
                                resolutionMethodCellularLoc(
                                        filter, validIpList, isEmergency, network);
                                break;

                            default:
                                Log.d(
                                        TAG,
                                        "Incorrect address resolution method "
                                                + addrResolutionMethod);
                        }
                    }

                    if (selectorCallback != null) {
                        if (!validIpList.isEmpty()) {
                            Collections.sort(validIpList, inetAddressComparator);
                            selectorCallback.onServerListChanged(
                                    transactionId, removeDuplicateIp(validIpList));
                        } else {
                            selectorCallback.onError(
                                    transactionId,
                                    new IwlanError(
                                            IwlanError.EPDG_SELECTOR_SERVER_SELECTION_FAILED));
                        }
                    }
                };
        Thread subThread = new Thread(doValidation);
        subThread.start();
        return new IwlanError(IwlanError.NO_ERROR);
    }
}
