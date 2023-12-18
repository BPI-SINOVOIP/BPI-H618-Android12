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

data class RectF(
    val left: Float = 0f,
    val top: Float = 0f,
    val right: Float = 0f,
    val bottom: Float = 0f
) {
    val height: Float get() = bottom - top
    val width: Float get() = right - left

    /**
     * Returns true if the rectangle is empty (left >= right or top >= bottom)
     */
    val isEmpty: Boolean
        get() = width == 0f || height == 0f
    val isNotEmpty: Boolean
        get() = !isEmpty

    /**
     * Returns a [Rect] version fo this rectangle.
     *
     * All fractional parts are rounded to 0
     */
    fun toRect(): Rect {
        return Rect(left.toInt(), top.toInt(), right.toInt(), bottom.toInt())
    }

    /**
     * Returns true iff the specified rectangle r is inside or equal to this
     * rectangle. An empty rectangle never contains another rectangle.
     *
     * @param r The rectangle being tested for containment.
     * @return true iff the specified rectangle r is inside or equal to this
     *              rectangle
     */
    operator fun contains(r: RectF): Boolean {
        // check for empty first
        return this.left < this.right && this.top < this.bottom && // now check for containment
                left <= r.left && top <= r.top && right >= r.right && bottom >= r.bottom
    }

    /**
     * If the rectangle specified by left,top,right,bottom intersects this
     * rectangle, return true and set this rectangle to that intersection,
     * otherwise return false and do not change this rectangle. No check is
     * performed to see if either rectangle is empty. Note: To just test for
     * intersection, use intersects()
     *
     * @param left The left side of the rectangle being intersected with this
     * rectangle
     * @param top The top of the rectangle being intersected with this rectangle
     * @param right The right side of the rectangle being intersected with this
     * rectangle.
     * @param bottom The bottom of the rectangle being intersected with this
     * rectangle.
     * @return A rectangle with the intersection coordinates
     */
    fun intersection(left: Float, top: Float, right: Float, bottom: Float): RectF {
        if (this.left < right && left < this.right && this.top < bottom && top < this.bottom) {
            var intersectionLeft = 0f
            var intersectionTop = 0f
            var intersectionRight = 0f
            var intersectionBottom = 0f

            if (this.left < left) {
                intersectionLeft = left
            }
            if (this.top < top) {
                intersectionTop = top
            }
            if (this.right > right) {
                intersectionRight = right
            }
            if (this.bottom > bottom) {
                intersectionBottom = bottom
            }
            return RectF(intersectionLeft, intersectionTop, intersectionRight, intersectionBottom)
        }
        return EMPTY
    }

    /**
     * If the specified rectangle intersects this rectangle, return true and set
     * this rectangle to that intersection, otherwise return false and do not
     * change this rectangle. No check is performed to see if either rectangle
     * is empty. To just test for intersection, use intersects()
     *
     * @param r The rectangle being intersected with this rectangle.
     * @return A rectangle with the intersection coordinates
     */
    fun intersection(r: RectF): RectF = intersection(r.left, r.top, r.right, r.bottom)

    fun prettyPrint(): String = prettyPrint(this)

    override fun toString(): String = if (isEmpty) "[empty]" else prettyPrint()

    companion object {
        val EMPTY = RectF()

        fun prettyPrint(rect: RectF): String {
            val left = FloatFormatter.format(rect.left)
            val top = FloatFormatter.format(rect.top)
            val right = FloatFormatter.format(rect.right)
            val bottom = FloatFormatter.format(rect.bottom)
            return "($left, $top) - ($right, $bottom)"
        }
    }
}
