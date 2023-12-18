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

import android.util.Log
import com.android.server.wm.flicker.monitor.ITransitionMonitor
import com.android.server.wm.traces.parser.DeviceStateDump
import com.android.server.wm.traces.parser.getCurrentState
import java.io.IOException
import java.nio.file.Files

/**
 * Runner to execute the transitions of a flicker test
 *
 * The commands are executed in the following order:
 * 1) [Flicker.testSetup]
 * 2) [Flicker.runSetup
 * 3) Start monitors
 * 4) [Flicker.transitions]
 * 5) Stop monitors
 * 6) [Flicker.runTeardown]
 * 7) [Flicker.testTeardown]
 *
 * If the tests were already executed, reuse the previous results
 *
 */
open class TransitionRunner {
    /**
     * Iteration identifier during test run
     */
    private var iteration = 0
    private val tags = mutableSetOf<String>()
    private var tagsResults = mutableListOf<FlickerRunResult>()

    /**
     * Executes the setup, transitions and teardown defined in [flicker]
     *
     * @param flicker test specification
     * @throws IllegalArgumentException If the transitions are empty or repetitions is set to 0
     */
    open fun execute(flicker: Flicker): FlickerResult {
        check(flicker)
        return run(flicker)
    }

    /**
     * Validate the [flicker] test specification before executing the transitions
     *
     * @param flicker test specification
     * @throws IllegalArgumentException If the transitions are empty or repetitions is set to 0
     */
    protected fun check(flicker: Flicker) {
        require(flicker.transitions.isNotEmpty()) {
            "A flicker test must include transitions to run" }
        require(flicker.repetitions > 0) {
            "Number of repetitions must be greater than 0" }
    }

    open fun cleanUp() {
        tags.clear()
        tagsResults.clear()
    }

    /**
     * Runs the actual setup, transitions and teardown defined in [flicker]
     *
     * @param flicker test specification
     */
    internal open fun run(flicker: Flicker): FlickerResult {
        val runs = mutableListOf<FlickerRunResult>()
        var executionError: Throwable? = null
        try {
            try {
                flicker.testSetup.forEach { it.invoke(flicker) }
                for (iteration in 0 until flicker.repetitions) {
                    try {
                        flicker.runSetup.forEach { it.invoke(flicker) }
                        flicker.traceMonitors.forEach { it.start() }
                        flicker.frameStatsMonitor?.run { start() }
                        flicker.transitions.forEach { it.invoke(flicker) }
                    } finally {
                        flicker.traceMonitors.forEach { it.tryStop() }
                        flicker.frameStatsMonitor?.run { tryStop() }
                        flicker.runTeardown.forEach { it.invoke(flicker) }
                    }
                    if (flicker.frameStatsMonitor?.jankyFramesDetected() == true) {
                        Log.e(FLICKER_TAG, "Skipping iteration " +
                            "$iteration/${flicker.repetitions - 1} " +
                            "for test ${flicker.testName} due to jank. $flicker.frameStatsMonitor")
                        continue
                    }
                }
            } finally {
                val runResults = saveResult(flicker, iteration)
                runs.addAll(runResults)
                flicker.testTeardown.forEach { it.invoke(flicker) }
            }
        } catch (e: Throwable) {
            executionError = e
        }

        runs.addAll(tagsResults)
        val result = FlickerResult(runs.toList(), tags.toSet(), executionError)
        cleanUp()
        return result
    }

    private fun saveResult(flicker: Flicker, iteration: Int): List<FlickerRunResult> {
        val resultBuilder = FlickerRunResult.Builder(iteration)
        flicker.traceMonitors.forEach {
            it.save(flicker.testName, iteration, resultBuilder)
        }

        return resultBuilder.buildAll()
    }

    private fun ITransitionMonitor.tryStop() {
        this.run {
            try {
                stop()
            } catch (e: Exception) {
                Log.e(FLICKER_TAG, "Unable to stop $this", e)
            }
        }
    }

    private fun getTaggedFilePath(flicker: Flicker, tag: String, file: String) =
        "${flicker.testName}_${iteration}_${tag}_$file"

    /**
     * Captures a snapshot of the device state and associates it with a new tag.
     *
     * This tag can be used to make assertions about the state of the device when the
     * snapshot is collected.
     *
     * [tag] is used as part of the trace file name, thus, only valid letters and digits
     * can be used
     *
     * @param flicker test specification
     * @throws IllegalArgumentException If [tag] contains invalid characters
     */
    open fun createTag(flicker: Flicker, tag: String) {
        require(!tag.contains(" ")) {
            "The test tag $tag can not contain spaces since it is a part of the file name"
        }
        tags.add(tag)

        val deviceStateBytes = getCurrentState(flicker.instrumentation.uiAutomation)
        val deviceState = DeviceStateDump.fromDump(deviceStateBytes.first, deviceStateBytes.second)
        try {
            val wmTraceFile = flicker.outputDir.resolve(
                getTaggedFilePath(flicker, tag, "wm_trace"))
            Files.write(wmTraceFile, deviceStateBytes.first)

            val layersTraceFile = flicker.outputDir.resolve(
                getTaggedFilePath(flicker, tag, "layers_trace"))
            Files.write(layersTraceFile, deviceStateBytes.second)

            val builder = FlickerRunResult.Builder(iteration)
            builder.wmTraceFile = wmTraceFile
            builder.layersTraceFile = layersTraceFile

            val result = builder.buildStateResult(
                tag,
                deviceState.wmTrace,
                deviceState.layersTrace
            )
            tagsResults.add(result)
        } catch (e: IOException) {
            throw RuntimeException("Unable to create trace file: ${e.message}", e)
        }
    }
}