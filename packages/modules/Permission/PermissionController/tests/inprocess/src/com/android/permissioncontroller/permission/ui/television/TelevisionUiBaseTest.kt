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

package com.android.permissioncontroller.permission.ui.television

import android.Manifest.permission_group.CAMERA
import android.Manifest.permission_group.SENSORS
import android.content.Intent
import com.android.permissioncontroller.R
import com.android.permissioncontroller.permission.ui.UiBaseTest
import com.android.permissioncontroller.permission.ui.uninstallTestApps
import com.android.permissioncontroller.permission.utils.KotlinUtils.getPermGroupLabel
import org.junit.After
import org.junit.Assume.assumeTrue
import org.junit.Before

abstract class TelevisionUiBaseTest : UiBaseTest() {
    val bodySensorsPermissionLabel by lazy { getPermGroupLabel(targetContext, SENSORS) }
    val cameraPermissionLabel by lazy { getPermGroupLabel(targetContext, CAMERA) }
    val otherPermissionsLabel by lazy { targetContext.getString(R.string.other_permissions_label) }
    val additionalPermissionsLabel by lazy {
        targetContext.getString(R.string.additional_permissions_label)
    }

    @Before
    fun assumeTelevision() = assumeTrue(isTelevision)

    @Before
    fun wakeUpAndGoToHomeScreen() {
        uiDevice.wakeUp()
        uiDevice.pressHome()
    }

    @After
    fun cleanUp() = uninstallTestApps()

    protected fun launchPermissionController() {
        Intent(Intent.ACTION_MANAGE_PERMISSIONS)
            .apply { addFlags(Intent.FLAG_ACTIVITY_NEW_TASK) }
            .also { instrumentationContext.startActivity(it) }
        uiDevice.waitForIdle()
    }
}