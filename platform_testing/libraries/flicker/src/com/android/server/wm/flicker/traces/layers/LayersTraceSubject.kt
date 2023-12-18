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

import android.graphics.Rect
import android.graphics.Region
import com.android.server.wm.flicker.assertions.Assertion
import com.android.server.wm.flicker.assertions.FlickerSubject
import com.android.server.wm.flicker.traces.FlickerFailureStrategy
import com.android.server.wm.flicker.traces.FlickerTraceSubject
import com.android.server.wm.traces.common.layers.Layer
import com.android.server.wm.traces.common.layers.LayersTrace
import com.android.server.wm.traces.parser.windowmanager.WindowManagerStateHelper
import com.google.common.truth.FailureMetadata
import com.google.common.truth.FailureStrategy
import com.google.common.truth.StandardSubjectBuilder
import com.google.common.truth.Subject
import com.google.common.truth.Subject.Factory

/**
 * Truth subject for [LayersTrace] objects, used to make assertions over behaviors that occur
 * throughout a whole trace
 *
 * To make assertions over a trace it is recommended to create a subject using
 * [LayersTraceSubject.assertThat](myTrace). Alternatively, it is also possible to use
 * Truth.assertAbout(LayersTraceSubject.FACTORY), however it will provide less debug
 * information because it uses Truth's default [FailureStrategy].
 *
 * Example:
 *    val trace = LayersTraceParser.parseFromTrace(myTraceFile)
 *    val subject = LayersTraceSubject.assertThat(trace)
 *        .contains("ValidLayer")
 *        .notContains("ImaginaryLayer")
 *        .coversExactly(DISPLAY_AREA)
 *        .forAllEntries()
 *
 * Example2:
 *    val trace = LayersTraceParser.parseFromTrace(myTraceFile)
 *    val subject = LayersTraceSubject.assertThat(trace) {
 *        check("Custom check") { myCustomAssertion(this) }
 *    }
 */
class LayersTraceSubject private constructor(
    fm: FailureMetadata,
    val trace: LayersTrace
) : FlickerTraceSubject<LayerTraceEntrySubject>(fm, trace) {
    override val defaultFacts: String by lazy {
        buildString {
            if (trace.hasSource()) {
                append("Path: ${trace.source}")
                append("\n")
            }
            append("Trace: $trace")
        }
    }

    override val subjects by lazy {
        trace.entries.map { LayerTraceEntrySubject.assertThat(it, this) }
    }

    /**
     * Executes a custom [assertion] on the current subject
     */
    operator fun invoke(assertion: Assertion<LayersTrace>): LayersTraceSubject = apply {
        assertion(this.trace)
    }

    /** {@inheritDoc} */
    override fun clone(): FlickerSubject {
        return LayersTraceSubject(fm, trace)
    }

    /**
     * Signal that the last assertion set is complete. The next assertion added will start a new
     * set of assertions.
     *
     * E.g.: checkA().then().checkB()
     *
     * Will produce two sets of assertions (checkA) and (checkB) and checkB will only be checked
     * after checkA passes.
     */
    fun then(): LayersTraceSubject = apply {
        startAssertionBlock()
    }

    fun isEmpty(): LayersTraceSubject = apply {
        check("Trace is empty").that(trace).isEmpty()
    }

    fun isNotEmpty() = apply {
        check("Trace is not empty").that(trace).isNotEmpty()
    }

    /**
     * @return LayerSubject that can be used to make assertions on a single layer matching
     * [name] and [frameNumber].
     */
    fun layer(name: String, frameNumber: Long): LayerSubject {
        return subjects
            .map { it.layer(name, frameNumber) }
            .firstOrNull { it.isNotEmpty }
            ?: LayerSubject.assertThat(null)
    }

    /**
     * Asserts that the visible area covered by any [Layer] with [Layer.name] containing any of
     * [layerName] covers at least [testRegion], that is, if its area of the layer's visible
     * region covers each point in the region.
     *
     * @param testRegion Expected covered area
     * @param layerName Name of the layer to search
     */
    fun coversAtLeast(
        testRegion: Rect,
        vararg layerName: String
    ): LayersTraceSubject = this.coversAtLeast(testRegion, *layerName)

    /**
     * Asserts that the visible area covered by any [Layer] with [Layer.name] containing any of
     * [layerName] covers at least [testRegion], that is, if its area of the layer's visible
     * region covers each point in the region.
     *
     * @param testRegion Expected covered area
     * @param layerName Name of the layer to search
     */
    fun coversAtLeast(
        testRegion: com.android.server.wm.traces.common.Rect,
        vararg layerName: String
    ): LayersTraceSubject = this.coversAtLeast(testRegion, *layerName)

    /**
     * Asserts that the visible area covered by any [Layer] with [Layer.name] containing any of
     * [layerName] covers at most [testRegion], that is, if the area of any layer doesn't
     * cover any point outside of [testRegion].
     *
     * @param testRegion Expected covered area
     * @param layerName Name of the layer to search
     */
    fun coversAtMost(
        testRegion: Rect,
        vararg layerName: String
    ): LayersTraceSubject = this.coversAtMost(testRegion, *layerName)

    /**
     * Asserts that the visible area covered by any [Layer] with [Layer.name] containing any of
     * [layerName] covers at most [testRegion], that is, if the area of any layer doesn't
     * cover any point outside of [testRegion].
     *
     * @param testRegion Expected covered area
     * @param layerName Name of the layer to search
     */
    fun coversAtMost(
        testRegion: com.android.server.wm.traces.common.Rect,
        vararg layerName: String
    ): LayersTraceSubject = this.coversAtMost(testRegion, *layerName)

    /**
     * Asserts that the visible area covered by any [Layer] with [Layer.name] containing any of
     * [layerName] covers at least [testRegion], that is, if its area of the layer's visible
     * region covers each point in the region.
     *
     * @param testRegion Expected covered area
     * @param layerName Name of the layer to search
     */
    fun coversAtLeast(
        testRegion: Region,
        vararg layerName: String
    ): LayersTraceSubject = apply {
        addAssertion("coversAtLeast($testRegion, ${layerName.joinToString(", ")})") {
            it.visibleRegion(*layerName).coversAtLeast(testRegion)
        }
    }

    /**
     * Asserts that the visible area covered by any [Layer] with [Layer.name] containing any of
     * [layerName] covers at least [testRegion], that is, if its area of the layer's visible
     * region covers each point in the region.
     *
     * @param testRegion Expected covered area
     * @param layerName Name of the layer to search
     */
    fun coversAtLeast(
        testRegion: com.android.server.wm.traces.common.Region,
        vararg layerName: String
    ): LayersTraceSubject = apply {
        addAssertion("coversAtLeast($testRegion, ${layerName.joinToString(", ")})") {
            it.visibleRegion(*layerName).coversAtLeast(testRegion)
        }
    }

    /**
     * Asserts that the visible area covered by any [Layer] with [Layer.name] containing any of
     * [layerName] covers at most [testRegion], that is, if the area of any layer doesn't
     * cover any point outside of [testRegion].
     *
     * @param testRegion Expected covered area
     * @param layerName Name of the layer to search
     */
    fun coversAtMost(
        testRegion: Region,
        vararg layerName: String
    ): LayersTraceSubject = apply {
        addAssertion("coversAtMost($testRegion, ${layerName.joinToString(", ")}") {
            it.visibleRegion(*layerName).coversAtMost(testRegion)
        }
    }

    /**
     * Asserts that the visible area covered by any [Layer] with [Layer.name] containing any of
     * [layerName] covers at most [testRegion], that is, if the area of any layer doesn't
     * cover any point outside of [testRegion].
     *
     * @param testRegion Expected covered area
     * @param layerName Name of the layer to search
     */
    fun coversAtMost(
        testRegion: com.android.server.wm.traces.common.Region,
        vararg layerName: String
    ): LayersTraceSubject = apply {
        addAssertion("coversAtMost($testRegion, ${layerName.joinToString(", ")}") {
            it.visibleRegion(*layerName).coversAtMost(testRegion)
        }
    }

    /**
     * Checks that all visible layers are shown for more than one consecutive entry
     */
    @JvmOverloads
    fun visibleLayersShownMoreThanOneConsecutiveEntry(
        ignoreLayers: List<String> = listOf(WindowManagerStateHelper.SPLASH_SCREEN_NAME,
            WindowManagerStateHelper.SNAPSHOT_WINDOW_NAME)
    ): LayersTraceSubject = apply {
        visibleEntriesShownMoreThanOneConsecutiveTime { subject ->
            subject.entry.visibleLayers
                .filter { ignoreLayers.none { layerName -> layerName in it.name } }
                .map { it.name }
                .toSet()
        }
    }

    /**
     * Asserts that a [Layer] with [Layer.name] containing any of [layerName] has a visible region
     * of exactly [expectedVisibleRegion] in trace entries.
     *
     * @param layerName Name of the layer to search
     * @param expectedVisibleRegion Expected visible region of the layer
     */
    fun coversExactly(
        expectedVisibleRegion: Region,
        vararg layerName: String
    ): LayersTraceSubject = apply {
        addAssertion("coversExactly(${layerName.joinToString(", ")}$expectedVisibleRegion)") {
            it.visibleRegion(*layerName).coversExactly(expectedVisibleRegion)
        }
    }

    /**
     * Asserts that each entry in the trace doesn't contain a [Layer] with [Layer.name]
     * containing [layerName].
     *
     * @param layerName Name of the layer to search
     */
    fun notContains(vararg layerName: String): LayersTraceSubject =
        apply {
            addAssertion("notContains(${layerName.joinToString(", ")})") {
                it.notContains(*layerName)
            }
        }

    /**
     * Asserts that each entry in the trace contains a [Layer] with [Layer.name] containing any of
     * [layerName].
     *
     * @param layerName Name of the layer to search
     */
    fun contains(vararg layerName: String): LayersTraceSubject =
        apply { addAssertion("contains(${layerName.joinToString(", ")})") {
            it.contains(*layerName) }
        }

    /**
     * Asserts that each entry in the trace contains a [Layer] with [Layer.name] containing any of
     * [layerName] that is visible.
     *
     * @param layerName Name of the layer to search
     */
    fun isVisible(vararg layerName: String): LayersTraceSubject =
        apply { addAssertion("isVisible(${layerName.joinToString(", ")})") {
            it.isVisible(*layerName) }
        }

    /**
     * Asserts that each entry in the trace doesn't contain a [Layer] with [Layer.name]
     * containing [layerName] or that the layer is not visible .
     *
     * @param layerName Name of the layer to search
     */
    fun isInvisible(vararg layerName: String): LayersTraceSubject =
        apply {
            addAssertion("hidesLayer(${layerName.joinToString(", ")})") {
                it.isInvisible(*layerName)
            }
        }

    /**
     * Executes a custom [assertion] on the current subject
     */
    operator fun invoke(
        name: String,
        assertion: Assertion<LayerTraceEntrySubject>
    ): LayersTraceSubject = apply { addAssertion(name, assertion) }

    fun hasFrameSequence(name: String, frameNumbers: Iterable<Long>): LayersTraceSubject = apply {
        val firstFrame = frameNumbers.first()
        val entries = trace.entries.asSequence()
            // map entry to buffer layers with name
            .map { it.getLayerWithBuffer(name) }
            // removing all entries without the layer
            .filterNotNull()
            // removing all entries with the same frame number
            .distinctBy { it.currFrame }
            // drop until the first frame we are interested in
            .dropWhile { layer -> layer.currFrame != firstFrame }

        var numFound = 0
        val frameNumbersMatch = entries.zip(frameNumbers.asSequence()) { layer, frameNumber ->
            numFound++
            layer.currFrame == frameNumber
        }.all { it }
        val allFramesFound = frameNumbers.count() == numFound
        if (!allFramesFound || !frameNumbersMatch) {
            val message = "Could not find Layer:" + name +
                " with frame sequence:" + frameNumbers.joinToString(",") +
                " Found:\n" + entries.joinToString("\n")
            fail(message)
        }
    }

    /**
     * Run the assertions for all trace entries within the specified time range
     */
    fun forRange(startTime: Long, endTime: Long) {
        val subjectsInRange = subjects.filter { it.entry.timestamp in startTime..endTime }
        assertionsChecker.test(subjectsInRange)
    }

    /**
     * User-defined entry point for the trace entry with [timestamp]
     *
     * @param timestamp of the entry
     */
    fun entry(timestamp: Long): LayerTraceEntrySubject =
        subjects.first { it.entry.timestamp == timestamp }

    companion object {
        /**
         * Boiler-plate Subject.Factory for LayersTraceSubject
         */
        private val FACTORY: Factory<Subject, LayersTrace> =
            Factory { fm, subject -> LayersTraceSubject(fm, subject) }

        /**
         * Creates a [LayersTraceSubject] to representing a SurfaceFlinger trace,
         * which can be used to make assertions.
         *
         * @param trace SurfaceFlinger trace
         */
        @JvmStatic
        fun assertThat(trace: LayersTrace): LayersTraceSubject {
            val strategy = FlickerFailureStrategy()
            val subject = StandardSubjectBuilder.forCustomFailureStrategy(strategy)
                .about(FACTORY)
                .that(trace) as LayersTraceSubject
            strategy.init(subject)
            return subject
        }

        /**
         * Static method for getting the subject factory (for use with assertAbout())
         */
        @JvmStatic
        fun entries(): Factory<Subject, LayersTrace> {
            return FACTORY
        }
    }
}
