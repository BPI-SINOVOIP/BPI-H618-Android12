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

import android.os.RemoteException
import android.view.WindowManagerGlobal
import com.android.server.wm.flicker.FlickerRunResult
import com.android.server.wm.flicker.traces.layers.LayersTraceSubject
import com.android.server.wm.traces.common.layers.LayersTrace
import java.nio.file.Path

/**
 * Captures [LayersTrace] from SurfaceFlinger.
 *
 * Use [LayersTraceSubject.assertThat] to make assertions on the trace
 */
open class LayersTraceMonitor(
    outputDir: Path,
    private val traceFlags: Int
) : TransitionMonitor(outputDir, "layers_trace.pb") {

    constructor(outputDir: Path) : this(outputDir, TRACE_FLAGS)

    private val windowManager = WindowManagerGlobal.getWindowManagerService()

    override fun start() {
        try {
            windowManager.setLayerTracingFlags(traceFlags)
            windowManager.isLayerTracing = true
        } catch (e: RemoteException) {
            throw RuntimeException("Could not start trace", e)
        }
    }

    override fun stop() {
        try {
            windowManager.isLayerTracing = false
        } catch (e: RemoteException) {
            throw RuntimeException("Could not stop trace", e)
        }
    }

    override val isEnabled: Boolean
        get() = windowManager.isLayerTracing

    override fun setResult(flickerRunResultBuilder: FlickerRunResult.Builder, traceFile: Path) {
        flickerRunResultBuilder.layersTraceFile = traceFile
    }

    override fun getTracePath(builder: FlickerRunResult.Builder) = builder.layersTraceFile

    companion object {
        const val TRACE_FLAGS = 0x47 // TRACE_CRITICAL|TRACE_INPUT|TRACE_COMPOSITION|TRACE_SYNC
    }
}
