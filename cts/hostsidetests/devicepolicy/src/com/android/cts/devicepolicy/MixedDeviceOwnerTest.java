/*
 * Copyright (C) 2015 The Android Open Source Project
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

package com.android.cts.devicepolicy;

import static com.android.cts.devicepolicy.metrics.DevicePolicyEventLogVerifier.assertMetricsLogged;

import static org.junit.Assert.fail;

import android.platform.test.annotations.FlakyTest;
import android.platform.test.annotations.LargeTest;
import android.stats.devicepolicy.EventId;

import com.android.cts.devicepolicy.DeviceAdminFeaturesCheckerRule.IgnoreOnHeadlessSystemUserMode;
import com.android.cts.devicepolicy.metrics.DevicePolicyEventWrapper;
import com.android.tradefed.device.DeviceNotAvailableException;
import com.android.tradefed.log.LogUtil.CLog;

import com.google.common.collect.ImmutableMap;

import org.junit.Ignore;
import org.junit.Test;

import java.util.ArrayList;
import java.util.Collections;
import java.util.HashMap;
import java.util.List;
import java.util.Map;

/**
 * Set of tests for device owner use cases that also apply to profile owners.
 * Tests that should be run identically in both cases are added in DeviceAndProfileOwnerTest.
 */
public final class MixedDeviceOwnerTest extends DeviceAndProfileOwnerTest {

    private static final String DELEGATION_NETWORK_LOGGING = "delegation-network-logging";
    private static final String LOG_TAG_DEVICE_OWNER = "device-owner";

    private static final String ARG_SECURITY_LOGGING_BATCH_NUMBER = "batchNumber";
    private static final int SECURITY_EVENTS_BATCH_SIZE = 100;

    private boolean mDeviceOwnerSet;

    @Override
    public void setUp() throws Exception {
        super.setUp();

        mUserId = mPrimaryUserId;

        CLog.i("%s.setUp(): mUserId=%d, mPrimaryUserId=%d, mInitialUserId=%d, "
                + "mDeviceOwnerUserId=%d", getClass(), mUserId, mPrimaryUserId, mInitialUserId,
                mDeviceOwnerUserId);

        installDeviceOwnerApp(DEVICE_ADMIN_APK);
        mDeviceOwnerSet = setDeviceOwner(DEVICE_ADMIN_COMPONENT_FLATTENED, mDeviceOwnerUserId,
                /*expectFailure= */ false);

        if (!mDeviceOwnerSet) {
            removeDeviceOwnerAdmin(DEVICE_ADMIN_COMPONENT_FLATTENED);
            getDevice().uninstallPackage(DEVICE_ADMIN_PKG);
            fail("Failed to set device owner on user " + mDeviceOwnerUserId);
        }
        if (isHeadlessSystemUserMode()) {
            affiliateUsers(DEVICE_ADMIN_PKG, mDeviceOwnerUserId, mPrimaryUserId);
        }
    }

    @Override
    public void tearDown() throws Exception {
        if (mDeviceOwnerSet) {
            removeDeviceOwnerAdmin(DEVICE_ADMIN_COMPONENT_FLATTENED);
        }

        super.tearDown();
    }

    @Test
    public void testLockTask_unaffiliatedUser() throws Exception {
        assumeCanCreateAdditionalUsers(1);

        final int userId = createSecondaryUserAsProfileOwner();
        runDeviceTestsAsUser(
                DEVICE_ADMIN_PKG,
                ".AffiliationTest",
                "testLockTaskMethodsThrowExceptionIfUnaffiliated",
                userId);

        setUserAsAffiliatedUserToPrimary(userId);
        runDeviceTestsAsUser(
                DEVICE_ADMIN_PKG,
                ".AffiliationTest",
                "testSetLockTaskPackagesClearedIfUserBecomesUnaffiliated",
                userId);
    }

    @Test
    public void testDelegatedCertInstallerDeviceIdAttestation() throws Exception {
        setUpDelegatedCertInstallerAndRunTests(() ->
                runDeviceTestsAsUser("com.android.cts.certinstaller",
                        ".DelegatedDeviceIdAttestationTest",
                        "testGenerateKeyPairWithDeviceIdAttestationExpectingSuccess", mUserId));
    }

    @FlakyTest
    @Override
    @Test
    public void testCaCertManagement() throws Exception {
        super.testCaCertManagement();
    }

    @FlakyTest(bugId = 141161038)
    @Override
    @Test
    public void testCannotRemoveUserIfRestrictionSet() throws Exception {
        super.testCannotRemoveUserIfRestrictionSet();
    }

    @FlakyTest
    @Override
    @Test
    public void testInstallCaCertLogged() throws Exception {
        super.testInstallCaCertLogged();
    }

    @FlakyTest(bugId = 137088260)
    @Test
    public void testWifi() throws Exception {
        assumeHasWifiFeature();

        executeDeviceTestMethod(".WifiTest", "testGetWifiMacAddress");
        assertMetricsLogged(getDevice(), () -> {
            executeDeviceTestMethod(".WifiTest", "testGetWifiMacAddress");
        }, new DevicePolicyEventWrapper.Builder(EventId.GET_WIFI_MAC_ADDRESS_VALUE)
                .setAdminPackageName(DEVICE_ADMIN_PKG)
                .build());
    }

    @Test
    public void testAdminConfiguredNetworks() throws Exception {
        executeDeviceTestClass(".AdminConfiguredNetworksTest");
    }

    @Override
    @Test
    @IgnoreOnHeadlessSystemUserMode(
            reason = "Per-user application restriction is not applicable for headless user")
    public void testApplicationRestrictions() throws Exception {
        super.testApplicationRestrictions();
    }

    @Test
    public void testSetTime() throws Exception {
        assertMetricsLogged(getDevice(), () -> {
            executeDeviceTestMethod(".TimeManagementTest", "testSetTime");
        }, new DevicePolicyEventWrapper.Builder(EventId.SET_TIME_VALUE)
                .setAdminPackageName(DEVICE_ADMIN_PKG)
                .build());

        executeDeviceTestMethod(".TimeManagementTest", "testSetTime_failWhenAutoTimeEnabled");
    }

    @Test
    public void testSetTimeZone() throws Exception {
        assertMetricsLogged(getDevice(), () -> {
            executeDeviceTestMethod(".TimeManagementTest", "testSetTimeZone");
        }, new DevicePolicyEventWrapper.Builder(EventId.SET_TIME_ZONE_VALUE)
                .setAdminPackageName(DEVICE_ADMIN_PKG)
                .build());

        executeDeviceTestMethod(".TimeManagementTest", "testSetTimeZone_failIfAutoTimeZoneEnabled");
    }

    Map<String, DevicePolicyEventWrapper[]> getAdditionalDelegationTests() {
        final Map<String, DevicePolicyEventWrapper[]> result = new HashMap<>();
        DevicePolicyEventWrapper[] expectedMetrics = new DevicePolicyEventWrapper[] {
                new DevicePolicyEventWrapper.Builder(EventId.SET_NETWORK_LOGGING_ENABLED_VALUE)
                        .setAdminPackageName(DELEGATE_APP_PKG)
                        .setBoolean(true)
                        .setInt(1)
                        .setStrings(LOG_TAG_DEVICE_OWNER)
                        .build(),
                new DevicePolicyEventWrapper.Builder(EventId.RETRIEVE_NETWORK_LOGS_VALUE)
                        .setAdminPackageName(DELEGATE_APP_PKG)
                        .setBoolean(true)
                        .setStrings(LOG_TAG_DEVICE_OWNER)
                        .build(),
                new DevicePolicyEventWrapper.Builder(EventId.SET_NETWORK_LOGGING_ENABLED_VALUE)
                        .setAdminPackageName(DELEGATE_APP_PKG)
                        .setBoolean(true)
                        .setInt(0)
                        .setStrings(LOG_TAG_DEVICE_OWNER)
                        .build(),
        };
        result.put(".NetworkLoggingDelegateTest", expectedMetrics);
        return result;
    }

    @Override
    List<String> getAdditionalDelegationScopes() {
        final List<String> result = new ArrayList<>();
        result.add(DELEGATION_NETWORK_LOGGING);
        return result;
    }

    @Test
    public void testLockScreenInfo() throws Exception {
        executeDeviceTestClass(".LockScreenInfoTest");

        assertMetricsLogged(getDevice(), () -> {
            executeDeviceTestMethod(".LockScreenInfoTest", "testSetAndGetLockInfo");
        }, new DevicePolicyEventWrapper.Builder(EventId.SET_DEVICE_OWNER_LOCK_SCREEN_INFO_VALUE)
                .setAdminPackageName(DEVICE_ADMIN_PKG)
                .build());
    }

    @Test
    public void testFactoryResetProtectionPolicy() throws Exception {
        try {
            executeDeviceTestMethod(".DeviceFeatureUtils", "testHasFactoryResetProtectionPolicy");
        } catch (AssertionError e) {
            // Unable to continue running tests because factory reset protection policy is not
            // supported on the device
            return;
        } catch (Exception e) {
            // Also skip test in case of other exceptions
            return;
        }

        assertMetricsLogged(getDevice(), () -> {
            executeDeviceTestClass(".FactoryResetProtectionPolicyTest");
        }, new DevicePolicyEventWrapper.Builder(EventId.SET_FACTORY_RESET_PROTECTION_VALUE)
                .setAdminPackageName(DEVICE_ADMIN_PKG)
                .build());
    }

    @Test
    public void testCommonCriteriaMode() throws Exception {
        executeDeviceTestClass(".CommonCriteriaModeTest");
    }

    @LargeTest
    @Test
    @Ignore("b/145932189")
    public void testSystemUpdatePolicy() throws Exception {
        executeDeviceTestClass(".systemupdate.SystemUpdatePolicyTest");
    }

    @Test
    public void testInstallUpdate() throws Exception {
        pushUpdateFileToDevice("notZip.zi");
        pushUpdateFileToDevice("empty.zip");
        pushUpdateFileToDevice("wrongPayload.zip");
        pushUpdateFileToDevice("wrongHash.zip");
        pushUpdateFileToDevice("wrongSize.zip");

        executeInstallUpdateTest(/* testName= */ null);
    }

    @Test
    public void testInstallUpdateLogged() throws Exception {
        assumeIsDeviceAb();

        pushUpdateFileToDevice("wrongHash.zip");
        assertMetricsLogged(getDevice(), () -> {
            executeInstallUpdateTest("testInstallUpdate_failWrongHash");
        }, new DevicePolicyEventWrapper.Builder(EventId.INSTALL_SYSTEM_UPDATE_VALUE)
                    .setAdminPackageName(DEVICE_ADMIN_PKG)
                    .setBoolean(/* isDeviceAb */ true)
                    .build(),
            new DevicePolicyEventWrapper.Builder(EventId.INSTALL_SYSTEM_UPDATE_ERROR_VALUE)
                    .setInt(UPDATE_ERROR_UPDATE_FILE_INVALID)
                    .build());
    }

    private void executeInstallUpdateTest(String testName) throws Exception {
        // This test must run on system user as it calls installSystemUpdate(), which takes a
        // Runnable callback (InstallSystemUpdateCallback) and hence it cannot be easily passed
        // around through IPC (on headless system user mode).
        executeDeviceTestMethodOnDeviceOwnerUser(".systemupdate.InstallUpdateTest", testName);
    }

    @Test
    public void testSecurityLoggingWithSingleUser() throws Exception {
        // Backup stay awake setting because testGenerateLogs() will turn it off.
        final String stayAwake = getDevice().getSetting("global", "stay_on_while_plugged_in");
        try {
            // Turn logging on.
            executeDeviceTestMethod(".SecurityLoggingTest", "testEnablingSecurityLogging");
            // Reboot to ensure ro.device_owner is set to true in logd and logging is on.
            rebootAndWaitUntilReady();
            waitForUserUnlock(mUserId);

            // Generate various types of events on device side and check that they are logged.
            executeDeviceTestMethod(".SecurityLoggingTest", "testGenerateLogs");
            getDevice().executeShellCommand("whoami"); // Generate adb command securty event
            executeDeviceTestMethod(".SecurityLoggingTest", "testVerifyGeneratedLogs");

            // Reboot the device, so the security event ids are reset.
            rebootAndWaitUntilReady();

            // Verify event ids are consistent across a consecutive batch.
            for (int batchNumber = 0; batchNumber < 3; batchNumber++) {
                generateTestSecurityLogs();
                executeDeviceTestMethod(".SecurityLoggingTest", "testVerifyLogIds",
                        Collections.singletonMap(ARG_SECURITY_LOGGING_BATCH_NUMBER,
                                Integer.toString(batchNumber)));
            }

            // Immediately attempting to fetch events again should fail.
            executeDeviceTestMethod(".SecurityLoggingTest",
                    "testSecurityLoggingRetrievalRateLimited");
        } finally {
            // Turn logging off.
            executeDeviceTestMethod(".SecurityLoggingTest", "testDisablingSecurityLogging");
            // Restore stay awake setting.
            if (stayAwake != null) {
                getDevice().setSetting("global", "stay_on_while_plugged_in", stayAwake);
            }
        }
    }

    @Test
    public void testSecurityLoggingEnabledLogged() throws Exception {
        assertMetricsLogged(getDevice(), () -> {
            executeDeviceTestMethod(".SecurityLoggingTest", "testEnablingSecurityLogging");
            executeDeviceTestMethod(".SecurityLoggingTest", "testDisablingSecurityLogging");
        }, new DevicePolicyEventWrapper.Builder(EventId.SET_SECURITY_LOGGING_ENABLED_VALUE)
                .setAdminPackageName(DEVICE_ADMIN_PKG)
                .setBoolean(true)
                .build(),
            new DevicePolicyEventWrapper.Builder(EventId.SET_SECURITY_LOGGING_ENABLED_VALUE)
                    .setAdminPackageName(DEVICE_ADMIN_PKG)
                    .setBoolean(false)
                    .build());
    }

    @Test
    public void testSecurityLoggingWithTwoUsers() throws Exception {
        assumeCanCreateAdditionalUsers(1);

        final int userId = createUser();
        try {
            // The feature can be enabled, but in a "paused" state. Attempting to retrieve logs
            // should throw security exception.
            executeDeviceTestMethod(".SecurityLoggingTest", "testEnablingSecurityLogging");
            executeDeviceTestMethod(".SecurityLoggingTest",
                    "testRetrievingSecurityLogsThrowsSecurityException");
            executeDeviceTestMethod(".SecurityLoggingTest",
                    "testRetrievingPreviousSecurityLogsThrowsSecurityException");
        } finally {
            removeUser(userId);
            executeDeviceTestMethod(".SecurityLoggingTest", "testDisablingSecurityLogging");
        }
    }

    @Test
    public void testSecurityLoggingDelegate() throws Exception {
        installAppAsUser(DELEGATE_APP_APK, mDeviceOwnerUserId);
        try {
            // Test that the delegate cannot access the logs already
            runDeviceTestsAsUser(DELEGATE_APP_PKG, ".SecurityLoggingDelegateTest",
                    "testCannotAccessApis", mDeviceOwnerUserId);

            // Set security logging delegate
            executeDeviceTestMethodOnDeviceOwnerUser(".SecurityLoggingTest",
                    "testSetDelegateScope_delegationSecurityLogging");

            runSecurityLoggingTests(DELEGATE_APP_PKG, ".SecurityLoggingDelegateTest");
        } finally {
            // Remove security logging delegate
            executeDeviceTestMethodOnDeviceOwnerUser(".SecurityLoggingTest",
                    "testSetDelegateScope_noDelegation");
        }
    }

    /**
     * Test for {@link DevicePolicyManager.setStorageEncryption} and
     * {@link DevicePolicyManager.getStorageEncryption}.
     *
     * <p>This test needs to run as as the device owner user ID since
     * {@link DevicePolicyManager#setStorageEncryption(ComponentName, boolean)}
     * is only allowed for system user.
     */
    @Override
    @Test
    public void testSetStorageEncryption() throws Exception {
        Map<String, String> params =
                ImmutableMap.of(IS_SYSTEM_USER_PARAM, String.valueOf(/* isSystemUser= */ true));
        runDeviceTestsAsUser(
                DEVICE_ADMIN_PKG, STORAGE_ENCRYPTION_TEST_CLASS, null, mDeviceOwnerUserId, params);
    }

    private void runSecurityLoggingTests(String packageName, String testClassName)
            throws Exception {
        int userId = mDeviceOwnerUserId;
        try {
            // Turn logging on.
            runDeviceTestsAsUser(packageName, testClassName, "testEnablingSecurityLogging", userId);
            // Reboot to ensure ro.organization_owned is set to true in logd and logging is on.
            rebootAndWaitUntilReady();
            waitForUserUnlock(userId);

            // Generate various types of events on device side and check that they are logged.
            runDeviceTestsAsUser(packageName, testClassName, "testGenerateLogs", userId);
            runDeviceTestsAsUser(packageName, testClassName, "testVerifyGeneratedLogs", userId);

            // Immediately attempting to fetch events again should fail.
            runDeviceTestsAsUser(packageName, testClassName,
                    "testSecurityLoggingRetrievalRateLimited", userId);
        } finally {
            // Turn logging off.
            runDeviceTestsAsUser(packageName, testClassName,
                    "testDisablingSecurityLogging", userId);
        }
    }

    @Test
    public void testLocationPermissionGrantNotifies() throws Exception {
        installAppPermissionAppAsUser();
        configureNotificationListener();
        executeDeviceTestMethod(".PermissionsTest",
                "testPermissionGrantStateGranted_userNotifiedOfLocationPermission");
    }

    @Override
    @Test
    public void testAdminControlOverSensorPermissionGrantsDefault() throws Exception {
        // In Device Owner mode, by default, admin should be able to grant sensors-related
        // permissions.
        executeDeviceTestMethod(".SensorPermissionGrantTest",
                "testAdminCanGrantSensorsPermissions");
    }

    @Override
    @Test
    public void testGrantOfSensorsRelatedPermissions() throws Exception {
        // Skip for now, re-enable when the code path sets DO as able to grant permissions.
    }

    @Override
    @Test
    public void testSensorsRelatedPermissionsNotGrantedViaPolicy() throws Exception {
        // Skip for now, re-enable when the code path sets DO as able to grant permissions.
    }

    @Override
    @Test
    public void testStateOfSensorsRelatedPermissionsCannotBeRead() throws Exception {
        // Skip because in DO mode the admin can read permission state.
    }

    //TODO(b/180413140) Investigate why the test fails on DO mode.
    @Override
    @Test
    public void testPermissionPrompts() throws Exception {
    }

    @Override
    @Test
    @IgnoreOnHeadlessSystemUserMode(reason = "Headless system user doesn't launch activities")
    public void testSuspendPackage() throws Exception {
        super.testSuspendPackage();
    }

    @Override
    @Test
    @IgnoreOnHeadlessSystemUserMode(reason = "Headless system user doesn't launch activities")
    public void testSuspendPackageWithPackageManager() throws Exception {
        super.testSuspendPackageWithPackageManager();
    }

    @Override
    @Test
    @IgnoreOnHeadlessSystemUserMode(reason = "Headless system user doesn't have credentials")
    public void testGetPasswordExpiration() throws Exception {
        super.testGetPasswordExpiration();
    }

    @Override
    @Test
    @IgnoreOnHeadlessSystemUserMode(reason = "Headless system user doesn't have credentials")
    public void testPasswordExpiration() throws Exception {
        super.testPasswordExpiration();
    }

    @Override
    @Test
    @IgnoreOnHeadlessSystemUserMode(reason = "Headless system user doesn't have credentials")
    public void testResetPasswordWithToken() throws Exception {
        super.testResetPasswordWithToken();
    }

    @Override
    @Test
    @IgnoreOnHeadlessSystemUserMode(reason = "Headless system user doesn't have credentials")
    public void testResetPasswordDeprecated() throws Exception {
        super.testResetPasswordDeprecated();
    }

    @Override
    @Test
    @IgnoreOnHeadlessSystemUserMode(reason = "Headless system user doesn't launch activities")
    public void testCreateAdminSupportIntent() throws Exception {
        super.testCreateAdminSupportIntent();
    }

    @Override
    @Test
    @IgnoreOnHeadlessSystemUserMode(reason = "Headless system user doesn't launch activities")
    public void testPersistentIntentResolving() throws Exception {
        super.testPersistentIntentResolving();
    }

    @Override
    @Test
    @IgnoreOnHeadlessSystemUserMode(reason = "Headless system user doesn't launch activities")
    public void testDisallowAutofill_allowed() throws Exception {
        super.testDisallowAutofill_allowed();
    }

    @Override
    public void testApplicationHidden() throws Exception {
        if (isHeadlessSystemUserMode()) {
            // Must run on user 0 because the test has a broadcast receiver that listen to packages
            // added / removed intents
            mUserId = mDeviceOwnerUserId;
            CLog.d("testApplicationHidden(): setting mUserId as %d before running it", mUserId);
        }
        super.testApplicationHidden();
    }

    @Override
    protected void installDelegateApp() throws Exception {
        // TODO(b/176993670): must call installDeviceOwnerApp() - even though it's not one - so
        // the permissions required to use DpmWrapper are set on headless system user mode
        installDeviceOwnerApp(DELEGATE_APP_APK);
    }

    @Override
    protected void runDeviceTestsAsUser(String pkgName, String testClassName, String testName,
            int userId, Map<String, String> params) throws DeviceNotAvailableException {
        Map<String, String> newParams = new HashMap(params);
        newParams.putAll(getParamsForDeviceOwnerTest());
        super.runDeviceTestsAsUser(
                pkgName, testClassName, testName, userId, newParams);
    }

    @Override
    protected void executeDeviceTestMethod(String className, String testName,
            Map<String, String> params) throws Exception {
        runDeviceTestsAsUser(DEVICE_ADMIN_PKG, className, testName, mUserId, params);
    }

    private void executeDeviceTestMethodOnDeviceOwnerUser(String className, String testName)
            throws Exception {
        executeDeviceTestMethod(className, testName, mDeviceOwnerUserId,
                /* params= */ new HashMap<>());
    }

    private void configureNotificationListener() throws DeviceNotAvailableException {
        getDevice().executeShellCommand("cmd notification allow_listener "
                + "com.android.cts.deviceandprofileowner/.NotificationListener");
    }

    private void generateTestSecurityLogs() throws Exception {
        // Trigger security events of type TAG_ADB_SHELL_CMD.
        for (int i = 0; i < SECURITY_EVENTS_BATCH_SIZE; i++) {
            getDevice().executeShellCommand("echo just_testing_" + i);
        }
    }

    private int createSecondaryUserAsProfileOwner() throws Exception {
        final int userId = createUserAndWaitStart();
        installAppAsUser(INTENT_RECEIVER_APK, userId);
        installAppAsUser(DEVICE_ADMIN_APK, userId);
        // For headless system user mode, PO is set on any secondary user created
        if (!isHeadlessSystemUserMode()) {
            setProfileOwnerOrFail(DEVICE_ADMIN_COMPONENT_FLATTENED, userId);
        }
        return userId;
    }

    private void switchToUser(int userId) throws Exception {
        switchUser(userId);
        waitForBroadcastIdle();
        wakeupAndDismissKeyguard();
    }

    private void setUserAsAffiliatedUserToPrimary(int userId) throws Exception {
        // Setting the same affiliation ids on both users
        runDeviceTestsAsUser(
                DEVICE_ADMIN_PKG, ".AffiliationTest", "testSetAffiliationId1", mPrimaryUserId);
        runDeviceTestsAsUser(
                DEVICE_ADMIN_PKG, ".AffiliationTest", "testSetAffiliationId1", userId);
    }
}
