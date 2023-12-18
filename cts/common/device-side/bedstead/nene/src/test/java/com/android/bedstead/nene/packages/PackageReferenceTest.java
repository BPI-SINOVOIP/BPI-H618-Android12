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

import static com.google.common.truth.Truth.assertThat;

import static org.testng.Assert.assertThrows;

import android.content.Context;

import com.android.bedstead.nene.TestApis;
import com.android.bedstead.nene.exceptions.NeneException;
import com.android.bedstead.nene.users.UserReference;

import org.junit.AfterClass;
import org.junit.Ignore;
import org.junit.Test;
import org.junit.runner.RunWith;
import org.junit.runners.JUnit4;

import java.io.File;

@RunWith(JUnit4.class)
public class PackageReferenceTest {

    private static final TestApis sTestApis = new TestApis();
    private static final UserReference sUser = sTestApis.users().instrumented();
    private static final String NON_EXISTING_PACKAGE_NAME = "com.package.does.not.exist";
    private static final String PACKAGE_NAME = NON_EXISTING_PACKAGE_NAME;
    private static final String EXISTING_PACKAGE_NAME = "com.android.providers.telephony";
    private final PackageReference mTestAppReference =
            sTestApis.packages().find(TEST_APP_PACKAGE_NAME);

    // Controlled by AndroidTest.xml
    private static final String TEST_APP_PACKAGE_NAME =
            "com.android.bedstead.nene.testapps.TestApp1";
    private static final File TEST_APP_APK_FILE =
            new File("/data/local/tmp/NeneTestApp1.apk");
    private static final Context sContext =
            sTestApis.context().instrumentedContext();
    private static final UserReference sOtherUser = sTestApis.users().createUser().createAndStart();

    private static final PackageReference sInstrumentedPackage =
            sTestApis.packages().find(sContext.getPackageName());

    // Relies on this being declared by AndroidManifest.xml
    // TODO(scottjonathan): Replace with TestApp
    private static final String INSTALL_PERMISSION = "android.permission.CHANGE_WIFI_STATE";
    private static final String UNDECLARED_RUNTIME_PERMISSION = "android.permission.RECEIVE_SMS";
    private static final String DECLARED_RUNTIME_PERMISSION =
            "android.permission.INTERACT_ACROSS_USERS";
    private static final String NON_EXISTING_PERMISSION = "aPermissionThatDoesntExist";
    private static final String USER_SPECIFIC_PERMISSION = "android.permission.READ_CONTACTS";


    @AfterClass
    public static void teardownClass() {
        // TODO(scottjonathan): Use annotations to share state instead of doing so manually
        sOtherUser.remove();
    }

    @Test
    public void packageName_returnsPackageName() {
        sTestApis.packages().find(PACKAGE_NAME).packageName().equals(PACKAGE_NAME);
    }

    @Test
    public void resolve_nonExistingPackage_returnsNull() {
        assertThat(sTestApis.packages().find(NON_EXISTING_PACKAGE_NAME).resolve()).isNull();
    }

    @Test
    public void resolve_existingPackage_returnsPackage() {
        assertThat(sTestApis.packages().find(EXISTING_PACKAGE_NAME).resolve()).isNotNull();
    }

    @Test
    public void install_alreadyInstalled_installsInUser() {
        sInstrumentedPackage.install(sOtherUser);

        try {
            assertThat(sInstrumentedPackage.resolve().installedOnUsers()).contains(sOtherUser);
        } finally {
            sInstrumentedPackage.uninstall(sOtherUser);
        }
    }

    @Test
    public void uninstallForAllUsers_isUninstalledForAllUsers() {
        PackageReference pkg = sTestApis.packages().install(sUser, TEST_APP_APK_FILE);
        try {
            sTestApis.packages().install(sOtherUser, TEST_APP_APK_FILE);

            mTestAppReference.uninstallFromAllUsers();

            Package resolvedPackage = mTestAppReference.resolve();
            // Might be null or might still resolve depending on device timing
            if (resolvedPackage != null) {
                assertThat(resolvedPackage.installedOnUsers()).isEmpty();
            }
        } finally {
            pkg.uninstall(sUser);
            pkg.uninstall(sOtherUser);
        }
    }

    @Test
    public void uninstall_packageIsInstalledForDifferentUser_isUninstalledForUser() {
        PackageReference pkg = sTestApis.packages().install(sUser, TEST_APP_APK_FILE);
        try {
            sTestApis.packages().install(sOtherUser, TEST_APP_APK_FILE);

            mTestAppReference.uninstall(sUser);

            assertThat(mTestAppReference.resolve().installedOnUsers()).containsExactly(sOtherUser);
        } finally {
            pkg.uninstall(sUser);
            pkg.uninstall(sOtherUser);
        }
    }

    @Test
    public void uninstall_packageIsUninstalled() {
        sTestApis.packages().install(sUser, TEST_APP_APK_FILE);

        mTestAppReference.uninstall(sUser);

        // Depending on when Android cleans up the users, this may either no longer resolve or
        // just have an empty user list
        Package pkg = mTestAppReference.resolve();
        if (pkg != null) {
            assertThat(pkg.installedOnUsers()).isEmpty();
        }
    }

    @Test
    public void uninstall_packageNotInstalledForUser_doesNotThrowException() {
        sTestApis.packages().install(sUser, TEST_APP_APK_FILE);

        try {
            mTestAppReference.uninstall(sOtherUser);
        } finally {
            mTestAppReference.uninstall(sUser);
        }
    }

    @Test
    public void uninstall_packageDoesNotExist_doesNotThrowException() {
        PackageReference packageReference = sTestApis.packages().find(NON_EXISTING_PACKAGE_NAME);

        packageReference.uninstall(sUser);
    }

    @Test
    public void grantPermission_installPermission_throwsException() {
        assertThrows(NeneException.class, () ->
                sTestApis.packages().find(sContext.getPackageName()).grantPermission(sUser,
                INSTALL_PERMISSION));
    }

    @Test
    public void grantPermission_nonDeclaredPermission_throwsException() {
        assertThrows(NeneException.class, () ->
                sTestApis.packages().find(sContext.getPackageName()).grantPermission(sUser,
                UNDECLARED_RUNTIME_PERMISSION));
    }

    @Test
    public void grantPermission_permissionIsGranted() {
        sInstrumentedPackage.install(sOtherUser);
        sInstrumentedPackage.grantPermission(sOtherUser, USER_SPECIFIC_PERMISSION);

        try {
            assertThat(sInstrumentedPackage.resolve().grantedPermissions(sOtherUser))
                    .contains(DECLARED_RUNTIME_PERMISSION);
        } finally {
            sInstrumentedPackage.denyPermission(sOtherUser, USER_SPECIFIC_PERMISSION);
        }
    }

    @Test
    public void grantPermission_permissionIsUserSpecific_permissionIsGrantedOnlyForThatUser() {
        // Permissions are auto-granted on the current user so we need to test against new users
        try (UserReference newUser = sTestApis.users().createUser().create()) {
            sInstrumentedPackage.install(sOtherUser);
            sInstrumentedPackage.install(newUser);

            sInstrumentedPackage.grantPermission(newUser, USER_SPECIFIC_PERMISSION);

            Package resolvedPackage = sInstrumentedPackage.resolve();
            assertThat(resolvedPackage.grantedPermissions(sOtherUser))
                    .doesNotContain(USER_SPECIFIC_PERMISSION);
            assertThat(resolvedPackage.grantedPermissions(newUser))
                    .contains(USER_SPECIFIC_PERMISSION);
        } finally {
            sInstrumentedPackage.uninstall(sOtherUser);
        }
    }

    @Test
    public void grantPermission_packageDoesNotExist_throwsException() {
        assertThrows(NeneException.class, () ->
                sTestApis.packages().find(NON_EXISTING_PACKAGE_NAME).grantPermission(sUser,
                DECLARED_RUNTIME_PERMISSION));
    }

    @Test
    public void grantPermission_permissionDoesNotExist_throwsException() {
        assertThrows(NeneException.class, () ->
                sTestApis.packages().find(sContext.getPackageName()).grantPermission(sUser,
                NON_EXISTING_PERMISSION));
    }

    @Test
    public void grantPermission_packageIsNotInstalledForUser_throwsException() {
        sInstrumentedPackage.uninstall(sOtherUser);

        assertThrows(NeneException.class,
                () -> sInstrumentedPackage.grantPermission(sOtherUser,
                        DECLARED_RUNTIME_PERMISSION));
    }

    @Test
    @Ignore("Cannot be tested because all runtime permissions are granted by default")
    public void denyPermission_ownPackage_permissionIsNotGranted_doesNotThrowException() {
        PackageReference packageReference = sTestApis.packages().find(sContext.getPackageName());

        packageReference.denyPermission(sUser, USER_SPECIFIC_PERMISSION);
    }

    @Test
    public void denyPermission_ownPackage_permissionIsGranted_throwsException() {
        PackageReference packageReference = sTestApis.packages().find(sContext.getPackageName());
        packageReference.grantPermission(sUser, USER_SPECIFIC_PERMISSION);

        assertThrows(NeneException.class, () ->
                packageReference.denyPermission(sUser, USER_SPECIFIC_PERMISSION));
    }

    @Test
    public void denyPermission_permissionIsNotGranted() {
        sInstrumentedPackage.install(sOtherUser);
        try {
            sInstrumentedPackage.grantPermission(sOtherUser, USER_SPECIFIC_PERMISSION);

            sInstrumentedPackage.denyPermission(sOtherUser, USER_SPECIFIC_PERMISSION);

            assertThat(sInstrumentedPackage.resolve().grantedPermissions(sOtherUser))
                    .doesNotContain(USER_SPECIFIC_PERMISSION);
        } finally {
            sInstrumentedPackage.uninstall(sOtherUser);
        }
    }

    @Test
    public void denyPermission_packageDoesNotExist_throwsException() {
        assertThrows(NeneException.class, () ->
                sTestApis.packages().find(NON_EXISTING_PACKAGE_NAME).denyPermission(sUser,
                        DECLARED_RUNTIME_PERMISSION));
    }

    @Test
    public void denyPermission_permissionDoesNotExist_throwsException() {
        assertThrows(NeneException.class, () ->
                sTestApis.packages().find(sContext.getPackageName()).denyPermission(sUser,
                        NON_EXISTING_PERMISSION));
    }

    @Test
    public void denyPermission_packageIsNotInstalledForUser_throwsException() {
        sInstrumentedPackage.uninstall(sOtherUser);

        assertThrows(NeneException.class,
                () -> sInstrumentedPackage.denyPermission(sOtherUser, DECLARED_RUNTIME_PERMISSION));
    }

    @Test
    public void denyPermission_installPermission_throwsException() {
        sInstrumentedPackage.install(sOtherUser);

        try {
            assertThrows(NeneException.class, () ->
                    sInstrumentedPackage.denyPermission(sOtherUser, INSTALL_PERMISSION));
        } finally {
            sInstrumentedPackage.uninstall(sOtherUser);
        }
    }

    @Test
    public void denyPermission_nonDeclaredPermission_throwsException() {
        assertThrows(NeneException.class, () ->
                sTestApis.packages().find(sContext.getPackageName()).denyPermission(sUser,
                        UNDECLARED_RUNTIME_PERMISSION));
    }

    @Test
    public void denyPermission_alreadyDenied_doesNothing() {
        sInstrumentedPackage.install(sOtherUser);
        try {
            sInstrumentedPackage.denyPermission(sOtherUser, USER_SPECIFIC_PERMISSION);
            sInstrumentedPackage.denyPermission(sOtherUser, USER_SPECIFIC_PERMISSION);

            assertThat(sInstrumentedPackage.resolve().grantedPermissions(sOtherUser))
                    .doesNotContain(USER_SPECIFIC_PERMISSION);
        } finally {
            sInstrumentedPackage.uninstall(sOtherUser);
        }
    }

    @Test
    public void denyPermission_permissionIsUserSpecific_permissionIsDeniedOnlyForThatUser() {
        // Permissions are auto-granted on the current user so we need to test against new users
        try (UserReference newUser = sTestApis.users().createUser().create()) {
            sInstrumentedPackage.install(sOtherUser);
            sInstrumentedPackage.install(newUser);
            sInstrumentedPackage.grantPermission(sOtherUser, USER_SPECIFIC_PERMISSION);
            sInstrumentedPackage.grantPermission(newUser, USER_SPECIFIC_PERMISSION);

            sInstrumentedPackage.denyPermission(newUser, USER_SPECIFIC_PERMISSION);

            Package resolvedPackage = sInstrumentedPackage.resolve();
            assertThat(resolvedPackage.grantedPermissions(newUser))
                    .doesNotContain(USER_SPECIFIC_PERMISSION);
            assertThat(resolvedPackage.grantedPermissions(sOtherUser))
                    .contains(USER_SPECIFIC_PERMISSION);
        } finally {
            sInstrumentedPackage.uninstall(sOtherUser);
        }
    }
}
