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

import android.app.Instrumentation
import android.util.Log
import android.view.FrameStats
import com.android.server.wm.flicker.FLICKER_TAG
import kotlin.math.max

/**
 * Monitors [android.view.WindowAnimationFrameStats] to detect janky frames.
 *
 *
 * Adapted from [androidx.test.jank.internal.WindowAnimationFrameStatsMonitorImpl] using
 * the same threshold to determine jank.
 */
open class WindowAnimationFrameStatsMonitor(
    private val instrumentation: Instrumentation
) : ITransitionMonitor {
    private var frameStats: FrameStats? = null
    private var numJankyFrames = 0
    private var longestFrameNano = 0L

    private fun analyze() {
        val stats = frameStats ?: throw IllegalStateException("Frame status are only available " +
                "once the monitor has been stopped")

        val frameCount = stats.frameCount
        val refreshPeriodNano = stats.refreshPeriodNano

        // Skip first frame
        for (i in 2 until frameCount) {
            // Handle frames that have not been presented.
            if (stats.getFramePresentedTimeNano(i) == FrameStats.UNDEFINED_TIME_NANO) {
                // The animation must not have completed. Warn and break out of the loop.
                Log.w(FLICKER_TAG, "Skipping fenced frame.")
                break
            }

            val frameDurationNano =
                    stats.getFramePresentedTimeNano(i) - stats.getFramePresentedTimeNano(i - 1)
            val normalized = frameDurationNano.toDouble() / refreshPeriodNano
            if (normalized < PAUSE_THRESHOLD) {
                if (normalized > 1.0f + MAX_ERROR) {
                    numJankyFrames++
                }
                longestFrameNano = max(longestFrameNano, frameDurationNano)
            }
        }
    }

    override fun start() {
        // Clear out any previous data
        frameStats = null
        numJankyFrames = 0
        longestFrameNano = 0
        instrumentation.uiAutomation.clearWindowAnimationFrameStats()
    }

    override fun stop() {
        frameStats = instrumentation.uiAutomation.windowAnimationFrameStats
        analyze()
    }

    fun jankyFramesDetected(): Boolean = frameStats?.frameCount ?: 0 > 0 && numJankyFrames > 0

    override fun toString(): String = """$frameStats
        RefreshPeriodNano: ${frameStats?.refreshPeriodNano}
        NumJankyFrames: $numJankyFrames
        LongestFrameNano: $longestFrameNano""".trimIndent()

    companion object {
        // Maximum normalized error in frame duration before the frame is considered janky
        private const val MAX_ERROR = 0.5

        // Maximum normalized frame duration before the frame is considered a pause
        private const val PAUSE_THRESHOLD = 15.0
    }
}