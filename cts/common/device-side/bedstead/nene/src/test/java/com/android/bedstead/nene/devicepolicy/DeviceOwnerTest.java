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

package com.android.bedstead.nene.devicepolicy;

import static com.google.common.truth.Truth.assertThat;

import android.content.ComponentName;

import com.android.bedstead.nene.TestApis;
import com.android.bedstead.nene.users.UserReference;
import com.android.bedstead.testapp.TestApp;
import com.android.bedstead.testapp.TestAppProvider;
import com.android.eventlib.premade.EventLibDeviceAdminReceiver;

import org.junit.AfterClass;
import org.junit.BeforeClass;
import org.junit.Test;
import org.junit.runner.RunWith;
import org.junit.runners.JUnit4;

@RunWith(JUnit4.class)
public class DeviceOwnerTest {

    //  TODO(180478924): We shouldn't need to hardcode this
    private static final String DEVICE_ADMIN_TESTAPP_PACKAGE_NAME = "android.DeviceAdminTestApp";
    private static final ComponentName DPC_COMPONENT_NAME =
            new ComponentName(DEVICE_ADMIN_TESTAPP_PACKAGE_NAME,
                    EventLibDeviceAdminReceiver.class.getName());

    private static final TestApis sTestApis = new TestApis();
    private static final UserReference sUser = sTestApis.users().instrumented();

    private static TestApp sTestApp;
    private static DeviceOwner sDeviceOwner;

    @BeforeClass
    public static void setupClass() {
        sTestApp = new TestAppProvider().query()
                .wherePackageName().isEqualTo(DEVICE_ADMIN_TESTAPP_PACKAGE_NAME)
                .get();

        sTestApp.install(sUser);

        sDeviceOwner = sTestApis.devicePolicy().setDeviceOwner(sUser, DPC_COMPONENT_NAME);
    }

    @AfterClass
    public static void teardownClass() {
        sDeviceOwner.remove();
        sTestApp.uninstall(sUser);
    }

    @Test
    public void user_returnsUser() {
        assertThat(sDeviceOwner.user()).isEqualTo(sUser);
    }

    @Test
    public void pkg_returnsPackage() {
        assertThat(sDeviceOwner.pkg()).isEqualTo(sTestApp.reference());
    }

    @Test
    public void componentName_returnsComponentName() {
        assertThat(sDeviceOwner.componentName()).isEqualTo(DPC_COMPONENT_NAME);
    }

    @Test
    public void remove_removesDeviceOwner() {
        sDeviceOwner.remove();
        try {
            assertThat(sTestApis.devicePolicy().getDeviceOwner()).isNull();
        } finally {
            sDeviceOwner = sTestApis.devicePolicy().setDeviceOwner(sUser, DPC_COMPONENT_NAME);
        }
    }
}
