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
 * limitations under the License.
 */

package android.car.cts;

import static com.android.tradefed.device.NativeDevice.INVALID_USER_ID;

import static com.google.common.truth.Truth.assertWithMessage;

import static org.junit.Assert.fail;

import android.platform.test.annotations.Presubmit;

import com.android.tradefed.log.LogUtil.CLog;
import com.android.tradefed.testtype.DeviceJUnit4ClassRunner;

import org.junit.After;
import org.junit.Before;
import org.junit.Test;
import org.junit.runner.RunWith;

import java.util.ArrayList;
import java.util.List;
import java.util.Map;
import java.util.stream.Collectors;

/**
 * Tests for pre-created users.
 */
@RunWith(DeviceJUnit4ClassRunner.class)
public final class PreCreateUsersHostTest extends CarHostJUnit4TestCase {

    private static int sNumberCreateadUsers;

    @Before
    public void checkMultiUserSupport() throws Exception {
        assumeSupportsMultipleUsers();
    }

    @After
    public void uninstallPackage() throws Exception {
        getDevice().uninstallPackage(APP_PKG);
    }

    /**
     * Makes sure an app installed for a regular user is not visible to a pre-created user.
     */
    @Presubmit
    @Test
    public void testAppsAreNotInstalledOnPreCreatedUser() throws Exception {
        appsAreNotInstalledOnPreCreatedUserTest(/* isGuest= */ false, /* afterReboot= */ false);
    }

    /**
     * Same as {@link #testAppsAreNotInstalledOnPreCreatedUser()}, but for a guest user.
     */
    @Presubmit
    @Test
    public void testAppsAreNotInstalledOnPreCreatedGuest() throws Exception {
        appsAreNotInstalledOnPreCreatedUserTest(/* isGuest= */ true, /* afterReboot= */ false);
    }

    /**
     * Makes sure an app installed for a regular user is not visible to a pre-created user, even
     * after the system restarts
     */
    @Presubmit
    @Test
    public void testAppsAreNotInstalledOnPreCreatedUserAfterReboot() throws Exception {
        appsAreNotInstalledOnPreCreatedUserTest(/* isGuest= */ false, /* afterReboot= */ true);
    }

    /**
     * Same as {@link #testAppsAreNotInstalledOnPreCreatedUserAfterReboot()}, but for a guest
     * user.
     */
    @Presubmit
    @Test
    public void testAppsAreNotInstalledOnPreCreatedGuestAfterReboot() throws Exception {
        appsAreNotInstalledOnPreCreatedUserTest(/* isGuest= */ true, /* afterReboot= */ true);
    }

    private void appsAreNotInstalledOnPreCreatedUserTest(boolean isGuest,
            boolean afterReboot) throws Exception {
        deletePreCreatedUsers();
        requiresExtraUsers(1);

        int initialUserId = getCurrentUserId();
        int preCreatedUserId = preCreateUser(isGuest);

        installPackageAsUser(APP_APK, /* grantPermission= */ false, initialUserId);

        assertAppInstalledForUser(APP_PKG, initialUserId);
        assertAppNotInstalledForUser(APP_PKG, preCreatedUserId);

        if (afterReboot) {
            restartSystem();

            // Checks again
            assertAppInstalledForUser(APP_PKG, initialUserId);
            assertAppNotInstalledForUser(APP_PKG, preCreatedUserId);
        }
        convertPreCreatedUser(isGuest, preCreatedUserId);
        assertAppNotInstalledForUser(APP_PKG, preCreatedUserId);
    }

    /**
     * Verifies a pre-created user have same packages as non-precreated users.
     */
    @Presubmit
    @Test
    public void testAppPermissionsPreCreatedUserPackages() throws Exception {
        appPermissionsPreCreatedUserPackagesTest(/* isGuest= */ false, /* afterReboot= */ false);
    }

    /**
     * Verifies a pre-created guest have same packages as non-precreated users.
     */
    @Presubmit
    @Test
    public void testAppPermissionsPreCreatedGuestPackages() throws Exception {
        appPermissionsPreCreatedUserPackagesTest(/* isGuest= */ true, /* afterReboot= */ false);
    }

    /**
     * Verifies a pre-created user have same packages as non-precreated users.
     */
    @Presubmit
    @Test
    public void testAppPermissionsPreCreatedUserPackagesAfterReboot() throws Exception {
        appPermissionsPreCreatedUserPackagesTest(/* isGuest= */ false, /* afterReboot= */ true);
    }

    /**
     * Verifies a pre-created guest have same packages as non-precreated users.
     */
    @Presubmit
    @Test
    public void testAppPermissionsPreCreatedGuestPackagesAfterReboot() throws Exception {
        appPermissionsPreCreatedUserPackagesTest(/* isGuest= */ true, /* afterReboot= */ true);
    }

    private void appPermissionsPreCreatedUserPackagesTest(boolean isGuest, boolean afterReboot)
            throws Exception {
        deletePreCreatedUsers();
        requiresExtraUsers(2);

        // Create a normal reference user
        int referenceUserId = isGuest
                ? createGuestUser("PreCreatedUsersTest_Reference_Guest")
                : createFullUser("PreCreatedUsersTest_Reference_User");
        // Some permissions (e.g. Role permission) are given only after initialization.
        switchUser(referenceUserId);
        waitUntilUserPermissionsIsReady(referenceUserId);
        Map<String, List<String>> refPkgMap = getPackagesAndPermissionsForUser(referenceUserId);

        // There can be just one guest by default, so remove it now otherwise
        // convertPreCreatedUser() below will fail
        if (isGuest && !afterReboot) {
            removeUser(referenceUserId);
        }

        int preCreatedUserId = preCreateUser(isGuest);

        if (afterReboot) {
            restartSystem();
        }

        convertPreCreatedUser(isGuest, preCreatedUserId);
        // Some permissions (e.g. Role permission) are given only after initialization.
        switchUser(preCreatedUserId);
        waitUntilUserPermissionsIsReady(preCreatedUserId);
        Map<String, List<String>> actualPkgMap = getPackagesAndPermissionsForUser(preCreatedUserId);

        List<String> errors = new ArrayList<>();
        for (String pkg: refPkgMap.keySet()) {
            addError(errors, () ->
                    assertWithMessage("permissions state mismatch for %s", pkg)
                            .that(actualPkgMap.get(pkg))
                            .isEqualTo(refPkgMap.get(pkg)));
        }
        assertWithMessage("found %s error", errors.size()).that(errors).isEmpty();
    }

    private void addError(List<String> error, Runnable r) {
        try {
            r.run();
        } catch (Throwable t) {
            error.add(t.getMessage());
        }
    }

    private void assertHasPreCreatedUser(int userId) throws Exception {
        List<Integer> existingIds = getPreCreatedUsers();
        CLog.d("assertHasPreCreatedUser(%d): pool=%s", userId, existingIds);
        assertWithMessage("pre-created user not found").that(existingIds).contains(userId);
    }

    private List<Integer> getPreCreatedUsers() throws Exception {
        return onAllUsers((allUsers) -> allUsers.stream()
                    .filter((u) -> u.otherState.contains("(pre-created)")
                            && !u.flags.contains("DISABLED"))
                    .map((u) -> u.id).collect(Collectors.toList()));
    }

    private int preCreateUser(boolean isGuest) throws Exception {
        return executeAndParseCommand((output) -> {
            int userId = INVALID_USER_ID;
            if (output.startsWith("Success")) {
                try {
                    userId = Integer.parseInt(output.substring(output.lastIndexOf(" ")).trim());
                    CLog.i("Pre-created user with id %d; waiting until it's initialized", userId);
                    markUserForRemovalAfterTest(userId);
                    waitForUserInitialized(userId);
                    assertHasPreCreatedUser(userId);
                    waitUntilUserDataIsPersisted(userId);
                } catch (Exception e) {
                    CLog.e("Exception pre-creating %s: %s", (isGuest ? "guest" : "user"), e);
                }
            }
            if (userId == INVALID_USER_ID) {
                throw new IllegalStateException("failed to pre-create user");
            }
            return userId;
        }, "pm create-user --pre-create-only%s", (isGuest ? " --guest" : ""));
    }

    // TODO(b/169588446): remove method and callers once it's not needed anymore
    private void waitUntilUserDataIsPersisted(int userId) throws InterruptedException {
        int napTimeSec = 10;
        CLog.i("Sleeping %ds to make sure user data for user %d is persisted", napTimeSec, userId);
        sleep(napTimeSec * 1_000);
    }

    // TODO(b/170263003): update this method after core framewokr's refactoring for proto
    private void waitUntilUserPermissionsIsReady(int userId) throws InterruptedException {
        int napTimeSec = 20;
        CLog.i("Sleeping %ds to make permissions for user %d is ready", napTimeSec, userId);
        sleep(napTimeSec * 1_000);
    }

    private void deletePreCreatedUsers() throws Exception {
        List<Integer> userIds = getPreCreatedUsers();
        for (int userId : userIds) {
            getDevice().removeUser(userId);
        }
    }

    private void convertPreCreatedUser(boolean isGuest, int expectedId) throws Exception {
        assertHasPreCreatedUser(expectedId);
        String type = isGuest ? "guest" : "user";
        int suffix = ++sNumberCreateadUsers;
        int newUserId = isGuest
                ? createGuestUser("PreCreatedUsersTest_ConvertedGuest_" + suffix)
                : createFullUser("PreCreatedUsersTest_ConvertedUser_" + suffix);
        if (newUserId == expectedId) {
            CLog.i("Created new %s from pre-created %s with id %d", type, type, newUserId);
            return;
        }
        fail("Created new " + type + " with id " + newUserId + ", which doesn't match pre-created "
                + "id " + expectedId);
    }

    private void restartSystem() throws Exception {
        // Restart the system to make sure PackageManager preserves the installed bit
        restartSystemServer();
    }
}
