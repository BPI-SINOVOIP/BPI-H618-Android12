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

import static org.junit.Assume.assumeFalse;
import static org.junit.Assume.assumeTrue;
import static org.testng.Assert.assertThrows;

import android.content.ComponentName;
import android.os.Build;

import com.android.bedstead.harrier.BedsteadJUnit4;
import com.android.bedstead.harrier.DeviceState;
import com.android.bedstead.harrier.annotations.AfterClass;
import com.android.bedstead.harrier.annotations.BeforeClass;
import com.android.bedstead.harrier.annotations.EnsureHasNoSecondaryUser;
import com.android.bedstead.harrier.annotations.EnsureHasNoWorkProfile;
import com.android.bedstead.harrier.annotations.EnsureHasSecondaryUser;
import com.android.bedstead.harrier.annotations.enterprise.EnsureHasDeviceOwner;
import com.android.bedstead.harrier.annotations.enterprise.EnsureHasNoDeviceOwner;
import com.android.bedstead.harrier.annotations.enterprise.EnsureHasNoProfileOwner;
import com.android.bedstead.nene.TestApis;
import com.android.bedstead.nene.exceptions.NeneException;
import com.android.bedstead.nene.users.UserReference;
import com.android.bedstead.nene.users.UserType;
import com.android.bedstead.nene.utils.Versions;
import com.android.bedstead.testapp.TestApp;
import com.android.bedstead.testapp.TestAppProvider;
import com.android.eventlib.premade.EventLibDeviceAdminReceiver;

import org.junit.ClassRule;
import org.junit.Ignore;
import org.junit.Rule;
import org.junit.Test;
import org.junit.runner.RunWith;

@RunWith(BedsteadJUnit4.class)
public class DevicePolicyTest {

    @ClassRule @Rule
    public static final DeviceState sDeviceState = new DeviceState();

    //  TODO(180478924): We shouldn't need to hardcode this
    private static final String DEVICE_ADMIN_TESTAPP_PACKAGE_NAME = "android.DeviceAdminTestApp";
    private static final ComponentName DPC_COMPONENT_NAME =
            new ComponentName(DEVICE_ADMIN_TESTAPP_PACKAGE_NAME,
                    EventLibDeviceAdminReceiver.class.getName());
    private static final ComponentName NOT_DPC_COMPONENT_NAME =
            new ComponentName(DEVICE_ADMIN_TESTAPP_PACKAGE_NAME,
                    "incorrect.class.name");

    private static final TestApis sTestApis = new TestApis();
    private static final UserReference sUser = sTestApis.users().instrumented();
    private static final UserReference NON_EXISTENT_USER = sTestApis.users().find(99999);

    private static TestApp sTestApp;

    @BeforeClass
    public static void setupClass() {
        sTestApp = new TestAppProvider().query()
                .wherePackageName().isEqualTo(DEVICE_ADMIN_TESTAPP_PACKAGE_NAME)
                .get();

        sTestApp.install(sUser);
    }

    @AfterClass
    public static void teardownClass() {
        sTestApp.uninstall(sUser);
    }

    @Test
    @EnsureHasNoDeviceOwner
    @EnsureHasNoWorkProfile
    public void setProfileOwner_profileOwnerIsSet() {
        UserReference profile = sTestApis.users().createUser()
                .parent(sUser)
                .type(sTestApis.users().supportedType(UserType.MANAGED_PROFILE_TYPE_NAME))
                .createAndStart();
        sTestApp.install(profile);

        ProfileOwner profileOwner =
                sTestApis.devicePolicy().setProfileOwner(profile, DPC_COMPONENT_NAME);

        try {
            assertThat(sTestApis.devicePolicy().getProfileOwner(profile)).isEqualTo(profileOwner);
        } finally {
            profile.remove();
        }
    }

    @Test
    @EnsureHasNoDeviceOwner
    @EnsureHasNoWorkProfile
    public void setProfileOwner_profileOwnerIsAlreadySet_throwsException() {
        UserReference profile = sTestApis.users().createUser()
                .parent(sUser)
                .type(sTestApis.users().supportedType(UserType.MANAGED_PROFILE_TYPE_NAME))
                .createAndStart();
        try {
            sTestApp.install(profile);

            sTestApis.devicePolicy().setProfileOwner(profile, DPC_COMPONENT_NAME);

            assertThrows(NeneException.class,
                    () -> sTestApis.devicePolicy().setProfileOwner(profile, DPC_COMPONENT_NAME));
        } finally {
            profile.remove();
        }
    }

    @Test
    @EnsureHasNoDeviceOwner
    @EnsureHasNoWorkProfile
    public void setProfileOwner_componentNameNotInstalled_throwsException() {
        UserReference profile = sTestApis.users().createUser()
                .parent(sUser)
                .type(sTestApis.users().supportedType(UserType.MANAGED_PROFILE_TYPE_NAME))
                .createAndStart();
        try {
            assertThrows(NeneException.class,
                    () -> sTestApis.devicePolicy().setProfileOwner(profile, DPC_COMPONENT_NAME));
        } finally {
            profile.remove();
        }
    }

    @Test
    @EnsureHasNoDeviceOwner
    @EnsureHasNoProfileOwner
    public void setProfileOwner_componentNameIsNotDPC_throwsException() {
        assertThrows(NeneException.class,
                () -> sTestApis.devicePolicy().setProfileOwner(sUser, NOT_DPC_COMPONENT_NAME));
    }

    @Test
    @EnsureHasNoDeviceOwner
    @EnsureHasNoProfileOwner
    public void setProfileOwner_nullUser_throwsException() {
        assertThrows(NullPointerException.class,
                () -> sTestApis.devicePolicy().setProfileOwner(
                        /* user= */ null, DPC_COMPONENT_NAME));
    }

    @Test
    @EnsureHasNoDeviceOwner
    @EnsureHasNoProfileOwner
    public void setProfileOwner_nullComponentName_throwsException() {
        assertThrows(NullPointerException.class,
                () -> sTestApis.devicePolicy().setProfileOwner(
                        sUser, /* profileOwnerComponent= */ null));
    }

    @Test
    @EnsureHasNoDeviceOwner
    @EnsureHasNoProfileOwner
    public void setProfileOwner_userDoesNotExist_throwsException() {
        assertThrows(NeneException.class,
                () -> sTestApis.devicePolicy().setProfileOwner(
                        NON_EXISTENT_USER, DPC_COMPONENT_NAME));
    }

    @Test
    @EnsureHasNoDeviceOwner
    @EnsureHasNoWorkProfile
    public void getProfileOwner_returnsProfileOwner() {
        UserReference profile = sTestApis.users().createUser()
                .parent(sUser)
                .type(sTestApis.users().supportedType(UserType.MANAGED_PROFILE_TYPE_NAME))
                .createAndStart();
        try {
            sTestApp.install(profile);

            ProfileOwner profileOwner =
                    sTestApis.devicePolicy().setProfileOwner(profile, DPC_COMPONENT_NAME);

            assertThat(sTestApis.devicePolicy().getProfileOwner(profile)).isEqualTo(profileOwner);
        } finally {
            profile.remove();
        }
    }

    @Test
    @EnsureHasNoDeviceOwner
    @EnsureHasNoWorkProfile
    public void getProfileOwner_noProfileOwner_returnsNull() {
        UserReference profile = sTestApis.users().createUser()
                .parent(sUser)
                .type(sTestApis.users().supportedType(UserType.MANAGED_PROFILE_TYPE_NAME))
                .createAndStart();

        try {
            assertThat(sTestApis.devicePolicy().getProfileOwner(profile)).isNull();
        } finally {
            profile.remove();
        }

    }

    @Test
    public void getProfileOwner_nullUser_throwsException() {
        assertThrows(NullPointerException.class,
                () -> sTestApis.devicePolicy().getProfileOwner(null));
    }

    @Test
    @EnsureHasNoDeviceOwner
    @EnsureHasNoProfileOwner
    public void setDeviceOwner_deviceOwnerIsSet() {
        DeviceOwner deviceOwner =
                sTestApis.devicePolicy().setDeviceOwner(sUser, DPC_COMPONENT_NAME);

        try {
            assertThat(sTestApis.devicePolicy().getDeviceOwner()).isEqualTo(deviceOwner);
        } finally {
            deviceOwner.remove();
        }
    }

    @Test
    @EnsureHasDeviceOwner
    public void setDeviceOwner_deviceOwnerIsAlreadySet_throwsException() {
        assertThrows(NeneException.class,
                () -> sTestApis.devicePolicy().setDeviceOwner(sUser, DPC_COMPONENT_NAME));
    }

    @Test
    @EnsureHasNoDeviceOwner
    @EnsureHasNoProfileOwner
    public void setDeviceOwner_componentNameNotInstalled_throwsException() {
        sTestApp.uninstall(sUser);
        try {
            assertThrows(NeneException.class,
                    () -> sTestApis.devicePolicy().setDeviceOwner(sUser, DPC_COMPONENT_NAME));
        } finally {
            sTestApp.install(sUser);
        }
    }

    @Test
    @EnsureHasNoDeviceOwner
    @EnsureHasNoProfileOwner
    public void setDeviceOwner_componentNameIsNotDPC_throwsException() {
        assertThrows(NeneException.class,
                () -> sTestApis.devicePolicy().setDeviceOwner(sUser, NOT_DPC_COMPONENT_NAME));
    }

    @Test
    @EnsureHasNoDeviceOwner
    @EnsureHasNoProfileOwner
    @EnsureHasSecondaryUser
    public void setDeviceOwner_preS_userAlreadyOnDevice_throwsException() {
        assumeFalse("After S, device owner can be set with users on the device",
                Versions.meetsMinimumSdkVersionRequirement(Build.VERSION_CODES.S));

        assertThrows(NeneException.class,
                () -> sTestApis.devicePolicy().setDeviceOwner(sUser, DPC_COMPONENT_NAME));
    }

    @Test
    @EnsureHasNoDeviceOwner
    @EnsureHasNoProfileOwner
    @EnsureHasSecondaryUser
    public void setDeviceOwner_sPlus_userAlreadyOnDevice_deviceOwnerIsSet() {
        assumeTrue("After S, device owner can be set with users on the device",
                Versions.meetsMinimumSdkVersionRequirement(Build.VERSION_CODES.S));

        DeviceOwner deviceOwner =
                sTestApis.devicePolicy().setDeviceOwner(sUser, DPC_COMPONENT_NAME);

        try {
            assertThat(sTestApis.devicePolicy().getDeviceOwner()).isNotNull();
        } finally {
            deviceOwner.remove();
        }
    }

    @Test
    @Ignore("TODO: Update once account support is added to Nene")
    public void setDeviceOwner_accountAlreadyOnDevice_throwsException() {
    }

    @Test
    @EnsureHasNoDeviceOwner
    @EnsureHasNoProfileOwner
    public void setDeviceOwner_nullUser_throwsException() {
        assertThrows(NullPointerException.class,
                () -> sTestApis.devicePolicy().setDeviceOwner(
                        /* user= */ null, DPC_COMPONENT_NAME));
    }

    @Test
    @EnsureHasNoDeviceOwner
    @EnsureHasNoProfileOwner
    public void setDeviceOwner_nullComponentName_throwsException() {
        assertThrows(NullPointerException.class,
                () -> sTestApis.devicePolicy().setDeviceOwner(
                        sUser, /* deviceOwnerComponent= */ null));
    }

    @Test
    @EnsureHasNoDeviceOwner
    @EnsureHasNoProfileOwner
    public void setDeviceOwner_userDoesNotExist_throwsException() {
        assertThrows(NeneException.class,
                () -> sTestApis.devicePolicy().setDeviceOwner(
                        NON_EXISTENT_USER, DPC_COMPONENT_NAME));
    }

    @Test
    @EnsureHasNoDeviceOwner
    @EnsureHasNoProfileOwner
    // TODO(scottjonathan): This could be made more generic by requiring no additional users
    @EnsureHasNoSecondaryUser
    @EnsureHasNoWorkProfile
    public void getDeviceOwner_returnsDeviceOwner() {
        DeviceOwner deviceOwner =
                sTestApis.devicePolicy().setDeviceOwner(sUser, DPC_COMPONENT_NAME);

        try {
            assertThat(sTestApis.devicePolicy().getDeviceOwner()).isEqualTo(deviceOwner);
        } finally {
            deviceOwner.remove();
        }
    }

    @Test
    @EnsureHasNoDeviceOwner
    public void getDeviceOwner_noDeviceOwner_returnsNull() {
        assertThat(sTestApis.devicePolicy().getDeviceOwner()).isNull();
    }

    @Test
    @EnsureHasNoDeviceOwner
    @EnsureHasNoProfileOwner
    public void profileOwner_autoclose_removesProfileOwner() {
        try (ProfileOwner profileOwner =
                     sTestApis.devicePolicy().setProfileOwner(sUser, DPC_COMPONENT_NAME)) {
            // We intentionally don't do anything here, just rely on the auto-close behaviour
        }

        assertThat(sTestApis.devicePolicy().getProfileOwner(sUser)).isNull();
    }

    @Test
    @EnsureHasNoDeviceOwner
    @EnsureHasNoProfileOwner
    public void deviceOwner_autoclose_removesDeviceOwner() {
        try (DeviceOwner deviceOwner =
                     sTestApis.devicePolicy().setDeviceOwner(sUser, DPC_COMPONENT_NAME)) {
            // We intentionally don't do anything here, just rely on the auto-close behaviour
        }

        assertThat(sTestApis.devicePolicy().getDeviceOwner()).isNull();
    }

    @Test
    @EnsureHasNoDeviceOwner
    @EnsureHasNoProfileOwner
    public void setDeviceOwner_recentlyUnsetProfileOwner_sets() {
        sTestApis.devicePolicy().setProfileOwner(sUser, DPC_COMPONENT_NAME).remove();

        sTestApis.devicePolicy().setDeviceOwner(sUser, DPC_COMPONENT_NAME);

        assertThat(sTestApis.devicePolicy().getDeviceOwner()).isNotNull();
    }

    @Test
    @EnsureHasNoDeviceOwner
    @EnsureHasNoProfileOwner
    public void setDeviceOwner_recentlyUnsetDeviceOwner_sets() {
        sTestApis.devicePolicy().setDeviceOwner(sUser, DPC_COMPONENT_NAME).remove();

        sTestApis.devicePolicy().setDeviceOwner(sUser, DPC_COMPONENT_NAME);

        assertThat(sTestApis.devicePolicy().getDeviceOwner()).isNotNull();
    }

    @Test
    @EnsureHasNoDeviceOwner
    @EnsureHasNoProfileOwner
    public void setProfileOwner_recentlyUnsetProfileOwner_sets() {
        sTestApis.devicePolicy().setProfileOwner(sUser, DPC_COMPONENT_NAME).remove();

        sTestApis.devicePolicy().setProfileOwner(sUser, DPC_COMPONENT_NAME);

        assertThat(sTestApis.devicePolicy().getProfileOwner(sUser)).isNotNull();
    }

    @Test
    @EnsureHasNoDeviceOwner
    @EnsureHasNoProfileOwner
    public void setProfileOwner_recentlyUnsetDeviceOwner_sets() {
        sTestApis.devicePolicy().setDeviceOwner(sUser, DPC_COMPONENT_NAME).remove();

        sTestApis.devicePolicy().setProfileOwner(sUser, DPC_COMPONENT_NAME);

        assertThat(sTestApis.devicePolicy().getProfileOwner(sUser)).isNotNull();
    }
}
