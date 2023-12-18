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
package com.android.car.admin;

import static android.app.Notification.EXTRA_TEXT;
import static android.app.Notification.EXTRA_TITLE;
import static android.app.Notification.FLAG_ONGOING_EVENT;

import static com.android.car.admin.NotificationHelper.CHANNEL_ID_DEFAULT;
import static com.android.car.admin.NotificationHelper.NEW_USER_DISCLAIMER_NOTIFICATION_ID;
import static com.android.dx.mockito.inline.extended.ExtendedMockito.doReturn;
import static com.android.dx.mockito.inline.extended.ExtendedMockito.verify;

import static com.google.common.truth.Truth.assertWithMessage;

import static org.mockito.Matchers.any;
import static org.mockito.Matchers.eq;
import static org.mockito.Mockito.spy;
import static org.mockito.Mockito.when;

import android.app.Notification;
import android.app.NotificationManager;
import android.app.PendingIntent;
import android.app.UiAutomation;
import android.car.test.mocks.AbstractExtendedMockitoTestCase;
import android.car.test.mocks.AndroidMockitoHelper;
import android.car.test.mocks.JavaMockitoHelper;
import android.content.Context;
import android.content.Intent;
import android.util.Log;
import android.widget.Button;

import androidx.test.ext.junit.runners.AndroidJUnit4;
import androidx.test.platform.app.InstrumentationRegistry;
import androidx.test.rule.ActivityTestRule;

import com.android.car.R;
import com.android.car.admin.ui.ManagedDeviceTextView;

import org.junit.Before;
import org.junit.Rule;
import org.junit.Test;
import org.junit.runner.RunWith;
import org.mockito.ArgumentCaptor;
import org.mockito.Mock;

import java.util.concurrent.CountDownLatch;

@RunWith(AndroidJUnit4.class)
public final class NewUserDisclaimerActivityTest extends AbstractExtendedMockitoTestCase {

    private static final String TAG = NewUserDisclaimerActivityTest.class.getSimpleName();

    private static final long TIMEOUT_MS = 1_000;

    private final Context mRealContext = InstrumentationRegistry.getInstrumentation()
            .getContext();

    private final UiAutomation mUiAutomation =
            InstrumentationRegistry.getInstrumentation().getUiAutomation();

    // NOTE: Cannot launch activity automatically as we need to mock
    // PerUserCarDevicePolicyService.getInstance() first
    @Rule
    public ActivityTestRule<NewUserDisclaimerActivity> mActivityRule = new ActivityTestRule(
            NewUserDisclaimerActivity.class,  /* initialTouchMode= */ false,
            /* launchActivity= */ false);

    private NewUserDisclaimerActivity mActivity;

    private Context mSpiedContext;

    @Mock
    private PerUserCarDevicePolicyService mService;

    @Mock
    private NotificationManager mNotificationManager;

    @Override
    protected void onSessionBuilder(CustomMockitoSessionBuilder session) {
        session.spyStatic(PerUserCarDevicePolicyService.class);
    }

    @Before
    public void setFixtures() {
        Log.v(TAG, "setFixtures(): mocking PerUserCarDevicePolicyService.getInstance()");
        doReturn(mService).when(() -> PerUserCarDevicePolicyService.getInstance(any()));
        mSpiedContext = spy(mRealContext);

        when(mSpiedContext.getSystemService(NotificationManager.class))
                .thenReturn(mNotificationManager);

        Log.v(TAG, "setFixtures(): launching activitiy");
        mActivity = mActivityRule.launchActivity(/* intent= */ null);

        // It's called onResume()
        verify(mService).setShown();
    }

    @Test
    public void testAccept() throws Exception {
        AndroidMockitoHelper.syncRunOnUiThread(mActivity, () -> {
            mActivity.onCreate(/* savedInstanceState= */ null);
            Button button = mActivity.getAcceptButton();
            Log.d(TAG, "Clicking accept button: " + button);
            button.performClick();
        });

        verify(mService).setAcknowledged();
        assertWithMessage("activity is finishing").that(mActivity.isFinishing()).isTrue();
    }

    @Test
    public void testShowNotification() {
        NewUserDisclaimerActivity.showNotification(mSpiedContext);

        ArgumentCaptor<Notification> captor = ArgumentCaptor.forClass(Notification.class);
        verify(mNotificationManager).notify(eq(NEW_USER_DISCLAIMER_NOTIFICATION_ID),
                captor.capture());

        Notification notification = captor.getValue();
        assertWithMessage("notification").that(notification).isNotNull();
        assertNotificationContents(notification);
    }

    @Test
    public void testCancelNotification() throws Exception {
        PendingIntent pendingIntent = NewUserDisclaimerActivity.getPendingIntent(mSpiedContext,
                /* extraFlags = */ 0);
        CountDownLatch cancelLatch = new CountDownLatch(1);
        pendingIntent.registerCancelListener(pi -> cancelLatch.countDown());

        NewUserDisclaimerActivity.cancelNotification(mSpiedContext);

        verify(mNotificationManager).cancel(NEW_USER_DISCLAIMER_NOTIFICATION_ID);

        // Assert pending intent was canceled (latch is counted down by the CancelListener)
        JavaMockitoHelper.await(cancelLatch, TIMEOUT_MS);
    }

    private void assertNotificationContents(Notification notification) {
        assertWithMessage("notification icon").that(notification.getSmallIcon()).isNotNull();
        assertWithMessage("notification channel").that(notification.getChannelId())
                .isEqualTo(CHANNEL_ID_DEFAULT);
        assertWithMessage("notification flags has FLAG_ONGOING_EVENT")
                .that(notification.flags & FLAG_ONGOING_EVENT).isEqualTo(FLAG_ONGOING_EVENT);

        assertWithMessage("notification content pending intent")
                .that(notification.contentIntent)
                .isNotNull();
        assertWithMessage("notification content pending intent is immutable")
                .that(notification.contentIntent.isImmutable()).isTrue();
        // Need android.permission.GET_INTENT_SENDER_INTENT to get the Intent
        Intent intent;
        mUiAutomation.adoptShellPermissionIdentity();
        try {
            intent = notification.contentIntent.getIntent();
        } finally {
            mUiAutomation.dropShellPermissionIdentity();
        }
        assertWithMessage("content intent").that(intent).isNotNull();
        assertWithMessage("content intent component").that(intent.getComponent())
                .isEqualTo(mActivity.getComponentName());

        assertWithMessage("notification extras").that(notification.extras).isNotNull();
        assertWithMessage("value of extra %s", EXTRA_TITLE)
                .that(notification.extras.getString(EXTRA_TITLE))
                .isEqualTo(mRealContext.getString(R.string.new_user_managed_notification_title));
        assertWithMessage("value of extra %s", EXTRA_TEXT)
                .that(notification.extras.getString(EXTRA_TEXT))
                .isEqualTo(ManagedDeviceTextView.getManagedDeviceText(mRealContext));
    }
}
