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

package com.android.bedstead.harrier;

import static android.Manifest.permission.INTERACT_ACROSS_PROFILES;
import static android.Manifest.permission.INTERACT_ACROSS_USERS_FULL;
import static android.content.pm.PackageManager.PERMISSION_DENIED;
import static android.content.pm.PackageManager.PERMISSION_GRANTED;

import static com.android.bedstead.harrier.DeviceState.UserType.ANY;
import static com.android.bedstead.harrier.DeviceState.UserType.PRIMARY_USER;
import static com.android.bedstead.harrier.annotations.RequireAospBuild.GMS_CORE_PACKAGE;
import static com.android.bedstead.harrier.annotations.RequireCnGmsBuild.CHINA_GOOGLE_SERVICES_FEATURE;
import static com.android.bedstead.nene.users.UserType.MANAGED_PROFILE_TYPE_NAME;
import static com.android.bedstead.nene.users.UserType.SECONDARY_USER_TYPE_NAME;
import static com.android.bedstead.nene.users.UserType.SYSTEM_USER_TYPE_NAME;

import static com.google.common.truth.Truth.assertThat;

import static org.testng.Assert.assertThrows;

import android.os.Build;

import com.android.bedstead.harrier.annotations.EnsureDoesNotHavePermission;
import com.android.bedstead.harrier.annotations.EnsureHasNoSecondaryUser;
import com.android.bedstead.harrier.annotations.EnsureHasNoTvProfile;
import com.android.bedstead.harrier.annotations.EnsureHasNoWorkProfile;
import com.android.bedstead.harrier.annotations.EnsureHasPermission;
import com.android.bedstead.harrier.annotations.EnsureHasSecondaryUser;
import com.android.bedstead.harrier.annotations.EnsureHasTvProfile;
import com.android.bedstead.harrier.annotations.EnsureHasWorkProfile;
import com.android.bedstead.harrier.annotations.EnsurePackageNotInstalled;
import com.android.bedstead.harrier.annotations.RequireAospBuild;
import com.android.bedstead.harrier.annotations.RequireCnGmsBuild;
import com.android.bedstead.harrier.annotations.RequireDoesNotHaveFeature;
import com.android.bedstead.harrier.annotations.RequireFeature;
import com.android.bedstead.harrier.annotations.RequireGmsBuild;
import com.android.bedstead.harrier.annotations.RequireNotCnGmsBuild;
import com.android.bedstead.harrier.annotations.RequirePackageInstalled;
import com.android.bedstead.harrier.annotations.RequirePackageNotInstalled;
import com.android.bedstead.harrier.annotations.RequireRunOnPrimaryUser;
import com.android.bedstead.harrier.annotations.RequireRunOnSecondaryUser;
import com.android.bedstead.harrier.annotations.RequireRunOnTvProfile;
import com.android.bedstead.harrier.annotations.RequireRunOnWorkProfile;
import com.android.bedstead.harrier.annotations.RequireSdkVersion;
import com.android.bedstead.harrier.annotations.RequireUserSupported;
import com.android.bedstead.harrier.annotations.enterprise.EnsureHasDeviceOwner;
import com.android.bedstead.harrier.annotations.enterprise.EnsureHasNoDeviceOwner;
import com.android.bedstead.harrier.annotations.enterprise.EnsureHasNoProfileOwner;
import com.android.bedstead.harrier.annotations.enterprise.EnsureHasProfileOwner;
import com.android.bedstead.harrier.annotations.parameterized.IncludeRunOnDeviceOwnerUser;
import com.android.bedstead.harrier.annotations.parameterized.IncludeRunOnNonAffiliatedDeviceOwnerSecondaryUser;
import com.android.bedstead.harrier.annotations.parameterized.IncludeRunOnParentOfProfileOwner;
import com.android.bedstead.harrier.annotations.parameterized.IncludeRunOnProfileOwnerProfile;
import com.android.bedstead.harrier.annotations.parameterized.IncludeRunOnSecondaryUserInDifferentProfileGroupToProfileOwner;
import com.android.bedstead.nene.TestApis;
import com.android.bedstead.nene.packages.Package;
import com.android.bedstead.nene.users.UserReference;

import org.junit.ClassRule;
import org.junit.Ignore;
import org.junit.Rule;
import org.junit.Test;
import org.junit.runner.RunWith;

@RunWith(BedsteadJUnit4.class)
public class DeviceStateTest {

    @ClassRule
    @Rule
    public static final DeviceState sDeviceState = new DeviceState();

    private static final TestApis sTestApis = new TestApis();
    private static final String TV_PROFILE_TYPE_NAME = "com.android.tv.profile";

    private static final String TEST_PERMISSION_1 = INTERACT_ACROSS_PROFILES;
    private static final String TEST_PERMISSION_2 = INTERACT_ACROSS_USERS_FULL;

    @Test
    @EnsureHasWorkProfile
    public void workProfile_workProfileProvided_returnsWorkProfile() {
        assertThat(sDeviceState.workProfile()).isNotNull();
    }

    @Test
    @EnsureHasWorkProfile
    public void profile_profileIsProvided_returnsProfile() {
        assertThat(sDeviceState.profile(MANAGED_PROFILE_TYPE_NAME)).isNotNull();
    }

    @Test
    @RequireRunOnWorkProfile
    public void workProfile_runningOnWorkProfile_returnsCurrentProfile() {
        assertThat(sDeviceState.workProfile()).isEqualTo(sTestApis.users().instrumented());
    }

    @Test
    @EnsureHasNoWorkProfile
    public void workProfile_noWorkProfile_throwsException() {
        assertThrows(IllegalStateException.class, sDeviceState::workProfile);
    }

    @Test
    @EnsureHasNoWorkProfile
    @EnsureHasNoDeviceOwner
    public void workProfile_createdWorkProfile_throwsException() {
        try (UserReference workProfile = sTestApis.users().createUser()
                .parent(sTestApis.users().instrumented())
                .type(sTestApis.users().supportedType(MANAGED_PROFILE_TYPE_NAME))
                .create()) {
            assertThrows(IllegalStateException.class, sDeviceState::workProfile);
        }
    }

    @Test
    @EnsureHasWorkProfile
    public void ensureHasWorkProfileAnnotation_workProfileExists() {
        assertThat(sTestApis.users().findProfileOfType(
                sTestApis.users().supportedType(MANAGED_PROFILE_TYPE_NAME),
                sTestApis.users().instrumented())
        ).isNotNull();
    }

    // TODO(scottjonathan): test the installTestApp argument
    // TODO(scottjonathan): When supported, test the forUser argument

    @Test
    @EnsureHasNoWorkProfile
    public void ensureHasNoWorkProfileAnnotation_workProfileDoesNotExist() {
        assertThat(sTestApis.users().findProfileOfType(
                sTestApis.users().supportedType(MANAGED_PROFILE_TYPE_NAME),
                sTestApis.users().instrumented())
        ).isNull();
    }

    @Test
    @EnsureHasTvProfile
    public void tvProfile_tvProfileProvided_returnsTvProfile() {
        assertThat(sDeviceState.tvProfile()).isNotNull();
    }

    @Test
    @RequireRunOnTvProfile
    public void tvProfile_runningOnTvProfile_returnsCurrentProfile() {
        assertThat(sDeviceState.tvProfile()).isEqualTo(sTestApis.users().instrumented());
    }

    @Test
    @EnsureHasNoTvProfile
    public void tvProfile_noTvProfile_throwsException() {
        assertThrows(IllegalStateException.class, sDeviceState::tvProfile);
    }

    @Test
    @RequireUserSupported(TV_PROFILE_TYPE_NAME)
    @EnsureHasNoTvProfile
    public void tvProfile_createdTvProfile_throwsException() {
        try (UserReference tvProfile = sTestApis.users().createUser()
                .parent(sTestApis.users().instrumented())
                .type(sTestApis.users().supportedType(TV_PROFILE_TYPE_NAME))
                .create()) {
            assertThrows(IllegalStateException.class, sDeviceState::tvProfile);
        }
    }

    @Test
    @EnsureHasTvProfile
    public void ensureHasTvProfileAnnotation_tvProfileExists() {
        assertThat(sTestApis.users().findProfileOfType(
                sTestApis.users().supportedType(TV_PROFILE_TYPE_NAME),
                sTestApis.users().instrumented())
        ).isNotNull();
    }

    // TODO(scottjonathan): test the installTestApp argument
    // TODO(scottjonathan): When supported, test the forUser argument

    @Test
    @RequireUserSupported(TV_PROFILE_TYPE_NAME)
    @EnsureHasNoTvProfile
    public void ensureHasNoTvProfileAnnotation_tvProfileDoesNotExist() {
        assertThat(sTestApis.users().findProfileOfType(
                sTestApis.users().supportedType(TV_PROFILE_TYPE_NAME),
                sTestApis.users().instrumented())
        ).isNull();
    }

    @Test
    @EnsureHasSecondaryUser
    public void secondaryUser_secondaryUserProvided_returnsSecondaryUser() {
        assertThat(sDeviceState.secondaryUser()).isNotNull();
    }

    @Test
    @EnsureHasSecondaryUser
    public void user_userProvided_returnUser() {
        assertThat(sDeviceState.user(SECONDARY_USER_TYPE_NAME)).isNotNull();
    }

    @Test
    @RequireRunOnSecondaryUser
    public void secondaryUser_runningOnSecondaryUser_returnsCurrentUser() {
        assertThat(sDeviceState.secondaryUser()).isEqualTo(sTestApis.users().instrumented());
    }

    @Test
    @EnsureHasNoSecondaryUser
    public void secondaryUser_noSecondaryUser_throwsException() {
        assertThrows(IllegalStateException.class, sDeviceState::secondaryUser);
    }

    @Test
    @EnsureHasNoSecondaryUser
    public void secondaryUser_createdSecondaryUser_throwsException() {
        try (UserReference secondaryUser = sTestApis.users().createUser()
                .type(sTestApis.users().supportedType(SECONDARY_USER_TYPE_NAME))
                .create()) {
            assertThrows(IllegalStateException.class, sDeviceState::secondaryUser);
        }
    }

    @Test
    @EnsureHasSecondaryUser
    public void ensureHasSecondaryUserAnnotation_secondaryUserExists() {
        assertThat(sTestApis.users().findUserOfType(
                sTestApis.users().supportedType(SECONDARY_USER_TYPE_NAME))
        ).isNotNull();
    }

    // TODO(scottjonathan): test the installTestApp argument
    // TODO(scottjonathan): Test the forUser argument

    @Test
    @EnsureHasNoSecondaryUser
    public void ensureHasNoSecondaryUserAnnotation_secondaryUserDoesNotExist() {
        assertThat(sTestApis.users().findUserOfType(
                sTestApis.users().supportedType(SECONDARY_USER_TYPE_NAME))
        ).isNull();
    }

    @Test
    @EnsureHasPermission(TEST_PERMISSION_1)
    public void ensureHasPermission_permissionIsGranted() {
        assertThat(sTestApis.context().instrumentedContext()
                .checkSelfPermission(TEST_PERMISSION_1)).isEqualTo(PERMISSION_GRANTED);
    }

    @Test
    @EnsureHasPermission({TEST_PERMISSION_1, TEST_PERMISSION_2})
    public void ensureHasPermission_multiplePermissions_permissionsAreGranted() {
        assertThat(sTestApis.context().instrumentedContext()
                .checkSelfPermission(TEST_PERMISSION_1)).isEqualTo(PERMISSION_GRANTED);
        assertThat(sTestApis.context().instrumentedContext()
                .checkSelfPermission(TEST_PERMISSION_2)).isEqualTo(PERMISSION_GRANTED);
    }

    @Test
    @EnsureDoesNotHavePermission(TEST_PERMISSION_1)
    public void ensureDoesNotHavePermission_permissionIsDenied() {
        assertThat(sTestApis.context().instrumentedContext()
                .checkSelfPermission(TEST_PERMISSION_1)).isEqualTo(PERMISSION_DENIED);
    }

    @Test
    @EnsureDoesNotHavePermission({TEST_PERMISSION_1, TEST_PERMISSION_2})
    public void ensureDoesNotHavePermission_multiplePermissions_permissionsAreDenied() {
        assertThat(sTestApis.context().instrumentedContext()
                .checkSelfPermission(TEST_PERMISSION_1)).isEqualTo(PERMISSION_DENIED);
        assertThat(sTestApis.context().instrumentedContext()
                .checkSelfPermission(TEST_PERMISSION_2)).isEqualTo(PERMISSION_DENIED);
    }

    @Test
    @EnsureHasPermission(TEST_PERMISSION_1)
    @EnsureDoesNotHavePermission(TEST_PERMISSION_2)
    public void ensureHasPermissionAndDoesNotHavePermission_permissionsAreCorrect() {
        assertThat(sTestApis.context().instrumentedContext()
                .checkSelfPermission(TEST_PERMISSION_1)).isEqualTo(PERMISSION_GRANTED);
        assertThat(sTestApis.context().instrumentedContext()
                .checkSelfPermission(TEST_PERMISSION_2)).isEqualTo(PERMISSION_DENIED);
    }

    @EnsureHasDeviceOwner
    public void ensureHasDeviceOwnerAnnotation_deviceOwnerIsSet() {
        assertThat(sTestApis.devicePolicy().getDeviceOwner()).isNotNull();
    }

    @Test
    @EnsureHasNoDeviceOwner
    public void ensureHasNoDeviceOwnerAnnotation_deviceOwnerIsNotSet() {
        assertThat(sTestApis.devicePolicy().getDeviceOwner()).isNull();
    }

    @Test
    @EnsureHasDeviceOwner
    public void deviceOwner_deviceOwnerIsSet_returnsDeviceOwner() {
        assertThat(sDeviceState.deviceOwner()).isNotNull();
    }

    @Test
    @EnsureHasNoDeviceOwner
    public void deviceOwner_deviceOwnerIsNotSet_throwsException() {
        assertThrows(IllegalStateException.class, sDeviceState::deviceOwner);
    }

    @Test
    @EnsureHasProfileOwner
    public void ensureHasProfileOwnerAnnotation_defaultUser_profileOwnerIsSet() {
        assertThat(sTestApis.devicePolicy().getProfileOwner(sTestApis.users().instrumented()))
                .isNotNull();
    }

    @Test
    @EnsureHasNoProfileOwner
    public void ensureHasNoProfileOwnerAnnotation_defaultUser_profileOwnerIsNotSet() {
        assertThat(sTestApis.devicePolicy().getProfileOwner(sTestApis.users().instrumented()))
                .isNull();
    }

    @EnsureHasSecondaryUser
    @EnsureHasProfileOwner(onUser = DeviceState.UserType.SECONDARY_USER)
    public void ensureHasProfileOwnerAnnotation_otherUser_setsProfileOwner() {
        assertThat(sTestApis.devicePolicy().getProfileOwner(sDeviceState.secondaryUser()))
                .isNotNull();
    }

    @EnsureHasSecondaryUser
    @EnsureHasNoProfileOwner(onUser = DeviceState.UserType.SECONDARY_USER)
    public void ensureHasNoProfileOwnerAnnotation_otherUser_profileOwnerIsNotSet() {
        assertThat(sTestApis.devicePolicy().getProfileOwner(sDeviceState.secondaryUser())).isNull();
    }

    @Test
    @EnsureHasNoProfileOwner
    public void profileOwner_defaultUser_profileOwnerIsNotSet_throwsException() {
        assertThrows(IllegalStateException.class, sDeviceState::profileOwner);
    }

    @Test
    @EnsureHasProfileOwner
    public void profileOwner_defaultUser_profileOwnerIsSet_returnsProfileOwner() {
        assertThat(sTestApis.devicePolicy().getProfileOwner(sTestApis.users().instrumented()))
                .isNotNull();
    }

    @EnsureHasSecondaryUser
    @EnsureHasProfileOwner(onUser = DeviceState.UserType.SECONDARY_USER)
    public void profileOwner_otherUser_profileOwnerIsSet_returnsProfileOwner() {
        assertThat(sDeviceState.profileOwner(sDeviceState.secondaryUser())).isNotNull();
    }

    @Test
    @EnsureHasSecondaryUser
    @EnsureHasNoProfileOwner(onUser = DeviceState.UserType.SECONDARY_USER)
    public void profileOwner_otherUser_profileOwnerIsNotSet_throwsException() {
        assertThrows(IllegalStateException.class, sDeviceState::profileOwner);
    }

    @Test
    public void profileOwner_userType_onUserIsNull_throwsException() {
        assertThrows(NullPointerException.class,
                () -> sDeviceState.profileOwner((DeviceState.UserType) null));
    }

    @Test
    public void profileOwner_userReference_onUserIsNull_throwsException() {
        assertThrows(NullPointerException.class,
                () -> sDeviceState.profileOwner((UserReference) null));
    }

    @RequireRunOnWorkProfile
    public void requireRunOnWorkProfileAnnotation_isRunningOnWorkProfile() {
        assertThat(
                sTestApis.users().instrumented().resolve().type().name())
                .isEqualTo(MANAGED_PROFILE_TYPE_NAME);
    }

    @Test
    @RequireRunOnWorkProfile
    public void requireRunOnWorkProfileAnnotation_workProfileHasProfileOwner() {
        assertThat(
                sTestApis.devicePolicy().getProfileOwner(sTestApis.users().instrumented())
        ).isNotNull();
    }

    @Test
    @RequireRunOnSecondaryUser
    public void requireRunOnSecondaryUserAnnotation_isRunningOnSecondaryUser() {
        assertThat(
                sTestApis.users().instrumented().resolve().type().name())
                .isEqualTo(SECONDARY_USER_TYPE_NAME);
    }

    @Test
    @IncludeRunOnDeviceOwnerUser
    public void includeRunOnDeviceOwnerUserAnnotation_isRunningOnDeviceOwnerUser() {
        assertThat(sTestApis.devicePolicy().getDeviceOwner().user())
                .isEqualTo(sTestApis.users().instrumented());
    }

    @Test
    @IncludeRunOnNonAffiliatedDeviceOwnerSecondaryUser
    public void includeRunOnNonAffiliatedDeviceOwnerSecondaryUserAnnotation_isRunningOnNonAffiliatedDeviceOwnerSecondaryUser() {
        assertThat(sTestApis.devicePolicy().getDeviceOwner().user())
                .isNotEqualTo(sTestApis.users().instrumented());
        assertThat(sTestApis.users().instrumented().resolve().type().name())
                .isEqualTo(SECONDARY_USER_TYPE_NAME);
    }

    @Test
    @IncludeRunOnProfileOwnerProfile
    public void includeRunOnProfileOwnerAnnotation_hasProfileOwner() {
        assertThat(
                sTestApis.devicePolicy().getProfileOwner(sTestApis.users().instrumented())
        ).isNotNull();
    }

    @Test
    @IncludeRunOnSecondaryUserInDifferentProfileGroupToProfileOwner
    public void includeRunOnSecondaryUserInDifferentProfileGroupToProfileOwnerAnnotation_isRunningOnSecondaryUserInDifferentProfileGroupToProfileOwner() {
        assertThat(sTestApis.users().instrumented().resolve().type().name())
                .isEqualTo(SECONDARY_USER_TYPE_NAME);
        assertThat(sDeviceState.workProfile(PRIMARY_USER))
                .isNotEqualTo(sTestApis.users().instrumented());
        assertThat(sTestApis.devicePolicy().getProfileOwner(sDeviceState.workProfile(PRIMARY_USER)))
                .isNotNull();

        // TODO(scottjonathan): Assert profile groups are different
    }

    @RequirePackageInstalled(value = GMS_CORE_PACKAGE, onUser = ANY)
    public void requirePackageInstalledAnnotation_anyUser_packageIsInstalled() {
        assertThat(sTestApis.packages().find(GMS_CORE_PACKAGE).resolve()).isNotNull();
    }

    @Test
    @RequirePackageInstalled(GMS_CORE_PACKAGE)
    public void requirePackageInstalledAnnotation_currentUser_packageIsInstalled() {
        assertThat(sTestApis.packages().find(GMS_CORE_PACKAGE).resolve().installedOnUsers())
                .contains(sTestApis.users().instrumented());
    }

    @Test
    @RequirePackageNotInstalled(value = GMS_CORE_PACKAGE, onUser = ANY)
    public void requirePackageNotInstalledAnnotation_anyUser_packageIsNotInstalled() {
        assertThat(sTestApis.packages().find(GMS_CORE_PACKAGE).resolve()).isNull();

    }

    @Test
    @RequirePackageNotInstalled(GMS_CORE_PACKAGE)
    public void requirePackageNotInstalledAnnotation_currentUser_packageIsNotInstalled() {
        Package resolvedPackage = sTestApis.packages().find(GMS_CORE_PACKAGE).resolve();

        if (resolvedPackage != null) {
            assertThat(resolvedPackage.installedOnUsers())
                    .doesNotContain(sTestApis.users().instrumented());
        }
    }

    @Test
    @EnsurePackageNotInstalled(value = GMS_CORE_PACKAGE, onUser = ANY)
    @Ignore // TODO(scottjonathan): Restore this with a package which can be uninstalled
    public void ensurePackageNotInstalledAnnotation_anyUser_packageIsNotInstalled() {
        assertThat(sTestApis.packages().find(GMS_CORE_PACKAGE).resolve()).isNull();
    }

    @Test
    @EnsurePackageNotInstalled(GMS_CORE_PACKAGE)
    @Ignore // TODO(scottjonathan): Restore this with a package which can be uninstalled
    public void ensurePackageNotInstalledAnnotation_currentUser_packageIsNotInstalled() {
        Package resolvedPackage = sTestApis.packages().find(GMS_CORE_PACKAGE).resolve();

        if (resolvedPackage != null) {
            assertThat(resolvedPackage.installedOnUsers())
                    .doesNotContain(sTestApis.users().instrumented());
        }
    }

    @Test
    @RequireAospBuild
    public void requireAospBuildAnnotation_isRunningOnAospBuild() {
        assertThat(sTestApis.packages().find(GMS_CORE_PACKAGE).resolve()).isNull();
    }

    @Test
    @RequireGmsBuild
    public void requireGmsBuildAnnotation_isRunningOnGmsbuild() {
        assertThat(sTestApis.packages().find(GMS_CORE_PACKAGE).resolve()).isNotNull();
    }

    @Test
    @RequireCnGmsBuild
    public void requireCnGmsBuildAnnotation_isRunningOnCnGmsBuild() {
        assertThat(sTestApis.packages().features()).contains(CHINA_GOOGLE_SERVICES_FEATURE);
    }

    @Test
    @RequireNotCnGmsBuild
    public void requireNotCnGmsBuildAnnotation_isNotRunningOnCnGmsBuild() {
        assertThat(sTestApis.packages().features()).doesNotContain(CHINA_GOOGLE_SERVICES_FEATURE);

    }

    @Test
    @RequireFeature(CHINA_GOOGLE_SERVICES_FEATURE)
    public void requireHasFeatureAnnotation_doesNotHaveFeature() {
        assertThat(sTestApis.packages().features()).contains(CHINA_GOOGLE_SERVICES_FEATURE);
    }

    @Test
    @RequireDoesNotHaveFeature(CHINA_GOOGLE_SERVICES_FEATURE)
    public void requireDoesNotHaveFeatureAnnotation_doesNotHaveFeature() {
        assertThat(sTestApis.packages().features()).doesNotContain(CHINA_GOOGLE_SERVICES_FEATURE);
    }

    @Test
    @RequireSdkVersion(min = 27)
    public void requireSdkVersionAnnotation_min_minIsMet() {
        assertThat(Build.VERSION.SDK_INT).isGreaterThan(26);
    }

    @Test
    @RequireSdkVersion(max = 30)
    public void requireSdkVersionAnnotation_max_maxIsMet() {
        assertThat(Build.VERSION.SDK_INT).isLessThan(31);
    }

    @Test
    @RequireSdkVersion(min = 27, max = 30)
    public void requireSdkVersionAnnotation_minAndMax_bothAreMet() {
        assertThat(Build.VERSION.SDK_INT).isGreaterThan(26);
        assertThat(Build.VERSION.SDK_INT).isLessThan(31);
    }

    @Test
    @RequireRunOnPrimaryUser
    public void requireRunOnPrimaryUserAnnotation_isRunningOnPrimaryUser() {
        assertThat(sTestApis.users().instrumented().resolve().type().name())
                .isEqualTo(SYSTEM_USER_TYPE_NAME);
    }

    @Test
    @RequireRunOnTvProfile
    public void requireRunOnTvProfileAnnotation_isRunningOnTvProfile() {
        assertThat(sTestApis.users().instrumented().resolve().type().name())
                .isEqualTo(TV_PROFILE_TYPE_NAME);
    }

    @Test
    @IncludeRunOnParentOfProfileOwner
    public void includeRunOnParentOfProfileOwnerAnnotation_isRunningOnParentOfProfileOwner() {
        assertThat(sDeviceState.workProfile()).isNotNull();
    }
}
