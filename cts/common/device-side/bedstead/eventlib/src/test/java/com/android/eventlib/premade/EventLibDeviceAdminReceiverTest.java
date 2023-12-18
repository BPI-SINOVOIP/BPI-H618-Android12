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

package com.android.eventlib.premade;

import static com.google.common.truth.Truth.assertThat;

import android.app.admin.DevicePolicyManager;
import android.content.ComponentName;
import android.content.Context;
import android.content.Intent;

import com.android.bedstead.nene.TestApis;
import com.android.bedstead.nene.devicepolicy.DeviceOwner;
import com.android.bedstead.nene.devicepolicy.ProfileOwner;
import com.android.bedstead.nene.users.UserReference;
import com.android.eventlib.EventLogs;
import com.android.eventlib.events.deviceadminreceivers.DeviceAdminDisableRequestedEvent;
import com.android.eventlib.events.deviceadminreceivers.DeviceAdminDisabledEvent;
import com.android.eventlib.events.deviceadminreceivers.DeviceAdminEnabledEvent;
import com.android.eventlib.events.deviceadminreceivers.DeviceAdminPasswordChangedEvent;
import com.android.eventlib.events.deviceadminreceivers.DeviceAdminPasswordFailedEvent;
import com.android.eventlib.events.deviceadminreceivers.DeviceAdminPasswordSucceededEvent;
import com.android.eventlib.events.deviceadminreceivers.DeviceAdminSystemUpdatePendingEvent;

import org.junit.Before;
import org.junit.Test;
import org.junit.runner.RunWith;
import org.junit.runners.JUnit4;

@RunWith(JUnit4.class)
public class EventLibDeviceAdminReceiverTest {

    private static final TestApis sTestApis = new TestApis();
    private static final Context sContext = sTestApis.context().instrumentedContext();
    private static final ComponentName DEVICE_ADMIN_COMPONENT =
            new ComponentName(
                    sContext.getPackageName(), EventLibDeviceAdminReceiver.class.getName());
    private static final UserReference sUser = sTestApis.users().instrumented();
    private static final DevicePolicyManager sDevicePolicyManager =
            sContext.getSystemService(DevicePolicyManager.class);
    private static final Intent sIntent = new Intent();

    @Before
    public void setUp() {
        EventLogs.resetLogs();
    }

    @Test
    public void enableDeviceOwner_logsEnabledEvent() {
        DeviceOwner deviceOwner =
                sTestApis.devicePolicy().setDeviceOwner(sUser, DEVICE_ADMIN_COMPONENT);

        try {
            EventLogs<DeviceAdminEnabledEvent> eventLogs =
                    DeviceAdminEnabledEvent.queryPackage(sContext.getPackageName());

            assertThat(eventLogs.poll()).isNotNull();
        } finally {
            deviceOwner.remove();
        }
    }

    @Test
    public void enableProfileOwner_logsEnabledEvent() {
        ProfileOwner profileOwner =
                sTestApis.devicePolicy().setProfileOwner(sUser, DEVICE_ADMIN_COMPONENT);

        try {
            EventLogs<DeviceAdminEnabledEvent> eventLogs =
                    DeviceAdminEnabledEvent.queryPackage(sContext.getPackageName());

            assertThat(eventLogs.poll()).isNotNull();
        } finally {
            profileOwner.remove();
        }
    }

    @Test
    public void disableProfileOwner_logsDisableRequestedEvent() {
        EventLibDeviceAdminReceiver receiver = new EventLibDeviceAdminReceiver();

        receiver.onDisableRequested(sContext, sIntent);

        EventLogs<DeviceAdminDisableRequestedEvent> eventLogs =
                DeviceAdminDisableRequestedEvent.queryPackage(sContext.getPackageName());
        assertThat(eventLogs.poll()).isNotNull();
    }

    @Test
    public void disableProfileOwner_logsDisabledEvent() {
        EventLibDeviceAdminReceiver receiver = new EventLibDeviceAdminReceiver();

        receiver.onDisabled(sContext, sIntent);

        EventLogs<DeviceAdminDisabledEvent> eventLogs =
                DeviceAdminDisabledEvent.queryPackage(sContext.getPackageName());
        assertThat(eventLogs.poll()).isNotNull();
    }

    @Test
    public void changePassword_logsPasswordChangedEvent() {
        EventLibDeviceAdminReceiver receiver = new EventLibDeviceAdminReceiver();

        receiver.onPasswordChanged(sContext, sIntent);

        EventLogs<DeviceAdminPasswordChangedEvent> eventLogs =
                DeviceAdminPasswordChangedEvent.queryPackage(sContext.getPackageName());
        assertThat(eventLogs.poll()).isNotNull();
    }

    @Test
    public void changePasswordWithUserHandle_logsPasswordChangedEvent() {
        EventLibDeviceAdminReceiver receiver = new EventLibDeviceAdminReceiver();

        receiver.onPasswordChanged(sContext, sIntent, sUser.userHandle());

        EventLogs<DeviceAdminPasswordChangedEvent> eventLogs =
                DeviceAdminPasswordChangedEvent.queryPackage(sContext.getPackageName());
        assertThat(eventLogs.poll().userHandle()).isEqualTo(sUser.userHandle());
    }

    @Test
    public void failPassword_logsPasswordFailedEvent() {
        EventLibDeviceAdminReceiver receiver = new EventLibDeviceAdminReceiver();

        receiver.onPasswordFailed(sContext, sIntent);

        EventLogs<DeviceAdminPasswordFailedEvent> eventLogs =
                DeviceAdminPasswordFailedEvent.queryPackage(sContext.getPackageName());
        assertThat(eventLogs.poll()).isNotNull();
    }

    @Test
    public void failPasswordWithUserHandle_logsPasswordFailedEvent() {
        EventLibDeviceAdminReceiver receiver = new EventLibDeviceAdminReceiver();

        receiver.onPasswordFailed(sContext, sIntent, sUser.userHandle());

        EventLogs<DeviceAdminPasswordFailedEvent> eventLogs =
                DeviceAdminPasswordFailedEvent.queryPackage(sContext.getPackageName());
        assertThat(eventLogs.poll().userHandle()).isEqualTo(sUser.userHandle());
    }

    @Test
    public void succeedPassword_logsPasswordSucceededEvent() {
        EventLibDeviceAdminReceiver receiver = new EventLibDeviceAdminReceiver();

        receiver.onPasswordSucceeded(sContext, sIntent);

        EventLogs<DeviceAdminPasswordSucceededEvent> eventLogs =
                DeviceAdminPasswordSucceededEvent.queryPackage(sContext.getPackageName());
        assertThat(eventLogs.poll()).isNotNull();
    }

    @Test
    public void succeedPasswordWithUserHandle_logsPasswordSucceededEvent() {
        EventLibDeviceAdminReceiver receiver = new EventLibDeviceAdminReceiver();

        receiver.onPasswordSucceeded(sContext, sIntent, sUser.userHandle());

        EventLogs<DeviceAdminPasswordSucceededEvent> eventLogs =
                DeviceAdminPasswordSucceededEvent.queryPackage(sContext.getPackageName());
        assertThat(eventLogs.poll().userHandle()).isEqualTo(sUser.userHandle());
    }

    @Test
    public void systemUpdatePending_logsSystemUpdatePendingEvent() {
        EventLibDeviceAdminReceiver receiver = new EventLibDeviceAdminReceiver();
        long receivedTime = System.currentTimeMillis();

        receiver.onSystemUpdatePending(sContext, sIntent, receivedTime);

        EventLogs<DeviceAdminSystemUpdatePendingEvent> eventLogs =
                DeviceAdminSystemUpdatePendingEvent.queryPackage(sContext.getPackageName());
        assertThat(eventLogs.poll().receivedTime()).isEqualTo(receivedTime);
    }
}
