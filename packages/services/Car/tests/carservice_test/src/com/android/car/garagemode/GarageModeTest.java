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

package com.android.car.garagemode;

import static com.google.common.truth.Truth.assertThat;
import static com.google.common.truth.Truth.assertWithMessage;

import static org.mockito.ArgumentMatchers.any;
import static org.mockito.ArgumentMatchers.anyInt;
import static org.mockito.Mockito.doAnswer;
import static org.mockito.Mockito.verify;
import static org.mockito.Mockito.when;

import android.app.job.JobScheduler;
import android.car.user.CarUserManager;
import android.car.user.CarUserManager.UserLifecycleEvent;
import android.car.user.CarUserManager.UserLifecycleListener;
import android.os.Handler;
import android.os.Looper;

import androidx.test.ext.junit.runners.AndroidJUnit4;
import androidx.test.filters.SmallTest;

import com.android.car.CarLocalServices;
import com.android.car.user.CarUserService;

import org.junit.After;
import org.junit.Before;
import org.junit.Rule;
import org.junit.Test;
import org.junit.runner.RunWith;
import org.mockito.ArgumentCaptor;
import org.mockito.Mock;
import org.mockito.junit.MockitoJUnit;
import org.mockito.junit.MockitoRule;

import java.util.ArrayList;
import java.util.Arrays;
import java.util.concurrent.CountDownLatch;
import java.util.concurrent.TimeUnit;

@RunWith(AndroidJUnit4.class)
@SmallTest
public final class GarageModeTest {

    private static final int DEFAULT_TIMEOUT_MS = 100;

    @Rule
    public final MockitoRule rule = MockitoJUnit.rule();
    private GarageMode mGarageMode;
    @Mock
    private Controller mController;
    @Mock
    private JobScheduler mJobScheduler;
    @Mock
    private CarUserService mCarUserService;
    private final Handler mHandler = new Handler(Looper.getMainLooper());

    @Before
    public void setUp() {
        when(mController.getHandler()).thenReturn(mHandler);
        when(mController.getJobSchedulerService()).thenReturn(mJobScheduler);

        mGarageMode = new GarageMode(mController);
        CarLocalServices.removeServiceForTest(CarUserService.class);
        CarLocalServices.addService(CarUserService.class, mCarUserService);
        mGarageMode.init();
    }

    @After
    public void teardown() {
        CarLocalServices.removeServiceForTest(CarUserService.class);
    }

    @Test
    public void test_releaseRemoveListener() {
        mGarageMode.release();

        verify(mCarUserService).removeUserLifecycleListener(any());
    }

    @Test
    public void test_backgroundUsersStopedOnGarageModeCancel() throws Exception {
        ArrayList<Integer> userToStartInBackground = new ArrayList<>(Arrays.asList(101, 102, 103));
        when(mCarUserService.startAllBackgroundUsersInGarageMode())
                .thenReturn(userToStartInBackground);

        CountDownLatch latch = new CountDownLatch(3); // 3 for three users
        mockCarUserServiceStopUserCall(getEventListener(), latch);

        mGarageMode.enterGarageMode(/* future= */ null);

        mGarageMode.cancel();

        waitForHandlerThreadToFinish(latch);
        verify(mCarUserService).startAllBackgroundUsersInGarageMode();
        verify(mCarUserService).stopBackgroundUserInGagageMode(101);
        verify(mCarUserService).stopBackgroundUserInGagageMode(102);
        verify(mCarUserService).stopBackgroundUserInGagageMode(103);
    }

    @Test
    public void test_restartingGarageModeStorePreviouslyStartedUsers() throws Exception {
        ArrayList<Integer> userToStartInBackground = new ArrayList<>(Arrays.asList(101, 102, 103));
        CountDownLatch latch = mockCarUserServiceStartUsersCall(userToStartInBackground);
        mGarageMode.enterGarageMode(/* future= */ null);

        waitForHandlerThreadToFinish(latch);
        assertThat(mGarageMode.getStartedBackgroundUsers()).containsExactly(101, 102, 103);

        userToStartInBackground = new ArrayList<>(Arrays.asList(103, 104, 105));
        latch = mockCarUserServiceStartUsersCall(userToStartInBackground);
        mGarageMode.enterGarageMode(/* future= */ null);

        waitForHandlerThreadToFinish(latch);
        assertThat(mGarageMode.getStartedBackgroundUsers()).containsExactly(101, 102, 103, 104,
                105);
    }

    private void waitForHandlerThreadToFinish(CountDownLatch latch) throws Exception {
        assertWithMessage("Latch has timed out.")
                .that(latch.await(DEFAULT_TIMEOUT_MS, TimeUnit.MILLISECONDS)).isTrue();
        mHandler.runWithScissors(() -> {}, DEFAULT_TIMEOUT_MS);
    }

    private CountDownLatch mockCarUserServiceStartUsersCall(
            ArrayList<Integer> userToStartInBackground) {
        CountDownLatch latch = new CountDownLatch(1);
        doAnswer(inv -> {
            latch.countDown();
            return userToStartInBackground;
        }).when(mCarUserService).startAllBackgroundUsersInGarageMode();

        return latch;
    }

    private UserLifecycleListener getEventListener() {
        ArgumentCaptor<UserLifecycleListener> listenerCaptor =
                ArgumentCaptor.forClass(UserLifecycleListener.class);
        verify(mCarUserService).addUserLifecycleListener(listenerCaptor.capture());
        UserLifecycleListener listener = listenerCaptor.getValue();
        return listener;
    }

    private void mockCarUserServiceStopUserCall(UserLifecycleListener listener,
            CountDownLatch latch) {
        doAnswer(inv -> {
            int userId = (int) inv.getArguments()[0];
            latch.countDown();
            listener.onEvent(new UserLifecycleEvent(
                    CarUserManager.USER_LIFECYCLE_EVENT_TYPE_STOPPED, userId));
            return null;
        }).when(mCarUserService).stopBackgroundUserInGagageMode(anyInt());
    }
}

