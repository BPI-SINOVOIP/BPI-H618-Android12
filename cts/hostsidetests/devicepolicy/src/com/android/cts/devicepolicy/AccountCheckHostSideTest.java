/*
 * Copyright (C) 2016 The Android Open Source Project
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

import static org.junit.Assert.assertTrue;
import static org.junit.Assert.fail;
import static org.junit.Assume.assumeTrue;

import android.platform.test.annotations.LargeTest;

import com.android.tradefed.log.LogUtil.CLog;

import org.junit.Test;

import java.util.regex.Matcher;
import java.util.regex.Pattern;

/**
 * Set of tests to test setting DO and PO when there is account on the user.
 *
 * <p>For example, setting DO or PO shall fail at DPMS.hasIncompatibleAccountsOrNonAdbNoLock when
 * there was incompatible account on the user.
 */
public class AccountCheckHostSideTest extends BaseDevicePolicyTest {
    private static final String APK_NON_TEST_ONLY = "CtsAccountCheckNonTestOnlyOwnerApp.apk";
    private static final String APK_TEST_ONLY = "CtsAccountCheckTestOnlyOwnerApp.apk";
    private static final String APK_TEST_ONLY_UPDATE = "CtsAccountCheckTestOnlyOwnerUpdateApp.apk";
    private static final String APK_AUTH = "CtsAccountCheckAuthApp.apk";

    private static final String PACKAGE_NON_TEST_ONLY =
            "com.android.cts.devicepolicy.accountcheck.nontestonly";
    private static final String PACKAGE_TEST_ONLY =
            "com.android.cts.devicepolicy.accountcheck.testonly";
    private static final String PACKAGE_AUTH = "com.android.cts.devicepolicy.accountcheck.auth";

    private static final String OWNER_TEST_ONLY = PACKAGE_TEST_ONLY
            + "/com.android.cts.devicepolicy.accountcheck.owner.AdminReceiver";
    private static final String OWNER_NON_TEST_ONLY = PACKAGE_NON_TEST_ONLY
            + "/com.android.cts.devicepolicy.accountcheck.owner.AdminReceiver";

    private static final String TEST_CLASS =
            "com.android.cts.devicepolicy.accountcheck.AccountCheckTest";

    private static final String DISALLOW_MODIFY_ACCOUNTS = "no_modify_accounts";

    private boolean mDeviceOwnerCanHaveAccounts;
    private boolean mProfileOwnerCanHaveAccounts;
    private int mProfileOwnerUserId;

    @Override
    public void setUp() throws Exception {
        super.setUp();
        mProfileOwnerUserId = mPrimaryUserId;
        mDeviceOwnerCanHaveAccounts = !isRestrictionSetOnUser(mDeviceOwnerUserId,
                DISALLOW_MODIFY_ACCOUNTS);
        // Optimization to avoid running dumpsys again
        if (mProfileOwnerUserId == mDeviceOwnerUserId) {
            mProfileOwnerCanHaveAccounts = mDeviceOwnerCanHaveAccounts;
        } else {
            mProfileOwnerCanHaveAccounts = !isRestrictionSetOnUser(mProfileOwnerUserId,
                    DISALLOW_MODIFY_ACCOUNTS);
        }
        CLog.d("mDeviceOwnerUserId: " +  mDeviceOwnerUserId
                + " mDeviceOwnerCanHaveAccounts: " + mDeviceOwnerCanHaveAccounts
                + " mProfileOwnerUserId: " + mProfileOwnerUserId
                + " mProfileOwnerCanHaveAccounts: " + mProfileOwnerCanHaveAccounts);
        assumeTrue("Neither primary user or device owner user is allowed to add accounts",
                mDeviceOwnerCanHaveAccounts || mProfileOwnerCanHaveAccounts);
    }

    @Override
    public void tearDown() throws Exception {
        if (getDevice().getInstalledPackageNames().contains(PACKAGE_AUTH)) {
            runCleanupTestOnlyOwnerAllowingFailure(mProfileOwnerUserId);
            if (mDeviceOwnerUserId != mProfileOwnerUserId) {
                runCleanupTestOnlyOwnerAllowingFailure(mDeviceOwnerUserId);
            }
            runCleanupNonTestOnlyOwnerAllowingFailure();

            // This shouldn't be needed since we're uninstalling the authenticator,
            // but sometimes the account manager fails to clean up?
            removeAllAccountsAllowingFailure();
        }

        getDevice().uninstallPackage(PACKAGE_AUTH);
        getDevice().uninstallPackage(PACKAGE_TEST_ONLY);
        getDevice().uninstallPackage(PACKAGE_NON_TEST_ONLY);

        super.tearDown();
    }

    private void runTest(String method) throws Exception {
        runTestAsUser(method, mProfileOwnerUserId);
        if (mDeviceOwnerCanHaveAccounts && mProfileOwnerUserId != mDeviceOwnerUserId) {
            runTestAsUser(method, mDeviceOwnerUserId);
        }
    }

    private void runTestAsUser(String method, int userId) throws Exception {
        runDeviceTestsAsUser(PACKAGE_AUTH, TEST_CLASS, method, userId);
    }

    private void runCleanupTestOnlyOwner(int userId) throws Exception {
        assertTrue(removeAdmin(OWNER_TEST_ONLY, userId));
    }

    private void runCleanupTestOnlyOwnerAllowingFailure(int userId) throws Exception {
        try {
            runCleanupTestOnlyOwner(userId);
        } catch (AssertionError ignore) {
        }
    }

    private void runCleanupNonTestOnlyOwner() throws Exception {
        runTest("testCleanUpNonTestOwner");
    }

    private void runCleanupNonTestOnlyOwnerAllowingFailure() throws Exception {
        try {
            runCleanupNonTestOnlyOwner();
        } catch (AssertionError ignore) {
        }
    }

    private void removeAllAccounts() throws Exception {
        runTest("testRemoveAllAccounts");
    }

    private void removeAllAccountsAllowingFailure() throws Exception {
        try {
            removeAllAccounts();
        } catch (AssertionError ignore) {
        }
    }

    private void assertTestOnlyInstallable() throws Exception {
        if (mDeviceOwnerCanHaveAccounts) {
            setDeviceOwnerOrFail(OWNER_TEST_ONLY, mDeviceOwnerUserId);
            runCleanupTestOnlyOwner(mDeviceOwnerUserId);
        }
        if (mProfileOwnerCanHaveAccounts) {
            setProfileOwnerOrFail(OWNER_TEST_ONLY, mProfileOwnerUserId);
            runCleanupTestOnlyOwner(mProfileOwnerUserId);
        }
    }

    private void assertNonTestOnlyInstallable() throws Exception {
        if (mDeviceOwnerCanHaveAccounts) {
            setDeviceOwnerOrFail(OWNER_NON_TEST_ONLY, mDeviceOwnerUserId);
            runCleanupNonTestOnlyOwner();
        }
        if (mProfileOwnerCanHaveAccounts) {
            setProfileOwnerOrFail(OWNER_NON_TEST_ONLY, mProfileOwnerUserId);
            runCleanupNonTestOnlyOwner();
        }
    }

    private void assertTestOnlyNotInstallable() throws Exception {
        if (mDeviceOwnerCanHaveAccounts) {
            setDeviceOwnerExpectingFailure(OWNER_TEST_ONLY, mDeviceOwnerUserId);
            runCleanupTestOnlyOwnerAllowingFailure(mDeviceOwnerUserId);
        }
        if (mProfileOwnerCanHaveAccounts) {
            setProfileOwnerExpectingFailure(OWNER_TEST_ONLY, mProfileOwnerUserId);
            runCleanupTestOnlyOwnerAllowingFailure(mProfileOwnerUserId);
        }
    }

    private void assertNonTestOnlyNotInstallable() throws Exception {
        if (mDeviceOwnerCanHaveAccounts) {
            setDeviceOwnerExpectingFailure(OWNER_NON_TEST_ONLY, mDeviceOwnerUserId);
            runCleanupNonTestOnlyOwnerAllowingFailure();
        }
        if (mProfileOwnerCanHaveAccounts) {
            setProfileOwnerExpectingFailure(OWNER_NON_TEST_ONLY, mProfileOwnerUserId);
            runCleanupNonTestOnlyOwnerAllowingFailure();
        }
    }

    private boolean hasAccounts() throws Exception {
        final String accountDump = getDevice().executeShellCommand("dumpsys account");

        final Pattern p = Pattern.compile("^\\s*Accounts\\:\\s*(\\d+)", Pattern.MULTILINE);
        final Matcher m = p.matcher(accountDump);
        if (!m.find()) {
            fail("Unable to obtain # of accounts");
            return true;
        }
        final String count = m.group(1);

        CLog.i("# of preconfigured accounts=" + count);

        return Integer.parseInt(count) > 0;
    }

    /**
     * This set of tests will test whether DO and PO can be set on the user when
     * there is/are different types of accounts added on the target test user.
     */
    @Test
    @LargeTest
    public void testAccountCheck() throws Exception {
        installAppAsUser(APK_AUTH, mProfileOwnerUserId);
        installAppAsUser(APK_NON_TEST_ONLY, mProfileOwnerUserId);
        installAppAsUser(APK_TEST_ONLY, mProfileOwnerUserId);
        runCleanupTestOnlyOwnerAllowingFailure(mProfileOwnerUserId);

        // For tests in headless system user mode, test packages need to be installed for
        // system user even for PO tests since PO will be set via adb command which will require
        // TestAuthenticator installed on system user.
        if (mDeviceOwnerUserId != mProfileOwnerUserId) {
            installAppAsUser(APK_AUTH, mDeviceOwnerUserId);
            installAppAsUser(APK_NON_TEST_ONLY, mDeviceOwnerUserId);
            installAppAsUser(APK_TEST_ONLY, mDeviceOwnerUserId);
            runCleanupTestOnlyOwnerAllowingFailure(mDeviceOwnerUserId);
        }

        runCleanupNonTestOnlyOwnerAllowingFailure();
        removeAllAccountsAllowingFailure();
        try {
//            runTest("testCheckPreconfiguredAccountFeatures");
//
//            final boolean hasPreconfiguredAccounts = hasAccounts();
//
//            // All pre-configured accounts must be "compatible", so the test-only owner can be
//            // installed.
//            assertTestOnlyInstallable();
//
//            if (hasPreconfiguredAccounts) {
//                assertNonTestOnlyNotInstallable();
//            } else {
//                assertNonTestOnlyInstallable();
//            }
//
            // Incompatible, type A.
            runTest("testAddIncompatibleA");

            assertTestOnlyNotInstallable();
            assertNonTestOnlyNotInstallable();

            // Incompatible, type B.
            removeAllAccounts();
            runTest("testAddIncompatibleB");

            assertTestOnlyNotInstallable();
            assertNonTestOnlyNotInstallable();

            // Incompatible, type C.
            removeAllAccounts();
            runTest("testAddIncompatibleC");

            assertTestOnlyNotInstallable();
            assertNonTestOnlyNotInstallable();

            // Compatible.
            removeAllAccounts();
            runTest("testAddCompatible");

            assertTestOnlyInstallable(); // Now test-only owner can be accepted.
            assertNonTestOnlyNotInstallable();

            // 2 compatible accounts.
            removeAllAccounts();
            runTest("testAddCompatible");
            runTest("testAddCompatible");

            assertTestOnlyInstallable(); // Now test-only owner can be accepted.

            assertNonTestOnlyNotInstallable();

            // 2 compatible accounts + 1 incompatible.
            removeAllAccounts();
            runTest("testAddIncompatibleA");
            runTest("testAddCompatible");
            runTest("testAddCompatible");

            assertTestOnlyNotInstallable();
            assertNonTestOnlyNotInstallable();

            // 2 compatible accounts + 1 incompatible, different order.
            removeAllAccounts();
            runTest("testAddCompatible");
            runTest("testAddCompatible");
            runTest("testAddIncompatibleB");

            assertTestOnlyNotInstallable();
            assertNonTestOnlyNotInstallable();
        } catch (Throwable th) {
            CLog.w("Tests failed; current accounts are:");
            CLog.w(getDevice().executeShellCommand("dumpsys account"));

            // Dump accounts
            throw th;
        }
    }

    /**
     * Make sure even if the "test-only" flag changes when an app is updated, we still respect
     * the original value.
     */
    @Test
    public void testInheritTestOnly() throws Exception {
        installAppAsUser(APK_TEST_ONLY, mDeviceOwnerUserId);

        // Set as DO.
        try {
            setDeviceOwnerOrFail(OWNER_TEST_ONLY, mDeviceOwnerUserId);
        } catch (Throwable e) {
            CLog.e("Unable to install DO, can't continue the test. Skipping.  hasAccounts="
                    + hasAccounts());
            return;
        }
        try {

            // Override with a package that's not test-only.
            installAppAsUser(APK_TEST_ONLY_UPDATE, mDeviceOwnerUserId);

            // But DPMS keeps the original test-only flag, so it's still removable.
            runCleanupTestOnlyOwner(mDeviceOwnerUserId);

            return;
        } catch (Throwable e) {
            // If failed, re-install the APK with test-only=true.
            try {
                installAppAsUser(APK_TEST_ONLY, mDeviceOwnerUserId);
                runCleanupTestOnlyOwner(mDeviceOwnerUserId);
            } catch (Exception inner) {
                CLog.e("Unable to clean up after a failure: " + e.getMessage());
            }

            throw e;
        }
    }
}
