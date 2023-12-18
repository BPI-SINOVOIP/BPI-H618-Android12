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

package android.devicepolicy.cts;

import static android.Manifest.permission.INTERACT_ACROSS_USERS;
import static android.Manifest.permission.INTERACT_ACROSS_USERS_FULL;
import static android.Manifest.permission.WRITE_SECURE_SETTINGS;
import static android.app.AppOpsManager.MODE_ALLOWED;

import static com.google.common.truth.Truth.assertThat;
import static com.google.common.truth.Truth.assertWithMessage;

import android.accounts.Account;
import android.accounts.AccountManager;
import android.app.AppOpsManager;
import android.app.admin.DevicePolicyManager;
import android.app.admin.FullyManagedDeviceProvisioningParams;
import android.app.admin.ManagedProfileProvisioningParams;
import android.content.ComponentName;
import android.content.Context;
import android.content.SharedPreferences;
import android.content.pm.CrossProfileApps;
import android.content.pm.PackageManager;
import android.os.UserHandle;
import android.os.UserManager;
import android.provider.Settings;

import androidx.test.core.app.ApplicationProvider;

import com.android.bedstead.deviceadminapp.DeviceAdminApp;
import com.android.bedstead.harrier.BedsteadJUnit4;
import com.android.bedstead.harrier.DeviceState;
import com.android.bedstead.harrier.annotations.EnsureHasNoWorkProfile;
import com.android.bedstead.harrier.annotations.EnsureHasPermission;
import com.android.bedstead.harrier.annotations.RequireDoesNotHaveFeature;
import com.android.bedstead.harrier.annotations.RequireFeature;
import com.android.bedstead.harrier.annotations.RequireRunOnPrimaryUser;
import com.android.bedstead.harrier.annotations.Postsubmit;
import com.android.bedstead.nene.TestApis;
import com.android.bedstead.nene.packages.Package;
import com.android.bedstead.nene.permissions.PermissionContext;
import com.android.compatibility.common.util.SystemUtil;

import org.junit.ClassRule;
import org.junit.Ignore;
import org.junit.Rule;
import org.junit.Test;
import org.junit.runner.RunWith;

import java.util.ArrayList;
import java.util.Collections;
import java.util.Set;
import java.util.stream.Collectors;

@RunWith(BedsteadJUnit4.class)
public final class DevicePolicyManagerTest {
    private static final Context sContext = ApplicationProvider.getApplicationContext();
    private static final DevicePolicyManager sDevicePolicyManager =
            sContext.getSystemService(DevicePolicyManager.class);
    private static final PackageManager sPackageManager = sContext.getPackageManager();
    private static final UserManager sUserManager = sContext.getSystemService(UserManager.class);
    private static final SharedPreferences sSharedPreferences =
            sContext.getSharedPreferences("required-apps.txt", Context.MODE_PRIVATE);
    private static final TestApis sTestApis = new TestApis();

    private static final ComponentName DEVICE_ADMIN_COMPONENT_NAME =
            DeviceAdminApp.deviceAdminComponentName(sContext);

    private static final String MANAGE_PROFILE_AND_DEVICE_OWNERS =
            "android.permission.MANAGE_PROFILE_AND_DEVICE_OWNERS";
    private static final String MANAGE_DEVICE_ADMINS = "android.permission.MANAGE_DEVICE_ADMINS";

    private static final String PROFILE_OWNER_NAME = "testDeviceAdmin";
    private static final String DEVICE_OWNER_NAME = "testDeviceAdmin";

    private static final String ACCOUNT_NAME = "CTS";
    private static final String ACCOUNT_TYPE = "com.android.cts.test";
    private static final Account TEST_ACCOUNT = new Account(ACCOUNT_NAME, ACCOUNT_TYPE);

    private static final String USER_SETUP_COMPLETE_KEY = "user_setup_complete";

    private static final String KEY_PRE_PROVISIONING_SYSTEM_APPS = "pre_provisioning_system_apps";
    private static final String KEY_PRE_PROVISIONING_NON_SYSTEM_APPS =
            "pre_provisioning_non_system_apps";

    private static final String SET_DEVICE_OWNER_ACTIVE_ADMIN_COMMAND =
            "dpm set-active-admin --user cur " + DEVICE_ADMIN_COMPONENT_NAME.flattenToString();
    private static final String SET_DEVICE_OWNER_COMMAND =
            "dpm set-device-owner --user cur " + DEVICE_ADMIN_COMPONENT_NAME.flattenToString();
    private static final String REMOVE_ACTIVE_ADMIN_COMMAND =
            "dpm remove-active-admin --user cur " + DEVICE_ADMIN_COMPONENT_NAME.flattenToString();

    @ClassRule
    @Rule
    public static final DeviceState sDeviceState = new DeviceState();


    @RequireRunOnPrimaryUser
    @EnsureHasNoWorkProfile
    @RequireFeature(PackageManager.FEATURE_DEVICE_ADMIN)
    @RequireFeature(PackageManager.FEATURE_MANAGED_USERS)
    @Test
    @Postsubmit(reason="b/181207615 flaky")
    @EnsureHasPermission(MANAGE_PROFILE_AND_DEVICE_OWNERS)
    public void newlyProvisionedManagedProfile_createsProfile() throws Exception {
        UserHandle profile = null;
        try {
            ManagedProfileProvisioningParams params =
                    createManagedProfileProvisioningParamsBuilder().build();
            profile = provisionManagedProfile(params);

            assertThat(profile).isNotNull();
        } finally {
            if (profile != null) {
                sTestApis.users().find(profile).remove();
            }
        }
    }

    @RequireRunOnPrimaryUser
    @EnsureHasNoWorkProfile
    @RequireFeature(PackageManager.FEATURE_DEVICE_ADMIN)
    @RequireFeature(PackageManager.FEATURE_MANAGED_USERS)
    @Test
    @Postsubmit(reason="b/181207615 flaky")
    @EnsureHasPermission(MANAGE_PROFILE_AND_DEVICE_OWNERS)
    public void newlyProvisionedManagedProfile_createsManagedProfile() throws Exception {
        UserHandle profile = null;
        try {
            ManagedProfileProvisioningParams params =
                    createManagedProfileProvisioningParamsBuilder().build();
            profile = provisionManagedProfile(params);

            assertThat(sUserManager.isManagedProfile(profile.getIdentifier())).isTrue();
        } finally {
            if (profile != null) {
                sTestApis.users().find(profile).remove();
            }
        }
    }

    @RequireRunOnPrimaryUser
    @EnsureHasNoWorkProfile
    @RequireFeature(PackageManager.FEATURE_DEVICE_ADMIN)
    @RequireFeature(PackageManager.FEATURE_MANAGED_USERS)
    @Test
    @Postsubmit(reason="b/181207615 flaky")
    @EnsureHasPermission({MANAGE_PROFILE_AND_DEVICE_OWNERS, INTERACT_ACROSS_USERS_FULL})
    public void newlyProvisionedManagedProfile_setsActiveAdmin() throws Exception {
        UserHandle profile = null;
        try {
            ManagedProfileProvisioningParams params =
                    createManagedProfileProvisioningParamsBuilder().build();
            profile = provisionManagedProfile(params);

            assertThat(getDpmForUser(profile).getActiveAdmins()).hasSize(1);
            assertThat(getDpmForUser(profile).getActiveAdmins().get(0))
                    .isEqualTo(DEVICE_ADMIN_COMPONENT_NAME);
        } finally {
            if (profile != null) {
                sTestApis.users().find(profile).remove();
            }
        }
    }

    @RequireRunOnPrimaryUser
    @EnsureHasNoWorkProfile
    @RequireFeature(PackageManager.FEATURE_DEVICE_ADMIN)
    @RequireFeature(PackageManager.FEATURE_MANAGED_USERS)
    @Test
    @Postsubmit(reason="b/181207615 flaky")
    @EnsureHasPermission({MANAGE_PROFILE_AND_DEVICE_OWNERS, INTERACT_ACROSS_USERS})
    public void newlyProvisionedManagedProfile_setsProfileOwner() throws Exception {
        UserHandle profile = null;
        try {
            ManagedProfileProvisioningParams params =
                    createManagedProfileProvisioningParamsBuilder().build();
            profile = provisionManagedProfile(params);

            DevicePolicyManager profileDpm = getDpmForUser(profile);
            assertThat(profileDpm.isProfileOwnerApp(sContext.getPackageName())).isTrue();
        } finally {
            if (profile != null) {
                sTestApis.users().find(profile).remove();
            }
        }
    }

    @RequireRunOnPrimaryUser
    @EnsureHasNoWorkProfile
    @RequireFeature(PackageManager.FEATURE_DEVICE_ADMIN)
    @RequireFeature(PackageManager.FEATURE_MANAGED_USERS)
    @Test
    @Postsubmit(reason="new test")
    @Ignore
    @EnsureHasPermission(MANAGE_PROFILE_AND_DEVICE_OWNERS)
    public void newlyProvisionedManagedProfile_copiesAccountToProfile() throws Exception {
        UserHandle profile = null;
        try {
            // TODO(kholoudm): Add account to account manager once the API is ready in Nene
            ManagedProfileProvisioningParams params =
                    createManagedProfileProvisioningParamsBuilder()
                            .setAccountToMigrate(TEST_ACCOUNT)
                            .build();
            profile = provisionManagedProfile(params);

            assertThat(hasTestAccount(profile)).isTrue();
        } finally {
            if (profile != null) {
                sTestApis.users().find(profile).remove();
            }
        }
    }

    @RequireRunOnPrimaryUser
    @EnsureHasNoWorkProfile
    @RequireFeature(PackageManager.FEATURE_DEVICE_ADMIN)
    @RequireFeature(PackageManager.FEATURE_MANAGED_USERS)
    @Test
    @Postsubmit(reason="new test")
    @EnsureHasPermission(MANAGE_PROFILE_AND_DEVICE_OWNERS)
    public void newlyProvisionedManagedProfile_removesAccountFromParentByDefault()
            throws Exception {
        UserHandle profile = null;
        try {
            // TODO(kholoudm): Add account to account manager once the API is ready in Nene
            ManagedProfileProvisioningParams params =
                    createManagedProfileProvisioningParamsBuilder()
                            .setAccountToMigrate(TEST_ACCOUNT)
                            .build();
            profile = provisionManagedProfile(params);

            assertThat(hasTestAccount(sContext.getUser())).isFalse();
        } finally {
            if (profile != null) {
                sTestApis.users().find(profile).remove();
            }
        }
    }

    @RequireRunOnPrimaryUser
    @EnsureHasNoWorkProfile
    @RequireFeature(PackageManager.FEATURE_DEVICE_ADMIN)
    @RequireFeature(PackageManager.FEATURE_MANAGED_USERS)
    @Test
    @Ignore
    @Postsubmit(reason="new test")
    @EnsureHasPermission(MANAGE_PROFILE_AND_DEVICE_OWNERS)
    public void newlyProvisionedManagedProfile_keepsAccountInParentIfRequested() throws Exception {
        UserHandle profile = null;
        try {
            // TODO(kholoudm): Add account to account manager once the API is ready in Nene
            ManagedProfileProvisioningParams params =
                    createManagedProfileProvisioningParamsBuilder()
                            .setAccountToMigrate(TEST_ACCOUNT)
                            .setKeepAccountMigrated(true)
                            .build();
            profile = provisionManagedProfile(params);

            assertThat(hasTestAccount(sContext.getUser())).isTrue();
        } finally {
            if (profile != null) {
                sTestApis.users().find(profile).remove();
            }
        }
    }

    @RequireRunOnPrimaryUser
    @EnsureHasNoWorkProfile
    @RequireFeature(PackageManager.FEATURE_DEVICE_ADMIN)
    @RequireFeature(PackageManager.FEATURE_MANAGED_USERS)
    @Test
    @Postsubmit(reason="new test")
    @EnsureHasPermission(MANAGE_PROFILE_AND_DEVICE_OWNERS)
    public void newlyProvisionedManagedProfile_removesNonRequiredAppsFromProfile()
            throws Exception {
        UserHandle profile = null;
        try {
            Set<String> nonRequiredApps = sDevicePolicyManager.getDisallowedSystemApps(
                    DEVICE_ADMIN_COMPONENT_NAME,
                    sContext.getUserId(),
                    DevicePolicyManager.ACTION_PROVISION_MANAGED_PROFILE);
            ManagedProfileProvisioningParams params =
                    createManagedProfileProvisioningParamsBuilder().build();
            profile = provisionManagedProfile(params);

            assertThat(getInstalledPackagesOnUser(nonRequiredApps, profile)).isEmpty();
        } finally {
            if (profile != null) {
                sTestApis.users().find(profile).remove();
            }
        }
    }

    @RequireRunOnPrimaryUser
    @EnsureHasNoWorkProfile
    @RequireFeature(PackageManager.FEATURE_DEVICE_ADMIN)
    @RequireFeature(PackageManager.FEATURE_MANAGED_USERS)
    @Test
    @Postsubmit(reason="new test")
    @EnsureHasPermission(MANAGE_PROFILE_AND_DEVICE_OWNERS)
    public void newlyProvisionedManagedProfile_setsCrossProfilePackages()
            throws Exception {
        UserHandle profile = null;
        try {
            ManagedProfileProvisioningParams params =
                    createManagedProfileProvisioningParamsBuilder().build();
            profile = provisionManagedProfile(params);

            Set<String> crossProfilePackages = getConfigurableDefaultCrossProfilePackages();
            for(String crossProfilePackage : crossProfilePackages) {
                assertIsCrossProfilePackageIfInstalled(crossProfilePackage);
            }
        } finally {
            if (profile != null) {
                sTestApis.users().find(profile).remove();
            }
        }
    }

    private void assertIsCrossProfilePackageIfInstalled(String packageName) throws Exception {
        if (!isPackageInstalledOnCurrentUser(packageName)) {
            return;
        }
        for (UserHandle profile : sUserManager.getUserProfiles()) {
            assertThat(isCrossProfilePackage(packageName, profile)).isTrue();
        }
    }

    private boolean isCrossProfilePackage(String packageName, UserHandle profile)
            throws Exception {
        return getCrossProfileAppOp(packageName, profile) == MODE_ALLOWED;
    }

    private int getCrossProfileAppOp(String packageName, UserHandle userHandle) throws Exception {
        return sContext.getSystemService(AppOpsManager.class).unsafeCheckOpNoThrow(
                AppOpsManager.permissionToOp(android.Manifest.permission.INTERACT_ACROSS_PROFILES),
                getUidForPackageName(packageName, userHandle),
                packageName);
    }

    private int getUidForPackageName(String packageName, UserHandle userHandle) throws Exception {
        return sContext.createContextAsUser(userHandle, /* flags= */ 0)
                .getPackageManager()
                .getPackageUid(packageName, /* flags= */ 0);
    }

    private UserHandle provisionManagedProfile(ManagedProfileProvisioningParams params)
            throws Exception {
        return sDevicePolicyManager.createAndProvisionManagedProfile(params);
    }

    private ManagedProfileProvisioningParams.Builder
    createManagedProfileProvisioningParamsBuilder() {
        return new ManagedProfileProvisioningParams.Builder(
                        DEVICE_ADMIN_COMPONENT_NAME,
                        PROFILE_OWNER_NAME);
    }

    private boolean hasTestAccount(UserHandle user) {
        AccountManager am = getContextForUser(user).getSystemService(AccountManager.class);
        Account[] userAccounts = am.getAccountsByType(ACCOUNT_TYPE);
        for (Account account : userAccounts) {
            if (TEST_ACCOUNT.equals(account)) {
                return true;
            }
        }
        return false;
    }

    private Set<String> getInstalledPackagesOnUser(Set<String> packages, UserHandle user) {
        return packages.stream().filter(p -> isPackageInstalledOnUser(p, user))
                .collect(Collectors.toSet());
    }

    private boolean isPackageInstalledOnCurrentUser(String packageName) {
        return isPackageInstalledOnUser(packageName, sContext.getUser());
    }

    private boolean isPackageInstalledOnUser(String packageName, UserHandle user) {
        Package resolvedPackage = sTestApis.packages().find(packageName).resolve();
        if (resolvedPackage == null) {
            return false;
        }
        return resolvedPackage.installedOnUsers().contains(sTestApis.users().find(user));
    }

    private Set<String> getConfigurableDefaultCrossProfilePackages() {
        Set<String> defaultPackages = sDevicePolicyManager.getDefaultCrossProfilePackages();
        CrossProfileApps crossProfileApps = sContext.getSystemService(CrossProfileApps.class);
        return defaultPackages.stream().filter(
                crossProfileApps::canConfigureInteractAcrossProfiles).collect(
                Collectors.toSet());
    }

    private DevicePolicyManager getDpmForUser(UserHandle user) {
        return getContextForUser(user).getSystemService(DevicePolicyManager.class);
    }

    private Context getContextForUser(UserHandle user) {
        if (sContext.getUserId() == user.getIdentifier()) {
            return sContext;
        }
        try (PermissionContext p =
                     sTestApis.permissions().withPermission(INTERACT_ACROSS_USERS_FULL)) {
            return sContext.createContextAsUser(user, /* flags= */ 0);
        }
    }

    @RequireRunOnPrimaryUser
    @RequireFeature(PackageManager.FEATURE_DEVICE_ADMIN)
    @Test
    @EnsureHasPermission({MANAGE_PROFILE_AND_DEVICE_OWNERS})
    public void newlyProvisionedFullyManagedDevice_setsDeviceOwner() throws Exception {
        try {
            FullyManagedDeviceProvisioningParams params =
                    createDefaultManagedDeviceProvisioningParamsBuilder().build();
            resetUserSetupCompletedFlag();
            sDevicePolicyManager.provisionFullyManagedDevice(params);

            assertThat(sDevicePolicyManager.isDeviceOwnerApp(sContext.getPackageName())).isTrue();
        } finally {
            sDevicePolicyManager.forceRemoveActiveAdmin(
                    DEVICE_ADMIN_COMPONENT_NAME, sContext.getUserId());
            setUserSetupCompletedFlag();
        }
    }

    @RequireRunOnPrimaryUser
    @RequireFeature(PackageManager.FEATURE_DEVICE_ADMIN)
    @Test
    @EnsureHasPermission(MANAGE_PROFILE_AND_DEVICE_OWNERS)
    public void newlyProvisionedFullyManagedDevice_doesNotThrowException() throws Exception {
        try {
            FullyManagedDeviceProvisioningParams params =
                    createDefaultManagedDeviceProvisioningParamsBuilder().build();
            resetUserSetupCompletedFlag();
            sDevicePolicyManager.provisionFullyManagedDevice(params);
        } finally {
            sDevicePolicyManager.forceRemoveActiveAdmin(
                    DEVICE_ADMIN_COMPONENT_NAME, sContext.getUserId());
            setUserSetupCompletedFlag();
        }
    }

    @RequireRunOnPrimaryUser
    @RequireFeature(PackageManager.FEATURE_DEVICE_ADMIN)
    @Test
    @EnsureHasPermission(MANAGE_PROFILE_AND_DEVICE_OWNERS)
    public void newlyProvisionedFullyManagedDevice_canControlSensorPermissionGrantsByDefault()
            throws Exception {
        try {
            FullyManagedDeviceProvisioningParams params =
                    createDefaultManagedDeviceProvisioningParamsBuilder().build();
            resetUserSetupCompletedFlag();
            sDevicePolicyManager.provisionFullyManagedDevice(params);

            assertThat(sDevicePolicyManager.canAdminGrantSensorsPermissions()).isTrue();
        } finally {
            sDevicePolicyManager.forceRemoveActiveAdmin(
                    DEVICE_ADMIN_COMPONENT_NAME, sContext.getUserId());
            setUserSetupCompletedFlag();
        }
    }

    @RequireRunOnPrimaryUser
    @RequireFeature(PackageManager.FEATURE_DEVICE_ADMIN)
    @Test
    @EnsureHasPermission(MANAGE_PROFILE_AND_DEVICE_OWNERS)
    public void newlyProvisionedFullyManagedDevice_canOptOutOfControllingSensorPermissionGrants()
            throws Exception {
        try {
            FullyManagedDeviceProvisioningParams params =
                    createDefaultManagedDeviceProvisioningParamsBuilder()
                            .setDeviceOwnerCanGrantSensorsPermissions(false)
                            .build();
            resetUserSetupCompletedFlag();
            sDevicePolicyManager.provisionFullyManagedDevice(params);

            assertThat(sDevicePolicyManager.canAdminGrantSensorsPermissions()).isFalse();
        } finally {
            sDevicePolicyManager.forceRemoveActiveAdmin(
                    DEVICE_ADMIN_COMPONENT_NAME, sContext.getUserId());
            setUserSetupCompletedFlag();
        }
    }

    @RequireRunOnPrimaryUser
    @RequireFeature(PackageManager.FEATURE_DEVICE_ADMIN)
    @Test
    @Postsubmit(reason="new test")
    @EnsureHasPermission(MANAGE_PROFILE_AND_DEVICE_OWNERS)
    public void newlyProvisionedFullyManagedDevice_leavesAllSystemAppsEnabledWhenRequested()
            throws Exception {
        try {
            FullyManagedDeviceProvisioningParams params =
                    createDefaultManagedDeviceProvisioningParamsBuilder()
                            .setLeaveAllSystemAppsEnabled(true)
                            .build();
            resetUserSetupCompletedFlag();
            sDevicePolicyManager.provisionFullyManagedDevice(params);
            Set<String> systemAppsBeforeProvisioning = findSystemApps();

            Set<String> systemAppsAfterProvisioning = findSystemApps();
            assertThat(systemAppsAfterProvisioning).isEqualTo(systemAppsBeforeProvisioning);
        } finally {
            sDevicePolicyManager.forceRemoveActiveAdmin(
                    DEVICE_ADMIN_COMPONENT_NAME, sContext.getUserId());
            setUserSetupCompletedFlag();
        }
    }


    @RequireDoesNotHaveFeature(PackageManager.FEATURE_AUTOMOTIVE)
    @EnsureHasPermission(MANAGE_DEVICE_ADMINS)
    @Test
    public void getPolicyExemptAppsCanOnlyBeDefinedOnAutomotiveBuilds() throws Exception {
        assertWithMessage("list of policy-exempt apps")
                .that(sDevicePolicyManager.getPolicyExemptApps())
                .isEmpty();
    }

    FullyManagedDeviceProvisioningParams.Builder
            createDefaultManagedDeviceProvisioningParamsBuilder() {
        return new FullyManagedDeviceProvisioningParams.Builder(
                DEVICE_ADMIN_COMPONENT_NAME,
                DEVICE_OWNER_NAME)
                // Don't remove system apps during provisioning until the testing
                // infrastructure supports restoring uninstalled apps.
                .setLeaveAllSystemAppsEnabled(true);
    }

    private void resetUserSetupCompletedFlag() {
        try (PermissionContext p = sTestApis.permissions().withPermission(WRITE_SECURE_SETTINGS)) {
            Settings.Secure.putInt(sContext.getContentResolver(), USER_SETUP_COMPLETE_KEY, 0);
        }
        sDevicePolicyManager.forceUpdateUserSetupComplete(sContext.getUserId());
    }

    private void setUserSetupCompletedFlag() {
        try (PermissionContext p = sTestApis.permissions().withPermission(WRITE_SECURE_SETTINGS)) {
            Settings.Secure.putInt(sContext.getContentResolver(), USER_SETUP_COMPLETE_KEY, 1);
        }
        sDevicePolicyManager.forceUpdateUserSetupComplete(sContext.getUserId());
    }

    private Set<String> findSystemApps() {
        return sPackageManager.getInstalledApplications(PackageManager.MATCH_SYSTEM_ONLY)
                .stream()
                .map(applicationInfo -> applicationInfo.packageName)
                .collect(Collectors.toSet());
    }

    // TODO(b/175380793): Add remaining cts test for DPM#provisionManagedDevice and
    //  DPM#createAndProvisionManagedProfile.
    //  Currently the following methods are not used.
    /**
     * Allows {@link #restorePreProvisioningApps} to be called to restore the pre-provisioning apps
     * that were uninstalled during provisioning.
     */
    private void persistPreProvisioningApps() {
        SystemUtil.runShellCommand(SET_DEVICE_OWNER_ACTIVE_ADMIN_COMMAND);
        SystemUtil.runShellCommand(SET_DEVICE_OWNER_COMMAND);

        Set<String> systemApps = findSystemApps();
        sSharedPreferences.edit()
                .putStringSet(KEY_PRE_PROVISIONING_SYSTEM_APPS, systemApps)
                .commit();
        Set<String> nonSystemApps = findNonSystemApps(systemApps);
        sSharedPreferences.edit()
                .putStringSet(KEY_PRE_PROVISIONING_NON_SYSTEM_APPS, nonSystemApps)
                .commit();
        sDevicePolicyManager.setKeepUninstalledPackages(
                DEVICE_ADMIN_COMPONENT_NAME, new ArrayList<>(nonSystemApps));

        SystemUtil.runShellCommand(REMOVE_ACTIVE_ADMIN_COMMAND);
    }

    /**
     * Restores apps that were uninstalled prior to provisioning. No-op if {@link
     * #persistPreProvisioningApps()} was not called prior to provisioning. Subsequent
     * calls will need another prior call to {@link #persistPreProvisioningApps()} to avoid being a
     * no-op.
     */
    public void restorePreProvisioningApps() {
        SystemUtil.runShellCommand(SET_DEVICE_OWNER_ACTIVE_ADMIN_COMMAND);
        SystemUtil.runShellCommand(SET_DEVICE_OWNER_COMMAND);

        Set<String> postProvisioningSystemApps = findSystemApps();
        restorePreProvisioningSystemApps(postProvisioningSystemApps);
        restorePreProvisioningNonSystemApps(postProvisioningSystemApps);
        sSharedPreferences.edit().clear().commit();
        sDevicePolicyManager.setKeepUninstalledPackages(
                DEVICE_ADMIN_COMPONENT_NAME, new ArrayList<>());

        SystemUtil.runShellCommand(REMOVE_ACTIVE_ADMIN_COMMAND);
    }

    private void restorePreProvisioningSystemApps(Set<String> postProvisioningSystemApps) {
        Set<String> preProvisioningSystemApps = sSharedPreferences.getStringSet(
                KEY_PRE_PROVISIONING_SYSTEM_APPS, Collections.emptySet());
        for (String preProvisioningSystemApp : preProvisioningSystemApps) {
            if (postProvisioningSystemApps.contains(preProvisioningSystemApp)) {
                continue;
            }
            sDevicePolicyManager.enableSystemApp(
                    DEVICE_ADMIN_COMPONENT_NAME, preProvisioningSystemApp);
        }
    }

    private void restorePreProvisioningNonSystemApps(Set<String> postProvisioningSystemApps) {
        Set<String> preProvisioningNonSystemApps = sSharedPreferences.getStringSet(
                KEY_PRE_PROVISIONING_NON_SYSTEM_APPS, Collections.emptySet());
        Set<String> postProvisioningNonSystemApps = findNonSystemApps(postProvisioningSystemApps);
        for (String preProvisioningNonSystemApp : preProvisioningNonSystemApps) {
            if (postProvisioningNonSystemApps.contains(preProvisioningNonSystemApp)) {
                continue;
            }
            sDevicePolicyManager.installExistingPackage(
                    DEVICE_ADMIN_COMPONENT_NAME, preProvisioningNonSystemApp);
        }
    }

    private Set<String> findNonSystemApps(Set<String> systemApps) {
        return sPackageManager.getInstalledApplications(PackageManager.MATCH_ALL)
                .stream()
                .map(applicationInfo -> applicationInfo.packageName)
                .filter(packageName -> !systemApps.contains(packageName))
                .collect(Collectors.toSet());
    }
}
