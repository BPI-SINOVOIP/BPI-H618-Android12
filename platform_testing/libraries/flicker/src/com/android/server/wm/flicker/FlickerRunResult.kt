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
import androidx.annotation.VisibleForTesting
import com.android.server.wm.flicker.assertions.FlickerAssertionError
import com.android.server.wm.flicker.assertions.FlickerSubject
import com.android.server.wm.flicker.dsl.AssertionTag
import com.android.server.wm.flicker.traces.eventlog.EventLogSubject
import com.android.server.wm.traces.common.windowmanager.WindowManagerTrace
import com.android.server.wm.flicker.traces.eventlog.FocusEvent
import com.android.server.wm.flicker.traces.layers.LayersTraceSubject
import com.android.server.wm.flicker.traces.windowmanager.WindowManagerTraceSubject
import com.android.server.wm.traces.common.layers.LayerTraceEntry
import com.android.server.wm.traces.common.layers.LayersTrace
import com.android.server.wm.traces.common.windowmanager.WindowManagerState
import com.android.server.wm.traces.parser.layers.LayersTraceParser
import com.android.server.wm.traces.parser.windowmanager.WindowManagerTraceParser
import java.io.IOException
import java.nio.file.Files
import java.nio.file.Path

/**
 * Defines the result of a flicker run
 */
class FlickerRunResult private constructor(
    /**
     * Run identifier
     */
    @JvmField val iteration: Int,
    /**
     * Path to the trace files associated with the result (incl. screen recording)
     */
    @JvmField val traceFiles: List<Path>,
    /**
     * Determines which assertions to run (e.g., start, end, all, or a custom tag)
     */
    @JvmField var assertionTag: String,
    /**
     * Truth subject that corresponds to a [WindowManagerTrace] or [WindowManagerState]
     */
    private val wmSubject: FlickerSubject?,
    /**
     * Truth subject that corresponds to a [LayersTrace] or [LayerTraceEntry]
     */
    private val layersSubject: FlickerSubject?,
    /**
     * Truth subject that corresponds to a list of [FocusEvent]
     */
    @VisibleForTesting
    val eventLogSubject: EventLogSubject?
) {
    fun getSubjects(): List<FlickerSubject> {
        val result = mutableListOf<FlickerSubject>()

        wmSubject?.run { result.add(this.clone()) }
        layersSubject?.run { result.add(this.clone()) }
        eventLogSubject?.run { result.add(this.clone()) }

        return result
    }

    private fun Path?.tryDelete() {
        try {
            this?.let { Files.deleteIfExists(it) }
        } catch (e: IOException) {
            Log.e(FLICKER_TAG, "Unable do delete $this", e)
        }
    }

    fun canDelete(failures: List<FlickerAssertionError>): Boolean {
        return failures.flatMap { it.traceFiles }.none { failureTrace ->
            this.traceFiles.any { it == failureTrace }
        }
    }

    /**
     * Delete the trace files collected
     */
    fun cleanUp() {
        this.traceFiles.forEach { it.tryDelete() }
    }

    class Builder @JvmOverloads constructor(private val iteration: Int = 0) {
        /**
         * Path to the WindowManager trace file, if collected
         */
        var wmTraceFile: Path? = null

        /**
         * Path to the SurfaceFlinger trace file, if collected
         */
        var layersTraceFile: Path? = null

        /**
         * Path to screen recording of the run, if collected
         */
        var screenRecording: Path? = null

        /**
         * List of focus events, if collected
         */
        var eventLog: List<FocusEvent>? = null

        private fun getTraceFiles() = listOfNotNull(wmTraceFile, layersTraceFile, screenRecording)

        private fun buildResult(
            assertionTag: String,
            wmSubject: FlickerSubject?,
            layersSubject: FlickerSubject?,
            eventLogSubject: EventLogSubject? = null
        ): FlickerRunResult {
            return FlickerRunResult(iteration,
                getTraceFiles(),
                assertionTag,
                wmSubject,
                layersSubject,
                eventLogSubject
            )
        }

        /**
         * Builds a new [FlickerRunResult] for a trace
         *
         * @param assertionTag Tag to associate with the result
         * @param wmTrace WindowManager trace
         * @param layersTrace Layers trace
         */
        fun buildStateResult(
            assertionTag: String,
            wmTrace: WindowManagerTrace?,
            layersTrace: LayersTrace?
        ): FlickerRunResult {
            val wmSubject = wmTrace?.let { WindowManagerTraceSubject.assertThat(it).first() }
            val layersSubject = layersTrace?.let { LayersTraceSubject.assertThat(it).first() }
            return buildResult(assertionTag, wmSubject, layersSubject)
        }

        @VisibleForTesting
        fun buildEventLogResult(): FlickerRunResult {
            val events = eventLog ?: emptyList()
            return buildResult(
                AssertionTag.ALL,
                wmSubject = null,
                layersSubject = null,
                eventLogSubject = EventLogSubject.assertThat(events)
            )
        }

        @VisibleForTesting
        fun buildTraceResults(): List<FlickerRunResult> {
            var wmTrace: WindowManagerTrace? = null
            var layersTrace: LayersTrace? = null

            if (wmTrace == null && wmTraceFile != null) {
                Log.v(FLICKER_TAG, "Parsing WM trace")
                wmTrace = wmTraceFile?.let {
                    val traceData = Files.readAllBytes(it)
                    WindowManagerTraceParser.parseFromTrace(traceData)
                }
            }

            if (layersTrace == null && layersTraceFile != null) {
                Log.v(FLICKER_TAG, "Parsing Layers trace")
                layersTrace = layersTraceFile?.let {
                    val traceData = Files.readAllBytes(it)
                    LayersTraceParser.parseFromTrace(traceData)
                }
            }

            val wmSubject = wmTrace?.let { WindowManagerTraceSubject.assertThat(it) }
            val layersSubject = layersTrace?.let { LayersTraceSubject.assertThat(it) }

            val traceResult = buildResult(
                AssertionTag.ALL, wmSubject, layersSubject)
            val initialStateResult = buildResult(
                AssertionTag.START, wmSubject?.first(), layersSubject?.first())
            val finalStateResult = buildResult(
                AssertionTag.END, wmSubject?.last(), layersSubject?.last())

            return listOf(initialStateResult, finalStateResult, traceResult)
        }

        fun buildAll(): List<FlickerRunResult> {
            val result = buildTraceResults().toMutableList()
            if (eventLog != null) {
                result.add(buildEventLogResult())
            }

            return result
        }
    }
}
