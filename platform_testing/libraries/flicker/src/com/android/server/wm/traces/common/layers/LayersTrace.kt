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

package com.android.server.wm.traces.common.layers

import com.android.server.wm.traces.common.ITrace

/**
 * Contains a collection of parsed Layers trace entries and assertions to apply over a single entry.
 *
 * Each entry is parsed into a list of [LayerTraceEntry] objects.
 *
 * This is a generic object that is reused by both Flicker and Winscope and cannot
 * access internal Java/Android functionality
 *
 */
open class LayersTrace(
    override val entries: List<LayerTraceEntry>,
    override val source: String = "",
    override val sourceChecksum: String = ""
) : ITrace<LayerTraceEntry>, List<LayerTraceEntry> by entries {
    constructor(entry: LayerTraceEntry): this(listOf(entry))

    override fun toString(): String {
        return "LayersTrace(Start: ${entries.first()}, " +
            "End: ${entries.last()})"
    }
}