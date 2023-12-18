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
package com.android.car.assist;

import android.content.Context;
import android.os.Bundle;
import android.os.Handler;
import android.service.notification.StatusBarNotification;
import android.service.voice.VoiceInteractionService;
import android.service.voice.VoiceInteractionSession;

import androidx.annotation.StringDef;
import androidx.core.app.NotificationManagerCompat;

import com.android.car.assist.payloadhandlers.NotificationPayloadHandler;
import com.android.car.messenger.common.Conversation;

/**
 * An active voice interaction session on the car, providing additional actions which assistant
 * should act on. Override the {@link #onShow(String, Bundle, int)} to received the action specified
 * by the voice session initiator.
 */
public abstract class CarVoiceInteractionSession extends VoiceInteractionSession {
    /** The key used for the action {@link String} in the payload {@link Bundle}. */
    public static final String KEY_ACTION = "KEY_ACTION";

    /**
     * The key used for the {@link CarVoiceInteractionSession#VOICE_ACTION_HANDLE_EXCEPTION} payload
     * {@link Bundle}. Must map to a {@link ExceptionValue}.
     */
    public static final String KEY_EXCEPTION = "KEY_EXCEPTION";

    /**
     * The key used for the {@link CarVoiceInteractionSession#VOICE_ACTION_HANDLE_EXCEPTION} payload
     * {@link Bundle}. Must map to a boolean. If value is true, the Fallback Assistant that can
     * handle the user's request has been disabled.
     */
    public static final String KEY_FALLBACK_ASSISTANT_ENABLED = "KEY_FALLBACK_ASSISTANT_ENABLED";

    /**
     * The key used for a substitute package name the Digital Assistant should read out in lieu of
     * package name associated with the {@link StatusBarNotification}.
     *
     * <p>Only system packages which lump together a bunch of unrelated stuff may substitute a
     * different name to make the purpose of the notification more clear. The correct package label
     * should always be accessible via SystemUI.
     */
    public static final String EXTRA_SUBSTITUTE_APP_NAME = "android.substName";

    /**
     * The key used for the payload {@link Bundle}, if a {@link StatusBarNotification} is used as
     * the payload.
     */
    public static final String KEY_NOTIFICATION = "KEY_NOTIFICATION";

    /**
     * The key used for the payload {@link Bundle}, if a {@link Conversation} is used as the
     * payload.
     */
    public static final String KEY_CONVERSATION = "KEY_CONVERSATION";

    /** Indicates to assistant that no action was specified. */
    public static final String VOICE_ACTION_NO_ACTION = "VOICE_ACTION_NO_ACTION";

    /**
     * Indicates to assistant that a read action is being requested for a given payload. A {@link
     * StatusBarNotification} object will be provided in the payload
     */
    public static final String VOICE_ACTION_READ_NOTIFICATION = "VOICE_ACTION_READ_NOTIFICATION";

    /**
     * Indicates to assistant that a reply action is being requested for a given payload. A {@link
     * StatusBarNotification} object will be provided in the payload
     */
    public static final String VOICE_ACTION_REPLY_NOTIFICATION = "VOICE_ACTION_REPLY_NOTIFICATION";

    /**
     * Indicates to assistant that a read conversation action is being requested. A {@link
     * Conversation} object will be provided in the payload.
     */
    public static final String VOICE_ACTION_READ_CONVERSATION = "VOICE_ACTION_READ_CONVERSATION";

    /**
     * Indicates to assistant that a reply conversation action is being requested. A {@link
     * Conversation} object will be provided in the payload.
     */
    public static final String VOICE_ACTION_REPLY_CONVERSATION = "VOICE_ACTION_REPLY_CONVERSATION";

    /**
     * Indicates to digital assistant that it should capture a SMS message from the user,
     * potentially finding which contact to send the message to and which device to send the message
     * from (only if the application does not send the digital assistant this information in the
     * bundle). Once the digital assistant has gathered the information from the user, it should
     * send back the PendingIntent (provided in the bundle) with the information so the application
     * can actually send the SMS.
     */
    public static final String VOICE_ACTION_SEND_SMS = "VOICE_ACTION_SEND_SMS";

    /* Recipient's phone number. If this and the recipient name are not provided,
     * by the application, digital assistant must do contact disambiguation
     * and add the phone number to the pending intent
     */
    public static final String KEY_PHONE_NUMBER = "KEY_PHONE_NUMBER";

    /**
     * Recipient's name. If this and the recipient phone number are not provided by the application,
     * digital assistant must do contact disambiguation but is not required to add the name to the
     * PendingIntent.
     */
    public static final String KEY_RECIPIENT_NAME = "KEY RECIPIENT NAME";

    /* Recipient's UID in the Contact Provider database. Optionally provided by the application.
     * Not required to be sent back by the digital assistant.
     */
    public static final String KEY_RECIPIENT_UID = "KEY_RECIPIENT_UID";

    /* Friendly name of the device in which to send the message from. If not
     * provided by the application, digital assistant must do device disambiguation
     * but is not required to add it to the Pending Intent.
     */
    public static final String KEY_DEVICE_NAME = "KEY DEVICE_NAME";

    /* Bluetooth device address of the device of which to send the message from.
     * If not provided by the application, assistant must do device disambiguation
     * and add this to the Pendingintent.
     */
    public static final String KEY_DEVICE_ADDRESS = "KEY_DEVICE_ADDRESS";

    /* KEY_SEND_PENDING_INTENT is not null and always be provided by the application.
     * The application must preload the pending intent with any KEYs, it provides the assistant
     * that is also needed to send the message.
     * (i.e. if the application passes in the KEY_PHONE_NUMBER in the Bundle,
     * the assistant can assume the application has already put this in the
     * PendingIntent and may not re-add it to the PendingIntent).
     */
    public static final String KEY_SEND_PENDING_INTENT = "KEY_SEND_PENDING INTENT";

    /**
     * Indicates to assistant that it should resolve the exception in the given payload (found in
     * {@link CarVoiceInteractionSession#KEY_EXCEPTION}'s value).
     */
    public static final String VOICE_ACTION_HANDLE_EXCEPTION = "VOICE_ACTION_HANDLE_EXCEPTION";

    /** The list of exceptions the active voice service must handle. */
    @StringDef({EXCEPTION_NOTIFICATION_LISTENER_PERMISSIONS_MISSING})
    public @interface ExceptionValue {}

    /**
     * Indicates to assistant that it is missing the Notification Listener permission, and should
     * request this permission from the user.
     */
    public static final String EXCEPTION_NOTIFICATION_LISTENER_PERMISSIONS_MISSING =
            "EXCEPTION_NOTIFICATION_LISTENER_PERMISSIONS_MISSING";

    private final NotificationPayloadHandler mNotificationPayloadHandler;

    public CarVoiceInteractionSession(Context context) {
        super(context);
        mNotificationPayloadHandler = new NotificationPayloadHandler(getContext());
    }

    public CarVoiceInteractionSession(Context context, Handler handler) {
        super(context, handler);
        mNotificationPayloadHandler = new NotificationPayloadHandler(getContext());
    }

    /**
     * Returns the notification payload handler, which can be used to handle actions related to
     * notification payloads.
     */
    public NotificationPayloadHandler getNotificationPayloadHandler() {
        return mNotificationPayloadHandler;
    }

    @Override
    public final void onShow(Bundle args, int showFlags) {
        super.onShow(args, showFlags);
        addNotificationAccessExceptionIfNeeded(args);
        if (args != null) {
            String action = getRequestedVoiceAction(args);
            if (!VOICE_ACTION_NO_ACTION.equals(action)) {
                onShow(action, args, showFlags);
                return;
            }
        }
        onShow(VOICE_ACTION_NO_ACTION, args, showFlags);
    }

    /**
     * Called when the session UI is going to be shown. This is called after {@link
     * #onCreateContentView} (if the session's content UI needed to be created) and immediately
     * prior to the window being shown. This may be called while the window is already shown, if a
     * show request has come in while it is shown, to allow you to update the UI to match the new
     * show arguments.
     *
     * @param action The action that is being requested for this session (e.g. {@link
     *     CarVoiceInteractionSession#VOICE_ACTION_READ_NOTIFICATION}, {@link
     *     CarVoiceInteractionSession#VOICE_ACTION_REPLY_NOTIFICATION}).
     * @param args The arguments that were supplied to {@link VoiceInteractionService#showSession
     *     VoiceInteractionService.showSession}.
     * @param flags The show flags originally provided to {@link VoiceInteractionService#showSession
     *     VoiceInteractionService.showSession}.
     */
    protected abstract void onShow(String action, Bundle args, int flags);

    /**
     * Transforms bundle to {@link KEY_EXCEPTION} with
     * value {@link EXCEPTION_NOTIFICATION_LISTENER_PERMISSIONS_MISSING} if
     * Notification Listener permissions are missing.
     */
    private void addNotificationAccessExceptionIfNeeded(Bundle args) {
      if (needsNotificationAccess(args)) {
              args.putString(KEY_ACTION, VOICE_ACTION_HANDLE_EXCEPTION);
              args.putString(KEY_EXCEPTION, EXCEPTION_NOTIFICATION_LISTENER_PERMISSIONS_MISSING);
              args.putBoolean(KEY_FALLBACK_ASSISTANT_ENABLED, false);
      }
    }

    private boolean needsNotificationAccess(Bundle args) {
      return isNotificationAction(args) && !hasNotificationAccess(getContext());
    }

    /**
     * Returns {@code true} if the given {@code args} is a notification action, {@code false}
     * otherwise
     */
    private static boolean isNotificationAction(Bundle args) {
        if (args == null) {
          return false;
        }
        String action = args.getString(KEY_ACTION);
        return VOICE_ACTION_REPLY_NOTIFICATION.equals(action)
            || VOICE_ACTION_READ_NOTIFICATION.equals(action);
    }

    private boolean hasNotificationAccess(Context context) {
        return NotificationManagerCompat
            .getEnabledListenerPackages(context).contains(context.getPackageName());
    }

    /**
     * Returns the action {@link String} provided in the args {@link Bundle}, or {@link
     * CarVoiceInteractionSession#VOICE_ACTION_NO_ACTION} if no such string was provided.
     */
    protected static String getRequestedVoiceAction(Bundle args) {
        return args.getString(KEY_ACTION, VOICE_ACTION_NO_ACTION);
    }
}