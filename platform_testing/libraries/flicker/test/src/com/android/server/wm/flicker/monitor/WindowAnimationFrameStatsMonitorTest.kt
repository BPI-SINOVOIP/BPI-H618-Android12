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

package com.android.server.wm.flicker.monitor

import androidx.test.platform.app.InstrumentationRegistry
import androidx.test.uiautomator.UiDevice
import com.android.server.wm.flicker.helpers.wakeUpAndGoToHomeScreen
import org.junit.Before
import org.junit.FixMethodOrder
import org.junit.Test
import org.junit.runners.MethodSorters

/**
 * Contains [WindowAnimationFrameStatsMonitor] tests. To run this test: `atest
 * FlickerLibTest:WindowAnimationFrameStatsMonitorTest`
 */
@FixMethodOrder(MethodSorters.NAME_ASCENDING)
class WindowAnimationFrameStatsMonitorTest {
    private val instrumentation = InstrumentationRegistry.getInstrumentation()
    private val frameStatsMonitor = WindowAnimationFrameStatsMonitor(instrumentation)
    private val uiDevice = UiDevice.getInstance(instrumentation)

    @Before
    fun setup() {
        uiDevice.wakeUpAndGoToHomeScreen()
    }

    @Test
    fun captureWindowAnimationFrameStats() {
        frameStatsMonitor.start()
        uiDevice.pressRecentApps()
        frameStatsMonitor.stop()
    }
}
