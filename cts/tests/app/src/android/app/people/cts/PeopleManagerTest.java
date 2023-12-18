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

package android.app.people.cts;

import static android.app.NotificationManager.IMPORTANCE_DEFAULT;
import static android.app.people.ConversationStatus.ACTIVITY_ANNIVERSARY;
import static android.app.people.ConversationStatus.ACTIVITY_GAME;
import static android.app.people.ConversationStatus.AVAILABILITY_AVAILABLE;
import static android.app.people.ConversationStatus.AVAILABILITY_BUSY;

import static junit.framework.Assert.fail;

import static java.lang.Thread.sleep;

import android.app.Notification;
import android.app.NotificationChannel;
import android.app.NotificationManager;
import android.app.Person;
import android.app.people.ConversationStatus;
import android.app.people.PeopleManager;
import android.app.stubs.R;
import android.app.stubs.SendBubbleActivity;
import android.content.Intent;
import android.content.pm.ShortcutInfo;
import android.content.pm.ShortcutManager;
import android.graphics.drawable.Icon;
import android.os.SystemClock;
import android.test.AndroidTestCase;
import android.util.ArraySet;

import androidx.test.InstrumentationRegistry;

import java.util.Arrays;
import java.util.Collections;
import java.util.List;
import java.util.Set;
import java.util.UUID;


public class PeopleManagerTest extends AndroidTestCase {
    final String TAG = PeopleManagerTest.class.getSimpleName();
    static final String NOTIFICATION_CHANNEL_ID = "PeopleManagerTest";
    static final String PERSON_CHANNEL_ID = "PersonTest";

    private static final String SHARE_SHORTCUT_ID = "shareShortcut";
    private static final String SHARE_SHORTCUT_ID2 = "shareShortcut2";
    private static final String SHARE_SHORTCUT_CATEGORY =
            "android.app.stubs.SHARE_SHORTCUT_CATEGORY";

    private static final long TIMEOUT_MS = 4000;

    private NotificationManager mNotificationManager;
    private ShortcutManager mShortcutManager;
    private PeopleManager mPeopleManager;
    private String mId;


    @Override
    protected void setUp() throws Exception {
        super.setUp();
        // This will leave a set of channels on the device with each test run.
        mId = UUID.randomUUID().toString();
        mNotificationManager = mContext.getSystemService(NotificationManager.class);
        mShortcutManager = mContext.getSystemService(ShortcutManager.class);
        mPeopleManager = mContext.getSystemService(PeopleManager.class);
        assertNotNull(mPeopleManager);

        createDynamicShortcut();
        mNotificationManager.createNotificationChannel(new NotificationChannel(
                NOTIFICATION_CHANNEL_ID, "name", IMPORTANCE_DEFAULT));
        NotificationChannel personChannel =
                new NotificationChannel(PERSON_CHANNEL_ID, "person", IMPORTANCE_DEFAULT);
        personChannel.setConversationId(NOTIFICATION_CHANNEL_ID, SHARE_SHORTCUT_ID);
        mNotificationManager.createNotificationChannel(personChannel);

        mNotificationManager.notify(177, getConversationNotification().build());
        sleep(500);
    }

    @Override
    protected void tearDown() throws Exception {
        super.tearDown();
        mNotificationManager.cancelAll();

        List<NotificationChannel> channels = mNotificationManager.getNotificationChannels();
        // Delete all channels.
        for (NotificationChannel nc : channels) {
            if (NotificationChannel.DEFAULT_CHANNEL_ID.equals(nc.getId())) {
                continue;
            }
            mNotificationManager.deleteNotificationChannel(nc.getId());
        }
        deleteShortcuts();
    }

    /** Creates a dynamic, longlived, sharing shortcut. Call {@link #deleteShortcuts()} after. */
    private void createDynamicShortcut() {
        Person person = new Person.Builder()
                .setBot(false)
                .setIcon(Icon.createWithResource(mContext, R.drawable.icon_black))
                .setName("BubbleBot")
                .setImportant(true)
                .build();

        Set<String> categorySet = new ArraySet<>();
        categorySet.add(SHARE_SHORTCUT_CATEGORY);
        Intent shortcutIntent = new Intent(mContext, SendBubbleActivity.class);
        shortcutIntent.setAction(Intent.ACTION_VIEW);

        ShortcutInfo shortcut = new ShortcutInfo.Builder(mContext, SHARE_SHORTCUT_ID)
                .setShortLabel(SHARE_SHORTCUT_ID)
                .setIcon(Icon.createWithResource(mContext, R.drawable.icon_black))
                .setIntent(shortcutIntent)
                .setPerson(person)
                .setCategories(categorySet)
                .setLongLived(true)
                .build();

        ShortcutInfo shortcut2 = new ShortcutInfo.Builder(mContext, SHARE_SHORTCUT_ID2)
                .setShortLabel(SHARE_SHORTCUT_ID2)
                .setIcon(Icon.createWithResource(mContext, R.drawable.icon_black))
                .setIntent(shortcutIntent)
                .setPerson(person)
                .setCategories(categorySet)
                .setLongLived(true)
                .build();

        mShortcutManager.addDynamicShortcuts(Arrays.asList(shortcut, shortcut2));
    }

    private void deleteShortcuts() {
        mShortcutManager.removeAllDynamicShortcuts();
        mShortcutManager.removeLongLivedShortcuts(Collections.singletonList(SHARE_SHORTCUT_ID));
    }

    private Notification.Builder getConversationNotification() {
        Person person = new Person.Builder()
                .setName("bubblebot")
                .build();
        Notification.Builder nb = new Notification.Builder(mContext, PERSON_CHANNEL_ID)
                .setContentTitle("foo")
                .setShortcutId(SHARE_SHORTCUT_ID)
                .setStyle(new Notification.MessagingStyle(person)
                        .setConversationTitle("Bubble Chat")
                        .addMessage("Hello?",
                                SystemClock.currentThreadTimeMillis() - 300000, person)
                        .addMessage("Is it me you're looking for?",
                                SystemClock.currentThreadTimeMillis(), person)
                )
                .setSmallIcon(android.R.drawable.sym_def_app_icon);
        return nb;
    }
    public void testIsConversationWithoutPermission() throws Exception {
        try {
            mPeopleManager.isConversation(mContext.getPackageName(), SHARE_SHORTCUT_ID);
            fail("Expected SecurityException");
        } catch (Exception e) {
            //expected
        }
    }

    public void testIsConversationWithPermission() throws Exception {
        try {
            InstrumentationRegistry.getInstrumentation().getUiAutomation()
                .adoptShellPermissionIdentity("android.permission.READ_PEOPLE_DATA");

            // Shortcut exists and has label
            assertTrue(mPeopleManager.isConversation(
                    mContext.getPackageName(), SHARE_SHORTCUT_ID));
            // Shortcut doesn't exist
            assertFalse(mPeopleManager.isConversation(
                    mContext.getPackageName(), SHARE_SHORTCUT_ID + 1));
        } finally {
            InstrumentationRegistry.getInstrumentation().getUiAutomation()
                .dropShellPermissionIdentity();
        }
    }

    public void testAddOrUpdateStatus_add() throws Exception {
        ConversationStatus cs = new ConversationStatus.Builder("id", ACTIVITY_GAME)
                .setAvailability(AVAILABILITY_AVAILABLE)
                .build();
        mPeopleManager.addOrUpdateStatus(SHARE_SHORTCUT_ID, cs);

        List<ConversationStatus> statuses = mPeopleManager.getStatuses(SHARE_SHORTCUT_ID);

        assertTrue(statuses.contains(cs));
    }

    public void testAddOrUpdateStatus_update() throws Exception {
        ConversationStatus.Builder cs = new ConversationStatus.Builder("id", ACTIVITY_GAME)
                .setAvailability(AVAILABILITY_AVAILABLE);
        mPeopleManager.addOrUpdateStatus(SHARE_SHORTCUT_ID, cs.build());

        List<ConversationStatus> statuses = mPeopleManager.getStatuses(SHARE_SHORTCUT_ID);
        assertTrue(statuses.contains(cs.build()));

        cs.setStartTimeMillis(100).setDescription("Playing chess");
        mPeopleManager.addOrUpdateStatus(SHARE_SHORTCUT_ID, cs.build());

        statuses = mPeopleManager.getStatuses(SHARE_SHORTCUT_ID);
        assertTrue(statuses.toString(), statuses.contains(cs.build()));
    }

    public void testGetStatuses() throws Exception {
        ConversationStatus cs = new ConversationStatus.Builder("id", ACTIVITY_GAME)
                .setAvailability(AVAILABILITY_BUSY)
                .build();
        mPeopleManager.addOrUpdateStatus(SHARE_SHORTCUT_ID, cs);

        ConversationStatus cs2 = new ConversationStatus.Builder("another", ACTIVITY_ANNIVERSARY)
                .setAvailability(AVAILABILITY_AVAILABLE)
                .build();
        mPeopleManager.addOrUpdateStatus(SHARE_SHORTCUT_ID, cs2);

        List<ConversationStatus> statuses = mPeopleManager.getStatuses(SHARE_SHORTCUT_ID);

        assertTrue(statuses.contains(cs));
        assertTrue(statuses.contains(cs2));
    }

    public void testGetStatuses_multipleShortcuts() throws Exception {
        ConversationStatus cs = new ConversationStatus.Builder("id", ACTIVITY_GAME)
                .setAvailability(AVAILABILITY_BUSY)
                .build();
        mPeopleManager.addOrUpdateStatus(SHARE_SHORTCUT_ID, cs);

        ConversationStatus cs2 = new ConversationStatus.Builder("another", ACTIVITY_ANNIVERSARY)
                .setAvailability(AVAILABILITY_AVAILABLE)
                .build();
        mPeopleManager.addOrUpdateStatus(SHARE_SHORTCUT_ID2, cs2);

        List<ConversationStatus> statuses = mPeopleManager.getStatuses(SHARE_SHORTCUT_ID);
        List<ConversationStatus> statuses2 = mPeopleManager.getStatuses(SHARE_SHORTCUT_ID2);
        assertTrue(statuses.contains(cs));
        assertTrue(statuses2.contains(cs2));
    }

    public void testClearStatuses() throws Exception {
        ConversationStatus cs = new ConversationStatus.Builder("id", ACTIVITY_GAME)
                .setAvailability(AVAILABILITY_BUSY)
                .build();
        mPeopleManager.addOrUpdateStatus(SHARE_SHORTCUT_ID, cs);

        ConversationStatus cs2 = new ConversationStatus.Builder("another", ACTIVITY_ANNIVERSARY)
                .setAvailability(AVAILABILITY_AVAILABLE)
                .build();
        mPeopleManager.addOrUpdateStatus(SHARE_SHORTCUT_ID, cs2);

        mPeopleManager.clearStatuses(SHARE_SHORTCUT_ID);
        List<ConversationStatus> statuses = mPeopleManager.getStatuses(SHARE_SHORTCUT_ID);

        assertTrue(statuses.isEmpty());
    }

    public void testClearStatus() throws Exception {
        ConversationStatus cs = new ConversationStatus.Builder("id", ACTIVITY_GAME)
                .setAvailability(AVAILABILITY_BUSY)
                .build();
        mPeopleManager.addOrUpdateStatus(SHARE_SHORTCUT_ID, cs);

        ConversationStatus cs2 = new ConversationStatus.Builder("another", ACTIVITY_ANNIVERSARY)
                .setAvailability(AVAILABILITY_AVAILABLE)
                .build();
        mPeopleManager.addOrUpdateStatus(SHARE_SHORTCUT_ID, cs2);

        mPeopleManager.clearStatus(SHARE_SHORTCUT_ID, cs2.getId());
        List<ConversationStatus> statuses = mPeopleManager.getStatuses(SHARE_SHORTCUT_ID);

        assertTrue(statuses.contains(cs));
        assertFalse(statuses.contains(cs2));
    }
}
