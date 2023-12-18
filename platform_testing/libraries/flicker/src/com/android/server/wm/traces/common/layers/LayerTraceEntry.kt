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

import com.android.server.wm.traces.common.ITraceEntry
import com.android.server.wm.traces.common.prettyTimestamp

/**
 * Represents a single Layer trace entry.
 *
 * This is a generic object that is reused by both Flicker and Winscope and cannot
 * access internal Java/Android functionality
 *
 **/
open class LayerTraceEntry constructor(
    override val timestamp: Long, // hierarchical representation of layers
    val hwcBlob: String,
    val where: String,
    _rootLayers: Array<Layer>
) : ITraceEntry {
    val flattenedLayers: Array<Layer> = fillFlattenedLayers(_rootLayers)
    val rootLayers: Array<Layer> get() = flattenedLayers.filter { it.isRootLayer }.toTypedArray()

    private fun fillFlattenedLayers(rootLayers: Array<Layer>): Array<Layer> {
        val opaqueLayers = mutableListOf<Layer>()
        val transparentLayers = mutableListOf<Layer>()
        val layers = mutableListOf<Layer>()
        val roots = rootLayers.fillOcclusionState(
            opaqueLayers, transparentLayers).toMutableList()
        while (roots.isNotEmpty()) {
            val layer = roots.removeAt(0)
            layers.add(layer)
            roots.addAll(layer.children)
        }
        return layers.toTypedArray()
    }

    private fun Array<Layer>.topDownTraversal(): List<Layer> {
        return this
                .sortedBy { it.z }
                .flatMap { it.topDownTraversal() }
    }

    val visibleLayers: Array<Layer>
        get() = flattenedLayers.filter { it.isVisible }.toTypedArray()

    private fun Layer.topDownTraversal(): List<Layer> {
        val traverseList = mutableListOf(this)

        this.children.sortedBy { it.z }
                .forEach { childLayer ->
                    traverseList.addAll(childLayer.topDownTraversal())
                }

        return traverseList
    }

    private fun Array<Layer>.fillOcclusionState(
        opaqueLayers: MutableList<Layer>,
        transparentLayers: MutableList<Layer>
    ): Array<Layer> {
        val traversalList = topDownTraversal().reversed()

        traversalList.forEach { layer ->
            val visible = layer.isVisible

            if (visible) {
                layer.occludedBy.addAll(opaqueLayers
                    .filter { it.contains(layer) && !it.hasRoundedCorners })
                layer.partiallyOccludedBy.addAll(
                    opaqueLayers.filter { it.overlaps(layer) && it !in layer.occludedBy })
                layer.coveredBy.addAll(transparentLayers.filter { it.overlaps(layer) })

                if (layer.isOpaque) {
                    opaqueLayers.add(layer)
                } else {
                    transparentLayers.add(layer)
                }
            }
        }

        return this
    }

    fun getLayerWithBuffer(name: String): Layer? {
        return flattenedLayers.firstOrNull {
            it.name.contains(name) && it.activeBuffer.isNotEmpty
        }
    }

    /**
     * Check if at least one window which matches provided window name is visible.
     */
    fun isVisible(windowName: String): Boolean =
        visibleLayers.any { it.name == windowName }

    override fun toString(): String {
        return prettyTimestamp(timestamp)
    }
}