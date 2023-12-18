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
package android.permission4.cts

import android.Manifest
import android.app.Instrumentation
import android.app.UiAutomation
import android.app.compat.CompatChanges
import android.content.Context
import android.content.Intent
import android.content.pm.PackageManager
import android.hardware.camera2.CameraManager
import android.os.Process
import android.provider.DeviceConfig
import android.provider.Settings
import android.server.wm.WindowManagerStateHelper
import android.support.test.uiautomator.By
import android.support.test.uiautomator.UiDevice
import android.support.test.uiautomator.UiSelector
import androidx.test.platform.app.InstrumentationRegistry
import com.android.compatibility.common.util.DisableAnimationRule
import com.android.compatibility.common.util.SystemUtil.*
import org.junit.After
import org.junit.Assert.*
import org.junit.Assume.assumeFalse
import org.junit.Assume.assumeTrue
import org.junit.Before
import org.junit.Rule
import org.junit.Test

private const val APP_LABEL = "CtsCameraMicAccess"
private const val USE_CAMERA = "use_camera"
private const val USE_MICROPHONE = "use_microphone"
private const val USE_HOTWORD = "use_hotword"
private const val INTENT_ACTION = "test.action.USE_CAMERA_OR_MIC"
private const val PRIVACY_CHIP_ID = "com.android.systemui:id/privacy_chip"
private const val INDICATORS_FLAG = "camera_mic_icons_enabled"
private const val PERMISSION_INDICATORS_NOT_PRESENT = 162547999L
private const val IDLE_TIMEOUT_MILLIS: Long = 1000
private const val UNEXPECTED_TIMEOUT_MILLIS = 1000
private const val TIMEOUT_MILLIS: Long = 20000
private const val TV_MIC_INDICATOR_WINDOW_TITLE = "MicrophoneCaptureIndicator"

class CameraMicIndicatorsPermissionTest {
    private val instrumentation: Instrumentation = InstrumentationRegistry.getInstrumentation()
    private val context: Context = instrumentation.context
    private val uiAutomation: UiAutomation = instrumentation.uiAutomation
    private val uiDevice: UiDevice = UiDevice.getInstance(instrumentation)
    private val packageManager: PackageManager = context.packageManager

    private var wasEnabled = false
    private val micLabel = packageManager.getPermissionGroupInfo(
        Manifest.permission_group.MICROPHONE, 0).loadLabel(packageManager).toString()
    private val cameraLabel = packageManager.getPermissionGroupInfo(
        Manifest.permission_group.CAMERA, 0).loadLabel(packageManager).toString()

    private var screenTimeoutBeforeTest: Long = 0L

    @get:Rule
    val disableAnimationRule = DisableAnimationRule()

    @Before
    fun setUp() {
        runWithShellPermissionIdentity {
            screenTimeoutBeforeTest = Settings.System.getLong(
                context.contentResolver, Settings.System.SCREEN_OFF_TIMEOUT
            )
            Settings.System.putLong(
                context.contentResolver, Settings.System.SCREEN_OFF_TIMEOUT, 1800000L
            )
        }

        uiDevice.wakeUp()
        runShellCommand(instrumentation, "wm dismiss-keyguard")

        uiDevice.findObject(By.text("Close"))?.click()
        wasEnabled = setIndicatorsEnabledStateIfNeeded(true)
        // If the change Id is not present, then isChangeEnabled will return true. To bypass this,
        // the change is set to "false" if present.
        assumeFalse("feature not present on this device", callWithShellPermissionIdentity {
            CompatChanges.isChangeEnabled(PERMISSION_INDICATORS_NOT_PRESENT, Process.SYSTEM_UID)
        })
    }

    private fun setIndicatorsEnabledStateIfNeeded(shouldBeEnabled: Boolean): Boolean {
        var currentlyEnabled = false
        runWithShellPermissionIdentity {
            currentlyEnabled = DeviceConfig.getBoolean(DeviceConfig.NAMESPACE_PRIVACY,
                INDICATORS_FLAG, true)
            if (currentlyEnabled != shouldBeEnabled) {
                DeviceConfig.setProperty(DeviceConfig.NAMESPACE_PRIVACY, INDICATORS_FLAG,
                    shouldBeEnabled.toString(), false)
            }
        }
        return currentlyEnabled
    }

    @After
    fun tearDown() {
        if (!wasEnabled) {
            setIndicatorsEnabledStateIfNeeded(false)
        }
        runWithShellPermissionIdentity {
            Settings.System.putLong(
                context.contentResolver, Settings.System.SCREEN_OFF_TIMEOUT,
                screenTimeoutBeforeTest
            )
        }

        pressBack()
        pressBack()
        pressHome()
        pressHome()
        Thread.sleep(3000)
    }

    private fun openApp(useMic: Boolean, useCamera: Boolean, useHotword: Boolean) {
        context.startActivity(Intent(INTENT_ACTION).apply {
            putExtra(USE_CAMERA, useCamera)
            putExtra(USE_MICROPHONE, useMic)
            putExtra(USE_HOTWORD, useHotword)
            addFlags(Intent.FLAG_ACTIVITY_NEW_TASK)
        })
    }

    @Test
    fun testCameraIndicator() {
        val manager = context.getSystemService(CameraManager::class.java)!!
        assumeTrue(manager.cameraIdList.isNotEmpty())
        testCameraAndMicIndicator(useMic = false, useCamera = true)
    }

    @Test
    fun testMicIndicator() {
        testCameraAndMicIndicator(useMic = true, useCamera = false)
    }

    @Test
    fun testHotwordIndicatorBehavior() {
        testCameraAndMicIndicator(useMic = false, useCamera = false, useHotword = true)
    }

    private fun testCameraAndMicIndicator(
        useMic: Boolean,
        useCamera: Boolean,
        useHotword: Boolean = false
    ) {
        openApp(useMic, useCamera, useHotword)
        eventually {
            val appView = uiDevice.findObject(UiSelector().textContains(APP_LABEL))
            assertTrue("View with text $APP_LABEL not found", appView.exists())
        }

        if (packageManager.hasSystemFeature(PackageManager.FEATURE_LEANBACK)) {
            assertTvIndicatorsShown(useMic, useCamera, useHotword)
        } else if (packageManager.hasSystemFeature(PackageManager.FEATURE_AUTOMOTIVE)) {
            assertCarIndicatorsShown(useMic, useCamera)
        } else {
            uiDevice.openQuickSettings()
            assertPrivacyChipAndIndicatorsPresent(useMic, useCamera)
        }
    }

    private fun assertTvIndicatorsShown(useMic: Boolean, useCamera: Boolean, useHotword: Boolean) {
        if (useMic || useHotword) {
            val found = WindowManagerStateHelper()
                .waitFor("Waiting for the mic indicator window to come up") {
                    it.containsWindow(TV_MIC_INDICATOR_WINDOW_TITLE) &&
                    it.isWindowVisible(TV_MIC_INDICATOR_WINDOW_TITLE)
                }
            if (useMic) {
                assertTrue("Did not find chip", found)
            } else {
                assertFalse("Found chip, but did not expect to", found)
            }
        }
        if (useCamera) {
            // There is no camera indicator on TVs.
        }
    }

    private fun assertCarIndicatorsShown(useMic: Boolean, useCamera: Boolean) {
        // Ensure the privacy chip is present (or not)
        val chipFound = isChipPresent()
        if (useMic || useCamera) {
            assertTrue("Did not find chip", chipFound)
        } else {
            assertFalse("Found chip, but did not expect to", chipFound)
            return
        }

        eventually {
            if (useMic) {
                val appView = uiDevice.findObject(UiSelector().textContains(micLabel))
                assertTrue("View with text $APP_LABEL not found", appView.exists())
            }
            if (useCamera) {
                // There is no camera indicator in Cars.
            }
            val appView = uiDevice.findObject(UiSelector().textContains(APP_LABEL))
            assertTrue("View with text $APP_LABEL not found", appView.exists())
        }
    }

    private fun assertPrivacyChipAndIndicatorsPresent(useMic: Boolean, useCamera: Boolean) {
        // Ensure the privacy chip is present (or not)
        val chipFound = isChipPresent()
        if (useMic || useCamera) {
            assertTrue("Did not find chip", chipFound)
        } else {
            assertFalse("Found chip, but did not expect to", chipFound)
            return
        }

        eventually {
            if (useMic) {
                val iconView = uiDevice.findObject(UiSelector().descriptionContains(micLabel))
                assertTrue("View with description $micLabel not found", iconView.exists())
            }
            if (useCamera) {
                val iconView = uiDevice.findObject(UiSelector().descriptionContains(cameraLabel))
                assertTrue("View with text $APP_LABEL not found", iconView.exists())
            }
            val appView = uiDevice.findObject(UiSelector().textContains(APP_LABEL))
            assertTrue("View with text $APP_LABEL not found", appView.exists())
        }
    }

    private fun isChipPresent(): Boolean {
        var chipFound = false
        try {
            eventually {
                val privacyChip = uiDevice.findObject(By.res(PRIVACY_CHIP_ID))
                assertNotNull("view with id $PRIVACY_CHIP_ID not found", privacyChip)
                privacyChip.click()
                chipFound = true
            }
        } catch (e: Exception) {
            // Handle more gracefully after
        }
        return chipFound
    }

    private fun pressBack() {
        uiDevice.pressBack()
        waitForIdle()
    }

    private fun pressHome() {
        uiDevice.pressHome()
        waitForIdle()
    }

    private fun waitForIdle() =
        uiAutomation.waitForIdle(IDLE_TIMEOUT_MILLIS, TIMEOUT_MILLIS)
}