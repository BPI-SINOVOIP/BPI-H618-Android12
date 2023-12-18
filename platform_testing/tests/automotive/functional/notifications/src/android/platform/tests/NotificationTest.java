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

package android.platform.tests;

import static junit.framework.Assert.assertFalse;
import static junit.framework.Assert.assertTrue;

import android.platform.helpers.AutoUtility;
import android.platform.helpers.HelperAccessor;
import android.platform.helpers.IAutoNotificationMockingHelper;
import android.platform.helpers.IAutoNotificationHelper;
import androidx.test.runner.AndroidJUnit4;

import org.junit.After;
import org.junit.Before;
import org.junit.BeforeClass;
import org.junit.runner.RunWith;
import org.junit.Test;

@RunWith(AndroidJUnit4.class)
public class NotificationTest {
    private HelperAccessor<IAutoNotificationHelper> mNotificationHelper;
    private HelperAccessor<IAutoNotificationMockingHelper> mNotificationMockingHelper;

    private static String NOTIFICATION_TITLE = "AUTO TEST NOTIFICATION";

    public NotificationTest() {
        mNotificationHelper = new HelperAccessor<>(IAutoNotificationHelper.class);
        mNotificationMockingHelper = new HelperAccessor<>(IAutoNotificationMockingHelper.class);
    }

    @BeforeClass
    public static void exitSuw() {
        AutoUtility.exitSuw();
    }

    @Before
    public void clearAllNotification() {
        mNotificationMockingHelper.get().clearAllNotification();
    }

    @After
    public void exit() {
        mNotificationHelper.get().exit();
    }

    @Test
    public void testOpenCloseNotification() {
        mNotificationHelper.get().open();
        assertTrue("Notification did not open.", mNotificationHelper.get().isAppInForeground());
        mNotificationHelper.get().exit();
        assertFalse("Notification did not close.", mNotificationHelper.get().isAppInForeground());
    }

    @Test
    public void testClearAllNotification() {
        mNotificationMockingHelper.get().postNotifications(1);
        mNotificationHelper.get().tapClearAllBtn();
        mNotificationHelper.get().exit();
        assertFalse(
                "Notifications were not cleared.",
                mNotificationHelper.get().checkNotificationExists(NOTIFICATION_TITLE));
    }

    @Test
    public void testPostNotification() {
        mNotificationMockingHelper.get().postNotifications(1);
        assertTrue(
                "Unable to find posted notification.",
                mNotificationHelper.get().checkNotificationExists(NOTIFICATION_TITLE));
    }

    @Test
    public void testSwipeAwayNotification() {
        mNotificationMockingHelper.get().postNotifications(1);
        assertTrue(
                "Unable to find posted notification.",
                mNotificationHelper.get().checkNotificationExists(NOTIFICATION_TITLE));
        mNotificationHelper.get().removeNotification(NOTIFICATION_TITLE);
        assertFalse(
                "Notifications were not cleared.",
                mNotificationHelper.get().checkNotificationExists(NOTIFICATION_TITLE));
    }

    @Test
    public void testSwipeDownNotification() {
        mNotificationHelper.get().openNotification();
        assertTrue("Notification is not open", mNotificationHelper.get().isAppInForeground());
    }
}
