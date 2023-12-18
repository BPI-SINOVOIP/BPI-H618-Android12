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

package com.android.server.wm.flicker

import android.app.Instrumentation
import android.support.test.launcherhelper.ILauncherStrategy
import android.util.Log
import androidx.test.uiautomator.UiDevice
import com.android.server.wm.flicker.assertions.AssertionData
import com.android.server.wm.flicker.monitor.ITransitionMonitor
import com.android.server.wm.flicker.monitor.WindowAnimationFrameStatsMonitor
import com.android.server.wm.traces.common.layers.LayersTrace
import com.android.server.wm.traces.common.windowmanager.WindowManagerTrace
import com.android.server.wm.traces.parser.windowmanager.WindowManagerStateHelper
import java.nio.file.Path

@DslMarker
annotation class FlickerDslMarker

/**
 * Defines the runner for the flicker tests. This component is responsible for running the flicker
 * tests and executing assertions on the traces to check for inconsistent behaviors on
 * [WindowManagerTrace] and [LayersTrace]
 */
@FlickerDslMarker
class Flicker(
    /**
     * Instrumentation to run the tests
     */
    @JvmField val instrumentation: Instrumentation,
    /**
     * Test automation component used to interact with the device
     */
    @JvmField val device: UiDevice,
    /**
     * Strategy used to interact with the launcher
     */
    @JvmField val launcherStrategy: ILauncherStrategy,
    /**
     * Output directory for test results
     */
    @JvmField val outputDir: Path,
    /**
     * Test name used to store the test results
     */
    @JvmField val testName: String,
    /**
     * Number of times the test should be executed
     */
    @JvmField var repetitions: Int,
    /**
     * Monitor for janky frames, when filtering out janky runs
     */
    @JvmField val frameStatsMonitor: WindowAnimationFrameStatsMonitor?,
    /**
     * Enabled tracing monitors
     */
    @JvmField val traceMonitors: List<ITransitionMonitor>,
    /**
     * Commands to be executed before each run
     */
    @JvmField val testSetup: List<Flicker.() -> Any>,
    /**
     * Commands to be executed before the test
     */
    @JvmField val runSetup: List<Flicker.() -> Any>,
    /**
     * Commands to be executed after the test
     */
    @JvmField val testTeardown: List<Flicker.() -> Any>,
    /**
     * Commands to be executed after the run
     */
    @JvmField val runTeardown: List<Flicker.() -> Any>,
    /**
     * Test commands
     */
    @JvmField val transitions: List<Flicker.() -> Any>,
    /**
     * Runner to execute the test transitions
     */
    @JvmField val runner: TransitionRunner,
    /**
     * Helper object for WM Synchronization
     */
    val wmHelper: WindowManagerStateHelper
) {
    var result: FlickerResult? = null
        private set

    /**
     * Executes the test.
     *
     * @throws IllegalStateException If cannot execute the transition
     */
    fun execute(): Flicker = apply {
        val result = runner.execute(this)
        this.result = result
        checkIsExecuted()
    }

    /**
     * Asserts if the transition of this flicker test has ben executed
     */
    private fun checkIsExecuted() {
        if (result == null) {
            execute()
        }
        val error = result?.error
        if (error != null) {
            throw IllegalStateException("Unable to execute transition", error)
        }
    }

    /**
     * Run an assertion on the trace
     *
     * @param assertion Assertion to run
     * @throws AssertionError If the assertions fail or the transition crashed
     */
    fun checkAssertion(assertion: AssertionData) {
        checkIsExecuted()
        val result = result
        requireNotNull(result)

        val failures = result.checkAssertions(listOf(assertion))
        val failureMessage = failures.joinToString("\n") { it.message }

        if (failureMessage.isNotEmpty()) {
            throw AssertionError(failureMessage)
        }
    }

    /**
     * Deletes the traces files for successful assertions and clears the cached runner results
     *
     */
    fun clear() {
        Log.v(FLICKER_TAG, "Cleaning up spec $testName")
        runner.cleanUp()
        result?.cleanUp()
        result = null
    }

    /**
     * Runs a set of commands and, at the end, creates a tag containing the device state
     *
     * @param tag Identifier for the tag to be created
     * @param commands Commands to execute before creating the tag
     * @throws IllegalArgumentException If [tag] cannot be converted to a valid filename
     */
    fun withTag(tag: String, commands: Flicker.() -> Any) {
        commands()
        runner.createTag(this, tag)
    }

    fun createTag(tag: String) {
        withTag(tag) {}
    }

    override fun toString(): String {
        return this.testName
    }
}