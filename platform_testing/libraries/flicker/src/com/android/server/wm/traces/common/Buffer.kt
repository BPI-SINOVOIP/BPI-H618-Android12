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

class Buffer(width: Int, height: Int, val stride: Int, val format: Int) : Bounds(width, height) {
    override fun prettyPrint(): String = prettyPrint(this)

    override fun equals(other: Any?): Boolean =
        other is Buffer &&
        other.height == height &&
        other.width == width &&
        other.stride == stride &&
        other.format == format

    override fun hashCode(): Int {
        var result = height
        result = 31 * result + width
        result = 31 * result + stride
        result = 31 * result + format
        return result
    }

    companion object {
        val EMPTY: Buffer = Buffer(0, 0, 0, 0)

        fun prettyPrint(buffer: Buffer): String = "w:${buffer.width}, h:${buffer.height}, " +
            "stride:${buffer.stride}, format:${buffer.format}"
    }
}
