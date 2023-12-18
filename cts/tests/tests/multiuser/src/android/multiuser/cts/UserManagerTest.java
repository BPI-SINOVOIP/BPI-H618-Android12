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
 * limitations under the License
 */

package android.multiuser.cts;

import static android.multiuser.cts.TestingUtils.getBooleanProperty;

import static com.google.common.truth.Truth.assertThat;
import static com.google.common.truth.Truth.assertWithMessage;

import android.Manifest;
import android.app.Instrumentation;
import android.content.Context;
import android.content.pm.UserInfo;
import android.os.UserHandle;
import android.os.UserManager;
import android.platform.test.annotations.SystemUserOnly;

import androidx.test.platform.app.InstrumentationRegistry;

import org.junit.Before;
import org.junit.Test;
import org.junit.runner.RunWith;
import org.junit.runners.JUnit4;

import java.util.HashSet;
import java.util.List;
import java.util.Set;
import java.util.stream.Collectors;

@RunWith(JUnit4.class)
public final class UserManagerTest {

    private final Instrumentation mInstrumentation = InstrumentationRegistry.getInstrumentation();
    private final Context mContext = mInstrumentation.getContext();

    private UserManager mUserManager;

    @Before
    public void setTestFixtures() {
        mUserManager = mContext.getSystemService(UserManager.class);

        assertWithMessage("UserManager service").that(mUserManager).isNotNull();
    }

    /**
     * Verify that the isUserAGoat() method always returns false for API level 30. This is
     * because apps targeting R no longer have access to package queries by default.
     */
    @Test
    public void testUserGoat_api30() {
        assertWithMessage("isUserAGoat()").that(mUserManager.isUserAGoat()).isFalse();
    }

    @Test
    public void testIsHeadlessSystemUserMode() throws Exception {
        boolean expected = getBooleanProperty(mInstrumentation,
                "ro.fw.mu.headless_system_user");
        assertWithMessage("isHeadlessSystemUserMode()")
                .that(UserManager.isHeadlessSystemUserMode()).isEqualTo(expected);
    }

    @Test
    public void testIsUserForeground_currentUser() throws Exception {
        assertWithMessage("isUserForeground() for current user")
                .that(mUserManager.isUserForeground()).isTrue();
    }
    // TODO(b/173541467): add testIsUserForeground_backgroundUser()
    // TODO(b/179163496): add testIsUserForeground_ tests for profile users

    @Test
    @SystemUserOnly(reason = "Profiles are only supported on system user.")
    public void testCloneUser() throws Exception {
        // Need CREATE_USERS permission to create user in test
        mInstrumentation.getUiAutomation().adoptShellPermissionIdentity(
                Manifest.permission.CREATE_USERS, Manifest.permission.INTERACT_ACROSS_USERS);
        Set<String> disallowedPackages = new HashSet<String>();
        UserHandle userHandle = mUserManager.createProfile(
                "Clone user", UserManager.USER_TYPE_PROFILE_CLONE, disallowedPackages);
        assertThat(userHandle).isNotNull();

        try {
            final Context userContext = mContext.createPackageContextAsUser("system", 0,
                    userHandle);
            final UserManager cloneUserManager = userContext.getSystemService(UserManager.class);
            assertThat(cloneUserManager.isMediaSharedWithParent()).isTrue();
            assertThat(cloneUserManager.isCloneProfile()).isTrue();

            List<UserInfo> list = mUserManager.getUsers(true, true, true);
            List<UserInfo> cloneUsers = list.stream().filter(
                    user -> (user.id == userHandle.getIdentifier()
                            && user.isCloneProfile()))
                    .collect(Collectors.toList());
            assertThat(cloneUsers.size()).isEqualTo(1);
        } finally {
            assertThat(mUserManager.removeUser(userHandle)).isTrue();
            mInstrumentation.getUiAutomation().dropShellPermissionIdentity();
        }
    }
}
