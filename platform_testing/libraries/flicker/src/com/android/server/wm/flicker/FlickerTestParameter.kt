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

import android.view.Surface
import android.view.WindowManagerPolicyConstants
import com.android.server.wm.flicker.assertions.AssertionData
import com.android.server.wm.flicker.assertions.FlickerSubject
import com.android.server.wm.flicker.dsl.AssertionTag
import com.android.server.wm.flicker.traces.eventlog.EventLogSubject
import com.android.server.wm.flicker.traces.layers.LayerTraceEntrySubject
import com.android.server.wm.flicker.traces.layers.LayersTraceSubject
import com.android.server.wm.flicker.traces.windowmanager.WindowManagerStateSubject
import com.android.server.wm.flicker.traces.windowmanager.WindowManagerTraceSubject

/**
 * Specification of a flicker test for JUnit ParameterizedRunner class
 */
data class FlickerTestParameter(
    @JvmField val config: MutableMap<String, Any?>,
    @JvmField val name: String = defaultName(config)
) {
    internal var internalFlicker: Flicker? = null
    internal val flicker: Flicker get() = internalFlicker ?: error("Flicker not initialized")
    val isRotated: Boolean
        get() = config.startRotation == Surface.ROTATION_90 ||
            config.startRotation == Surface.ROTATION_270

    fun clear() {
        internalFlicker?.clear()
    }

    fun assertWmStart(assertion: WindowManagerStateSubject.() -> Unit) {
        val assertionData = buildWmStartAssertion(assertion)
        this.flicker.checkAssertion(assertionData)
    }

    fun assertWmEnd(assertion: WindowManagerStateSubject.() -> Unit) {
        val assertionData = buildWmEndAssertion(assertion)
        this.flicker.checkAssertion(assertionData)
    }

    fun assertWm(assertion: WindowManagerTraceSubject.() -> Unit) {
        val assertionData = buildWMAssertion(assertion)
        this.flicker.checkAssertion(assertionData)
    }

    fun assertWmTag(tag: String, assertion: WindowManagerStateSubject.() -> Unit) {
        val assertionData = buildWMTagAssertion(tag, assertion)
        this.flicker.checkAssertion(assertionData)
    }

    fun assertLayersStart(assertion: LayerTraceEntrySubject.() -> Unit) {
        val assertionData = buildLayersStartAssertion(assertion)
        this.flicker.checkAssertion(assertionData)
    }

    fun assertLayersEnd(assertion: LayerTraceEntrySubject.() -> Unit) {
        val assertionData = buildLayersEndAssertion(assertion)
        this.flicker.checkAssertion(assertionData)
    }

    fun assertLayers(assertion: LayersTraceSubject.() -> Unit) {
        val assertionData = buildLayersAssertion(assertion)
        this.flicker.checkAssertion(assertionData)
    }

    fun assertLayersTag(tag: String, assertion: LayerTraceEntrySubject.() -> Unit) {
        val assertionData = buildLayersTagAssertion(tag, assertion)
        this.flicker.checkAssertion(assertionData)
    }

    fun assertEventLog(assertion: EventLogSubject.() -> Unit) {
        val assertionData = buildEventLogAssertion(assertion)
        this.flicker.checkAssertion(assertionData)
    }

    override fun toString(): String = name

    companion object {
        fun defaultName(config: Map<String, Any?>) = buildString {
            append(config.startRotationName)
            if (config.endRotation != config.startRotation) {
                append("_${config.endRotationName}")
            }
            if (config.navBarMode.isNotEmpty()) {
                append("_${config.navBarModeName}")
            }
        }

        @JvmStatic
        fun buildWmStartAssertion(assertion: WindowManagerStateSubject.() -> Unit): AssertionData =
            AssertionData(tag = AssertionTag.START,
                expectedSubjectClass = WindowManagerStateSubject::class,
                assertion = assertion as FlickerSubject.() -> Unit)

        @JvmStatic
        fun buildWmEndAssertion(assertion: WindowManagerStateSubject.() -> Unit): AssertionData =
            AssertionData(tag = AssertionTag.END,
                expectedSubjectClass = WindowManagerStateSubject::class,
                assertion = assertion as FlickerSubject.() -> Unit)

        @JvmStatic
        fun buildWMAssertion(assertion: WindowManagerTraceSubject.() -> Unit): AssertionData {
            val closedAssertion: WindowManagerTraceSubject.() -> Unit = {
                assertion()
                this.forAllEntries()
            }
            return AssertionData(tag = AssertionTag.ALL,
                expectedSubjectClass = WindowManagerTraceSubject::class,
                assertion = closedAssertion as FlickerSubject.() -> Unit)
        }

        @JvmStatic
        fun buildWMTagAssertion(
            tag: String,
            assertion: WindowManagerStateSubject.() -> Unit
        ): AssertionData = AssertionData(tag = tag,
            expectedSubjectClass = WindowManagerStateSubject::class,
            assertion = assertion as FlickerSubject.() -> Unit)

        @JvmStatic
        fun buildLayersStartAssertion(assertion: LayerTraceEntrySubject.() -> Unit): AssertionData =
            AssertionData(tag = AssertionTag.START,
                expectedSubjectClass = LayerTraceEntrySubject::class,
                assertion = assertion as FlickerSubject.() -> Unit)

        @JvmStatic
        fun buildLayersEndAssertion(assertion: LayerTraceEntrySubject.() -> Unit): AssertionData =
            AssertionData(tag = AssertionTag.END,
                expectedSubjectClass = LayerTraceEntrySubject::class,
                assertion = assertion as FlickerSubject.() -> Unit)

        @JvmStatic
        fun buildLayersAssertion(assertion: LayersTraceSubject.() -> Unit): AssertionData {
            val closedAssertion: LayersTraceSubject.() -> Unit = {
                assertion()
                this.forAllEntries()
            }

            return AssertionData(tag = AssertionTag.ALL,
                expectedSubjectClass = LayersTraceSubject::class,
                assertion = closedAssertion as FlickerSubject.() -> Unit)
        }

        @JvmStatic
        fun buildLayersTagAssertion(
            tag: String,
            assertion: LayerTraceEntrySubject.() -> Unit
        ): AssertionData = AssertionData(tag = tag,
            expectedSubjectClass = LayerTraceEntrySubject::class,
            assertion = assertion as FlickerSubject.() -> Unit)

        @JvmStatic
        fun buildEventLogAssertion(assertion: EventLogSubject.() -> Unit): AssertionData =
            AssertionData(tag = AssertionTag.ALL,
                expectedSubjectClass = EventLogSubject::class,
                assertion = assertion as FlickerSubject.() -> Unit)
    }
}

internal const val REPETITIONS = "repetitions"
internal const val START_ROTATION = "startRotation"
internal const val END_ROTATION = "endRotation"
internal const val NAV_BAR_MODE = "navBarMode"

val Map<String, Any?>.repetitions: Int
    get() = this.getOrDefault(REPETITIONS, 1) as Int

val Map<String, Any?>.startRotation: Int
    get() = this.getOrDefault(START_ROTATION, Surface.ROTATION_0) as Int

val Map<String, Any?>.endRotation: Int
    get() = this.getOrDefault(END_ROTATION, startRotation) as Int

val Map<String, Any?>.startRotationName: String
    get() = Surface.rotationToString(startRotation)

val Map<String, Any?>.endRotationName: String
    get() = Surface.rotationToString(endRotation)

val Map<String, Any?>.navBarMode: String
    get() = this.getOrDefault(NAV_BAR_MODE,
        WindowManagerPolicyConstants.NAV_BAR_MODE_GESTURAL_OVERLAY) as String

val Map<String, Any?>.navBarModeName
    get() = when (this.navBarMode) {
        WindowManagerPolicyConstants.NAV_BAR_MODE_3BUTTON_OVERLAY -> "3_BUTTON_NAV"
        WindowManagerPolicyConstants.NAV_BAR_MODE_2BUTTON_OVERLAY -> "2_BUTTON_NAV"
        WindowManagerPolicyConstants.NAV_BAR_MODE_GESTURAL_OVERLAY -> "GESTURAL_NAV"
        else -> "UNKNOWN_NAV_BAR_MODE(${this.navBarMode}"
    }