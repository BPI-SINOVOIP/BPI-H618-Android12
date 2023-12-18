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
 * limitations under the License.
 */

package com.android.server.wifi;

import static android.Manifest.permission.NETWORK_SETTINGS;
import static android.telephony.TelephonyManager.DATA_ENABLED_REASON_USER;

import android.annotation.IntDef;
import android.annotation.NonNull;
import android.app.AlertDialog;
import android.app.Notification;
import android.app.PendingIntent;
import android.content.BroadcastReceiver;
import android.content.Context;
import android.content.Intent;
import android.content.IntentFilter;
import android.content.pm.PackageInfo;
import android.content.pm.PackageManager;
import android.content.res.Resources;
import android.database.ContentObserver;
import android.graphics.drawable.Icon;
import android.net.Uri;
import android.net.wifi.WifiConfiguration;
import android.net.wifi.WifiEnterpriseConfig;
import android.net.wifi.WifiInfo;
import android.net.wifi.WifiManager;
import android.net.wifi.hotspot2.PasspointConfiguration;
import android.net.wifi.hotspot2.pps.Credential;
import android.os.Build;
import android.os.Handler;
import android.os.HandlerExecutor;
import android.os.PersistableBundle;
import android.os.UserHandle;
import android.telephony.CarrierConfigManager;
import android.telephony.ImsiEncryptionInfo;
import android.telephony.SubscriptionInfo;
import android.telephony.SubscriptionManager;
import android.telephony.TelephonyCallback;
import android.telephony.TelephonyManager;
import android.text.TextUtils;
import android.util.Base64;
import android.util.Log;
import android.util.Pair;
import android.util.SparseArray;
import android.util.SparseBooleanArray;
import android.view.WindowManager;

import androidx.annotation.RequiresApi;

import com.android.internal.annotations.VisibleForTesting;
import com.android.internal.messages.nano.SystemMessageProto.SystemMessage;
import com.android.modules.utils.build.SdkLevel;
import com.android.wifi.resources.R;

import java.io.FileDescriptor;
import java.io.PrintWriter;
import java.lang.annotation.Retention;
import java.lang.annotation.RetentionPolicy;
import java.security.InvalidKeyException;
import java.security.NoSuchAlgorithmException;
import java.security.PublicKey;
import java.util.ArrayList;
import java.util.HashMap;
import java.util.List;
import java.util.Map;

import javax.annotation.Nullable;
import javax.crypto.BadPaddingException;
import javax.crypto.Cipher;
import javax.crypto.IllegalBlockSizeException;
import javax.crypto.NoSuchPaddingException;

/**
 * This class provide APIs to get carrier info from telephony service.
 * TODO(b/132188983): Refactor into TelephonyFacade which owns all instances of
 *  TelephonyManager/SubscriptionManager in Wifi
 */
public class WifiCarrierInfoManager {
    public static final String TAG = "WifiCarrierInfoManager";
    public static final String DEFAULT_EAP_PREFIX = "\0";

    public static final int CARRIER_INVALID_TYPE = -1;
    public static final int CARRIER_MNO_TYPE = 0; // Mobile Network Operator
    public static final int CARRIER_MVNO_TYPE = 1; // Mobile Virtual Network Operator
    public static final String ANONYMOUS_IDENTITY = "anonymous";
    public static final String THREE_GPP_NAI_REALM_FORMAT = "wlan.mnc%s.mcc%s.3gppnetwork.org";
    /** Intent when user tapped action button to allow the app. */
    @VisibleForTesting
    public static final String NOTIFICATION_USER_ALLOWED_CARRIER_INTENT_ACTION =
            "com.android.server.wifi.action.CarrierNetwork.USER_ALLOWED_CARRIER";
    /** Intent when user tapped action button to disallow the app. */
    @VisibleForTesting
    public static final String NOTIFICATION_USER_DISALLOWED_CARRIER_INTENT_ACTION =
            "com.android.server.wifi.action.CarrierNetwork.USER_DISALLOWED_CARRIER";
    /** Intent when user dismissed the notification. */
    @VisibleForTesting
    public static final String NOTIFICATION_USER_DISMISSED_INTENT_ACTION =
            "com.android.server.wifi.action.CarrierNetwork.USER_DISMISSED";
    /** Intent when user clicked on the notification. */
    @VisibleForTesting
    public static final String NOTIFICATION_USER_CLICKED_INTENT_ACTION =
            "com.android.server.wifi.action.CarrierNetwork.USER_CLICKED";
    @VisibleForTesting
    public static final String EXTRA_CARRIER_NAME =
            "com.android.server.wifi.extra.CarrierNetwork.CARRIER_NAME";
    @VisibleForTesting
    public static final String EXTRA_CARRIER_ID =
            "com.android.server.wifi.extra.CarrierNetwork.CARRIER_ID";

    // IMSI encryption method: RSA-OAEP with SHA-256 hash function
    private static final String IMSI_CIPHER_TRANSFORMATION =
            "RSA/ECB/OAEPwithSHA-256andMGF1Padding";

    private static final HashMap<Integer, String> EAP_METHOD_PREFIX = new HashMap<>();
    static {
        EAP_METHOD_PREFIX.put(WifiEnterpriseConfig.Eap.AKA, "0");
        EAP_METHOD_PREFIX.put(WifiEnterpriseConfig.Eap.SIM, "1");
        EAP_METHOD_PREFIX.put(WifiEnterpriseConfig.Eap.AKA_PRIME, "6");
    }

    public static final int ACTION_USER_ALLOWED_CARRIER = 1;
    public static final int ACTION_USER_DISALLOWED_CARRIER = 2;
    public static final int ACTION_USER_DISMISS = 3;

    @IntDef(prefix = { "ACTION_USER_" }, value = {
            ACTION_USER_ALLOWED_CARRIER,
            ACTION_USER_DISALLOWED_CARRIER,
            ACTION_USER_DISMISS
    })
    @Retention(RetentionPolicy.SOURCE)
    public @interface UserActionCode { }

    /**
     * 3GPP TS 11.11  2G_authentication command/response
     *                Input: [RAND]
     *                Output: [SRES][Cipher Key Kc]
     */
    private static final int START_SRES_POS = 0; // MUST be 0
    private static final int SRES_LEN = 4;
    private static final int START_KC_POS = START_SRES_POS + SRES_LEN;
    private static final int KC_LEN = 8;

    private static final Uri CONTENT_URI = Uri.parse("content://carrier_information/carrier");
    /**
     * Expiration timeout for user notification in milliseconds. (15 min)
     */
    private static final long NOTIFICATION_EXPIRY_MILLS = 15 * 60 * 1000;
    /**
     * Notification update delay in milliseconds. (10 min)
     */
    private static final long NOTIFICATION_UPDATE_DELAY_MILLS = 10 * 60 * 1000;

    private final WifiContext mContext;
    private final Handler mHandler;
    private final WifiInjector mWifiInjector;
    private final Resources mResources;
    private final TelephonyManager mTelephonyManager;
    private final SubscriptionManager mSubscriptionManager;
    private final WifiNotificationManager mNotificationManager;
    private final WifiMetrics mWifiMetrics;
    private final Clock mClock;
    /**
     * Cached Map of <subscription ID, CarrierConfig PersistableBundle> since retrieving the
     * PersistableBundle from CarrierConfigManager is somewhat expensive as it has hundreds of
     * fields. This cache is cleared when the CarrierConfig changes to ensure data freshness.
     */
    private final SparseArray<PersistableBundle> mCachedCarrierConfigPerSubId = new SparseArray<>();

    /**
     * Intent filter for processing notification actions.
     */
    private final IntentFilter mIntentFilter;
    private final FrameworkFacade mFrameworkFacade;

    private boolean mVerboseLogEnabled = false;
    private SparseBooleanArray mImsiEncryptionInfoAvailable = new SparseBooleanArray();
    private final Map<Integer, Boolean> mImsiPrivacyProtectionExemptionMap = new HashMap<>();
    private final Map<Integer, Boolean> mMergedCarrierNetworkOffloadMap = new HashMap<>();
    private final Map<Integer, Boolean> mUnmergedCarrierNetworkOffloadMap = new HashMap<>();
    private final List<OnUserApproveCarrierListener> mOnUserApproveCarrierListeners =
            new ArrayList<>();
    private final SparseBooleanArray mUserDataEnabled = new SparseBooleanArray();
    private final List<UserDataEnabledChangedListener>  mUserDataEnabledListenerList =
            new ArrayList<>();
    private final List<OnCarrierOffloadDisabledListener> mOnCarrierOffloadDisabledListeners =
            new ArrayList<>();
    private final SparseArray<SimInfo> mSubIdToSimInfoSparseArray = new SparseArray<>();

    private List<SubscriptionInfo> mActiveSubInfos = null;

    private boolean mHasNewUserDataToSerialize = false;
    private boolean mHasNewSharedDataToSerialize = false;
    private boolean mUserDataLoaded = false;
    private boolean mIsLastUserApprovalUiDialog = false;
    private CarrierConfigManager mCarrierConfigManager;
    /**
     * The {@link Clock#getElapsedSinceBootMillis()} must be at least this value for us
     * to update/show the notification.
     */
    private long mNotificationUpdateTime = 0L;

    private static class SimInfo {
        public final String imsi;
        public final String mccMnc;
        public final int carrierIdFromSimMccMnc;
        public final int simCarrierId;

        SimInfo(String imsi, String mccMnc, int carrierIdFromSimMccMnc, int simCarrierId) {
            this.imsi = imsi;
            this.mccMnc = mccMnc;
            this.carrierIdFromSimMccMnc = carrierIdFromSimMccMnc;
            this.simCarrierId = simCarrierId;
        }

        /**
         * Get the carrier type of current SIM.
         */
        public int getCarrierType() {
            if (carrierIdFromSimMccMnc == simCarrierId) {
                return CARRIER_MNO_TYPE;
            }
            return CARRIER_MVNO_TYPE;
        }
        @Override
        public String toString() {
            StringBuilder sb = new StringBuilder("SimInfo[ ")
                    .append("IMSI=").append(imsi)
                    .append(", MCCMNC=").append(mccMnc)
                    .append(", carrierIdFromSimMccMnc=").append(carrierIdFromSimMccMnc)
                    .append(", simCarrierId=").append(simCarrierId)
                    .append(" ]");
            return sb.toString();
        }
    }

    /**
     * Implement of {@link TelephonyCallback.DataEnabledListener}
     */
    @VisibleForTesting
    @RequiresApi(Build.VERSION_CODES.S)
    public final class UserDataEnabledChangedListener extends TelephonyCallback implements
            TelephonyCallback.DataEnabledListener {
        private final int mSubscriptionId;

        public UserDataEnabledChangedListener(int subscriptionId) {
            mSubscriptionId = subscriptionId;
        }

        @Override
        public void onDataEnabledChanged(boolean enabled, int reason) {
            if (reason == DATA_ENABLED_REASON_USER) {
                Log.d(TAG, "Mobile data change by user to "
                        + (enabled ? "enabled" : "disabled") + " for subId: " + mSubscriptionId);
                mUserDataEnabled.put(mSubscriptionId, enabled);
                if (!enabled) {
                    for (OnCarrierOffloadDisabledListener listener :
                            mOnCarrierOffloadDisabledListeners) {
                        listener.onCarrierOffloadDisabled(mSubscriptionId, true);
                    }
                }
            }
        }

        /**
         * Unregister the listener from TelephonyManager,
         */
        public void unregisterListener() {
            mTelephonyManager.createForSubscriptionId(mSubscriptionId)
                    .unregisterTelephonyCallback(this);

        }
    }

    /**
     * Interface for other modules to listen to the user approve IMSI protection exemption.
     */
    public interface OnUserApproveCarrierListener {

        /**
         * Invoke when user approve the IMSI protection exemption.
         */
        void onUserAllowed(int carrierId);
    }

    /**
     * Interface for other modules to listen to the carrier network offload disabled.
     */
    public interface OnCarrierOffloadDisabledListener {

        /**
         * Invoke when carrier offload on target subscriptionId is disabled.
         */
        void onCarrierOffloadDisabled(int subscriptionId, boolean merged);
    }

    /**
     * Module to interact with the wifi config store.
     */
    private class WifiCarrierInfoStoreManagerDataSource implements
            WifiCarrierInfoStoreManagerData.DataSource {

        @Override
        public Map<Integer, Boolean> toSerializeMergedCarrierNetworkOffloadMap() {
            return mMergedCarrierNetworkOffloadMap;
        }

        @Override
        public Map<Integer, Boolean> toSerializeUnmergedCarrierNetworkOffloadMap() {
            return mUnmergedCarrierNetworkOffloadMap;
        }

        @Override
        public void serializeComplete() {
            mHasNewSharedDataToSerialize = false;
        }


        @Override
        public void fromMergedCarrierNetworkOffloadMapDeserialized(
                Map<Integer, Boolean> carrierOffloadMap) {
            mMergedCarrierNetworkOffloadMap.clear();
            mMergedCarrierNetworkOffloadMap.putAll(carrierOffloadMap);
        }

        @Override
        public void fromUnmergedCarrierNetworkOffloadMapDeserialized(
                Map<Integer, Boolean> subscriptionOffloadMap) {
            mUnmergedCarrierNetworkOffloadMap.clear();
            mUnmergedCarrierNetworkOffloadMap.putAll(subscriptionOffloadMap);
        }

        @Override
        public void reset() {
            mMergedCarrierNetworkOffloadMap.clear();
            mUnmergedCarrierNetworkOffloadMap.clear();
        }

        @Override
        public boolean hasNewDataToSerialize() {
            return mHasNewSharedDataToSerialize;
        }
    }

    /**
     * Module to interact with the wifi config store.
     */
    private class ImsiProtectionExemptionDataSource implements
            ImsiPrivacyProtectionExemptionStoreData.DataSource {
        @Override
        public Map<Integer, Boolean> toSerialize() {
            // Clear the flag after writing to disk.
            mHasNewUserDataToSerialize = false;
            return mImsiPrivacyProtectionExemptionMap;
        }

        @Override
        public void fromDeserialized(Map<Integer, Boolean> imsiProtectionExemptionMap) {
            mImsiPrivacyProtectionExemptionMap.clear();
            mImsiPrivacyProtectionExemptionMap.putAll(imsiProtectionExemptionMap);
            mUserDataLoaded = true;
        }

        @Override
        public void reset() {
            mUserDataLoaded = false;
            mImsiPrivacyProtectionExemptionMap.clear();
        }

        @Override
        public boolean hasNewDataToSerialize() {
            return mHasNewUserDataToSerialize;
        }
    }

    private final BroadcastReceiver mBroadcastReceiver =
            new BroadcastReceiver() {
                @Override
                public void onReceive(Context context, Intent intent) {
                    String carrierName = intent.getStringExtra(EXTRA_CARRIER_NAME);
                    int carrierId = intent.getIntExtra(EXTRA_CARRIER_ID, -1);
                    if (carrierName == null || carrierId == -1) {
                        Log.e(TAG, "No carrier name or carrier id found in intent");
                        return;
                    }

                    switch (intent.getAction()) {
                        case NOTIFICATION_USER_ALLOWED_CARRIER_INTENT_ACTION:
                            handleUserAllowCarrierExemptionAction(carrierName, carrierId);
                            break;
                        case NOTIFICATION_USER_DISALLOWED_CARRIER_INTENT_ACTION:
                            handleUserDisallowCarrierExemptionAction(carrierName, carrierId);
                            break;
                        case NOTIFICATION_USER_CLICKED_INTENT_ACTION:
                            sendImsiPrivacyConfirmationDialog(carrierName, carrierId);
                            // Collapse the notification bar
                            mContext.sendBroadcast(new Intent(Intent.ACTION_CLOSE_SYSTEM_DIALOGS));
                            break;
                        case NOTIFICATION_USER_DISMISSED_INTENT_ACTION:
                            handleUserDismissAction();
                            return; // no need to cancel a dismissed notification, return.
                        default:
                            Log.e(TAG, "Unknown action " + intent.getAction());
                            return;
                    }
                    // Clear notification once the user interacts with it.
                    mNotificationManager.cancel(SystemMessage.NOTE_CARRIER_SUGGESTION_AVAILABLE);
                }
            };
    private void handleUserDismissAction() {
        Log.i(TAG, "User dismissed the notification");
        mNotificationUpdateTime = 0;
        mWifiMetrics.addUserApprovalCarrierUiReaction(ACTION_USER_DISMISS,
                mIsLastUserApprovalUiDialog);
    }

    private void handleUserAllowCarrierExemptionAction(String carrierName, int carrierId) {
        Log.i(TAG, "User clicked to allow carrier:" + carrierName);
        setHasUserApprovedImsiPrivacyExemptionForCarrier(true, carrierId);
        mNotificationUpdateTime = 0;
        mWifiMetrics.addUserApprovalCarrierUiReaction(ACTION_USER_ALLOWED_CARRIER,
                mIsLastUserApprovalUiDialog);

    }

    private void handleUserDisallowCarrierExemptionAction(String carrierName, int carrierId) {
        Log.i(TAG, "User clicked to disallow carrier:" + carrierName);
        setHasUserApprovedImsiPrivacyExemptionForCarrier(false, carrierId);
        mNotificationUpdateTime = 0;
        mWifiMetrics.addUserApprovalCarrierUiReaction(
                ACTION_USER_DISALLOWED_CARRIER, mIsLastUserApprovalUiDialog);
    }

    private class SubscriptionChangeListener extends
            SubscriptionManager.OnSubscriptionsChangedListener {
        @Override
        public void onSubscriptionsChanged() {
            mActiveSubInfos = mSubscriptionManager.getActiveSubscriptionInfoList();
            mSubIdToSimInfoSparseArray.clear();
            if (mVerboseLogEnabled) {
                Log.v(TAG, "active subscription changes: " + mActiveSubInfos);
            }
        }
    }

    /**
     * Gets the instance of WifiCarrierInfoManager.
     * @param telephonyManager Instance of {@link TelephonyManager}
     * @param subscriptionManager Instance of {@link SubscriptionManager}
     * @param wifiInjector Instance of {@link WifiInjector}
     * @return The instance of WifiCarrierInfoManager
     */
    public WifiCarrierInfoManager(@NonNull TelephonyManager telephonyManager,
            @NonNull SubscriptionManager subscriptionManager,
            @NonNull WifiInjector wifiInjector,
            @NonNull FrameworkFacade frameworkFacade,
            @NonNull WifiContext context,
            @NonNull WifiConfigStore configStore,
            @NonNull Handler handler,
            @NonNull WifiMetrics wifiMetrics,
            @NonNull Clock clock) {
        mTelephonyManager = telephonyManager;
        mContext = context;
        mResources = mContext.getResources();
        mWifiInjector = wifiInjector;
        mHandler = handler;
        mSubscriptionManager = subscriptionManager;
        mFrameworkFacade = frameworkFacade;
        mWifiMetrics = wifiMetrics;
        mNotificationManager = mWifiInjector.getWifiNotificationManager();
        mClock = clock;
        // Register broadcast receiver for UI interactions.
        mIntentFilter = new IntentFilter();
        mIntentFilter.addAction(NOTIFICATION_USER_DISMISSED_INTENT_ACTION);
        mIntentFilter.addAction(NOTIFICATION_USER_ALLOWED_CARRIER_INTENT_ACTION);
        mIntentFilter.addAction(NOTIFICATION_USER_DISALLOWED_CARRIER_INTENT_ACTION);
        mIntentFilter.addAction(NOTIFICATION_USER_CLICKED_INTENT_ACTION);

        mContext.registerReceiver(mBroadcastReceiver, mIntentFilter, NETWORK_SETTINGS, handler);
        configStore.registerStoreData(wifiInjector.makeWifiCarrierInfoStoreManagerData(
                new WifiCarrierInfoStoreManagerDataSource()));
        configStore.registerStoreData(wifiInjector.makeImsiPrivacyProtectionExemptionStoreData(
                new ImsiProtectionExemptionDataSource()));

        mSubscriptionManager.addOnSubscriptionsChangedListener(new HandlerExecutor(mHandler),
                new SubscriptionChangeListener());
        onCarrierConfigChanged(context);

        // Monitor for carrier config changes.
        IntentFilter filter = new IntentFilter();
        filter.addAction(CarrierConfigManager.ACTION_CARRIER_CONFIG_CHANGED);
        context.registerReceiver(new BroadcastReceiver() {
            @Override
            public void onReceive(Context context, Intent intent) {
                if (CarrierConfigManager.ACTION_CARRIER_CONFIG_CHANGED
                        .equals(intent.getAction())) {
                    mHandler.post(() -> onCarrierConfigChanged(context));
                }
            }
        }, filter);

        frameworkFacade.registerContentObserver(context, CONTENT_URI, false,
                new ContentObserver(handler) {
                    @Override
                    public void onChange(boolean selfChange) {
                        mHandler.post(() -> onCarrierConfigChanged(context));
                    }
                });
    }

    /**
     * Enable/disable verbose logging.
     */
    public void enableVerboseLogging(int verbose) {
        mVerboseLogEnabled = verbose > 0;
    }

    void onUnlockedUserSwitching(int currentUserId) {
        // Retrieve list of broadcast receivers for this broadcast & send them directed
        // broadcasts to wake them up (if they're in background).
        final List<PackageInfo> provisioningPackageInfos =
                mContext.getPackageManager().getPackagesHoldingPermissions(
                        new String[] {android.Manifest.permission.NETWORK_CARRIER_PROVISIONING},
                        PackageManager.MATCH_UNINSTALLED_PACKAGES);

        vlogd("switched to current unlocked user. notify apps with"
                + " NETWORK_CARRIER_PROVISIONING permission for user - " + currentUserId);

        for (PackageInfo packageInfo : provisioningPackageInfos) {
            Intent intentToSend = new Intent(WifiManager.ACTION_REFRESH_USER_PROVISIONING);
            intentToSend.setPackage(packageInfo.packageName);
            mContext.sendBroadcastAsUser(intentToSend, UserHandle.CURRENT,
                    android.Manifest.permission.NETWORK_CARRIER_PROVISIONING);
        }
    }

    private PersistableBundle getCarrierConfigForSubId(int subId) {
        if (mCachedCarrierConfigPerSubId.contains(subId)) {
            return mCachedCarrierConfigPerSubId.get(subId);
        }
        TelephonyManager specifiedTm = mTelephonyManager.createForSubscriptionId(subId);
        if (specifiedTm.getSimApplicationState() != TelephonyManager.SIM_STATE_LOADED) {
            return null;
        }
        if (mCarrierConfigManager == null) {
            mCarrierConfigManager = mContext.getSystemService(CarrierConfigManager.class);
        }
        if (mCarrierConfigManager == null) {
            return null;
        }
        PersistableBundle carrierConfig = mCarrierConfigManager.getConfigForSubId(subId);
        if (!CarrierConfigManager.isConfigForIdentifiedCarrier(carrierConfig)) {
            return null;
        }
        mCachedCarrierConfigPerSubId.put(subId, carrierConfig);
        return carrierConfig;
    }

    /**
     * Checks whether MAC randomization should be disabled for the provided WifiConfiguration,
     * based on an exception list in the CarrierConfigManager per subId.
     * @param ssid the SSID of a WifiConfiguration, surrounded by double quotes.
     * @param carrierId the ID associated with the network operator for this network suggestion.
     * @param subId the best match subscriptionId for this network suggestion.
     */
    public boolean shouldDisableMacRandomization(String ssid, int carrierId, int subId) {
        if (!SdkLevel.isAtLeastS()) {
            return false;
        }
        if (carrierId == TelephonyManager.UNKNOWN_CARRIER_ID) {
            // only carrier networks are allowed to disable MAC randomization through this path.
            return false;
        }
        PersistableBundle carrierConfig = getCarrierConfigForSubId(subId);
        if (carrierConfig == null) {
            return false;
        }
        String sanitizedSsid = WifiInfo.sanitizeSsid(ssid);
        String[] macRandDisabledSsids = carrierConfig.getStringArray(CarrierConfigManager.Wifi
                .KEY_SUGGESTION_SSID_LIST_WITH_MAC_RANDOMIZATION_DISABLED);
        if (macRandDisabledSsids == null) {
            return false;
        }
        for (String curSsid : macRandDisabledSsids) {
            if (sanitizedSsid.equals(curSsid)) {
                return true;
            }
        }
        return false;
    }

    /**
     * Checks whether merged carrier WiFi networks are permitted for the carrier based on a flag
     * in the CarrierConfigManager.
     *
     * @param subId the best match subscriptionId for this network suggestion.
     */
    public boolean areMergedCarrierWifiNetworksAllowed(int subId) {
        if (!SdkLevel.isAtLeastS()) {
            return false;
        }
        PersistableBundle carrierConfig = getCarrierConfigForSubId(subId);
        if (carrierConfig == null) {
            return false;
        }

        return carrierConfig.getBoolean(
                CarrierConfigManager.KEY_CARRIER_PROVISIONS_WIFI_MERGED_NETWORKS_BOOL, false);
    }

    /**
     * Updates the IMSI encryption information and clears cached CarrierConfig data.
     */
    private void onCarrierConfigChanged(Context context) {
        SparseArray<PersistableBundle> cachedCarrierConfigPerSubIdOld =
                mCachedCarrierConfigPerSubId.clone();
        mCachedCarrierConfigPerSubId.clear();
        mImsiEncryptionInfoAvailable.clear();
        if (mActiveSubInfos == null || mActiveSubInfos.isEmpty()) {
            return;
        }
        for (SubscriptionInfo subInfo : mActiveSubInfos) {
            int subId = subInfo.getSubscriptionId();
            PersistableBundle bundle = getCarrierConfigForSubId(subId);
            if (bundle == null) {
                Log.e(TAG, "Carrier config is missing for: " + subId);
            } else {
                try {
                    if (requiresImsiEncryption(subId)
                            && mTelephonyManager.createForSubscriptionId(subId)
                            .getCarrierInfoForImsiEncryption(TelephonyManager.KEY_TYPE_WLAN)
                            != null) {
                        vlogd("IMSI encryption info is available for " + subId);
                        mImsiEncryptionInfoAvailable.put(subId, true);
                    }
                } catch (IllegalArgumentException e) {
                    vlogd("IMSI encryption info is not available.");
                }
            }
            PersistableBundle bundleOld = cachedCarrierConfigPerSubIdOld.get(subId);
            if (bundleOld != null && bundleOld.getBoolean(CarrierConfigManager
                    .KEY_CARRIER_PROVISIONS_WIFI_MERGED_NETWORKS_BOOL)
                    && !areMergedCarrierWifiNetworksAllowed(subId)) {
                vlogd("Allow carrier merged change from true to false");
                for (OnCarrierOffloadDisabledListener listener :
                        mOnCarrierOffloadDisabledListeners) {
                    listener.onCarrierOffloadDisabled(subId, true);
                }
            }

        }
    }

    /**
     * Check if the IMSI encryption is required for the SIM card.
     * Note: should only be called when {@link #isSimReady(int)} is true, or the result may not be
     * correct.
     *
     * @param subId The subscription ID of SIM card.
     * @return true if the IMSI encryption is required, otherwise false.
     */
    public boolean requiresImsiEncryption(int subId) {
        PersistableBundle bundle = getCarrierConfigForSubId(subId);
        if (bundle == null) {
            Log.wtf(TAG, "requiresImsiEncryption is called when SIM is not ready!");
            return false;
        }
        return (bundle.getInt(CarrierConfigManager.IMSI_KEY_AVAILABILITY_INT)
                & TelephonyManager.KEY_TYPE_WLAN) != 0;
    }

    /**
     * Check if the IMSI encryption is downloaded(available) for the SIM card.
     *
     * @param subId The subscription ID of SIM card.
     * @return true if the IMSI encryption is available, otherwise false.
     */
    public boolean isImsiEncryptionInfoAvailable(int subId) {
        return mImsiEncryptionInfoAvailable.get(subId);
    }

    /**
     * Gets the SubscriptionId of SIM card which is from the carrier specified in config.
     *
     * @param config the instance of {@link WifiConfiguration}
     * @return the best match SubscriptionId
     */
    public int getBestMatchSubscriptionId(@NonNull WifiConfiguration config) {
        if (config.subscriptionId != SubscriptionManager.INVALID_SUBSCRIPTION_ID) {
            return config.subscriptionId;
        }
        if (config.isPasspoint()) {
            return getMatchingSubId(config.carrierId);
        } else {
            return getBestMatchSubscriptionIdForEnterprise(config);
        }
    }

    /**
     * Gets the SubscriptionId of SIM card for given carrier Id
     *
     * @param carrierId carrier id for target carrier
     * @return the matched SubscriptionId
     */
    public int getMatchingSubId(int carrierId) {
        if (mActiveSubInfos == null || mActiveSubInfos.isEmpty()) {
            return SubscriptionManager.INVALID_SUBSCRIPTION_ID;
        }

        int dataSubId = SubscriptionManager.getDefaultDataSubscriptionId();
        int matchSubId = SubscriptionManager.INVALID_SUBSCRIPTION_ID;
        for (SubscriptionInfo subInfo : mActiveSubInfos) {
            if (subInfo.getCarrierId() == carrierId) {
                matchSubId = subInfo.getSubscriptionId();
                if (matchSubId == dataSubId) {
                    // Priority of Data sub is higher than non data sub.
                    break;
                }
            }
        }
        vlogd("matching subId is " + matchSubId);
        return matchSubId;
    }

    private int getBestMatchSubscriptionIdForEnterprise(WifiConfiguration config) {
        if (config.carrierId != TelephonyManager.UNKNOWN_CARRIER_ID) {
            return getMatchingSubId(config.carrierId);
        }
        // Legacy WifiConfiguration without carrier ID
        if (config.enterpriseConfig == null
                 || !config.enterpriseConfig.isAuthenticationSimBased()) {
            Log.w(TAG, "The legacy config is not using EAP-SIM.");
            return SubscriptionManager.INVALID_SUBSCRIPTION_ID;
        }
        int dataSubId = SubscriptionManager.getDefaultDataSubscriptionId();
        if (isSimReady(dataSubId)) {
            vlogd("carrierId is not assigned, using the default data sub.");
            return dataSubId;
        }
        vlogd("data sim is not present.");
        return SubscriptionManager.INVALID_SUBSCRIPTION_ID;
    }

    /**
     * Check if the specified SIM card is ready for Wi-Fi connection on the device.
     *
     * @param subId subscription ID of SIM card in the device.
     * @return true if the SIM is active and all info are available, otherwise false.
     */
    public boolean isSimReady(int subId) {
        if (!SubscriptionManager.isValidSubscriptionId(subId)) {
            return false;
        }
        if (mActiveSubInfos == null || mActiveSubInfos.isEmpty()) {
            return false;
        }
        if (getSimInfo(subId) == null || getCarrierConfigForSubId(subId) == null) {
            return false;
        }
        return mActiveSubInfos.stream().anyMatch(info -> info.getSubscriptionId() == subId);
    }

    /**
     * Get the identity for the current SIM or null if the SIM is not available
     *
     * @param config WifiConfiguration that indicates what sort of authentication is necessary
     * @return Pair<identify, encrypted identity> or null if the SIM is not available
     * or config is invalid
     */
    public Pair<String, String> getSimIdentity(WifiConfiguration config) {
        int subId = getBestMatchSubscriptionId(config);
        if (!SubscriptionManager.isValidSubscriptionId(subId)) {
            return null;
        }

        SimInfo simInfo = getSimInfo(subId);
        if (simInfo == null) {
            return null;
        }

        String identity = buildIdentity(getSimMethodForConfig(config), simInfo.imsi,
                simInfo.mccMnc, false);
        if (identity == null) {
            Log.e(TAG, "Failed to build the identity");
            return null;
        }

        if (!requiresImsiEncryption(subId)) {
            return Pair.create(identity, "");
        }
        TelephonyManager specifiedTm = mTelephonyManager.createForSubscriptionId(subId);
        ImsiEncryptionInfo imsiEncryptionInfo;
        try {
            imsiEncryptionInfo = specifiedTm.getCarrierInfoForImsiEncryption(
                    TelephonyManager.KEY_TYPE_WLAN);
        } catch (RuntimeException e) {
            Log.e(TAG, "Failed to get imsi encryption info: " + e.getMessage());
            return null;
        }
        if (imsiEncryptionInfo == null) {
            // Does not support encrypted identity.
            return Pair.create(identity, "");
        }

        String encryptedIdentity = buildEncryptedIdentity(identity,
                    imsiEncryptionInfo);

        // In case of failure for encryption, abort current EAP authentication.
        if (encryptedIdentity == null) {
            Log.e(TAG, "failed to encrypt the identity");
            return null;
        }
        return Pair.create(identity, encryptedIdentity);
    }

    /**
     * Gets Anonymous identity for current active SIM.
     *
     * @param config the instance of WifiConfiguration.
     * @return anonymous identity@realm which is based on current MCC/MNC, {@code null} if SIM is
     * not ready or absent.
     */
    public String getAnonymousIdentityWith3GppRealm(@NonNull WifiConfiguration config) {
        int subId = getBestMatchSubscriptionId(config);
        if (subId == SubscriptionManager.INVALID_SUBSCRIPTION_ID) {
            return null;
        }
        SimInfo simInfo = getSimInfo(subId);
        if (simInfo == null) {
            return null;
        }
        Pair<String, String> mccMncPair = extractMccMnc(simInfo.imsi, simInfo.mccMnc);
        if (mccMncPair == null) {
            return null;
        }

        String realm = String.format(THREE_GPP_NAI_REALM_FORMAT, mccMncPair.second,
                mccMncPair.first);
        StringBuilder sb = new StringBuilder();
        if (isEapMethodPrefixEnabled(subId)) {
            // Set the EAP method as a prefix
            String eapMethod = EAP_METHOD_PREFIX.get(config.enterpriseConfig.getEapMethod());
            if (!TextUtils.isEmpty(eapMethod)) {
                sb.append(eapMethod);
            }
        }
        return sb.append(ANONYMOUS_IDENTITY).append("@").append(realm).toString();
    }

    /**
     * Encrypt the given data with the given public key.  The encrypted data will be returned as
     * a Base64 encoded string.
     *
     * @param key The public key to use for encryption
     * @param data The data need to be encrypted
     * @param encodingFlag base64 encoding flag
     * @return Base64 encoded string, or null if encryption failed
     */
    @VisibleForTesting
    public static String encryptDataUsingPublicKey(PublicKey key, byte[] data, int encodingFlag) {
        try {
            Cipher cipher = Cipher.getInstance(IMSI_CIPHER_TRANSFORMATION);
            cipher.init(Cipher.ENCRYPT_MODE, key);
            byte[] encryptedBytes = cipher.doFinal(data);

            return Base64.encodeToString(encryptedBytes, 0, encryptedBytes.length, encodingFlag);
        } catch (NoSuchAlgorithmException | NoSuchPaddingException | InvalidKeyException
                | IllegalBlockSizeException | BadPaddingException e) {
            Log.e(TAG, "Encryption failed: " + e.getMessage());
            return null;
        }
    }

    /**
     * Create the encrypted identity.
     *
     * Prefix value:
     * "0" - EAP-AKA Identity
     * "1" - EAP-SIM Identity
     * "6" - EAP-AKA' Identity
     * Encrypted identity format: prefix|IMSI@<NAIRealm>
     * @param identity           permanent identity with format based on section 4.1.1.6 of RFC 4187
     *                           and 4.2.1.6 of RFC 4186.
     * @param imsiEncryptionInfo The IMSI encryption info retrieved from the SIM
     * @return "\0" + encryptedIdentity + "{, Key Identifier AVP}"
     */
    private static String buildEncryptedIdentity(String identity,
            ImsiEncryptionInfo imsiEncryptionInfo) {
        if (imsiEncryptionInfo == null) {
            Log.e(TAG, "imsiEncryptionInfo is not valid");
            return null;
        }
        if (identity == null) {
            Log.e(TAG, "identity is not valid");
            return null;
        }

        // Build and return the encrypted identity.
        String encryptedIdentity = encryptDataUsingPublicKey(
                imsiEncryptionInfo.getPublicKey(), identity.getBytes(), Base64.NO_WRAP);
        if (encryptedIdentity == null) {
            Log.e(TAG, "Failed to encrypt IMSI");
            return null;
        }
        encryptedIdentity = DEFAULT_EAP_PREFIX + encryptedIdentity;
        if (imsiEncryptionInfo.getKeyIdentifier() != null) {
            // Include key identifier AVP (Attribute Value Pair).
            encryptedIdentity = encryptedIdentity + "," + imsiEncryptionInfo.getKeyIdentifier();
        }
        return encryptedIdentity;
    }

    /**
     * Create an identity used for SIM-based EAP authentication. The identity will be based on
     * the info retrieved from the SIM card, such as IMSI and IMSI encryption info. The IMSI
     * contained in the identity will be encrypted if IMSI encryption info is provided.
     *
     * See  rfc4186 & rfc4187 & rfc5448:
     *
     * Identity format:
     * Prefix | [IMSI || Encrypted IMSI] | @realm | {, Key Identifier AVP}
     * where "|" denotes concatenation, "||" denotes exclusive value, "{}"
     * denotes optional value, and realm is the 3GPP network domain name derived from the given
     * MCC/MNC according to the 3GGP spec(TS23.003).
     *
     * Prefix value:
     * "\0" - Encrypted Identity
     * "0" - EAP-AKA Identity
     * "1" - EAP-SIM Identity
     * "6" - EAP-AKA' Identity
     *
     * Encrypted IMSI:
     * Base64{RSA_Public_Key_Encryption{eapPrefix | IMSI}}
     * where "|" denotes concatenation,
     *
     * @param eapMethod EAP authentication method: EAP-SIM, EAP-AKA, EAP-AKA'
     * @param imsi The IMSI retrieved from the SIM
     * @param mccMnc The MCC MNC identifier retrieved from the SIM
     * @param isEncrypted Whether the imsi is encrypted or not.
     * @return the eap identity, built using either the encrypted or un-encrypted IMSI.
     */
    private String buildIdentity(int eapMethod, String imsi, String mccMnc,
                                        boolean isEncrypted) {
        if (imsi == null || imsi.isEmpty()) {
            Log.e(TAG, "No IMSI or IMSI is null");
            return null;
        }

        String prefix = isEncrypted ? DEFAULT_EAP_PREFIX : EAP_METHOD_PREFIX.get(eapMethod);
        if (prefix == null) {
            return null;
        }

        Pair<String, String> mccMncPair = extractMccMnc(imsi, mccMnc);

        String naiRealm = String.format(THREE_GPP_NAI_REALM_FORMAT, mccMncPair.second,
                mccMncPair.first);
        return prefix + imsi + "@" + naiRealm;
    }

    /**
     * Return the associated SIM method for the configuration.
     *
     * @param config WifiConfiguration corresponding to the network.
     * @return the outer EAP method associated with this SIM configuration.
     */
    private static int getSimMethodForConfig(WifiConfiguration config) {
        if (config == null || config.enterpriseConfig == null
                || !config.enterpriseConfig.isAuthenticationSimBased()) {
            return WifiEnterpriseConfig.Eap.NONE;
        }
        int eapMethod = config.enterpriseConfig.getEapMethod();
        if (eapMethod == WifiEnterpriseConfig.Eap.PEAP) {
            // Translate known inner eap methods into an equivalent outer eap method.
            switch (config.enterpriseConfig.getPhase2Method()) {
                case WifiEnterpriseConfig.Phase2.SIM:
                    eapMethod = WifiEnterpriseConfig.Eap.SIM;
                    break;
                case WifiEnterpriseConfig.Phase2.AKA:
                    eapMethod = WifiEnterpriseConfig.Eap.AKA;
                    break;
                case WifiEnterpriseConfig.Phase2.AKA_PRIME:
                    eapMethod = WifiEnterpriseConfig.Eap.AKA_PRIME;
                    break;
            }
        }

        return eapMethod;
    }

    /**
     * Returns true if {@code identity} contains an anonymous@realm identity, false otherwise.
     */
    public static boolean isAnonymousAtRealmIdentity(String identity) {
        if (TextUtils.isEmpty(identity)) return false;
        final String anonymousId = WifiCarrierInfoManager.ANONYMOUS_IDENTITY + "@";
        return identity.startsWith(anonymousId)
                || identity.substring(1).startsWith(anonymousId);
    }

    // TODO replace some of this code with Byte.parseByte
    private static int parseHex(char ch) {
        if ('0' <= ch && ch <= '9') {
            return ch - '0';
        } else if ('a' <= ch && ch <= 'f') {
            return ch - 'a' + 10;
        } else if ('A' <= ch && ch <= 'F') {
            return ch - 'A' + 10;
        } else {
            throw new NumberFormatException("" + ch + " is not a valid hex digit");
        }
    }

    private static byte[] parseHex(String hex) {
        /* This only works for good input; don't throw bad data at it */
        if (hex == null) {
            return new byte[0];
        }

        if (hex.length() % 2 != 0) {
            throw new NumberFormatException(hex + " is not a valid hex string");
        }

        byte[] result = new byte[(hex.length()) / 2 + 1];
        result[0] = (byte) ((hex.length()) / 2);
        for (int i = 0, j = 1; i < hex.length(); i += 2, j++) {
            int val = parseHex(hex.charAt(i)) * 16 + parseHex(hex.charAt(i + 1));
            byte b = (byte) (val & 0xFF);
            result[j] = b;
        }

        return result;
    }

    private static byte[] parseHexWithoutLength(String hex) {
        byte[] tmpRes = parseHex(hex);
        if (tmpRes.length == 0) {
            return tmpRes;
        }

        byte[] result = new byte[tmpRes.length - 1];
        System.arraycopy(tmpRes, 1, result, 0, tmpRes.length - 1);

        return result;
    }

    private static String makeHex(byte[] bytes) {
        StringBuilder sb = new StringBuilder();
        for (byte b : bytes) {
            sb.append(String.format("%02x", b));
        }
        return sb.toString();
    }

    private static String makeHex(byte[] bytes, int from, int len) {
        StringBuilder sb = new StringBuilder();
        for (int i = 0; i < len; i++) {
            sb.append(String.format("%02x", bytes[from + i]));
        }
        return sb.toString();
    }

    private static byte[] concatHex(byte[] array1, byte[] array2) {

        int len = array1.length + array2.length;

        byte[] result = new byte[len];

        int index = 0;
        if (array1.length != 0) {
            for (byte b : array1) {
                result[index] = b;
                index++;
            }
        }

        if (array2.length != 0) {
            for (byte b : array2) {
                result[index] = b;
                index++;
            }
        }

        return result;
    }

    /**
     * Calculate SRES and KC as 3G authentication.
     *
     * Standard       Cellular_auth     Type Command
     *
     * 3GPP TS 31.102 3G_authentication [Length][RAND][Length][AUTN]
     *                         [Length][RES][Length][CK][Length][IK] and more
     *
     * @param requestData RAND data from server.
     * @param config The instance of WifiConfiguration.
     * @return the response data processed by SIM. If all request data is malformed, then returns
     * empty string. If request data is invalid, then returns null.
     */
    public String getGsmSimAuthResponse(String[] requestData, WifiConfiguration config) {
        return getGsmAuthResponseWithLength(requestData, config, TelephonyManager.APPTYPE_USIM);
    }

    /**
     * Calculate SRES and KC as 2G authentication.
     *
     * Standard       Cellular_auth     Type Command
     *
     * 3GPP TS 31.102 2G_authentication [Length][RAND]
     *                         [Length][SRES][Length][Cipher Key Kc]
     *
     * @param requestData RAND data from server.
     * @param config The instance of WifiConfiguration.
     * @return the response data processed by SIM. If all request data is malformed, then returns
     * empty string. If request data is invalid, then returns null.
     */
    public String getGsmSimpleSimAuthResponse(String[] requestData,
            WifiConfiguration config) {
        return getGsmAuthResponseWithLength(requestData, config, TelephonyManager.APPTYPE_SIM);
    }

    private String getGsmAuthResponseWithLength(String[] requestData,
            WifiConfiguration config, int appType) {
        int subId = getBestMatchSubscriptionId(config);
        if (!SubscriptionManager.isValidSubscriptionId(subId)) {
            return null;
        }

        StringBuilder sb = new StringBuilder();
        for (String challenge : requestData) {
            if (challenge == null || challenge.isEmpty()) {
                continue;
            }
            Log.d(TAG, "RAND = " + challenge);

            byte[] rand = null;
            try {
                rand = parseHex(challenge);
            } catch (NumberFormatException e) {
                Log.e(TAG, "malformed challenge");
                continue;
            }

            String base64Challenge = Base64.encodeToString(rand, Base64.NO_WRAP);
            TelephonyManager specifiedTm = mTelephonyManager.createForSubscriptionId(subId);
            String tmResponse = specifiedTm.getIccAuthentication(
                    appType, TelephonyManager.AUTHTYPE_EAP_SIM, base64Challenge);
            Log.v(TAG, "Raw Response - " + tmResponse);

            if (tmResponse == null || tmResponse.length() <= 4) {
                Log.e(TAG, "bad response - " + tmResponse);
                return null;
            }

            byte[] result = Base64.decode(tmResponse, Base64.DEFAULT);
            Log.v(TAG, "Hex Response -" + makeHex(result));
            int sresLen = result[0];
            if (sresLen < 0 || sresLen >= result.length) {
                Log.e(TAG, "malformed response - " + tmResponse);
                return null;
            }
            String sres = makeHex(result, 1, sresLen);
            int kcOffset = 1 + sresLen;
            if (kcOffset >= result.length) {
                Log.e(TAG, "malformed response - " + tmResponse);
                return null;
            }
            int kcLen = result[kcOffset];
            if (kcLen < 0 || kcOffset + kcLen > result.length) {
                Log.e(TAG, "malformed response - " + tmResponse);
                return null;
            }
            String kc = makeHex(result, 1 + kcOffset, kcLen);
            sb.append(":" + kc + ":" + sres);
            Log.v(TAG, "kc:" + kc + " sres:" + sres);
        }

        return sb.toString();
    }

    /**
     * Calculate SRES and KC as 2G authentication.
     *
     * Standard       Cellular_auth     Type Command
     *
     * 3GPP TS 11.11  2G_authentication [RAND]
     *                         [SRES][Cipher Key Kc]
     *
     * @param requestData RAND data from server.
     * @param config the instance of WifiConfiguration.
     * @return the response data processed by SIM. If all request data is malformed, then returns
     * empty string. If request data is invalid, then returns null.
     */
    public String getGsmSimpleSimNoLengthAuthResponse(String[] requestData,
            @NonNull WifiConfiguration config) {

        int subId = getBestMatchSubscriptionId(config);
        if (!SubscriptionManager.isValidSubscriptionId(subId)) {
            return null;
        }

        StringBuilder sb = new StringBuilder();
        for (String challenge : requestData) {
            if (challenge == null || challenge.isEmpty()) {
                continue;
            }
            Log.d(TAG, "RAND = " + challenge);

            byte[] rand = null;
            try {
                rand = parseHexWithoutLength(challenge);
            } catch (NumberFormatException e) {
                Log.e(TAG, "malformed challenge");
                continue;
            }

            String base64Challenge = Base64.encodeToString(rand, Base64.NO_WRAP);
            TelephonyManager specifiedTm = mTelephonyManager.createForSubscriptionId(subId);
            String tmResponse = specifiedTm.getIccAuthentication(TelephonyManager.APPTYPE_SIM,
                    TelephonyManager.AUTHTYPE_EAP_SIM, base64Challenge);
            Log.v(TAG, "Raw Response - " + tmResponse);

            if (tmResponse == null || tmResponse.length() <= 4) {
                Log.e(TAG, "bad response - " + tmResponse);
                return null;
            }

            byte[] result = Base64.decode(tmResponse, Base64.DEFAULT);
            if (SRES_LEN + KC_LEN != result.length) {
                Log.e(TAG, "malformed response - " + tmResponse);
                return null;
            }
            Log.v(TAG, "Hex Response -" + makeHex(result));
            String sres = makeHex(result, START_SRES_POS, SRES_LEN);
            String kc = makeHex(result, START_KC_POS, KC_LEN);
            sb.append(":" + kc + ":" + sres);
            Log.v(TAG, "kc:" + kc + " sres:" + sres);
        }

        return sb.toString();
    }

    /**
     * Data supplied when making a SIM Auth Request
     */
    public static class SimAuthRequestData {
        public SimAuthRequestData() {}
        public SimAuthRequestData(int networkId, int protocol, String ssid, String[] data) {
            this.networkId = networkId;
            this.protocol = protocol;
            this.ssid = ssid;
            this.data = data;
        }

        public int networkId;
        public int protocol;
        public String ssid;
        // EAP-SIM: data[] contains the 3 rand, one for each of the 3 challenges
        // EAP-AKA/AKA': data[] contains rand & authn couple for the single challenge
        public String[] data;
    }

    /**
     * The response to a SIM Auth request if successful
     */
    public static class SimAuthResponseData {
        public SimAuthResponseData(String type, String response) {
            this.type = type;
            this.response = response;
        }

        public String type;
        public String response;
    }

    /**
     * Get the response data for 3G authentication.
     *
     * @param requestData authentication request data from server.
     * @param config the instance of WifiConfiguration.
     * @return the response data processed by SIM. If request data is invalid, then returns null.
     */
    public SimAuthResponseData get3GAuthResponse(SimAuthRequestData requestData,
            WifiConfiguration config) {
        StringBuilder sb = new StringBuilder();
        byte[] rand = null;
        byte[] authn = null;
        String resType = WifiNative.SIM_AUTH_RESP_TYPE_UMTS_AUTH;

        if (requestData.data.length == 2) {
            try {
                rand = parseHex(requestData.data[0]);
                authn = parseHex(requestData.data[1]);
            } catch (NumberFormatException e) {
                Log.e(TAG, "malformed challenge");
            }
        } else {
            Log.e(TAG, "malformed challenge");
        }

        String tmResponse = "";
        if (rand != null && authn != null) {
            String base64Challenge = Base64.encodeToString(concatHex(rand, authn), Base64.NO_WRAP);
            int subId = getBestMatchSubscriptionId(config);
            if (!SubscriptionManager.isValidSubscriptionId(subId)) {
                return null;
            }
            tmResponse = mTelephonyManager
                    .createForSubscriptionId(subId)
                    .getIccAuthentication(TelephonyManager.APPTYPE_USIM,
                            TelephonyManager.AUTHTYPE_EAP_AKA, base64Challenge);
            Log.v(TAG, "Raw Response - " + tmResponse);
        }

        boolean goodReponse = false;
        if (tmResponse != null && tmResponse.length() > 4) {
            byte[] result = Base64.decode(tmResponse, Base64.DEFAULT);
            Log.e(TAG, "Hex Response - " + makeHex(result));
            byte tag = result[0];
            if (tag == (byte) 0xdb) {
                Log.v(TAG, "successful 3G authentication ");
                int resLen = result[1];
                String res = makeHex(result, 2, resLen);
                int ckLen = result[resLen + 2];
                String ck = makeHex(result, resLen + 3, ckLen);
                int ikLen = result[resLen + ckLen + 3];
                String ik = makeHex(result, resLen + ckLen + 4, ikLen);
                sb.append(":" + ik + ":" + ck + ":" + res);
                Log.v(TAG, "ik:" + ik + "ck:" + ck + " res:" + res);
                goodReponse = true;
            } else if (tag == (byte) 0xdc) {
                Log.e(TAG, "synchronisation failure");
                int autsLen = result[1];
                String auts = makeHex(result, 2, autsLen);
                resType = WifiNative.SIM_AUTH_RESP_TYPE_UMTS_AUTS;
                sb.append(":" + auts);
                Log.v(TAG, "auts:" + auts);
                goodReponse = true;
            } else {
                Log.e(TAG, "bad response - unknown tag = " + tag);
            }
        } else {
            Log.e(TAG, "bad response - " + tmResponse);
        }

        if (goodReponse) {
            String response = sb.toString();
            Log.v(TAG, "Supplicant Response -" + response);
            return new SimAuthResponseData(resType, response);
        } else {
            return null;
        }
    }

    /**
     * Decorates a pseudonym with the NAI realm, in case it wasn't provided by the server
     *
     * @param config The instance of WifiConfiguration
     * @param pseudonym The pseudonym (temporary identity) provided by the server
     * @return pseudonym@realm which is based on current MCC/MNC, {@code null} if SIM is
     * not ready or absent.
     */
    public String decoratePseudonymWith3GppRealm(@NonNull WifiConfiguration config,
            String pseudonym) {
        if (TextUtils.isEmpty(pseudonym)) {
            return null;
        }
        if (pseudonym.contains("@")) {
            // Pseudonym is already decorated
            return pseudonym;
        }
        int subId = getBestMatchSubscriptionId(config);

        SimInfo simInfo = getSimInfo(subId);
        if (simInfo == null) {
            return null;
        }
        Pair<String, String> mccMncPair = extractMccMnc(simInfo.imsi, simInfo.mccMnc);
        if (mccMncPair == null) {
            return null;
        }

        String realm = String.format(THREE_GPP_NAI_REALM_FORMAT, mccMncPair.second,
                mccMncPair.first);
        return String.format("%s@%s", pseudonym, realm);
    }

    /**
     * Reset the downloaded IMSI encryption key.
     * @param config Instance of WifiConfiguration
     */
    public void resetCarrierKeysForImsiEncryption(@NonNull WifiConfiguration config) {
        int subId = getBestMatchSubscriptionId(config);
        if (!SubscriptionManager.isValidSubscriptionId(subId)) {
            return;
        }
        TelephonyManager specifiedTm = mTelephonyManager.createForSubscriptionId(subId);
        specifiedTm.resetCarrierKeysForImsiEncryption();
    }

    /**
     * Updates the carrier ID for passpoint configuration with SIM credential.
     *
     * @param config The instance of PasspointConfiguration.
     * @return true if the carrier ID is updated, false otherwise
     */
    public boolean tryUpdateCarrierIdForPasspoint(PasspointConfiguration config) {
        if (config.getCarrierId() != TelephonyManager.UNKNOWN_CARRIER_ID) {
            return false;
        }

        Credential.SimCredential simCredential = config.getCredential().getSimCredential();
        if (simCredential == null) {
            // carrier ID is not required.
            return false;
        }

        IMSIParameter imsiParameter = IMSIParameter.build(simCredential.getImsi());
        // If the IMSI is not full, the carrier ID can not be matched for sure, so it should
        // be ignored.
        if (imsiParameter == null || !imsiParameter.isFullImsi()) {
            vlogd("IMSI is not available or not full");
            return false;
        }
        if (mActiveSubInfos == null || mActiveSubInfos.isEmpty()) {
            return false;
        }
        // Find the active matching SIM card with the full IMSI from passpoint profile.
        for (SubscriptionInfo subInfo : mActiveSubInfos) {
            SimInfo simInfo = getSimInfo(subInfo.getSubscriptionId());
            if (simInfo == null) {
                continue;
            }
            if (imsiParameter.matchesImsi(simInfo.imsi)) {
                config.setCarrierId(subInfo.getCarrierId());
                return true;
            }
        }

        return false;
    }

    /**
     * Get the IMSI and carrier ID of the SIM card which is matched with the given subscription ID.
     *
     * @param subId The subscription ID see {@link SubscriptionInfo#getSubscriptionId()}
     * @return null if there is no matching SIM card, otherwise the IMSI and carrier ID of the
     * matching SIM card
     */
    public @Nullable String getMatchingImsiBySubId(int subId) {
        if (!isSimReady(subId)) {
            return null;
        }
        if (subId != SubscriptionManager.INVALID_SUBSCRIPTION_ID) {
            if (requiresImsiEncryption(subId) && !isImsiEncryptionInfoAvailable(subId)) {
                vlogd("required IMSI encryption information is not available.");
                return null;
            }
            SimInfo simInfo = getSimInfo(subId);
            if (simInfo != null) {
                return simInfo.imsi;
            }
        }
        vlogd("no active SIM card to match the carrier ID.");
        return null;
    }

    /**
     * Get the IMSI and carrier ID of the SIM card which is matched with the given IMSI
     * (only prefix of IMSI - mccmnc*) from passpoint profile.
     *
     * @param imsiPrefix The IMSI parameter from passpoint profile.
     * @return null if there is no matching SIM card, otherwise the IMSI and carrier ID of the
     * matching SIM card
     */
    public @Nullable Pair<String, Integer> getMatchingImsiCarrierId(
            String imsiPrefix) {
        IMSIParameter imsiParameter = IMSIParameter.build(imsiPrefix);
        if (imsiParameter == null) {
            return null;
        }
        if (mActiveSubInfos == null) {
            return null;
        }
        int dataSubId = SubscriptionManager.getDefaultDataSubscriptionId();
        //Pair<IMSI, carrier ID> the IMSI and carrier ID of matched SIM card
        Pair<String, Integer> matchedPair = null;
        // matchedDataPair check if the data SIM is matched.
        Pair<String, Integer> matchedDataPair = null;
        // matchedMnoPair check if any matched SIM card is MNO.
        Pair<String, Integer> matchedMnoPair = null;

        // Find the active matched SIM card with the priority order of Data MNO SIM,
        // Nondata MNO SIM, Data MVNO SIM, Nondata MVNO SIM.
        for (SubscriptionInfo subInfo : mActiveSubInfos) {
            int subId = subInfo.getSubscriptionId();
            if (requiresImsiEncryption(subId) && !isImsiEncryptionInfoAvailable(subId)) {
                vlogd("required IMSI encryption information is not available.");
                continue;
            }
            SimInfo simInfo = getSimInfo(subId);
            if (simInfo == null) {
                continue;
            }
            if (simInfo.mccMnc != null && imsiParameter.matchesMccMnc(simInfo.mccMnc)) {
                if (TextUtils.isEmpty(simInfo.imsi)) {
                    continue;
                }
                matchedPair = new Pair<>(simInfo.imsi, subInfo.getCarrierId());
                if (subId == dataSubId) {
                    matchedDataPair = matchedPair;
                    if (simInfo.getCarrierType() == CARRIER_MNO_TYPE) {
                        vlogd("MNO data is matched via IMSI.");
                        return matchedDataPair;
                    }
                }
                if (simInfo.getCarrierType() == CARRIER_MNO_TYPE) {
                    matchedMnoPair = matchedPair;
                }
            }
        }

        if (matchedMnoPair != null) {
            vlogd("MNO sub is matched via IMSI.");
            return matchedMnoPair;
        }

        if (matchedDataPair != null) {
            vlogd("MVNO data sub is matched via IMSI.");
            return matchedDataPair;
        }

        return matchedPair;
    }

    private void vlogd(String msg) {
        if (!mVerboseLogEnabled) {
            return;
        }

        Log.d(TAG, msg);
    }

    /** Dump state. */
    public void dump(FileDescriptor fd, PrintWriter pw, String[] args) {
        pw.println(TAG + ": ");
        pw.println("mImsiEncryptionInfoAvailable=" + mImsiEncryptionInfoAvailable);
        pw.println("mImsiPrivacyProtectionExemptionMap=" + mImsiPrivacyProtectionExemptionMap);
        pw.println("mMergedCarrierNetworkOffloadMap=" + mMergedCarrierNetworkOffloadMap);
        pw.println("mSubIdToSimInfoSparseArray=" + mSubIdToSimInfoSparseArray);
        pw.println("mActiveSubInfos=" + mActiveSubInfos);
        pw.println("mCachedCarrierConfigPerSubId=" + mCachedCarrierConfigPerSubId);
    }

    /**
     * Get the carrier ID {@link TelephonyManager#getSimCarrierId()} of the carrier which give
     * target package carrier privileges.
     *
     * @param packageName target package to check if grant privileges by any carrier.
     * @return Carrier ID who give privilege to this package. If package isn't granted privilege
     *         by any available carrier, will return UNKNOWN_CARRIER_ID.
     */
    public int getCarrierIdForPackageWithCarrierPrivileges(String packageName) {
        if (mActiveSubInfos == null || mActiveSubInfos.isEmpty()) {
            if (mVerboseLogEnabled) Log.v(TAG, "No subs for carrier privilege check");
            return TelephonyManager.UNKNOWN_CARRIER_ID;
        }
        for (SubscriptionInfo info : mActiveSubInfos) {
            TelephonyManager specifiedTm =
                    mTelephonyManager.createForSubscriptionId(info.getSubscriptionId());
            if (specifiedTm.checkCarrierPrivilegesForPackage(packageName)
                    == TelephonyManager.CARRIER_PRIVILEGE_STATUS_HAS_ACCESS) {
                return info.getCarrierId();
            }
        }
        return TelephonyManager.UNKNOWN_CARRIER_ID;
    }

    /**
     * Get the carrier name for target subscription id.
     * @param subId Subscription id
     * @return String of carrier name.
     */
    public String getCarrierNameForSubId(int subId) {
        TelephonyManager specifiedTm =
                mTelephonyManager.createForSubscriptionId(subId);

        CharSequence name = specifiedTm.getSimCarrierIdName();
        if (name == null) {
            return null;
        }
        return name.toString();
    }

    /**
     * Check if a config is carrier network and from the non default data SIM.
     * @return True if it is carrier network and from non default data SIM,otherwise return false.
     */
    public boolean isCarrierNetworkFromNonDefaultDataSim(WifiConfiguration config) {
        if (config.carrierId == TelephonyManager.UNKNOWN_CARRIER_ID) {
            return false;
        }
        int subId = getBestMatchSubscriptionId(config);
        return subId != SubscriptionManager.getDefaultDataSubscriptionId();
    }

    /**
     * Get the carrier Id of the default data sim.
     */
    public int getDefaultDataSimCarrierId() {
        int subId = SubscriptionManager.getDefaultDataSubscriptionId();
        SimInfo simInfo = getSimInfo(subId);
        if (simInfo == null) {
            return TelephonyManager.UNKNOWN_CARRIER_ID;
        }
        return simInfo.simCarrierId;
    }

    /**
     * Add a listener to monitor user approval IMSI protection exemption.
     */
    public void addImsiExemptionUserApprovalListener(
            OnUserApproveCarrierListener listener) {
        mOnUserApproveCarrierListeners.add(listener);
    }

    /**
     * Add a listener to monitor carrier offload disabled.
     */
    public void addOnCarrierOffloadDisabledListener(
            OnCarrierOffloadDisabledListener listener) {
        mOnCarrierOffloadDisabledListeners.add(listener);
    }

    /**
     * remove a {@link OnCarrierOffloadDisabledListener}.
     */
    public void removeOnCarrierOffloadDisabledListener(
            OnCarrierOffloadDisabledListener listener) {
        mOnCarrierOffloadDisabledListeners.remove(listener);
    }

    /**
     * Clear the Imsi Privacy Exemption user approval info the target carrier.
     */
    public void clearImsiPrivacyExemptionForCarrier(int carrierId) {
        mImsiPrivacyProtectionExemptionMap.remove(carrierId);
        saveToStore();
    }

    /**
     * Check if carrier have user approved exemption for IMSI protection
     */
    public boolean hasUserApprovedImsiPrivacyExemptionForCarrier(int carrierId) {
        return  mImsiPrivacyProtectionExemptionMap.getOrDefault(carrierId, false);
    }

    /**
     * Enable or disable exemption on IMSI protection.
     */
    public void setHasUserApprovedImsiPrivacyExemptionForCarrier(boolean approved, int carrierId) {
        if (mVerboseLogEnabled) {
            Log.v(TAG, "Setting Imsi privacy exemption for carrier " + carrierId
                    + (approved ? " approved" : " not approved"));
        }
        mImsiPrivacyProtectionExemptionMap.put(carrierId, approved);
        // If user approved the exemption restore to initial auto join configure.
        if (approved) {
            for (OnUserApproveCarrierListener listener : mOnUserApproveCarrierListeners) {
                listener.onUserAllowed(carrierId);
            }
        }
        saveToStore();
    }

    private void sendImsiPrivacyNotification(int carrierId) {
        String carrierName = getCarrierNameForSubId(getMatchingSubId(carrierId));
        if (carrierName == null) {
            // If carrier name could not be retrieved, do not send notification.
            return;
        }
        Notification.Action userAllowAppNotificationAction =
                new Notification.Action.Builder(null,
                        mResources.getText(R.string
                                .wifi_suggestion_action_allow_imsi_privacy_exemption_carrier),
                        getPrivateBroadcast(NOTIFICATION_USER_ALLOWED_CARRIER_INTENT_ACTION,
                                Pair.create(EXTRA_CARRIER_NAME, carrierName),
                                Pair.create(EXTRA_CARRIER_ID, carrierId)))
                        .build();
        Notification.Action userDisallowAppNotificationAction =
                new Notification.Action.Builder(null,
                        mResources.getText(R.string
                                .wifi_suggestion_action_disallow_imsi_privacy_exemption_carrier),
                        getPrivateBroadcast(NOTIFICATION_USER_DISALLOWED_CARRIER_INTENT_ACTION,
                                Pair.create(EXTRA_CARRIER_NAME, carrierName),
                                Pair.create(EXTRA_CARRIER_ID, carrierId)))
                        .build();

        Notification notification = mFrameworkFacade.makeNotificationBuilder(
                mContext, WifiService.NOTIFICATION_NETWORK_STATUS)
                .setSmallIcon(Icon.createWithResource(mContext.getWifiOverlayApkPkgName(),
                        com.android.wifi.resources.R.drawable.stat_notify_wifi_in_range))
                .setTicker(mResources.getString(
                        R.string.wifi_suggestion_imsi_privacy_title, carrierName))
                .setContentTitle(mResources.getString(
                        R.string.wifi_suggestion_imsi_privacy_title, carrierName))
                .setStyle(new Notification.BigTextStyle()
                        .bigText(mResources.getString(
                                R.string.wifi_suggestion_imsi_privacy_content)))
                .setContentIntent(getPrivateBroadcast(NOTIFICATION_USER_CLICKED_INTENT_ACTION,
                        Pair.create(EXTRA_CARRIER_NAME, carrierName),
                        Pair.create(EXTRA_CARRIER_ID, carrierId)))
                .setDeleteIntent(getPrivateBroadcast(NOTIFICATION_USER_DISMISSED_INTENT_ACTION,
                        Pair.create(EXTRA_CARRIER_NAME, carrierName),
                        Pair.create(EXTRA_CARRIER_ID, carrierId)))
                .setShowWhen(false)
                .setLocalOnly(true)
                .setColor(mResources.getColor(android.R.color.system_notification_accent_color,
                        mContext.getTheme()))
                .addAction(userDisallowAppNotificationAction)
                .addAction(userAllowAppNotificationAction)
                .setTimeoutAfter(NOTIFICATION_EXPIRY_MILLS)
                .build();

        // Post the notification.
        mNotificationManager.notify(SystemMessage.NOTE_CARRIER_SUGGESTION_AVAILABLE, notification);
        mNotificationUpdateTime = mClock.getElapsedSinceBootMillis()
                + NOTIFICATION_UPDATE_DELAY_MILLS;
        mIsLastUserApprovalUiDialog = false;
    }

    private void sendImsiPrivacyConfirmationDialog(@NonNull String carrierName, int carrierId) {
        mWifiMetrics.addUserApprovalCarrierUiReaction(ACTION_USER_ALLOWED_CARRIER,
                mIsLastUserApprovalUiDialog);
        AlertDialog dialog = mFrameworkFacade.makeAlertDialogBuilder(mContext)
                .setTitle(mResources.getString(
                        R.string.wifi_suggestion_imsi_privacy_exemption_confirmation_title))
                .setMessage(mResources.getString(
                        R.string.wifi_suggestion_imsi_privacy_exemption_confirmation_content,
                        carrierName))
                .setPositiveButton(mResources.getText(
                        R.string.wifi_suggestion_action_allow_imsi_privacy_exemption_confirmation),
                        (d, which) -> mHandler.post(
                                () -> handleUserAllowCarrierExemptionAction(
                                        carrierName, carrierId)))
                .setNegativeButton(mResources.getText(
                        R.string.wifi_suggestion_action_disallow_imsi_privacy_exemption_confirmation),
                        (d, which) -> mHandler.post(
                                () -> handleUserDisallowCarrierExemptionAction(
                                        carrierName, carrierId)))
                .setOnDismissListener(
                        (d) -> mHandler.post(this::handleUserDismissAction))
                .setOnCancelListener(
                        (d) -> mHandler.post(this::handleUserDismissAction))
                .create();
        dialog.setCanceledOnTouchOutside(false);
        dialog.getWindow().setType(WindowManager.LayoutParams.TYPE_SYSTEM_ALERT);
        dialog.getWindow().addSystemFlags(
                WindowManager.LayoutParams.SYSTEM_FLAG_SHOW_FOR_ALL_USERS);
        dialog.show();
        mIsLastUserApprovalUiDialog = true;
    }

    /**
     * Send notification for exemption of IMSI protection if user never made choice before.
     */
    public void sendImsiProtectionExemptionNotificationIfRequired(int carrierId) {
        int subId = getMatchingSubId(carrierId);
        // If user data isn't loaded, don't send notification.
        if (!mUserDataLoaded) {
            return;
        }
        PersistableBundle bundle = getCarrierConfigForSubId(subId);
        if (bundle == null) {
            return;
        }
        if (requiresImsiEncryption(subId)) {
            return;
        }
        if (mImsiPrivacyProtectionExemptionMap.containsKey(carrierId)) {
            return;
        }
        if (mNotificationUpdateTime > mClock.getElapsedSinceBootMillis()) {
            return; // Active notification is still available, do not update.
        }
        Log.i(TAG, "Sending IMSI protection notification for " + carrierId);
        sendImsiPrivacyNotification(carrierId);
    }

    /**
     * Check if the target subscription has a matched carrier Id.
     * @param subId Subscription Id for which this carrier network is valid.
     * @param carrierId Carrier Id for this carrier network.
     * @return true if matches, false otherwise.
     */
    public boolean isSubIdMatchingCarrierId(int subId, int carrierId) {
        if (subId == SubscriptionManager.INVALID_SUBSCRIPTION_ID) {
            // If Subscription Id is not set, consider it matches. Best matching one will be used to
            // connect.
            return true;
        }
        if (mActiveSubInfos == null || mActiveSubInfos.isEmpty()) {
            return false;
        }
        for (SubscriptionInfo info : mActiveSubInfos) {
            if (info.getSubscriptionId() == subId) {
                return info.getCarrierId() == carrierId;
            }
        }
        return false;
    }

    private PendingIntent getPrivateBroadcast(@NonNull String action,
            @NonNull Pair<String, String> extra1, @NonNull Pair<String, Integer> extra2) {
        Intent intent = new Intent(action)
                .setPackage(mContext.getServiceWifiPackageName())
                .putExtra(extra1.first, extra1.second)
                .putExtra(extra2.first, extra2.second);
        return mFrameworkFacade.getBroadcast(mContext, 0, intent,
                PendingIntent.FLAG_UPDATE_CURRENT | PendingIntent.FLAG_IMMUTABLE);
    }

    /**
     * Set carrier network offload enabled/disabled.
     * @param subscriptionId Subscription Id to change carrier offload
     * @param merged True for carrier merged network, false otherwise.
     * @param enabled True for enabled carrier offload, false otherwise.
     */
    public void setCarrierNetworkOffloadEnabled(int subscriptionId, boolean merged,
            boolean enabled) {
        if (merged) {
            mMergedCarrierNetworkOffloadMap.put(subscriptionId, enabled);
        } else {
            mUnmergedCarrierNetworkOffloadMap.put(subscriptionId, enabled);
        }
        if (!enabled) {
            for (OnCarrierOffloadDisabledListener listener : mOnCarrierOffloadDisabledListeners) {
                listener.onCarrierOffloadDisabled(subscriptionId, merged);
            }
        }
        saveToStore();
    }

    /**
     * Check if carrier network offload is enabled/disabled.
     * @param subId Subscription Id to check offload state.
     * @param merged True for carrier merged network, false otherwise.
     * @return True to indicate carrier offload is enabled, false otherwise.
     */
    public boolean isCarrierNetworkOffloadEnabled(int subId, boolean merged) {
        if (merged) {
            return mMergedCarrierNetworkOffloadMap.getOrDefault(subId, true)
                    && isMobileDataEnabled(subId);
        } else {
            return mUnmergedCarrierNetworkOffloadMap.getOrDefault(subId, true);
        }
    }

    private boolean isMobileDataEnabled(int subId) {
        if (!SdkLevel.isAtLeastS()) {
            // As the carrier offload enabled API and the merged carrier API (which is controlled by
            // this toggle) were added in S. Don't check the mobile data toggle when Sdk is less
            // than S.
            return true;
        }
        if (mUserDataEnabled.indexOfKey(subId) >= 0) {
            return mUserDataEnabled.get(subId);
        }
        TelephonyManager specifiedTm = mTelephonyManager.createForSubscriptionId(subId);
        boolean enabled = specifiedTm.isDataEnabled();
        mUserDataEnabled.put(subId, enabled);
        UserDataEnabledChangedListener listener = new UserDataEnabledChangedListener(subId);
        specifiedTm.registerTelephonyCallback(new HandlerExecutor(mHandler), listener);
        mUserDataEnabledListenerList.add(listener);

        return enabled;
    }

    private void saveToStore() {
        // Set the flag to let WifiConfigStore that we have new data to write.
        mHasNewUserDataToSerialize = true;
        mHasNewSharedDataToSerialize = true;
        if (!mWifiInjector.getWifiConfigManager().saveToStore(true)) {
            Log.w(TAG, "Failed to save to store");
        }
    }

    /**
     * Helper method for user factory reset network setting.
     */
    public void clear() {
        mImsiPrivacyProtectionExemptionMap.clear();
        mMergedCarrierNetworkOffloadMap.clear();
        mUnmergedCarrierNetworkOffloadMap.clear();
        mUserDataEnabled.clear();
        if (SdkLevel.isAtLeastS()) {
            for (UserDataEnabledChangedListener listener : mUserDataEnabledListenerList) {
                listener.unregisterListener();
            }
            mUserDataEnabledListenerList.clear();
        }
        resetNotification();
        saveToStore();
    }

    public void resetNotification() {
        mNotificationManager.cancel(SystemMessage.NOTE_CARRIER_SUGGESTION_AVAILABLE);
        mNotificationUpdateTime = 0;
    }

    private SimInfo getSimInfo(int subId) {
        SimInfo simInfo = mSubIdToSimInfoSparseArray.get(subId);
        // If mccmnc is not available, try to get it again.
        if (simInfo != null && simInfo.mccMnc != null && !simInfo.mccMnc.isEmpty()) {
            return simInfo;
        }
        TelephonyManager specifiedTm = mTelephonyManager.createForSubscriptionId(subId);
        if (specifiedTm.getSimApplicationState() != TelephonyManager.SIM_STATE_LOADED) {
            return null;
        }
        String imsi = specifiedTm.getSubscriberId();
        String mccMnc = specifiedTm.getSimOperator();
        if (imsi == null || imsi.isEmpty()) {
            Log.wtf(TAG, "Get invalid imsi when SIM is ready!");
            return null;
        }
        int CarrierIdFromSimMccMnc = specifiedTm.getCarrierIdFromSimMccMnc();
        int SimCarrierId = specifiedTm.getSimCarrierId();
        simInfo = new SimInfo(imsi, mccMnc, CarrierIdFromSimMccMnc, SimCarrierId);
        mSubIdToSimInfoSparseArray.put(subId, simInfo);
        return simInfo;
    }

    /**
     * Try to extract mcc and mnc from IMSI and MCCMNC
     * @return a pair of string represent <mcc, mnc>. Pair.first is mcc, pair.second is mnc.
     */
    private Pair<String, String> extractMccMnc(String imsi, String mccMnc) {
        String mcc;
        String mnc;
        if (mccMnc != null && !mccMnc.isEmpty() && mccMnc.length() >= 5) {
            mcc = mccMnc.substring(0, 3);
            mnc = mccMnc.substring(3);
            if (mnc.length() == 2) {
                mnc = "0" + mnc;
            }
        } else if (imsi != null && !imsi.isEmpty() && imsi.length() >= 6) {
            // extract mcc & mnc from IMSI, assume mnc size is 3
            mcc = imsi.substring(0, 3);
            mnc = imsi.substring(3, 6);
            vlogd("Guessing from IMSI, MCC=" + mcc + "; MNC=" + mnc);
        } else {
            return null;
        }
        return Pair.create(mcc, mnc);
    }

    private boolean isEapMethodPrefixEnabled(int subId) {
        PersistableBundle bundle = getCarrierConfigForSubId(subId);
        if (bundle == null) {
            return false;
        }
        return bundle.getBoolean(CarrierConfigManager.ENABLE_EAP_METHOD_PREFIX_BOOL);
    }
}
