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

import android.graphics.Region
import androidx.test.filters.FlakyTest
import com.android.server.wm.flicker.traces.layers.LayersTraceSubject
import com.android.server.wm.flicker.traces.layers.LayersTraceSubject.Companion.assertThat
import com.android.server.wm.traces.common.layers.LayersTrace
import com.google.common.truth.Truth
import org.junit.FixMethodOrder
import org.junit.Test
import org.junit.runners.MethodSorters

/**
 * Contains [LayersTraceSubject] tests. To run this test: `atest
 * FlickerLibTest:LayersTraceSubjectTest`
 */
@FixMethodOrder(MethodSorters.NAME_ASCENDING)
class LayersTraceSubjectTest {
    @Test
    fun exceptionContainsDebugInfo() {
        val layersTraceEntries = readLayerTraceFromFile("layers_trace_launch_split_screen.pb")
        val error = assertThrows(AssertionError::class.java) {
            assertThat(layersTraceEntries)
                .isEmpty()
        }
        Truth.assertThat(error).hasMessageThat().contains("Trace:")
        Truth.assertThat(error).hasMessageThat().contains("Path: ")
        Truth.assertThat(error).hasMessageThat().contains("Start:")
        Truth.assertThat(error).hasMessageThat().contains("End:")
    }

    @Test
    fun testCanDetectEmptyRegionFromLayerTrace() {
        val layersTraceEntries = readLayerTraceFromFile("layers_trace_emptyregion.pb")
        try {
            assertThat(layersTraceEntries)
                .coversAtLeast(DISPLAY_REGION)
                .forAllEntries()
            error("Assertion should not have passed")
        } catch (e: Throwable) {
            assertFailure(e).factValue("Region to test").contains(DISPLAY_REGION.toString())
            assertFailure(e).factValue("Uncovered region").contains("SkRegion((0,1440,1440,2880))")
        }
    }

    @Test
    fun testCanInspectBeginning() {
        val layersTraceEntries = readLayerTraceFromFile("layers_trace_launch_split_screen.pb")
        assertThat(layersTraceEntries)
            .first()
            .isVisible("NavigationBar0#0")
            .notContains("DockedStackDivider#0")
            .isVisible("NexusLauncherActivity#0")
    }

    @Test
    fun testCanInspectEnd() {
        val layersTraceEntries = readLayerTraceFromFile("layers_trace_launch_split_screen.pb")
        assertThat(layersTraceEntries)
            .last()
            .isVisible("NavigationBar0#0")
            .isVisible("DockedStackDivider#0")
    }

    @Test
    fun testCanDetectChangingAssertions() {
        val layersTraceEntries = readLayerTraceFromFile("layers_trace_launch_split_screen.pb")
        assertThat(layersTraceEntries)
            .isVisible("NavigationBar0#0")
            .notContains("DockedStackDivider#0")
            .then()
            .isVisible("NavigationBar0#0")
            .isInvisible("DockedStackDivider#0")
            .then()
            .isVisible("NavigationBar0#0")
            .isVisible("DockedStackDivider#0")
            .forAllEntries()
    }

    @FlakyTest
    @Test
    fun testCanDetectIncorrectVisibilityFromLayerTrace() {
        val layersTraceEntries = readLayerTraceFromFile("layers_trace_invalid_layer_visibility.pb")
        val error = assertThrows(AssertionError::class.java) {
            assertThat(layersTraceEntries)
                .isVisible("com.android.server.wm.flicker.testapp")
                .then()
                .isInvisible("com.android.server.wm.flicker.testapp")
                .forAllEntries()
        }

        assertFailure(error)
            .hasMessageThat()
            .contains("layers_trace_invalid_layer_visibility.pb")
        assertFailure(error)
            .hasMessageThat()
            .contains("2d22h13m14s303ms")
        assertFailure(error)
            .hasMessageThat()
            .contains("!isVisible")
        assertFailure(error)
            .hasMessageThat()
            .contains("com.android.server.wm.flicker.testapp/" +
                "com.android.server.wm.flicker.testapp.SimpleActivity#0 is visible")
    }

    @Test
    fun testCanDetectInvalidVisibleLayerForMoreThanOneConsecutiveEntry() {
        val layersTraceEntries = readLayerTraceFromFile("layers_trace_invalid_visible_layers.pb")
        val error = assertThrows(AssertionError::class.java) {
            assertThat(layersTraceEntries)
                .visibleLayersShownMoreThanOneConsecutiveEntry()
                .forAllEntries()
            error("Assertion should not have passed")
        }

        Truth.assertThat(error).hasMessageThat().contains("2d18h35m56s397ms")
        assertFailure(error)
            .hasMessageThat()
            .contains("StatusBar#0")
        assertFailure(error)
            .hasMessageThat()
            .contains("is not visible for 2 entries")
    }

    private fun testCanDetectVisibleLayersMoreThanOneConsecutiveEntry(trace: LayersTrace) {
        assertThat(trace)
            .visibleLayersShownMoreThanOneConsecutiveEntry()
            .forAllEntries()
    }

    @Test
    fun testCanDetectVisibleLayersMoreThanOneConsecutiveEntry() {
        testCanDetectVisibleLayersMoreThanOneConsecutiveEntry(
            readLayerTraceFromFile("layers_trace_snapshot_visible.pb"))
    }

    @Test
    fun testCanIgnoreLayerEqualNameInVisibleLayersMoreThanOneConsecutiveEntry() {
        val layersTraceEntries = readLayerTraceFromFile(
                "layers_trace_invalid_visible_layers.pb")
        assertThat(layersTraceEntries)
                .visibleLayersShownMoreThanOneConsecutiveEntry(listOf("StatusBar#0"))
                .forAllEntries()
    }

    @Test
    fun testCanIgnoreLayerShorterNameInVisibleLayersMoreThanOneConsecutiveEntry() {
        val layersTraceEntries = readLayerTraceFromFile(
                "one_visible_layer_launcher_trace.pb")
        assertThat(layersTraceEntries)
                .visibleLayersShownMoreThanOneConsecutiveEntry(listOf("Launcher"))
                .forAllEntries()
    }

    private fun detectRootLayer(fileName: String) {
        val layersTrace = readLayerTraceFromFile(fileName)
        for (entry in layersTrace.entries) {
            val rootLayers = entry.rootLayers
            Truth.assertWithMessage("Does not have any root layer")
                    .that(rootLayers.size)
                    .isGreaterThan(0)
            val firstParentId = rootLayers.first().parentId
            Truth.assertWithMessage("Has multiple root layers")
                    .that(rootLayers.all { it.parentId == firstParentId })
                    .isTrue()
        }
    }

    @Test
    fun testCanDetectRootLayer() {
        detectRootLayer("layers_trace_root.pb")
    }

    @Test
    fun testCanDetectRootLayerAOSP() {
        detectRootLayer("layers_trace_root_aosp.pb")
    }

    companion object {
        private val DISPLAY_REGION = Region(0, 0, 1440, 2880)
    }
}
