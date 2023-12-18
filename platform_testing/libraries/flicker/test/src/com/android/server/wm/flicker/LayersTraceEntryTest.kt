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

import com.android.server.wm.traces.common.layers.LayerTraceEntry
import com.android.server.wm.flicker.traces.layers.LayersTraceSubject.Companion.assertThat
import com.google.common.truth.Truth
import org.junit.FixMethodOrder
import org.junit.Test
import org.junit.runners.MethodSorters
import kotlin.AssertionError

/**
 * Contains [LayerTraceEntry] tests. To run this test: `atest
 * FlickerLibTest:LayersTraceTest`
 */
@FixMethodOrder(MethodSorters.NAME_ASCENDING)
class LayersTraceEntryTest {
    @Test
    fun exceptionContainsDebugInfo() {
        val layersTraceEntries = readLayerTraceFromFile("layers_trace_emptyregion.pb")
        val error = assertThrows(AssertionError::class.java) {
            assertThat(layersTraceEntries)
                .first()
                .contains("ImaginaryLayer")
        }
        Truth.assertThat(error).hasMessageThat().contains("Trace:")
        Truth.assertThat(error).hasMessageThat().contains("Path: ")
        Truth.assertThat(error).hasMessageThat().contains("Entry:")
    }

    @Test
    fun canParseAllLayers() {
        val trace = readLayerTraceFromFile("layers_trace_emptyregion.pb")
        Truth.assertThat(trace.entries).isNotEmpty()
        Truth.assertThat(trace.first().timestamp).isEqualTo(922839428857)
        Truth.assertThat(trace.last().timestamp).isEqualTo(941432656959)
        Truth.assertThat(trace.last().flattenedLayers).asList().hasSize(57)
    }

    @Test
    fun canParseVisibleLayersLauncher() {
        val trace = readLayerTraceFromFile("layers_trace_launch_split_screen.pb")
        val visibleLayers = trace.getEntry(90480846872160).visibleLayers
        val msg = "Visible Layers:\n" + visibleLayers.joinToString("\n") { "\t" + it.name }
        Truth.assertWithMessage(msg).that(visibleLayers).asList().hasSize(6)
        Truth.assertThat(msg).contains("ScreenDecorOverlay#0")
        Truth.assertThat(msg).contains("ScreenDecorOverlayBottom#0")
        Truth.assertThat(msg).contains("NavigationBar0#0")
        Truth.assertThat(msg).contains("ImageWallpaper#0")
        Truth.assertThat(msg).contains("StatusBar#0")
        Truth.assertThat(msg).contains("NexusLauncherActivity#0")
    }

    @Test
    fun canParseVisibleLayersSplitScreen() {
        val trace = readLayerTraceFromFile("layers_trace_launch_split_screen.pb")
        val visibleLayers = trace.getEntry(90493757372977).visibleLayers
        val msg = "Visible Layers:\n" + visibleLayers.joinToString("\n") { "\t" + it.name }
        Truth.assertWithMessage(msg).that(visibleLayers).asList().hasSize(7)
        Truth.assertThat(msg).contains("ScreenDecorOverlayBottom#0")
        Truth.assertThat(msg).contains("ScreenDecorOverlay#0")
        Truth.assertThat(msg).contains("NavigationBar0#0")
        Truth.assertThat(msg).contains("StatusBar#0")
        Truth.assertThat(msg).contains("DockedStackDivider#0")
        Truth.assertThat(msg).contains("ConversationListActivity#0")
        Truth.assertThat(msg).contains("GoogleDialtactsActivity#0")
    }

    @Test
    fun canParseVisibleLayersInTransition() {
        val trace = readLayerTraceFromFile("layers_trace_launch_split_screen.pb")
        val visibleLayers = trace.getEntry(90488463619533).visibleLayers
        val msg = "Visible Layers:\n" + visibleLayers.joinToString("\n") { "\t" + it.name }
        Truth.assertWithMessage(msg).that(visibleLayers).asList().hasSize(10)
        Truth.assertThat(msg).contains("ScreenDecorOverlayBottom#0")
        Truth.assertThat(msg).contains("ScreenDecorOverlay#0")
        Truth.assertThat(msg).contains("NavigationBar0#0")
        Truth.assertThat(msg).contains("StatusBar#0")
        Truth.assertThat(msg).contains("DockedStackDivider#0")
        Truth.assertThat(msg).contains("SnapshotStartingWindow for taskId=21 - " +
            "task-snapshot-surface#0")
        Truth.assertThat(msg).contains("SnapshotStartingWindow for taskId=21")
        Truth.assertThat(msg).contains("NexusLauncherActivity#0")
        Truth.assertThat(msg).contains("ImageWallpaper#0")
        Truth.assertThat(msg).contains("ConversationListActivity#0")
    }

    @Test
    fun canParseLayerHierarchy() {
        val trace = readLayerTraceFromFile("layers_trace_emptyregion.pb")
        Truth.assertThat(trace.entries).isNotEmpty()
        Truth.assertThat(trace.entries.first().timestamp).isEqualTo(922839428857)
        Truth.assertThat(trace.entries.last().timestamp).isEqualTo(941432656959)
        Truth.assertThat(trace.entries.first().flattenedLayers).asList().hasSize(57)
        val layers = trace.entries.first().rootLayers
        Truth.assertThat(layers[0].children).hasSize(3)
        Truth.assertThat(layers[1].children).isEmpty()
    }

    // b/76099859
    @Test
    fun canDetectOrphanLayers() {
        try {
            readLayerTraceFromFile("layers_trace_orphanlayers.pb", ignoreOrphanLayers = false)
            error("Failed to detect orphaned layers.")
        } catch (exception: RuntimeException) {
            Truth.assertThat(exception.message)
                .contains("Failed to parse layers trace. Found orphan layer with id = 49 with" +
                    " parentId = 1006")
        }
    }

    @Test
    fun testCanParseNonCroppedLayerWithHWC() {
        val layerName = "BackColorSurface#0"
        val layersTrace = readLayerTraceFromFile("layers_trace_backcolorsurface.pb")
        val entry = layersTrace.getEntry(131954021476)
        Truth.assertWithMessage("$layerName should not be visible")
            .that(entry.visibleLayers.map { it.name })
            .doesNotContain(layerName)
        val layer = entry.flattenedLayers.first { it.name == layerName }
        Truth.assertWithMessage("$layerName should be invisible because of HWC region")
            .that(layer.visibilityReason)
            .contains("Visible region calculated by Composition Engine is empty")
    }

    @Test
    fun testCanParseNonCroppedLayerWithoutHWC() {
        val layersTrace = readLayerTraceFromFile("layers_trace_no_hwc_composition.pb")
        val entry = layersTrace.getEntry(238517209878020)
        Truth.assertWithMessage("IME should be visible")
            .that(entry.visibleLayers.map { it.name })
            .contains("InputMethod#0")

        val messagesApp = "com.google.android.apps.messaging/" +
            "com.google.android.apps.messaging.ui.ConversationListActivity#0"
        Truth.assertWithMessage("Messages app should not be visible")
            .that(entry.visibleLayers.map { it.name })
            .doesNotContain(messagesApp)

        Truth.assertWithMessage("Should have visible layers in all trace entries")
            .that(entry.flattenedLayers.map { it.name })
            .doesNotContain(messagesApp)
    }
}
