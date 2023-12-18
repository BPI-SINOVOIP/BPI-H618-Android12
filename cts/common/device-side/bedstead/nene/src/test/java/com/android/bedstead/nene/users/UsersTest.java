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

package com.android.bedstead.nene.users;

import static android.os.Build.VERSION.SDK_INT;

import static com.android.bedstead.nene.users.UserType.MANAGED_PROFILE_TYPE_NAME;
import static com.android.bedstead.nene.users.UserType.SECONDARY_USER_TYPE_NAME;
import static com.android.bedstead.nene.users.UserType.SYSTEM_USER_TYPE_NAME;

import static com.google.common.truth.Truth.assertThat;

import static org.junit.Assume.assumeTrue;
import static org.testng.Assert.assertThrows;

import android.os.Build;
import android.os.UserHandle;

import com.android.bedstead.harrier.BedsteadJUnit4;
import com.android.bedstead.harrier.DeviceState;
import com.android.bedstead.harrier.annotations.EnsureHasNoSecondaryUser;
import com.android.bedstead.harrier.annotations.EnsureHasNoWorkProfile;
import com.android.bedstead.harrier.annotations.EnsureHasSecondaryUser;
import com.android.bedstead.harrier.annotations.EnsureHasWorkProfile;
import com.android.bedstead.harrier.annotations.enterprise.EnsureHasNoDeviceOwner;
import com.android.bedstead.nene.TestApis;
import com.android.bedstead.nene.exceptions.NeneException;

import org.junit.ClassRule;
import org.junit.Ignore;
import org.junit.Rule;
import org.junit.Test;
import org.junit.runner.RunWith;

@RunWith(BedsteadJUnit4.class)
public class UsersTest {

    private static final int MAX_SYSTEM_USERS = UserType.UNLIMITED;
    private static final int MAX_SYSTEM_USERS_PER_PARENT = UserType.UNLIMITED;
    private static final String INVALID_TYPE_NAME = "invalidTypeName";
    private static final int MAX_MANAGED_PROFILES = UserType.UNLIMITED;
    private static final int MAX_MANAGED_PROFILES_PER_PARENT = 1;
    private static final int NON_EXISTING_USER_ID = 10000;
    private static final int USER_ID = NON_EXISTING_USER_ID;
    private static final String USER_NAME = "userName";

    private final TestApis mTestApis = new TestApis();
    private final UserType mSecondaryUserType =
            mTestApis.users().supportedType(SECONDARY_USER_TYPE_NAME);
    private final UserType mManagedProfileType =
            mTestApis.users().supportedType(MANAGED_PROFILE_TYPE_NAME);
    private final UserReference mInstrumentedUser = mTestApis.users().instrumented();

    @ClassRule
    @Rule
    public static final DeviceState sDeviceState = new DeviceState();

    // We don't want to test the exact list of any specific device, so we check that it returns
    // some known types which will exist on the emulators (used for presubmit tests).

    @Test
    public void supportedTypes_containsManagedProfile() {
        UserType managedProfileUserType =
                mTestApis.users().supportedTypes().stream().filter(
                        (ut) -> ut.name().equals(MANAGED_PROFILE_TYPE_NAME)).findFirst().get();

        assertThat(managedProfileUserType.baseType()).containsExactly(UserType.BaseType.PROFILE);
        assertThat(managedProfileUserType.enabled()).isTrue();
        assertThat(managedProfileUserType.maxAllowed()).isEqualTo(MAX_MANAGED_PROFILES);
        assertThat(managedProfileUserType.maxAllowedPerParent())
                .isEqualTo(MAX_MANAGED_PROFILES_PER_PARENT);
    }

    @Test
    public void supportedTypes_containsSystemUser() {
        UserType systemUserType =
                mTestApis.users().supportedTypes().stream().filter(
                        (ut) -> ut.name().equals(SYSTEM_USER_TYPE_NAME)).findFirst().get();

        assertThat(systemUserType.baseType()).containsExactly(
                UserType.BaseType.SYSTEM, UserType.BaseType.FULL);
        assertThat(systemUserType.enabled()).isTrue();
        assertThat(systemUserType.maxAllowed()).isEqualTo(MAX_SYSTEM_USERS);
        assertThat(systemUserType.maxAllowedPerParent()).isEqualTo(MAX_SYSTEM_USERS_PER_PARENT);
    }

    @Test
    public void supportedType_validType_returnsType() {
        UserType managedProfileUserType =
                mTestApis.users().supportedType(MANAGED_PROFILE_TYPE_NAME);

        assertThat(managedProfileUserType.baseType()).containsExactly(UserType.BaseType.PROFILE);
        assertThat(managedProfileUserType.enabled()).isTrue();
        assertThat(managedProfileUserType.maxAllowed()).isEqualTo(MAX_MANAGED_PROFILES);
        assertThat(managedProfileUserType.maxAllowedPerParent())
                .isEqualTo(MAX_MANAGED_PROFILES_PER_PARENT);
    }

    @Test
    public void supportedType_invalidType_returnsNull() {
        assertThat(mTestApis.users().supportedType(INVALID_TYPE_NAME)).isNull();
    }

    @Test
    public void all_containsCreatedUser() {
        UserReference user = mTestApis.users().createUser().create();

        try {
            assertThat(mTestApis.users().all()).contains(user);
        } finally {
            user.remove();
        }
    }

    @Test
    public void all_userAddedSinceLastCallToUsers_containsNewUser() {
        UserReference user = mTestApis.users().createUser().create();
        mTestApis.users().all();
        UserReference user2 = mTestApis.users().createUser().create();

        try {
            assertThat(mTestApis.users().all()).contains(user2);
        } finally {
            user.remove();
            user2.remove();
        }
    }

    @Test
    public void all_userRemovedSinceLastCallToUsers_doesNotContainRemovedUser() {
        UserReference user = mTestApis.users().createUser().create();
        mTestApis.users().all();
        user.remove();

        assertThat(mTestApis.users().all()).doesNotContain(user);
    }

    @Test
    public void find_userExists_returnsUserReference() {
        UserReference user = mTestApis.users().createUser().create();
        try {
            assertThat(mTestApis.users().find(user.id())).isEqualTo(user);
        } finally {
            user.remove();
        }
    }

    @Test
    public void find_userDoesNotExist_returnsUserReference() {
        assertThat(mTestApis.users().find(NON_EXISTING_USER_ID)).isNotNull();
    }

    @Test
    public void find_fromUserHandle_referencesCorrectId() {
        assertThat(mTestApis.users().find(UserHandle.of(USER_ID)).id()).isEqualTo(USER_ID);
    }

    @Test
    public void find_constructedReferenceReferencesCorrectId() {
        assertThat(mTestApis.users().find(USER_ID).id()).isEqualTo(USER_ID);
    }

    @Test
    public void createUser_additionalSystemUser_throwsException()  {
        assertThrows(NeneException.class, () ->
                mTestApis.users().createUser()
                        .type(mTestApis.users().supportedType(SYSTEM_USER_TYPE_NAME))
                        .create());
    }

    @Test
    public void createUser_userIsCreated()  {
        UserReference user = mTestApis.users().createUser().create();

        try {
            assertThat(mTestApis.users().all()).contains(user);
        } finally {
            user.remove();
        }
    }

    @Test
    public void createUser_createdUserHasCorrectName() {
        UserReference userReference = mTestApis.users().createUser()
                .name(USER_NAME)
                .create();

        try {
            assertThat(userReference.resolve().name()).isEqualTo(USER_NAME);
        } finally {
            userReference.remove();
        }
    }

    @Test
    public void createUser_createdUserHasCorrectTypeName() {
        UserReference userReference = mTestApis.users().createUser()
                .type(mSecondaryUserType)
                .create();

        try {
            assertThat(userReference.resolve().type()).isEqualTo(mSecondaryUserType);
        } finally {
            userReference.remove();
        }
    }

    @Test
    public void createUser_specifiesNullUserType_throwsException() {
        UserBuilder userBuilder = mTestApis.users().createUser();

        assertThrows(NullPointerException.class, () -> userBuilder.type(null));
    }

    @Test
    public void createUser_specifiesSystemUserType_throwsException() {
        UserType type = mTestApis.users().supportedType(SYSTEM_USER_TYPE_NAME);
        UserBuilder userBuilder = mTestApis.users().createUser()
                .type(type);

        assertThrows(NeneException.class, userBuilder::create);
    }

    @Test
    public void createUser_specifiesSecondaryUserType_createsUser() {
        UserReference user = mTestApis.users().createUser().type(mSecondaryUserType).create();

        try {
            assertThat(user.resolve()).isNotNull();
        } finally {
            user.remove();
        }
    }

    @Test
    @EnsureHasNoDeviceOwner // Device Owners can disable managed profiles
    public void createUser_specifiesManagedProfileUserType_createsUser() {
        UserReference systemUser = mTestApis.users().system();
        UserReference user = mTestApis.users().createUser()
                .type(mManagedProfileType).parent(systemUser).create();

        try {
            assertThat(user.resolve()).isNotNull();
        } finally {
            user.remove();
        }
    }

    @Test
    public void createUser_createsProfile_parentIsSet() {
        UserReference systemUser = mTestApis.users().system();
        UserReference user = mTestApis.users().createUser()
                .type(mManagedProfileType).parent(systemUser).create();

        try {
            assertThat(user.resolve().parent()).isEqualTo(mTestApis.users().system());
        } finally {
            user.remove();
        }
    }

    @Test
    public void createUser_specifiesParentOnNonProfileType_throwsException() {
        UserReference systemUser = mTestApis.users().system();
        UserBuilder userBuilder = mTestApis.users().createUser()
                .type(mSecondaryUserType).parent(systemUser);

        assertThrows(NeneException.class, userBuilder::create);
    }

    @Test
    public void createUser_specifiesProfileTypeWithoutParent_throwsException() {
        UserBuilder userBuilder = mTestApis.users().createUser()
                .type(mManagedProfileType);

        assertThrows(NeneException.class, userBuilder::create);
    }

    @Test
    public void createUser_androidLessThanS_createsManagedProfileNotOnSystemUser_throwsException() {
        assumeTrue("After Android S, managed profiles may be a profile of a non-system user",
                SDK_INT < Build.VERSION_CODES.S);

        UserReference nonSystemUser = mTestApis.users().createUser().create();

        try {
            UserBuilder userBuilder = mTestApis.users().createUser()
                    .type(mManagedProfileType)
                    .parent(nonSystemUser);

            assertThrows(NeneException.class, userBuilder::create);
        } finally {
            nonSystemUser.remove();
        }
    }

    @Test
    public void createAndStart_isStarted() {
        User user = null;

        try {
            user = mTestApis.users().createUser().name(USER_NAME).createAndStart().resolve();
            assertThat(user.state()).isEqualTo(User.UserState.RUNNING_UNLOCKED);
        } finally {
            if (user != null) {
                user.remove();
            }
        }
    }

    @Test
    public void system_hasId0() {
        assertThat(mTestApis.users().system().id()).isEqualTo(0);
    }

    @Test
    public void instrumented_hasCurrentProccessId() {
        assertThat(mTestApis.users().instrumented().id())
                .isEqualTo(android.os.Process.myUserHandle().getIdentifier());
    }

    @Test
    @EnsureHasNoSecondaryUser
    public void findUsersOfType_noMatching_returnsEmptySet() {
        assertThat(mTestApis.users().findUsersOfType(mSecondaryUserType)).isEmpty();
    }

    @Test
    public void findUsersOfType_nullType_throwsException() {
        assertThrows(NullPointerException.class,
                () -> mTestApis.users().findUsersOfType(null));
    }

    @Test
    @EnsureHasSecondaryUser
    @Ignore("TODO: Re-enable when harrier .secondaryUser() only"
            + " returns the harrier-managed secondary user")
    public void findUsersOfType_returnsUsers() {
        try (UserReference additionalUser = mTestApis.users().createUser().create()) {
            assertThat(mTestApis.users().findUsersOfType(mSecondaryUserType))
                    .containsExactly(sDeviceState.secondaryUser(), additionalUser);
        }
    }

    @Test
    public void findUsersOfType_profileType_throwsException() {
        assertThrows(NeneException.class,
                () -> mTestApis.users().findUsersOfType(mManagedProfileType));
    }

    @Test
    @EnsureHasNoSecondaryUser
    public void findUserOfType_noMatching_returnsNull() {
        assertThat(mTestApis.users().findUserOfType(mSecondaryUserType)).isNull();
    }

    @Test
    public void findUserOfType_nullType_throwsException() {
        assertThrows(NullPointerException.class,
                () -> mTestApis.users().findUserOfType(null));
    }

    @Test
    @EnsureHasSecondaryUser
    public void findUserOfType_multipleMatchingUsers_throwsException() {
        try (UserReference additionalUser = mTestApis.users().createUser().create()) {
            assertThrows(NeneException.class,
                    () -> mTestApis.users().findUserOfType(mSecondaryUserType));
        }
    }

    @Test
    @EnsureHasSecondaryUser // TODO(scottjonathan): This should have a way of specifying exactly 1
    public void findUserOfType_oneMatchingUser_returnsUser() {
        assertThat(mTestApis.users().findUserOfType(mSecondaryUserType)).isNotNull();
    }

    @Test
    public void findUserOfType_profileType_throwsException() {
        assertThrows(NeneException.class,
                () -> mTestApis.users().findUserOfType(mManagedProfileType));
    }

    @Test
    @EnsureHasNoWorkProfile
    public void findProfilesOfType_noMatching_returnsEmptySet() {
        assertThat(mTestApis.users().findProfilesOfType(mManagedProfileType, mInstrumentedUser))
                .isEmpty();
    }

    @Test
    public void findProfilesOfType_nullType_throwsException() {
        assertThrows(NullPointerException.class,
                () -> mTestApis.users().findProfilesOfType(
                        /* userType= */ null, mInstrumentedUser));
    }

    @Test
    public void findProfilesOfType_nullParent_throwsException() {
        assertThrows(NullPointerException.class,
                () -> mTestApis.users().findProfilesOfType(
                        mManagedProfileType, /* parent= */ null));
    }

    // TODO(scottjonathan): Once we have profiles which support more than one instance, test this

    @Test
    @EnsureHasNoWorkProfile
    public void findProfileOfType_noMatching_returnsNull() {
        assertThat(mTestApis.users().findProfileOfType(mManagedProfileType, mInstrumentedUser))
                .isNull();
    }

    @Test
    public void findProfilesOfType_nonProfileType_throwsException() {
        assertThrows(NeneException.class,
                () -> mTestApis.users().findProfilesOfType(mSecondaryUserType, mInstrumentedUser));
    }

    @Test
    public void findProfileOfType_nullType_throwsException() {
        assertThrows(NullPointerException.class,
                () -> mTestApis.users().findProfileOfType(/* userType= */ null, mInstrumentedUser));
    }

    @Test
    public void findProfileOfType_nonProfileType_throwsException() {
        assertThrows(NeneException.class,
                () -> mTestApis.users().findProfileOfType(mSecondaryUserType, mInstrumentedUser));
    }

    @Test
    public void findProfileOfType_nullParent_throwsException() {
        assertThrows(NullPointerException.class,
                () -> mTestApis.users().findProfileOfType(mManagedProfileType, /* parent= */ null));
    }

    @Test
    @EnsureHasWorkProfile // TODO(scottjonathan): This should have a way of specifying exactly 1
    public void findProfileOfType_oneMatchingUser_returnsUser() {
        assertThat(mTestApis.users().findProfileOfType(mManagedProfileType, mInstrumentedUser))
                .isNotNull();
    }

    @Test
    public void nonExisting_userDoesNotExist() {
        UserReference userReference = mTestApis.users().nonExisting();

        assertThat(userReference.resolve()).isNull();
    }
}
