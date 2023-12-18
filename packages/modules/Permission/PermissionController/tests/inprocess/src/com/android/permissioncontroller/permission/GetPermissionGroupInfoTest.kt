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

package com.android.permissioncontroller.permission

import android.content.Context
import android.os.Build
import androidx.test.filters.SdkSuppress
import androidx.test.platform.app.InstrumentationRegistry
import com.android.permissioncontroller.permission.utils.Utils
import com.google.common.truth.Truth.assertThat
import org.junit.Test
import java.util.concurrent.CountDownLatch
import java.util.concurrent.TimeUnit

@SdkSuppress(minSdkVersion = Build.VERSION_CODES.S, codeName = "S")
class GetPermissionGroupInfoTest {
    private val context = InstrumentationRegistry.getInstrumentation().context as Context
    private val packageManager = context.packageManager
    private val timeoutMs: Long = 10000

    @Test
    fun assertAllPlatformPermGroupPermListsMatch() {
        val groups = Utils.getPlatformPermissionGroups()
        var returnedPerms: List<String>? = null
        for (group in groups) {
            val latch = CountDownLatch(1)
            packageManager.getPlatformPermissionsForGroup(group, context.mainExecutor) {
                returnedPerms = it
                latch.countDown()
            }
            latch.await(timeoutMs, TimeUnit.MILLISECONDS)
            assertThat(returnedPerms).isEqualTo(
                    Utils.getPlatformPermissionNamesOfGroup(group))
        }
    }

    @Test
    fun assertAllPlatformPermGroupsMatch() {
        val groups = Utils.getPlatformPermissionGroups()
        for (group in groups) {
            val perms = Utils.getPlatformPermissionNamesOfGroup(group)
            for (permName in perms) {
                var permGroup: String? = null
                val latch = CountDownLatch(1)
                packageManager.getGroupOfPlatformPermission(permName, context.mainExecutor) {
                    permGroup = it
                    latch.countDown()
                }
                latch.await(timeoutMs, TimeUnit.MILLISECONDS)
                assertThat(permGroup).isEqualTo(Utils.getGroupOfPlatformPermission(permName))
            }
        }
    }
}
