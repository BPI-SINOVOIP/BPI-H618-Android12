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

package com.android.server.wm.flicker.traces.windowmanager

import com.android.server.wm.flicker.assertions.Assertion
import com.android.server.wm.flicker.assertions.FlickerSubject
import com.android.server.wm.flicker.traces.FlickerFailureStrategy
import com.android.server.wm.flicker.traces.RegionSubject
import com.android.server.wm.traces.common.windowmanager.windows.WindowState
import com.google.common.truth.FailureMetadata
import com.google.common.truth.FailureStrategy
import com.google.common.truth.StandardSubjectBuilder

/**
 * Truth subject for [WindowState] objects, used to make assertions over behaviors that occur on a
 * single [WindowState] of a WM state.
 *
 * To make assertions over a layer from a state it is recommended to create a subject
 * using [WindowManagerStateSubject.windowState](windowStateName)
 *
 * Alternatively, it is also possible to use [WindowStateSubject.assertThat](myWindow) or
 * Truth.assertAbout([WindowStateSubject.getFactory]), however they will provide less debug
 * information because it uses Truth's default [FailureStrategy].
 *
 * Example:
 *    val trace = WindowManagerTraceParser.parseFromTrace(myTraceFile)
 *    val subject = WindowManagerTraceSubject.assertThat(trace).first()
 *        .windowState("ValidWindow")
 *        .exists()
 *        { myCustomAssertion(this) }
 */
class WindowStateSubject private constructor(
    fm: FailureMetadata,
    val windowState: WindowState?,
    private val entry: WindowManagerStateSubject?,
    private val windowTitle: String? = null
) : FlickerSubject(fm, windowState) {
    val isEmpty: Boolean get() = windowState == null
    val isNotEmpty: Boolean get() = !isEmpty
    val isVisible: Boolean get() = windowState?.isVisible == true
    val isInvisible: Boolean get() = windowState?.isVisible == false
    val name: String get() = windowState?.name ?: windowTitle ?: ""
    val frame: RegionSubject get() = RegionSubject.assertThat(windowState?.frame, listOf(this))

    override val defaultFacts: String =
        "${entry?.defaultFacts ?: ""}\nWindowTitle: ${windowState?.title}"

    /**
     * If the [windowState] exists, executes a custom [assertion] on the current subject
     */
    operator fun invoke(assertion: Assertion<WindowState>): WindowStateSubject = apply {
        windowState ?: return exists()
        assertion(this.windowState)
    }

    /** {@inheritDoc} */
    override fun clone(): FlickerSubject {
        return WindowStateSubject(fm, windowState, entry, windowTitle)
    }

    /**
     * Asserts that current subject doesn't exist in the window hierarchy
     */
    fun doesNotExist(): WindowStateSubject = apply {
        check("doesNotExist").that(windowState).isNull()
    }

    /**
     * Asserts that current subject exists in the window hierarchy
     */
    fun exists(): WindowStateSubject = apply {
        check("$windowTitle does not exists").that(windowState).isNotNull()
    }

    override fun toString(): String {
        return "WindowState:${windowState?.name}"
    }

    companion object {
        /**
         * Boiler-plate Subject.Factory for LayerSubject
         */
        @JvmStatic
        @JvmOverloads
        fun getFactory(entry: WindowManagerStateSubject? = null) =
            Factory { fm: FailureMetadata, subject: WindowState? ->
                WindowStateSubject(fm, subject, entry)
            }

        /**
         * User-defined entry point for existing layers
         */
        @JvmStatic
        @JvmOverloads
        fun assertThat(
            layer: WindowState?,
            entry: WindowManagerStateSubject? = null
        ): WindowStateSubject {
            val strategy = FlickerFailureStrategy()
            val subject = StandardSubjectBuilder.forCustomFailureStrategy(strategy)
                .about(getFactory(entry))
                .that(layer) as WindowStateSubject
            strategy.init(subject)
            return subject
        }

        /**
         * User-defined entry point for non existing layers
         */
        @JvmStatic
        internal fun assertThat(
            name: String,
            entry: WindowManagerStateSubject?
        ): WindowStateSubject {
            val strategy = FlickerFailureStrategy()
            val subject = StandardSubjectBuilder.forCustomFailureStrategy(strategy)
                .about(getFactory(entry, name))
                .that(null) as WindowStateSubject
            strategy.init(subject)
            return subject
        }

        /**
         * Boiler-plate Subject.Factory for LayerSubject
         */
        @JvmStatic
        internal fun getFactory(entry: WindowManagerStateSubject?, name: String) =
            Factory { fm: FailureMetadata, subject: WindowState? ->
                WindowStateSubject(fm, subject, entry, name)
            }
    }
}