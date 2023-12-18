/*
 * Copyright (C) 2021 The Android Open Source Project
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

package com.android.car.settings.wifi;

import android.content.BroadcastReceiver;
import android.content.Context;
import android.content.Intent;
import android.content.IntentFilter;
import android.net.ConnectivityManager;
import android.net.TetheringManager;
import android.net.wifi.WifiManager;

import androidx.annotation.VisibleForTesting;
import androidx.lifecycle.Lifecycle;
import androidx.localbroadcastmanager.content.LocalBroadcastManager;

import com.android.internal.util.ConcurrentUtils;

/**
 * Consolidates Wifi tethering logic into one handler so we can have consistent logic across various
 * parts of the Settings app.
 */
public class WifiTetheringHandler {

    private Context mContext;
    private CarWifiManager mCarWifiManager;
    private TetheringManager mTetheringManager;
    private boolean mRestartBooked = false;
    private WifiTetheringAvailabilityListener mWifiTetheringAvailabilityListener;

    private WifiManager.SoftApCallback mSoftApCallback = new WifiManager.SoftApCallback() {
        @Override
        public void onStateChanged(int state, int failureReason) {
            handleWifiApStateChanged(state);
        }
    };
    private final BroadcastReceiver mRestartReceiver = new BroadcastReceiver() {
        @Override
        public void onReceive(Context context, Intent intent) {
            if (mCarWifiManager != null && mCarWifiManager.isWifiApEnabled()) {
                restartTethering();
            }
        }
    };

    public WifiTetheringHandler(Context context, Lifecycle lifecycle,
            WifiTetheringAvailabilityListener wifiTetherAvailabilityListener) {
        mContext = context;
        mCarWifiManager = new CarWifiManager(mContext, lifecycle);
        mTetheringManager = mContext.getSystemService(TetheringManager.class);
        mWifiTetheringAvailabilityListener = wifiTetherAvailabilityListener;
    }

    /**
     * Handles operations that should happen in host's onStartInternal().
     */
    public void onStartInternal() {
        mCarWifiManager.registerSoftApCallback(mContext.getMainExecutor(), mSoftApCallback);
        LocalBroadcastManager.getInstance(mContext).registerReceiver(mRestartReceiver,
                new IntentFilter(
                        WifiTetherBasePreferenceController.ACTION_RESTART_WIFI_TETHERING));
    }

    /**
     * Handles operations that should happen in host's onStopInternal().
     */
    public void onStopInternal() {
        mCarWifiManager.unregisterSoftApCallback(mSoftApCallback);
        LocalBroadcastManager.getInstance(mContext).unregisterReceiver(mRestartReceiver);
    }

    /**
     * Returns whether wifi tethering is enabled
     * @return whether wifi tethering is enabled
     */
    public boolean isWifiTetheringEnabled() {
        return mCarWifiManager.isWifiApEnabled();
    }

    /**
     * Changes the Wifi tethering state
     *
     * @param enable Whether to attempt to turn Wifi tethering on or off
     */
    public void updateWifiTetheringState(boolean enable) {
        if (enable) {
            startTethering();
        } else {
            stopTethering();
        }
    }

    @VisibleForTesting
    void setCarWifiManager(CarWifiManager carWifiManager) {
        mCarWifiManager = carWifiManager;
    }

    @VisibleForTesting
    void setTetheringManager(TetheringManager tetheringManager) {
        mTetheringManager = tetheringManager;
    }

    @VisibleForTesting
    void handleWifiApStateChanged(int state) {
        switch (state) {
            case WifiManager.WIFI_AP_STATE_ENABLING:
                mWifiTetheringAvailabilityListener.disablePreference();
                break;
            case WifiManager.WIFI_AP_STATE_ENABLED:
                mWifiTetheringAvailabilityListener.enablePreference();
                mWifiTetheringAvailabilityListener.onWifiTetheringAvailable();
                break;
            case WifiManager.WIFI_AP_STATE_DISABLING:
                mWifiTetheringAvailabilityListener.disablePreference();
                mWifiTetheringAvailabilityListener.onWifiTetheringUnavailable();
                break;
            case WifiManager.WIFI_AP_STATE_DISABLED:
                mWifiTetheringAvailabilityListener.onWifiTetheringUnavailable();
                mWifiTetheringAvailabilityListener.enablePreference();
                if (mRestartBooked) {
                    // Hotspot was disabled as part of a restart request - we can now re-enable it
                    mWifiTetheringAvailabilityListener.disablePreference();
                    startTethering();
                    mRestartBooked = false;
                }
                break;
            default:
                mWifiTetheringAvailabilityListener.onWifiTetheringUnavailable();
                mWifiTetheringAvailabilityListener.enablePreference();
                break;
        }
    }

    private void startTethering() {
        mTetheringManager.startTethering(ConnectivityManager.TETHERING_WIFI,
                ConcurrentUtils.DIRECT_EXECUTOR,
                new TetheringManager.StartTetheringCallback() {
                    @Override
                    public void onTetheringFailed(final int result) {
                        mWifiTetheringAvailabilityListener.onWifiTetheringUnavailable();
                        mWifiTetheringAvailabilityListener.enablePreference();
                    }
                });
    }

    private void stopTethering() {
        mTetheringManager.stopTethering(ConnectivityManager.TETHERING_WIFI);
    }

    private void restartTethering() {
        stopTethering();
        mRestartBooked = true;
    }

    /**
     * Interface for receiving Wifi tethering status updates
     */
    public interface WifiTetheringAvailabilityListener {
        /**
         * Callback for when Wifi tethering is available
         */
        void onWifiTetheringAvailable();

        /**
         * Callback for when Wifi tethering is unavailable
         */
        void onWifiTetheringUnavailable();

        /**
         * Listener should allow further changes to Wifi tethering
         */
        void enablePreference();

        /**
         * Listener should disallow further changes to Wifi tethering
         */
        void disablePreference();
    }
}
