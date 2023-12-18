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

package com.android.net.module.util

import android.Manifest.permission.NETWORK_STACK
import android.content.Context
import android.content.pm.PackageManager.PERMISSION_DENIED
import android.content.pm.PackageManager.PERMISSION_GRANTED
import android.net.NetworkStack.PERMISSION_MAINLINE_NETWORK_STACK
import androidx.test.filters.SmallTest
import androidx.test.runner.AndroidJUnit4
import com.android.net.module.util.PermissionUtils.checkAnyPermissionOf
import com.android.net.module.util.PermissionUtils.enforceNetworkStackPermission
import com.android.net.module.util.PermissionUtils.enforceNetworkStackPermissionOr
import com.android.net.module.util.PermissionUtils.enforceAnyPermissionOf
import org.junit.Assert.assertFalse
import org.junit.Assert.assertTrue
import org.junit.Test
import org.junit.runner.RunWith
import org.mockito.ArgumentMatchers.any
import org.mockito.Mockito.doReturn
import org.mockito.Mockito.mock
import kotlin.test.assertFailsWith

/** Tests for PermissionUtils */
@RunWith(AndroidJUnit4::class)
@SmallTest
class PermissionUtilsTest {
    private val TEST_PERMISSION1 = "android.permission.TEST_PERMISSION1"
    private val TEST_PERMISSION2 = "android.permission.TEST_PERMISSION2"
    private val context = mock(Context::class.java)

    @Test
    fun testEnforceAnyPermissionOf() {
        doReturn(PERMISSION_GRANTED).`when`(context).checkCallingOrSelfPermission(TEST_PERMISSION1)
        doReturn(PERMISSION_DENIED).`when`(context).checkCallingOrSelfPermission(TEST_PERMISSION2)
        assertTrue(checkAnyPermissionOf(context, TEST_PERMISSION1, TEST_PERMISSION2))
        enforceAnyPermissionOf(context, TEST_PERMISSION1, TEST_PERMISSION2)

        doReturn(PERMISSION_DENIED).`when`(context).checkCallingOrSelfPermission(TEST_PERMISSION1)
        doReturn(PERMISSION_GRANTED).`when`(context).checkCallingOrSelfPermission(TEST_PERMISSION2)
        assertTrue(checkAnyPermissionOf(context, TEST_PERMISSION1, TEST_PERMISSION2))
        enforceAnyPermissionOf(context, TEST_PERMISSION1, TEST_PERMISSION2)

        doReturn(PERMISSION_DENIED).`when`(context).checkCallingOrSelfPermission(any())
        assertFalse(checkAnyPermissionOf(context, TEST_PERMISSION1, TEST_PERMISSION2))
        assertFailsWith<SecurityException>("Expect fail but permission granted.") {
            enforceAnyPermissionOf(context, TEST_PERMISSION1, TEST_PERMISSION2) }
    }

    @Test
    fun testEnforceNetworkStackPermissionOr() {
        doReturn(PERMISSION_GRANTED).`when`(context).checkCallingOrSelfPermission(NETWORK_STACK)
        doReturn(PERMISSION_DENIED).`when`(context)
                .checkCallingOrSelfPermission(PERMISSION_MAINLINE_NETWORK_STACK)
        enforceNetworkStackPermission(context)
        enforceNetworkStackPermissionOr(context, TEST_PERMISSION1)

        doReturn(PERMISSION_DENIED).`when`(context).checkCallingOrSelfPermission(NETWORK_STACK)
        doReturn(PERMISSION_GRANTED).`when`(context)
                .checkCallingOrSelfPermission(PERMISSION_MAINLINE_NETWORK_STACK)
        enforceNetworkStackPermission(context)
        enforceNetworkStackPermissionOr(context, TEST_PERMISSION2)

        doReturn(PERMISSION_DENIED).`when`(context).checkCallingOrSelfPermission(NETWORK_STACK)
        doReturn(PERMISSION_DENIED).`when`(context)
                .checkCallingOrSelfPermission(PERMISSION_MAINLINE_NETWORK_STACK)
        doReturn(PERMISSION_GRANTED).`when`(context).checkCallingOrSelfPermission(TEST_PERMISSION1)
        assertFailsWith<SecurityException>("Expect fail but permission granted.") {
            enforceNetworkStackPermission(context) }
        enforceNetworkStackPermissionOr(context, TEST_PERMISSION1)

        doReturn(PERMISSION_DENIED).`when`(context).checkCallingOrSelfPermission(any())
        assertFailsWith<SecurityException>("Expect fail but permission granted.") {
            enforceNetworkStackPermission(context) }
        assertFailsWith<SecurityException>("Expect fail but permission granted.") {
            enforceNetworkStackPermissionOr(context, TEST_PERMISSION2) }
    }
}
