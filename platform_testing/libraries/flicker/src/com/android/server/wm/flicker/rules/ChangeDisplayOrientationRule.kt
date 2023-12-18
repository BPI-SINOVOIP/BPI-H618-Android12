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

package com.android.server.wm.flicker.rules

import android.app.Instrumentation
import android.content.Context
import android.os.RemoteException
import android.view.Surface
import android.view.WindowManager
import androidx.test.platform.app.InstrumentationRegistry
import androidx.test.uiautomator.UiDevice
import com.android.server.wm.traces.parser.windowmanager.WindowManagerStateHelper
import org.junit.rules.TestWatcher
import org.junit.runner.Description

data class ChangeDisplayOrientationRule @JvmOverloads constructor(
    private val targetOrientation: Int,
    private val instrumentation: Instrumentation = InstrumentationRegistry.getInstrumentation()
) : TestWatcher() {
    private var initialOrientation = -1

    override fun starting(description: Description?) {
        val wm = instrumentation.context.getSystemService(Context.WINDOW_SERVICE) as WindowManager
        initialOrientation = wm.defaultDisplay.rotation
        setRotation(targetOrientation, instrumentation)
    }

    override fun finished(description: Description?) {
        setRotation(initialOrientation, instrumentation)
    }

    companion object {
        @JvmOverloads
        fun setRotation(
            rotation: Int,
            instrumentation: Instrumentation = InstrumentationRegistry.getInstrumentation(),
            wmHelper: WindowManagerStateHelper = WindowManagerStateHelper(instrumentation)
        ) {
            val device: UiDevice = UiDevice.getInstance(instrumentation)

            try {
                when (rotation) {
                    Surface.ROTATION_270 -> device.setOrientationRight()
                    Surface.ROTATION_90 -> device.setOrientationLeft()
                    Surface.ROTATION_0 -> device.setOrientationNatural()
                    else -> device.setOrientationNatural()
                }

                wmHelper.waitForRotation(rotation)
                // During seamless rotation the app window is shown
                val currWmState = wmHelper.currentState.wmState
                if (currWmState.visibleWindows.none { it.isFullscreen }) {
                    wmHelper.waitForNavBarStatusBarVisible()
                }
                wmHelper.waitForAppTransitionIdle()

                // Ensure WindowManagerService wait until all animations have completed
                instrumentation.uiAutomation.syncInputTransactions()
            } catch (e: RemoteException) {
                throw RuntimeException(e)
            }
        }
    }
}