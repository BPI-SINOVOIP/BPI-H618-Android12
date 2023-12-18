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

package com.android.server.wm.traces.common.windowmanager.windows

import com.android.server.wm.traces.common.Bounds
import com.android.server.wm.traces.common.Rect
import com.android.server.wm.traces.common.Region

/**
 * Represents a window in the window manager hierarchy
 *
 * This is a generic object that is reused by both Flicker and Winscope and cannot
 * access internal Java/Android functionality
 *
 */
open class WindowState(
    val attributes: WindowLayoutParams,
    val displayId: Int,
    val stackId: Int,
    val layer: Int,
    val isSurfaceShown: Boolean,
    val windowType: Int,
    val requestedSize: Bounds,
    val surfacePosition: Rect?,
    _frame: Rect?,
    _containingFrame: Rect?,
    _parentFrame: Rect?,
    _contentFrame: Rect?,
    _contentInsets: Rect?,
    _surfaceInsets: Rect?,
    _givenContentInsets: Rect?,
    _crop: Rect?,
    windowContainer: WindowContainer,
    val isAppWindow: Boolean
) : WindowContainer(windowContainer, getWindowTitle(windowContainer.title)) {
    override val isVisible: Boolean get() = super.isVisible && attributes.alpha > 0

    val frame: Rect = _frame ?: Rect.EMPTY
    val containingFrame: Rect = _containingFrame ?: Rect.EMPTY
    val parentFrame: Rect = _parentFrame ?: Rect.EMPTY
    val contentFrame: Rect = _contentFrame ?: Rect.EMPTY
    val contentInsets: Rect = _contentInsets ?: Rect.EMPTY
    val surfaceInsets: Rect = _surfaceInsets ?: Rect.EMPTY
    val givenContentInsets: Rect = _givenContentInsets ?: Rect.EMPTY
    val crop: Rect = _crop ?: Rect.EMPTY
    override val isFullscreen: Boolean get() = this.attributes.flags.and(FLAG_FULLSCREEN) > 0

    val isStartingWindow: Boolean = windowType == WINDOW_TYPE_STARTING
    val isExitingWindow: Boolean = windowType == WINDOW_TYPE_EXITING
    val isDebuggerWindow: Boolean = windowType == WINDOW_TYPE_DEBUGGER
    val isValidNavBarType: Boolean = attributes.isValidNavBarType

    val frameRegion: Region = Region(frame)

    private fun getWindowTypeSuffix(windowType: Int): String =
        when (windowType) {
            WINDOW_TYPE_STARTING -> " STARTING"
            WINDOW_TYPE_EXITING -> " EXITING"
            WINDOW_TYPE_DEBUGGER -> " DEBUGGER"
            else -> ""
        }

    override fun toString(): String = "${this::class.simpleName}: " +
        "{$token $title${getWindowTypeSuffix(windowType)}} " +
        "type=${attributes.type} cf=$containingFrame pf=$parentFrame"

    override fun equals(other: Any?): Boolean {
        return other is WindowState &&
            other.stableId == stableId &&
            other.attributes == attributes &&
            other.token == token &&
            other.title == title &&
            other.containingFrame == containingFrame &&
            other.parentFrame == parentFrame
    }

    override fun hashCode(): Int {
        var result = attributes.hashCode()
        result = 31 * result + displayId
        result = 31 * result + stackId
        result = 31 * result + layer
        result = 31 * result + isSurfaceShown.hashCode()
        result = 31 * result + windowType
        result = 31 * result + frame.hashCode()
        result = 31 * result + containingFrame.hashCode()
        result = 31 * result + parentFrame.hashCode()
        result = 31 * result + contentFrame.hashCode()
        result = 31 * result + contentInsets.hashCode()
        result = 31 * result + surfaceInsets.hashCode()
        result = 31 * result + givenContentInsets.hashCode()
        result = 31 * result + crop.hashCode()
        result = 31 * result + isAppWindow.hashCode()
        result = 31 * result + isStartingWindow.hashCode()
        result = 31 * result + isExitingWindow.hashCode()
        result = 31 * result + isDebuggerWindow.hashCode()
        result = 31 * result + isValidNavBarType.hashCode()
        result = 31 * result + frameRegion.hashCode()
        return result
    }

    companion object {
        /**
         * From {@see android.view.WindowManager.FLAG_FULLSCREEN}.
         *
         * This class is shared between JVM and JS (Winscope) and cannot access
         * Android internals
         */
        private const val FLAG_FULLSCREEN = 0x00000400
        internal const val WINDOW_TYPE_STARTING = 1
        internal const val WINDOW_TYPE_EXITING = 2
        private const val WINDOW_TYPE_DEBUGGER = 3

        internal const val STARTING_WINDOW_PREFIX = "Starting "
        internal const val DEBUGGER_WINDOW_PREFIX = "Waiting For Debugger: "

        private fun getWindowTitle(title: String): String {
            return when {
                // Existing code depends on the prefix being removed
                title.startsWith(STARTING_WINDOW_PREFIX) ->
                    title.substring(STARTING_WINDOW_PREFIX.length)
                title.startsWith(DEBUGGER_WINDOW_PREFIX) ->
                    title.substring(DEBUGGER_WINDOW_PREFIX.length)
                else -> title
            }
        }
    }
}
