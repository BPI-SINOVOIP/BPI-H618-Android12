/*
 * Copyright (C) 2018 The Android Open Source Project
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

import static android.net.wifi.WifiManager.EASY_CONNECT_NETWORK_ROLE_AP;

import android.annotation.Nullable;
import android.content.Context;
import android.hardware.wifi.supplicant.V1_2.DppAkm;
import android.hardware.wifi.supplicant.V1_2.DppNetRole;
import android.hardware.wifi.supplicant.V1_3.DppProgressCode;
import android.hardware.wifi.supplicant.V1_3.DppSuccessCode;
import android.hardware.wifi.supplicant.V1_4.DppCurve;
import android.hardware.wifi.supplicant.V1_4.DppFailureCode;
import android.net.wifi.EasyConnectStatusCallback;
import android.net.wifi.IDppCallback;
import android.net.wifi.ScanResult;
import android.net.wifi.WifiConfiguration;
import android.net.wifi.WifiManager;
import android.os.Build;
import android.os.Handler;
import android.os.IBinder;
import android.os.RemoteException;
import android.text.TextUtils;
import android.util.Log;
import android.util.SparseArray;

import androidx.annotation.RequiresApi;

import com.android.internal.annotations.VisibleForTesting;
import com.android.internal.util.WakeupMessage;
import com.android.modules.utils.build.SdkLevel;
import com.android.server.wifi.WifiNative.DppEventCallback;
import com.android.server.wifi.util.ApConfigUtil;
import com.android.server.wifi.util.WifiPermissionsUtil;

import java.util.ArrayList;
import java.util.List;

/**
 * DPP Manager class
 * Implements the DPP Initiator APIs and callbacks
 */
public class DppManager {
    private static final String TAG = "DppManager";
    private final Handler mHandler;

    private DppRequestInfo mDppRequestInfo = null;
    private final WifiNative mWifiNative;
    private String mClientIfaceName;
    private boolean mVerboseLoggingEnabled;
    WifiConfigManager mWifiConfigManager;
    private final Context mContext;
    @VisibleForTesting
    public WakeupMessage mDppTimeoutMessage = null;
    private final Clock mClock;
    private static final String DPP_TIMEOUT_TAG = TAG + " Request Timeout";
    private static final int DPP_TIMEOUT_MS = 40_000; // 40 seconds
    private static final int DPP_RESPONDER_TIMEOUT_MS = 300_000; // 5 minutes
    public static final int DPP_AUTH_ROLE_INACTIVE = -1;
    public static final int DPP_AUTH_ROLE_INITIATOR = 0;
    public static final int DPP_AUTH_ROLE_RESPONDER = 1;
    private final DppMetrics mDppMetrics;
    private final ScanRequestProxy mScanRequestProxy;
    private final WifiPermissionsUtil mWifiPermissionsUtil;

    private final DppEventCallback mDppEventCallback = new DppEventCallback() {
        @Override
        public void onSuccessConfigReceived(WifiConfiguration newWifiConfiguration) {
            mHandler.post(() -> {
                DppManager.this.onSuccessConfigReceived(newWifiConfiguration);
            });
        }

        @Override
        public void onSuccess(int dppStatusCode) {
            mHandler.post(() -> {
                DppManager.this.onSuccess(dppStatusCode);
            });
        }

        @Override
        public void onProgress(int dppStatusCode) {
            mHandler.post(() -> {
                DppManager.this.onProgress(dppStatusCode);
            });
        }

        @Override
        public void onFailure(int dppStatusCode, String ssid, String channelList, int[] bandList) {
            mHandler.post(() -> {
                DppManager.this.onFailure(dppStatusCode, ssid, channelList, bandList);
            });
        }
    };

    DppManager(Handler handler, WifiNative wifiNative, WifiConfigManager wifiConfigManager,
            Context context, DppMetrics dppMetrics, ScanRequestProxy scanRequestProxy,
            WifiPermissionsUtil wifiPermissionsUtil) {
        mHandler = handler;
        mWifiNative = wifiNative;
        mWifiConfigManager = wifiConfigManager;
        mWifiNative.registerDppEventCallback(mDppEventCallback);
        mContext = context;
        mClock = new Clock();
        mDppMetrics = dppMetrics;
        mScanRequestProxy = scanRequestProxy;
        mWifiPermissionsUtil = wifiPermissionsUtil;

        // Setup timer
        mDppTimeoutMessage = new WakeupMessage(mContext, mHandler,
                DPP_TIMEOUT_TAG, () -> {
            timeoutDppRequest();
        });
    }

    private static String encodeStringToHex(String str) {
        if ((str.length() > 1) && (str.charAt(0) == '"') && (str.charAt(str.length() - 1) == '"')) {
            // Remove the surrounding quotes
            str = str.substring(1, str.length() - 1);

            // Convert to Hex
            char[] charsArray = str.toCharArray();
            StringBuffer hexBuffer = new StringBuffer();
            for (int i = 0; i < charsArray.length; i++) {
                hexBuffer.append(Integer.toHexString((int) charsArray[i]));
            }
            return hexBuffer.toString();
        }
        return str;
    }

    private void timeoutDppRequest() {
        logd("DPP timeout");

        if (mDppRequestInfo == null) {
            Log.e(TAG, "DPP timeout with no request info");
            return;
        }

        // Clean up supplicant resources
        if (!mWifiNative.stopDppInitiator(mClientIfaceName)) {
            Log.e(TAG, "Failed to stop DPP Initiator");
        }

        // Clean up resources and let the caller know about the timeout
        onFailure(DppFailureCode.TIMEOUT);
    }

    /**
     * Start DPP request in Configurator-Initiator mode. The goal of this call is to send the
     * selected Wi-Fi configuration to a remote peer so it could join that network.
     *
     * @param uid                 UID
     * @param packageName         Package name of the calling app
     * @param clientIfaceName     Client interface to use for this operation.
     * @param binder              Binder object
     * @param enrolleeUri         The Enrollee URI, scanned externally (e.g. via QR code)
     * @param selectedNetworkId   The selected Wi-Fi network ID to be sent
     * @param enrolleeNetworkRole Network role of remote enrollee: STA or AP
     * @param callback            DPP Callback object
     */
    public void startDppAsConfiguratorInitiator(int uid, @Nullable String packageName,
            @Nullable String clientIfaceName, IBinder binder, String enrolleeUri,
            int selectedNetworkId, @WifiManager.EasyConnectNetworkRole int enrolleeNetworkRole,
            IDppCallback callback) {
        mDppMetrics.updateDppConfiguratorInitiatorRequests();
        if (isSessionInProgress()) {
            try {
                Log.e(TAG, "DPP request already in progress");
                Log.e(TAG, "Ongoing request - UID: " + mDppRequestInfo.uid
                        + " Package: " + mDppRequestInfo.packageName
                        + ", New request - UID: " + uid + " Package: " + packageName);

                mDppMetrics.updateDppFailure(EasyConnectStatusCallback
                        .EASY_CONNECT_EVENT_FAILURE_BUSY);
                // On going DPP. Call the failure callback directly
                callback.onFailure(EasyConnectStatusCallback.EASY_CONNECT_EVENT_FAILURE_BUSY, null,
                        null, new int[0]);
            } catch (RemoteException e) {
                // Empty
            }
            return;
        }

        mClientIfaceName = clientIfaceName;
        if (mClientIfaceName == null) {
            try {
                Log.e(TAG, "Wi-Fi client interface does not exist");
                // On going DPP. Call the failure callback directly
                mDppMetrics.updateDppFailure(EasyConnectStatusCallback
                        .EASY_CONNECT_EVENT_FAILURE_GENERIC);
                callback.onFailure(EasyConnectStatusCallback.EASY_CONNECT_EVENT_FAILURE_GENERIC,
                        null, null, new int[0]);
            } catch (RemoteException e) {
                // Empty
            }
            return;
        }

        WifiConfiguration selectedNetwork = mWifiConfigManager
                .getConfiguredNetworkWithoutMasking(selectedNetworkId);

        if (selectedNetwork == null) {
            try {
                Log.e(TAG, "Selected network is null");
                // On going DPP. Call the failure callback directly
                mDppMetrics.updateDppFailure(EasyConnectStatusCallback
                        .EASY_CONNECT_EVENT_FAILURE_INVALID_NETWORK);
                callback.onFailure(EasyConnectStatusCallback
                        .EASY_CONNECT_EVENT_FAILURE_INVALID_NETWORK, null, null, new int[0]);
            } catch (RemoteException e) {
                // Empty
            }
            return;
        }

        String password = null;
        String psk = null;
        int securityAkm;

        // Currently support either SAE mode or PSK mode
        // Check PSK first because PSK config always has a SAE type as a upgrading type.
        if (selectedNetwork.isSecurityType(WifiConfiguration.SECURITY_TYPE_PSK)) {
            if (selectedNetwork.preSharedKey.matches(String.format("[0-9A-Fa-f]{%d}", 64))) {
                // PSK
                psk = selectedNetwork.preSharedKey;
            } else {
                // Passphrase
                password = selectedNetwork.preSharedKey;
            }
            securityAkm = DppAkm.PSK;
        } else if (selectedNetwork.isSecurityType(WifiConfiguration.SECURITY_TYPE_SAE)) {
            // SAE
            password = selectedNetwork.preSharedKey;
            securityAkm = DppAkm.SAE;
        } else {
            try {
                // Key management must be either PSK or SAE
                Log.e(TAG, "Key management must be either PSK or SAE");
                mDppMetrics.updateDppFailure(EasyConnectStatusCallback
                        .EASY_CONNECT_EVENT_FAILURE_INVALID_NETWORK);
                callback.onFailure(
                        EasyConnectStatusCallback.EASY_CONNECT_EVENT_FAILURE_INVALID_NETWORK, null,
                        null, new int[0]);
            } catch (RemoteException e) {
                // Empty
            }
            return;
        }

        mDppRequestInfo = new DppRequestInfo();
        mDppRequestInfo.uid = uid;
        mDppRequestInfo.packageName = packageName;
        mDppRequestInfo.binder = binder;
        mDppRequestInfo.callback = callback;
        mDppRequestInfo.authRole = DPP_AUTH_ROLE_INITIATOR;

        if (!linkToDeath(mDppRequestInfo)) {
            // Notify failure and clean up
            onFailure(EasyConnectStatusCallback.EASY_CONNECT_EVENT_FAILURE_GENERIC);
            return;
        }

        logd("Interface " + mClientIfaceName + ": Initializing URI: " + enrolleeUri);

        mDppRequestInfo.startTime = mClock.getElapsedSinceBootMillis();
        mDppTimeoutMessage.schedule(mDppRequestInfo.startTime + DPP_TIMEOUT_MS);

        // Send Enrollee URI and get a peer ID
        int peerId = mWifiNative.addDppPeerUri(mClientIfaceName, enrolleeUri);

        if (peerId < 0) {
            Log.e(TAG, "DPP add URI failure");

            // Notify failure and clean up
            onFailure(DppFailureCode.INVALID_URI);
            return;
        }
        mDppRequestInfo.peerId = peerId;

        // Auth init
        logd("Authenticating");

        String ssidEncoded = encodeStringToHex(selectedNetwork.SSID);
        String passwordEncoded = null;

        if (password != null) {
            passwordEncoded = encodeStringToHex(selectedNetwork.preSharedKey);
        }

        if (!mWifiNative.startDppConfiguratorInitiator(mClientIfaceName,
                mDppRequestInfo.peerId, 0, ssidEncoded, passwordEncoded, psk,
                enrolleeNetworkRole == EASY_CONNECT_NETWORK_ROLE_AP ? DppNetRole.AP
                        : DppNetRole.STA,
                securityAkm)) {
            Log.e(TAG, "DPP Start Configurator Initiator failure");

            // Notify failure and clean up
            onFailure(DppFailureCode.FAILURE);
            return;
        }

        logd("Success: Started DPP Initiator with peer ID "
                + mDppRequestInfo.peerId);
    }

    /**
     * Start DPP request in Enrollee-Initiator mode. The goal of this call is to receive a
     * Wi-Fi configuration object from the peer configurator in order to join a network.
     *
     * @param uid             UID
     * @param clientIfaceName     Client interface to use for this operation.
     * @param binder          Binder object
     * @param configuratorUri The Configurator URI, scanned externally (e.g. via QR code)
     * @param callback        DPP Callback object
     */
    public void startDppAsEnrolleeInitiator(int uid, @Nullable String clientIfaceName,
            IBinder binder, String configuratorUri, IDppCallback callback) {
        mDppMetrics.updateDppEnrolleeInitiatorRequests();
        if (isSessionInProgress()) {
            try {
                Log.e(TAG, "DPP request already in progress");
                Log.e(TAG, "Ongoing request UID: " + mDppRequestInfo.uid + ", new UID: "
                        + uid);

                mDppMetrics.updateDppFailure(EasyConnectStatusCallback
                        .EASY_CONNECT_EVENT_FAILURE_BUSY);
                // On going DPP. Call the failure callback directly
                callback.onFailure(EasyConnectStatusCallback.EASY_CONNECT_EVENT_FAILURE_BUSY, null,
                        null, new int[0]);
            } catch (RemoteException e) {
                // Empty
            }
            return;
        }

        mDppRequestInfo = new DppRequestInfo();
        mDppRequestInfo.uid = uid;
        mDppRequestInfo.binder = binder;
        mDppRequestInfo.callback = callback;
        mDppRequestInfo.authRole = DPP_AUTH_ROLE_INITIATOR;

        if (!linkToDeath(mDppRequestInfo)) {
            // Notify failure and clean up
            onFailure(DppFailureCode.FAILURE);
            return;
        }

        mDppRequestInfo.startTime = mClock.getElapsedSinceBootMillis();
        mDppTimeoutMessage.schedule(mDppRequestInfo.startTime + DPP_TIMEOUT_MS);

        mClientIfaceName = clientIfaceName;
        logd("Interface " + mClientIfaceName + ": Initializing URI: " + configuratorUri);

        // Send Configurator URI and get a peer ID
        int peerId = mWifiNative.addDppPeerUri(mClientIfaceName, configuratorUri);

        if (peerId < 0) {
            Log.e(TAG, "DPP add URI failure");
            onFailure(DppFailureCode.INVALID_URI);
            return;
        }
        mDppRequestInfo.peerId = peerId;

        // Auth init
        logd("Authenticating");

        if (!mWifiNative.startDppEnrolleeInitiator(mClientIfaceName, mDppRequestInfo.peerId,
                0)) {
            Log.e(TAG, "DPP Start Enrollee Initiator failure");

            // Notify failure and clean up
            onFailure(DppFailureCode.FAILURE);
            return;
        }

        logd("Success: Started DPP Initiator with peer ID "
                + mDppRequestInfo.peerId);
    }

    /**
     * Start DPP request in Enrollee-Responder mode. The goal of this call is to receive a
     * Wi-Fi configuration object from the peer configurator by showing a QR code and being scanned
     * by the peer configurator.
     *
     * @param uid             UID
     * @param clientIfaceName Client interface to use for this operation.
     * @param binder          Binder object
     * @param deviceInfo      The Device specific info to attach to the generated URI
     * @param curve           Elliptic curve cryptography type used to generate DPP
     *                        public/private key pair.
     * @param callback        DPP Callback object
     */
    public void startDppAsEnrolleeResponder(int uid, @Nullable String clientIfaceName,
            IBinder binder, @Nullable String deviceInfo,
            @WifiManager.EasyConnectCryptographyCurve int curve, IDppCallback callback) {
        mDppMetrics.updateDppEnrolleeResponderRequests();
        if (isSessionInProgress()) {
            try {
                Log.e(TAG, "DPP request already in progress");
                Log.e(TAG, "Ongoing request UID: " + mDppRequestInfo.uid + ", new UID: "
                        + uid);

                mDppMetrics.updateDppFailure(EasyConnectStatusCallback
                        .EASY_CONNECT_EVENT_FAILURE_BUSY);
                // On going DPP. Call the failure callback directly
                callback.onFailure(EasyConnectStatusCallback.EASY_CONNECT_EVENT_FAILURE_BUSY,
                        null, null, new int[0]);
            } catch (RemoteException e) {
                // Empty
            }
            return;
        }

        mDppRequestInfo = new DppRequestInfo();
        mDppRequestInfo.uid = uid;
        mDppRequestInfo.binder = binder;
        mDppRequestInfo.callback = callback;
        mDppRequestInfo.authRole = DPP_AUTH_ROLE_RESPONDER;

        if (!linkToDeath(mDppRequestInfo)) {
            // Notify failure and clean up
            onFailure(DppFailureCode.FAILURE);
            return;
        }

        mDppRequestInfo.startTime = mClock.getElapsedSinceBootMillis();
        mDppTimeoutMessage.schedule(mDppRequestInfo.startTime + DPP_RESPONDER_TIMEOUT_MS);

        mClientIfaceName = clientIfaceName;
        logd("Interface " + mClientIfaceName + " Product Info: " + deviceInfo
                + " Curve: " + curve);

        String info = deviceInfo == null ? "" : deviceInfo;
        // Generate a QR code based bootstrap info
        WifiNative.DppBootstrapQrCodeInfo bootstrapInfo = null;
        if (SdkLevel.isAtLeastS()) {
            bootstrapInfo =
                    mWifiNative.generateDppBootstrapInfoForResponder(mClientIfaceName, deviceInfo,
                            convertEasyConnectCryptographyCurveToHidlDppCurve(curve));
        }

        if (bootstrapInfo == null || bootstrapInfo.bootstrapId < 0
                || TextUtils.isEmpty(bootstrapInfo.uri)) {
            Log.e(TAG, "DPP request to generate URI failed");
            onFailure(DppFailureCode.URI_GENERATION);
            return;
        }

        mDppRequestInfo.bootstrapId = bootstrapInfo.bootstrapId;
        logd("BootstrapId:" + mDppRequestInfo.bootstrapId + " URI: " + bootstrapInfo.uri);

        if (!mWifiNative.startDppEnrolleeResponder(mClientIfaceName, bootstrapInfo.listenChannel)) {
            Log.e(TAG, "DPP Start Enrollee Responder failure");
            // Notify failure and clean up
            onFailure(DppFailureCode.FAILURE);
            return;
        }

        logd("Success: Started DPP Enrollee Responder on listen channel "
                + bootstrapInfo.listenChannel);

        try {
            mDppRequestInfo.callback.onBootstrapUriGenerated(bootstrapInfo.uri);
        } catch (RemoteException e) {
            Log.e(TAG, " onBootstrapUriGenerated Callback failure");
            onFailure(DppFailureCode.FAILURE);
            return;
        }
    }

    /**
     * Stop a current DPP session
     *
     * @param uid User ID
     */
    public void stopDppSession(int uid) {
        if (!isSessionInProgress()) {
            logd("UID " + uid + " called stop DPP session with no active DPP session");
            return;
        }

        if (mDppRequestInfo.uid != uid) {
            Log.e(TAG, "UID " + uid + " called stop DPP session but UID " + mDppRequestInfo.uid
                    + " has started it");
            return;
        }

        // Clean up supplicant resources
        if (mDppRequestInfo.authRole == DPP_AUTH_ROLE_INITIATOR) {
            if (!mWifiNative.stopDppInitiator(mClientIfaceName)) {
                Log.e(TAG, "Failed to stop DPP Initiator");
            }
        }

        cleanupDppResources();

        logd("Success: Stopped DPP Session");
    }

    private void cleanupDppResources() {
        logd("DPP clean up resources");
        if (!isSessionInProgress()) {
            return;
        }

        // Cancel pending timeout
        mDppTimeoutMessage.cancel();

        // Remove the URI from the supplicant list
        if (mDppRequestInfo.authRole == DPP_AUTH_ROLE_INITIATOR) {
            if (!mWifiNative.removeDppUri(mClientIfaceName, mDppRequestInfo.peerId)) {
                Log.e(TAG, "Failed to remove DPP URI ID " + mDppRequestInfo.peerId);
            }
        } else if (mDppRequestInfo.authRole == DPP_AUTH_ROLE_RESPONDER) {
            if (!mWifiNative.stopDppResponder(mClientIfaceName, mDppRequestInfo.bootstrapId)) {
                Log.e(TAG, "Failed to stop DPP Responder");
            }
        }

        mDppRequestInfo.binder.unlinkToDeath(mDppRequestInfo.dr, 0);

        mDppRequestInfo = null;
    }

    /**
     * Indicates whether there is a dpp session in progress or not.
     */
    public boolean isSessionInProgress() {
        return mDppRequestInfo != null;
    }

    private static class DppRequestInfo {
        public int uid;
        public String packageName;
        public IBinder binder;
        public IBinder.DeathRecipient dr;
        public int peerId;
        public IDppCallback callback;
        public long startTime;
        public int authRole = DPP_AUTH_ROLE_INACTIVE;
        public int bootstrapId;

        @Override
        public String toString() {
            return new StringBuilder("DppRequestInfo: uid=").append(uid).append(", binder=").append(
                    binder).append(", dr=").append(dr)
                    .append(", callback=").append(callback)
                    .append(", peerId=").append(peerId)
                    .append(", authRole=").append(authRole)
                    .append(", bootstrapId=").append(bootstrapId).toString();
        }
    }

    /**
     * Enable vervose logging from DppManager
     *
     * @param verbose 0 to disable verbose logging, or any other value to enable.
     */
    public void enableVerboseLogging(int verbose) {
        mVerboseLoggingEnabled = verbose != 0 ? true : false;
    }

    private void onSuccessConfigReceived(WifiConfiguration newWifiConfiguration) {
        try {
            logd("onSuccessConfigReceived");

            if (mDppRequestInfo != null) {
                long now = mClock.getElapsedSinceBootMillis();
                mDppMetrics.updateDppOperationTime((int) (now - mDppRequestInfo.startTime));

                NetworkUpdateResult networkUpdateResult = mWifiConfigManager
                        .addOrUpdateNetwork(newWifiConfiguration, mDppRequestInfo.uid);

                if (networkUpdateResult.isSuccess()) {
                    mDppMetrics.updateDppEnrolleeSuccess();
                    if (mDppRequestInfo.authRole == DPP_AUTH_ROLE_RESPONDER) {
                        mDppMetrics.updateDppEnrolleeResponderSuccess();
                    }
                    mDppRequestInfo.callback.onSuccessConfigReceived(
                            WifiConfigurationUtil.addSecurityTypeToNetworkId(
                                    networkUpdateResult.getNetworkId(),
                                    newWifiConfiguration.getDefaultSecurityParams()
                                            .getSecurityType()));
                } else {
                    Log.e(TAG, "DPP configuration received, but failed to update network");
                    mDppMetrics.updateDppFailure(EasyConnectStatusCallback
                            .EASY_CONNECT_EVENT_FAILURE_CONFIGURATION);
                    mDppRequestInfo.callback.onFailure(EasyConnectStatusCallback
                            .EASY_CONNECT_EVENT_FAILURE_CONFIGURATION, null, null, new int[0]);
                }
            } else {
                Log.e(TAG, "Unexpected null Wi-Fi configuration object");
            }
        } catch (RemoteException e) {
            Log.e(TAG, "Callback failure");
        }

        // Success, DPP is complete. Clear the DPP session automatically
        cleanupDppResources();
    }

    private void onSuccess(int dppStatusCode) {
        try {
            if (mDppRequestInfo == null) {
                Log.e(TAG, "onSuccess event without a request information object");
                return;
            }

            logd("onSuccess: " + dppStatusCode);
            long now = mClock.getElapsedSinceBootMillis();
            mDppMetrics.updateDppOperationTime((int) (now - mDppRequestInfo.startTime));

            int dppSuccessCode;

            // Convert from HAL codes to WifiManager/user codes
            switch (dppStatusCode) {
                case DppSuccessCode.CONFIGURATION_SENT:
                    mDppMetrics.updateDppR1CapableEnrolleeResponderDevices();
                    dppSuccessCode = EasyConnectStatusCallback
                            .EASY_CONNECT_EVENT_SUCCESS_CONFIGURATION_SENT;
                    break;

                case DppSuccessCode.CONFIGURATION_APPLIED:
                    dppSuccessCode = EasyConnectStatusCallback
                            .EASY_CONNECT_EVENT_SUCCESS_CONFIGURATION_APPLIED;
                    break;

                default:
                    Log.e(TAG, "onSuccess: unknown code " + dppStatusCode);
                    // Success, DPP is complete. Clear the DPP session automatically
                    cleanupDppResources();
                    return;
            }

            mDppMetrics.updateDppConfiguratorSuccess(dppSuccessCode);
            mDppRequestInfo.callback.onSuccess(dppSuccessCode);

        } catch (RemoteException e) {
            Log.e(TAG, "Callback failure");
        }

        // Success, DPP is complete. Clear the DPP session automatically
        cleanupDppResources();
    }

    private void onProgress(int dppStatusCode) {
        try {
            if (mDppRequestInfo == null) {
                Log.e(TAG, "onProgress event without a request information object");
                return;
            }

            logd("onProgress: " + dppStatusCode);

            int dppProgressCode;

            // Convert from HAL codes to WifiManager/user codes
            switch (dppStatusCode) {
                case DppProgressCode.AUTHENTICATION_SUCCESS:
                    dppProgressCode = EasyConnectStatusCallback
                            .EASY_CONNECT_EVENT_PROGRESS_AUTHENTICATION_SUCCESS;
                    break;

                case DppProgressCode.RESPONSE_PENDING:
                    dppProgressCode = EasyConnectStatusCallback
                            .EASY_CONNECT_EVENT_PROGRESS_RESPONSE_PENDING;
                    break;

                case DppProgressCode.CONFIGURATION_SENT_WAITING_RESPONSE:
                    mDppMetrics.updateDppR2CapableEnrolleeResponderDevices();
                    dppProgressCode = EasyConnectStatusCallback
                            .EASY_CONNECT_EVENT_PROGRESS_CONFIGURATION_SENT_WAITING_RESPONSE;
                    break;

                case DppProgressCode.CONFIGURATION_ACCEPTED:
                    dppProgressCode = EasyConnectStatusCallback
                            .EASY_CONNECT_EVENT_PROGRESS_CONFIGURATION_ACCEPTED;
                    break;

                default:
                    Log.e(TAG, "onProgress: unknown code " + dppStatusCode);
                    return;
            }

            mDppRequestInfo.callback.onProgress(dppProgressCode);

        } catch (RemoteException e) {
            Log.e(TAG, "Callback failure");
        }
    }

    private void onFailure(int dppStatusCode) {
        onFailure(dppStatusCode, null, null, null);
    }

    /**
     *
     * This function performs the Enrollee compatibility check with the network.
     * Compatibilty check is done based on the channel match.
     * The logic looks into the scan cache and checks if network's
     * operating channel match with one of the channel in enrollee's scanned channel list.
     *
     * @param ssid Network name.
     * @param channelList contains the list of operating class/channels enrollee used to search for
     *                    the network.
     *                    Reference: DPP spec section: DPP Connection Status Object section.
     *                    (eg for channelList: "81/1,2,3,4,5,6,7,8,9,10,11,117/40,115/48")
     * @return True On compatibility check failures due to error conditions or
     *              when AP is not seen in scan cache or when AP is seen in scan cache and
     *              operating channel is included in enrollee's scanned channel list.
     *         False when network's operating channel is not included in Enrollee's
     *              scanned channel list.
     *
     */
    private boolean isEnrolleeCompatibleWithNetwork(String ssid, String channelList) {
        if (ssid == null || channelList == null) {
            return true;
        }
        SparseArray<int[]> dppChannelList = WifiManager.parseDppChannelList(channelList);

        if (dppChannelList.size() == 0) {
            Log.d(TAG, "No channels found after parsing channel list string");
            return true;
        }

        List<Integer> freqList = new ArrayList<Integer>();

        /* Convert the received operatingClass/channels to list of frequencies */
        for (int i = 0; i < dppChannelList.size(); i++) {
            /* Derive the band corresponding to operating class */
            int operatingClass = dppChannelList.keyAt(i);
            int[] channels = dppChannelList.get(operatingClass);
            int band = ApConfigUtil.getBandFromOperatingClass(operatingClass);
            if (band < 0) {
                Log.e(TAG, "Band corresponding to the operating class: " + operatingClass
                        + " not found in the table");
                continue;
            }
            /* Derive frequency list from channel and band */
            for (int j = 0; j < channels.length; j++) {
                int freq = ApConfigUtil.convertChannelToFrequency(channels[j], band);
                if (freq < 0) {
                    Log.e(TAG, "Invalid frequency after converting channel: " + channels[j]
                            + " band: " + band);
                    continue;
                }
                freqList.add(freq);
            }
        }

        if (freqList.size() == 0) {
            Log.d(TAG, "frequency list is empty");
            return true;
        }

        /* Check the scan cache for the network enrollee tried to find */
        boolean isNetworkInScanCache = false;
        boolean channelMatch = false;
        for (ScanResult scanResult : mScanRequestProxy.getScanResults()) {
            if (!ssid.equals(scanResult.SSID)) {
                continue;
            }
            isNetworkInScanCache = true;
            if (freqList.contains(scanResult.frequency)) {
                channelMatch = true;
                break;
            }
        }

        if (isNetworkInScanCache & !channelMatch) {
            Log.d(TAG, "Optionally update the error code to "
                    + "ENROLLEE_FAILED_TO_SCAN_NETWORK_CHANNEL as enrollee didn't scan"
                    + "network's operating channel");
            mDppMetrics.updateDppR2EnrolleeResponderIncompatibleConfiguration();
            return false;
        }
        return true;
    }

    private @EasyConnectStatusCallback.EasyConnectFailureStatusCode int
            getFailureStatusCodeOnEnrolleeInCompatibleWithNetwork() {
        if (!SdkLevel.isAtLeastS() || mDppRequestInfo.packageName != null
                && mWifiPermissionsUtil.isTargetSdkLessThan(
                mDppRequestInfo.packageName, Build.VERSION_CODES.S,
                mDppRequestInfo.uid)) {
            return EasyConnectStatusCallback
                    .EASY_CONNECT_EVENT_FAILURE_NOT_COMPATIBLE;
        } else {
            return EasyConnectStatusCallback
                    .EASY_CONNECT_EVENT_FAILURE_ENROLLEE_FAILED_TO_SCAN_NETWORK_CHANNEL;
        }
    }

    private void onFailure(int dppStatusCode, String ssid, String channelList, int[] bandList) {
        try {
            if (mDppRequestInfo == null) {
                Log.e(TAG, "onFailure event without a request information object");
                return;
            }

            logd("OnFailure: " + dppStatusCode);

            long now = mClock.getElapsedSinceBootMillis();
            mDppMetrics.updateDppOperationTime((int) (now - mDppRequestInfo.startTime));

            int dppFailureCode;

            // Convert from HAL codes to WifiManager/user codes
            switch (dppStatusCode) {
                case DppFailureCode.INVALID_URI:
                    dppFailureCode =
                            EasyConnectStatusCallback.EASY_CONNECT_EVENT_FAILURE_INVALID_URI;
                    break;

                case DppFailureCode.AUTHENTICATION:
                    dppFailureCode =
                            EasyConnectStatusCallback.EASY_CONNECT_EVENT_FAILURE_AUTHENTICATION;
                    break;

                case DppFailureCode.NOT_COMPATIBLE:
                    dppFailureCode =
                            EasyConnectStatusCallback.EASY_CONNECT_EVENT_FAILURE_NOT_COMPATIBLE;
                    break;

                case DppFailureCode.CONFIGURATION:
                    dppFailureCode =
                            EasyConnectStatusCallback.EASY_CONNECT_EVENT_FAILURE_CONFIGURATION;
                    break;

                case DppFailureCode.BUSY:
                    dppFailureCode = EasyConnectStatusCallback.EASY_CONNECT_EVENT_FAILURE_BUSY;
                    break;

                case DppFailureCode.TIMEOUT:
                    dppFailureCode = EasyConnectStatusCallback.EASY_CONNECT_EVENT_FAILURE_TIMEOUT;
                    break;

                case DppFailureCode.NOT_SUPPORTED:
                    dppFailureCode =
                            EasyConnectStatusCallback.EASY_CONNECT_EVENT_FAILURE_NOT_SUPPORTED;
                    break;

                case DppFailureCode.CANNOT_FIND_NETWORK:
                    // This is the only case where channel list is populated, according to the
                    // DPP spec section 6.3.5.2 DPP Connection Status Object
                    if (isEnrolleeCompatibleWithNetwork(ssid, channelList)) {
                        dppFailureCode =
                                EasyConnectStatusCallback
                                .EASY_CONNECT_EVENT_FAILURE_CANNOT_FIND_NETWORK;
                    } else {
                        dppFailureCode = getFailureStatusCodeOnEnrolleeInCompatibleWithNetwork();
                    }
                    break;

                case DppFailureCode.ENROLLEE_AUTHENTICATION:
                    dppFailureCode = EasyConnectStatusCallback
                            .EASY_CONNECT_EVENT_FAILURE_ENROLLEE_AUTHENTICATION;
                    break;

                case DppFailureCode.CONFIGURATION_REJECTED:
                    dppFailureCode = EasyConnectStatusCallback
                            .EASY_CONNECT_EVENT_FAILURE_ENROLLEE_REJECTED_CONFIGURATION;
                    break;

                case DppFailureCode.URI_GENERATION:
                    if (SdkLevel.isAtLeastS()) {
                        dppFailureCode = EasyConnectStatusCallback
                                .EASY_CONNECT_EVENT_FAILURE_URI_GENERATION;
                    } else {
                        dppFailureCode = EasyConnectStatusCallback
                                .EASY_CONNECT_EVENT_FAILURE_GENERIC;
                    }
                    break;

                case DppFailureCode.FAILURE:
                default:
                    dppFailureCode = EasyConnectStatusCallback.EASY_CONNECT_EVENT_FAILURE_GENERIC;
                    break;
            }

            mDppMetrics.updateDppFailure(dppFailureCode);
            if (bandList == null) {
                bandList = new int[0];
            }
            mDppRequestInfo.callback.onFailure(dppFailureCode, ssid, channelList, bandList);

        } catch (RemoteException e) {
            Log.e(TAG, "Callback failure");
        }

        // All failures are fatal, clear the DPP session
        cleanupDppResources();
    }

    private void logd(String message) {
        if (mVerboseLoggingEnabled) {
            Log.d(TAG, message);
        }
    }

    private boolean linkToDeath(DppRequestInfo dppRequestInfo) {
        // register for binder death
        dppRequestInfo.dr = new IBinder.DeathRecipient() {
            @Override
            public void binderDied() {
                if (dppRequestInfo == null) {
                    return;
                }

                logd("binderDied: uid=" + dppRequestInfo.uid);

                mHandler.post(() -> {
                    cleanupDppResources();
                });
            }
        };

        try {
            dppRequestInfo.binder.linkToDeath(dppRequestInfo.dr, 0);
        } catch (RemoteException e) {
            Log.e(TAG, "Error on linkToDeath - " + e);
            dppRequestInfo.dr = null;
            return false;
        }

        return true;
    }

    @RequiresApi(Build.VERSION_CODES.S)
    private int convertEasyConnectCryptographyCurveToHidlDppCurve(
            @WifiManager.EasyConnectCryptographyCurve int curve) {
        switch (curve) {
            case WifiManager.EASY_CONNECT_CRYPTOGRAPHY_CURVE_SECP384R1:
                return DppCurve.SECP384R1;
            case WifiManager.EASY_CONNECT_CRYPTOGRAPHY_CURVE_SECP521R1:
                return DppCurve.SECP521R1;
            case WifiManager.EASY_CONNECT_CRYPTOGRAPHY_CURVE_BRAINPOOLP256R1:
                return DppCurve.BRAINPOOLP256R1;
            case WifiManager.EASY_CONNECT_CRYPTOGRAPHY_CURVE_BRAINPOOLP384R1:
                return DppCurve.BRAINPOOLP384R1;
            case WifiManager.EASY_CONNECT_CRYPTOGRAPHY_CURVE_BRAINPOOLP512R1:
                return DppCurve.BRAINPOOLP512R1;
            case WifiManager.EASY_CONNECT_CRYPTOGRAPHY_CURVE_PRIME256V1:
            default:
                return DppCurve.PRIME256V1;
        }
    }
}
