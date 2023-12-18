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

package com.android.permissioncontroller.permission.ui

import android.content.Intent
import android.os.UserHandle
import android.os.UserHandle.myUserId
import android.permission.cts.PermissionUtils.install
import android.permission.cts.PermissionUtils.uninstallApp
import androidx.test.ext.junit.runners.AndroidJUnit4
import androidx.test.rule.ActivityTestRule
import com.android.compatibility.common.util.SystemUtil.eventually
import com.android.permissioncontroller.permission.ui.ManagePermissionsActivity.EXTRA_CALLER_NAME
import com.android.permissioncontroller.wakeUpScreen
import com.google.common.truth.Truth.assertThat
import org.junit.After
import org.junit.Assume
import org.junit.Before
import org.junit.Rule
import org.junit.Test
import org.junit.runner.RunWith

/**
 * Simple tests for {@link AppPermissionFragment}
 * Currently, does NOT run on TV.
 * TODO(b/178576541): Adapt and run on TV.
 * Run with:
 * atest AppPermissionFragmentTest
 */
@RunWith(AndroidJUnit4::class)
class AppPermissionFragmentTest : UiBaseTest() {
    private val ONE_PERMISSION_DEFINER_APK =
            "/data/local/tmp/permissioncontroller/tests/inprocess/" +
                "AppThatDefinesAdditionalPermission.apk"
    private val PERMISSION_USER_APK =
            "/data/local/tmp/permissioncontroller/tests/inprocess/" +
                    "AppThatUsesAdditionalPermission.apk"
    private val DEFINER_PKG = "com.android.permissioncontroller.tests.appthatdefinespermission"
    private val USER_PKG = "com.android.permissioncontroller.tests.appthatrequestpermission"

    private val PERM = "com.android.permissioncontroller.tests.A"

    private val managePermissionsActivity = object : ActivityTestRule<ManagePermissionsActivity>(
            ManagePermissionsActivity::class.java) {
        override fun getActivityIntent() = Intent(Intent.ACTION_MANAGE_APP_PERMISSION)
                .putExtra(Intent.EXTRA_PACKAGE_NAME, USER_PKG)
                .putExtra(Intent.EXTRA_PERMISSION_NAME, PERM)
                .putExtra(Intent.EXTRA_PERMISSION_GROUP_NAME, PERM)
                .putExtra(Intent.EXTRA_USER, UserHandle.of(myUserId()))
                .putExtra(EXTRA_CALLER_NAME, "")

        override fun beforeActivityLaunched() {
            install(ONE_PERMISSION_DEFINER_APK)
            install(PERMISSION_USER_APK)
        }
    }

    @Rule
    fun activityRule() = if (isTelevision) noOpTestRule else managePermissionsActivity

    @Before
    fun assumeNotTelevision() = Assume.assumeFalse(isTelevision)

    @Before
    fun wakeScreenUp() {
        wakeUpScreen()
    }

    @Test
    fun activityIsClosedWhenUserIsUninstalled() {
        uninstallApp(USER_PKG)
        eventually {
            assertThat(managePermissionsActivity.activity.isDestroyed()).isTrue()
        }
    }

    @Test
    fun activityIsClosedWhenDefinerIsUninstalled() {
        uninstallApp(DEFINER_PKG)
        eventually {
            assertThat(managePermissionsActivity.activity.isDestroyed()).isTrue()
        }
    }

    @After
    fun uninstallTestApp() {
        uninstallApp(DEFINER_PKG)
        uninstallApp(USER_PKG)
    }
}