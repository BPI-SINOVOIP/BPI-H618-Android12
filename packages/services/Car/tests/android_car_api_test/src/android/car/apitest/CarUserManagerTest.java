/*
 * Copyright (C) 2020 The Android Open Source Project
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
package android.car.apitest;

import static android.car.test.util.UserTestingHelper.clearUserLockCredentials;
import static android.car.test.util.UserTestingHelper.setMaxSupportedUsers;
import static android.car.test.util.UserTestingHelper.setUserLockCredentials;
import static android.car.user.CarUserManager.USER_LIFECYCLE_EVENT_TYPE_SWITCHING;
import static android.car.user.CarUserManager.USER_LIFECYCLE_EVENT_TYPE_UNLOCKED;

import static com.google.common.truth.Truth.assertWithMessage;

import android.app.ActivityManager;
import android.car.testapi.BlockingUserLifecycleListener;
import android.car.user.CarUserManager.UserLifecycleEvent;
import android.content.pm.UserInfo;
import android.os.UserManager;
import android.util.Log;

import org.junit.AfterClass;
import org.junit.BeforeClass;
import org.junit.Test;

public final class CarUserManagerTest extends CarMultiUserTestBase {

    private static final String TAG = CarUserManagerTest.class.getSimpleName();

    private static final int PIN = 2345;

    private static final int SWITCH_TIMEOUT_MS = 70_000;

    private static final int sMaxNumberUsersBefore = UserManager.getMaxSupportedUsers();
    private static boolean sChangedMaxNumberUsers;

    @BeforeClass
    public static void setupMaxNumberOfUsers() {
        int requiredUsers = 3; // system user, current user, 1 extra user
        if (sMaxNumberUsersBefore < requiredUsers) {
            sChangedMaxNumberUsers = true;
            Log.i(TAG, "Increasing maximing number of users from " + sMaxNumberUsersBefore + " to "
                    + requiredUsers);
            setMaxSupportedUsers(requiredUsers);
        }
    }

    @AfterClass
    public static void restoreMaxNumberOfUsers() {
        if (sChangedMaxNumberUsers) {
            Log.i(TAG, "Restoring maximum number of users to " + sMaxNumberUsersBefore);
            setMaxSupportedUsers(sMaxNumberUsersBefore);
        }
    }

    @Test
    public void testCreateUser() throws Exception {
        UserInfo newUser = createUser("DaNewUserInTheBlock");
        assertWithMessage("(%s).isGuest()", newUser.toFullString()).that(newUser.isGuest())
                .isFalse();

        assertWithMessage("user(%s).name", newUser.toFullString()).that(newUser.name)
                .contains("DaNewUserInTheBlock");

        // Make sure the user exists
        UserInfo loadedUser = getUser(newUser.id);
        assertUserInfo(newUser, loadedUser);
    }

    @Test
    public void testCreateGuest() throws Exception {
        UserInfo newGuest = createGuest("DaNewGuestInTheBlock");
        assertWithMessage("(%s).isGuest()", newGuest.toFullString()).that(newGuest.isGuest())
                .isTrue();

        assertWithMessage("guest(%s).name ", newGuest.toFullString()).that(newGuest.name)
                .contains("DaNewGuestInTheBlock");

        // Make sure the guest exists
        UserInfo loadedGuest = getUser(newGuest.id);
        assertUserInfo(newGuest, loadedGuest);
    }

    /**
     * Tests resume behavior when current user is ephemeral guest, a new guest user should be
     * created and switched to.
     */
    @Test
    public void testGuestUserResumeToNewGuestUser() throws Exception {
        // Create new guest user
        UserInfo guestUser = createGuest();
        int guestUserId = guestUser.id;

        // Wait for this user to be active
        BlockingUserLifecycleListener listener1 = BlockingUserLifecycleListener
                .forSpecificEvents()
                .forUser(guestUserId)
                .setTimeout(SWITCH_TIMEOUT_MS)
                .addExpectedEvent(USER_LIFECYCLE_EVENT_TYPE_UNLOCKED)
                .build();
        mCarUserManager.addListener(Runnable::run, listener1);
        switchUser(guestUserId);
        listener1.waitForEvents();
        mCarUserManager.removeListener(listener1);

        BlockingUserLifecycleListener listener2 = BlockingUserLifecycleListener
                .forSpecificEvents()
                .forPreviousUser(guestUserId)
                .setTimeout(SWITCH_TIMEOUT_MS)
                .addExpectedEvent(USER_LIFECYCLE_EVENT_TYPE_SWITCHING)
                .build();
        // Make sure listener callback was executed in the proper threaqd
        mCarUserManager.addListener(Runnable::run, listener2);

        // Emulate suspend to RAM
        suspendToRamAndResume(/* disableBackgroundUsersStart=*/ true);
        UserLifecycleEvent event = listener2.waitForEvents().get(0);

        int newGuestId = event.getUserId();

        assertWithMessage("userId on event %s", event).that(newGuestId)
                .isNotEqualTo(guestUserId);
        assertWithMessage("current user id").that(newGuestId).isEqualTo(getCurrentUserId());
        UserInfo newGuest = mUserManager.getUserInfo(newGuestId);
        assertWithMessage("new user (%s) is a guest", newGuest.toFullString())
                .that(newGuest.isGuest()).isTrue();
        assertWithMessage("name of new guest(%s)", newGuest.toFullString())
                .that(newGuest.name).isNotEqualTo(guestUser.name);
        mCarUserManager.removeListener(listener2);
    }

    /**
     * Tests resume behavior when current user is guest but with secured lock screen,
     * resume to same guest user.
     */
    @Test
    public void testSecuredGuestUserResumeToSameUser() throws Exception {
        // Create new guest user
        UserInfo guestUser = createGuest();
        int guestUserId = guestUser.id;

        // Wait for this user to be active
        BlockingUserLifecycleListener listener = BlockingUserLifecycleListener
                .forSpecificEvents()
                .forUser(guestUserId)
                .setTimeout(SWITCH_TIMEOUT_MS)
                .addExpectedEvent(USER_LIFECYCLE_EVENT_TYPE_UNLOCKED)
                .build();
        mCarUserManager.addListener(Runnable::run, listener);

        switchUser(guestUserId);

        listener.waitForEvents();
        mCarUserManager.removeListener(listener);

        setUserLockCredentials(guestUserId, PIN);
        try {
            // Emulate suspend to RAM
            suspendToRamAndResume(/* disableBackgroundUsersStart=*/ true);

            assertWithMessage("current user remains guest user (%s)", guestUser)
                    .that(getCurrentUserId()).isEqualTo(guestUserId);
        } finally {
            clearUserLockCredentials(guestUserId, PIN);
        }
    }

    /**
     * Tests resume behavior when current user is persistent user.
     */
    @Test
    public void testPersistentUserResumeToUser() throws Exception {
        int newUserId = createUser().id;
        BlockingUserLifecycleListener listener = BlockingUserLifecycleListener
                .forSpecificEvents()
                .forUser(newUserId)
                .setTimeout(SWITCH_TIMEOUT_MS)
                .addExpectedEvent(USER_LIFECYCLE_EVENT_TYPE_UNLOCKED)
                .build();
        mCarUserManager.addListener(Runnable::run, listener);
        switchUser(newUserId);
        listener.waitForEvents();

        // Emulate suspend to RAM
        suspendToRamAndResume(/* disableBackgroundUsersStart=*/ true);

        listener.waitForEvents();
        assertWithMessage("current user remains new user (%s)", newUserId)
                .that(ActivityManager.getCurrentUser()).isEqualTo(newUserId);

        mCarUserManager.removeListener(listener);
    }
}
