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

package android.permission3.cts

import android.Manifest.permission.ACCESS_BACKGROUND_LOCATION
import android.Manifest.permission.ACCESS_FINE_LOCATION
import android.Manifest.permission.BLUETOOTH_SCAN
import android.app.AppOpsManager
import android.bluetooth.BluetoothAdapter
import android.bluetooth.BluetoothManager
import android.bluetooth.cts.BTAdapterUtils
import android.content.Intent
import android.content.pm.PackageManager
import android.content.pm.PackageManager.FLAG_PERMISSION_REVOKED_COMPAT
import android.os.Build
import android.os.Process
import androidx.test.InstrumentationRegistry
import androidx.test.filters.SdkSuppress
import com.android.compatibility.common.util.SystemUtil.runShellCommandOrThrow
import com.android.compatibility.common.util.SystemUtil.runWithShellPermissionIdentity
import junit.framework.Assert.assertEquals
import junit.framework.Assert.assertTrue
import junit.framework.AssertionFailedError
import org.junit.After
import org.junit.Assert.assertNotEquals
import org.junit.Assume.assumeTrue
import org.junit.Before
import org.junit.Test

/**
 * Runtime Bluetooth-permission behavior of apps targeting API 30
 */
@SdkSuppress(minSdkVersion = Build.VERSION_CODES.S, codeName = "S")
class PermissionTest30WithBluetooth : BaseUsePermissionTest() {

    private val TEST_APP_AUTHORITY =
        "android.permission3.cts.usepermission.AccessBluetoothOnCommand"
    private val TEST_APP_PKG =
        "android.permission3.cts.usepermission"
    private lateinit var bluetoothAdapter: BluetoothAdapter
    private var bluetoothAdapterWasEnabled: Boolean = false

    private enum class BluetoothScanResult {
        UNKNOWN, ERROR, EXCEPTION, EMPTY, FILTERED, FULL
    }

    @Before
    fun installApp() {
        installPackage(APP_APK_PATH_30_WITH_BLUETOOTH)
    }

    private fun reinstallApp() {
        installPackage(APP_APK_PATH_30_WITH_BLUETOOTH, reinstall = true)
    }

    @Before
    fun enableBluetooth() {
        assumeTrue(supportsBluetooth())
        bluetoothAdapter = context.getSystemService(BluetoothManager::class.java).adapter
        bluetoothAdapterWasEnabled = bluetoothAdapter.isEnabled()
        runWithShellPermissionIdentity {
            assertTrue(BTAdapterUtils.enableAdapter(bluetoothAdapter, context))
        }
        enableTestMode()
    }

    @After
    fun disableBluetooth() {
        assumeTrue(supportsBluetooth())
        disableTestMode()
        if (!bluetoothAdapterWasEnabled) {
            runWithShellPermissionIdentity {
                assertTrue(BTAdapterUtils.disableAdapter(bluetoothAdapter, context))
            }
        }
    }

    @Test
    fun testGivenBluetoothIsDeniedWhenScanIsAttemptedThenThenGetEmptyScanResult() {
        assertBluetoothRevokedCompatState(revoked = false)
        // Should return empty while the app does not have location
        assertEquals(BluetoothScanResult.EMPTY, scanForBluetoothDevices())

        uiAutomation.grantRuntimePermission(TEST_APP_PKG, ACCESS_FINE_LOCATION)
        uiAutomation.grantRuntimePermission(TEST_APP_PKG, ACCESS_BACKGROUND_LOCATION)
        runWithShellPermissionIdentity {
            context.getSystemService(AppOpsManager::class.java)!!.setUidMode(
                AppOpsManager.OPSTR_FINE_LOCATION,
                packageManager.getPackageUid(context.packageName, 0), AppOpsManager.MODE_ALLOWED)
        }

        assertEquals(BluetoothScanResult.FULL, scanForBluetoothDevices())
        revokeAppPermissions(BLUETOOTH_SCAN, isLegacyApp = true)
        assertBluetoothRevokedCompatState(revoked = true)
        val res = scanForBluetoothDevices()
        if (res != BluetoothScanResult.ERROR && res != BluetoothScanResult.EMPTY) {
            throw AssertionFailedError("Expected to be EMPTY or ERROR, but was $res")
        }
    }

    @Test
    fun testRevokedCompatPersistsOnReinstall() {
        assertBluetoothRevokedCompatState(revoked = false)
        revokeAppPermissions(BLUETOOTH_SCAN, isLegacyApp = true)
        assertBluetoothRevokedCompatState(revoked = true)
        reinstallApp()
        assertBluetoothRevokedCompatState(revoked = true)
        installApp()
        assertBluetoothRevokedCompatState(revoked = true)
    }

    private fun assertBluetoothRevokedCompatState(revoked: Boolean = true) {
        runWithShellPermissionIdentity {
            val flag = context.packageManager.getPermissionFlags(BLUETOOTH_SCAN,
                TEST_APP_PKG, Process.myUserHandle()) and FLAG_PERMISSION_REVOKED_COMPAT
            if (revoked) {
                assertNotEquals(0, flag)
            } else {
                assertEquals(0, flag)
            }
        }
    }
    private fun scanForBluetoothDevices(): BluetoothScanResult {
        val resolver = InstrumentationRegistry.getTargetContext().getContentResolver()
        val result = resolver.call(TEST_APP_AUTHORITY, "", null, null)
        return BluetoothScanResult.values()[result!!.getInt(Intent.EXTRA_INDEX)]
    }

    private fun supportsBluetooth(): Boolean =
        context.packageManager.hasSystemFeature(PackageManager.FEATURE_BLUETOOTH)

    private fun enableTestMode() = runShellCommandOrThrow("dumpsys activity service" +
        " com.android.bluetooth/.btservice.AdapterService set-test-mode enabled")

    private fun disableTestMode() = runShellCommandOrThrow("dumpsys activity service" +
        " com.android.bluetooth/.btservice.AdapterService set-test-mode disabled")
}
