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

import static android.app.admin.DevicePolicyManager.ACTION_SHOW_NEW_USER_DISCLAIMER;

import static com.android.car.admin.PerUserCarDevicePolicyService.NEW_USER_DISCLAIMER_STATUS_ACKED;
import static com.android.car.admin.PerUserCarDevicePolicyService.NEW_USER_DISCLAIMER_STATUS_NEVER_RECEIVED;
import static com.android.car.admin.PerUserCarDevicePolicyService.NEW_USER_DISCLAIMER_STATUS_NOTIFICATION_SENT;
import static com.android.car.admin.PerUserCarDevicePolicyService.NEW_USER_DISCLAIMER_STATUS_RECEIVED;
import static com.android.car.admin.PerUserCarDevicePolicyService.NEW_USER_DISCLAIMER_STATUS_SHOWN;
import static com.android.car.admin.PerUserCarDevicePolicyService.newUserDisclaimerStatusToString;
import static com.android.dx.mockito.inline.extended.ExtendedMockito.doAnswer;
import static com.android.dx.mockito.inline.extended.ExtendedMockito.doNothing;
import static com.android.dx.mockito.inline.extended.ExtendedMockito.verify;

import static com.google.common.truth.Truth.assertWithMessage;

import static org.mockito.Matchers.any;
import static org.mockito.Mockito.verify;
import static org.mockito.Mockito.when;
import static org.testng.Assert.expectThrows;

import android.app.admin.DevicePolicyManager;
import android.car.test.mocks.AbstractExtendedMockitoTestCase;
import android.content.BroadcastReceiver;
import android.content.Context;
import android.content.Intent;

import com.android.car.admin.PerUserCarDevicePolicyService.NewUserDisclaimerStatus;

import org.junit.Before;
import org.junit.Test;
import org.mockito.ArgumentCaptor;
import org.mockito.Mock;

public final class PerUserCarDevicePolicyServiceTest extends AbstractExtendedMockitoTestCase {

    @Mock
    private Context mContext;

    private PerUserCarDevicePolicyService mInstance;

    @Mock
    private DevicePolicyManager mDpm;

    @Override
    protected void onSessionBuilder(CustomMockitoSessionBuilder session) {
        session.spyStatic(NewUserDisclaimerActivity.class);
    }

    @Before
    public void setFixtures() {
        when(mContext.getApplicationContext()).thenReturn(mContext);
        when(mContext.getSystemService(DevicePolicyManager.class)).thenReturn(mDpm);

        mInstance = new PerUserCarDevicePolicyService(mContext);
        assertStatusString(NEW_USER_DISCLAIMER_STATUS_NEVER_RECEIVED);
    }

    @Test
    public void testGetInstance() {
        PerUserCarDevicePolicyService instance1 = PerUserCarDevicePolicyService
                .getInstance(mContext);
        assertWithMessage("getInstance()#1").that(instance1).isNotNull();
        assertWithMessage("getInstance()#1").that(instance1).isNotSameInstanceAs(mInstance);

        PerUserCarDevicePolicyService instance2 = PerUserCarDevicePolicyService
                .getInstance(mContext);
        assertWithMessage("getInstance()#2").that(instance2).isNotNull();
        assertWithMessage("getInstance()#2").that(instance2).isNotSameInstanceAs(mInstance);

        assertWithMessage("getInstance()#2").that(instance2).isSameInstanceAs(instance1);
        assertWithMessage("getInstance()#1").that(instance1).isSameInstanceAs(instance2);
    }

    @Test
    public void testGetInstance_nullContext() {
        NullPointerException exception = expectThrows(NullPointerException.class,
                () -> PerUserCarDevicePolicyService.getInstance(null));
        assertWithMessage("exception message").that(exception.getMessage()).contains("context");
    }

    @Test
    public void testCreateAndDestroy() {
        BroadcastReceiver receiver = callOnCreate();

        callOnDestroy(receiver);
    }

    @Test
    public void testShowWhenIntentReceived() {
        doAnswer((inv) -> {
            assertStatusString(NEW_USER_DISCLAIMER_STATUS_RECEIVED);
            return null;
        }).when(() -> NewUserDisclaimerActivity.showNotification(any()));
        BroadcastReceiver receiver  = callOnCreate();

        sendShowNewUserDisclaimerBroadcast(receiver);

        assertStatusString(NEW_USER_DISCLAIMER_STATUS_NOTIFICATION_SENT);
        verify(() -> NewUserDisclaimerActivity.showNotification(mContext));
    }

    @Test
    public void testSetShown() {
        mInstance.setShown();

        assertStatusString(NEW_USER_DISCLAIMER_STATUS_SHOWN);
    }

    @Test
    public void testSetAcknowledged() {
        doNothing().when(() -> NewUserDisclaimerActivity.cancelNotification(any()));

        mInstance.setAcknowledged();

        assertStatusString(NEW_USER_DISCLAIMER_STATUS_ACKED);
        verify(() -> NewUserDisclaimerActivity.cancelNotification(mContext));

        verify(mDpm).resetNewUserDisclaimer();
    }

    private BroadcastReceiver callOnCreate() {
        ArgumentCaptor<BroadcastReceiver> captor = ArgumentCaptor.forClass(BroadcastReceiver.class);

        mInstance.onCreate();

        verify(mContext).registerReceiver(captor.capture(), any());
        BroadcastReceiver receiver = captor.getValue();
        assertWithMessage("BroadcastReceiver captured on onCreate()").that(receiver).isNotNull();

        return receiver;
    }

    private void callOnDestroy(BroadcastReceiver receiver) {
        mInstance.onDestroy();

        verify(mContext).unregisterReceiver(receiver);
    }

    private void sendShowNewUserDisclaimerBroadcast(BroadcastReceiver receiver) {
        receiver.onReceive(mContext, new Intent(ACTION_SHOW_NEW_USER_DISCLAIMER));
    }

    private void assertStatusString(@NewUserDisclaimerStatus int expectedStatus) {
        int actualStatus = mInstance.getNewUserDisclaimerStatus();
        assertWithMessage("newUserDisclaimerStatus (%s=%s, %s=%s)",
                expectedStatus, newUserDisclaimerStatusToString(expectedStatus),
                actualStatus, newUserDisclaimerStatusToString(actualStatus))
                        .that(actualStatus).isEqualTo(expectedStatus);
    }
}
