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

package com.android.cts.crossprofileappstest;

import static com.google.common.truth.Truth.assertThat;

import static org.junit.Assert.fail;

import android.app.AppOpsManager;
import android.app.admin.DevicePolicyManager;
import android.content.ComponentName;
import android.content.Context;
import android.content.Intent;
import android.content.pm.CrossProfileApps;
import android.content.pm.PackageManager;
import android.os.Binder;
import android.os.Bundle;
import android.os.UserHandle;
import android.os.UserManager;

import androidx.test.InstrumentationRegistry;
import androidx.test.runner.AndroidJUnit4;

import com.android.bedstead.nene.TestApis;
import com.android.bedstead.nene.permissions.PermissionContext;

import org.junit.After;
import org.junit.Test;
import org.junit.runner.RunWith;

import java.util.Collections;

@RunWith(AndroidJUnit4.class)
public class CrossProfileAppsPermissionToInteractTest {
    public static final String MANAGE_APP_OPS_MODES_PERMISSION =
            "android.permission.MANAGE_APP_OPS_MODES";
    public static final String INTERACT_ACROSS_PROFILES_PERMISSION =
            "android.permission.INTERACT_ACROSS_PROFILES";
    public static final String INTERACT_ACROSS_USERS_PERMISSION =
            "android.permission.INTERACT_ACROSS_USERS";
    public static final String INTERACT_ACROSS_USERS_FULL_PERMISSION =
            "android.permission.INTERACT_ACROSS_USERS_FULL";
    public static final String ACTION_MANAGE_CROSS_PROFILE_ACCESS =
            "android.settings.MANAGE_CROSS_PROFILE_ACCESS";

    private static final ComponentName ADMIN_RECEIVER_COMPONENT =
            new ComponentName(
                    AdminReceiver.class.getPackage().getName(), AdminReceiver.class.getName());
    private static final String PARAM_CROSS_PROFILE_PACKAGE = "crossProfilePackage";
    private static final TestApis sTestApis = new TestApis();

    private final Context mContext = InstrumentationRegistry.getContext();
    private final CrossProfileApps mCrossProfileApps =
            mContext.getSystemService(CrossProfileApps.class);
    private final AppOpsManager mAppOpsManager = mContext.getSystemService(AppOpsManager.class);

    @After
    public void tearDown() {
        InstrumentationRegistry.getInstrumentation().getUiAutomation()
                .dropShellPermissionIdentity();
    }

    @Test
    public void testCanRequestInteractAcrossProfiles_returnsFalse() {
        assertThat(mCrossProfileApps.canRequestInteractAcrossProfiles()).isFalse();
    }

    @Test
    public void testCanRequestInteractAcrossProfiles_returnsTrue() {
        assertThat(mCrossProfileApps.canRequestInteractAcrossProfiles()).isTrue();
    }

    @Test
    public void testCanInteractAcrossProfiles_withAppOpEnabled_returnsTrue() {
        setAppOpOnAllProfiles(AppOpsManager.MODE_ALLOWED);

        assertThat(mCrossProfileApps.canInteractAcrossProfiles()).isTrue();
    }

    @Test
    public void testCanInteractAcrossProfiles_withCrossProfilesPermission_returnsTrue() {
        // Ideally we want to grant the permission in the other profile instead of allowing the
        // appop, however UiAutomation#adoptShellPermission can't be used for multiple UIDs.
        setAppOpOnAllProfiles(AppOpsManager.MODE_ALLOWED, /* includeCallingProfile= */ false);
        setAppOpOnCurrentProfile(AppOpsManager.MODE_IGNORED);

        try (PermissionContext p = sTestApis.permissions().withPermission(
                INTERACT_ACROSS_PROFILES_PERMISSION)) {
            assertThat(mCrossProfileApps.canInteractAcrossProfiles()).isTrue();
        }
    }

    @Test
    public void testCanInteractAcrossProfiles_withCrossUsersPermission_returnsTrue() {
        // Ideally we want to grant the permission in the other profile instead of allowing the
        // appop, however UiAutomation#adoptShellPermission can't be used for multiple UIDs.
        setAppOpOnAllProfiles(AppOpsManager.MODE_ALLOWED, /* includeCallingProfile= */ false);
        setAppOpOnCurrentProfile(AppOpsManager.MODE_IGNORED);

        try (PermissionContext p = sTestApis.permissions().withPermission(
                INTERACT_ACROSS_USERS_PERMISSION)) {
            assertThat(mCrossProfileApps.canInteractAcrossProfiles()).isTrue();
        }
    }

    @Test
    public void testCanInteractAcrossProfiles_withCrossUsersFullPermission_returnsTrue() {
        // Ideally we want to grant the permission in the other profile instead of allowing the
        // appop, however UiAutomation#adoptShellPermission can't be used for multiple UIDs.
        setAppOpOnAllProfiles(AppOpsManager.MODE_ALLOWED, /* includeCallingProfile= */ false);
        setAppOpOnCurrentProfile(AppOpsManager.MODE_IGNORED);

        try (PermissionContext p = sTestApis.permissions().withPermission(
                INTERACT_ACROSS_USERS_FULL_PERMISSION)) {
            assertThat(mCrossProfileApps.canInteractAcrossProfiles()).isTrue();
        }
    }

    @Test
    public void testCanInteractAcrossProfiles_withAppOpDisabledOnCallingProfile_returnsFalse() {
        setAppOpOnAllProfiles(AppOpsManager.MODE_ALLOWED, /* includeCallingProfile= */ false);
        setAppOpOnCurrentProfile(AppOpsManager.MODE_IGNORED);

        assertThat(mCrossProfileApps.canInteractAcrossProfiles()).isFalse();
    }

    @Test
    public void testCanInteractAcrossProfiles_withAppOpDisabledOnOtherProfiles_returnsFalse() {
        setAppOpOnAllProfiles(AppOpsManager.MODE_IGNORED, /* includeCallingProfile= */ false);
        setAppOpOnCurrentProfile(AppOpsManager.MODE_ALLOWED);

        assertThat(mCrossProfileApps.canInteractAcrossProfiles()).isFalse();
    }

    @Test
    public void testCanInteractAcrossProfiles_withNoOtherProfile_returnsFalse() {
        setAppOpOnCurrentProfile(AppOpsManager.MODE_ALLOWED);

        assertThat(mCrossProfileApps.canInteractAcrossProfiles()).isFalse();
    }

    @Test
    public void testCreateRequestInteractAcrossProfilesIntent_canRequestInteraction_returnsIntent() {
        Intent intent = mCrossProfileApps.createRequestInteractAcrossProfilesIntent();

        assertThat(intent).isNotNull();
        assertThat(intent.getAction()).isEqualTo(ACTION_MANAGE_CROSS_PROFILE_ACCESS);
        assertThat(intent.getData()).isNotNull();
        assertThat(intent.getData().getSchemeSpecificPart()).isEqualTo(mContext.getPackageName());
    }

    @Test
    public void testCreateRequestInteractAcrossProfilesIntent_canNotRequestInteraction_throwsSecurityException() {
        try {
            mCrossProfileApps.createRequestInteractAcrossProfilesIntent();
        } catch (SecurityException e) {
            return;
        }
        fail("Should throw a Security Exception");
    }

    /**
     * Calls {@link CrossProfileApps#createRequestInteractAcrossProfilesIntent()}. This can then be
     * used by host-side tests.
     */
    @Test
    public void testCreateRequestInteractAcrossProfilesIntent_noAsserts() {
        mCrossProfileApps.createRequestInteractAcrossProfilesIntent();
    }

    @Test
    public void testSetCrossProfilePackages_noAsserts() {
        final DevicePolicyManager devicePolicyManager =
                (DevicePolicyManager) mContext.getSystemService(Context.DEVICE_POLICY_SERVICE);
        devicePolicyManager.setCrossProfilePackages(
                ADMIN_RECEIVER_COMPONENT, Collections.singleton(getCrossProfilePackage()));
    }

    private String getCrossProfilePackage() {
        final Bundle testArguments = InstrumentationRegistry.getArguments();
        if (testArguments.containsKey(PARAM_CROSS_PROFILE_PACKAGE)) {
            try {
                return testArguments.getString(PARAM_CROSS_PROFILE_PACKAGE);
            } catch (NumberFormatException ignore) {
            }
        }
        fail("cross profile package param not found.");
        return null;
    }

    private void setAppOpOnCurrentProfile(int mode) {
        try (PermissionContext p = sTestApis.permissions().withPermission(
                MANAGE_APP_OPS_MODES_PERMISSION)) {
            mAppOpsManager.setMode(AppOpsManager.OPSTR_INTERACT_ACROSS_PROFILES,
                    Binder.getCallingUid(), mContext.getPackageName(), mode);
        }
    }

    private void setAppOpOnAllProfiles(int mode) {
        setAppOpOnAllProfiles(mode, /* includeCallingProfile= */ true);
    }

    private void setAppOpOnAllProfiles(int mode, boolean includeCallingProfile) {
        try (PermissionContext p = sTestApis.permissions().withPermission(
                MANAGE_APP_OPS_MODES_PERMISSION, INTERACT_ACROSS_USERS_PERMISSION)) {
            for (UserHandle profile : mContext.getSystemService(
                    UserManager.class).getAllProfiles()) {
                if (!includeCallingProfile && profile.getIdentifier() == mContext.getUserId()) {
                    continue;
                }
                try {
                    final int uid = mContext.createContextAsUser(profile, /* flags= */ 0)
                            .getPackageManager().getPackageUid(
                                    mContext.getPackageName(), /* flags= */ 0);
                    mAppOpsManager.setMode(AppOpsManager.OPSTR_INTERACT_ACROSS_PROFILES,
                            uid, mContext.getPackageName(), mode);
                } catch (PackageManager.NameNotFoundException e) {
                    // Do nothing
                }
            }
        }
    }
}
