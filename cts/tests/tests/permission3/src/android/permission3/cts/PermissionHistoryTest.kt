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

import android.Manifest
import android.content.Intent
import android.os.Build
import android.support.test.uiautomator.By
import androidx.test.filters.SdkSuppress
import com.android.compatibility.common.util.SystemUtil
import org.junit.After
import org.junit.Before
import org.junit.Ignore
import org.junit.Test

private const val APP_LABEL_1 = "CtsMicAccess"
private const val APP_LABEL_2 = "CtsMicAccess2"
private const val INTENT_ACTION_1 = "test.action.USE_MIC"
private const val INTENT_ACTION_2 = "test.action.USE_MIC_2"
private const val PERMISSION_CONTROLLER_PACKAGE_ID_PREFIX = "com.android.permissioncontroller:id/"
private const val HISTORY_PREFERENCE_ICON = "permission_history_icon"
private const val HISTORY_PREFERENCE_TIME = "permission_history_time"
private const val SHOW_SYSTEM = "Show system"
private const val MORE_OPTIONS = "More options"

@SdkSuppress(minSdkVersion = Build.VERSION_CODES.S)
class PermissionHistoryTest : BasePermissionTest() {
    private val micLabel = packageManager.getPermissionGroupInfo(
            Manifest.permission_group.MICROPHONE, 0).loadLabel(packageManager).toString()

    @Before
    fun installApps() {
        uninstallPackage(APP_PACKAGE_NAME, requireSuccess = false)
        uninstallPackage(APP2_PACKAGE_NAME, requireSuccess = false)
        installPackage(APP_APK_PATH, grantRuntimePermissions = true)
        installPackage(APP2_APK_PATH, grantRuntimePermissions = true)
    }

    @After
    fun uninstallApps() {
        uninstallPackage(APP_PACKAGE_NAME, requireSuccess = false)
        uninstallPackage(APP2_PACKAGE_NAME, requireSuccess = false)
    }

    @Test
    fun testToggleSystemApps() {
        // I had some hard time mocking a system app.
        // Hence here I am only testing if the toggle is there.
        // Will comeback and add the system app for testing if we
        // need the line coverage for this. - theianchen@
        openMicrophoneApp(INTENT_ACTION_1)
        waitFindObject(By.textContains(APP_LABEL_1))

        openMicrophoneTimeline()
        val menuView = waitFindObject(By.descContains(MORE_OPTIONS))
        menuView.click()

        waitFindObject(By.text(SHOW_SYSTEM))
        uiDevice.pressBack()
    }

    @Test
    fun testMicrophoneTimelineWithOneApp() {
        openMicrophoneApp(INTENT_ACTION_1)
        waitFindObject(By.textContains(APP_LABEL_1))

        openMicrophoneTimeline()
        waitFindObject(By.descContains(micLabel))
        waitFindObject(By.textContains(APP_LABEL_1))
        waitFindObject(By.res(
                PERMISSION_CONTROLLER_PACKAGE_ID_PREFIX + HISTORY_PREFERENCE_ICON))
        waitFindObject(By.res(
                PERMISSION_CONTROLLER_PACKAGE_ID_PREFIX + HISTORY_PREFERENCE_TIME))
    }

    @Ignore("b/186656826#comment27")
    @Test
    fun testCameraTimelineWithMultipleApps() {
        openMicrophoneApp(INTENT_ACTION_1)
        waitFindObject(By.textContains(APP_LABEL_1))

        openMicrophoneApp(INTENT_ACTION_2)
        waitFindObject(By.textContains(APP_LABEL_2))

        openMicrophoneTimeline()
        waitFindObject(By.descContains(micLabel))
        waitFindObject(By.textContains(APP_LABEL_1))
        waitFindObject(By.textContains(APP_LABEL_2))
    }

    private fun openMicrophoneApp(intentAction: String) {
        context.startActivity(Intent(intentAction).apply {
            addFlags(Intent.FLAG_ACTIVITY_NEW_TASK)
        })
    }

    private fun openMicrophoneTimeline() {
        SystemUtil.runWithShellPermissionIdentity {
            context.startActivity(Intent(Intent.ACTION_REVIEW_PERMISSION_HISTORY).apply {
                putExtra(Intent.EXTRA_PERMISSION_GROUP_NAME, Manifest.permission_group.MICROPHONE)
                addFlags(Intent.FLAG_ACTIVITY_NEW_TASK)
            })
        }
    }

    companion object {
        const val APP_APK_PATH = "$APK_DIRECTORY/CtsAccessMicrophoneApp.apk"
        const val APP_PACKAGE_NAME = "android.permission3.cts.accessmicrophoneapp"
        const val APP2_APK_PATH = "$APK_DIRECTORY/CtsAccessMicrophoneApp2.apk"
        const val APP2_PACKAGE_NAME = "android.permission3.cts.accessmicrophoneapp2"
    }
}
