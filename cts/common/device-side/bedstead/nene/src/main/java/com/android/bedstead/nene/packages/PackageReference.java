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

package com.android.bedstead.nene.packages;

import static android.Manifest.permission.INTERACT_ACROSS_USERS_FULL;
import static android.content.pm.PermissionInfo.PROTECTION_DANGEROUS;
import static android.content.pm.PermissionInfo.PROTECTION_FLAG_DEVELOPMENT;

import static com.google.common.truth.Truth.assertWithMessage;

import android.content.Intent;
import android.content.IntentFilter;
import android.content.pm.PackageManager;
import android.content.pm.PermissionInfo;

import androidx.annotation.Nullable;

import com.android.bedstead.nene.TestApis;
import com.android.bedstead.nene.annotations.Experimental;
import com.android.bedstead.nene.exceptions.AdbException;
import com.android.bedstead.nene.exceptions.NeneException;
import com.android.bedstead.nene.permissions.PermissionContext;
import com.android.bedstead.nene.users.UserReference;
import com.android.bedstead.nene.utils.ShellCommand;
import com.android.compatibility.common.util.BlockingBroadcastReceiver;

import java.io.File;
import java.util.Arrays;
import java.util.Set;
import java.util.stream.Collectors;

/**
 * A representation of a package on device which may or may not exist.
 *
 * <p>To resolve the package into a {@link Package}, see {@link #resolve()}.
 */
public abstract class PackageReference {
    private final TestApis mTestApis;
    private final String mPackageName;

    private static final int PIDS_PER_USER_ID = 100000;

    private final PackageManager mPackageManager;

    PackageReference(TestApis testApis, String packageName) {
        mTestApis = testApis;
        mPackageManager = mTestApis.context().instrumentedContext().getPackageManager();
        mPackageName = packageName;
    }

    /** Return the package's name. */
    public String packageName() {
        return mPackageName;
    }

    /**
     * Get the current state of the {@link Package} from the device, or {@code null} if the package
     * does not exist.
     */
    @Nullable
    public Package resolve() {
        return mTestApis.packages().fetchPackage(mPackageName);
    }

    /**
     * Install the package on the given user.
     *
     * <p>If you wish to install a package which is not already installed on another user, see
     * {@link Packages#install(UserReference, File)}.
     */
    public PackageReference install(UserReference user) {
        if (user == null) {
            throw new NullPointerException();
        }
        try {
            // Expected output "Package X installed for user: Y"
            ShellCommand.builderForUser(user, "cmd package install-existing")
                    .addOperand(mPackageName)
                    .validate(
                            (output) -> output.contains("installed for user"))
                    .execute();
            return this;
        } catch (AdbException e) {
            throw new NeneException("Could not install-existing package " + this, e);
        }
    }

    /**
     * Uninstall the package for all users.
     *
     * <p>The package will no longer {@link #resolve()}.
     */
    public PackageReference uninstallFromAllUsers() {
        Package pkg = resolve();
        if (pkg == null) {
            return this;
        }

        for (UserReference user : pkg.installedOnUsers()) {
            pkg.uninstall(user);
        }

        return this;
    }

    /**
     * Uninstall the package for the given user.
     *
     * <p>If this is the last user which has this package installed, then the package will no
     * longer {@link #resolve()}.
     *
     * <p>If the package is not installed for the given user, nothing will happen.
     */
    public PackageReference uninstall(UserReference user) {
        if (user == null) {
            throw new NullPointerException();
        }

        IntentFilter packageRemovedIntentFilter =
                new IntentFilter(Intent.ACTION_PACKAGE_REMOVED);
        packageRemovedIntentFilter.addDataScheme("package");

        BlockingBroadcastReceiver broadcastReceiver = BlockingBroadcastReceiver.create(
                mTestApis.context().androidContextAsUser(user),
                packageRemovedIntentFilter);

        try {
            try (PermissionContext p = mTestApis.permissions().withPermission(
                    INTERACT_ACROSS_USERS_FULL)) {
                broadcastReceiver.register();
            }

            // Expected output "Success"
            String output = ShellCommand.builderForUser(user, "pm uninstall")
                    .addOperand(mPackageName)
                    .validate((o) -> {
                        o = o.toUpperCase();
                        return o.startsWith("SUCCESS") || o.contains("NOT INSTALLED FOR");
                    })
                    .execute();

            if (output.toUpperCase().startsWith("SUCCESS")) {
                broadcastReceiver.awaitForBroadcastOrFail();
            }

            return this;
        } catch (AdbException e) {
            throw new NeneException("Could not uninstall package " + this, e);
        } finally {
            broadcastReceiver.unregisterQuietly();
        }
    }

    /**
     * Enable this package for the given {@link UserReference}.
     */
    @Experimental
    public PackageReference enable(UserReference user) {
        try {
            ShellCommand.builderForUser(user, "pm enable")
                    .addOperand(mPackageName)
                    .validate(o -> o.contains("new state"))
                    .execute();
        } catch (AdbException e) {
            throw new NeneException("Error enabling package " + this + " for user " + user, e);
        }
        return this;
    }

    /**
     * Enable this package on the instrumented user.
     */
    @Experimental
    public PackageReference enable() {
        return enable(mTestApis.users().instrumented());
    }

    /**
     * Disable this package for the given {@link UserReference}.
     */
    @Experimental
    public PackageReference disable(UserReference user) {
        try {
            ShellCommand.builderForUser(user, "pm disable")
                    .addOperand(mPackageName)
                    .validate(o -> o.contains("new state"))
                    .execute();
        } catch (AdbException e) {
            throw new NeneException("Error disabling package " + this + " for user " + user, e);
        }
        return this;
    }

    /**
     * Disable this package on the instrumented user.
     */
    @Experimental
    public PackageReference disable() {
        return disable(mTestApis.users().instrumented());
    }

    /**
     * Get a reference to the given {@code componentName} within this package.
     *
     * <p>This does not guarantee that the component exists.
     */
    @Experimental
    public ComponentReference component(String componentName) {
        return new ComponentReference(mTestApis, this, componentName);
    }

    /**
     * Grant a permission for the package on the given user.
     *
     * <p>The package must be installed on the user, must request the given permission, and the
     * permission must be a runtime permission.
     */
    public PackageReference grantPermission(UserReference user, String permission) {
        // There is no readable output upon failure so we need to check ourselves
        checkCanGrantOrRevokePermission(user, permission);

        try {
            ShellCommand.builderForUser(user, "pm grant")
                    .addOperand(packageName())
                    .addOperand(permission)
                    .allowEmptyOutput(true)
                    .validate(String::isEmpty)
                    .execute();

            assertWithMessage("Error granting permission " + permission
                    + " to package " + this + " on user " + user
                    + ". Command appeared successful but not set.")
                    .that(resolve().grantedPermissions(user)).contains(permission);

            return this;
        } catch (AdbException e) {
            throw new NeneException("Error granting permission " + permission + " to package "
                    + this + " on user " + user, e);
        }
    }

    /**
     * Deny a permission for the package on the given user.
     *
     * <p>The package must be installed on the user, must request the given permission, and the
     * permission must be a runtime permission.
     *
     * <p>You can not deny permissions for the current package on the current user.
     */
    public PackageReference denyPermission(UserReference user, String permission) {
        // There is no readable output upon failure so we need to check ourselves
        checkCanGrantOrRevokePermission(user, permission);

        if (packageName().equals(mTestApis.context().instrumentedContext().getPackageName())
                && user.equals(mTestApis.users().instrumented())) {
            Package resolved = resolve();
            if (!resolved.grantedPermissions(user).contains(permission)) {
                return this; // Already denied
            }
            throw new NeneException("Cannot deny permission from current package");
        }

        try {
            ShellCommand.builderForUser(user, "pm revoke")
                    .addOperand(packageName())
                    .addOperand(permission)
                    .allowEmptyOutput(true)
                    .validate(String::isEmpty)
                    .execute();

            assertWithMessage("Error denying permission " + permission
                    + " to package " + this + " on user " + user
                    + ". Command appeared successful but not set.")
                    .that(resolve().grantedPermissions(user)).doesNotContain(permission);

            return this;
        } catch (AdbException e) {
            throw new NeneException("Error denying permission " + permission + " to package "
                    + this + " on user " + user, e);
        }
    }

    private void checkCanGrantOrRevokePermission(UserReference user, String permission) {
        Package resolved = resolve();
        if (resolved == null || !resolved.installedOnUsers().contains(user)) {
            throw new NeneException("Attempting to grant " + permission + " to " + this
                    + " on user " + user + ". But it is not installed");
        }

        try {
            PermissionInfo permissionInfo =
                    mPackageManager.getPermissionInfo(permission, /* flags= */ 0);

            if (!protectionIsDangerous(permissionInfo.protectionLevel)
                    && !protectionIsDevelopment(permissionInfo.protectionLevel)) {
                throw new NeneException("Cannot grant non-runtime permission "
                        + permission + ", protection level is " + permissionInfo.protectionLevel);
            }

            if (!resolved.requestedPermissions().contains(permission)) {
                throw new NeneException("Cannot grant permission "
                        + permission + " which was not requested by package " + packageName());
            }
        } catch (PackageManager.NameNotFoundException e) {
            throw new NeneException("Permission does not exist: " + permission);
        }
    }

    private boolean protectionIsDangerous(int protectionLevel) {
        return (protectionLevel & PROTECTION_DANGEROUS) != 0;
    }

    private boolean protectionIsDevelopment(int protectionLevel) {
        return (protectionLevel & PROTECTION_FLAG_DEVELOPMENT) != 0;
    }

    @Experimental
    public Set<ProcessReference> runningProcesses() {
        // TODO(scottjonathan): See if this can be remade using
        //  ActivityManager#getRunningappProcesses
        try {
            return ShellCommand.builder("ps")
                    .addOperand("-A")
                    .addOperand("-n")
                    .executeAndParseOutput(o -> parsePsOutput(o).stream()
                    .filter(p -> p.mPackageName.equals(mPackageName))
                    .map(p -> new ProcessReference(this, p.mPid, mTestApis.users().find(p.mUserId))))
                    .collect(Collectors.toSet());
        } catch (AdbException e) {
            throw new NeneException("Error getting running processes ", e);
        }
    }

    private Set<ProcessInfo> parsePsOutput(String psOutput) {
        return Arrays.stream(psOutput.split("\n"))
                .skip(1) // Skip the title line
                .map(s -> s.split("\\s+"))
                .map(m -> new ProcessInfo(
                        m[8], Integer.parseInt(m[1]), Integer.parseInt(m[0]) / PIDS_PER_USER_ID))
                .collect(Collectors.toSet());
    }

    private static final class ProcessInfo {
        public final String mPackageName;
        public final int mPid;
        public final int mUserId;

        public ProcessInfo(String packageName, int pid, int userId) {
            if (packageName == null) {
                throw new NullPointerException();
            }
            mPackageName = packageName;
            mPid = pid;
            mUserId = userId;
        }

        @Override
        public String toString() {
            return "ProcessInfo{packageName=" + mPackageName + ", pid="
                    + mPid + ", userId=" + mUserId + "}";
        }
    }

    @Experimental
    @Nullable
    public ProcessReference runningProcess(UserReference user) {
        return runningProcesses().stream().filter(
                i -> i.user().equals(user))
                .findAny()
                .orElse(null);
    }


    @Override
    public int hashCode() {
        return mPackageName.hashCode();
    }

    @Override
    public boolean equals(Object obj) {
        if (!(obj instanceof PackageReference)) {
            return false;
        }

        PackageReference other = (PackageReference) obj;
        return other.mPackageName.equals(mPackageName);
    }

    @Override
    public String toString() {
        StringBuilder stringBuilder = new StringBuilder("PackageReference{");
        stringBuilder.append("packageName=" + mPackageName);
        stringBuilder.append("}");
        return stringBuilder.toString();
    }
}
