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

package com.android.server.wm.traces.common

open class Bounds(val width: Int, val height: Int) {
    open val isEmpty: Boolean
        get() = height == 0 || width == 0

    val isNotEmpty: Boolean
        get() = !isEmpty

    val size: Bounds
        get() = Bounds(width, height)

    open fun prettyPrint(): String = prettyPrint(this)

    override fun toString(): String = if (isEmpty) "[empty]" else prettyPrint()

    override fun equals(other: Any?): Boolean =
        other is Bounds &&
            other.height == height &&
            other.width == width

    override fun hashCode(): Int {
        var result = width
        result = 31 * result + height
        return result
    }

    companion object {
        val EMPTY: Bounds = Bounds(0, 0)

        fun prettyPrint(bounds: Bounds): String = "${bounds.width} x ${bounds.height}"
    }
}