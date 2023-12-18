/*
 * Copyright (C) 2019 The Android Open Source Project
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

package android.os.cts

import android.companion.CompanionDeviceManager
import android.content.pm.PackageManager.FEATURE_AUTOMOTIVE
import android.content.pm.PackageManager.FEATURE_COMPANION_DEVICE_SETUP
import android.content.pm.PackageManager.PERMISSION_GRANTED
import android.net.MacAddress
import android.os.Binder
import android.os.Bundle
import android.os.Parcelable
import android.os.UserHandle
import android.platform.test.annotations.AppModeFull
import android.test.InstrumentationTestCase
import android.util.Size
import android.util.SizeF
import android.util.SparseArray
import android.view.accessibility.AccessibilityNodeInfo
import android.view.accessibility.AccessibilityNodeInfo.ACTION_ARGUMENT_SET_TEXT_CHARSEQUENCE
import android.view.accessibility.AccessibilityNodeInfo.ACTION_SET_TEXT
import android.widget.EditText
import android.widget.TextView
import androidx.test.InstrumentationRegistry
import androidx.test.runner.AndroidJUnit4
import com.android.compatibility.common.util.MatcherUtils
import com.android.compatibility.common.util.MatcherUtils.hasIdThat
import com.android.compatibility.common.util.SystemUtil.eventually
import com.android.compatibility.common.util.SystemUtil.getEventually
import com.android.compatibility.common.util.SystemUtil.runShellCommand
import com.android.compatibility.common.util.SystemUtil.runShellCommandOrThrow
import com.android.compatibility.common.util.SystemUtil.runWithShellPermissionIdentity
import com.android.compatibility.common.util.ThrowingSupplier
import com.android.compatibility.common.util.UiAutomatorUtils.waitFindObject
import com.android.compatibility.common.util.children
import com.android.compatibility.common.util.click
import org.hamcrest.CoreMatchers.`is`
import org.hamcrest.CoreMatchers.containsString
import org.hamcrest.CoreMatchers.equalTo
import org.hamcrest.Matcher
import org.hamcrest.Matchers.empty
import org.hamcrest.Matchers.not
import org.junit.Assert.assertThat
import org.junit.Assume.assumeFalse
import org.junit.Assume.assumeTrue
import org.junit.Before
import org.junit.After
import org.junit.Test
import org.junit.runner.RunWith
import java.io.Serializable

const val COMPANION_APPROVE_WIFI_CONNECTIONS =
        "android.permission.COMPANION_APPROVE_WIFI_CONNECTIONS"
const val DUMMY_MAC_ADDRESS = "00:00:00:00:00:10"
const val MANAGE_COMPANION_DEVICES = "android.permission.MANAGE_COMPANION_DEVICES"
const val SHELL_PACKAGE_NAME = "com.android.shell"
val InstrumentationTestCase.context get() = InstrumentationRegistry.getTargetContext()

/**
 * Test for [CompanionDeviceManager]
 */
@RunWith(AndroidJUnit4::class)
class CompanionDeviceManagerTest : InstrumentationTestCase() {

    val cdm: CompanionDeviceManager by lazy {
        context.getSystemService(CompanionDeviceManager::class.java)
    }

    private fun isShellAssociated(macAddress: String, packageName: String): Boolean {
        val userId = context.userId
        return runShellCommand("cmd companiondevice list $userId")
                .lines()
                .any {
                    packageName in it && macAddress in it
                }
    }

    private fun isCdmAssociated(
        macAddress: String,
        packageName: String,
        vararg permissions: String
    ): Boolean {
        return runWithShellPermissionIdentity(ThrowingSupplier {
            cdm.isDeviceAssociatedForWifiConnection(packageName,
                    MacAddress.fromString(macAddress), context.user)
        }, *permissions)
    }

    @Before
    fun assumeHasFeature() {
        assumeTrue(context.packageManager.hasSystemFeature(FEATURE_COMPANION_DEVICE_SETUP))
        // TODO(b/191699828) test does not work in automotive due to accessibility issue
        assumeFalse(context.packageManager.hasSystemFeature(FEATURE_AUTOMOTIVE))
    }

    @After
    fun removeAllAssociations() {
        val packageName = "android.os.cts.companiontestapp"
        val userId = context.userId
        val associations = getAssociatedDevices(packageName)

        for (address in associations) {
            runShellCommandOrThrow("cmd companiondevice disassociate $userId $packageName $address")
        }
    }

    @AppModeFull(reason = "Companion API for non-instant apps only")
    @Test
    fun testIsDeviceAssociated() {
        val userId = context.userId
        val packageName = context.packageName

        assertFalse(isCdmAssociated(DUMMY_MAC_ADDRESS, packageName, MANAGE_COMPANION_DEVICES))
        assertFalse(isShellAssociated(DUMMY_MAC_ADDRESS, packageName))

        try {
            runShellCommand(
                    "cmd companiondevice associate $userId $packageName $DUMMY_MAC_ADDRESS")
            assertTrue(isCdmAssociated(DUMMY_MAC_ADDRESS, packageName, MANAGE_COMPANION_DEVICES))
            assertTrue(isShellAssociated(DUMMY_MAC_ADDRESS, packageName))
        } finally {
            runShellCommand(
                    "cmd companiondevice disassociate $userId $packageName $DUMMY_MAC_ADDRESS")
        }
    }

    @AppModeFull(reason = "Companion API for non-instant apps only")
    @Test
    fun testIsDeviceAssociatedWithCompanionApproveWifiConnectionsPermission() {
        assertTrue(isCdmAssociated(
            DUMMY_MAC_ADDRESS, SHELL_PACKAGE_NAME, MANAGE_COMPANION_DEVICES,
            COMPANION_APPROVE_WIFI_CONNECTIONS))
        assertFalse(isShellAssociated(DUMMY_MAC_ADDRESS, SHELL_PACKAGE_NAME))
    }

    @AppModeFull(reason = "Companion API for non-instant apps only")
    @Test
    fun testDump() {
        val userId = context.userId
        val packageName = context.packageName

        try {
            runShellCommand(
                    "cmd companiondevice associate $userId $packageName $DUMMY_MAC_ADDRESS")
            val output = runShellCommand("dumpsys companiondevice")
            assertThat(output, containsString(packageName))
            assertThat(output, containsString(DUMMY_MAC_ADDRESS))
        } finally {
            runShellCommand(
                    "cmd companiondevice disassociate $userId $packageName $DUMMY_MAC_ADDRESS")
        }
    }

    @AppModeFull(reason = "Companion API for non-instant apps only")
    @Test
    fun testProfiles() {
        val packageName = "android.os.cts.companiontestapp"
        installApk(
                "--user ${UserHandle.myUserId()} /data/local/tmp/cts/os/CtsCompanionTestApp.apk")
        startApp(packageName)

        waitFindNode(hasClassThat(`is`(equalTo(EditText::class.java.name))))
                .performAction(ACTION_SET_TEXT,
                        bundleOf(ACTION_ARGUMENT_SET_TEXT_CHARSEQUENCE to ""))
        waitForIdle()

        click("Watch")
        val device = getEventually({
            click("Associate")
            waitFindNode(hasIdThat(containsString("device_list")),
                    failMsg = "Test requires a discoverable bluetooth device nearby",
                    timeoutMs = 9_000)
                    .children
                    .find { it.className == TextView::class.java.name }
                    .assertNotNull { "Empty device list" }
        }, 90_000)
        device!!.click()

        eventually {
            assertThat(getAssociatedDevices(packageName), not(empty()))
        }
        val deviceAddress = getAssociatedDevices(packageName).last()

        runShellCommandOrThrow("cmd companiondevice simulate_connect $deviceAddress")
        assertPermission(packageName, "android.permission.CALL_PHONE", PERMISSION_GRANTED)

        runShellCommandOrThrow("cmd companiondevice simulate_disconnect $deviceAddress")
        assertPermission(packageName, "android.permission.CALL_PHONE", PERMISSION_GRANTED)
    }

    @AppModeFull(reason = "Companion API for non-instant apps only")
    @Test
    fun testRequestNotifications() {
        val packageName = "android.os.cts.companiontestapp"
        installApk(
                "--user ${UserHandle.myUserId()} /data/local/tmp/cts/os/CtsCompanionTestApp.apk")
        startApp(packageName)

        waitFindNode(hasClassThat(`is`(equalTo(EditText::class.java.name))))
                .performAction(ACTION_SET_TEXT,
                        bundleOf(ACTION_ARGUMENT_SET_TEXT_CHARSEQUENCE to ""))
        waitForIdle()

        val deviceForAssociation = getEventually({
            click("Associate")
            waitFindNode(hasIdThat(containsString("device_list")),
                    failMsg = "Test requires a discoverable bluetooth device nearby",
                    timeoutMs = 5_000)
                    .children
                    .find { it.className == TextView::class.java.name }
                    .assertNotNull { "Empty device list" }
        }, 60_000)

        deviceForAssociation!!.click()

        waitForIdle()

        val deviceForNotifications = getEventually({
            click("Request Notifications")
            waitFindNode(hasIdThat(containsString("button1")),
                    failMsg = "The Request Notifications dialog is not showing up",
                    timeoutMs = 5_000)
                    .assertNotNull { "Request Notifications is not implemented" }
        }, 60_000)

        deviceForNotifications!!.click()

        waitForIdle()
    }

    private fun getAssociatedDevices(
        pkg: String,
        user: UserHandle = android.os.Process.myUserHandle()
    ): List<String> {
        return runShellCommandOrThrow("cmd companiondevice list ${user.identifier}")
                .lines()
                .filter { it.startsWith(pkg) }
                .map { it.substringAfterLast(" ") }
    }
}

private fun click(label: String) {
    waitFindObject(byTextIgnoreCase(label)).click()
    waitForIdle()
}

fun hasClassThat(condition: Matcher<in String?>?): Matcher<AccessibilityNodeInfo> {
    return MatcherUtils.propertyMatches(
            "class",
            { obj: AccessibilityNodeInfo -> obj.className },
            condition)
}

fun bundleOf(vararg entries: Pair<String, Any>) = Bundle().apply {
    entries.forEach { (k, v) -> set(k, v) }
}

operator fun Bundle.set(key: String, value: Any?) {
    if (value is Array<*> && value.isArrayOf<Parcelable>()) {
        putParcelableArray(key, value as Array<Parcelable>)
        return
    }
    if (value is Array<*> && value.isArrayOf<CharSequence>()) {
        putCharSequenceArray(key, value as Array<CharSequence>)
        return
    }
    when (value) {
        is Byte -> putByte(key, value)
        is Char -> putChar(key, value)
        is Short -> putShort(key, value)
        is Float -> putFloat(key, value)
        is CharSequence -> putCharSequence(key, value)
        is Parcelable -> putParcelable(key, value)
        is Size -> putSize(key, value)
        is SizeF -> putSizeF(key, value)
        is ArrayList<*> -> putParcelableArrayList(key, value as ArrayList<Parcelable>)
        is SparseArray<*> -> putSparseParcelableArray(key, value as SparseArray<Parcelable>)
        is Serializable -> putSerializable(key, value)
        is ByteArray -> putByteArray(key, value)
        is ShortArray -> putShortArray(key, value)
        is CharArray -> putCharArray(key, value)
        is FloatArray -> putFloatArray(key, value)
        is Bundle -> putBundle(key, value)
        is Binder -> putBinder(key, value)
        else -> throw IllegalArgumentException("" + value)
    }
}