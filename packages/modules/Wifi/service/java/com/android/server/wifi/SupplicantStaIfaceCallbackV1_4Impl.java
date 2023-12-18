/*
 * Copyright (C) 2019 The Android Open Source Project
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
package com.android.server.wifi;


import android.annotation.NonNull;
import android.hardware.wifi.supplicant.V1_4.ISupplicantStaIfaceCallback.AssociationRejectionData;
import android.net.wifi.WifiManager;

import com.android.server.wifi.hotspot2.WnmData;
import com.android.server.wifi.util.NativeUtil;

import java.util.ArrayList;

abstract class SupplicantStaIfaceCallbackV1_4Impl extends
        android.hardware.wifi.supplicant.V1_4.ISupplicantStaIfaceCallback.Stub {
    private static final String TAG = SupplicantStaIfaceCallbackV1_4Impl.class.getSimpleName();
    private final SupplicantStaIfaceHal mStaIfaceHal;
    private final String mIfaceName;
    private final WifiMonitor mWifiMonitor;
    private final Object mLock;
    private final SupplicantStaIfaceHal.SupplicantStaIfaceHalCallbackV1_3 mCallbackV13;
    private final SupplicantStaIfaceHal.SupplicantStaIfaceHalCallback mCallbackV10;

    SupplicantStaIfaceCallbackV1_4Impl(@NonNull SupplicantStaIfaceHal staIfaceHal,
            @NonNull String ifaceName, @NonNull Object lock,
            @NonNull WifiMonitor wifiMonitor) {
        mStaIfaceHal = staIfaceHal;
        mIfaceName = ifaceName;
        mLock = lock;
        mWifiMonitor = wifiMonitor;
        // Create an older callback for function delegation,
        // and it would cascadingly create older one.
        mCallbackV13 = mStaIfaceHal.new SupplicantStaIfaceHalCallbackV1_3(mIfaceName);
        mCallbackV10 = mStaIfaceHal.new SupplicantStaIfaceHalCallback(mIfaceName);
    }

    @Override
    public void onNetworkAdded(int id) {
        mCallbackV13.onNetworkAdded(id);
    }

    @Override
    public void onNetworkRemoved(int id) {
        mCallbackV13.onNetworkRemoved(id);
    }

    @Override
    public void onStateChanged(int newState, byte[/* 6 */] bssid, int id,
            ArrayList<Byte> ssid) {
        mCallbackV13.onStateChanged(newState, bssid, id, ssid);
    }

    @Override
    public void onAnqpQueryDone_1_4(byte[/* 6 */] bssid,
            AnqpData data,
            Hs20AnqpData hs20Data) {
        synchronized (mLock) {
            mStaIfaceHal.logCallback("onAnqpQueryDone_1_4");
            mCallbackV10.onAnqpQueryDone(bssid, data.V1_0 /* v1.0 elemnt */, hs20Data,
                    data /* v1.4 element */);
        }
    }

    @Override
    public void onAnqpQueryDone(byte[/* 6 */] bssid,
            android.hardware.wifi.supplicant.V1_0.ISupplicantStaIfaceCallback.AnqpData data,
            Hs20AnqpData hs20Data) {
        mCallbackV13.onAnqpQueryDone(bssid, data, hs20Data);
    }

    @Override
    public void onHs20IconQueryDone(byte[/* 6 */] bssid, String fileName,
            ArrayList<Byte> data) {
        mCallbackV13.onHs20IconQueryDone(bssid, fileName, data);
    }

    @Override
    public void onHs20SubscriptionRemediation(byte[/* 6 */] bssid,
            byte osuMethod, String url) {
        mCallbackV13.onHs20SubscriptionRemediation(bssid, osuMethod, url);
    }

    @Override
    public void onHs20DeauthImminentNotice(byte[/* 6 */] bssid, int reasonCode,
            int reAuthDelayInSec, String url) {
        mCallbackV13.onHs20DeauthImminentNotice(bssid, reasonCode, reAuthDelayInSec, url);
    }

    @Override
    public void onDisconnected(byte[/* 6 */] bssid, boolean locallyGenerated,
            int reasonCode) {
        mCallbackV13.onDisconnected(bssid, locallyGenerated, reasonCode);
    }

    @Override
    public void onAssociationRejected_1_4(AssociationRejectionData assocRejectData) {
        synchronized (mLock) {
            mStaIfaceHal.logCallback("onAssociationRejected_1_4");
            mCallbackV10.onAssociationRejected(assocRejectData);
        }
    }

    @Override
    public void onAssociationRejected(byte[/* 6 */] bssid, int statusCode,
            boolean timedOut) {
        mCallbackV13.onAssociationRejected(bssid, statusCode, timedOut);
    }

    @Override
    public void onAuthenticationTimeout(byte[/* 6 */] bssid) {
        mCallbackV13.onAuthenticationTimeout(bssid);
    }

    @Override
    public void onBssidChanged(byte reason, byte[/* 6 */] bssid) {
        mCallbackV13.onBssidChanged(reason, bssid);
    }

    @Override
    public void onEapFailure() {
        mCallbackV13.onEapFailure();
    }

    @Override
    public void onEapFailure_1_1(int code) {
        mCallbackV13.onEapFailure_1_1(code);
    }

    @Override
    public void onEapFailure_1_3(int code) {
        mWifiMonitor.broadcastAuthenticationFailureEvent(
                mIfaceName, WifiManager.ERROR_AUTH_FAILURE_EAP_FAILURE, code);
    }

    @Override
    public void onWpsEventSuccess() {
        mCallbackV13.onWpsEventSuccess();
    }

    @Override
    public void onWpsEventFail(byte[/* 6 */] bssid, short configError, short errorInd) {
        mCallbackV13.onWpsEventFail(bssid, configError, errorInd);
    }

    @Override
    public void onWpsEventPbcOverlap() {
        mCallbackV13.onWpsEventPbcOverlap();
    }

    @Override
    public void onExtRadioWorkStart(int id) {
        mCallbackV13.onExtRadioWorkStart(id);
    }

    @Override
    public void onExtRadioWorkTimeout(int id) {
        mCallbackV13.onExtRadioWorkTimeout(id);
    }

    @Override
    public void onDppSuccessConfigReceived(ArrayList<Byte> ssid, String password,
            byte[] psk, int securityAkm) {
        mCallbackV13.onDppSuccessConfigReceived(
                ssid, password, psk, securityAkm);
    }

    @Override
    public void onDppSuccessConfigSent() {
        mCallbackV13.onDppSuccessConfigSent();
    }

    @Override
    public void onDppProgress(int code) {
        mCallbackV13.onDppProgress(code);
    }

    @Override
    public void onDppFailure(int code) {
        mCallbackV13.onDppFailure(code);
    }

    @Override
    public void onPmkCacheAdded(long expirationTimeInSec, ArrayList<Byte> serializedEntry) {
        mCallbackV13.onPmkCacheAdded(expirationTimeInSec, serializedEntry);
    }

    @Override
    public void onDppProgress_1_3(int code) {
        mCallbackV13.onDppProgress_1_3(code);
    }

    @Override
    public void onDppFailure_1_3(int code, String ssid, String channelList,
            ArrayList<Short> bandList) {
        mCallbackV13.onDppFailure_1_3(code, ssid, channelList, bandList);
    }

    @Override
    public void onDppSuccess(int code) {
        mCallbackV13.onDppSuccess(code);
    }

    @Override
    public void onBssTmHandlingDone(BssTmData tmData) {
        mCallbackV13.onBssTmHandlingDone(tmData);
    }

    @Override
    public void onStateChanged_1_3(int newState, byte[/* 6 */] bssid, int id,
            ArrayList<Byte> ssid, boolean filsHlpSent) {
        mCallbackV13.onStateChanged_1_3(newState, bssid, id, ssid, filsHlpSent);
    }

    @Override
    public void onHs20TermsAndConditionsAcceptanceRequestedNotification(byte[/* 6 */] bssid,
            String url) {
        synchronized (mLock) {
            mStaIfaceHal.logCallback("onHs20TermsAndConditionsAcceptanceRequestedNotification");
            mWifiMonitor.broadcastWnmEvent(mIfaceName,
                    WnmData.createTermsAndConditionsAccetanceRequiredEvent(
                            NativeUtil.macAddressToLong(bssid), url));
        }
    }

    @Override
    public void onNetworkNotFound(ArrayList<Byte> ssid) {
        mStaIfaceHal.logCallback("onNetworkNotFoundNotification");
        mWifiMonitor.broadcastNetworkNotFoundEvent(mIfaceName, NativeUtil.encodeSsid(ssid));
    }
}
