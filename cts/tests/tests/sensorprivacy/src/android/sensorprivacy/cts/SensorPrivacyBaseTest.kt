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

package android.sensorprivacy.cts

import android.app.KeyguardManager
import android.app.AppOpsManager
import android.content.Intent
import android.content.pm.PackageManager
import android.hardware.SensorPrivacyManager
import android.hardware.SensorPrivacyManager.OnSensorPrivacyChangedListener
import android.os.PowerManager
import android.platform.test.annotations.AppModeFull
import android.hardware.SensorPrivacyManager.Sensors.CAMERA
import android.hardware.SensorPrivacyManager.Sensors.MICROPHONE
import android.hardware.SensorPrivacyManager.Sources.OTHER
import android.support.test.uiautomator.By
import android.view.KeyEvent
import androidx.test.platform.app.InstrumentationRegistry
import androidx.test.uiautomator.UiDevice
import com.android.compatibility.common.util.SystemUtil.callWithShellPermissionIdentity
import com.android.compatibility.common.util.SystemUtil.eventually
import com.android.compatibility.common.util.SystemUtil.runShellCommandOrThrow
import com.android.compatibility.common.util.SystemUtil.runWithShellPermissionIdentity
import com.android.compatibility.common.util.UiAutomatorUtils
import org.junit.After
import org.junit.Assert.assertEquals
import org.junit.Assert.assertFalse
import org.junit.Assert.assertNull
import org.junit.Assert.assertTrue
import org.junit.Assume
import org.junit.Before
import org.junit.Test
import java.util.concurrent.CountDownLatch
import java.util.concurrent.Executors
import java.util.concurrent.TimeUnit
import java.util.regex.Pattern

abstract class SensorPrivacyBaseTest(
    val sensor: Int,
    vararg val extras: String
) {

    companion object {
        const val MIC_CAM_ACTIVITY_ACTION =
                "android.sensorprivacy.cts.usemiccamera.action.USE_MIC_CAM"
        const val FINISH_MIC_CAM_ACTIVITY_ACTION =
                "android.sensorprivacy.cts.usemiccamera.action.FINISH_USE_MIC_CAM"
        const val USE_MIC_EXTRA =
                "android.sensorprivacy.cts.usemiccamera.extra.USE_MICROPHONE"
        const val USE_CAM_EXTRA =
                "android.sensorprivacy.cts.usemiccamera.extra.USE_CAMERA"
        const val DELAYED_ACTIVITY_EXTRA =
                "android.sensorprivacy.cts.usemiccamera.extra.DELAYED_ACTIVITY"
        const val DELAYED_ACTIVITY_NEW_TASK_EXTRA =
                "android.sensorprivacy.cts.usemiccamera.extra.DELAYED_ACTIVITY_NEW_TASK"
        const val PKG_NAME = "android.sensorprivacy.cts.usemiccamera"
        const val RECORDING_FILE_NAME = "${PKG_NAME}_record.mp4"
        const val ACTIVITY_TITLE_SNIP = "CtsUseMic"
        const val SENSOR_USE_TIME_MS = 5L
    }

    protected val instrumentation = InstrumentationRegistry.getInstrumentation()!!
    protected val uiAutomation = instrumentation.uiAutomation!!
    protected val uiDevice = UiDevice.getInstance(instrumentation)!!
    protected val context = instrumentation.targetContext!!
    protected val spm = context.getSystemService(SensorPrivacyManager::class.java)!!
    protected val packageManager = context.packageManager!!
    protected val op = getOpForSensor(sensor)

    var oldState: Boolean = false

    @Before
    fun init() {
        oldState = isSensorPrivacyEnabled()
        setSensor(false)
        Assume.assumeTrue(spm.supportsSensorToggle(sensor))
        uiDevice.wakeUp()
        runShellCommandOrThrow("wm dismiss-keyguard")
        uiDevice.waitForIdle()
    }

    @After
    fun tearDown() {
        finishTestApp()
        Thread.sleep(3000)
        setSensor(oldState)
    }

    @Test
    fun testSetSensor() {
        setSensor(true)
        assertTrue(isSensorPrivacyEnabled())

        setSensor(false)
        assertFalse(isSensorPrivacyEnabled())
    }

    @Test
    fun testDialog() {
        testDialog(delayedActivity = false, delayedActivityNewTask = false)
    }

    @Test
    fun testDialog_remainsOnTop() {
        testDialog(delayedActivity = true, delayedActivityNewTask = false)
    }

    @Test
    fun testDialog_remainsOnTop_newTask() {
        testDialog(delayedActivity = true, delayedActivityNewTask = true)
    }

    fun testDialog(delayedActivity: Boolean = false, delayedActivityNewTask: Boolean = false) {
        try {
            setSensor(true)
            val intent = Intent(MIC_CAM_ACTIVITY_ACTION)
                    .addFlags(Intent.FLAG_ACTIVITY_NEW_TASK)
                    .addFlags(Intent.FLAG_ACTIVITY_MATCH_EXTERNAL)
            for (extra in extras) {
                intent.putExtra(extra, true)
            }
            intent.putExtra(DELAYED_ACTIVITY_EXTRA, delayedActivity)
            intent.putExtra(DELAYED_ACTIVITY_NEW_TASK_EXTRA, delayedActivityNewTask)
            context.startActivity(intent)
            if (delayedActivity || delayedActivityNewTask) {
                Thread.sleep(3000)
            }
            unblockSensorWithDialogAndAssert()
        } finally {
            runShellCommandOrThrow("am broadcast" +
                    " --user ${context.userId}" +
                    " -a $FINISH_MIC_CAM_ACTIVITY_ACTION" +
                    " -f ${Intent.FLAG_RECEIVER_VISIBLE_TO_INSTANT_APPS}")
        }
    }

    @Test
    fun testListener() {
        val executor = Executors.newSingleThreadExecutor()
        setSensor(false)
        val latchEnabled = CountDownLatch(1)
        var listener =
                OnSensorPrivacyChangedListener { _, enabled: Boolean ->
                    if (enabled) {
                        latchEnabled.countDown()
                    }
                }
        runWithShellPermissionIdentity {
            spm.addSensorPrivacyListener(sensor, executor, listener)
        }
        setSensor(true)
        latchEnabled.await(100, TimeUnit.MILLISECONDS)
        runWithShellPermissionIdentity {
            spm.removeSensorPrivacyListener(sensor, listener)
        }

        val latchDisabled = CountDownLatch(1)
        listener = OnSensorPrivacyChangedListener { _, enabled: Boolean ->
            if (!enabled) {
                latchDisabled.countDown()
            }
        }
        runWithShellPermissionIdentity {
            spm.addSensorPrivacyListener(sensor, executor, listener)
        }
        setSensor(false)
        latchEnabled.await(100, TimeUnit.MILLISECONDS)
        runWithShellPermissionIdentity {
            spm.removeSensorPrivacyListener(sensor, listener)
        }
    }

    @Test
    @AppModeFull(reason = "Instant apps can't manage keyguard")
    fun testCantChangeWhenLocked() {
        setSensor(false)
        assertFalse(isSensorPrivacyEnabled())
        runWhileLocked {
            setSensor(true)
            assertFalse("State was changed while device is locked",
                    isSensorPrivacyEnabled())
        }

        setSensor(true)
        assertTrue(isSensorPrivacyEnabled())
        runWhileLocked {
            setSensor(false)
            assertTrue("State was changed while device is locked",
                    isSensorPrivacyEnabled())
        }
    }

    fun unblockSensorWithDialogAndAssert() {
        UiAutomatorUtils.waitFindObject(By.text(
                Pattern.compile("Unblock", Pattern.CASE_INSENSITIVE))).click()
        eventually {
            assertFalse(isSensorPrivacyEnabled())
        }
    }

    @Test
    @AppModeFull(reason = "Uses secondary app, instant apps have no visibility")
    fun testOpNotRunningWhileSensorPrivacyEnabled() {
        setSensor(false)
        val before = System.currentTimeMillis()
        startTestApp()
        eventually {
            assertOpRunning(true)
        }
        Thread.sleep(SENSOR_USE_TIME_MS)
        setSensor(true)
        eventually {
            val after = System.currentTimeMillis()
            assertOpRunning(false)
            assertLastAccessTimeAndDuration(before, after)
        }
    }

    @Test
    @AppModeFull(reason = "Uses secondary app, instant apps have no visibility")
    fun testOpStartsRunningAfterStartedWithSensoryPrivacyEnabled() {
        setSensor(true)
        startTestApp()
        UiAutomatorUtils.waitFindObject(By.text(
                Pattern.compile("Cancel", Pattern.CASE_INSENSITIVE))).click()
        assertOpRunning(false)
        setSensor(false)
        eventually {
            assertOpRunning(true)
        }
    }

    @Test
    @AppModeFull(reason = "Uses secondary app, instant apps have no visibility")
    fun testOpGetsRecordedAfterStartedWithSensorPrivacyEnabled() {
        setSensor(true)
        startTestApp()
        UiAutomatorUtils.waitFindObject(By.text(
                Pattern.compile("Cancel", Pattern.CASE_INSENSITIVE))).click()
        val before = System.currentTimeMillis()
        setSensor(false)
        eventually {
            assertOpRunning(true)
        }
        setSensor(true)
        eventually {
            val after = System.currentTimeMillis()
            assertLastAccessTimeAndDuration(before, after)
        }
    }

    @Test
    @AppModeFull(reason = "Uses secondary app, instant apps have no visibility")
    fun testOpLastAccessUpdatesAfterToggleSensorPrivacy() {
        setSensor(false)
        val before = System.currentTimeMillis()
        startTestApp()
        eventually {
            assertOpRunning(true)
        }
        Thread.sleep(SENSOR_USE_TIME_MS)
        setSensor(true)
        eventually {
            val after = System.currentTimeMillis()
            assertOpRunning(false)
            assertLastAccessTimeAndDuration(before, after)
        }

        val before2 = System.currentTimeMillis()
        setSensor(false)
        eventually {
            assertOpRunning(true)
        }
        Thread.sleep(SENSOR_USE_TIME_MS)
        setSensor(true)
        eventually {
            val after = System.currentTimeMillis()
            assertOpRunning(false)
            assertLastAccessTimeAndDuration(before2, after)
        }
    }

    @Test
    @AppModeFull(reason = "Uses secondary app, instant apps have no visibility")
    fun testOpFinishedWhileToggleOn() {
        setSensor(false)
        startTestApp()
        eventually {
            assertOpRunning(true)
        }
        setSensor(true)
        Thread.sleep(5000)
        eventually {
            assertOpRunning(false)
        }
        finishTestApp()
        Thread.sleep(1000)
        setSensor(false)
        Thread.sleep(1000)
        assertOpRunning(false)
    }

    private fun startTestApp() {
        val intent = Intent(MIC_CAM_ACTIVITY_ACTION)
                .addFlags(Intent.FLAG_ACTIVITY_NEW_TASK)
                .addFlags(Intent.FLAG_ACTIVITY_MATCH_EXTERNAL)
        for (extra in extras) {
            intent.putExtra(extra, true)
        }
        context.startActivity(intent)
        // Wait for app to open
        UiAutomatorUtils.waitFindObject(By.textContains(ACTIVITY_TITLE_SNIP))
    }

    private fun finishTestApp() {
        // instant apps can't broadcast to other instant apps; use the shell
        runShellCommandOrThrow("am broadcast" +
                " --user ${context.userId}" +
                " -a $FINISH_MIC_CAM_ACTIVITY_ACTION" +
                " -f ${Intent.FLAG_RECEIVER_VISIBLE_TO_INSTANT_APPS}")
    }

    protected fun setSensor(enable: Boolean) {
        runWithShellPermissionIdentity {
            spm.setSensorPrivacy(OTHER, sensor, enable)
        }
    }

    private fun isSensorPrivacyEnabled(): Boolean {
        return callWithShellPermissionIdentity {
            spm.isSensorPrivacyEnabled(sensor)
        }
    }

    private fun getOpForSensor(sensor: Int): String? {
        return when (sensor) {
            CAMERA -> AppOpsManager.OPSTR_CAMERA
            MICROPHONE -> AppOpsManager.OPSTR_RECORD_AUDIO
            else -> null
        }
    }

    private fun getOpForPackage(): AppOpsManager.PackageOps {
        return callWithShellPermissionIdentity {
            val uid = try {
                packageManager.getPackageUid(PKG_NAME, 0)
            } catch (e: PackageManager.NameNotFoundException) {
                // fail test
                assertNull(e)
                -1
            }
            val appOpsManager: AppOpsManager =
                    context.getSystemService(AppOpsManager::class.java)!!
            val pkgOps = appOpsManager.getOpsForPackage(uid, PKG_NAME, op)
            assertFalse("expected non empty app op list", pkgOps.isEmpty())
            pkgOps[0]
        }
    }

    private fun assertOpRunning(isRunning: Boolean) {
        val pkgOp = getOpForPackage()
        for (op in pkgOp.ops) {
            for ((_, attrOp) in op.attributedOpEntries) {
                assertEquals("Unexpected op running state", isRunning, attrOp.isRunning)
            }
        }
    }

    private fun assertLastAccessTimeAndDuration(before: Long, after: Long) {
        val pkgOp = getOpForPackage()
        for (op in pkgOp.ops) {
            for ((_, attrOp) in op.attributedOpEntries) {
                val lastAccess = attrOp.getLastAccessTime(AppOpsManager.OP_FLAGS_ALL_TRUSTED)
                val lastDuration = attrOp.getLastDuration(AppOpsManager.OP_FLAGS_ALL_TRUSTED)
                assertTrue("lastAccess was $lastAccess, not between $before and $after",
                        lastAccess in before..after)
                assertTrue("lastAccess had duration $lastDuration, greater than ${after - before}",
                lastDuration <= (after - before))
            }
        }
    }

    fun runWhileLocked(r: () -> Unit) {
        val km = context.getSystemService(KeyguardManager::class.java)!!
        val pm = context.getSystemService(PowerManager::class.java)!!
        val password = byteArrayOf(1, 2, 3, 4)
        try {
            runWithShellPermissionIdentity {
                km!!.setLock(KeyguardManager.PIN, password, KeyguardManager.PIN, null)
            }
            eventually {
                uiDevice.pressKeyCode(KeyEvent.KEYCODE_SLEEP)
                assertFalse("Device never slept.", pm.isInteractive)
            }
            eventually {
                uiDevice.pressKeyCode(KeyEvent.KEYCODE_WAKEUP)
                assertTrue("Device never woke up.", pm.isInteractive)
            }
            eventually {
                assertTrue("Device isn't locked", km.isDeviceLocked)
            }

            r.invoke()
        } finally {
            runWithShellPermissionIdentity {
                km!!.setLock(KeyguardManager.PIN, null, KeyguardManager.PIN, password)
            }

            // Recycle the screen power in case the keyguard is stuck open
            eventually {
                uiDevice.pressKeyCode(KeyEvent.KEYCODE_SLEEP)
                assertFalse("Device never slept.", pm.isInteractive)
            }
            eventually {
                uiDevice.pressKeyCode(KeyEvent.KEYCODE_WAKEUP)
                assertTrue("Device never woke up.", pm.isInteractive)
            }

            eventually {
                assertFalse("Device isn't unlocked", km.isDeviceLocked)
            }
        }
    }
}
