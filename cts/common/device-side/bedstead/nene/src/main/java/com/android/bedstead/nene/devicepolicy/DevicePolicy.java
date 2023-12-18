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

import static android.Manifest.permission.INTERACT_ACROSS_USERS;
import static android.Manifest.permission.INTERACT_ACROSS_USERS_FULL;
import static android.Manifest.permission.WRITE_SECURE_SETTINGS;
import static android.os.Build.VERSION.SDK_INT;

import static com.android.bedstead.nene.permissions.Permissions.MANAGE_DEVICE_ADMINS;
import static com.android.bedstead.nene.permissions.Permissions.MANAGE_PROFILE_AND_DEVICE_OWNERS;

import android.app.admin.DevicePolicyManager;
import android.content.ComponentName;
import android.content.Intent;
import android.content.pm.PackageManager;
import android.content.pm.ResolveInfo;
import android.os.Build;
import android.provider.Settings;
import android.util.Log;

import androidx.annotation.Nullable;

import com.android.bedstead.nene.TestApis;
import com.android.bedstead.nene.annotations.Experimental;
import com.android.bedstead.nene.exceptions.AdbException;
import com.android.bedstead.nene.exceptions.AdbParseException;
import com.android.bedstead.nene.exceptions.NeneException;
import com.android.bedstead.nene.packages.PackageReference;
import com.android.bedstead.nene.permissions.PermissionContext;
import com.android.bedstead.nene.users.User;
import com.android.bedstead.nene.users.UserReference;
import com.android.bedstead.nene.utils.ShellCommand;
import com.android.bedstead.nene.utils.ShellCommandUtils;
import com.android.bedstead.nene.utils.Versions;
import com.android.compatibility.common.util.PollingCheck;

import java.util.Arrays;
import java.util.Collection;
import java.util.HashSet;
import java.util.List;
import java.util.Map;
import java.util.Set;


/**
 * Test APIs related to device policy.
 */
public final class DevicePolicy {

    private static final String LOG_TAG = "DevicePolicy";

    private static final String USER_SETUP_COMPLETE_KEY = "user_setup_complete";

    private final TestApis mTestApis;
    private final AdbDevicePolicyParser mParser;

    private DeviceOwner mCachedDeviceOwner;
    private Map<UserReference, ProfileOwner> mCachedProfileOwners;

    public DevicePolicy(TestApis testApis) {
        if (testApis == null) {
            throw new NullPointerException();
        }

        mTestApis = testApis;
        mParser = AdbDevicePolicyParser.get(mTestApis, SDK_INT);
    }

    /**
     * Set the profile owner for a given {@link UserReference}.
     */
    public ProfileOwner setProfileOwner(UserReference user, ComponentName profileOwnerComponent) {
        if (user == null || profileOwnerComponent == null) {
            throw new NullPointerException();
        }

        ShellCommand.Builder command =
                ShellCommand.builderForUser(user, "dpm set-profile-owner")
                .addOperand(profileOwnerComponent.flattenToShortString())
                .validate(ShellCommandUtils::startsWithSuccess);

        // TODO(b/187925230): If it fails, we check for terminal failure states - and if not
        //  we retry because if the profile owner was recently removed, it can take some time
        //  to be allowed to set it again
        retryIfNotTerminal(
                () -> command.executeOrThrowNeneException("Could not set profile owner for user "
                        + user + " component " + profileOwnerComponent),
                () -> checkForTerminalProfileOwnerFailures(user, profileOwnerComponent));
        return new ProfileOwner(mTestApis, user,
                mTestApis.packages().find(
                        profileOwnerComponent.getPackageName()), profileOwnerComponent);
    }

    private void checkForTerminalProfileOwnerFailures(
            UserReference user, ComponentName profileOwnerComponent) {
        ProfileOwner profileOwner = getProfileOwner(user);
        if (profileOwner != null) {
            // TODO(scottjonathan): Should we actually fail here if the component name is the
            //  same?

            throw new NeneException(
                    "Could not set profile owner for user " + user
                            + " as a profile owner is already set: " + profileOwner);
        }

        PackageReference pkg = mTestApis.packages().find(
                profileOwnerComponent.getPackageName());
        if (!mTestApis.packages().installedForUser(user).contains(pkg)) {
            throw new NeneException(
                    "Could not set profile owner for user " + user
                            + " as the package " + pkg + " is not installed");
        }

        if (!componentCanBeSetAsDeviceAdmin(profileOwnerComponent, user)) {
            throw new NeneException("Could not set profile owner for user "
                    + user + " as component " + profileOwnerComponent + " is not valid");
        }
    }

    /**
     * Get the profile owner for a given {@link UserReference}.
     */
    public ProfileOwner getProfileOwner(UserReference user) {
        if (user == null) {
            throw new NullPointerException();
        }
        fillCache();
        return mCachedProfileOwners.get(user);
    }

    /**
     * Set the device owner.
     */
    public DeviceOwner setDeviceOwner(UserReference user, ComponentName deviceOwnerComponent) {
        if (user == null || deviceOwnerComponent == null) {
            throw new NullPointerException();
        }

        if (!Versions.meetsMinimumSdkVersionRequirement(Build.VERSION_CODES.S)) {
            return setDeviceOwnerPreS(user, deviceOwnerComponent);
        }

        DevicePolicyManager devicePolicyManager =
                mTestApis.context().instrumentedContext()
                        .getSystemService(DevicePolicyManager.class);

        boolean userSetupComplete = getUserSetupComplete();

        try {
            setUserSetupComplete(false);

            try (PermissionContext p =
                         mTestApis.permissions().withPermission(
                                 MANAGE_PROFILE_AND_DEVICE_OWNERS, MANAGE_DEVICE_ADMINS,
                                 INTERACT_ACROSS_USERS_FULL, INTERACT_ACROSS_USERS)) {
                devicePolicyManager.setActiveAdmin(deviceOwnerComponent,
                        /* refreshing= */ true, user.id());

                // TODO(b/187925230): If it fails, we check for terminal failure states - and if not
                //  we retry because if the DO/PO was recently removed, it can take some time
                //  to be allowed to set it again
                retryIfNotTerminal(
                        () -> devicePolicyManager.setDeviceOwner(
                                deviceOwnerComponent, "Nene", user.id()),
                        () -> checkForTerminalDeviceOwnerFailures(
                                user, deviceOwnerComponent, /* allowAdditionalUsers= */ true));
            } catch (IllegalArgumentException | IllegalStateException | SecurityException e) {
                throw new NeneException("Error setting device owner", e);
            }
        } finally {
            setUserSetupComplete(userSetupComplete);
        }

        return new DeviceOwner(mTestApis, user,
                mTestApis.packages().find(
                        deviceOwnerComponent.getPackageName()), deviceOwnerComponent);
    }

    /**
     * Runs {@code operation}. If it fails, runs {@code terminalCheck} and then retries
     * {@code operation} until it does not fail or for a maximum of 30 seconds.
     *
     * <p>The {@code operation} is considered to be successful if it does not throw an exception
     */
    private void retryIfNotTerminal(
            Runnable operation, Runnable terminalCheck,
            Class<? extends RuntimeException>... exceptions) {
        Set<Class<? extends RuntimeException>> exceptionSet =
                new HashSet<>(Arrays.asList(exceptions));
        try {
            operation.run();
        } catch (RuntimeException e) {
            if (!exceptionSet.contains(e.getClass())) {
                throw e;
            }

            terminalCheck.run();

            try {
                PollingCheck.waitFor(30_000, () -> {
                    try {
                        operation.run();
                        return true;
                    } catch (RuntimeException e2) {
                        if (!exceptionSet.contains(e2.getClass())) {
                            throw e2;
                        }
                        return false;
                    }
                });
            } catch (AssertionError e3) {
                operation.run();
            }
        }
    }


    private void setUserSetupComplete(boolean complete) {
        DevicePolicyManager devicePolicyManager =
                mTestApis.context().instrumentedContext()
                        .getSystemService(DevicePolicyManager.class);
        try (PermissionContext p = mTestApis.permissions().withPermission(
                WRITE_SECURE_SETTINGS, MANAGE_PROFILE_AND_DEVICE_OWNERS,
                INTERACT_ACROSS_USERS_FULL)) {
            Settings.Secure.putInt(mTestApis.context().androidContextAsUser(
                    mTestApis.users().system()).getContentResolver(),
                    USER_SETUP_COMPLETE_KEY, complete ? 1 : 0);
            devicePolicyManager.forceUpdateUserSetupComplete(mTestApis.users().system().id());
        }
    }

    private boolean getUserSetupComplete() {
        return Settings.Secure.getInt(
                mTestApis.context().instrumentedContext().getContentResolver(),
                USER_SETUP_COMPLETE_KEY, /* def= */ 0) == 1;
    }

    private DeviceOwner setDeviceOwnerPreS(UserReference user, ComponentName deviceOwnerComponent) {
        ShellCommand.Builder command = ShellCommand.builderForUser(
                user, "dpm set-device-owner")
                .addOperand(deviceOwnerComponent.flattenToShortString())
                .validate(ShellCommandUtils::startsWithSuccess);

        // TODO(b/187925230): If it fails, we check for terminal failure states - and if not
        //  we retry because if the device owner was recently removed, it can take some time
        //  to be allowed to set it again
        retryIfNotTerminal(
                () -> command.executeOrThrowNeneException("Could not set device owner for user "
                        + user + " component " + deviceOwnerComponent),
                () -> checkForTerminalDeviceOwnerFailures(
                    user, deviceOwnerComponent, /* allowAdditionalUsers= */ false));

        return new DeviceOwner(mTestApis, user,
                mTestApis.packages().find(
                        deviceOwnerComponent.getPackageName()), deviceOwnerComponent);
    }

    private void checkForTerminalDeviceOwnerFailures(
            UserReference user, ComponentName deviceOwnerComponent, boolean allowAdditionalUsers) {
        DeviceOwner deviceOwner = getDeviceOwner();
        if (deviceOwner != null) {
            // TODO(scottjonathan): Should we actually fail here if the component name is the
            //  same?

            throw new NeneException(
                    "Could not set device owner for user " + user
                            + " as a device owner is already set: " + deviceOwner);
        }

        PackageReference pkg = mTestApis.packages().find(
                deviceOwnerComponent.getPackageName());
        if (!mTestApis.packages().installedForUser(user).contains(pkg)) {
            throw new NeneException(
                    "Could not set device owner for user " + user
                            + " as the package " + pkg + " is not installed");
        }

        if (!componentCanBeSetAsDeviceAdmin(deviceOwnerComponent, user)) {
            throw new NeneException("Could not set device owner for user "
                    + user + " as component " + deviceOwnerComponent + " is not valid");
        }

        if (!allowAdditionalUsers) {
            Collection<User> users = mTestApis.users().all();

            if (users.size() > 1) {
                throw new NeneException("Could not set device owner for user "
                        + user + " as there are already additional users on the device: " + users);
            }

        }
        // TODO(scottjonathan): Check accounts
    }

    private boolean componentCanBeSetAsDeviceAdmin(ComponentName component, UserReference user) {
        PackageManager packageManager =
                mTestApis.context().instrumentedContext().getPackageManager();
        Intent intent = new Intent("android.app.action.DEVICE_ADMIN_ENABLED");
        intent.setComponent(component);

        try (PermissionContext p =
                     mTestApis.permissions().withPermission(INTERACT_ACROSS_USERS_FULL)) {
            List<ResolveInfo> r =
                    packageManager.queryBroadcastReceiversAsUser(
                            intent, /* flags= */ 0, user.userHandle());
            return (!r.isEmpty());
        }
    }

    /**
     * Get the device owner.
     */
    @Nullable
    public DeviceOwner getDeviceOwner() {
        fillCache();
        return mCachedDeviceOwner;
    }

    private void fillCache() {
        int retries = 5;
        while (true) {
            try {
                // TODO: Replace use of adb on supported versions of Android
                String devicePolicyDumpsysOutput =
                        ShellCommand.builder("dumpsys device_policy").execute();
                AdbDevicePolicyParser.ParseResult result = mParser.parse(devicePolicyDumpsysOutput);

                mCachedDeviceOwner = result.mDeviceOwner;
                mCachedProfileOwners = result.mProfileOwners;
                return;
            } catch (AdbParseException e) {
                if (e.adbOutput().contains("DUMP TIMEOUT") && retries-- > 0) {
                    // Sometimes this call times out - just retry
                    Log.e(LOG_TAG, "Dump timeout when filling cache, retrying", e);
                } else {
                    throw new NeneException("Error filling cache", e);
                }
            } catch (AdbException e) {
                throw new NeneException("Error filling cache", e);
            }
        }
    }

    /** See {@link android.app.admin.DevicePolicyManager#getPolicyExemptApps()}. */
    @Experimental
    public Set<String> getPolicyExemptApps() {
        try (PermissionContext p = mTestApis.permissions().withPermission(MANAGE_DEVICE_ADMINS)) {
            return mTestApis.context()
                    .instrumentedContext()
                    .getSystemService(DevicePolicyManager.class)
                    .getPolicyExemptApps();
        }
    }
}
