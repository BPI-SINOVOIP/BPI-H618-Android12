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
import com.android.server.wm.flicker.traces.layers.LayerTraceEntrySubject
import com.android.server.wm.flicker.traces.layers.LayersTraceSubject
import com.google.common.truth.Truth
import org.junit.FixMethodOrder
import org.junit.Test
import org.junit.runners.MethodSorters

/**
 * Contains [LayerTraceEntrySubject] tests. To run this test: `atest
 * FlickerLibTest:LayersTraceTest`
 */
@FixMethodOrder(MethodSorters.NAME_ASCENDING)
class LayerTraceEntrySubjectTest {
    @Test
    fun exceptionContainsDebugInfo() {
        val layersTraceEntries = readLayerTraceFromFile("layers_trace_emptyregion.pb")
        val error = assertThrows(AssertionError::class.java) {
            LayersTraceSubject.assertThat(layersTraceEntries)
                .first()
                .contains("ImaginaryLayer")
        }
        Truth.assertThat(error).hasMessageThat().contains("Trace:")
        Truth.assertThat(error).hasMessageThat().contains("Path: ")
        Truth.assertThat(error).hasMessageThat().contains("Entry:")
    }

    @Test
    fun testCanInspectBeginning() {
        val layersTraceEntries = readLayerTraceFromFile("layers_trace_launch_split_screen.pb")
        LayerTraceEntrySubject.assertThat(layersTraceEntries.entries.first())
            .isVisible("NavigationBar0#0")
            .notContains("DockedStackDivider#0")
            .isVisible("NexusLauncherActivity#0")
    }

    @Test
    fun testCanInspectEnd() {
        val layersTraceEntries = readLayerTraceFromFile("layers_trace_launch_split_screen.pb")
        LayerTraceEntrySubject.assertThat(layersTraceEntries.entries.last())
            .isVisible("NavigationBar0#0")
            .isVisible("DockedStackDivider#0")
    }

    // b/75276931
    @Test
    fun canDetectUncoveredRegion() {
        val trace = readLayerTraceFromFile("layers_trace_emptyregion.pb")
        val expectedRegion = Region(0, 0, 1440, 2960)
        val error = assertThrows(AssertionError::class.java) {
            LayersTraceSubject.assertThat(trace).entry(935346112030)
                .visibleRegion()
                .coversAtLeast(expectedRegion)
        }
        assertFailure(error)
            .factValue("Region to test")
            .contains("SkRegion((0,0,1440,2960))")

        assertFailure(error)
            .factValue("Uncovered region")
            .contains("SkRegion((0,1440,1440,2960))")
    }

    // Visible region tests
    @Test
    fun canTestLayerVisibleRegion_layerDoesNotExist() {
        val imaginaryLayer = "ImaginaryLayer"
        val trace = readLayerTraceFromFile("layers_trace_emptyregion.pb")
        val expectedVisibleRegion = Region(0, 0, 1, 1)
        val error = assertThrows(AssertionError::class.java) {
            LayersTraceSubject.assertThat(trace).entry(937229257165)
                .visibleRegion(imaginaryLayer)
                .coversExactly(expectedVisibleRegion)
        }
        assertFailure(error)
            .factValue("Could not find")
            .isEqualTo(imaginaryLayer)
    }

    @Test
    fun canTestLayerVisibleRegion_layerDoesNotHaveExpectedVisibleRegion() {
        val trace = readLayerTraceFromFile("layers_trace_emptyregion.pb")
        val expectedVisibleRegion = Region(0, 0, 1, 1)
        val error = assertThrows(AssertionError::class.java) {
            LayersTraceSubject.assertThat(trace).entry(937126074082)
                .visibleRegion("DockedStackDivider#0")
                .coversExactly(expectedVisibleRegion)
        }
        assertFailure(error)
            .factValue("Covered region")
            .contains("SkRegion()")
    }

    @Test
    fun canTestLayerVisibleRegion_layerIsHiddenByParent() {
        val trace = readLayerTraceFromFile("layers_trace_emptyregion.pb")
        val expectedVisibleRegion = Region(0, 0, 1, 1)
        val error = assertThrows(AssertionError::class.java) {
            LayersTraceSubject.assertThat(trace).entry(935346112030)
                .visibleRegion("SimpleActivity#0")
                .coversExactly(expectedVisibleRegion)
        }
        assertFailure(error)
            .factValue("Covered region")
            .contains("SkRegion()")
    }

    @Test
    fun canTestLayerVisibleRegion_incorrectRegionSize() {
        val trace = readLayerTraceFromFile("layers_trace_emptyregion.pb")
        val expectedVisibleRegion = Region(0, 0, 1440, 99)
        val error = assertThrows(AssertionError::class.java) {
            LayersTraceSubject.assertThat(trace).entry(937126074082)
                .visibleRegion("StatusBar")
                .coversExactly(expectedVisibleRegion)
        }
        assertFailure(error)
            .factValue("Region to test")
            .contains("SkRegion((0,0,1440,99))")
    }

    @Test
    fun canTestLayerVisibleRegion() {
        val trace = readLayerTraceFromFile("layers_trace_launch_split_screen.pb")
        val expectedVisibleRegion = Region(0, 0, 1080, 145)
        LayersTraceSubject.assertThat(trace).entry(90480846872160)
            .visibleRegion("StatusBar")
            .coversExactly(expectedVisibleRegion)
    }

    @Test
    fun canTestLayerVisibleRegion_layerIsNotVisible() {
        val trace = readLayerTraceFromFile("layers_trace_invalid_layer_visibility.pb")
        val error = assertThrows(AssertionError::class.java) {
            LayersTraceSubject.assertThat(trace).entry(252794268378458)
                .isVisible("com.android.server.wm.flicker.testapp")
        }
        assertFailure(error)
            .factValue("Is Invisible")
            .contains("Bounds is 0x0")
    }

    @Test
    fun testCanParseWithoutHWC_visibleRegion() {
        val layersTrace = readLayerTraceFromFile("layers_trace_no_hwc_composition.pb")
        val entry = LayersTraceSubject.assertThat(layersTrace)
            .entry(238517209878020)

        entry.visibleRegion(useCompositionEngineRegionOnly = false)
            .coversExactly(Region(0, 0, 1440, 2960))

        entry.visibleRegion("InputMethod#0", useCompositionEngineRegionOnly = false)
            .coversExactly(Region(0, 171, 1440, 2960))
    }
}