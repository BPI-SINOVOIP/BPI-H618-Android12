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

package com.android.permissioncontroller.permission.ui.handheld.dashboard

import android.Manifest.permission.CAMERA
import android.content.Intent
import android.os.Build
import android.permission.cts.PermissionUtils.grantPermission
import android.permission.cts.PermissionUtils.install
import android.permission.cts.PermissionUtils.uninstallApp
import androidx.test.espresso.Espresso.onView
import androidx.test.espresso.action.ViewActions.click
import androidx.test.espresso.matcher.ViewMatchers.withContentDescription
import androidx.test.espresso.matcher.ViewMatchers.withResourceName
import androidx.test.espresso.matcher.ViewMatchers.withText
import androidx.test.ext.junit.runners.AndroidJUnit4
import androidx.test.filters.SdkSuppress
import androidx.test.rule.ActivityTestRule
import com.android.compatibility.common.util.SystemUtil.eventually
import com.android.permissioncontroller.R
import com.android.permissioncontroller.permission.PermissionHub2Test
import com.android.permissioncontroller.permission.ui.ManagePermissionsActivity
import com.android.permissioncontroller.scrollToPreference
import org.hamcrest.Matchers.allOf
import org.junit.After
import org.junit.Rule
import org.junit.Test
import org.junit.runner.RunWith
import com.android.permissioncontroller.wakeUpScreen

/**
 * Simple tests for {@link PermissionUsageV2Fragment}
 */
@RunWith(AndroidJUnit4::class)
@SdkSuppress(minSdkVersion = Build.VERSION_CODES.S)
class PermissionUsageV2FragmentTest : PermissionHub2Test() {
    private val APK =
            "/data/local/tmp/permissioncontroller/tests/inprocess/AppThatUsesCameraPermission.apk"
    private val APP = "com.android.permissioncontroller.tests.appthatrequestpermission"
    private val APP_LABEL = "CameraRequestApp"
    private val CAMERA_PREF_LABEL = "Camera"

    @get:Rule
    val managePermissionsActivity = object : ActivityTestRule<ManagePermissionsActivity>(
            ManagePermissionsActivity::class.java) {
        override fun getActivityIntent() = Intent(Intent.ACTION_REVIEW_PERMISSION_USAGE)

        override fun beforeActivityLaunched() {
            wakeUpScreen()
            install(APK)
            grantPermission(APP, CAMERA)

            accessCamera()
        }
    }

    @Test
    fun cameraAccessShouldBeListed() {

        eventually {
            try {
                onView(allOf(withText(CAMERA_PREF_LABEL), withResourceName("title")))
                        .perform(click())
            } catch (e: Exception) {
                onView(withContentDescription(R.string.permission_usage_refresh)).perform(click())
                throw e
            }
        }

        scrollToPreference(APP_LABEL)
    }

    @After
    fun uninstallTestApp() {
        uninstallApp(APP)
    }
}
