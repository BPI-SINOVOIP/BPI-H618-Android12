/*
 * Copyright (C) 2019 The Android Open Source Project
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
package android.host.multiuser;

import static com.google.common.truth.Truth.assertWithMessage;

import static org.junit.Assert.assertTrue;

import android.platform.test.annotations.LargeTest;
import android.platform.test.annotations.Presubmit;

import com.android.tradefed.testtype.DeviceJUnit4ClassRunner;

import org.junit.Rule;
import org.junit.Test;
import org.junit.runner.RunWith;

/**
 * Test verifies that ephemeral users are removed after switched away and after reboot.
 *
 * Run: atest android.host.multiuser.EphemeralTest
 */
@LargeTest
@RunWith(DeviceJUnit4ClassRunner.class)
public class EphemeralTest extends BaseMultiUserTest {

    private static final String TEST_APP_PKG_NAME = "com.android.cts.multiuser";
    private static final String TEST_APP_PKG_APK = "CtsMultiuserApp.apk";

    @Rule
    public final SupportsMultiUserRule mSupportsMultiUserRule = new SupportsMultiUserRule(this);

    /** Test to verify ephemeral user is removed after switch out to another user. */
    @Presubmit
    @Test
    public void testSwitchAndRemoveEphemeralUser() throws Exception {
        final int ephemeralUserId = createEphemeralUser();

        assertSwitchToNewUser(ephemeralUserId);
        assertSwitchToUser(ephemeralUserId, mInitialUserId);
        waitForUserRemove(ephemeralUserId);
        assertUserNotPresent(ephemeralUserId);
    }

    /** Test to verify ephemeral user is removed after reboot. */
    @Presubmit
    @Test
    public void testRebootAndRemoveEphemeralUser() throws Exception {
        final int ephemeralUserId = createEphemeralUser();

        assertSwitchToNewUser(ephemeralUserId);
        getDevice().reboot();
        assertUserNotPresent(ephemeralUserId);
    }

    /**
     * Test to verify that an ephemeral user, with an account, is safely removed after rebooting
     * from it.
     */
    @Test
    public void testRebootAndRemoveEphemeralUser_withAccount() throws Exception {
        final int ephemeralUserId = createEphemeralUser();
        assertSwitchToNewUser(ephemeralUserId);

        installPackageAsUser(
                TEST_APP_PKG_APK, /* grantPermissions= */true, ephemeralUserId, /* options= */"-t");
        assertTrue(getDevice().isPackageInstalled(TEST_APP_PKG_NAME,
                String.valueOf(ephemeralUserId)));

        final boolean appResult = runDeviceTests(getDevice(),
                TEST_APP_PKG_NAME,
                TEST_APP_PKG_NAME + ".AccountCreator",
                "addMockAccountForCurrentUser",
                ephemeralUserId,
                5 * 60 * 1000L /* ms */);
        assertTrue("Failed to successfully run app", appResult);

        getDevice().reboot();
        assertUserNotPresent(ephemeralUserId);
    }

    /**
     * Test to verify that {@link android.os.UserManager#removeUserOrSetEphemeral(int)} immediately
     * removes a user that isn't running.
     *
     * <p>Indirectly executed by means of the --set-ephemeral-if-in-use flag
     */
    @Presubmit
    @Test
    public void testRemoveUserOrSetEphemeral_nonRunningUserRemoved() throws Exception {
        final int userId = createUser();

        executeRemoveUserOrSetEphemeralAdbCommand(userId);

        assertUserNotPresent(userId);
    }

    /**
     * Test to verify that {@link android.os.UserManager#removeUserOrSetEphemeral(int)} sets the
     * current user to ephemeral and removes the user after user switch.
     *
     * <p>Indirectly executed by means of the --set-ephemeral-if-in-use flag
     */
    @Presubmit
    @Test
    public void testRemoveUserOrSetEphemeral_currentUserSetEphemeral_removeAfterSwitch()
            throws Exception {
        final int userId = createUser();

        assertSwitchToNewUser(userId);
        executeRemoveUserOrSetEphemeralAdbCommand(userId);
        assertUserEphemeral(userId);

        assertSwitchToUser(userId, mInitialUserId);
        waitForUserRemove(userId);
        assertUserNotPresent(userId);
    }

    /**
     * Test to verify that {@link android.os.UserManager#removeUserOrSetEphemeral(int)} sets the
     * current user to ephemeral and removes that user after reboot.
     *
     * <p>Indirectly executed by means of the --set-ephemeral-if-in-use flag
     */
    @Presubmit
    @Test
    public void testRemoveUserOrSetEphemeral_currentUserSetEphemeral_removeAfterReboot()
            throws Exception {
        final int userId = createUser();

        assertSwitchToNewUser(userId);
        executeRemoveUserOrSetEphemeralAdbCommand(userId);
        assertUserEphemeral(userId);

        getDevice().reboot();
        assertUserNotPresent(userId);
    }

    private void executeRemoveUserOrSetEphemeralAdbCommand(int userId) throws Exception {
        getDevice().executeShellV2Command("pm remove-user --set-ephemeral-if-in-use " + userId);
    }

    private void assertUserEphemeral(int userId) throws Exception {
        assertUserPresent(userId);
        assertWithMessage("User ID %s should be flagged as ephemeral", userId)
                .that(getDevice().getUserInfos().get(userId).isEphemeral()).isTrue();
    }

    private int createUser() throws Exception {
        return createUser(/* isGuest= */ false, /* isEphemeral= */ false);
    }

    private int createEphemeralUser() throws Exception {
        return createUser(/* isGuest= */ false, /* isEphemeral= */ true);
    }

    private int createUser(boolean isGuest, boolean isEphemeral) throws Exception {
        final String name = "TestUser_" + System.currentTimeMillis();
        try {
            return getDevice().createUser(name, isGuest, isEphemeral);
        } catch (Exception e) {
            throw new IllegalStateException(String.format(
                    "Failed to create user (name=%s, isGuest=%s, isEphemeral=%s)",
                    name, isGuest, isEphemeral), e);
        }
    }
}
