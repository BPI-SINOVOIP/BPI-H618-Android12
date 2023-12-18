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

import static com.android.server.wifi.WifiSettingsConfigStore.WIFI_DEFAULT_COUNTRY_CODE;

import android.annotation.NonNull;
import android.annotation.Nullable;
import android.content.Context;
import android.os.SystemProperties;
import android.telephony.TelephonyManager;
import android.text.TextUtils;
import android.util.ArrayMap;
import android.util.Log;

import com.android.internal.annotations.VisibleForTesting;
import com.android.modules.utils.build.SdkLevel;
import com.android.wifi.resources.R;

import java.io.FileDescriptor;
import java.io.PrintWriter;
import java.text.SimpleDateFormat;
import java.util.ArrayList;
import java.util.Date;
import java.util.List;
import java.util.Locale;
import java.util.Map;
import java.util.Set;

/**
 * Provide functions for making changes to WiFi country code.
 * This Country Code is from MCC or phone default setting. This class sends Country Code
 * to driver through wpa_supplicant when ClientModeImpl marks current state as ready
 * using setReadyForChange(true).
 */
public class WifiCountryCode {
    private static final String TAG = "WifiCountryCode";
    private static final String BOOT_DEFAULT_WIFI_COUNTRY_CODE = "ro.boot.wificountrycode";
    private final Context mContext;
    private final TelephonyManager mTelephonyManager;
    private final ActiveModeWarden mActiveModeWarden;
    private final WifiNative mWifiNative;
    private final WifiSettingsConfigStore mSettingsConfigStore;
    private List<ChangeListener> mListeners = new ArrayList<>();
    private boolean DBG = false;
    /**
     * Map of active ClientModeManager instance to whether it is ready for country code change.
     *
     * - When a new ClientModeManager instance is created, it is added to this map and starts out
     * ready for any country code changes (value = true).
     * - When the ClientModeManager instance starts a connection attempt, it is marked not ready for
     * country code changes (value = false).
     * - When the ClientModeManager instance ends the connection, it is again marked ready for
     * country code changes (value = true).
     * - When the ClientModeManager instance is destroyed, it is removed from this map.
     */
    private final Map<ActiveModeManager, Boolean> mAmmToReadyForChangeMap =
            new ArrayMap<>();
    private static final SimpleDateFormat FORMATTER = new SimpleDateFormat("MM-dd HH:mm:ss.SSS");

    private String mTelephonyCountryCode = null;
    private String mOverrideCountryCode = null;
    private String mDriverCountryCode = null;
    private String mReceivedDriverCountryCode = null;
    private String mTelephonyCountryTimestamp = null;
    private String mDriverCountryTimestamp = null;
    private String mReadyTimestamp = null;

    private class ModeChangeCallbackInternal implements ActiveModeWarden.ModeChangeCallback {
        @Override
        public void onActiveModeManagerAdded(@NonNull ActiveModeManager activeModeManager) {
            if (activeModeManager.getRole() instanceof ActiveModeManager.ClientRole
                    || activeModeManager instanceof SoftApManager) {
                // Add this CMM for tracking. Interface is up and HAL is initialized at this point.
                // If this device runs the 1.5 HAL version, use the IWifiChip.setCountryCode()
                // to set the country code.
                mAmmToReadyForChangeMap.put(activeModeManager, true);
                evaluateAllCmmStateAndApplyIfAllReady();
            }
        }

        @Override
        public void onActiveModeManagerRemoved(@NonNull ActiveModeManager activeModeManager) {
            if (mAmmToReadyForChangeMap.remove(activeModeManager) != null) {
                // Remove this CMM from tracking.
                evaluateAllCmmStateAndApplyIfAllReady();
            }
        }

        @Override
        public void onActiveModeManagerRoleChanged(@NonNull ActiveModeManager activeModeManager) {
            if (activeModeManager.getRole() == ActiveModeManager.ROLE_CLIENT_PRIMARY) {
                // Set this CMM ready for change. This is needed to handle the transition from
                // ROLE_CLIENT_SCAN_ONLY to ROLE_CLIENT_PRIMARY on devices running older HAL
                // versions (since the IWifiChip.setCountryCode() was only added in the 1.5 HAL
                // version, before that we need to wait till supplicant is up for country code
                // change.
                mAmmToReadyForChangeMap.put(activeModeManager, true);
                evaluateAllCmmStateAndApplyIfAllReady();
            }
        }
    }

    private class ClientModeListenerInternal implements ClientModeImplListener {
        @Override
        public void onConnectionStart(@NonNull ConcreteClientModeManager clientModeManager) {
            if (mAmmToReadyForChangeMap.get(clientModeManager) == null) {
                Log.wtf(TAG, "Connection start received from unknown client mode manager");
            }
            // connection start. CMM not ready for country code change.
            mAmmToReadyForChangeMap.put(clientModeManager, false);
            evaluateAllCmmStateAndApplyIfAllReady();
        }

        @Override
        public void onConnectionEnd(@NonNull ConcreteClientModeManager clientModeManager) {
            if (mAmmToReadyForChangeMap.get(clientModeManager) == null) {
                Log.wtf(TAG, "Connection end received from unknown client mode manager");
            }
            // connection end. CMM ready for country code change.
            mAmmToReadyForChangeMap.put(clientModeManager, true);
            evaluateAllCmmStateAndApplyIfAllReady();
        }

    }

    private class CountryChangeListenerInternal implements ChangeListener {
        @Override
        public void onDriverCountryCodeChanged(String country) {
            if (TextUtils.equals(country, mReceivedDriverCountryCode)) {
                return;
            }
            Log.i(TAG, "Receive onDriverCountryCodeChanged " + country);
            mReceivedDriverCountryCode = country;
            updateDriverCountryCodeAndNotifyListener(country);
        }

        @Override
        public void onSetCountryCodeSucceeded(String country) {
            Log.i(TAG, "Receive onSetCountryCodeSucceeded " + country);
            // The driver country code updated, don't need to trigger again.
            if (TextUtils.equals(country, mReceivedDriverCountryCode)) {
                return;
            }
            updateDriverCountryCodeAndNotifyListener(country);
        }
    }

    public WifiCountryCode(
            Context context,
            ActiveModeWarden activeModeWarden,
            ClientModeImplMonitor clientModeImplMonitor,
            WifiNative wifiNative,
            @NonNull WifiSettingsConfigStore settingsConfigStore) {
        mContext = context;
        mTelephonyManager = (TelephonyManager) context.getSystemService(Context.TELEPHONY_SERVICE);
        mActiveModeWarden = activeModeWarden;
        mWifiNative = wifiNative;
        mSettingsConfigStore = settingsConfigStore;

        mActiveModeWarden.registerModeChangeCallback(new ModeChangeCallbackInternal());
        clientModeImplMonitor.registerListener(new ClientModeListenerInternal());
        mWifiNative.registerCountryCodeEventListener(new CountryChangeListenerInternal());

        Log.d(TAG, "Default country code from system property "
                + BOOT_DEFAULT_WIFI_COUNTRY_CODE + " is " + getOemDefaultCountryCode());
    }

    /**
     * Default country code stored in system property
     * @return Country code if available, null otherwise.
     */
    public static String getOemDefaultCountryCode() {
        String country = SystemProperties.get(BOOT_DEFAULT_WIFI_COUNTRY_CODE);
        return WifiCountryCode.isValid(country) ? country.toUpperCase(Locale.US) : null;
    }

    /**
     * Is this a valid country code
     * @param countryCode A 2-Character alphanumeric country code.
     * @return true if the countryCode is valid, false otherwise.
     */
    public static boolean isValid(String countryCode) {
        return countryCode != null && countryCode.length() == 2
                && countryCode.chars().allMatch(Character::isLetterOrDigit);
    }

    /**
     * The class for country code related change listener
     */
    public interface ChangeListener {
        /**
         * Called when receiving country code changed from driver.
         */
        void onDriverCountryCodeChanged(String countryCode);

        /**
         * Called when country code set to native layer successful, framework sends event to
         * force country code changed.
         *
         * Reason: The country code change listener from wificond rely on driver supported
         * NL80211_CMD_REG_CHANGE/NL80211_CMD_WIPHY_REG_CHANGE. Trigger update country code
         * to listener here for non-supported platform.
         */
        default void onSetCountryCodeSucceeded(String country) {}
    }

    /**
     * Register Country code changed listener.
     */
    public void registerListener(@NonNull ChangeListener listener) {
        mListeners.add(listener);
        if (mDriverCountryCode != null) {
            listener.onDriverCountryCodeChanged(mDriverCountryCode);
        }
    }

    /**
     * Enable verbose logging for WifiCountryCode.
     */
    public void enableVerboseLogging(boolean verbose) {
        DBG = verbose;
    }

    private void initializeTelephonyCountryCodeIfNeeded() {
        // If we don't have telephony country code set yet, poll it.
        if (mTelephonyCountryCode == null) {
            Log.d(TAG, "Reading country code from telephony");
            setTelephonyCountryCode(mTelephonyManager.getNetworkCountryIso());
        }
    }

    /**
     * We call native code to request country code changes only if all {@link ClientModeManager}
     * instances are ready for country code change. Country code is a chip level configuration and
     * results in all the connections on the chip being disrupted.
     *
     * @return true if there are active CMM's and all are ready for country code change.
     */
    private boolean isReady() {
        return !mAmmToReadyForChangeMap.isEmpty()
                && mAmmToReadyForChangeMap.values().stream().allMatch(r -> r);
    }

    /**
     * Check all active CMM instances and apply country code change if ready.
     */
    private void evaluateAllCmmStateAndApplyIfAllReady() {
        Log.d(TAG, "evaluateAllCmmStateAndApplyIfAllReady: " + mAmmToReadyForChangeMap);
        if (isReady()) {
            mReadyTimestamp = FORMATTER.format(new Date(System.currentTimeMillis()));
            // We are ready to set country code now.
            // We need to post pending country code request.
            initializeTelephonyCountryCodeIfNeeded();
            updateCountryCode();
        }
    }

    /**
     * This call will override any existing country code.
     * This is for test purpose only and we should disallow any update from
     * telephony in this mode.
     * @param countryCode A 2-Character alphanumeric country code.
     */
    public synchronized void setOverrideCountryCode(String countryCode) {
        if (TextUtils.isEmpty(countryCode)) {
            Log.d(TAG, "Fail to override country code because"
                    + "the received country code is empty");
            return;
        }
        mOverrideCountryCode = countryCode.toUpperCase(Locale.US);

        // If wpa_supplicant is ready we set the country code now, otherwise it will be
        // set once wpa_supplicant is ready.
        if (isReady()) {
            updateCountryCode();
        } else {
            Log.d(TAG, "skip update supplicant not ready yet");
        }
    }

    /**
     * This is for clearing the country code previously set through #setOverrideCountryCode() method
     */
    public synchronized void clearOverrideCountryCode() {
        mOverrideCountryCode = null;

        // If wpa_supplicant is ready we set the country code now, otherwise it will be
        // set once wpa_supplicant is ready.
        if (isReady()) {
            updateCountryCode();
        } else {
            Log.d(TAG, "skip update supplicant not ready yet");
        }
    }

    private void setTelephonyCountryCode(String countryCode) {
        Log.d(TAG, "Set telephony country code to: " + countryCode);
        mTelephonyCountryTimestamp = FORMATTER.format(new Date(System.currentTimeMillis()));

        // Empty country code.
        if (TextUtils.isEmpty(countryCode)) {
            if (mContext.getResources()
                        .getBoolean(R.bool.config_wifi_revert_country_code_on_cellular_loss)) {
                Log.d(TAG, "Received empty country code, reset to default country code");
                mTelephonyCountryCode = null;
            }
        } else {
            mTelephonyCountryCode = countryCode.toUpperCase(Locale.US);
        }
    }

    /**
     * Handle country code change request.
     * @param countryCode The country code intended to set.
     * This is supposed to be from Telephony service.
     * otherwise we think it is from other applications.
     * @return Returns true if the country code passed in is acceptable.
     */
    public boolean setTelephonyCountryCodeAndUpdate(String countryCode) {
        setTelephonyCountryCode(countryCode);
        if (mOverrideCountryCode != null) {
            Log.d(TAG, "Skip Telephony Country code update due to override country code set");
            return false;
        }
        // If wpa_supplicant is ready we set the country code now, otherwise it will be
        // set once wpa_supplicant is ready.
        if (isReady()) {
            updateCountryCode();
        } else {
            Log.d(TAG, "skip update supplicant not ready yet");
        }

        return true;
    }

    /**
     * Method to get the Country Code that was sent to wpa_supplicant.
     *
     * @return Returns the local copy of the Country Code that was sent to the driver upon
     * setReadyForChange(true).
     * If wpa_supplicant was never started, this may be null even if Telephony reported a valid
     * country code.
     * Returns null if no Country Code was sent to driver.
     */
    @VisibleForTesting
    public synchronized String getCountryCodeSentToDriver() {
        return mDriverCountryCode;
    }

    /**
     * Method to return the currently reported Country Code resolved from various sources:
     * e.g. default country code, cellular network country code, country code override, etc.
     *
     * @return The current Wifi Country Code resolved from various sources. Returns null when there
     * is no Country Code available.
     */
    @Nullable
    public synchronized String getCountryCode() {
        initializeTelephonyCountryCodeIfNeeded();
        return pickCountryCode();
    }

    /**
     * set default country code
     * @param countryCode A 2-Character alphanumeric country code.
     */
    public synchronized void setDefaultCountryCode(String countryCode) {
        if (TextUtils.isEmpty(countryCode)) {
            Log.d(TAG, "Fail to set default country code because the country code is empty");
            return;
        }

        mSettingsConfigStore.put(WIFI_DEFAULT_COUNTRY_CODE,
                countryCode.toUpperCase(Locale.US));
        Log.i(TAG, "Default country code updated in config store: " + countryCode);

        // If wpa_supplicant is ready we set the country code now, otherwise it will be
        // set once wpa_supplicant is ready.
        if (isReady()) {
            updateCountryCode();
        } else {
            Log.d(TAG, "skip update supplicant not ready yet");
        }
    }

    /**
     * Method to dump the current state of this WifiCounrtyCode object.
     */
    public synchronized void dump(FileDescriptor fd, PrintWriter pw, String[] args) {
        pw.println("mRevertCountryCodeOnCellularLoss: "
                + mContext.getResources().getBoolean(
                        R.bool.config_wifi_revert_country_code_on_cellular_loss));
        pw.println("DefaultCountryCode(system property): " + getOemDefaultCountryCode());
        pw.println("DefaultCountryCode(config store): "
                + mSettingsConfigStore.get(WIFI_DEFAULT_COUNTRY_CODE));
        pw.println("mDriverCountryCode: " + mDriverCountryCode);
        pw.println("mTelephonyCountryCode: " + mTelephonyCountryCode);
        pw.println("mTelephonyCountryTimestamp: " + mTelephonyCountryTimestamp);
        pw.println("mOverrideCountryCode: " + mOverrideCountryCode);
        pw.println("mDriverCountryTimestamp: " + mDriverCountryTimestamp);
        pw.println("mReadyTimestamp: " + mReadyTimestamp);
        pw.println("isReady: " + isReady());
        pw.println("mAmmToReadyForChangeMap: " + mAmmToReadyForChangeMap);
    }

    private void updateCountryCode() {
        String country = pickCountryCode();
        Log.d(TAG, "updateCountryCode to " + country);

        // We do not check if the country code equals the current one.
        // There are two reasons:
        // 1. Wpa supplicant may silently modify the country code.
        // 2. If Wifi restarted therefore wpa_supplicant also restarted,
        // the country code counld be reset to '00' by wpa_supplicant.
        if (country != null) {
            setCountryCodeNative(country);
        }
        // We do not set country code if there is no candidate. This is reasonable
        // because wpa_supplicant usually starts with an international safe country
        // code setting: '00'.
    }

    private String pickCountryCode() {
        if (mOverrideCountryCode != null) {
            return mOverrideCountryCode;
        }
        if (mTelephonyCountryCode != null) {
            return mTelephonyCountryCode;
        }
        return mSettingsConfigStore.get(WIFI_DEFAULT_COUNTRY_CODE);
    }

    private boolean setCountryCodeNative(String country) {
        Set<ActiveModeManager> amms = mAmmToReadyForChangeMap.keySet();
        boolean isConcreteClientModeManagerUpdated = false;
        boolean anyAmmConfigured = false;
        for (ActiveModeManager am : amms) {
            if (!isConcreteClientModeManagerUpdated && am instanceof ConcreteClientModeManager) {
                // Set the country code using one of the active mode managers. Since
                // country code is a chip level global setting, it can be set as long
                // as there is at least one active interface to communicate to Wifi chip
                ConcreteClientModeManager cm = (ConcreteClientModeManager) am;
                if (!cm.setCountryCode(country)) {
                    Log.d(TAG, "Failed to set country code (ConcreteClientModeManager) to "
                            + country);
                } else {
                    isConcreteClientModeManagerUpdated = true;
                    anyAmmConfigured = true;
                    // Start from S, frameworks support country code callback from wificond,
                    // move "notify the lister" to CountryChangeListenerInternal.
                    if (!SdkLevel.isAtLeastS()) {
                        updateDriverCountryCodeAndNotifyListener(country);
                    }
                }
            } else if (am instanceof SoftApManager) {
                // The API:updateCountryCode in SoftApManager is asynchronous, it requires a new
                // callback support in S to trigger "updateDriverCountryCodeAndNotifyListener" for
                // the new S API: SoftApCapability#getSupportedChannelList(band).
                // It requires:
                // 1. a new overlay configuration which is introduced from S.
                // 2. wificond support in S for S API: SoftApCapability#getSupportedChannelList
                // Any case if device supported to set country code in R,
                // the new S API: SoftApCapability#getSupportedChannelList(band) still doesn't work
                // normally in R build when wifi disabled.
                SoftApManager sm = (SoftApManager) am;
                if (!sm.updateCountryCode(country)) {
                    Log.d(TAG, "Can't set country code (SoftApManager) to "
                            + country + " (Device doesn't support it)");
                } else {
                    anyAmmConfigured = true;
                }
            }
        }
        return anyAmmConfigured;
    }

    private void updateDriverCountryCodeAndNotifyListener(String country) {
        mDriverCountryTimestamp = FORMATTER.format(new Date(System.currentTimeMillis()));
        mDriverCountryCode = country;
        for (ChangeListener listener : mListeners) {
            listener.onDriverCountryCodeChanged(country);
        }
    }
}

