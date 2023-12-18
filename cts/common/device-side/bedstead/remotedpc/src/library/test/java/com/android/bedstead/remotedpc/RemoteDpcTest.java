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

package com.android.bedstead.remotedpc;

import static com.google.common.truth.Truth.assertThat;

import static org.testng.Assert.assertThrows;

import android.content.ComponentName;
import android.os.UserHandle;

import com.android.bedstead.harrier.BedsteadJUnit4;
import com.android.bedstead.harrier.DeviceState;
import com.android.bedstead.harrier.annotations.AfterClass;
import com.android.bedstead.harrier.annotations.BeforeClass;
import com.android.bedstead.harrier.annotations.enterprise.EnsureHasNoDeviceOwner;
import com.android.bedstead.nene.TestApis;
import com.android.bedstead.nene.devicepolicy.DeviceOwner;
import com.android.bedstead.nene.devicepolicy.ProfileOwner;
import com.android.bedstead.nene.exceptions.NeneException;
import com.android.bedstead.nene.users.UserReference;
import com.android.bedstead.nene.users.UserType;
import com.android.bedstead.testapp.TestApp;
import com.android.bedstead.testapp.TestAppProvider;
import com.android.eventlib.premade.EventLibDeviceAdminReceiver;

import org.junit.ClassRule;
import org.junit.Rule;
import org.junit.Test;
import org.junit.runner.RunWith;

@RunWith(BedsteadJUnit4.class)
public class RemoteDpcTest {
    // TODO(scottjonathan): Add annotations to ensure that there is no DO/PO on appropriate methods
    //  TODO(180478924): We shouldn't need to hardcode this
    private static final String DEVICE_ADMIN_TESTAPP_PACKAGE_NAME = "android.DeviceAdminTestApp";
    private static final ComponentName NON_REMOTE_DPC_COMPONENT =
            new ComponentName(DEVICE_ADMIN_TESTAPP_PACKAGE_NAME,
                    EventLibDeviceAdminReceiver.class.getName());

    @ClassRule @Rule
    public static DeviceState sDeviceState = new DeviceState();

    private static final TestApis sTestApis = new TestApis();
    private static TestApp sNonRemoteDpcTestApp;
    private static final UserReference sUser = sTestApis.users().instrumented();
    private static final UserReference NON_EXISTING_USER_REFERENCE =
            sTestApis.users().find(99999);
    private static final UserHandle NON_EXISTING_USER_HANDLE =
            NON_EXISTING_USER_REFERENCE.userHandle();

    @BeforeClass
    public static void setupClass() {
        sNonRemoteDpcTestApp = new TestAppProvider().query()
                // TODO(scottjonathan): Query by feature not package name
                .wherePackageName().isEqualTo(DEVICE_ADMIN_TESTAPP_PACKAGE_NAME)
                .get();

        sNonRemoteDpcTestApp.install(sUser);
    }

    @AfterClass
    public static void teardownClass() {
        sNonRemoteDpcTestApp.uninstall(sUser);
    }

    @Test
    @EnsureHasNoDeviceOwner
    public void deviceOwner_noDeviceOwner_returnsNull() {
        assertThat(RemoteDpc.deviceOwner()).isNull();
    }

    @Test
    public void deviceOwner_nonRemoteDpcDeviceOwner_returnsNull() {
        DeviceOwner deviceOwner =
                sTestApis.devicePolicy().setDeviceOwner(sUser, NON_REMOTE_DPC_COMPONENT);
        try {
            assertThat(RemoteDpc.deviceOwner()).isNull();
        } finally {
            deviceOwner.remove();
        }
    }

    @Test
    public void deviceOwner_remoteDpcDeviceOwner_returnsInstance() {
        RemoteDpc remoteDPC = RemoteDpc.setAsDeviceOwner(sUser);

        try {
            assertThat(RemoteDpc.deviceOwner()).isNotNull();
        } finally {
            remoteDPC.devicePolicyController().remove();
        }
    }

    @Test
    public void profileOwner_noProfileOwner_returnsNull() {
        assertThat(RemoteDpc.profileOwner()).isNull();
    }

    @Test
    public void profileOwner_nonRemoteDpcProfileOwner_returnsNull() {
        ProfileOwner profileOwner =
                sTestApis.devicePolicy().setProfileOwner(sUser, NON_REMOTE_DPC_COMPONENT);
        try {
            assertThat(RemoteDpc.profileOwner()).isNull();
        } finally {
            profileOwner.remove();
        }
    }

    @Test
    public void profileOwner_remoteDpcProfileOwner_returnsInstance() {
        RemoteDpc remoteDPC = RemoteDpc.setAsProfileOwner(sUser);
        try {
            assertThat(RemoteDpc.profileOwner()).isNotNull();
        } finally {
            remoteDPC.devicePolicyController().remove();
        }
    }

    @Test
    public void profileOwner_userHandle_null_throwsException() {
        assertThrows(NullPointerException.class, () -> RemoteDpc.profileOwner((UserHandle) null));
    }

    @Test
    @EnsureHasNoDeviceOwner
    public void profileOwner_userHandle_noProfileOwner_returnsNull() {
        UserReference profile = sTestApis.users().createUser()
                .parent(sUser)
                .type(sTestApis.users().supportedType(UserType.MANAGED_PROFILE_TYPE_NAME))
                .createAndStart();
        try {
            assertThat(RemoteDpc.profileOwner(profile.userHandle())).isNull();
        } finally {
            profile.remove();
        }
    }

    @Test
    @EnsureHasNoDeviceOwner
    public void profileOwner_userHandle_nonRemoteDpcProfileOwner_returnsNull() {
        UserReference profile = sTestApis.users().createUser()
                .parent(sUser)
                .type(sTestApis.users().supportedType(UserType.MANAGED_PROFILE_TYPE_NAME))
                .createAndStart();
        sNonRemoteDpcTestApp.install(profile);
        try {
            sTestApis.devicePolicy().setProfileOwner(profile, NON_REMOTE_DPC_COMPONENT);

            assertThat(RemoteDpc.profileOwner(profile.userHandle())).isNull();
        } finally {
            profile.remove();
        }
    }

    @Test
    @EnsureHasNoDeviceOwner
    public void profileOwner_userHandle_remoteDpcProfileOwner_returnsInstance() {
        UserReference profile = sTestApis.users().createUser()
                .parent(sUser)
                .type(sTestApis.users().supportedType(UserType.MANAGED_PROFILE_TYPE_NAME))
                .createAndStart();
        RemoteDpc.setAsProfileOwner(profile);
        try {
            assertThat(RemoteDpc.profileOwner(profile.userHandle())).isNotNull();
        } finally {
            profile.remove();
        }
    }

    @Test
    public void profileOwner_userReference_null_throwsException() {
        assertThrows(NullPointerException.class,
                () -> RemoteDpc.profileOwner((UserReference) null));
    }

    @Test
    @EnsureHasNoDeviceOwner
    public void profileOwner_userReference_noProfileOwner_returnsNull() {
        UserReference profile = sTestApis.users().createUser()
                .parent(sUser)
                .type(sTestApis.users().supportedType(UserType.MANAGED_PROFILE_TYPE_NAME))
                .createAndStart();
        try {
            assertThat(RemoteDpc.profileOwner(profile)).isNull();
        } finally {
            profile.remove();
        }
    }

    @Test
    @EnsureHasNoDeviceOwner
    public void profileOwner_userReference_nonRemoteDpcProfileOwner_returnsNull() {
        UserReference profile = sTestApis.users().createUser()
                .parent(sUser)
                .type(sTestApis.users().supportedType(UserType.MANAGED_PROFILE_TYPE_NAME))
                .createAndStart();
        sNonRemoteDpcTestApp.install(profile);
        try {
            sTestApis.devicePolicy().setProfileOwner(profile, NON_REMOTE_DPC_COMPONENT);

            assertThat(RemoteDpc.profileOwner(profile)).isNull();
        } finally {
            profile.remove();
        }
    }

    @Test
    @EnsureHasNoDeviceOwner
    public void profileOwner_userReference_remoteDpcProfileOwner_returnsInstance() {
        UserReference profile = sTestApis.users().createUser()
                .parent(sUser)
                .type(sTestApis.users().supportedType(UserType.MANAGED_PROFILE_TYPE_NAME))
                .createAndStart();
        RemoteDpc.setAsProfileOwner(profile);
        try {
            assertThat(RemoteDpc.profileOwner(profile)).isNotNull();
        } finally {
            profile.remove();
        }
    }

    @Test
    public void any_noDeviceOwner_noProfileOwner_returnsNull() {
        assertThat(RemoteDpc.any()).isNull();
    }

    @Test
    public void any_noDeviceOwner_nonRemoteDpcProfileOwner_returnsNull() {
        ProfileOwner profileOwner = sTestApis.devicePolicy().setProfileOwner(sUser,
                NON_REMOTE_DPC_COMPONENT);

        try {
            assertThat(RemoteDpc.any()).isNull();
        } finally {
            profileOwner.remove();
        }
    }

    @Test
    public void any_nonRemoteDpcDeviceOwner_noProfileOwner_returnsNull() {
        DeviceOwner deviceOwner = sTestApis.devicePolicy().setDeviceOwner(sUser,
                NON_REMOTE_DPC_COMPONENT);

        try {
            assertThat(RemoteDpc.any()).isNull();
        } finally {
            deviceOwner.remove();
        }
    }

    @Test
    public void any_remoteDpcDeviceOwner_returnsDeviceOwner() {
        RemoteDpc remoteDPC = RemoteDpc.setAsDeviceOwner(sUser);

        try {
            assertThat(RemoteDpc.any()).isNotNull();
        } finally {
            remoteDPC.devicePolicyController().remove();
        }
    }

    @Test
    public void any_remoteDpcProfileOwner_returnsProfileOwner() {
        RemoteDpc remoteDPC = RemoteDpc.setAsProfileOwner(sUser);

        try {
            assertThat(RemoteDpc.any()).isNotNull();
        } finally {
            remoteDPC.devicePolicyController().remove();
        }
    }

    @Test
    public void any_userHandle_null_throwsException() {
        assertThrows(NullPointerException.class, () -> RemoteDpc.any((UserHandle) null));
    }

    @Test
    public void any_userHandle_noDeviceOwner_noProfileOwner_returnsNull() {
        assertThat(RemoteDpc.any(sUser.userHandle())).isNull();
    }

    @Test
    @EnsureHasNoDeviceOwner
    public void any_userHandle_noDeviceOwner_nonRemoteDpcProfileOwner_returnsNull() {
        UserReference profile = sTestApis.users().createUser()
                .parent(sUser)
                .type(sTestApis.users().supportedType(UserType.MANAGED_PROFILE_TYPE_NAME))
                .createAndStart();
        sNonRemoteDpcTestApp.install(profile);
        try {
            sTestApis.devicePolicy().setProfileOwner(profile, NON_REMOTE_DPC_COMPONENT);

            assertThat(RemoteDpc.any(profile.userHandle())).isNull();
        } finally {
            profile.remove();
        }
    }

    @Test
    public void any_userHandle_nonRemoteDpcDeviceOwner_noProfileOwner_returnsNull() {
        DeviceOwner deviceOwner = sTestApis.devicePolicy().setDeviceOwner(sUser,
                NON_REMOTE_DPC_COMPONENT);

        try {
            assertThat(RemoteDpc.any(sUser.userHandle())).isNull();
        } finally {
            deviceOwner.remove();
        }
    }

    @Test
    public void any_userHandle_remoteDpcDeviceOwner_returnsDeviceOwner() {
        RemoteDpc deviceOwner = RemoteDpc.setAsDeviceOwner(sUser);

        try {
            assertThat(RemoteDpc.any(sUser.userHandle())).isEqualTo(deviceOwner);
        } finally {
            deviceOwner.devicePolicyController().remove();
        }
    }

    @Test
    @EnsureHasNoDeviceOwner
    public void any_userHandle_remoteDpcProfileOwner_returnsProfileOwner() {
        UserReference profile = sTestApis.users().createUser()
                .parent(sUser)
                .type(sTestApis.users().supportedType(UserType.MANAGED_PROFILE_TYPE_NAME))
                .createAndStart();
        try {
            RemoteDpc profileOwner = RemoteDpc.setAsProfileOwner(profile);

            assertThat(RemoteDpc.any(profile.userHandle())).isEqualTo(profileOwner);
        } finally {
            profile.remove();
        }
    }

    @Test
    public void any_userReference_null_throwsException() {
        assertThrows(NullPointerException.class, () -> RemoteDpc.any((UserReference) null));
    }

    @Test
    public void any_userReference_noDeviceOwner_noProfileOwner_returnsNull() {
        assertThat(RemoteDpc.any(sUser)).isNull();
    }

    @Test
    @EnsureHasNoDeviceOwner
    public void any_userReference_noDeviceOwner_nonRemoteDpcProfileOwner_returnsNull() {
        UserReference profile = sTestApis.users().createUser()
                .parent(sUser)
                .type(sTestApis.users().supportedType(UserType.MANAGED_PROFILE_TYPE_NAME))
                .createAndStart();
        sNonRemoteDpcTestApp.install(profile);
        try {
            sTestApis.devicePolicy().setProfileOwner(profile, NON_REMOTE_DPC_COMPONENT);

            assertThat(RemoteDpc.any(profile)).isNull();
        } finally {
            profile.remove();
        }
    }

    @Test
    public void any_userReference_nonRemoteDpcDeviceOwner_noProfileOwner_returnsNull() {
        DeviceOwner deviceOwner = sTestApis.devicePolicy().setDeviceOwner(sUser,
                NON_REMOTE_DPC_COMPONENT);

        try {
            assertThat(RemoteDpc.any(sUser)).isNull();
        } finally {
            deviceOwner.remove();
        }
    }

    @Test
    public void any_userReference_remoteDpcDeviceOwner_returnsDeviceOwner() {
        RemoteDpc deviceOwner = RemoteDpc.setAsDeviceOwner(sUser);

        try {
            assertThat(RemoteDpc.any(sUser)).isEqualTo(deviceOwner);
        } finally {
            deviceOwner.devicePolicyController().remove();
        }
    }

    @Test
    @EnsureHasNoDeviceOwner
    public void any_userReference_remoteDpcProfileOwner_returnsProfileOwner() {
        UserReference profile = sTestApis.users().createUser()
                .parent(sUser)
                .type(sTestApis.users().supportedType(UserType.MANAGED_PROFILE_TYPE_NAME))
                .createAndStart();
        try {
            RemoteDpc profileOwner = RemoteDpc.setAsProfileOwner(profile);

            assertThat(RemoteDpc.any(profile)).isEqualTo(profileOwner);
        } finally {
            profile.remove();
        }
    }

    @Test
    public void setAsDeviceOwner_userHandle_null_throwsException() {
        assertThrows(NullPointerException.class,
                () -> RemoteDpc.setAsDeviceOwner((UserHandle) null));
    }

    @Test
    public void setAsDeviceOwner_userHandle_nonExistingUser_throwsException() {
        assertThrows(NeneException.class,
                () -> RemoteDpc.setAsDeviceOwner(NON_EXISTING_USER_HANDLE));
    }

    @Test
    public void setAsDeviceOwner_userHandle_alreadySet_doesNothing() {
        RemoteDpc.setAsDeviceOwner(sUser.userHandle());

        DeviceOwner deviceOwner = sTestApis.devicePolicy().getDeviceOwner();
        try {
            RemoteDpc.setAsDeviceOwner(sUser.userHandle());

            deviceOwner = sTestApis.devicePolicy().getDeviceOwner();
            assertThat(deviceOwner).isNotNull();
        } finally {
            if (deviceOwner != null) {
                deviceOwner.remove();
            }
        }
    }

    @Test
    public void setAsDeviceOwner_userHandle_alreadyHasDeviceOwner_replacesDeviceOwner() {
        sTestApis.devicePolicy().setDeviceOwner(sUser, NON_REMOTE_DPC_COMPONENT);

        try {
            RemoteDpc remoteDPC = RemoteDpc.setAsDeviceOwner(sUser.userHandle());

            DeviceOwner deviceOwner = sTestApis.devicePolicy().getDeviceOwner();
            assertThat(deviceOwner).isEqualTo(remoteDPC.devicePolicyController());
        } finally {
            sTestApis.devicePolicy().getDeviceOwner().remove();
        }
    }

    @Test
    public void setAsDeviceOwner_userHandle_doesNotHaveDeviceOwner_setsDeviceOwner() {
        RemoteDpc.setAsDeviceOwner(sUser.userHandle());

        DeviceOwner deviceOwner = sTestApis.devicePolicy().getDeviceOwner();
        try {
            assertThat(deviceOwner).isNotNull();
        } finally {
            if (deviceOwner != null) {
                deviceOwner.remove();
            }
        }
    }

    @Test
    public void setAsDeviceOwner_userReference_null_throwsException() {
        assertThrows(NullPointerException.class,
                () -> RemoteDpc.setAsDeviceOwner((UserReference) null));
    }

    @Test
    public void setAsDeviceOwner_userReference_nonExistingUser_throwsException() {
        assertThrows(NeneException.class,
                () -> RemoteDpc.setAsDeviceOwner(NON_EXISTING_USER_REFERENCE));
    }

    @Test
    public void setAsDeviceOwner_userReference_alreadySet_doesNothing() {
        RemoteDpc.setAsDeviceOwner(sUser);

        DeviceOwner deviceOwner = sTestApis.devicePolicy().getDeviceOwner();
        try {
            RemoteDpc.setAsDeviceOwner(sUser);

            deviceOwner = sTestApis.devicePolicy().getDeviceOwner();
            assertThat(deviceOwner).isNotNull();
        } finally {
            if (deviceOwner != null) {
                deviceOwner.remove();
            }
        }
    }

    @Test
    public void setAsDeviceOwner_userReference_alreadyHasDeviceOwner_replacesDeviceOwner() {
        sTestApis.devicePolicy().setDeviceOwner(sUser, NON_REMOTE_DPC_COMPONENT);

        try {
            RemoteDpc remoteDPC = RemoteDpc.setAsDeviceOwner(sUser);

            DeviceOwner deviceOwner = sTestApis.devicePolicy().getDeviceOwner();
            assertThat(deviceOwner).isEqualTo(remoteDPC.devicePolicyController());
        } finally {
            sTestApis.devicePolicy().getDeviceOwner().remove();
        }
    }

    @Test
    public void setAsDeviceOwner_userReference_doesNotHaveDeviceOwner_setsDeviceOwner() {
        RemoteDpc.setAsDeviceOwner(sUser);

        DeviceOwner deviceOwner = sTestApis.devicePolicy().getDeviceOwner();
        try {
            assertThat(deviceOwner).isNotNull();
        } finally {
            if (deviceOwner != null) {
                deviceOwner.remove();
            }
        }
    }

    @Test
    public void setAsProfileOwner_userHandle_null_throwsException() {
        assertThrows(NullPointerException.class,
                () -> RemoteDpc.setAsProfileOwner((UserHandle) null));
    }

    @Test
    public void setAsProfileOwner_userHandle_nonExistingUser_throwsException() {
        assertThrows(NeneException.class,
                () -> RemoteDpc.setAsProfileOwner(NON_EXISTING_USER_HANDLE));
    }

    @Test
    @EnsureHasNoDeviceOwner
    public void setAsProfileOwner_userHandle_alreadySet_doesNothing() {
        UserReference profile = sTestApis.users().createUser()
                .parent(sUser)
                .type(sTestApis.users().supportedType(UserType.MANAGED_PROFILE_TYPE_NAME))
                .createAndStart();
        try {
            RemoteDpc.setAsProfileOwner(profile.userHandle());

            RemoteDpc.setAsProfileOwner(profile.userHandle());

            assertThat(sTestApis.devicePolicy().getProfileOwner(profile)).isNotNull();
        } finally {
            profile.remove();
        }
    }

    @Test
    @EnsureHasNoDeviceOwner
    public void setAsProfileOwner_userHandle_alreadyHasProfileOwner_replacesProfileOwner() {
        UserReference profile = sTestApis.users().createUser()
                .parent(sUser)
                .type(sTestApis.users().supportedType(UserType.MANAGED_PROFILE_TYPE_NAME))
                .createAndStart();
        sNonRemoteDpcTestApp.install(profile);
        try {
            sTestApis.devicePolicy().setProfileOwner(profile, NON_REMOTE_DPC_COMPONENT);

            RemoteDpc remoteDPC = RemoteDpc.setAsProfileOwner(profile.userHandle());

            assertThat(sTestApis.devicePolicy().getProfileOwner(profile))
                    .isEqualTo(remoteDPC.devicePolicyController());
        } finally {
            profile.remove();
        }
    }

    @Test
    @EnsureHasNoDeviceOwner
    public void setAsProfileOwner_userHandle_doesNotHaveProfileOwner_setsProfileOwner() {
        UserReference profile = sTestApis.users().createUser()
                .parent(sUser)
                .type(sTestApis.users().supportedType(UserType.MANAGED_PROFILE_TYPE_NAME))
                .createAndStart();
        try {
            RemoteDpc.setAsProfileOwner(profile.userHandle());

            assertThat(sTestApis.devicePolicy().getProfileOwner(profile)).isNotNull();
        } finally {
            profile.remove();
        }
    }

    @Test
    public void setAsProfileOwner_userReference_null_throwsException() {
        assertThrows(NullPointerException.class,
                () -> RemoteDpc.setAsProfileOwner((UserReference) null));
    }

    @Test
    public void setAsProfileOwner_userReference_nonExistingUser_throwsException() {
        assertThrows(NeneException.class,
                () -> RemoteDpc.setAsProfileOwner(NON_EXISTING_USER_REFERENCE));
    }

    @Test
    @EnsureHasNoDeviceOwner
    public void setAsProfileOwner_userReference_alreadySet_doesNothing() {
        UserReference profile = sTestApis.users().createUser()
                .parent(sUser)
                .type(sTestApis.users().supportedType(UserType.MANAGED_PROFILE_TYPE_NAME))
                .createAndStart();
        try {
            RemoteDpc.setAsProfileOwner(profile);

            RemoteDpc.setAsProfileOwner(profile);

            assertThat(sTestApis.devicePolicy().getProfileOwner(profile)).isNotNull();
        } finally {
            profile.remove();
        }
    }

    @Test
    @EnsureHasNoDeviceOwner
    public void setAsProfileOwner_userReference_alreadyHasProfileOwner_replacesProfileOwner() {
        UserReference profile = sTestApis.users().createUser()
                .parent(sUser)
                .type(sTestApis.users().supportedType(UserType.MANAGED_PROFILE_TYPE_NAME))
                .createAndStart();
        sNonRemoteDpcTestApp.install(profile);
        try {
            sTestApis.devicePolicy().setProfileOwner(profile, NON_REMOTE_DPC_COMPONENT);

            RemoteDpc remoteDPC = RemoteDpc.setAsProfileOwner(profile);

            assertThat(sTestApis.devicePolicy().getProfileOwner(profile))
                    .isEqualTo(remoteDPC.devicePolicyController());
        } finally {
            profile.remove();
        }
    }

    @Test
    @EnsureHasNoDeviceOwner
    public void setAsProfileOwner_userReference_doesNotHaveProfileOwner_setsProfileOwner() {
        UserReference profile = sTestApis.users().createUser()
                .parent(sUser)
                .type(sTestApis.users().supportedType(UserType.MANAGED_PROFILE_TYPE_NAME))
                .createAndStart();
        try {
            RemoteDpc.setAsProfileOwner(profile);

            assertThat(sTestApis.devicePolicy().getProfileOwner(profile)).isNotNull();
        } finally {
            profile.remove();
        }
    }

    @Test
    public void devicePolicyController_returnsDevicePolicyController() {
        RemoteDpc remoteDPC = RemoteDpc.setAsDeviceOwner(sUser);

        try {
            assertThat(remoteDPC.devicePolicyController())
                    .isEqualTo(sTestApis.devicePolicy().getDeviceOwner());
        } finally {
            remoteDPC.remove();
        }
    }

    @Test
    public void remove_deviceOwner_removes() {
        RemoteDpc remoteDPC = RemoteDpc.setAsDeviceOwner(sUser);

        remoteDPC.remove();

        assertThat(sTestApis.devicePolicy().getDeviceOwner()).isNull();
    }

    @Test
    @EnsureHasNoDeviceOwner
    public void remove_profileOwner_removes() {
        try (UserReference profile = sTestApis.users().createUser()
                .parent(sUser)
                .type(sTestApis.users().supportedType(UserType.MANAGED_PROFILE_TYPE_NAME))
                .createAndStart()) {
            RemoteDpc remoteDPC = RemoteDpc.setAsProfileOwner(profile);

            remoteDPC.remove();

            assertThat(sTestApis.devicePolicy().getProfileOwner(profile)).isNull();
        }
    }

    // TODO(scottjonathan): Do we need to support the case where there is both a DO and a PO on
    //  older versions of Android?

    @Test
    public void frameworkCall_makesCall() {
        RemoteDpc remoteDPC = RemoteDpc.setAsDeviceOwner(sUser);

        try {
            // Checking that the call succeeds
            remoteDPC.devicePolicyManager().getCurrentFailedPasswordAttempts();
        } finally {
            remoteDPC.remove();
        }
    }

    @Test
    @EnsureHasNoDeviceOwner
    public void frameworkCall_onProfile_makesCall() {
        try (UserReference profile = sTestApis.users().createUser()
                .parent(sUser)
                .type(sTestApis.users().supportedType(UserType.MANAGED_PROFILE_TYPE_NAME))
                .createAndStart()) {
            RemoteDpc remoteDPC = RemoteDpc.setAsProfileOwner(profile);

            // Checking that the call succeeds
            remoteDPC.devicePolicyManager().isUsingUnifiedPassword();
        }
    }

    @Test
    public void frameworkCallRequiresProfileOwner_notProfileOwner_throwsSecurityException() {
        RemoteDpc remoteDPC = RemoteDpc.setAsDeviceOwner(sUser);

        try {
            assertThrows(SecurityException.class,
                    () -> remoteDPC.devicePolicyManager().isUsingUnifiedPassword());
        } finally {
            remoteDPC.remove();
        }
    }

    @Test
    public void forDevicePolicyController_nullDevicePolicyController_throwsException() {
        assertThrows(NullPointerException.class, () -> RemoteDpc.forDevicePolicyController(null));
    }

    @Test
    public void forDevicePolicyController_nonRemoteDpcDevicePolicyController_throwsException() {
        DeviceOwner deviceOwner = sTestApis.devicePolicy().setDeviceOwner(sUser,
                NON_REMOTE_DPC_COMPONENT);

        try {
            assertThrows(IllegalStateException.class,
                    () -> RemoteDpc.forDevicePolicyController(deviceOwner));
        } finally {
            deviceOwner.remove();
        }
    }

    @Test
    public void forDevicePolicyController_remoteDpcDevicePolicyController_returnsRemoteDpc() {
        RemoteDpc remoteDPC = RemoteDpc.setAsDeviceOwner(sUser);

        try {
            assertThat(RemoteDpc.forDevicePolicyController(remoteDPC.devicePolicyController()))
                    .isNotNull();
        } finally {
            remoteDPC.remove();
        }

    }
}