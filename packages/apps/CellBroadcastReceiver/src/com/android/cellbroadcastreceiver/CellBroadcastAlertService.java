/*
 * Copyright (C) 2011 The Android Open Source Project
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

package com.android.cellbroadcastreceiver;

import static android.telephony.SmsCbMessage.MESSAGE_FORMAT_3GPP;
import static android.telephony.SmsCbMessage.MESSAGE_FORMAT_3GPP2;

import android.annotation.NonNull;
import android.app.ActivityManager;
import android.app.Notification;
import android.app.NotificationChannel;
import android.app.NotificationManager;
import android.app.PendingIntent;
import android.app.Service;
import android.bluetooth.BluetoothDevice;
import android.bluetooth.BluetoothManager;
import android.content.ContentValues;
import android.content.Context;
import android.content.Intent;
import android.content.SharedPreferences;
import android.content.pm.PackageManager;
import android.content.res.Resources;
import android.net.Uri;
import android.os.Binder;
import android.os.Bundle;
import android.os.Handler;
import android.os.IBinder;
import android.os.Looper;
import android.os.SystemProperties;
import android.os.UserHandle;
import android.preference.PreferenceManager;
import android.provider.Telephony;
import android.service.notification.StatusBarNotification;
import android.telephony.PhoneStateListener;
import android.telephony.SmsCbEtwsInfo;
import android.telephony.SmsCbMessage;
import android.telephony.TelephonyManager;
import android.text.TextUtils;
import android.util.Log;

import com.android.cellbroadcastreceiver.CellBroadcastChannelManager.CellBroadcastChannelRange;
import com.android.cellbroadcastservice.CellBroadcastStatsLog;
import com.android.internal.annotations.VisibleForTesting;

import java.util.ArrayList;
import java.util.Locale;
import java.util.Set;

/**
 * This service manages the display and animation of broadcast messages.
 * Emergency messages display with a flashing animated exclamation mark icon,
 * and an alert tone is played when the alert is first shown to the user
 * (but not when the user views a previously received broadcast).
 */
public class CellBroadcastAlertService extends Service {
    private static final String TAG = "CBAlertService";

    /** Intent action to display alert dialog/notification, after verifying the alert is new. */
    @VisibleForTesting
    public static final String SHOW_NEW_ALERT_ACTION = "cellbroadcastreceiver.SHOW_NEW_ALERT";

    /** Identifier for getExtra() when adding this object to an Intent. */
    public static final String SMS_CB_MESSAGE_EXTRA =
            "com.android.cellbroadcastreceiver.SMS_CB_MESSAGE";

    /** Intent extra indicate this intent is to dismiss the alert dialog */
    public static final String DISMISS_DIALOG = "com.android.cellbroadcastreceiver.DIMISS_DIALOG";

    /**
     * Use different request code to create distinct pendingIntent for notification deleteIntent
     * and contentIntent.
     */
    private static final int REQUEST_CODE_CONTENT_INTENT = 1;
    private static final int REQUEST_CODE_DELETE_INTENT = 2;

    /** Use the same notification ID for non-emergency alerts. */
    @VisibleForTesting
    public static final int NOTIFICATION_ID = 1;
    public static final int SETTINGS_CHANGED_NOTIFICATION_ID = 2;

    /**
     * Notification channel containing for non-emergency alerts.
     */
    static final String NOTIFICATION_CHANNEL_NON_EMERGENCY_ALERTS = "broadcastMessagesNonEmergency";

    /**
     * Notification channel for notifications accompanied by the alert dialog.
     * e.g, only show when the device has active connections to companion devices.
     */
    static final String NOTIFICATION_CHANNEL_EMERGENCY_ALERTS = "broadcastMessages";

    /**
     * Notification channel for emergency alerts. This is used when users dismiss the alert
     * dialog without officially hitting "OK" (e.g. by pressing the home button). In this case we
     * pop up a notification for them to refer to later.
     *
     * This notification channel is HIGH_PRIORITY.
     */
    static final String NOTIFICATION_CHANNEL_HIGH_PRIORITY_EMERGENCY_ALERTS =
            "broadcastMessagesHighPriority";

    /**
     * Notification channel for emergency alerts during voice call. This is used when users in a
     * voice call, emergency alert will be displayed in a notification format rather than playing
     * alert tone.
     */
    static final String NOTIFICATION_CHANNEL_EMERGENCY_ALERTS_IN_VOICECALL =
        "broadcastMessagesInVoiceCall";

    /**
     * Notification channel for informing the user when a new Carrier's WEA settings have been
     * automatically applied.
     */
    static final String NOTIFICATION_CHANNEL_SETTINGS_UPDATES = "settingsUpdates";

    /** Intent extra for passing a SmsCbMessage */
    private static final String EXTRA_MESSAGE = "message";

    /**
     * Key for accessing message filter from SystemProperties. For testing use.
     */
    private static final String MESSAGE_FILTER_PROPERTY_KEY =
            "persist.cellbroadcast.message_filter";

    private Context mContext;

    /**
     * Alert type
     */
    public enum AlertType {
        DEFAULT,
        ETWS_DEFAULT,
        ETWS_EARTHQUAKE,
        ETWS_TSUNAMI,
        TEST,
        AREA,
        INFO,
        OTHER
    }

    private TelephonyManager mTelephonyManager;

    /**
     * Do not preempt active voice call, instead post notifications and play the ringtone/vibrate
     * when the voicecall finish
     */
    private static boolean sRemindAfterCallFinish = false;


    @Override
    public int onStartCommand(Intent intent, int flags, int startId) {
        mContext = getApplicationContext();
        String action = intent.getAction();
        Log.d(TAG, "onStartCommand: " + action);
        if (Telephony.Sms.Intents.ACTION_SMS_EMERGENCY_CB_RECEIVED.equals(action) ||
                Telephony.Sms.Intents.SMS_CB_RECEIVED_ACTION.equals(action)) {
            handleCellBroadcastIntent(intent);
        } else if (SHOW_NEW_ALERT_ACTION.equals(action)) {
            if (UserHandle.myUserId() == ((ActivityManager) getSystemService(
                    Context.ACTIVITY_SERVICE)).getCurrentUser()) {
                showNewAlert(intent);
            } else {
                Log.d(TAG, "Not active user, ignore the alert display");
            }
        } else {
            Log.e(TAG, "Unrecognized intent action: " + action);
        }
        return START_NOT_STICKY;
    }

    @Override
    public void onCreate() {
        mTelephonyManager = (TelephonyManager)
                getApplicationContext().getSystemService(Context.TELEPHONY_SERVICE);
        mTelephonyManager.listen(mPhoneStateListener, PhoneStateListener.LISTEN_CALL_STATE);
    }

    @Override
    public void onDestroy() {
        // Stop listening for incoming calls.
        mTelephonyManager.listen(mPhoneStateListener, 0);

    }

    /**
     * Check if the enabled message should be displayed to users in the form of pop-up dialog.
     *
     * @param message
     * @return True if the full screen alert should be displayed to the users. False otherwise.
     */
    public boolean shouldDisplayFullScreenMessage(@NonNull SmsCbMessage message) {
        CellBroadcastChannelManager channelManager =
                new CellBroadcastChannelManager(mContext, message.getSubscriptionId());
        // check the full-screen message settings to hide or show message to users.
        if (channelManager.checkCellBroadcastChannelRange(message.getServiceCategory(),
                R.array.public_safety_messages_channels_range_strings)) {
            return PreferenceManager.getDefaultSharedPreferences(this)
                    .getBoolean(CellBroadcastSettings.KEY_ENABLE_PUBLIC_SAFETY_MESSAGES_FULL_SCREEN,
                            true);
        }
        // if no separate full-screen message settings exists, then display the message by default.
        return true;
    }

    /**
     * Check if we should display the received cell broadcast message.
     *
     * @param message Cell broadcast message
     * @return True if the message should be displayed to the user.
     */
    @VisibleForTesting
    public boolean shouldDisplayMessage(SmsCbMessage message) {
        TelephonyManager tm = ((TelephonyManager) mContext.getSystemService(
                Context.TELEPHONY_SERVICE)).createForSubscriptionId(message.getSubscriptionId());
        if (tm.getEmergencyCallbackMode() && CellBroadcastSettings.getResources(
                mContext, message.getSubscriptionId()).getBoolean(R.bool.ignore_messages_in_ecbm)) {
            // Ignore the message in ECBM.
            // It is for LTE only mode. For 1xRTT, incoming pages should be ignored in the modem.
            Log.d(TAG, "ignoring alert of type " + message.getServiceCategory() + " in ECBM");
            return false;
        }
        // Check if the channel is enabled by the user or configuration.
        if (!isChannelEnabled(message)) {
            Log.d(TAG, "ignoring alert of type " + message.getServiceCategory()
                    + " by user preference");
            return false;
        }

        // Check if message body is empty
        String msgBody = message.getMessageBody();
        if (msgBody == null || msgBody.length() == 0) {
            Log.e(TAG, "Empty content or Unsupported charset");
            return false;
        }

        // Check if we need to perform language filtering.
        CellBroadcastChannelManager channelManager = new CellBroadcastChannelManager(mContext,
                message.getSubscriptionId());
        CellBroadcastChannelRange range = channelManager
                .getCellBroadcastChannelRangeFromMessage(message);
        String messageLanguage = message.getLanguageCode();
        if (range != null && range.mFilterLanguage) {
            // language filtering based on CBR second language settings
            final String secondLanguageCode = CellBroadcastSettings.getResources(mContext,
                    message.getSubscriptionId())
                    .getString(R.string.emergency_alert_second_language_code);
            if (!secondLanguageCode.isEmpty()) {
                SharedPreferences prefs = PreferenceManager.getDefaultSharedPreferences(this);
                boolean receiveInSecondLanguage = prefs.getBoolean(
                        CellBroadcastSettings.KEY_RECEIVE_CMAS_IN_SECOND_LANGUAGE, false);
                // For DCS values that bit 6 is 1 and bit 7 is 0, language field is not defined so
                // ap receives it as null value and so alert is not shown to the user.
                // bypass language filter in this case.
                if (!TextUtils.isEmpty(messageLanguage)
                        && !secondLanguageCode.equalsIgnoreCase(messageLanguage)) {
                    Log.w(TAG, "Ignoring message in the unspecified second language:"
                            + messageLanguage);
                    return false;
                } else if (!receiveInSecondLanguage) {
                    Log.d(TAG, "Ignoring message in second language because setting is off");
                    return false;
                }
            } else {
                // language filtering based on device language settings.
                String deviceLanguage = Locale.getDefault().getLanguage();
                // Apply If the message's language does not match device's message, we don't
                // display the message.
                if (!TextUtils.isEmpty(messageLanguage)
                        && !messageLanguage.equalsIgnoreCase(deviceLanguage)) {
                    Log.d(TAG, "ignoring the alert due to language mismatch. Message lang="
                            + messageLanguage + ", device lang=" + deviceLanguage);
                    return false;
                }
            }
        }

        // If the alert is set for test-mode only, then we should check if device is currently under
        // testing mode (testing mode can be enabled by dialer code *#*#CMAS#*#*.
        if (range != null && range.mTestMode && !CellBroadcastReceiver.isTestingMode(mContext)) {
            Log.d(TAG, "ignoring the alert due to not in testing mode");
            return false;
        }

        // Check for custom filtering
        String messageFilters = SystemProperties.get(MESSAGE_FILTER_PROPERTY_KEY, "");
        if (!TextUtils.isEmpty(messageFilters)) {
            String[] filters = messageFilters.split(",");
            for (String filter : filters) {
                if (!TextUtils.isEmpty(filter)) {
                    if (message.getMessageBody().toLowerCase().contains(filter)) {
                        Log.i(TAG, "Skipped message due to filter: " + filter);
                        return false;
                    }
                }
            }
        }

        return true;
    }

    private void handleCellBroadcastIntent(Intent intent) {
        Bundle extras = intent.getExtras();
        if (extras == null) {
            Log.e(TAG, "received SMS_CB_RECEIVED_ACTION with no extras!");
            return;
        }

        SmsCbMessage message = (SmsCbMessage) extras.get(EXTRA_MESSAGE);

        if (message == null) {
            Log.e(TAG, "received SMS_CB_RECEIVED_ACTION with no message extra");
            return;
        }

        if (message.getMessageFormat() == MESSAGE_FORMAT_3GPP) {
            CellBroadcastStatsLog.write(CellBroadcastStatsLog.CB_MESSAGE_REPORTED,
                    CellBroadcastStatsLog.CELL_BROADCAST_MESSAGE_REPORTED__TYPE__GSM,
                    CellBroadcastStatsLog.CELL_BROADCAST_MESSAGE_REPORTED__SOURCE__CB_RECEIVER_APP);
        } else if (message.getMessageFormat() == MESSAGE_FORMAT_3GPP2) {
            CellBroadcastStatsLog.write(CellBroadcastStatsLog.CB_MESSAGE_REPORTED,
                    CellBroadcastStatsLog.CELL_BROADCAST_MESSAGE_REPORTED__TYPE__CDMA,
                    CellBroadcastStatsLog.CELL_BROADCAST_MESSAGE_REPORTED__SOURCE__CB_RECEIVER_APP);
        }

        if (!shouldDisplayMessage(message)) {
            return;
        }

        final Intent alertIntent = new Intent(SHOW_NEW_ALERT_ACTION);
        alertIntent.setClass(this, CellBroadcastAlertService.class);
        alertIntent.putExtra(EXTRA_MESSAGE, message);

        // write to database on a background thread
        new CellBroadcastContentProvider.AsyncCellBroadcastTask(getContentResolver())
                .execute((CellBroadcastContentProvider.CellBroadcastOperation) provider -> {
                    CellBroadcastChannelManager channelManager =
                            new CellBroadcastChannelManager(mContext, message.getSubscriptionId());
                    CellBroadcastChannelRange range = channelManager
                            .getCellBroadcastChannelRangeFromMessage(message);
                    // Check if the message was marked as do not display. Some channels
                    // are reserved for biz purpose where the msg should be routed as a data SMS
                    // rather than being displayed as pop-up or notification. However,
                    // per requirements those messages might also need to write to sms inbox...
                    boolean ret = false;
                    if (range != null && range.mDisplay == true) {
                        if (provider.insertNewBroadcast(message)) {
                            // new message, show the alert or notification on UI thread
                            // if not display..
                            startService(alertIntent);
                            // mark the message as displayed to the user.
                            markMessageDisplayed(message);
                            ret = true;
                        }
                    } else {
                        Log.d(TAG, "ignoring the alert due to configured channels was marked "
                                + "as do not display");
                    }
                    if (CellBroadcastSettings.getResources(mContext, message.getSubscriptionId())
                            .getBoolean(R.bool.enable_write_alerts_to_sms_inbox)) {
                        if (CellBroadcastReceiver.isTestingMode(getApplicationContext())
                                || (range != null && range.mWriteToSmsInbox)) {
                            provider.writeMessageToSmsInbox(message, mContext);
                        }
                    }
                    return ret;
                });
    }

    /**
     * Mark the message as displayed in cell broadcast service's database.
     *
     * @param message The cell broadcast message.
     */
    private void markMessageDisplayed(SmsCbMessage message) {
        mContext.getContentResolver().update(
                Uri.withAppendedPath(Telephony.CellBroadcasts.CONTENT_URI,"displayed"),
                new ContentValues(),
                Telephony.CellBroadcasts.RECEIVED_TIME + "=?",
                new String[] {Long.toString(message.getReceivedTime())});
    }

    private void showNewAlert(Intent intent) {
        Bundle extras = intent.getExtras();
        if (extras == null) {
            Log.e(TAG, "received SHOW_NEW_ALERT_ACTION with no extras!");
            return;
        }

        SmsCbMessage cbm = intent.getParcelableExtra(EXTRA_MESSAGE);

        if (cbm == null) {
            Log.e(TAG, "received SHOW_NEW_ALERT_ACTION with no message extra");
            return;
        }

        if (mTelephonyManager.getCallState() != TelephonyManager.CALL_STATE_IDLE
                && CellBroadcastSettings.getResources(mContext, cbm.getSubscriptionId())
                .getBoolean(R.bool.enable_alert_handling_during_call)) {
            Log.d(TAG, "CMAS received in dialing/during voicecall.");
            sRemindAfterCallFinish = true;
        }

        // Either shown the dialog, adding it to notification (non emergency, or delayed emergency),
        CellBroadcastChannelManager channelManager = new CellBroadcastChannelManager(
                mContext, cbm.getSubscriptionId());
        if (channelManager.isEmergencyMessage(cbm) && !sRemindAfterCallFinish) {
            // start alert sound / vibration / TTS and display full-screen alert
            openEmergencyAlertNotification(cbm);
            Resources res = CellBroadcastSettings.getResources(mContext, cbm.getSubscriptionId());
            // KR carriers mandate to always show notifications along with alert dialog.
            if (res.getBoolean(R.bool.show_alert_dialog_with_notification) ||
                    // to support emergency alert on companion devices use flag
                    // show_notification_if_connected_to_companion_devices instead.
                    (res.getBoolean(R.bool.show_notification_if_connected_to_companion_devices)
                            && isConnectedToCompanionDevices())) {
                // add notification to the bar by passing the list of unread non-emergency
                // cell broadcast messages. The notification should be of LOW_IMPORTANCE if the
                // notification is shown together with full-screen dialog.
                addToNotificationBar(cbm, CellBroadcastReceiverApp.addNewMessageToList(cbm),
                        this, false, true, shouldDisplayFullScreenMessage(cbm));
            }
        } else {
            // add notification to the bar by passing the list of unread non-emergency
            // cell broadcast messages
            ArrayList<SmsCbMessage> messageList = CellBroadcastReceiverApp
                    .addNewMessageToList(cbm);
            addToNotificationBar(cbm, messageList, this, false, true, false);
        }
    }

    /**
     * Check if the message's channel is enabled on the device.
     *
     * @param message the message to check
     * @return true if the channel is enabled on the device, otherwise false.
     */
    private boolean isChannelEnabled(SmsCbMessage message) {
        CellBroadcastChannelManager channelManager = new CellBroadcastChannelManager(mContext,
                message.getSubscriptionId());
        CellBroadcastChannelRange chanelrange = channelManager
                .getCellBroadcastChannelRangeFromMessage(message);
        Resources res = CellBroadcastSettings.getResources(mContext, message.getSubscriptionId());
        if (chanelrange != null && chanelrange.mAlwaysOn) {
            Log.d(TAG, "channel is enabled due to always-on, ignoring preference check");
            return true;
        }

        SharedPreferences prefs = PreferenceManager.getDefaultSharedPreferences(this);
        // Check if all emergency alerts are disabled.
        boolean emergencyAlertEnabled =
                prefs.getBoolean(CellBroadcastSettings.KEY_ENABLE_ALERTS_MASTER_TOGGLE, true);
        int channel = message.getServiceCategory();

        SmsCbEtwsInfo etwsInfo = message.getEtwsWarningInfo();
        if ((etwsInfo != null && etwsInfo.getWarningType()
                == SmsCbEtwsInfo.ETWS_WARNING_TYPE_TEST_MESSAGE)
                || channelManager.checkCellBroadcastChannelRange(channel,
                R.array.etws_test_alerts_range_strings)) {
            return emergencyAlertEnabled
                    && CellBroadcastSettings.isTestAlertsToggleVisible(getApplicationContext())
                    && PreferenceManager.getDefaultSharedPreferences(this)
                    .getBoolean(CellBroadcastSettings.KEY_ENABLE_TEST_ALERTS, false);
        }

        if (message.isEtwsMessage() || channelManager.checkCellBroadcastChannelRange(channel,
                R.array.etws_alerts_range_strings)) {
            // ETWS messages.
            // Turn on/off emergency notifications is the only way to turn on/off ETWS messages.
            return emergencyAlertEnabled;
        }

        // Check if the messages are on additional channels enabled by the resource config.
        // If those channels are enabled by the carrier, but the device is actually roaming, we
        // should not allow the messages.
        ArrayList<CellBroadcastChannelRange> ranges = channelManager.getCellBroadcastChannelRanges(
                R.array.additional_cbs_channels_strings);

        for (CellBroadcastChannelRange range : ranges) {
            if (range.mStartId <= channel && range.mEndId >= channel) {
                // Check if the channel is within the scope. If not, ignore the alert message.
                if (!channelManager.checkScope(range.mScope)) {
                    Log.d(TAG, "The range [" + range.mStartId + "-" + range.mEndId
                            + "] is not within the scope. mScope = " + range.mScope);
                    return false;
                }

                if (range.mAlertType == AlertType.TEST) {
                    return emergencyAlertEnabled
                            && CellBroadcastSettings.isTestAlertsToggleVisible(
                                    getApplicationContext())
                            && PreferenceManager.getDefaultSharedPreferences(this)
                            .getBoolean(CellBroadcastSettings.KEY_ENABLE_TEST_ALERTS,
                                    false);
                }
                if (range.mAlertType == AlertType.AREA) {
                    return emergencyAlertEnabled && PreferenceManager
                            .getDefaultSharedPreferences(this)
                            .getBoolean(CellBroadcastSettings.KEY_ENABLE_AREA_UPDATE_INFO_ALERTS,
                                    false);
                }

                return emergencyAlertEnabled;
            }
        }

        if (channelManager.checkCellBroadcastChannelRange(channel,
                R.array.emergency_alerts_channels_range_strings)) {
            return emergencyAlertEnabled
                    && PreferenceManager.getDefaultSharedPreferences(this).getBoolean(
                            CellBroadcastSettings.KEY_ENABLE_EMERGENCY_ALERTS, true);
        }
        // CMAS warning types
        if (channelManager.checkCellBroadcastChannelRange(channel,
                R.array.cmas_presidential_alerts_channels_range_strings)) {
            // always enabled
            return true;
        }
        if (channelManager.checkCellBroadcastChannelRange(channel,
                R.array.cmas_alert_extreme_channels_range_strings)) {
            return emergencyAlertEnabled
                    && PreferenceManager.getDefaultSharedPreferences(this).getBoolean(
                            CellBroadcastSettings.KEY_ENABLE_CMAS_EXTREME_THREAT_ALERTS, true);
        }
        if (channelManager.checkCellBroadcastChannelRange(channel,
                R.array.cmas_alerts_severe_range_strings)) {
            return emergencyAlertEnabled
                    && PreferenceManager.getDefaultSharedPreferences(this).getBoolean(
                            CellBroadcastSettings.KEY_ENABLE_CMAS_SEVERE_THREAT_ALERTS, true);
        }
        if (channelManager.checkCellBroadcastChannelRange(channel,
                R.array.cmas_amber_alerts_channels_range_strings)) {
            return emergencyAlertEnabled
                    && PreferenceManager.getDefaultSharedPreferences(this)
                            .getBoolean(CellBroadcastSettings.KEY_ENABLE_CMAS_AMBER_ALERTS, true);
        }

        if (channelManager.checkCellBroadcastChannelRange(
                channel, R.array.exercise_alert_range_strings) &&
                res.getBoolean(R.bool.show_separate_exercise_settings)) {
            return emergencyAlertEnabled && PreferenceManager.getDefaultSharedPreferences(this)
                    .getBoolean(CellBroadcastSettings.KEY_ENABLE_EXERCISE_ALERTS, false);
        }

        if (channelManager.checkCellBroadcastChannelRange(
                channel, R.array.operator_defined_alert_range_strings) &&
                res.getBoolean(R.bool.show_separate_operator_defined_settings)) {
            return emergencyAlertEnabled && PreferenceManager.getDefaultSharedPreferences(this)
                    .getBoolean(CellBroadcastSettings.KEY_OPERATOR_DEFINED_ALERTS, false);
        }

        if (channelManager.checkCellBroadcastChannelRange(channel,
                R.array.required_monthly_test_range_strings)
                || channelManager.checkCellBroadcastChannelRange(channel,
                R.array.exercise_alert_range_strings)
                || channelManager.checkCellBroadcastChannelRange(channel,
                R.array.operator_defined_alert_range_strings)) {
            return emergencyAlertEnabled
                    && CellBroadcastSettings.isTestAlertsToggleVisible(getApplicationContext())
                    && PreferenceManager.getDefaultSharedPreferences(this)
                            .getBoolean(CellBroadcastSettings.KEY_ENABLE_TEST_ALERTS,
                                    false);
        }

        if (channelManager.checkCellBroadcastChannelRange(channel,
                R.array.public_safety_messages_channels_range_strings)) {
            return emergencyAlertEnabled
                    && PreferenceManager.getDefaultSharedPreferences(this)
                    .getBoolean(CellBroadcastSettings.KEY_ENABLE_PUBLIC_SAFETY_MESSAGES,
                            true);
        }

        if (channelManager.checkCellBroadcastChannelRange(channel,
                R.array.state_local_test_alert_range_strings)) {
            return emergencyAlertEnabled
                    && PreferenceManager.getDefaultSharedPreferences(this)
                    .getBoolean(CellBroadcastSettings.KEY_ENABLE_STATE_LOCAL_TEST_ALERTS,
                            false);
        }

        Log.e(TAG, "received undefined channels: " + channel);
        return false;
    }

    /**
     * Display an alert message for emergency alerts.
     * @param message the alert to display
     */
    private void openEmergencyAlertNotification(SmsCbMessage message) {
        if (!shouldDisplayFullScreenMessage(message)) {
            Log.d(TAG, "openEmergencyAlertNotification: do not show full screen alert "
                    + "due to user preference");
            return;
        }
        // Close dialogs and window shade
        Intent closeDialogs = new Intent(Intent.ACTION_CLOSE_SYSTEM_DIALOGS);
        sendBroadcast(closeDialogs);

        // start audio/vibration/speech service for emergency alerts
        Intent audioIntent = new Intent(this, CellBroadcastAlertAudio.class);
        audioIntent.setAction(CellBroadcastAlertAudio.ACTION_START_ALERT_AUDIO);
        SharedPreferences prefs = PreferenceManager.getDefaultSharedPreferences(this);

        CellBroadcastChannelManager channelManager = new CellBroadcastChannelManager(
                mContext, message.getSubscriptionId());

        AlertType alertType = AlertType.DEFAULT;
        if (message.isEtwsMessage()) {
            alertType = AlertType.ETWS_DEFAULT;

            if (message.getEtwsWarningInfo() != null) {
                int warningType = message.getEtwsWarningInfo().getWarningType();

                switch (warningType) {
                    case SmsCbEtwsInfo.ETWS_WARNING_TYPE_EARTHQUAKE:
                    case SmsCbEtwsInfo.ETWS_WARNING_TYPE_EARTHQUAKE_AND_TSUNAMI:
                        alertType = AlertType.ETWS_EARTHQUAKE;
                        break;
                    case SmsCbEtwsInfo.ETWS_WARNING_TYPE_TSUNAMI:
                        alertType = AlertType.ETWS_TSUNAMI;
                        break;
                    case SmsCbEtwsInfo.ETWS_WARNING_TYPE_TEST_MESSAGE:
                        alertType = AlertType.TEST;
                        break;
                    case SmsCbEtwsInfo.ETWS_WARNING_TYPE_OTHER_EMERGENCY:
                        alertType = AlertType.OTHER;
                        break;
                }
            }
        } else {
            int channel = message.getServiceCategory();
            ArrayList<CellBroadcastChannelRange> ranges = channelManager
                    .getAllCellBroadcastChannelRanges();
            for (CellBroadcastChannelRange range : ranges) {
                if (channel >= range.mStartId && channel <= range.mEndId) {
                    alertType = range.mAlertType;
                    break;
                }
            }
        }
        CellBroadcastChannelRange range = channelManager
                .getCellBroadcastChannelRangeFromMessage(message);
        audioIntent.putExtra(CellBroadcastAlertAudio.ALERT_AUDIO_TONE_TYPE, alertType);
        audioIntent.putExtra(
                CellBroadcastAlertAudio.ALERT_AUDIO_VIBRATION_PATTERN_EXTRA,
                (range != null)
                        ? range.mVibrationPattern
                        : CellBroadcastSettings.getResources(mContext, message.getSubscriptionId())
                        .getIntArray(R.array.default_vibration_pattern));
        // read key_override_dnd only when the toggle is visible.
        // range.mOverrideDnd is per channel configuration. override_dnd is the main config
        // applied for all channels.
        Resources res = CellBroadcastSettings.getResources(mContext, message.getSubscriptionId());
        if ((res.getBoolean(R.bool.show_override_dnd_settings)
                && prefs.getBoolean(CellBroadcastSettings.KEY_OVERRIDE_DND, false))
                || (range != null && range.mOverrideDnd) || res.getBoolean(R.bool.override_dnd)) {
            audioIntent.putExtra(CellBroadcastAlertAudio.ALERT_AUDIO_OVERRIDE_DND_EXTRA, true);
        }

        String messageBody = message.getMessageBody();

        if (!CellBroadcastSettings.getResourcesForDefaultSubId(mContext)
                .getBoolean(R.bool.show_alert_speech_setting)
                || prefs.getBoolean(CellBroadcastSettings.KEY_ENABLE_ALERT_SPEECH, true)) {
            audioIntent.putExtra(CellBroadcastAlertAudio.ALERT_AUDIO_MESSAGE_BODY, messageBody);

            String language = message.getLanguageCode();

            Log.d(TAG, "Message language = " + language);
            audioIntent.putExtra(CellBroadcastAlertAudio.ALERT_AUDIO_MESSAGE_LANGUAGE,
                    language);
        }

        audioIntent.putExtra(CellBroadcastAlertAudio.ALERT_AUDIO_SUB_INDEX,
                message.getSubscriptionId());
        audioIntent.putExtra(CellBroadcastAlertAudio.ALERT_AUDIO_DURATION,
                (range != null) ? range.mAlertDuration : -1);
        startService(audioIntent);

        ArrayList<SmsCbMessage> messageList = new ArrayList<>();
        messageList.add(message);

        // For FEATURE_WATCH, the dialog doesn't make sense from a UI/UX perspective
        if (getPackageManager().hasSystemFeature(PackageManager.FEATURE_WATCH)) {
            addToNotificationBar(message, messageList, this, false, true, false);
        } else {
            Intent alertDialogIntent = createDisplayMessageIntent(this,
                    CellBroadcastAlertDialog.class, messageList);
            alertDialogIntent.addFlags(Intent.FLAG_ACTIVITY_NEW_TASK);
            startActivity(alertDialogIntent);
        }

    }

    /**
     * Add the new alert to the notification bar (non-emergency alerts), launch a
     * high-priority immediate intent for emergency alerts or notifications for companion devices.
     * @param message the alert to display
     * @param shouldAlert only notify once if set to {@code false}.
     * @param fromDialog if {@code true} indicate this notification is coming from the alert dialog
     * with following behaviors:
     * 1. display when alert is shown in the foreground.
     * 2. dismiss when foreground alert is gone.
     * 3. dismiss foreground alert when swipe away the notification.
     * 4. no dialog open when tap the notification.
     */
    static void addToNotificationBar(SmsCbMessage message,
            ArrayList<SmsCbMessage> messageList, Context context,
            boolean fromSaveState, boolean shouldAlert, boolean fromDialog) {
        Resources res = CellBroadcastSettings.getResources(context, message.getSubscriptionId());
        int channelTitleId = CellBroadcastResources.getDialogTitleResource(context, message);
        CharSequence channelName = context.getText(channelTitleId);
        String messageBody = message.getMessageBody();
        final NotificationManager notificationManager =
                (NotificationManager) context.getSystemService(Context.NOTIFICATION_SERVICE);
        createNotificationChannels(context);

        boolean isWatch = context.getPackageManager()
                .hasSystemFeature(PackageManager.FEATURE_WATCH);
        // Create intent to show the new messages when user selects the notification.
        Intent intent;
        if (isWatch) {
            // For FEATURE_WATCH we want to mark as read
            intent = createMarkAsReadIntent(context, message.getReceivedTime());
        } else {
            // For anything else we handle it normally
            intent = createDisplayMessageIntent(context, CellBroadcastAlertDialog.class,
                    messageList);
        }

        // if this is an notification from on-going alert alert, do not clear the notification when
        // tap the notification. the notification should be gone either when users swipe away or
        // when the foreground dialog dismissed.
        intent.putExtra(CellBroadcastAlertDialog.DISMISS_NOTIFICATION_EXTRA, !fromDialog);
        intent.putExtra(CellBroadcastAlertDialog.FROM_SAVE_STATE_NOTIFICATION_EXTRA, fromSaveState);

        PendingIntent pi;
        if (isWatch) {
            pi = PendingIntent.getBroadcast(context, 0, intent, 0);
        } else {
            pi = PendingIntent.getActivity(context, REQUEST_CODE_CONTENT_INTENT, intent,
                    PendingIntent.FLAG_ONE_SHOT
                            | PendingIntent.FLAG_UPDATE_CURRENT
                            | PendingIntent.FLAG_IMMUTABLE);
        }
        CellBroadcastChannelManager channelManager = new CellBroadcastChannelManager(
                context, message.getSubscriptionId());

        String channelId;
        if (!channelManager.isEmergencyMessage(message)) {
            channelId = NOTIFICATION_CHANNEL_NON_EMERGENCY_ALERTS;
        } else if (sRemindAfterCallFinish) {
            channelId = NOTIFICATION_CHANNEL_EMERGENCY_ALERTS_IN_VOICECALL;
        } else if (fromDialog) {
            channelId = NOTIFICATION_CHANNEL_EMERGENCY_ALERTS;
        } else {
            channelId = NOTIFICATION_CHANNEL_HIGH_PRIORITY_EMERGENCY_ALERTS;
        }

        boolean nonSwipeableNotification = message.isEmergencyMessage()
                && CellBroadcastSettings.getResources(context, message.getSubscriptionId())
                .getBoolean(R.bool.non_swipeable_notification) || sRemindAfterCallFinish;

        // use default sound/vibration/lights for non-emergency broadcasts
        Notification.Builder builder =
                new Notification.Builder(context, channelId)
                        .setSmallIcon(R.drawable.ic_warning_googred)
                        .setTicker(channelName)
                        .setWhen(System.currentTimeMillis())
                        .setCategory(Notification.CATEGORY_SYSTEM)
                        .setPriority(Notification.PRIORITY_HIGH)
                        .setColor(res.getColor(R.color.notification_color))
                        .setVisibility(Notification.VISIBILITY_PUBLIC)
                        .setOngoing(nonSwipeableNotification)
                        .setOnlyAlertOnce(!shouldAlert);

        if (isWatch) {
            builder.setDeleteIntent(pi);
            // FEATURE_WATCH/CWH devices see this as priority
            builder.setVibrate(new long[]{0});
        } else {
            // If this is a notification coming from the foreground dialog, should dismiss the
            // foreground alert dialog when swipe the notification. This is needed
            // when receiving emergency alerts on companion devices are supported, so that users
            // swipe away notification on companion devices will synced to the parent devices
            // with the foreground dialog/sound/vibration dismissed and stopped. Delete intent is
            // also needed for regular notifications (e.g, pressing home button) to stop the
            // sound, vibration and alert reminder.
            Intent deleteIntent = new Intent(intent);
            deleteIntent.putExtra(CellBroadcastAlertService.DISMISS_DIALOG, true);
            builder.setDeleteIntent(PendingIntent.getActivity(context, REQUEST_CODE_DELETE_INTENT,
                    deleteIntent, PendingIntent.FLAG_ONE_SHOT
                            | PendingIntent.FLAG_UPDATE_CURRENT
                            | PendingIntent.FLAG_IMMUTABLE));

            builder.setContentIntent(pi);
            // This will break vibration on FEATURE_WATCH, so use it for anything else
            builder.setDefaults(Notification.DEFAULT_ALL);
        }

        // increment unread alert count (decremented when user dismisses alert dialog)
        int unreadCount = messageList.size();
        if (unreadCount > 1) {
            // use generic count of unread broadcasts if more than one unread
            builder.setContentTitle(context.getString(R.string.notification_multiple_title));
            builder.setContentText(context.getString(R.string.notification_multiple, unreadCount));
        } else {
            builder.setContentTitle(channelName)
                    .setContentText(messageBody)
                    .setStyle(new Notification.BigTextStyle()
                            .bigText(messageBody));
        }

        notificationManager.notify(NOTIFICATION_ID, builder.build());

        // FEATURE_WATCH devices do not have global sounds for notifications; only vibrate.
        // TW requires sounds for 911/919
        // Emergency messages use a different audio playback and display path. Since we use
        // addToNotification for the emergency display on FEATURE WATCH devices vs the
        // Alert Dialog, it will call this and override the emergency audio tone.
        if (isWatch && !channelManager.isEmergencyMessage(message)) {
            if (res.getBoolean(R.bool.watch_enable_non_emergency_audio)) {
                // start audio/vibration/speech service for non emergency alerts
                Intent audioIntent = new Intent(context, CellBroadcastAlertAudio.class);
                audioIntent.setAction(CellBroadcastAlertAudio.ACTION_START_ALERT_AUDIO);
                audioIntent.putExtra(CellBroadcastAlertAudio.ALERT_AUDIO_TONE_TYPE,
                        AlertType.OTHER);
                context.startService(audioIntent);
            }
        }

    }

    /**
     * Creates the notification channel and registers it with NotificationManager. If a channel
     * with the same ID is already registered, NotificationManager will ignore this call.
     */
    static void createNotificationChannels(Context context) {
        NotificationManager notificationManager =
                (NotificationManager) context.getSystemService(Context.NOTIFICATION_SERVICE);
        notificationManager.createNotificationChannel(
                new NotificationChannel(
                        NOTIFICATION_CHANNEL_HIGH_PRIORITY_EMERGENCY_ALERTS,
                        context.getString(
                                R.string.notification_channel_emergency_alerts_high_priority),
                        NotificationManager.IMPORTANCE_HIGH));
        notificationManager.createNotificationChannel(
                new NotificationChannel(
                        NOTIFICATION_CHANNEL_EMERGENCY_ALERTS,
                        context.getString(R.string.notification_channel_emergency_alerts),
                        NotificationManager.IMPORTANCE_LOW));
        final NotificationChannel nonEmergency = new NotificationChannel(
                NOTIFICATION_CHANNEL_NON_EMERGENCY_ALERTS,
                context.getString(R.string.notification_channel_broadcast_messages),
                NotificationManager.IMPORTANCE_DEFAULT);
        nonEmergency.enableVibration(true);
        notificationManager.createNotificationChannel(nonEmergency);

        final NotificationChannel emergencyAlertInVoiceCall = new NotificationChannel(
            NOTIFICATION_CHANNEL_EMERGENCY_ALERTS_IN_VOICECALL,
            context.getString(R.string.notification_channel_broadcast_messages_in_voicecall),
            NotificationManager.IMPORTANCE_HIGH);
        emergencyAlertInVoiceCall.enableVibration(true);
        notificationManager.createNotificationChannel(emergencyAlertInVoiceCall);

        final NotificationChannel settingsUpdate = new NotificationChannel(
                NOTIFICATION_CHANNEL_SETTINGS_UPDATES,
                context.getString(R.string.notification_channel_settings_updates),
                NotificationManager.IMPORTANCE_DEFAULT);
        notificationManager.createNotificationChannel(settingsUpdate);
    }


    private static Intent createDisplayMessageIntent(Context context, Class intentClass,
            ArrayList<SmsCbMessage> messageList) {
        // Trigger the list activity to fire up a dialog that shows the received messages
        Intent intent = new Intent(context, intentClass);
        intent.putParcelableArrayListExtra(CellBroadcastAlertService.SMS_CB_MESSAGE_EXTRA,
                messageList);
        intent.addFlags(Intent.FLAG_ACTIVITY_NO_USER_ACTION);
        return intent;
    }

    /**
     * Creates a delete intent that calls to the {@link CellBroadcastReceiver} in order to mark
     * a message as read
     *
     * @param context context of the caller
     * @param deliveryTime time the message was sent in order to mark as read
     * @return delete intent to add to the pending intent
     */
    static Intent createMarkAsReadIntent(Context context, long deliveryTime) {
        Intent deleteIntent = new Intent(context, CellBroadcastInternalReceiver.class);
        deleteIntent.setAction(CellBroadcastReceiver.ACTION_MARK_AS_READ);
        deleteIntent.putExtra(CellBroadcastReceiver.EXTRA_DELIVERY_TIME, deliveryTime);
        return deleteIntent;
    }

    @VisibleForTesting
    @Override
    public IBinder onBind(Intent intent) {
        return new LocalBinder();
    }

    @VisibleForTesting
    class LocalBinder extends Binder {
        public CellBroadcastAlertService getService() {
            return CellBroadcastAlertService.this;
        }
    }

    /**
     * Remove previous unread notifications and play stored unread
     * emergency messages after voice call finish.
     */
    private final PhoneStateListener mPhoneStateListener = new PhoneStateListener(
        new Handler(Looper.getMainLooper())::post) {
        @Override
        public void onCallStateChanged(int state, String incomingNumber) {

            switch (state) {
                case TelephonyManager.CALL_STATE_IDLE:
                    Log.d(TAG, "onCallStateChanged: CALL_STATE_IDLE");
                    playPendingAlert();
                    break;

                default:
                    Log.d(TAG, "onCallStateChanged: other state = " + state);
                    break;
            }
        }
    };

    private void playPendingAlert() {
        if (sRemindAfterCallFinish) {
            sRemindAfterCallFinish = false;
            NotificationManager notificationManager = (NotificationManager)
                    getApplicationContext().getSystemService(
                            Context.NOTIFICATION_SERVICE);

            StatusBarNotification[] notificationList =
                    notificationManager.getActiveNotifications();

            if(notificationList != null && notificationList.length >0) {
                notificationManager.cancel(CellBroadcastAlertService.NOTIFICATION_ID);
                ArrayList<SmsCbMessage> newMessageList =
                        CellBroadcastReceiverApp.getNewMessageList();

                for (int i = 0; i < newMessageList.size(); i++) {
                    openEmergencyAlertNotification(newMessageList.get(i));
                }
            }
            CellBroadcastReceiverApp.clearNewMessageList();
        }
    }

    private boolean isConnectedToCompanionDevices() {
        BluetoothManager bluetoothMgr = getSystemService(BluetoothManager.class);
        Set<BluetoothDevice> devices;
        try {
            devices = bluetoothMgr.getAdapter().getBondedDevices();
        } catch (SecurityException ex) {
            // running on S+ will need runtime permission grant
            // always return true here assuming there is connected devices to show alert in case
            // of permission denial.
            return true;
        }

        // TODO: filter out specific device types like wearable. no API support now.
        for (BluetoothDevice device : devices) {
            if (device.isConnected()) {
                Log.d(TAG, "connected to device: " + device.getName());
                return true;
            }
        }
        return false;
    }
}
