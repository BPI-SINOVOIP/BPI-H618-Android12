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

package com.android.car;

import static com.google.common.truth.Truth.assertThat;

import static org.mockito.ArgumentMatchers.any;
import static org.mockito.ArgumentMatchers.anyInt;
import static org.mockito.ArgumentMatchers.anyLong;
import static org.mockito.Mockito.doReturn;
import static org.mockito.Mockito.mock;
import static org.mockito.Mockito.spy;
import static org.mockito.Mockito.when;

import android.annotation.UserIdInt;
import android.app.IActivityManager;
import android.bluetooth.BluetoothAdapter;
import android.car.testapi.BlockingUserLifecycleListener;
import android.car.user.CarUserManager;
import android.content.ContentResolver;
import android.content.Context;
import android.content.pm.UserInfo;
import android.content.res.Resources;
import android.os.Handler;
import android.os.Looper;
import android.os.UserHandle;
import android.os.UserManager;
import android.provider.Settings;
import android.telecom.TelecomManager;
import android.test.mock.MockContentResolver;

import androidx.test.core.app.ApplicationProvider;

import com.android.car.hal.InputHalService;
import com.android.car.hal.UserHalService;
import com.android.car.internal.common.CommonConstants.UserLifecycleEventType;
import com.android.car.user.CarUserService;
import com.android.internal.app.AssistUtils;
import com.android.internal.util.test.BroadcastInterceptingContext;
import com.android.internal.util.test.FakeSettingsProvider;

import org.junit.After;
import org.junit.Before;
import org.junit.Test;
import org.junit.runner.RunWith;
import org.mockito.Mock;
import org.mockito.Spy;
import org.mockito.junit.MockitoJUnitRunner;

import java.util.function.BooleanSupplier;
import java.util.function.IntSupplier;
import java.util.function.Supplier;

@RunWith(MockitoJUnitRunner.class)
public class CarInputRotaryServiceTest {

    // TODO(b/152069895): decrease value once refactored. In fact, it should not even use
    // runWithScissors(), but only rely on CountdownLatches
    private static final long DEFAULT_TIMEOUT_MS = 5_000;

    @Mock private InputHalService mInputHalService;
    @Mock private TelecomManager mTelecomManager;
    @Mock private AssistUtils mAssistUtils;
    @Mock private CarInputService.KeyEventListener mDefaultMainListener;
    @Mock private Supplier<String> mLastCallSupplier;
    @Mock private IntSupplier mLongPressDelaySupplier;
    @Mock private BooleanSupplier mShouldCallButtonEndOngoingCallSupplier;
    @Mock private InputCaptureClientController mCaptureController;
    @Mock private CarOccupantZoneService mCarOccupantZoneService;
    @Mock private CarUxRestrictionsManagerService mUxRestrictionService;
    @Mock private BluetoothAdapter mBluetoothAdapter;

    @Spy private final Context mContext = ApplicationProvider.getApplicationContext();
    @Spy private final Handler mHandler = new Handler(Looper.getMainLooper());

    private MockContext mMockContext;
    private CarUserService mCarUserService;
    private CarInputService mCarInputService;

    /**
     * A mock {@link Context}.
     * This class uses a mock {@link ContentResolver} and
     * {@link android.content.ContentProvider} to
     * avoid changing real system settings. Besides, to emulate the case where the OEM changes
     * {@link R.string.rotaryService} to empty in the resource file (e.g., the OEM doesn't
     * want to
     * start RotaryService), this class allows to return a given String when retrieving {@link
     * R.string.rotaryService}.
     */
    private static class MockContext extends BroadcastInterceptingContext {
        private final MockContentResolver mContentResolver;
        private final FakeSettingsProvider mContentProvider;
        private final Resources mResources;

        MockContext(Context base, String rotaryService) {
            super(base);
            FakeSettingsProvider.clearSettingsProvider();
            mContentResolver = new MockContentResolver(this);
            mContentProvider = new FakeSettingsProvider();
            mContentResolver.addProvider(Settings.AUTHORITY, mContentProvider);

            mResources = spy(base.getResources());
            doReturn(rotaryService).when(mResources).getString(R.string.rotaryService);
        }

        void release() {
            FakeSettingsProvider.clearSettingsProvider();
        }

        @Override
        public ContentResolver getContentResolver() {
            return mContentResolver;
        }

        @Override
        public Resources getResources() {
            return mResources;
        }
    }

    @Before
    public void setUp() {
        when(mInputHalService.isKeyInputSupported()).thenReturn(true);
        // Delay Handler callbacks until flushHandler() is called.
        doReturn(true).when(mHandler).sendMessageAtTime(any(), anyLong());
    }

    @Test
    public void rotaryServiceSettingsUpdated_whenRotaryServiceIsNotEmpty() throws Exception {
        final String rotaryService = "com.android.car.rotary/com.android.car.rotary.RotaryService";
        init(rotaryService);
        assertThat(mMockContext.getString(R.string.rotaryService)).isEqualTo(rotaryService);

        final int userId = 11;

        // By default RotaryService is not enabled.
        String enabledServices = Settings.Secure.getStringForUser(
                mMockContext.getContentResolver(),
                Settings.Secure.ENABLED_ACCESSIBILITY_SERVICES,
                userId);
        assertThat(enabledServices == null ? "" : enabledServices).doesNotContain(rotaryService);

        String enabled = Settings.Secure.getStringForUser(
                mMockContext.getContentResolver(),
                Settings.Secure.ACCESSIBILITY_ENABLED,
                userId);
        assertThat(enabled).isNull();

        // Enable RotaryService by sending user switch event.
        sendUserLifecycleEvent(CarUserManager.USER_LIFECYCLE_EVENT_TYPE_SWITCHING, userId);

        enabledServices = Settings.Secure.getStringForUser(
                mMockContext.getContentResolver(),
                Settings.Secure.ENABLED_ACCESSIBILITY_SERVICES,
                userId);
        assertThat(enabledServices).contains(rotaryService);

        enabled = Settings.Secure.getStringForUser(
                mMockContext.getContentResolver(),
                Settings.Secure.ACCESSIBILITY_ENABLED,
                userId);
        assertThat(enabled).isEqualTo("1");
    }

    @Test
    public void rotaryServiceSettingsNotUpdated_whenRotaryServiceIsEmpty() throws Exception {
        final String rotaryService = "";
        init(rotaryService);
        assertThat(mMockContext.getString(R.string.rotaryService)).isEqualTo(rotaryService);

        final int userId = 11;

        // By default the Accessibility is disabled.
        String enabled = Settings.Secure.getStringForUser(
                mMockContext.getContentResolver(),
                Settings.Secure.ACCESSIBILITY_ENABLED,
                userId);
        assertThat(enabled).isNull();

        sendUserLifecycleEvent(CarUserManager.USER_LIFECYCLE_EVENT_TYPE_SWITCHING, userId);

        // Sending user switch event shouldn't enable the Accessibility because RotaryService is
        // empty.
        enabled = Settings.Secure.getStringForUser(
                mMockContext.getContentResolver(),
                Settings.Secure.ACCESSIBILITY_ENABLED,
                userId);
        assertThat(enabled).isNull();
    }

    @After
    public void tearDown() {
        if (mMockContext != null) {
            mMockContext.release();
            mMockContext = null;
        }
    }

    /**
     * Initializes {@link #mMockContext}, {@link #mCarUserService}, and {@link #mCarInputService}.
     */
    private void init(String rotaryService) {
        mMockContext = new MockContext(mContext, rotaryService);

        UserManager userManager = mock(UserManager.class);
        UserInfo userInfo = mock(UserInfo.class);
        doReturn(userInfo).when(userManager).getUserInfo(anyInt());
        UserHalService userHal = mock(UserHalService.class);
        IActivityManager iActivityManager = mock(IActivityManager.class);
        mCarUserService = new CarUserService(mMockContext, userHal,
                userManager, iActivityManager, /* maxRunningUsers= */ 2, mUxRestrictionService);

        mCarInputService = new CarInputService(mMockContext, mInputHalService, mCarUserService,
                mCarOccupantZoneService, mHandler, mTelecomManager, mAssistUtils,
                mDefaultMainListener, mLastCallSupplier, mLongPressDelaySupplier,
                mShouldCallButtonEndOngoingCallSupplier, mCaptureController, mBluetoothAdapter);
        mCarInputService.init();
    }

    private void sendUserLifecycleEvent(@UserLifecycleEventType int eventType,
            @UserIdInt int userId) throws InterruptedException {
        // Add a blocking listener to ensure CarUserService event notification is completed
        // before proceeding with test execution.
        BlockingUserLifecycleListener blockingListener =
                BlockingUserLifecycleListener.forAnyEvent().build();
        mCarUserService.addUserLifecycleListener(blockingListener);

        runOnMainThreadAndWaitForIdle(() -> mCarUserService.onUserLifecycleEvent(eventType,
                /* fromUserId= */ UserHandle.USER_NULL, userId));
        blockingListener.waitForAnyEvent();
    }

    private static void runOnMainThreadAndWaitForIdle(Runnable r) {
        Handler.getMain().runWithScissors(r, DEFAULT_TIMEOUT_MS);
        // Run empty runnable to make sure that all posted handlers are done.
        Handler.getMain().runWithScissors(() -> {
        }, DEFAULT_TIMEOUT_MS);
    }
}
