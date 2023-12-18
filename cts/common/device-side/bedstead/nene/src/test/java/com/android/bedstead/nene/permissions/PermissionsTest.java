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

package com.android.bedstead.nene.permissions;

import static android.content.pm.PackageManager.PERMISSION_DENIED;
import static android.content.pm.PackageManager.PERMISSION_GRANTED;

import static com.google.common.truth.Truth.assertThat;

import static org.junit.Assume.assumeFalse;
import static org.junit.Assume.assumeTrue;
import static org.testng.Assert.assertThrows;

import android.content.Context;
import android.os.Build;

import com.android.bedstead.nene.TestApis;
import com.android.bedstead.nene.exceptions.NeneException;
import com.android.bedstead.nene.utils.ShellCommandUtils;
import com.android.bedstead.nene.utils.Versions;

import org.junit.Test;
import org.junit.runner.RunWith;
import org.junit.runners.JUnit4;

@RunWith(JUnit4.class)
public class PermissionsTest {

    private static final String PERMISSION_HELD_BY_SHELL =
            "android.permission.INTERACT_ACROSS_PROFILES";
    private static final String DIFFERENT_PERMISSION_HELD_BY_SHELL =
            "android.permission.INTERACT_ACROSS_USERS_FULL";
    private static final TestApis sTestApis = new TestApis();
    private static final Context sContext = sTestApis.context().instrumentedContext();

    private static final String NON_EXISTING_PERMISSION = "permissionWhichDoesNotExist";

    // We expect these permissions are listed in the Manifest
    private static final String INSTALL_PERMISSION = "android.permission.CHANGE_WIFI_STATE";
    private static final String DECLARED_PERMISSION_NOT_HELD_BY_SHELL_PRE_S =
            "android.permission.INTERNET";

    @Test
    public void default_permissionIsNotGranted() {
        assertThat(sContext.checkSelfPermission(PERMISSION_HELD_BY_SHELL))
                .isEqualTo(PERMISSION_DENIED);
    }

    @Test
    public void withPermission_shellPermission_permissionIsGranted() {
        assumeTrue("assume shell identity is only available on Q+",
                Build.VERSION.SDK_INT >= Build.VERSION_CODES.Q);

        try (PermissionContext p =
                     sTestApis.permissions().withPermission(PERMISSION_HELD_BY_SHELL)) {
            assertThat(sContext.checkSelfPermission(PERMISSION_HELD_BY_SHELL))
                    .isEqualTo(PERMISSION_GRANTED);
        }
    }

    @Test
    public void withoutPermission_alreadyGranted_androidPreQ_throwsException() {
        assumeTrue("assume shell identity is only available on Q+",
                Build.VERSION.SDK_INT < Build.VERSION_CODES.Q);

        assertThrows(NeneException.class,
                () -> sTestApis.permissions().withoutPermission(
                        DECLARED_PERMISSION_NOT_HELD_BY_SHELL_PRE_S));
    }

    @Test
    public void withoutPermission_permissionIsNotGranted() {
        assumeTrue("assume shell identity is only available on Q+",
                Build.VERSION.SDK_INT >= Build.VERSION_CODES.Q);

        try (PermissionContext p = sTestApis.permissions().withPermission(PERMISSION_HELD_BY_SHELL);
             PermissionContext p2 = sTestApis.permissions().withoutPermission(
                     PERMISSION_HELD_BY_SHELL)) {

            assertThat(sContext.checkSelfPermission(PERMISSION_HELD_BY_SHELL))
                    .isEqualTo(PERMISSION_DENIED);
        }
    }

    @Test
    public void autoclose_withoutPermission_permissionIsGrantedAgain() {
        assumeTrue("assume shell identity is only available on Q+",
                Build.VERSION.SDK_INT >= Build.VERSION_CODES.Q);

        try (PermissionContext p =
                     sTestApis.permissions().withPermission(PERMISSION_HELD_BY_SHELL)) {
            try (PermissionContext p2 =
                         sTestApis.permissions().withoutPermission(PERMISSION_HELD_BY_SHELL)) {
                // Intentionally empty as we're testing that autoclosing restores the permission
            }

            assertThat(sContext.checkSelfPermission(PERMISSION_HELD_BY_SHELL))
                    .isEqualTo(PERMISSION_GRANTED);
        }
    }

    @Test
    public void withoutPermission_installPermission_androidPreQ_throwsException() {
        assumeTrue("assume shell identity is only available on Q+",
                Build.VERSION.SDK_INT < Build.VERSION_CODES.Q);

        assertThrows(NeneException.class,
                () -> sTestApis.permissions().withoutPermission(INSTALL_PERMISSION));
    }

    @Test
    public void withoutPermission_permissionIsAlreadyGrantedInInstrumentedApp_permissionIsNotGranted() {
        assumeTrue("assume shell identity is only available on Q+",
                Build.VERSION.SDK_INT >= Build.VERSION_CODES.Q);
        assumeFalse("After S, all available permissions are held by shell",
                Versions.meetsMinimumSdkVersionRequirement(Build.VERSION_CODES.S));

        try (PermissionContext p =
                    sTestApis.permissions().withoutPermission(
                            DECLARED_PERMISSION_NOT_HELD_BY_SHELL_PRE_S)) {
            assertThat(
                    sContext.checkSelfPermission(DECLARED_PERMISSION_NOT_HELD_BY_SHELL_PRE_S))
                    .isEqualTo(PERMISSION_DENIED);
        }
    }

    @Test
    public void withoutPermission_permissionIsAlreadyGrantedInInstrumentedApp_androidPreQ_throwsException() {
        assumeTrue("assume shell identity is only available on Q+",
                Build.VERSION.SDK_INT < Build.VERSION_CODES.Q);

        assertThrows(NeneException.class,
                () -> sTestApis.permissions().withoutPermission(
                        DECLARED_PERMISSION_NOT_HELD_BY_SHELL_PRE_S));
    }

    @Test
    public void withPermission_permissionIsAlreadyGrantedInInstrumentedApp_permissionIsGranted() {
        try (PermissionContext p =
                    sTestApis.permissions().withPermission(
                            DECLARED_PERMISSION_NOT_HELD_BY_SHELL_PRE_S)) {
            assertThat(
                    sContext.checkSelfPermission(DECLARED_PERMISSION_NOT_HELD_BY_SHELL_PRE_S))
                    .isEqualTo(PERMISSION_GRANTED);
        }
    }

    @Test
    public void withPermission_nonExistingPermission_throwsException() {
        assertThrows(NeneException.class,
                () -> sTestApis.permissions().withPermission(NON_EXISTING_PERMISSION));
    }

    @Test
    public void withoutPermission_nonExistingPermission_doesNotThrowException() {
        try (PermissionContext p =
                     sTestApis.permissions().withoutPermission(NON_EXISTING_PERMISSION)) {
            // Intentionally empty
        }
    }

    @Test
    public void withPermissionAndWithoutPermission_bothApplied() {
        try (PermissionContext p = sTestApis.permissions().withPermission(PERMISSION_HELD_BY_SHELL)
                .withoutPermission(DIFFERENT_PERMISSION_HELD_BY_SHELL)) {

            assertThat(sContext.checkSelfPermission(PERMISSION_HELD_BY_SHELL))
                    .isEqualTo(PERMISSION_GRANTED);
            assertThat(sContext.checkSelfPermission(DIFFERENT_PERMISSION_HELD_BY_SHELL))
                    .isEqualTo(PERMISSION_DENIED);
        }
    }

    @Test
    public void withoutPermissionAndWithPermission_bothApplied() {
        try (PermissionContext p = sTestApis.permissions()
                .withoutPermission(DIFFERENT_PERMISSION_HELD_BY_SHELL)
                .withPermission(PERMISSION_HELD_BY_SHELL)) {

            assertThat(sContext.checkSelfPermission(PERMISSION_HELD_BY_SHELL))
                    .isEqualTo(PERMISSION_GRANTED);
            assertThat(sContext.checkSelfPermission(DIFFERENT_PERMISSION_HELD_BY_SHELL))
                    .isEqualTo(PERMISSION_DENIED);
        }
    }

    @Test
    public void withPermissionAndWithoutPermission_contradictoryPermissions_throwsException() {
        assertThrows(NeneException.class, () -> sTestApis.permissions()
                .withPermission(PERMISSION_HELD_BY_SHELL)
                .withoutPermission(PERMISSION_HELD_BY_SHELL));
    }

    @Test
    public void withoutPermissionAndWithPermission_contradictoryPermissions_throwsException() {
        assertThrows(NeneException.class, () -> sTestApis.permissions()
                .withoutPermission(PERMISSION_HELD_BY_SHELL)
                .withPermission(PERMISSION_HELD_BY_SHELL));
    }

    @Test
    public void withoutPermission_androidSAndAbove_restoresPreviousPermissionContext() {
        assumeTrue("restoring permissions is only available on S+",
                Versions.meetsMinimumSdkVersionRequirement(Build.VERSION_CODES.S));

        ShellCommandUtils.uiAutomation().adoptShellPermissionIdentity(PERMISSION_HELD_BY_SHELL);

        try {
            PermissionContext p =
                    sTestApis.permissions()
                            .withoutPermission(PERMISSION_HELD_BY_SHELL);
            p.close();

            assertThat(sContext.checkSelfPermission(PERMISSION_HELD_BY_SHELL))
                    .isEqualTo(PERMISSION_GRANTED);
        } finally {
            ShellCommandUtils.uiAutomation().dropShellPermissionIdentity();
        }
    }

    // TODO(scottjonathan): Once we can install the testapp without granting all runtime
    //  permissions, add a test that this works pre-Q
}
