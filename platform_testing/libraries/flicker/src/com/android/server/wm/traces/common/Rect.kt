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

open class Rect(
    val left: Int = 0,
    val top: Int = 0,
    val right: Int = 0,
    val bottom: Int = 0
) {
    val height: Int get() = bottom - top
    val width: Int get() = right - left
    fun centerX(): Int = left + right / 2
    fun centerY(): Int = top + bottom / 2
    /**
     * Returns true if the rectangle is empty (left >= right or top >= bottom)
     */
    val isEmpty: Boolean = width == 0 || height == 0

    val isNotEmpty: Boolean = !isEmpty

    /**
     * Returns a [RectF] version fo this rectangle.
     */
    fun toRectF(): RectF {
        return RectF(left.toFloat(), top.toFloat(), right.toFloat(), bottom.toFloat())
    }

    open fun prettyPrint(): String = prettyPrint(this)

    override fun equals(other: Any?): Boolean = other?.toString() == this.toString()

    /**
     * Returns true iff the specified rectangle r is inside or equal to this
     * rectangle. An empty rectangle never contains another rectangle.
     *
     * @param rect The rectangle being tested for containment.
     * @return true iff the specified rectangle r is inside or equal to this
     *              rectangle
     */
    operator fun contains(rect: Rect): Boolean {
        val thisRect = toRectF()
        val otherRect = rect.toRectF()
        return thisRect.contains(otherRect)
    }

    /**
     * If the specified rectangle intersects this rectangle, return true and set
     * this rectangle to that intersection, otherwise return false and do not
     * change this rectangle. No check is performed to see if either rectangle
     * is empty. To just test for intersection, use intersects()
     *
     * @param rect The rectangle being intersected with this rectangle.
     * @return A rectangle with the intersection coordinates
     */
    fun intersection(rect: Rect): Rect {
        val thisRect = toRectF()
        val otherRect = rect.toRectF()
        return thisRect.intersection(otherRect).toRect()
    }

    override fun hashCode(): Int {
        var result = left
        result = 31 * result + top
        result = 31 * result + right
        result = 31 * result + bottom
        return result
    }

    override fun toString(): String = if (isEmpty) "[empty]" else prettyPrint()

    companion object {
        val EMPTY = Rect()

        fun prettyPrint(rect: Rect): String = "(${rect.left}, ${rect.top}) - " +
            "(${rect.right}, ${rect.bottom})"
    }
}