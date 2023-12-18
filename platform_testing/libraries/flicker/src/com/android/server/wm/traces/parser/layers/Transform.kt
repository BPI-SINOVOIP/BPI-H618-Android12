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

package com.android.server.wm.traces.parser.layers

import android.surfaceflinger.nano.Layers
import com.android.server.wm.traces.common.layers.Transform
import com.android.server.wm.traces.common.layers.Transform.Companion.FLIP_H_VAL
import com.android.server.wm.traces.common.layers.Transform.Companion.FLIP_V_VAL
import com.android.server.wm.traces.common.layers.Transform.Companion.ROTATE_VAL
import com.android.server.wm.traces.common.layers.Transform.Companion.ROT_90_VAL
import com.android.server.wm.traces.common.layers.Transform.Companion.SCALE_VAL
import com.android.server.wm.traces.common.layers.Transform.Companion.isFlagClear
import com.android.server.wm.traces.common.layers.Transform.Companion.isFlagSet

class Transform(transform: Layers.TransformProto?, position: Layers.PositionProto?) :
        Transform(
            transform?.type,
            getMatrix(transform, position)
        )

private fun getMatrix(transform: Layers.TransformProto?, position: Layers.PositionProto?):
        Transform.Matrix {
    val x = position?.x ?: 0f
    val y = position?.y ?: 0f

    return when {
        transform == null || Transform.isSimpleTransform(transform.type) ->
            transform?.type.getDefaultTransform(x, y)
        else ->
            Transform.Matrix(transform.dsdx, transform.dtdx, x, transform.dsdy, transform.dtdy, y)
    }
}

private fun Int?.getDefaultTransform(x: Float, y: Float): Transform.Matrix {
    return when {
        // IDENTITY
        this == null ->
            Transform.Matrix(1f, 0f, x, 0f, 1f, y)
        // // ROT_270 = ROT_90|FLIP_H|FLIP_V
        isFlagSet(ROT_90_VAL or FLIP_V_VAL or FLIP_H_VAL) ->
            Transform.Matrix(0f, -1f, x, 1f, 0f, y)
        // ROT_180 = FLIP_H|FLIP_V
        isFlagSet(FLIP_V_VAL or FLIP_H_VAL) ->
            Transform.Matrix(-1f, 0f, x, 0f, -1f, y)
        // ROT_90
        isFlagSet(ROT_90_VAL) ->
            Transform.Matrix(0f, 1f, x, -1f, 0f, y)
        // IDENTITY
        isFlagClear(SCALE_VAL or ROTATE_VAL) ->
            Transform.Matrix(1f, 0f, x, 0f, 1f, y)
        else ->
            throw IllegalStateException("Unknown transform type $this")
    }
}