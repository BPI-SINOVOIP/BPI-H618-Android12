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

package com.android.server.wm.traces.parser.layers

import android.graphics.Rect
import android.surfaceflinger.nano.Layers
import android.surfaceflinger.nano.Layers.RectProto
import android.surfaceflinger.nano.Layers.RegionProto
import android.surfaceflinger.nano.Layerstrace
import android.util.Log
import com.android.server.wm.traces.common.Buffer
import com.android.server.wm.traces.common.Color
import com.android.server.wm.traces.common.RectF
import com.android.server.wm.traces.common.Region
import com.android.server.wm.traces.common.layers.Layer
import com.android.server.wm.traces.common.layers.LayerTraceEntry
import com.android.server.wm.traces.common.layers.LayerTraceEntryBuilder
import com.android.server.wm.traces.common.layers.LayersTrace
import com.android.server.wm.traces.parser.LOG_TAG
import com.google.protobuf.nano.InvalidProtocolBufferNanoException
import java.nio.file.Path
import kotlin.system.measureTimeMillis

/**
 * Parser for [LayersTrace] objects containing traces or state dumps
 **/
class LayersTraceParser {
    companion object {
        /**
         * Parses [LayersTrace] from [data] and uses the proto to generates a list
         * of trace entries, storing the flattened layers into its hierarchical structure.
         *
         * @param data binary proto data
         * @param source Path to source of data for additional debug information
         * @param sourceChecksum Checksum of the source file
         * @param orphanLayerCallback a callback to handle any unexpected orphan layers
         */
        @JvmOverloads
        @JvmStatic
        fun parseFromTrace(
            data: ByteArray,
            source: Path? = null,
            sourceChecksum: String = "",
            orphanLayerCallback: ((Layer) -> Boolean)? = null
        ): LayersTrace {
            val fileProto: Layerstrace.LayersTraceFileProto
            try {
                measureTimeMillis {
                    fileProto = Layerstrace.LayersTraceFileProto.parseFrom(data)
                }.also {
                    Log.v(LOG_TAG, "Parsing proto (Layers Trace): ${it}ms")
                }
            } catch (e: Exception) {
                throw RuntimeException(e)
            }
            return parseFromTrace(fileProto, source, sourceChecksum, orphanLayerCallback)
        }

        /**
         * Parses [LayersTrace] from [proto] and uses the proto to generates a list
         * of trace entries, storing the flattened layers into its hierarchical structure.
         *
         * @param proto Parsed proto data
         * @param source Path to source of data for additional debug information
         * @param sourceChecksum Checksum of the source file
         * @param orphanLayerCallback a callback to handle any unexpected orphan layers
         */
        @JvmOverloads
        @JvmStatic
        fun parseFromTrace(
            proto: Layerstrace.LayersTraceFileProto,
            source: Path? = null,
            sourceChecksum: String = "",
            orphanLayerCallback: ((Layer) -> Boolean)? = null
        ): LayersTrace {
            val entries: MutableList<LayerTraceEntry> = ArrayList()
            var traceParseTime = 0L
            for (traceProto: Layerstrace.LayersTraceProto in proto.entry) {
                val entryParseTime = measureTimeMillis {
                    val entry = newEntry(
                        traceProto.elapsedRealtimeNanos, traceProto.layers.layers,
                        traceProto.hwcBlob, traceProto.where, orphanLayerCallback)
                    entries.add(entry)
                }
                traceParseTime += entryParseTime
            }
            Log.v(LOG_TAG, "Parsing duration (Layers Trace): ${traceParseTime}ms " +
                "(avg ${traceParseTime / entries.size}ms per entry)")
            return LayersTrace(entries, source?.toString() ?: "", sourceChecksum)
        }

        /**
         * Parses [LayersTrace] from [proto] and uses the proto to generates
         * a list of trace entries.
         *
         * @param proto Parsed proto data
         */
        @JvmStatic
        fun parseFromDump(proto: Layers.LayersProto): LayersTrace {
            val entry = newEntry(timestamp = 0, protos = proto.layers)
            return LayersTrace(entry)
        }

        /**
         * Parses [LayersTrace] from [data] and uses the proto to generates
         * a list of trace entries.
         *
         * @param data binary proto data
         */
        @JvmStatic
        fun parseFromDump(data: ByteArray?): LayersTrace {
            val traceProto = try {
                Layers.LayersProto.parseFrom(data)
            } catch (e: InvalidProtocolBufferNanoException) {
                throw RuntimeException(e)
            }
            return parseFromDump(traceProto)
        }

        @JvmStatic
        private fun newEntry(
            timestamp: Long,
            protos: Array<Layers.LayerProto>,
            hwcBlob: String = "",
            where: String = "",
            orphanLayerCallback: ((Layer) -> Boolean)? = null
        ): LayerTraceEntry {
            val layers = protos.map { newLayer(it) }
            val builder = LayerTraceEntryBuilder(timestamp, layers, hwcBlob, where)
            builder.setOrphanLayerCallback(orphanLayerCallback)
            return builder.build()
        }

        @JvmStatic
        private fun newLayer(proto: Layers.LayerProto): Layer {
            // Differentiate between the cases when there's no HWC data on
            // the trace, and when the visible region is actually empty
            val activeBuffer = proto.activeBuffer.toBuffer()
            var visibleRegion = proto.visibleRegion.toRegion()
            if (visibleRegion == null && activeBuffer.isEmpty) {
                visibleRegion = Region.EMPTY
            }
            val crop = getCrop(proto.crop)
            return Layer(
                    proto.name ?: "",
                    proto.id,
                    proto.parent,
                    proto.z,
                    visibleRegion,
                    activeBuffer,
                    proto.flags,
                    proto.bounds?.toRectF() ?: RectF.EMPTY,
                    proto.color.toColor(),
                    proto.isOpaque,
                    proto.shadowRadius,
                    proto.cornerRadius,
                    proto.type ?: "",
                    proto.screenBounds?.toRectF(),
                    Transform(proto.transform, proto.position),
                    proto.sourceBounds?.toRectF(),
                    proto.currFrame,
                    proto.effectiveScalingMode,
                    Transform(proto.bufferTransform, position = null),
                    proto.hwcCompositionType,
                    proto.hwcCrop.toRectF() ?: RectF.EMPTY,
                    proto.hwcFrame.toRect(),
                    proto.backgroundBlurRadius,
                    crop,
                    proto.isRelativeOf,
                    proto.zOrderRelativeOf
            )
        }

        @JvmStatic
        private fun Layers.FloatRectProto?.toRectF(): RectF? {
            return this?.let {
                RectF(left = left, top = top, right = right, bottom = bottom)
            }
        }

        @JvmStatic
        private fun Layers.ColorProto?.toColor(): Color {
            if (this == null) {
                return Color.EMPTY
            }
            return Color(r, g, b, a)
        }

        @JvmStatic
        private fun Layers.ActiveBufferProto?.toBuffer(): Buffer {
            if (this == null) {
                return Buffer.EMPTY
            }
            return Buffer(width, height, stride, format)
        }

        @JvmStatic
        private fun getCrop(crop: RectProto?): com.android.server.wm.traces.common.Rect? {
            return when {
                crop == null -> com.android.server.wm.traces.common.Rect.EMPTY
                // crop (0,0) (-1,-1) means no crop
                crop.right == -1 && crop.left == 0 && crop.bottom == -1 && crop.top == 0 ->
                    null
                (crop.right - crop.left) <= 0 || (crop.bottom - crop.top) <= 0 ->
                    com.android.server.wm.traces.common.Rect.EMPTY
                else ->
                    com.android.server.wm.traces.common.Rect(
                        crop.left, crop.top, crop.right, crop.bottom)
            }
        }

        /**
         * Extracts [Rect] from [RegionProto] by returning a rect that encompasses all
         * the rectangles making up the region.
         */
        @JvmStatic
        private fun RegionProto?.toRegion(): Region? {
            return if (this == null) {
                null
            } else {
                val rects = this.rect.map { it.toRect() }.toTypedArray()
                return Region(rects)
            }
        }

        @JvmStatic
        private fun RectProto?.toRect(): com.android.server.wm.traces.common.Rect {
            return if ((this == null) ||
                ((this.right - this.left) <= 0 || (this.bottom - this.top) <= 0)) {
                com.android.server.wm.traces.common.Rect.EMPTY
            } else {
                com.android.server.wm.traces.common.Rect(
                    this.left, this.top, this.right, this.bottom)
            }
        }
    }
}