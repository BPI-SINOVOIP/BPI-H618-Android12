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

package com.android.cellbroadcastreceiver;

import android.annotation.IntDef;
import android.annotation.NonNull;
import android.app.Activity;
import android.app.AlertDialog;
import android.app.KeyguardManager;
import android.app.NotificationManager;
import android.app.PendingIntent;
import android.app.RemoteAction;
import android.app.StatusBarManager;
import android.content.BroadcastReceiver;
import android.content.ClipData;
import android.content.ClipboardManager;
import android.content.Context;
import android.content.Intent;
import android.content.IntentFilter;
import android.content.SharedPreferences;
import android.content.res.Configuration;
import android.content.res.Resources;
import android.graphics.Point;
import android.graphics.drawable.Drawable;
import android.os.Bundle;
import android.os.Handler;
import android.os.Message;
import android.os.PowerManager;
import android.preference.PreferenceManager;
import android.provider.Telephony;
import android.telephony.SmsCbCmasInfo;
import android.telephony.SmsCbMessage;
import android.telephony.TelephonyManager;
import android.text.Spannable;
import android.text.SpannableString;
import android.text.TextUtils;
import android.text.format.DateUtils;
import android.text.method.LinkMovementMethod;
import android.text.style.ClickableSpan;
import android.text.util.Linkify;
import android.util.Log;
import android.view.Display;
import android.view.Gravity;
import android.view.KeyEvent;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;
import android.view.Window;
import android.view.WindowManager;
import android.view.textclassifier.TextClassification;
import android.view.textclassifier.TextClassification.Request;
import android.view.textclassifier.TextClassifier;
import android.view.textclassifier.TextLinks;
import android.view.textclassifier.TextLinks.TextLink;
import android.widget.ImageView;
import android.widget.TextView;
import android.widget.Toast;

import com.android.cellbroadcastreceiver.CellBroadcastChannelManager.CellBroadcastChannelRange;
import com.android.internal.annotations.VisibleForTesting;

import java.lang.annotation.Retention;
import java.lang.annotation.RetentionPolicy;
import java.text.SimpleDateFormat;
import java.util.ArrayList;
import java.util.Arrays;
import java.util.Collections;
import java.util.Comparator;
import java.util.Locale;
import java.util.concurrent.atomic.AtomicInteger;

/**
 * Custom alert dialog with optional flashing warning icon.
 * Alert audio and text-to-speech handled by {@link CellBroadcastAlertAudio}.
 */
public class CellBroadcastAlertDialog extends Activity {

    private static final String TAG = "CellBroadcastAlertDialog";

    /** Intent extra indicate this intent should not dismiss the notification */
    @VisibleForTesting
    public static final String DISMISS_NOTIFICATION_EXTRA = "dismiss_notification";

    // Intent extra to identify if notification was sent while trying to move away from the dialog
    //  without acknowledging the dialog
    static final String FROM_SAVE_STATE_NOTIFICATION_EXTRA = "from_save_state_notification";

    /** Not link any text. */
    private static final int LINK_METHOD_NONE = 0;

    private static final String LINK_METHOD_NONE_STRING = "none";

    /** Use {@link android.text.util.Linkify} to generate links. */
    private static final int LINK_METHOD_LEGACY_LINKIFY = 1;

    private static final String LINK_METHOD_LEGACY_LINKIFY_STRING = "legacy_linkify";

    /**
     * Use the machine learning based {@link TextClassifier} to generate links. Will fallback to
     * {@link #LINK_METHOD_LEGACY_LINKIFY} if not enabled.
     */
    private static final int LINK_METHOD_SMART_LINKIFY = 2;

    private static final String LINK_METHOD_SMART_LINKIFY_STRING = "smart_linkify";

    /**
     * Use the machine learning based {@link TextClassifier} to generate links but hiding copy
     * option. Will fallback to
     * {@link #LINK_METHOD_LEGACY_LINKIFY} if not enabled.
     */
    private static final int LINK_METHOD_SMART_LINKIFY_NO_COPY = 3;

    private static final String LINK_METHOD_SMART_LINKIFY_NO_COPY_STRING = "smart_linkify_no_copy";


    /**
     * Text link method
     * @hide
     */
    @Retention(RetentionPolicy.SOURCE)
    @IntDef(prefix = "LINK_METHOD_",
            value = {LINK_METHOD_NONE, LINK_METHOD_LEGACY_LINKIFY,
                    LINK_METHOD_SMART_LINKIFY, LINK_METHOD_SMART_LINKIFY_NO_COPY})
    private @interface LinkMethod {}


    /** List of cell broadcast messages to display (oldest to newest). */
    protected ArrayList<SmsCbMessage> mMessageList;

    /** Whether a CMAS alert other than Presidential Alert was displayed. */
    private boolean mShowOptOutDialog;

    /** Length of time for the warning icon to be visible. */
    private static final int WARNING_ICON_ON_DURATION_MSEC = 800;

    /** Length of time for the warning icon to be off. */
    private static final int WARNING_ICON_OFF_DURATION_MSEC = 800;

    /** Length of time to keep the screen turned on. */
    private static final int KEEP_SCREEN_ON_DURATION_MSEC = 60000;

    /** Animation handler for the flashing warning icon (emergency alerts only). */
    @VisibleForTesting
    public AnimationHandler mAnimationHandler = new AnimationHandler();

    /** Handler to add and remove screen on flags for emergency alerts. */
    private final ScreenOffHandler mScreenOffHandler = new ScreenOffHandler();

    // Show the opt-out dialog
    private AlertDialog mOptOutDialog;

    /** BroadcastReceiver for screen off events. When screen was off, remove FLAG_TURN_SCREEN_ON to
     * start from a clean state. Otherwise, the window flags from the first alert will be
     * automatically applied to the following alerts handled at onNewIntent.
     */
    private BroadcastReceiver mScreenOffReceiver = new BroadcastReceiver() {
        @Override
        public void onReceive(Context context, Intent intent){
            Log.d(TAG, "onSreenOff: remove FLAG_TURN_SCREEN_ON flag");
            getWindow().clearFlags(WindowManager.LayoutParams.FLAG_TURN_SCREEN_ON);
        }
    };

    /**
     * Animation handler for the flashing warning icon (emergency alerts only).
     */
    @VisibleForTesting
    public class AnimationHandler extends Handler {
        /** Latest {@code message.what} value for detecting old messages. */
        @VisibleForTesting
        public final AtomicInteger mCount = new AtomicInteger();

        /** Warning icon state: visible == true, hidden == false. */
        @VisibleForTesting
        public boolean mWarningIconVisible;

        /** The warning icon Drawable. */
        private Drawable mWarningIcon;

        /** The View containing the warning icon. */
        private ImageView mWarningIconView;

        /** Package local constructor (called from outer class). */
        AnimationHandler() {}

        /** Start the warning icon animation. */
        @VisibleForTesting
        public void startIconAnimation(int subId) {
            if (!initDrawableAndImageView(subId)) {
                return;     // init failure
            }
            mWarningIconVisible = true;
            mWarningIconView.setVisibility(View.VISIBLE);
            updateIconState();
            queueAnimateMessage();
        }

        /** Stop the warning icon animation. */
        @VisibleForTesting
        public void stopIconAnimation() {
            // Increment the counter so the handler will ignore the next message.
            mCount.incrementAndGet();
        }

        /** Update the visibility of the warning icon. */
        private void updateIconState() {
            mWarningIconView.setImageAlpha(mWarningIconVisible ? 255 : 0);
            mWarningIconView.invalidateDrawable(mWarningIcon);
        }

        /** Queue a message to animate the warning icon. */
        private void queueAnimateMessage() {
            int msgWhat = mCount.incrementAndGet();
            sendEmptyMessageDelayed(msgWhat, mWarningIconVisible ? WARNING_ICON_ON_DURATION_MSEC
                    : WARNING_ICON_OFF_DURATION_MSEC);
        }

        @Override
        public void handleMessage(Message msg) {
            if (msg.what == mCount.get()) {
                mWarningIconVisible = !mWarningIconVisible;
                updateIconState();
                queueAnimateMessage();
            }
        }

        /**
         * Initialize the Drawable and ImageView fields.
         *
         * @param subId Subscription index
         *
         * @return true if successful; false if any field failed to initialize
         */
        private boolean initDrawableAndImageView(int subId) {
            if (mWarningIcon == null) {
                try {
                    mWarningIcon = CellBroadcastSettings.getResources(getApplicationContext(),
                            subId).getDrawable(R.drawable.ic_warning_googred);
                } catch (Resources.NotFoundException e) {
                    Log.e(TAG, "warning icon resource not found", e);
                    return false;
                }
            }
            if (mWarningIconView == null) {
                mWarningIconView = (ImageView) findViewById(R.id.icon);
                if (mWarningIconView != null) {
                    mWarningIconView.setImageDrawable(mWarningIcon);
                } else {
                    Log.e(TAG, "failed to get ImageView for warning icon");
                    return false;
                }
            }
            return true;
        }
    }

    /**
     * Handler to add {@code FLAG_KEEP_SCREEN_ON} for emergency alerts. After a short delay,
     * remove the flag so the screen can turn off to conserve the battery.
     */
    private class ScreenOffHandler extends Handler {
        /** Latest {@code message.what} value for detecting old messages. */
        private final AtomicInteger mCount = new AtomicInteger();

        /** Package local constructor (called from outer class). */
        ScreenOffHandler() {}

        /** Add screen on window flags and queue a delayed message to remove them later. */
        void startScreenOnTimer(@NonNull SmsCbMessage message) {
            // if screenOnDuration in milliseconds. if set to 0, do not turn screen on.
            int screenOnDuration = KEEP_SCREEN_ON_DURATION_MSEC;
            CellBroadcastChannelManager channelManager = new CellBroadcastChannelManager(
                    getApplicationContext(), message.getSubscriptionId());
            CellBroadcastChannelRange range = channelManager
                    .getCellBroadcastChannelRangeFromMessage(message);
            if (range!= null) {
                screenOnDuration = range.mScreenOnDuration;
            }
            if (screenOnDuration == 0) {
                Log.d(TAG, "screenOnDuration set to 0, do not turn screen on");
                return;
            }
            addWindowFlags();
            int msgWhat = mCount.incrementAndGet();
            removeMessages(msgWhat - 1);    // Remove previous message, if any.
            sendEmptyMessageDelayed(msgWhat, screenOnDuration);
            Log.d(TAG, "added FLAG_KEEP_SCREEN_ON, queued screen off message id " + msgWhat);
        }

        /** Remove the screen on window flags and any queued screen off message. */
        void stopScreenOnTimer() {
            removeMessages(mCount.get());
            clearWindowFlags();
        }

        /** Set the screen on window flags. */
        private void addWindowFlags() {
            getWindow().addFlags(WindowManager.LayoutParams.FLAG_TURN_SCREEN_ON
                    | WindowManager.LayoutParams.FLAG_KEEP_SCREEN_ON);
        }

        /**
         * Clear the keep screen on window flags in order for powersaving but keep TURN_ON_SCREEN_ON
         * to make sure next wake up still turn screen on without unintended onStop triggered at
         * the beginning.
         */
        private void clearWindowFlags() {
            getWindow().clearFlags(WindowManager.LayoutParams.FLAG_KEEP_SCREEN_ON);
        }

        @Override
        public void handleMessage(Message msg) {
            int msgWhat = msg.what;
            if (msgWhat == mCount.get()) {
                clearWindowFlags();
                Log.d(TAG, "removed FLAG_KEEP_SCREEN_ON with id " + msgWhat);
            } else {
                Log.e(TAG, "discarding screen off message with id " + msgWhat);
            }
        }
    }

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        // if this is only to dismiss any pending alert dialog
        if (getIntent().getBooleanExtra(CellBroadcastAlertService.DISMISS_DIALOG, false)) {
            dismissAllFromNotification(getIntent());
            return;
        }

        final Window win = getWindow();

        // We use a custom title, so remove the standard dialog title bar
        win.requestFeature(Window.FEATURE_NO_TITLE);

        // Full screen alerts display above the keyguard and when device is locked.
        win.addFlags(WindowManager.LayoutParams.FLAG_FULLSCREEN
                | WindowManager.LayoutParams.FLAG_SHOW_WHEN_LOCKED
                | WindowManager.LayoutParams.FLAG_DISMISS_KEYGUARD);

        // Disable home button when alert dialog is showing if mute_by_physical_button is false.
        if (!CellBroadcastSettings.getResourcesForDefaultSubId(getApplicationContext())
                .getBoolean(R.bool.mute_by_physical_button)) {
            final View decorView = win.getDecorView();
            decorView.setSystemUiVisibility(View.SYSTEM_UI_FLAG_HIDE_NAVIGATION);
        }

        // Initialize the view.
        LayoutInflater inflater = LayoutInflater.from(this);
        setContentView(inflater.inflate(R.layout.cell_broadcast_alert, null));

        findViewById(R.id.dismissButton).setOnClickListener(v -> dismiss());

        // Get message list from saved Bundle or from Intent.
        if (savedInstanceState != null) {
            Log.d(TAG, "onCreate getting message list from saved instance state");
            mMessageList = savedInstanceState.getParcelableArrayList(
                    CellBroadcastAlertService.SMS_CB_MESSAGE_EXTRA);
        } else {
            Log.d(TAG, "onCreate getting message list from intent");
            Intent intent = getIntent();
            mMessageList = intent.getParcelableArrayListExtra(
                    CellBroadcastAlertService.SMS_CB_MESSAGE_EXTRA);

            // If we were started from a notification, dismiss it.
            clearNotification(intent);
        }

        registerReceiver(mScreenOffReceiver, new IntentFilter(Intent.ACTION_SCREEN_OFF));

        if (mMessageList == null || mMessageList.size() == 0) {
            Log.e(TAG, "onCreate failed as message list is null or empty");
            finish();
        } else {
            Log.d(TAG, "onCreate loaded message list of size " + mMessageList.size());

            // For emergency alerts, keep screen on so the user can read it
            SmsCbMessage message = getLatestMessage();

            if (message == null) {
                Log.e(TAG, "message is null");
                finish();
                return;
            }

            CellBroadcastChannelManager channelManager = new CellBroadcastChannelManager(
                    this, message.getSubscriptionId());
            if (channelManager.isEmergencyMessage(message)) {
                Log.d(TAG, "onCreate setting screen on timer for emergency alert for sub "
                        + message.getSubscriptionId());
                mScreenOffHandler.startScreenOnTimer(message);
            }

            setFinishAlertOnTouchOutside();

            updateAlertText(message);

            Resources res = CellBroadcastSettings.getResources(getApplicationContext(),
                    message.getSubscriptionId());
            if (res.getBoolean(R.bool.enable_text_copy)) {
                TextView textView = findViewById(R.id.message);
                if (textView != null) {
                    textView.setOnLongClickListener(v -> copyMessageToClipboard(message,
                            getApplicationContext()));
                }
            }
        }
    }

    @Override
    public void onStart() {
        super.onStart();
        getWindow().addSystemFlags(
                android.view.WindowManager.LayoutParams
                        .SYSTEM_FLAG_HIDE_NON_SYSTEM_OVERLAY_WINDOWS);
    }

    /**
     * Start animating warning icon.
     */
    @Override
    @VisibleForTesting
    public void onResume() {
        super.onResume();
        setWindowBottom();
        setMaxHeightScrollView();
        SmsCbMessage message = getLatestMessage();
        if (message != null) {
            int subId = message.getSubscriptionId();
            CellBroadcastChannelManager channelManager = new CellBroadcastChannelManager(this,
                    subId);
            CellBroadcastChannelRange range = channelManager
                    .getCellBroadcastChannelRangeFromMessage(message);
            if (channelManager.isEmergencyMessage(message)
                    && (range!= null && range.mDisplayIcon)) {
                mAnimationHandler.startIconAnimation(subId);
            }
        }
    }

    /**
     * Stop animating warning icon.
     */
    @Override
    @VisibleForTesting
    public void onPause() {
        Log.d(TAG, "onPause called");
        mAnimationHandler.stopIconAnimation();
        super.onPause();
    }

    @Override
    protected void onStop() {
        Log.d(TAG, "onStop called");
        // When the activity goes in background (eg. clicking Home button, dismissed by outside
        // touch if enabled), send notification.
        // Avoid doing this when activity will be recreated because of orientation change or if
        // screen goes off
        PowerManager pm = (PowerManager) getSystemService(Context.POWER_SERVICE);
        if (!(isChangingConfigurations() || getLatestMessage() == null) && pm.isScreenOn()) {
            CellBroadcastAlertService.addToNotificationBar(getLatestMessage(), mMessageList,
                    getApplicationContext(), true, true, false);
        }
        // Do not stop the audio here. Pressing power button should turn off screen but should not
        // interrupt the audio/vibration
        super.onStop();
    }

    @Override
    public void onWindowFocusChanged(boolean hasFocus) {
        super.onWindowFocusChanged(hasFocus);

        if (hasFocus) {
            Configuration config = getResources().getConfiguration();
            setPictogramAreaLayout(config.orientation);
        }
    }

    @Override
    public void onConfigurationChanged(Configuration newConfig) {
        super.onConfigurationChanged(newConfig);
        setPictogramAreaLayout(newConfig.orientation);
    }

    private void setWindowBottom() {
        // some OEMs require that the alert window is moved to the bottom of the screen to avoid
        // blocking other screen content
        if (getResources().getBoolean(R.bool.alert_dialog_bottom)) {
            Window window = getWindow();
            WindowManager.LayoutParams params = window.getAttributes();
            params.height = WindowManager.LayoutParams.WRAP_CONTENT;
            params.gravity = params.gravity | Gravity.BOTTOM | Gravity.CENTER_HORIZONTAL;
            params.verticalMargin = 0;
            window.setAttributes(params);
        }
    }

    /** Returns the currently displayed message. */
    SmsCbMessage getLatestMessage() {
        int index = mMessageList.size() - 1;
        if (index >= 0) {
            return mMessageList.get(index);
        } else {
            Log.d(TAG, "getLatestMessage returns null");
            return null;
        }
    }

    /** Removes and returns the currently displayed message. */
    private SmsCbMessage removeLatestMessage() {
        int index = mMessageList.size() - 1;
        if (index >= 0) {
            return mMessageList.remove(index);
        } else {
            return null;
        }
    }

    /**
     * Save the list of messages so the state can be restored later.
     * @param outState Bundle in which to place the saved state.
     */
    @Override
    protected void onSaveInstanceState(Bundle outState) {
        super.onSaveInstanceState(outState);
        outState.putParcelableArrayList(
                CellBroadcastAlertService.SMS_CB_MESSAGE_EXTRA, mMessageList);
    }

    /**
     * Get link method
     *
     * @param subId Subscription index
     * @return The link method
     */
    private @LinkMethod int getLinkMethod(int subId) {
        Resources res = CellBroadcastSettings.getResources(getApplicationContext(), subId);
        switch (res.getString(R.string.link_method)) {
            case LINK_METHOD_NONE_STRING: return LINK_METHOD_NONE;
            case LINK_METHOD_LEGACY_LINKIFY_STRING: return LINK_METHOD_LEGACY_LINKIFY;
            case LINK_METHOD_SMART_LINKIFY_STRING: return LINK_METHOD_SMART_LINKIFY;
            case LINK_METHOD_SMART_LINKIFY_NO_COPY_STRING: return LINK_METHOD_SMART_LINKIFY_NO_COPY;
        }
        return LINK_METHOD_NONE;
    }

    /**
     * Add URL links to the applicable texts.
     *
     * @param textView Text view
     * @param messageText The text string of the message
     * @param linkMethod Link method
     */
    private void addLinks(@NonNull TextView textView, @NonNull String messageText,
            @LinkMethod int linkMethod) {
        if (linkMethod == LINK_METHOD_LEGACY_LINKIFY) {
            Spannable text = new SpannableString(messageText);
            Linkify.addLinks(text, Linkify.ALL);
            textView.setMovementMethod(LinkMovementMethod.getInstance());
            textView.setText(text);
        } else if (linkMethod == LINK_METHOD_SMART_LINKIFY
                || linkMethod == LINK_METHOD_SMART_LINKIFY_NO_COPY) {
            // Text classification cannot be run in the main thread.
            new Thread(() -> {
                final TextClassifier classifier = textView.getTextClassifier();

                TextClassifier.EntityConfig entityConfig =
                        new TextClassifier.EntityConfig.Builder()
                                .setIncludedTypes(Arrays.asList(
                                        TextClassifier.TYPE_URL,
                                        TextClassifier.TYPE_EMAIL,
                                        TextClassifier.TYPE_PHONE,
                                        TextClassifier.TYPE_ADDRESS,
                                        TextClassifier.TYPE_FLIGHT_NUMBER))
                                .setExcludedTypes(Arrays.asList(
                                        TextClassifier.TYPE_DATE,
                                        TextClassifier.TYPE_DATE_TIME))
                                .build();

                TextLinks.Request request = new TextLinks.Request.Builder(messageText)
                        .setEntityConfig(entityConfig)
                        .build();
                Spannable text;
                if (linkMethod == LINK_METHOD_SMART_LINKIFY) {
                    text = new SpannableString(messageText);
                    // Add links to the spannable text.
                    classifier.generateLinks(request).apply(
                            text, TextLinks.APPLY_STRATEGY_REPLACE, null);
                } else {
                    TextLinks textLinks = classifier.generateLinks(request);
                    // Add links to the spannable text.
                    text = applyTextLinksToSpannable(messageText, textLinks, classifier);
                }
                // UI can be only updated in the main thread.
                runOnUiThread(() -> {
                    textView.setMovementMethod(LinkMovementMethod.getInstance());
                    textView.setText(text);
                });
            }).start();
        }
    }

    private Spannable applyTextLinksToSpannable(String text, TextLinks textLinks,
            TextClassifier textClassifier) {
        Spannable result = new SpannableString(text);
        for (TextLink link : textLinks.getLinks()) {
            TextClassification textClassification = textClassifier.classifyText(
                    new Request.Builder(
                            text,
                            link.getStart(),
                            link.getEnd())
                            .build());
            if (textClassification.getActions().isEmpty()) {
                continue;
            }
            RemoteAction remoteAction = textClassification.getActions().get(0);
            result.setSpan(new RemoteActionSpan(remoteAction), link.getStart(), link.getEnd(),
                    Spannable.SPAN_EXCLUSIVE_EXCLUSIVE);
        }
        return result;
    }

    private static class RemoteActionSpan extends ClickableSpan {
        private final RemoteAction mRemoteAction;
        private RemoteActionSpan(RemoteAction remoteAction) {
            mRemoteAction = remoteAction;
        }
        @Override
        public void onClick(@NonNull View view) {
            try {
                mRemoteAction.getActionIntent().send();
            } catch (PendingIntent.CanceledException e) {
                Log.e(TAG, "Failed to start the pendingintent.");
            }
        }
    }

    /**
     * If the carrier or country is configured to show the alert dialog title text in the
     * language matching the message, this method returns the string in that language. Otherwise
     * this method returns the string in the device's current language
     *
     * @param resId resource Id
     * @param res Resources for the subId
     * @param languageCode the ISO-639-1 language code for this message, or null if unspecified
     */
    private String overrideTranslation(int resId, Resources res, String languageCode,
            boolean forceOverride) {
        if (!TextUtils.isEmpty(languageCode)
                && (res.getBoolean(R.bool.override_alert_title_language_to_match_message_locale)
                || forceOverride)) {
            Configuration conf = res.getConfiguration();
            conf = new Configuration(conf);
            conf.setLocale(new Locale(languageCode));
            Context localizedContext = getApplicationContext().createConfigurationContext(conf);
            return localizedContext.getResources().getText(resId).toString();
        } else {
            return res.getText(resId).toString();
        }
    }

    /**
     * Update alert text when a new emergency alert arrives.
     * @param message CB message which is used to update alert text.
     */
    private void updateAlertText(@NonNull SmsCbMessage message) {
        Context context = getApplicationContext();
        int titleId = CellBroadcastResources.getDialogTitleResource(context, message);

        Resources res = CellBroadcastSettings.getResources(context, message.getSubscriptionId());

        CellBroadcastChannelManager channelManager = new CellBroadcastChannelManager(
                this, message.getSubscriptionId());
        CellBroadcastChannelRange range = channelManager
                .getCellBroadcastChannelRangeFromMessage(message);
        String languageCode;
        boolean forceOverride = false;
        if (range != null && !TextUtils.isEmpty(range.mLanguageCode)) {
            languageCode = range.mLanguageCode;
            forceOverride = true;
        } else {
            languageCode = message.getLanguageCode();
        }
        String title = overrideTranslation(titleId, res, languageCode, forceOverride);
        TextView titleTextView = findViewById(R.id.alertTitle);

        if (titleTextView != null) {
            String timeFormat = res.getString(R.string.date_time_format);
            if (!TextUtils.isEmpty(timeFormat)) {
                titleTextView.setSingleLine(false);
                title += "\n" + new SimpleDateFormat(timeFormat).format(message.getReceivedTime());
            }
            setTitle(title);
            titleTextView.setText(title);
        }

        String messageText = message.getMessageBody();
        TextView textView = findViewById(R.id.message);
        String messageBodyOverride = getMessageBodyOverride(context, message);
        if (!TextUtils.isEmpty(messageBodyOverride)) {
            messageText = messageBodyOverride;
        }
        if (textView != null && messageText != null) {
            int linkMethod = getLinkMethod(message.getSubscriptionId());
            if (linkMethod != LINK_METHOD_NONE) {
                addLinks(textView, messageText, linkMethod);
            } else {
                // Do not add any link to the message text.
                textView.setText(messageText);
            }
        }

        String dismissButtonText = getString(R.string.button_dismiss);

        if (mMessageList.size() > 1) {
            dismissButtonText += "  (1/" + mMessageList.size() + ")";
        }

        ((TextView) findViewById(R.id.dismissButton)).setText(dismissButtonText);


        setPictogram(context, message);
    }

    /**
     * @param message
     * @return the required message override for the service category for the carrier, or null if
     * it is not set
     */
    private String getMessageBodyOverride(Context context, SmsCbMessage message) {
        // return true if the carrier has configured this service category to have a fixed message
        Resources res = CellBroadcastSettings.getResources(context, message.getSubscriptionId());
        String[] overrides = res.getStringArray(R.array.message_body_override);
        if (overrides != null && overrides.length > 0) {
            for (String entry : overrides) {
                String[] serviceCategoryAndMessage = entry.split(":");
                if (message.getServiceCategory() == Integer.parseInt(
                        serviceCategoryAndMessage[0])) {
                    return insertCarrierNameIfNeeded(context, message.getSubscriptionId(),
                            serviceCategoryAndMessage[1]);
                }
            }
        }
        return null;
    }

    /**
     * If an override message must have the carrier name (represented with a '>' character), return
     * the message with the carrier name inserted. Otherwise just return the message.
     */
    private String insertCarrierNameIfNeeded(Context context, int subId, String message) {
        TelephonyManager tm = context.getSystemService(TelephonyManager.class)
                .createForSubscriptionId(subId);
        String carrierName = (String) tm.getSimSpecificCarrierIdName();
        if (TextUtils.isEmpty(carrierName)) {
            return message;
        }
        return message.replace(">", carrierName);
    }

    /**
     * Set pictogram image
     * @param context
     * @param message
     */
    private void setPictogram(Context context, SmsCbMessage message) {
        int resId = CellBroadcastResources.getDialogPictogramResource(context, message);
        ImageView image = findViewById(R.id.pictogramImage);
        if (resId != -1) {
            image.setImageResource(resId);
            image.setVisibility(View.VISIBLE);
        } else {
            image.setVisibility(View.GONE);
        }
    }

    /**
     * Set pictogram to match orientation
     *
     * @param orientation The orientation of the pictogram.
     */
    private void setPictogramAreaLayout(int orientation) {
        ImageView image = findViewById(R.id.pictogramImage);
        if (image.getVisibility() == View.VISIBLE) {
            ViewGroup.LayoutParams params = image.getLayoutParams();

            if (orientation == Configuration.ORIENTATION_LANDSCAPE) {
                Display display = getWindowManager().getDefaultDisplay();
                Point point = new Point();
                display.getSize(point);
                params.width = (int) (point.x * 0.3);
                params.height = (int) (point.y * 0.3);
            } else {
                params.width = ViewGroup.LayoutParams.WRAP_CONTENT;
                params.height = ViewGroup.LayoutParams.WRAP_CONTENT;
            }

            image.setLayoutParams(params);
        }
    }

    private void setMaxHeightScrollView() {
        int contentPanelMaxHeight = getResources().getDimensionPixelSize(
                R.dimen.alert_dialog_maxheight_content_panel);
        if (contentPanelMaxHeight > 0) {
            CustomHeightScrollView scrollView = (CustomHeightScrollView) findViewById(
                    R.id.scrollView);
            if (scrollView != null) {
                scrollView.setMaximumHeight(contentPanelMaxHeight);
            }
        }
    }

    /**
     * Called by {@link CellBroadcastAlertService} to add a new alert to the stack.
     * @param intent The new intent containing one or more {@link SmsCbMessage}.
     */
    @Override
    @VisibleForTesting
    public void onNewIntent(Intent intent) {
        if (intent.getBooleanExtra(CellBroadcastAlertService.DISMISS_DIALOG, false)) {
            dismissAllFromNotification(intent);
            return;
        }
        ArrayList<SmsCbMessage> newMessageList = intent.getParcelableArrayListExtra(
                CellBroadcastAlertService.SMS_CB_MESSAGE_EXTRA);
        if (newMessageList != null) {
            if (intent.getBooleanExtra(FROM_SAVE_STATE_NOTIFICATION_EXTRA, false)) {
                mMessageList = newMessageList;
            } else {
                // remove the duplicate messages
                for (SmsCbMessage message : newMessageList) {
                    mMessageList.removeIf(
                            msg -> msg.getReceivedTime() == message.getReceivedTime());
                }
                mMessageList.addAll(newMessageList);
                if (CellBroadcastSettings.getResourcesForDefaultSubId(getApplicationContext())
                        .getBoolean(R.bool.show_cmas_messages_in_priority_order)) {
                    // Sort message list to show messages in a different order than received by
                    // prioritizing them. Presidential Alert only has top priority.
                    Collections.sort(
                            mMessageList,
                            (Comparator) (o1, o2) -> {
                                boolean isPresidentialAlert1 =
                                        ((SmsCbMessage) o1).isCmasMessage()
                                                && ((SmsCbMessage) o1).getCmasWarningInfo()
                                                .getMessageClass() == SmsCbCmasInfo
                                                .CMAS_CLASS_PRESIDENTIAL_LEVEL_ALERT;
                                boolean isPresidentialAlert2 =
                                        ((SmsCbMessage) o2).isCmasMessage()
                                                && ((SmsCbMessage) o2).getCmasWarningInfo()
                                                .getMessageClass() == SmsCbCmasInfo
                                                .CMAS_CLASS_PRESIDENTIAL_LEVEL_ALERT;
                                if (isPresidentialAlert1 ^ isPresidentialAlert2) {
                                    return isPresidentialAlert1 ? 1 : -1;
                                }
                                Long time1 =
                                        new Long(((SmsCbMessage) o1).getReceivedTime());
                                Long time2 =
                                        new Long(((SmsCbMessage) o2).getReceivedTime());
                                return time2.compareTo(time1);
                            });
                }
            }
            Log.d(TAG, "onNewIntent called with message list of size " + newMessageList.size());

            // For emergency alerts, keep screen on so the user can read it
            SmsCbMessage message = getLatestMessage();
            if (message != null) {
                CellBroadcastChannelManager channelManager = new CellBroadcastChannelManager(
                        this, message.getSubscriptionId());
                if (channelManager.isEmergencyMessage(message)) {
                    Log.d(TAG, "onCreate setting screen on timer for emergency alert for sub "
                            + message.getSubscriptionId());
                    mScreenOffHandler.startScreenOnTimer(message);
                }
            }

            hideOptOutDialog(); // Hide opt-out dialog when new alert coming
            setFinishAlertOnTouchOutside();
            updateAlertText(getLatestMessage());
            // If the new intent was sent from a notification, dismiss it.
            clearNotification(intent);
        } else {
            Log.e(TAG, "onNewIntent called without SMS_CB_MESSAGE_EXTRA, ignoring");
        }
    }

    /**
     * Try to cancel any notification that may have started this activity.
     * @param intent Intent containing extras used to identify if notification needs to be cleared
     */
    private void clearNotification(Intent intent) {
        if (intent.getBooleanExtra(DISMISS_NOTIFICATION_EXTRA, false)) {
            NotificationManager notificationManager =
                    (NotificationManager) getSystemService(Context.NOTIFICATION_SERVICE);
            notificationManager.cancel(CellBroadcastAlertService.NOTIFICATION_ID);
            CellBroadcastReceiverApp.clearNewMessageList();
        }
    }

    /**
     * This will be called when users swipe away the notification, this will
     * 1. dismiss all foreground dialog, stop animating warning icon and stop the
     * {@link CellBroadcastAlertAudio} service.
     * 2. Does not mark message read.
     */
    public void dismissAllFromNotification(Intent intent) {
        Log.d(TAG, "dismissAllFromNotification");
        // Stop playing alert sound/vibration/speech (if started)
        stopService(new Intent(this, CellBroadcastAlertAudio.class));
        // Cancel any pending alert reminder
        CellBroadcastAlertReminder.cancelAlertReminder();
        // Remove the all current showing alert message from the list.
        if (mMessageList != null) {
            mMessageList.clear();
        }
        // clear notifications.
        clearNotification(intent);
        // Remove pending screen-off messages (animation messages are removed in onPause()).
        mScreenOffHandler.stopScreenOnTimer();
        finish();
    }

    /**
     * Stop animating warning icon and stop the {@link CellBroadcastAlertAudio}
     * service if necessary.
     */
    @VisibleForTesting
    public void dismiss() {
        Log.d(TAG, "dismiss");
        // Stop playing alert sound/vibration/speech (if started)
        stopService(new Intent(this, CellBroadcastAlertAudio.class));

        // Cancel any pending alert reminder
        CellBroadcastAlertReminder.cancelAlertReminder();

        // Remove the current alert message from the list.
        SmsCbMessage lastMessage = removeLatestMessage();
        if (lastMessage == null) {
            Log.e(TAG, "dismiss() called with empty message list!");
            finish();
            return;
        }

        // Remove the read message from the notification bar.
        // e.g, read the message from emergency alert history, need to update the notification bar.
        removeReadMessageFromNotificationBar(lastMessage, getApplicationContext());

        // Mark the alert as read.
        final long deliveryTime = lastMessage.getReceivedTime();

        // Mark broadcast as read on a background thread.
        new CellBroadcastContentProvider.AsyncCellBroadcastTask(getContentResolver())
                .execute((CellBroadcastContentProvider.CellBroadcastOperation) provider
                        -> provider.markBroadcastRead(Telephony.CellBroadcasts.DELIVERY_TIME,
                        deliveryTime));

        // Set the opt-out dialog flag if this is a CMAS alert (other than Always-on alert e.g,
        // Presidential alert).
        CellBroadcastChannelManager channelManager = new CellBroadcastChannelManager(
                getApplicationContext(),
                lastMessage.getSubscriptionId());
        CellBroadcastChannelRange range = channelManager
                .getCellBroadcastChannelRangeFromMessage(lastMessage);

        if (!neverShowOptOutDialog(lastMessage.getSubscriptionId()) && range != null
                && !range.mAlwaysOn) {
            mShowOptOutDialog = true;
        }

        // If there are older emergency alerts to display, update the alert text and return.
        SmsCbMessage nextMessage = getLatestMessage();
        if (nextMessage != null) {
            setFinishAlertOnTouchOutside();
            updateAlertText(nextMessage);
            int subId = nextMessage.getSubscriptionId();
            if (channelManager.isEmergencyMessage(nextMessage)
                    && (range!= null && range.mDisplayIcon)) {
                mAnimationHandler.startIconAnimation(subId);
            } else {
                mAnimationHandler.stopIconAnimation();
            }
            return;
        }

        // Remove pending screen-off messages (animation messages are removed in onPause()).
        mScreenOffHandler.stopScreenOnTimer();

        // Show opt-in/opt-out dialog when the first CMAS alert is received.
        if (mShowOptOutDialog) {
            SharedPreferences prefs = PreferenceManager.getDefaultSharedPreferences(this);
            if (prefs.getBoolean(CellBroadcastSettings.KEY_SHOW_CMAS_OPT_OUT_DIALOG, true)) {
                // Clear the flag so the user will only see the opt-out dialog once.
                prefs.edit().putBoolean(CellBroadcastSettings.KEY_SHOW_CMAS_OPT_OUT_DIALOG, false)
                        .apply();

                KeyguardManager km = (KeyguardManager) getSystemService(Context.KEYGUARD_SERVICE);
                if (km.inKeyguardRestrictedInputMode()) {
                    Log.d(TAG, "Showing opt-out dialog in new activity (secure keyguard)");
                    Intent intent = new Intent(this, CellBroadcastOptOutActivity.class);
                    startActivity(intent);
                } else {
                    Log.d(TAG, "Showing opt-out dialog in current activity");
                    mOptOutDialog = CellBroadcastOptOutActivity.showOptOutDialog(this);
                    return; // don't call finish() until user dismisses the dialog
                }
            }
        }
        finish();
    }

    @Override
    public void onDestroy() {
        try {
            unregisterReceiver(mScreenOffReceiver);
        } catch (IllegalArgumentException e) {
            Log.e(TAG, "Unregister Receiver fail", e);
        }
        super.onDestroy();
    }

    @Override
    public boolean onKeyDown(int keyCode, KeyEvent event) {
        Log.d(TAG, "onKeyDown: " + event);
        SmsCbMessage message = getLatestMessage();
        if (message != null && CellBroadcastSettings.getResources(getApplicationContext(),
                message.getSubscriptionId()).getBoolean(R.bool.mute_by_physical_button)) {
            switch (event.getKeyCode()) {
                // Volume keys and camera keys mute the alert sound/vibration (except ETWS).
                case KeyEvent.KEYCODE_VOLUME_UP:
                case KeyEvent.KEYCODE_VOLUME_DOWN:
                case KeyEvent.KEYCODE_VOLUME_MUTE:
                case KeyEvent.KEYCODE_CAMERA:
                case KeyEvent.KEYCODE_FOCUS:
                    // Stop playing alert sound/vibration/speech (if started)
                    stopService(new Intent(this, CellBroadcastAlertAudio.class));
                    return true;

                default:
                    break;
            }
            return super.onKeyDown(keyCode, event);
        } else {
            if (event.getKeyCode() == KeyEvent.KEYCODE_POWER) {
                // TODO: do something to prevent screen off
            }
            // Disable all physical keys if mute_by_physical_button is false
            return true;
        }
    }

    @Override
    public void onBackPressed() {
        // Disable back key
    }

    /**
     * Hide opt-out dialog.
     * In case of any emergency alert invisible, need to hide the opt-out dialog when
     * new alert coming.
     */
    private void hideOptOutDialog() {
        if (mOptOutDialog != null && mOptOutDialog.isShowing()) {
            SharedPreferences prefs = PreferenceManager.getDefaultSharedPreferences(this);
            prefs.edit().putBoolean(CellBroadcastSettings.KEY_SHOW_CMAS_OPT_OUT_DIALOG, true)
                    .apply();
            mOptOutDialog.dismiss();
        }
    }

    /**
     * @return true if the device is configured to never show the opt out dialog for the mcc/mnc
     */
    private boolean neverShowOptOutDialog(int subId) {
        return CellBroadcastSettings.getResources(getApplicationContext(), subId)
                .getBoolean(R.bool.disable_opt_out_dialog);
    }

    /**
     * Copy the message to clipboard.
     *
     * @param message Cell broadcast message.
     *
     * @return {@code true} if success, otherwise {@code false};
     */
    @VisibleForTesting
    public static boolean copyMessageToClipboard(SmsCbMessage message, Context context) {
        ClipboardManager cm = (ClipboardManager) context.getSystemService(CLIPBOARD_SERVICE);
        if (cm == null) return false;

        cm.setPrimaryClip(ClipData.newPlainText("Alert Message", message.getMessageBody()));

        String msg = CellBroadcastSettings.getResources(context,
                message.getSubscriptionId()).getString(R.string.message_copied);
        Toast.makeText(context, msg, Toast.LENGTH_SHORT).show();
        return true;
    }

    /**
     * Remove read message from the notification bar, update the notification text, count or cancel
     * the notification if there is no un-read messages.
     * @param message The dismissed/read message to be removed from the notification bar
     * @param context
     */
    private void removeReadMessageFromNotificationBar(SmsCbMessage message, Context context) {
        Log.d(TAG, "removeReadMessageFromNotificationBar, msg: " + message.toString());
        ArrayList<SmsCbMessage> unreadMessageList = CellBroadcastReceiverApp
                .removeReadMessage(message);
        if (unreadMessageList.isEmpty()) {
            Log.d(TAG, "removeReadMessageFromNotificationBar, cancel notification");
            NotificationManager notificationManager = getSystemService(NotificationManager.class);
            notificationManager.cancel(CellBroadcastAlertService.NOTIFICATION_ID);
        } else {
            Log.d(TAG, "removeReadMessageFromNotificationBar, update count to "
                    + unreadMessageList.size() );
            // do not alert if remove unread messages from the notification bar.
           CellBroadcastAlertService.addToNotificationBar(
                   CellBroadcastReceiverApp.getLatestMessage(),
                   unreadMessageList, context,false, false, false);
        }
    }

    /**
     * Finish alert dialog only if all messages are configured with DismissOnOutsideTouch.
     * When multiple messages are displayed, the message with dismissOnOutsideTouch(normally low
     * priority message) is displayed on top of other unread alerts without dismissOnOutsideTouch,
     * users can easily dismiss all messages by touching the screen. better way is to dismiss the
     * alert if and only if all messages with dismiss_on_outside_touch set true.
     */
    private void setFinishAlertOnTouchOutside() {
        if (mMessageList != null) {
            int dismissCount = 0;
            for (SmsCbMessage message : mMessageList) {
                CellBroadcastChannelManager channelManager = new CellBroadcastChannelManager(
                        this, message.getSubscriptionId());
                CellBroadcastChannelManager.CellBroadcastChannelRange range =
                        channelManager.getCellBroadcastChannelRangeFromMessage(message);
                if (range != null && range.mDismissOnOutsideTouch) {
                    dismissCount++;
                }
            }
            setFinishOnTouchOutside(mMessageList.size() > 0 && mMessageList.size() == dismissCount);
        }
    }
}
