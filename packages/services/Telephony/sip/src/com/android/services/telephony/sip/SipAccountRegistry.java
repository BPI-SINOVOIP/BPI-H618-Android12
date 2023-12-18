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

package com.android.services.telephony.sip;

import android.app.Notification;
import android.app.NotificationChannel;
import android.app.NotificationManager;
import android.app.PendingIntent;
import android.content.Context;
import android.content.Intent;
import android.net.sip.SipManager;
import android.net.sip.SipProfile;
import android.telecom.PhoneAccount;
import android.telecom.PhoneAccountHandle;
import android.telecom.TelecomManager;
import android.util.Log;

import com.android.phone.R;

import java.io.IOException;
import java.util.List;
import java.util.Objects;
import java.util.concurrent.CopyOnWriteArrayList;
import java.util.stream.Collectors;

/**
 * Manages the {@link PhoneAccount} entries for SIP calling.
 */
public final class SipAccountRegistry {
    private final class AccountEntry {
        private final SipProfile mProfile;

        AccountEntry(SipProfile profile) {
            mProfile = profile;
        }

        SipProfile getProfile() {
            return mProfile;
        }

        /**
         * Stops the SIP service associated with the SIP profile.  The {@code SipAccountRegistry} is
         * informed when the service has been stopped via an intent which triggers
         * {@link SipAccountRegistry#removeSipProfile(String)}.
         *
         * @param sipManager The SIP manager.
         * @return {@code True} if stop was successful.
         */
        boolean stopSipService(SipManager sipManager) {
            try {
                sipManager.close(mProfile.getUriString());
                return true;
            } catch (Exception e) {
                log("stopSipService, stop failed for profile: " + mProfile.getUriString() +
                        ", exception: " + e);
            }
            return false;
        }
    }

    private static final String PREFIX = "[SipAccountRegistry] ";
    private static final boolean VERBOSE = false; /* STOP SHIP if true */
    private static final SipAccountRegistry INSTANCE = new SipAccountRegistry();
    private static final String NOTIFICATION_TAG = SipAccountRegistry.class.getSimpleName();
    private static final int SIP_ACCOUNTS_REMOVED_NOTIFICATION_ID = 1;

    private static final String CHANNEL_ID_SIP_ACCOUNTS_REMOVED = "sipAccountsRemoved";

    private final List<AccountEntry> mAccounts = new CopyOnWriteArrayList<>();

    private NotificationChannel mNotificationChannel;
    private NotificationManager mNm;

    private SipAccountRegistry() {}

    public static SipAccountRegistry getInstance() {
        return INSTANCE;
    }

    /**
     * Sets up the Account registry and performs any upgrade operations before it is used.
     */
    public void setup(Context context) {
        setupNotificationChannel(context);
        verifyAndPurgeInvalidPhoneAccounts(context);
        startSipProfilesAsync(context);
    }

    private void setupNotificationChannel(Context context) {
        mNotificationChannel = new NotificationChannel(
                CHANNEL_ID_SIP_ACCOUNTS_REMOVED,
                context.getText(R.string.notification_channel_sip_account),
                NotificationManager.IMPORTANCE_HIGH);
        mNm = context.getSystemService(NotificationManager.class);
        if (mNm != null) {
            mNm.createNotificationChannel(mNotificationChannel);
        }
    }

    /**
     * Checks the existing SIP phone {@link PhoneAccount}s registered with telecom and deletes any
     * invalid accounts.
     *
     * @param context The context.
     */
    void verifyAndPurgeInvalidPhoneAccounts(Context context) {
        TelecomManager telecomManager = context.getSystemService(TelecomManager.class);
        SipProfileDb profileDb = new SipProfileDb(context);
        List<PhoneAccountHandle> accountHandles = telecomManager.getPhoneAccountsSupportingScheme(
                PhoneAccount.SCHEME_SIP);

        for (PhoneAccountHandle accountHandle : accountHandles) {
            String profileName = SipUtil.getSipProfileNameFromPhoneAccount(accountHandle);
            SipProfile profile = profileDb.retrieveSipProfileFromName(profileName);
            if (profile == null) {
                log("verifyAndPurgeInvalidPhoneAccounts, deleting account: " + accountHandle);
                telecomManager.unregisterPhoneAccount(accountHandle);
            }
        }
    }

    /**
     * Starts the SIP service for the specified SIP profile and ensures it has a valid registered
     * {@link PhoneAccount}.
     *
     * @param context The context.
     * @param sipProfileName The name of the {@link SipProfile} to start, or {@code null} for all.
     * @param enableProfile Sip account should be enabled
     */
    void startSipService(Context context, String sipProfileName, boolean enabledProfile) {
        startSipProfilesAsync(context);
    }

    /**
     * Removes a {@link SipProfile} from the account registry.  Does not stop/close the associated
     * SIP service (this method is invoked via an intent from the SipService once a profile has
     * been stopped/closed).
     *
     * @param sipProfileName Name of the SIP profile.
     */
    public void removeSipProfile(String sipProfileName) {
        AccountEntry accountEntry = getAccountEntry(sipProfileName);

        if (accountEntry != null) {
            mAccounts.remove(accountEntry);
        }
    }

    /**
     * Stops a SIP profile and un-registers its associated {@link android.telecom.PhoneAccount}.
     * Called after a SIP profile is deleted.  The {@link AccountEntry} will be removed when the
     * service has been stopped.  The {@code SipService} fires the {@code ACTION_SIP_REMOVE_PHONE}
     * intent, which triggers {@link SipAccountRegistry#removeSipProfile(String)} to perform the
     * removal.
     *
     * @param context The context.
     * @param sipProfileName Name of the SIP profile.
     */
    void stopSipService(Context context, String sipProfileName) {
        // Stop the sip service for the profile.
        AccountEntry accountEntry = getAccountEntry(sipProfileName);
        if (accountEntry != null ) {
            SipManager sipManager = SipManager.newInstance(context);
            accountEntry.stopSipService(sipManager);
        }

        // Un-register its PhoneAccount.
        PhoneAccountHandle handle = SipUtil.createAccountHandle(context, sipProfileName);
        TelecomManager tm = context.getSystemService(TelecomManager.class);
        tm.unregisterPhoneAccount(handle);
    }

    /**
     * Performs an asynchronous call to
     * {@link SipAccountRegistry#startSipProfiles(android.content.Context, String)}, starting the
     * specified SIP profile and registering its {@link android.telecom.PhoneAccount}.
     *
     * @param context The context.
     */
    private void startSipProfilesAsync(
            final Context context) {
        if (VERBOSE) log("startSipProfiles, start auto registration");

        new Thread(new Runnable() {
            @Override
            public void run() {
                startSipProfiles(context);
            }}
        ).start();
    }

    /**
     * Loops through all SIP accounts from the SIP database, starts each service and registers
     * each with the telecom framework. If a specific sipProfileName is specified, this will only
     * register the associated SIP account.
     *
     * @param context The context.
     */
    private void startSipProfiles(Context context) {
        SipProfileDb profileDb = new SipProfileDb(context);
        List<SipProfile> sipProfileList = profileDb.retrieveSipProfileList();

        // If there're SIP profiles existing in DB, display a notification and delete all these
        // profiles.
        if (!sipProfileList.isEmpty()) {
            for (SipProfile profile : sipProfileList) {
                stopSipService(context, profile.getProfileName());
                removeSipProfile(profile.getProfileName());
                try {
                    profileDb.deleteProfile(profile);
                } catch (IOException e) {
                    // Ignore
                }
            }
            sendSipAccountsRemovedNotification(context, sipProfileList);
        }
    }

    private void sendSipAccountsRemovedNotification(Context context, List<SipProfile> profiles) {
        String sipAccounts = profiles.stream().map(p -> p.getProfileName())
                .collect(Collectors.joining(","));

        Intent intent = new Intent(TelecomManager.ACTION_CHANGE_PHONE_ACCOUNTS);
        intent.setFlags(Intent.FLAG_RECEIVER_INCLUDE_BACKGROUND);
        PendingIntent pendingIntent = PendingIntent.getActivity(context, 0, intent, 0);

        Notification.Action action = new Notification.Action.Builder(R.drawable.ic_sim_card,
                context.getString(R.string.sip_accounts_removed_notification_action),
                pendingIntent).build();
        Notification.Builder builder = new Notification.Builder(context)
                .setSmallIcon(R.drawable.ic_sim_card)
                .setChannelId(CHANNEL_ID_SIP_ACCOUNTS_REMOVED)
                .setContentTitle(context.getText(R.string.sip_accounts_removed_notification_title))
                .setStyle(new Notification.BigTextStyle()
                .bigText(context.getString(
                        R.string.sip_accounts_removed_notification_message,
                        sipAccounts)))
                .setAutoCancel(true)
                .addAction(action);
        Notification notification = builder.build();
        if (mNm != null) {
            mNm.notify(NOTIFICATION_TAG, SIP_ACCOUNTS_REMOVED_NOTIFICATION_ID,
                    notification);
        } else {
            log("NotificationManager is null when send the notification of removed SIP accounts");
        }
    }

    /**
     * Retrieves the {@link AccountEntry} from the registry with the specified name.
     *
     * @param sipProfileName Name of the SIP profile to retrieve.
     * @return The {@link AccountEntry}, or {@code null} is it was not found.
     */
    private AccountEntry getAccountEntry(String sipProfileName) {
        for (AccountEntry entry : mAccounts) {
            if (Objects.equals(sipProfileName, entry.getProfile().getProfileName())) {
                return entry;
            }
        }
        return null;
    }

    private void log(String message) {
        Log.d(SipUtil.LOG_TAG, PREFIX + message);
    }
}
