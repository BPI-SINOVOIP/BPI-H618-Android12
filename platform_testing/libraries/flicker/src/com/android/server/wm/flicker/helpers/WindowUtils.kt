/*
 * Copyright (C) 2020 The Android Open Source Project
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

package com.android.server.wm.flicker.helpers

import android.content.Context
import android.graphics.Point
import android.graphics.Rect
import android.graphics.Region
import android.view.Surface
import android.view.WindowManager
import androidx.test.platform.app.InstrumentationRegistry
import kotlin.math.max
import kotlin.math.min

fun Int.isRotated() = this == Surface.ROTATION_90 || this == Surface.ROTATION_270

object WindowUtils {
    /**
     * Helper functions to retrieve system window sizes and positions.
     */
    private val context
        get() = InstrumentationRegistry.getInstrumentation().context

    private val resources
        get() = context.getResources()

    /**
     * Get the display bounds
     */
    val displayBounds: Rect
        get() {
            val display = Point()
            val wm = context.getSystemService(Context.WINDOW_SERVICE) as WindowManager
            wm.defaultDisplay.getRealSize(display)
            return Rect(0, 0, display.x, display.y)
        }

    /**
     * Gets the current display rotation
     */
    val displayRotation: Int
        get() {
            val wm = context.getSystemService(Context.WINDOW_SERVICE) as WindowManager
            return wm.defaultDisplay.rotation
        }

    /**
     * Get the display bounds when the device is at a specific rotation
     *
     * @param requestedRotation Device rotation
     */
    fun getDisplayBounds(requestedRotation: Int): Region {
        val displayIsRotated = displayRotation.isRotated()
        val requestedDisplayIsRotated = requestedRotation.isRotated()

        // if the current orientation changes with the requested rotation,
        // flip height and width of display bounds.
        return if (displayIsRotated != requestedDisplayIsRotated) {
            Region(0, 0, displayBounds.height(), displayBounds.width())
        } else {
            Region(0, 0, displayBounds.width(), displayBounds.height())
        }
    }

    /**
     * Gets the expected status bar position at a specific rotation
     *
     * @param requestedRotation Device rotation
     */
    fun getStatusBarPosition(requestedRotation: Int): Region {
        val displayBounds = displayBounds
        val resourceName: String
        val width: Int
        if (!requestedRotation.isRotated()) {
            resourceName = "status_bar_height_portrait"
            width = min(displayBounds.width(), displayBounds.height())
        } else {
            resourceName = "status_bar_height_landscape"
            width = max(displayBounds.width(), displayBounds.height())
        }
        val resourceId = resources.getIdentifier(resourceName, "dimen", "android")
        val height = resources.getDimensionPixelSize(resourceId)
        return Region(0, 0, width, height)
    }

    /**
     * Gets the expected navigation bar position at a specific rotation
     *
     * @param requestedRotation Device rotation
     */
    fun getNavigationBarPosition(requestedRotation: Int): Region {
        val displayBounds = displayBounds
        val displayWidth: Int
        val displayHeight: Int
        if (!requestedRotation.isRotated()) {
            displayWidth = displayBounds.width()
            displayHeight = displayBounds.height()
        } else {
            // swap display dimensions in landscape or seascape mode
            displayWidth = displayBounds.height()
            displayHeight = displayBounds.width()
        }
        val navBarWidth = getDimensionPixelSize("navigation_bar_width")
        val navBarHeight = navigationBarHeight

        return when {
            // nav bar is at the bottom of the screen
            requestedRotation in listOf(Surface.ROTATION_0, Surface.ROTATION_180) ||
                isGesturalNavigationEnabled ->
                Region(0, displayHeight - navBarHeight, displayWidth, displayHeight)
            // nav bar is at the right side
            requestedRotation == Surface.ROTATION_90 ->
                Region(displayWidth - navBarWidth, 0, displayWidth, displayHeight)
            // nav bar is at the left side
            requestedRotation == Surface.ROTATION_270 ->
                Region(0, 0, navBarWidth, displayHeight)
            else -> error("Unknown rotation $requestedRotation")
        }
    }

    /**
     * Checks if the device uses gestural navigation
     */
    val isGesturalNavigationEnabled: Boolean
        get() {
            val resourceId = resources
                    .getIdentifier("config_navBarInteractionMode", "integer", "android")
            return resources.getInteger(resourceId) == 2 /* NAV_BAR_MODE_GESTURAL */
        }

    fun getDimensionPixelSize(resourceName: String): Int {
        val resourceId = resources.getIdentifier(resourceName, "dimen", "android")
        return resources.getDimensionPixelSize(resourceId)
    }

    /**
     * Gets the navigation bar height
     */
    val navigationBarHeight: Int
        get() {
            var navBarHeight = getDimensionPixelSize("navigation_bar_height")
            if (isGesturalNavigationEnabled) {
                navBarHeight += getDimensionPixelSize("navigation_bar_gesture_height")
            }
            return navBarHeight
        }

    /**
     * Split screen divider inset height
     */
    val dockedStackDividerInset: Int
        get() {
            val resourceId = resources
                    .getIdentifier("docked_stack_divider_insets", "dimen", "android")
            return resources.getDimensionPixelSize(resourceId)
        }
}