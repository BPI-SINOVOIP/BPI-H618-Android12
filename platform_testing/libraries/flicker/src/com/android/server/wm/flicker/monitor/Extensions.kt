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

@file:JvmName("Extensions")
package com.android.server.wm.flicker.monitor

import com.android.server.wm.traces.parser.DeviceStateDump
import com.android.server.wm.traces.common.windowmanager.WindowManagerTrace
import com.android.server.wm.flicker.getDefaultFlickerOutputDir
import com.android.server.wm.traces.common.layers.LayersTrace
import com.android.server.wm.traces.parser.layers.LayersTraceParser
import com.android.server.wm.traces.parser.windowmanager.WindowManagerTraceParser
import java.nio.file.Path

/**
 * Acquire the [WindowManagerTrace] with the device state changes that happen when executing
 * the commands defined in the [predicate].
 *
 * @param outputDir Directory where to store the traces
 * @param predicate Commands to execute
 * @throws UnsupportedOperationException If tracing is already activated
 */
@JvmOverloads
fun withWMTracing(
    outputDir: Path = getDefaultFlickerOutputDir().resolve("withWMTracing"),
    predicate: () -> Unit
): WindowManagerTrace {
    return WindowManagerTraceParser.parseFromTrace(
        WindowManagerTraceMonitor(outputDir).withTracing(predicate))
}

/**
 * Acquire the [LayersTrace] with the device state changes that happen when executing
 * the commands defined in the [predicate].
 *
 * @param traceFlags Flags to indicate tracing level
 * @param outputDir Directory where to store the traces
 * @param predicate Commands to execute
 * @throws UnsupportedOperationException If tracing is already activated
 */
@JvmOverloads
fun withSFTracing(
    traceFlags: Int = LayersTraceMonitor.TRACE_FLAGS,
    outputDir: Path = getDefaultFlickerOutputDir().resolve("withSFTracing"),
    predicate: () -> Unit
): LayersTrace {
    return LayersTraceParser.parseFromTrace(
        LayersTraceMonitor(outputDir, traceFlags).withTracing(predicate))
}

/**
 * Acquire the [WindowManagerTrace] and [LayersTrace] with the device state changes that happen
 * when executing the commands defined in the [predicate].
 *
 * @param outputDir Directory where to store the traces
 * @param predicate Commands to execute
 * @throws UnsupportedOperationException If tracing is already activated
 */
@JvmOverloads
fun withTracing(
    outputDir: Path = getDefaultFlickerOutputDir(),
    predicate: () -> Unit
): DeviceStateDump {
    val traces = recordTraces(outputDir, predicate)
    val wmTraceData = traces.first
    val layersTraceData = traces.second
    return DeviceStateDump.fromTrace(wmTraceData, layersTraceData)
}

/**
 * Acquire the [WindowManagerTrace] and [LayersTrace] with the device state changes that happen
 * when executing the commands defined in the [predicate].
 *
 * @param outputDir Directory where to store the traces
 * @param predicate Commands to execute
 * @throws UnsupportedOperationException If tracing is already activated
 * @return a pair containing the WM and SF traces
 */
@JvmOverloads
fun recordTraces(
    outputDir: Path = getDefaultFlickerOutputDir(),
    predicate: () -> Unit
): Pair<ByteArray, ByteArray> {
    var wmTraceData = ByteArray(0)
    val layersOutputDir = outputDir.resolve("withSFTracing")
    val layersTraceData = LayersTraceMonitor(layersOutputDir).withTracing {
        val wmOutputDir = outputDir.resolve("withWMTracing")
        wmTraceData = WindowManagerTraceMonitor(wmOutputDir).withTracing(predicate)
    }

    return Pair(wmTraceData, layersTraceData)
}