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

package com.android.server.wm.flicker.traces.layers

import com.android.server.wm.flicker.assertions.Assertion
import com.android.server.wm.flicker.assertions.FlickerSubject
import com.android.server.wm.flicker.containsAny
import com.android.server.wm.flicker.traces.FlickerFailureStrategy
import com.android.server.wm.flicker.traces.FlickerSubjectException
import com.android.server.wm.flicker.traces.RegionSubject
import com.android.server.wm.traces.common.layers.Layer
import com.android.server.wm.traces.common.layers.LayerTraceEntry
import com.google.common.truth.ExpectFailure
import com.google.common.truth.Fact
import com.google.common.truth.FailureMetadata
import com.google.common.truth.FailureStrategy
import com.google.common.truth.StandardSubjectBuilder
import com.google.common.truth.Subject

/**
 * Truth subject for [LayerTraceEntry] objects, used to make assertions over behaviors that
 * occur on a single SurfaceFlinger state.
 *
 * To make assertions over a specific state from a trace it is recommended to create a subject
 * using [LayersTraceSubject.assertThat](myTrace) and select the specific state using:
 *     [LayersTraceSubject.first]
 *     [LayersTraceSubject.last]
 *     [LayersTraceSubject.entry]
 *
 * Alternatively, it is also possible to use [LayerTraceEntrySubject.assertThat](myState) or
 * Truth.assertAbout([LayerTraceEntrySubject.getFactory]), however they will provide less debug
 * information because it uses Truth's default [FailureStrategy].
 *
 * Example:
 *    val trace = LayersTraceParser.parseFromTrace(myTraceFile)
 *    val subject = LayersTraceSubject.assertThat(trace).first()
 *        .contains("ValidLayer")
 *        .notContains("ImaginaryLayer")
 *        .coversExactly(DISPLAY_AREA)
 *        .invoke { myCustomAssertion(this) }
 */
class LayerTraceEntrySubject private constructor(
    fm: FailureMetadata,
    val entry: LayerTraceEntry,
    val trace: LayersTraceSubject?
) : FlickerSubject(fm, entry) {
    override val defaultFacts: String = "${trace?.defaultFacts ?: ""}\nEntry: $entry"

    val subjects by lazy {
        entry.flattenedLayers.map { LayerSubject.assertThat(it, this) }
    }

    /**
     * Executes a custom [assertion] on the current subject
     */
    operator fun invoke(assertion: Assertion<LayerTraceEntry>): LayerTraceEntrySubject = apply {
        assertion(this.entry)
    }

    /** {@inheritDoc} */
    override fun clone(): FlickerSubject {
        return LayerTraceEntrySubject(fm, entry, trace)
    }

    /**
     * Asserts that current entry subject has an [LayerTraceEntry.timestamp] equals to
     * [timestamp]
     */
    fun hasTimestamp(timestamp: Long): LayerTraceEntrySubject = apply {
        check("Wrong  entry timestamp").that(entry.timestamp).isEqualTo(timestamp)
    }

    /**
     * Asserts that the current SurfaceFlinger state doesn't contain layers
     */
    fun isEmpty(): LayerTraceEntrySubject = apply {
        check("Entry should not be empty")
            .that(entry.flattenedLayers)
            .isEmpty()
    }

    /**
     * Asserts that the current SurfaceFlinger state contains layers
     */
    fun isNotEmpty(): LayerTraceEntrySubject = apply {
        check("Entry should not be empty")
            .that(entry.flattenedLayers)
            .isNotEmpty()
    }

    /**
     * Asserts that the current SurfaceFlinger state has [numberLayers] layers
     */
    fun hasLayersSize(numberLayers: Int): LayerTraceEntrySubject = apply {
        check("Wrong number of layers in entry")
            .that(entry.flattenedLayers.size)
            .isEqualTo(numberLayers)
    }

    /**
     * Obtains the region occupied by all layers with name containing any of [partialLayerNames]
     *
     * @param partialLayerNames Name of the layer to search
     * @param useCompositionEngineRegionOnly If true, uses only the region calculated from the
     *   Composition Engine (CE) -- visibleRegion in the proto definition. Otherwise calculates
     *   the visible region when the information is not available from the CE
     */
    fun visibleRegion(
        vararg partialLayerNames: String,
        useCompositionEngineRegionOnly: Boolean = true
    ): RegionSubject {
        val selectedLayers = subjects
            .filter { it.name.containsAny(*partialLayerNames) }

        if (selectedLayers.isEmpty()) {
            fail("Could not find", partialLayerNames.joinToString(", "))
        }

        val visibleLayers = selectedLayers.filter { it.isVisible }
        return if (useCompositionEngineRegionOnly) {
            val visibleAreas = visibleLayers.mapNotNull { it.layer?.visibleRegion }.toTypedArray()
            RegionSubject.assertThat(visibleAreas, selectedLayers)
        } else {
            val visibleAreas = visibleLayers.mapNotNull { it.layer?.screenBounds }.toTypedArray()
            RegionSubject.assertThat(visibleAreas, selectedLayers)
        }
    }

    /**
     * Asserts that the SurfaceFlinger state contains a [Layer] with [Layer.name] containing any of
     * [partialLayerNames].
     *
     * @param partialLayerNames Name of the layers to search
     */
    fun contains(vararg partialLayerNames: String): LayerTraceEntrySubject = apply {
        val found = entry.flattenedLayers.any { it.name.containsAny(*partialLayerNames) }
        if (partialLayerNames.isNotEmpty() && !found) {
            fail("Could not find", partialLayerNames.joinToString(", "))
        }
    }

    /**
     * Asserts that the SurfaceFlinger state doesn't contain a [Layer] with [Layer.name] containing any of
     *
     * @param partialLayerNames Name of the layers to search
     */
    fun notContains(vararg partialLayerNames: String): LayerTraceEntrySubject = apply {
        val found = entry.flattenedLayers.none { it.name.containsAny(*partialLayerNames) }
        if (!found) {
            fail("Could find", partialLayerNames)
        }
    }

    /**
     * Asserts that a [Layer] with [Layer.name] containing any of [partialLayerNames] is visible.
     *
     * @param partialLayerNames Name of the layers to search
     */
    fun isVisible(vararg partialLayerNames: String): LayerTraceEntrySubject = apply {
        contains(*partialLayerNames)
        var reason: Fact? = null
        val filteredLayers = entry.flattenedLayers
            .filter { it.name.containsAny(*partialLayerNames) }
        for (layer in filteredLayers) {
            if (layer.isHiddenByParent) {
                reason = Fact.fact("Hidden by parent", layer.parent.name)
                continue
            }
            if (layer.isInvisible) {
                reason = Fact.fact("Is Invisible", layer.visibilityReason)
                continue
            }
            reason = null
            break
        }

        if (reason != null) {
            fail(reason)
        }
    }

    /**
     * Asserts that a [Layer] with [Layer.name] containing any of [partialLayerNames] doesn't exist or
     * is invisible.
     *
     * @param partialLayerNames Name of the layers to search
     */
    fun isInvisible(vararg partialLayerNames: String): LayerTraceEntrySubject = apply {
        try {
            isVisible(*partialLayerNames)
        } catch (e: FlickerSubjectException) {
            val cause = e.cause
            require(cause is AssertionError)
            ExpectFailure.assertThat(cause).factKeys().isNotEmpty()
            return@apply
        }
        fail("Layer is visible", partialLayerNames)
    }

    /**
     * Obtains a [LayerSubject] for the first occurrence of a [Layer] with [Layer.name]
     * containing [name] in [frameNumber].
     *
     * Always returns a subject, event when the layer doesn't exist. To verify if layer
     * actually exists in the hierarchy use [LayerSubject.exists] or [LayerSubject.doesNotExist]
     *
     * @return LayerSubject that can be used to make assertions on a single layer matching
     * [name] and [frameNumber].
     */
    fun layer(name: String, frameNumber: Long): LayerSubject {
        return subjects.firstOrNull {
            it.layer?.name?.contains(name) == true &&
                it.layer.currFrame == frameNumber
        } ?: LayerSubject.assertThat(name, this)
    }

    override fun toString(): String {
        return "LayerTraceEntrySubject($entry)"
    }

    companion object {
        /**
         * Boiler-plate Subject.Factory for LayersTraceSubject
         */
        private fun getFactory(
            trace: LayersTraceSubject? = null
        ): Factory<Subject, LayerTraceEntry> =
            Factory { fm, subject -> LayerTraceEntrySubject(fm, subject, trace) }

        /**
         * Creates a [LayerTraceEntrySubject] to representing a SurfaceFlinger state[entry],
         * which can be used to make assertions.
         *
         * @param entry SurfaceFlinger trace entry
         * @param trace Trace that contains this entry (optional)
         */
        @JvmStatic
        @JvmOverloads
        fun assertThat(
            entry: LayerTraceEntry,
            trace: LayersTraceSubject? = null
        ): LayerTraceEntrySubject {
            val strategy = FlickerFailureStrategy()
            val subject = StandardSubjectBuilder.forCustomFailureStrategy(strategy)
                .about(getFactory(trace))
                .that(entry) as LayerTraceEntrySubject
            strategy.init(subject)
            return subject
        }

        /**
         * Static method for getting the subject factory (for use with assertAbout())
         */
        @JvmStatic
        @JvmOverloads
        fun entries(trace: LayersTraceSubject? = null): Factory<Subject, LayerTraceEntry> {
            return getFactory(trace)
        }
    }
}