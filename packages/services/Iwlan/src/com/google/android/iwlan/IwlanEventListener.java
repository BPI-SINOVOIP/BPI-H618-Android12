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
import android.content.Intent;
import android.database.ContentObserver;
import android.net.Uri;
import android.net.wifi.WifiInfo;
import android.net.wifi.WifiManager;
import android.os.Handler;
import android.os.HandlerThread;
import android.os.Looper;
import android.support.annotation.IntDef;
import android.support.annotation.NonNull;
import android.telephony.CarrierConfigManager;
import android.telephony.SubscriptionManager;
import android.telephony.TelephonyManager;
import android.telephony.ims.ImsManager;
import android.telephony.ims.ImsMmTelManager;
import android.util.Log;
import android.util.SparseArray;

import com.android.internal.annotations.VisibleForTesting;

import java.util.HashSet;
import java.util.List;
import java.util.Map;
import java.util.Set;
import java.util.concurrent.ConcurrentHashMap;

public class IwlanEventListener {

    public static final int UNKNOWN_EVENT = -1;

    /** On receiving {@link CarrierConfigManager#ACTION_CARRIER_CONFIG_CHANGED} intent. */
    public static final int CARRIER_CONFIG_CHANGED_EVENT = 1;

    /** Wifi turned off or disabled. */
    public static final int WIFI_DISABLE_EVENT = 2;

    /** Airplane mode turned off or disabled. */
    public static final int APM_DISABLE_EVENT = 3;
    /** Airplame mode turned on or enabled */
    public static final int APM_ENABLE_EVENT = 4;

    /** Wifi AccessPoint changed. */
    public static final int WIFI_AP_CHANGED_EVENT = 5;

    /** Wifi calling turned on or enabled */
    public static final int WIFI_CALLING_ENABLE_EVENT = 6;

    /** Wifi calling turned off or disabled */
    public static final int WIFI_CALLING_DISABLE_EVENT = 7;

    /** Cross sim calling enabled */
    public static final int CROSS_SIM_CALLING_ENABLE_EVENT = 8;

    /** Cross sim calling disabled */
    public static final int CROSS_SIM_CALLING_DISABLE_EVENT = 9;

    /**
     * On receiving {@link CarrierConfigManager#ACTION_CARRIER_CONFIG_CHANGED} intent with
     * UNKNOWN_CARRIER_ID.
     */
    public static final int CARRIER_CONFIG_UNKNOWN_CARRIER_EVENT = 10;

    @IntDef({
        CARRIER_CONFIG_CHANGED_EVENT,
        WIFI_DISABLE_EVENT,
        APM_DISABLE_EVENT,
        APM_ENABLE_EVENT,
        WIFI_AP_CHANGED_EVENT,
        WIFI_CALLING_ENABLE_EVENT,
        WIFI_CALLING_DISABLE_EVENT,
        CROSS_SIM_CALLING_ENABLE_EVENT,
        CROSS_SIM_CALLING_DISABLE_EVENT,
        CARRIER_CONFIG_UNKNOWN_CARRIER_EVENT
    })
    @interface IwlanEventType {};

    private static String LOG_TAG = IwlanEventListener.class.getSimpleName();

    private final String SUB_TAG;

    private static Boolean sIsAirplaneModeOn;

    private static String sWifiSSID = new String();

    private static Map<Integer, IwlanEventListener> mInstances = new ConcurrentHashMap<>();

    private Context mContext;
    private int mSlotId;
    private Uri mCrossSimCallingUri;
    private Uri mWfcEnabledUri;
    private UserSettingContentObserver mUserSettingContentObserver;
    private HandlerThread mUserSettingHandlerThread;

    SparseArray<Set<Handler>> eventHandlers = new SparseArray<>();

    private class UserSettingContentObserver extends ContentObserver {
        UserSettingContentObserver(Handler h) {
            super(h);
        }

        @Override
        public void onChange(boolean selfChange, Uri uri) {
            if (mCrossSimCallingUri.equals(uri)) {
                getCurrentUriSetting(uri);
            } else if (mWfcEnabledUri.equals(uri)) {
                getCurrentUriSetting(uri);
            }
        }
    }

    /** Returns IwlanEventListener instance */
    public static IwlanEventListener getInstance(@NonNull Context context, int slotId) {
        return mInstances.computeIfAbsent(slotId, k -> new IwlanEventListener(context, slotId));
    }

    /**
     * Adds handler for the list of events.
     *
     * @param events lists of events for which the handler needs to be notified.
     * @param handler handler to be called when the events happen
     */
    public synchronized void addEventListener(List<Integer> events, Handler handler) {
        for (@IwlanEventType int event : events) {
            if (eventHandlers.contains(event)) {
                eventHandlers.get(event).add(handler);
            } else {
                Set<Handler> handlers = new HashSet<>();
                handlers.add(handler);
                eventHandlers.append(event, handlers);
            }
        }
    }

    /**
     * Removes handler for the list of events.
     *
     * @param events lists of events for which the handler needs to be removed.
     * @param handler handler to be removed
     */
    public synchronized void removeEventListener(List<Integer> events, Handler handler) {
        for (int event : events) {
            if (eventHandlers.contains(event)) {
                Set<Handler> handlers = eventHandlers.get(event);
                handlers.remove(handler);
                if (handlers.isEmpty()) {
                    eventHandlers.delete(event);
                }
            }
        }
        if (eventHandlers.size() == 0) {
            mInstances.remove(mSlotId, this);
        }
    }

    /**
     * Removes handler for all events it is registered
     *
     * @param handler handler to be removed
     */
    public synchronized void removeEventListener(Handler handler) {
        for (int i = 0; i < eventHandlers.size(); i++) {
            Set<Handler> handlers = eventHandlers.valueAt(i);
            handlers.remove(handler);
            if (handlers.isEmpty()) {
                eventHandlers.delete(eventHandlers.keyAt(i));
                i--;
            }
        }
        if (eventHandlers.size() == 0) {
            mInstances.remove(mSlotId, this);
        }
    }

    /**
     * Report a Broadcast received. Mainly used by IwlanBroadcastReceiver to report the following
     * broadcasts CARRIER_CONFIG_CHANGED
     *
     * @param Intent intent
     */
    public static synchronized void onBroadcastReceived(Intent intent) {
        int event = UNKNOWN_EVENT;
        switch (intent.getAction()) {
            case CarrierConfigManager.ACTION_CARRIER_CONFIG_CHANGED:
                int slotId =
                        intent.getIntExtra(
                                CarrierConfigManager.EXTRA_SLOT_INDEX,
                                SubscriptionManager.INVALID_SIM_SLOT_INDEX);
                int carrierId =
                        intent.getIntExtra(
                                TelephonyManager.EXTRA_CARRIER_ID,
                                TelephonyManager.UNKNOWN_CARRIER_ID);

                Context context = IwlanDataService.getContext();
                if (slotId != SubscriptionManager.INVALID_SIM_SLOT_INDEX && context != null) {
                    if (carrierId != TelephonyManager.UNKNOWN_CARRIER_ID) {
                        event = CARRIER_CONFIG_CHANGED_EVENT;
                        getInstance(context, slotId).registerContentObserver();
                    } else {
                        event = CARRIER_CONFIG_UNKNOWN_CARRIER_EVENT;
                    }
                    getInstance(context, slotId).updateHandlers(event);
                }
                break;
            case Intent.ACTION_AIRPLANE_MODE_CHANGED:
                Boolean isAirplaneModeOn = new Boolean(intent.getBooleanExtra("state", false));
                if (sIsAirplaneModeOn != null && sIsAirplaneModeOn.equals(isAirplaneModeOn)) {
                    // no change in apm state
                    break;
                }
                sIsAirplaneModeOn = isAirplaneModeOn;
                event = sIsAirplaneModeOn ? APM_ENABLE_EVENT : APM_DISABLE_EVENT;
                for (Map.Entry<Integer, IwlanEventListener> entry : mInstances.entrySet()) {
                    IwlanEventListener instance = entry.getValue();
                    instance.updateHandlers(event);
                }
                break;
            case WifiManager.WIFI_STATE_CHANGED_ACTION:
                int wifiState =
                        intent.getIntExtra(
                                WifiManager.EXTRA_WIFI_STATE, WifiManager.WIFI_STATE_UNKNOWN);
                if (wifiState == WifiManager.WIFI_STATE_DISABLED) {
                    event = WIFI_DISABLE_EVENT;
                    for (Map.Entry<Integer, IwlanEventListener> entry : mInstances.entrySet()) {
                        IwlanEventListener instance = entry.getValue();
                        instance.updateHandlers(event);
                    }
                }
                break;
        }
    }

    /**
     * Broadcast WIFI_AP_CHANGED_EVENT if Wifi SSID changed after Wifi connected.
     *
     * @param Context context
     */
    public static void onWifiConnected(Context context) {
        WifiManager wifiManager = context.getSystemService(WifiManager.class);
        if (wifiManager == null) {
            Log.e(LOG_TAG, "Could not find wifiManager");
            return;
        }
        WifiInfo wifiInfo = wifiManager.getConnectionInfo();
        if (wifiInfo == null) {
            Log.e(LOG_TAG, "wifiInfo is null");
            return;
        }
        String wifiSSID = wifiInfo.getSSID();
        if (wifiSSID.equals(WifiManager.UNKNOWN_SSID)) {
            Log.e(LOG_TAG, "Could not get Wifi SSID");
            return;
        }

        // Check sWifiSSID is greater than 0 to avoid trigger event after device first camps on
        // Wifi.
        if (sWifiSSID.length() > 0 && !sWifiSSID.equals(wifiSSID)) {
            Log.d(LOG_TAG, "Wifi SSID changed");
            int event = WIFI_AP_CHANGED_EVENT;
            for (Map.Entry<Integer, IwlanEventListener> entry : mInstances.entrySet()) {
                IwlanEventListener instance = entry.getValue();
                if (instance != null) {
                    instance.updateHandlers(event);
                }
            }
        }
        sWifiSSID = wifiSSID;
    }

    /**
     * Returns the Event id of the String. String that matches the name of the event
     *
     * @param event String form of the event.
     * @param int form of the event.
     */
    public static int getUnthrottlingEvent(String event) {
        int ret = UNKNOWN_EVENT;
        switch (event) {
            case "CARRIER_CONFIG_CHANGED_EVENT":
                ret = CARRIER_CONFIG_CHANGED_EVENT;
                break;
            case "WIFI_DISABLE_EVENT":
                ret = WIFI_DISABLE_EVENT;
                break;
            case "APM_DISABLE_EVENT":
                ret = APM_DISABLE_EVENT;
                break;
            case "APM_ENABLE_EVENT":
                ret = APM_ENABLE_EVENT;
                break;
            case "WIFI_AP_CHANGED_EVENT":
                ret = WIFI_AP_CHANGED_EVENT;
                break;
            case "WIFI_CALLING_ENABLE_EVENT":
                ret = WIFI_CALLING_ENABLE_EVENT;
                break;
            case "WIFI_CALLING_DISABLE_EVENT":
                ret = WIFI_CALLING_DISABLE_EVENT;
                break;
            case "CROSS_SIM_CALLING_ENABLE_EVENT":
                ret = CROSS_SIM_CALLING_ENABLE_EVENT;
                break;
            case "CROSS_SIM_CALLING_DISABLE_EVENT":
                ret = CROSS_SIM_CALLING_DISABLE_EVENT;
                break;
            case "CARRIER_CONFIG_UNKNOWN_CARRIER_EVENT":
                ret = CARRIER_CONFIG_UNKNOWN_CARRIER_EVENT;
                break;
        }
        return ret;
    }

    private IwlanEventListener(Context context, int slotId) {
        mContext = context;
        mSlotId = slotId;
        SUB_TAG = IwlanEventListener.class.getSimpleName() + "[" + slotId + "]";
        sIsAirplaneModeOn = null;
    }

    private void registerContentObserver() {
        // Register for content observer
        if (mUserSettingContentObserver == null) {
            mUserSettingHandlerThread =
                    new HandlerThread(IwlanNetworkService.class.getSimpleName());
            mUserSettingHandlerThread.start();
            Looper mlooper = mUserSettingHandlerThread.getLooper();
            Handler mhandler = new Handler(mlooper);
            mUserSettingContentObserver = new UserSettingContentObserver(mhandler);

            // Register for CrossSimCalling setting uri
            mCrossSimCallingUri =
                    Uri.withAppendedPath(
                            SubscriptionManager.CROSS_SIM_ENABLED_CONTENT_URI,
                            String.valueOf(IwlanHelper.getSubId(mContext, mSlotId)));
            mContext.getContentResolver()
                    .registerContentObserver(
                            mCrossSimCallingUri, true, mUserSettingContentObserver);

            // Register for WifiCalling setting uri
            mWfcEnabledUri =
                    Uri.withAppendedPath(
                            SubscriptionManager.WFC_ENABLED_CONTENT_URI,
                            String.valueOf(IwlanHelper.getSubId(mContext, mSlotId)));
            mContext.getContentResolver()
                    .registerContentObserver(mWfcEnabledUri, true, mUserSettingContentObserver);
        }
        // Update current Cross Sim Calling setting
        getCurrentUriSetting(mCrossSimCallingUri);

        // Update current Wifi Calling setting
        getCurrentUriSetting(mWfcEnabledUri);
    }

    @VisibleForTesting
    void getCurrentUriSetting(Uri uri) {
        String uriString = uri.getPath();
        int subIndex = Integer.parseInt(uriString.substring(uriString.lastIndexOf('/') + 1));
        int slotIndex = SubscriptionManager.getSlotIndex(subIndex);

        if (slotIndex == SubscriptionManager.INVALID_SIM_SLOT_INDEX) {
            Log.e(SUB_TAG, "Invalid slot index: " + slotIndex);
            return;
        }

        if (uri.equals(mCrossSimCallingUri)) {
            boolean isCstEnabled = IwlanHelper.isCrossSimCallingEnabled(mContext, slotIndex);
            int event =
                    (isCstEnabled)
                            ? CROSS_SIM_CALLING_ENABLE_EVENT
                            : CROSS_SIM_CALLING_DISABLE_EVENT;
            getInstance(mContext, slotIndex).updateHandlers(event);
        } else if (uri.equals(mWfcEnabledUri)) {
            ImsManager imsManager = mContext.getSystemService(ImsManager.class);
            if (imsManager == null) {
                Log.e(SUB_TAG, "Could not find  ImsManager");
                return;
            }
            ImsMmTelManager imsMmTelManager = imsManager.getImsMmTelManager(subIndex);
            if (imsMmTelManager == null) {
                Log.e(SUB_TAG, "Could not find  ImsMmTelManager");
                return;
            }
            boolean wfcEnabled = imsMmTelManager.isVoWiFiSettingEnabled();
            int event = (wfcEnabled) ? WIFI_CALLING_ENABLE_EVENT : WIFI_CALLING_DISABLE_EVENT;
            getInstance(mContext, slotIndex).updateHandlers(event);
        } else {
            Log.e(SUB_TAG, "Unknown Uri : " + uri);
        }
    }

    @VisibleForTesting
    void setCrossSimCallingUri(Uri uri) {
        mCrossSimCallingUri = uri;
    }

    @VisibleForTesting
    void setWfcEnabledUri(Uri uri) {
        mWfcEnabledUri = uri;
    }

    private synchronized void updateHandlers(int event) {
        if (eventHandlers.contains(event)) {
            Log.d(SUB_TAG, "Updating handlers for the event: " + event);
            for (Handler handler : eventHandlers.get(event)) {
                handler.obtainMessage(event).sendToTarget();
            }
        }
    }
}
