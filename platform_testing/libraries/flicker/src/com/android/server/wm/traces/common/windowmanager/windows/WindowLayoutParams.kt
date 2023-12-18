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

package com.android.server.wm.traces.common.windowmanager.windows

/**
 * Represents the attributes of a WindowState in the window manager hierarchy
 *
 * This is a generic object that is reused by both Flicker and Winscope and cannot
 * access internal Java/Android functionality
 *
 */
data class WindowLayoutParams(
    val type: Int,
    val x: Int,
    val y: Int,
    val width: Int,
    val height: Int,
    val horizontalMargin: Float,
    val verticalMargin: Float,
    val gravity: Int,
    val softInputMode: Int,
    val format: Int,
    val windowAnimations: Int,
    val alpha: Float,
    val screenBrightness: Float,
    val buttonBrightness: Float,
    val rotationAnimation: Int,
    val preferredRefreshRate: Float,
    val preferredDisplayModeId: Int,
    val hasSystemUiListeners: Boolean,
    val inputFeatureFlags: Int,
    val userActivityTimeout: Long,
    val colorMode: Int,
    val flags: Int,
    val privateFlags: Int,
    val systemUiVisibilityFlags: Int,
    val subtreeSystemUiVisibilityFlags: Int,
    val appearance: Int,
    val behavior: Int,
    val fitInsetsTypes: Int,
    val fitInsetsSides: Int,
    val fitIgnoreVisibility: Boolean
) {
    val isValidNavBarType: Boolean = this.type == TYPE_NAVIGATION_BAR

    companion object {
        /**
         * @see WindowManager.LayoutParams
         */
        private const val TYPE_NAVIGATION_BAR = 2019
    }
}