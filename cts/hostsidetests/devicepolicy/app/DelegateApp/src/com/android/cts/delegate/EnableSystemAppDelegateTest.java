/*
 * Copyright (C) 2017 The Android Open Source Project
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
package com.android.cts.delegate;

import static android.app.admin.DevicePolicyManager.DELEGATION_ENABLE_SYSTEM_APP;

import static com.android.cts.delegate.DelegateTestUtils.assertExpectException;

import android.app.admin.DevicePolicyManager;
import android.content.Intent;

import java.util.List;

/**
 * Test that an app given the {@link DevicePolicyManager#DELEGATION_PERMISSION_GRANT} scope via
 * {@link DevicePolicyManager#setDelegatedScopes} can grant permissions and check permission grant
 * state.
 */
public class EnableSystemAppDelegateTest extends BaseJUnit3TestCase {

    private static final String TEST_APP_PKG = "com.android.cts.launcherapps.simpleapp";

    public void testCannotAccessApis() {
        assertFalse("DelegateApp should not be an enable system app delegate",
            amIEnableSystemAppDelegate());

        // Exercise enableSystemApp(String).
        assertExpectException(SecurityException.class,
                "Calling identity is not authorized", () -> {
                    mDpm.enableSystemApp(null, TEST_APP_PKG);
                });

        // Exercise enableSystemApp(Intent).
        assertExpectException(SecurityException.class,
                "Calling identity is not authorized", () -> {
                    mDpm.enableSystemApp(null, new Intent().setPackage(TEST_APP_PKG));
                });
    }

    public void testCanAccessApis() {
        assertTrue("DelegateApp is not an enable system app delegate",
            amIEnableSystemAppDelegate());

        // Exercise enableSystemApp(String).
        assertExpectException(IllegalArgumentException.class,
                "Only system apps can be enabled this way", () -> {
                    mDpm.enableSystemApp(null, TEST_APP_PKG);
                });

        // Exercise enableSystemApp(Intent).
        mDpm.enableSystemApp(null, new Intent());
    }

    private boolean amIEnableSystemAppDelegate() {
        final String packageName = getInstrumentation().getContext().getPackageName();
        final List<String> scopes = mDpm.getDelegatedScopes(null, packageName);
        return scopes.contains(DELEGATION_ENABLE_SYSTEM_APP);
    }
}
