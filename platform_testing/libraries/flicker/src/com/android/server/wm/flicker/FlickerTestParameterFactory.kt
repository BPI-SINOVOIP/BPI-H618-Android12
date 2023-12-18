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

/**
 * Factory for creating JUnit4 compatible tests based on the flicker DSL
 *
 * This class recreates behavior from JUnit5 TestFactory that is not available on JUnit4
 */
open class FlickerTestParameterFactory {
    /**
     * Gets a list of test configurations.
     *
     * Each configurations has a start orientation.
     */
    @JvmOverloads
    open fun getConfigNonRotationTests(
        supportedRotations: List<Int> = listOf(Surface.ROTATION_0, Surface.ROTATION_90),
        supportedNavigationModes: List<String> = listOf(
            WindowManagerPolicyConstants.NAV_BAR_MODE_3BUTTON_OVERLAY,
            WindowManagerPolicyConstants.NAV_BAR_MODE_GESTURAL_OVERLAY),
        repetitions: Int = 1
    ): List<FlickerTestParameter> {
        return supportedNavigationModes.flatMap { navBarMode ->
            supportedRotations.map { rotation ->
                createParam(repetitions, navBarMode, rotation)
            }
        }
    }

    /**
     * Gets a list of test configurations.
     *
     * Each configurations has a start and end orientation.
     */
    @JvmOverloads
    open fun getConfigRotationTests(
        supportedRotations: List<Int> = listOf(Surface.ROTATION_0, Surface.ROTATION_90),
        supportedNavigationModes: List<String> = listOf(
            WindowManagerPolicyConstants.NAV_BAR_MODE_3BUTTON_OVERLAY,
            WindowManagerPolicyConstants.NAV_BAR_MODE_GESTURAL_OVERLAY),
        repetitions: Int = 1
    ): List<FlickerTestParameter> {
        return supportedNavigationModes.flatMap { navBarMode ->
            supportedRotations
            .flatMap { start -> supportedRotations.map { end -> start to end } }
            .filter { (start, end) -> start != end }
            .map { (start, end) ->
                createParam(repetitions, navBarMode, start, end)
            }
        }
    }

    private fun createParam(
        repetitions: Int,
        navBarMode: String,
        startRotation: Int,
        endRotation: Int = startRotation
    ) = FlickerTestParameter(
        mutableMapOf(
            NAV_BAR_MODE to navBarMode,
            START_ROTATION to startRotation,
            END_ROTATION to endRotation,
            REPETITIONS to repetitions
        )
    )

    companion object {
        private lateinit var instance: FlickerTestParameterFactory

        @JvmStatic
        fun getInstance(): FlickerTestParameterFactory {
            if (!::instance.isInitialized) {
                instance = FlickerTestParameterFactory()
            }

            return instance
        }
    }
}