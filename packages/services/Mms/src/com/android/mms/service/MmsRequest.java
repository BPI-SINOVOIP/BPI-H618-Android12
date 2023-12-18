/*
 * Copyright (C) 2014 The Android Open Source Project
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

package com.android.mms.service;

import android.app.Activity;
import android.app.PendingIntent;
import android.content.Context;
import android.content.Intent;
import android.net.Uri;
import android.net.wifi.WifiInfo;
import android.net.wifi.WifiManager;
import android.os.Bundle;
import android.service.carrier.CarrierMessagingService;
import android.service.carrier.CarrierMessagingServiceWrapper.CarrierMessagingCallback;
import android.telephony.AnomalyReporter;
import android.telephony.PhoneStateListener;
import android.telephony.PreciseDataConnectionState;
import android.telephony.TelephonyCallback;
import android.telephony.data.ApnSetting;
import android.telephony.ims.feature.MmTelFeature;
import android.telephony.ims.ImsMmTelManager;
import android.telephony.ims.stub.ImsRegistrationImplBase;
import android.telephony.SmsManager;
import android.telephony.TelephonyManager;
import android.text.TextUtils;

import com.android.mms.service.exception.ApnException;
import com.android.mms.service.exception.MmsHttpException;
import com.android.mms.service.exception.MmsNetworkException;

import java.util.UUID;

/**
 * Base class for MMS requests. This has the common logic of sending/downloading MMS.
 */
public abstract class MmsRequest {
    private static final int RETRY_TIMES = 3;
    // Signal level threshold for both wifi and cellular
    private static final int SIGNAL_LEVEL_THRESHOLD = 2;
    // MMS anomaly uuid
    private final UUID mAnomalyUUID = UUID.fromString("e4330975-17be-43b7-87d6-d9f281d33278");
    public static final String EXTRA_LAST_CONNECTION_FAILURE_CAUSE_CODE
            = "android.telephony.extra.LAST_CONNECTION_FAILURE_CAUSE_CODE";
    public static final String EXTRA_HANDLED_BY_CARRIER_APP
            = "android.telephony.extra.HANDLED_BY_CARRIER_APP";

    /**
     * Interface for certain functionalities from MmsService
     */
    public static interface RequestManager {
        /**
         * Enqueue an MMS request
         *
         * @param request the request to enqueue
         */
        public void addSimRequest(MmsRequest request);

        /*
         * @return Whether to auto persist received MMS
         */
        public boolean getAutoPersistingPref();

        /**
         * Read pdu (up to maxSize bytes) from supplied content uri
         * @param contentUri content uri from which to read
         * @param maxSize maximum number of bytes to read
         * @return read pdu (else null in case of error or too big)
         */
        public byte[] readPduFromContentUri(final Uri contentUri, final int maxSize);

        /**
         * Write pdu to supplied content uri
         * @param contentUri content uri to which bytes should be written
         * @param pdu pdu bytes to write
         * @return true in case of success (else false)
         */
        public boolean writePduToContentUri(final Uri contentUri, final byte[] pdu);
    }

    // The reference to the pending requests manager (i.e. the MmsService)
    protected RequestManager mRequestManager;
    // The SIM id
    protected int mSubId;
    // The creator app
    protected String mCreator;
    // MMS config
    protected Bundle mMmsConfig;
    // MMS config overrides that will be applied to mMmsConfig when we eventually load it.
    protected Bundle mMmsConfigOverrides;
    // Context used to get TelephonyManager.
    protected Context mContext;
    protected long mMessageId;
    protected int mLastConnectionFailure;

    class MonitorTelephonyCallback extends TelephonyCallback implements
            TelephonyCallback.PreciseDataConnectionStateListener {
        @Override
        public void onPreciseDataConnectionStateChanged(
                PreciseDataConnectionState connectionState) {
            if (connectionState == null) {
                return;
            }
            ApnSetting apnSetting = connectionState.getApnSetting();
            int apnTypes = apnSetting.getApnTypeBitmask();
            if ((apnTypes & ApnSetting.TYPE_MMS) != 0) {
                mLastConnectionFailure = connectionState.getLastCauseCode();
                LogUtil.d("onPreciseDataConnectionStateChanged mLastConnectionFailure: "
                        + mLastConnectionFailure);
            }
        }
    }

    public MmsRequest(RequestManager requestManager, int subId, String creator,
            Bundle configOverrides, Context context, long messageId) {
        mRequestManager = requestManager;
        mSubId = subId;
        mCreator = creator;
        mMmsConfigOverrides = configOverrides;
        mMmsConfig = null;
        mContext = context;
        mMessageId = messageId;
    }

    public int getSubId() {
        return mSubId;
    }

    private boolean ensureMmsConfigLoaded() {
        if (mMmsConfig == null) {
            // Not yet retrieved from mms config manager. Try getting it.
            final Bundle config = MmsConfigManager.getInstance().getMmsConfigBySubId(mSubId);
            if (config != null) {
                mMmsConfig = config;
                // TODO: Make MmsConfigManager authoritative for user agent and don't consult
                // TelephonyManager.
                final TelephonyManager telephonyManager = ((TelephonyManager) mContext
                        .getSystemService(Context.TELEPHONY_SERVICE))
                        .createForSubscriptionId(mSubId);
                final String userAgent = telephonyManager.getMmsUserAgent();
                if (!TextUtils.isEmpty(userAgent)) {
                    config.putString(SmsManager.MMS_CONFIG_USER_AGENT, userAgent);
                }
                final String userAgentProfileUrl = telephonyManager.getMmsUAProfUrl();
                if (!TextUtils.isEmpty(userAgentProfileUrl)) {
                    config.putString(SmsManager.MMS_CONFIG_UA_PROF_URL, userAgentProfileUrl);
                }
                // Apply overrides
                if (mMmsConfigOverrides != null) {
                    mMmsConfig.putAll(mMmsConfigOverrides);
                }
            }
        }
        return mMmsConfig != null;
    }

    /**
     * Execute the request
     *
     * @param context The context
     * @param networkManager The network manager to use
     */
    public void execute(Context context, MmsNetworkManager networkManager) {
        final String requestId = this.getRequestId();
        LogUtil.i(requestId, "Executing...");
        int result = SmsManager.MMS_ERROR_UNSPECIFIED;
        int httpStatusCode = 0;
        byte[] response = null;
        // TODO: add mms data channel check back to fast fail if no way to send mms,
        // when telephony provides such API.
        if (!ensureMmsConfigLoaded()) { // Check mms config
            LogUtil.e(requestId, "mms config is not loaded yet");
            result = SmsManager.MMS_ERROR_CONFIGURATION_ERROR;
        } else if (!prepareForHttpRequest()) { // Prepare request, like reading pdu data from user
            LogUtil.e(requestId, "Failed to prepare for request");
            result = SmsManager.MMS_ERROR_IO_ERROR;
        } else { // Execute
            long retryDelaySecs = 2;
            // Try multiple times of MMS HTTP request, depending on the error.
            for (int i = 0; i < RETRY_TIMES; i++) {
                httpStatusCode = 0; // Clear for retry.
                MonitorTelephonyCallback connectionStateCallback = new MonitorTelephonyCallback();
                try {
                    listenToDataConnectionState(connectionStateCallback);
                    networkManager.acquireNetwork(requestId);
                    final String apnName = networkManager.getApnName();
                    LogUtil.d(requestId, "APN name is " + apnName);
                    try {
                        ApnSettings apn = null;
                        try {
                            apn = ApnSettings.load(context, apnName, mSubId, requestId);
                        } catch (ApnException e) {
                            // If no APN could be found, fall back to trying without the APN name
                            if (apnName == null) {
                                // If the APN name was already null then don't need to retry
                                throw (e);
                            }
                            LogUtil.i(requestId, "No match with APN name: "
                                    + apnName + ", try with no name");
                            apn = ApnSettings.load(context, null, mSubId, requestId);
                        }
                        LogUtil.i(requestId, "Using " + apn.toString());
                        response = doHttp(context, networkManager, apn);
                        result = Activity.RESULT_OK;
                        // Success
                        break;
                    } finally {
                        // Release the MMS network immediately except successful DownloadRequest.
                        networkManager.releaseNetwork(requestId,
                                this instanceof DownloadRequest && result == Activity.RESULT_OK);
                    }
                } catch (ApnException e) {
                    LogUtil.e(requestId, "APN failure", e);
                    result = SmsManager.MMS_ERROR_INVALID_APN;
                    break;
                } catch (MmsNetworkException e) {
                    LogUtil.e(requestId, "MMS network acquiring failure", e);
                    result = SmsManager.MMS_ERROR_UNABLE_CONNECT_MMS;
                    break;
                } catch (MmsHttpException e) {
                    LogUtil.e(requestId, "HTTP or network I/O failure", e);
                    result = SmsManager.MMS_ERROR_HTTP_FAILURE;
                    httpStatusCode = e.getStatusCode();
                    // Retry
                } catch (Exception e) {
                    LogUtil.e(requestId, "Unexpected failure", e);
                    result = SmsManager.MMS_ERROR_UNSPECIFIED;
                    break;
                } finally {
                    stopListeningToDataConnectionState(connectionStateCallback);
                }
                try {
                    Thread.sleep(retryDelaySecs * 1000, 0/*nano*/);
                } catch (InterruptedException e) {}
                retryDelaySecs <<= 1;
            }
        }
        processResult(context, result, response, httpStatusCode, /* handledByCarrierApp= */ false);
    }

    private void listenToDataConnectionState(MonitorTelephonyCallback connectionStateCallback) {
        final TelephonyManager telephonyManager = mContext.getSystemService(
                TelephonyManager.class).createForSubscriptionId(mSubId);
        telephonyManager.registerTelephonyCallback(r -> r.run(), connectionStateCallback);
    }

    private void stopListeningToDataConnectionState(
            MonitorTelephonyCallback connectionStateCallback) {
        final TelephonyManager telephonyManager = mContext.getSystemService(
                TelephonyManager.class).createForSubscriptionId(mSubId);
        telephonyManager.unregisterTelephonyCallback(connectionStateCallback);
    }

    /**
     * Process the result of the completed request, including updating the message status
     * in database and sending back the result via pending intents.
     * @param context The context
     * @param result The result code of execution
     * @param response The response body
     * @param httpStatusCode The optional http status code in case of http failure
     * @param handledByCarrierApp True if the sending/downloading was handled by a carrier app
     *                            rather than MmsService.
     */
    public void processResult(Context context, int result, byte[] response, int httpStatusCode,
            boolean handledByCarrierApp) {
        final Uri messageUri = persistIfRequired(context, result, response);

        final String requestId = this.getRequestId();
        // As noted in the @param comment above, the httpStatusCode is only set when there's
        // an http failure. On success, such as an http code of 200, the value here will be 0.
        // "httpStatusCode: xxx" is now reported for an http failure only.
        LogUtil.i(requestId, "processResult: "
                + (result == Activity.RESULT_OK ? "success" : "failure(" + result + ")")
                + (httpStatusCode != 0 ? ", httpStatusCode: " + httpStatusCode : "")
                + " handledByCarrierApp: " + handledByCarrierApp
                + " mLastConnectionFailure: " + mLastConnectionFailure);

        // Return MMS HTTP request result via PendingIntent
        final PendingIntent pendingIntent = getPendingIntent();
        if (pendingIntent != null) {
            boolean succeeded = true;
            // Extra information to send back with the pending intent
            Intent fillIn = new Intent();
            if (response != null) {
                succeeded = transferResponse(fillIn, response);
            }
            if (messageUri != null) {
                fillIn.putExtra("uri", messageUri.toString());
            }
            if (result == SmsManager.MMS_ERROR_HTTP_FAILURE && httpStatusCode != 0) {
                fillIn.putExtra(SmsManager.EXTRA_MMS_HTTP_STATUS, httpStatusCode);
            }
            fillIn.putExtra(EXTRA_LAST_CONNECTION_FAILURE_CAUSE_CODE,
                    mLastConnectionFailure);
            fillIn.putExtra(EXTRA_HANDLED_BY_CARRIER_APP, handledByCarrierApp);
            try {
                if (!succeeded) {
                    result = SmsManager.MMS_ERROR_IO_ERROR;
                }
                reportPossibleAnomaly(result, httpStatusCode);
                pendingIntent.send(context, result, fillIn);
            } catch (PendingIntent.CanceledException e) {
                LogUtil.e(requestId, "Sending pending intent canceled", e);
            }
        }

        revokeUriPermission(context);
    }

    private void reportPossibleAnomaly(int result, int httpStatusCode) {
        switch (result) {
            case SmsManager.MMS_ERROR_HTTP_FAILURE:
                if (isPoorSignal()) {
                    LogUtil.i(this.toString(), "Poor Signal");
                    break;
                }
            case SmsManager.MMS_ERROR_INVALID_APN:
            case SmsManager.MMS_ERROR_UNABLE_CONNECT_MMS:
            case SmsManager.MMS_ERROR_UNSPECIFIED:
            case SmsManager.MMS_ERROR_IO_ERROR:
                String message = "MMS failed";
                LogUtil.i(this.toString(),
                        message + " with error: " + result + " httpStatus:" + httpStatusCode);
                AnomalyReporter.reportAnomaly(generateUUID(result, httpStatusCode), message);
                break;
            default:
                break;
        }
    }

    private UUID generateUUID(int result, int httpStatusCode) {
        long lresult = result;
        long lhttpStatusCode = httpStatusCode;
        return new UUID(mAnomalyUUID.getMostSignificantBits(),
                mAnomalyUUID.getLeastSignificantBits() + ((lhttpStatusCode << 32) + lresult));
    }

    private boolean isPoorSignal() {
        // Check Wifi signal strength when IMS registers via Wifi
        if (isImsOnWifi()) {
            int rssi = 0;
            WifiManager wifiManager = mContext.getSystemService(WifiManager.class);
            final WifiInfo wifiInfo = wifiManager.getConnectionInfo();
            if (wifiInfo != null) {
                rssi = wifiInfo.getRssi();
            } else {
                return false;
            }
            final int wifiLevel = wifiManager.calculateSignalLevel(rssi);
            LogUtil.d(this.toString(), "Wifi signal rssi: " + rssi + " level:" + wifiLevel);
            if (wifiLevel <= SIGNAL_LEVEL_THRESHOLD) {
                return true;
            }
            return false;
        } else {
            // Check cellular signal strength
            final TelephonyManager telephonyManager = mContext.getSystemService(
                    TelephonyManager.class).createForSubscriptionId(mSubId);
            final int cellLevel = telephonyManager.getSignalStrength().getLevel();
            LogUtil.d(this.toString(), "Cellular signal level:" + cellLevel);
            if (cellLevel <= SIGNAL_LEVEL_THRESHOLD) {
                return true;
            }
            return false;
        }
    }

    private boolean isImsOnWifi() {
        ImsMmTelManager imsManager;
        try {
            imsManager = ImsMmTelManager.createForSubscriptionId(mSubId);
        } catch (IllegalArgumentException e) {
            LogUtil.e(this.toString(), "invalid subid:" + mSubId);
            return false;
        }
        if (imsManager != null) {
            return imsManager.isAvailable(MmTelFeature.MmTelCapabilities.CAPABILITY_TYPE_VOICE,
                    ImsRegistrationImplBase.REGISTRATION_TECH_IWLAN);
        } else {
            return false;
        }
    }

    /**
     * Returns true if sending / downloading using the carrier app has failed and completes the
     * action using platform API's, otherwise false.
     */
    protected boolean maybeFallbackToRegularDelivery(int carrierMessagingAppResult) {
        if (carrierMessagingAppResult
                == CarrierMessagingService.SEND_STATUS_RETRY_ON_CARRIER_NETWORK
                || carrierMessagingAppResult
                        == CarrierMessagingService.DOWNLOAD_STATUS_RETRY_ON_CARRIER_NETWORK) {
            LogUtil.d(this.toString(), "Sending/downloading MMS by IP failed. "
                    + MmsService.formatCrossStackMessageId(mMessageId));
            mRequestManager.addSimRequest(MmsRequest.this);
            return true;
        } else {
            return false;
        }
    }

    /**
     * Converts from {@code carrierMessagingAppResult} to a platform result code.
     */
    protected static int toSmsManagerResult(int carrierMessagingAppResult) {
        switch (carrierMessagingAppResult) {
            case CarrierMessagingService.SEND_STATUS_OK:
                return Activity.RESULT_OK;
            case CarrierMessagingService.SEND_STATUS_RETRY_ON_CARRIER_NETWORK:
                return SmsManager.MMS_ERROR_RETRY;
            default:
                return SmsManager.MMS_ERROR_UNSPECIFIED;
        }
    }

    @Override
    public String toString() {
        return getClass().getSimpleName() + '@' + Integer.toHexString(hashCode())
                + " " + MmsService.formatCrossStackMessageId(mMessageId);
    }

    protected String getRequestId() {
        return this.toString();
    }

    /**
     * Making the HTTP request to MMSC
     *
     * @param context The context
     * @param netMgr The current {@link MmsNetworkManager}
     * @param apn The APN setting
     * @return The HTTP response data
     * @throws MmsHttpException If any network error happens
     */
    protected abstract byte[] doHttp(Context context, MmsNetworkManager netMgr, ApnSettings apn)
            throws MmsHttpException;

    /**
     * @return The PendingIntent associate with the MMS sending invocation
     */
    protected abstract PendingIntent getPendingIntent();

    /**
     * @return The queue should be used by this request, 0 is sending and 1 is downloading
     */
    protected abstract int getQueueType();

    /**
     * Persist message into telephony if required (i.e. when auto-persisting is on or
     * the calling app is non-default sms app for sending)
     *
     * @param context The context
     * @param result The result code of execution
     * @param response The response body
     * @return The persisted URI of the message or null if we don't persist or fail
     */
    protected abstract Uri persistIfRequired(Context context, int result, byte[] response);

    /**
     * Prepare to make the HTTP request - will download message for sending
     * @return true if preparation succeeds (and request can proceed) else false
     */
    protected abstract boolean prepareForHttpRequest();

    /**
     * Transfer the received response to the caller
     *
     * @param fillIn the intent that will be returned to the caller
     * @param response the pdu to transfer
     * @return true if response transfer succeeds else false
     */
    protected abstract boolean transferResponse(Intent fillIn, byte[] response);

    /**
     * Revoke the content URI permission granted by the MMS app to the phone package.
     *
     * @param context The context
     */
    protected abstract void revokeUriPermission(Context context);

    /**
     * Base class for handling carrier app send / download result.
     */
    protected abstract class CarrierMmsActionCallback implements CarrierMessagingCallback {
        @Override
        public void onSendSmsComplete(int result, int messageRef) {
            LogUtil.e("Unexpected onSendSmsComplete call for "
                    + MmsService.formatCrossStackMessageId(mMessageId)
                    + " with result: " + result);
        }

        @Override
        public void onSendMultipartSmsComplete(int result, int[] messageRefs) {
            LogUtil.e("Unexpected onSendMultipartSmsComplete call for "
                    + MmsService.formatCrossStackMessageId(mMessageId)
                    + " with result: " + result);
        }

        @Override
        public void onReceiveSmsComplete(int result) {
            LogUtil.e("Unexpected onFilterComplete call for "
                    + MmsService.formatCrossStackMessageId(mMessageId)
                    + " with result: " + result);
        }
    }
}
