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

package com.google.android.car.evs;

import android.content.Context;
import android.opengl.GLSurfaceView;
import android.view.MotionEvent;

/**
 * GPU-backed SurfaceView to render a hardware buffer described by CarEvsBufferDescriptor.
 */
public final class CarEvsCameraGLSurfaceView extends GLSurfaceView {
    private static final String TAG = CarEvsCameraGLSurfaceView.class.getSimpleName();

    private final GLES20CarEvsCameraPreviewRenderer mRenderer;

    public CarEvsCameraGLSurfaceView(Context context, CarEvsCameraPreviewActivity activity) {
        super(context);
        setEGLContextClientVersion(2);

        mRenderer = new GLES20CarEvsCameraPreviewRenderer(context, activity);
        setRenderer(mRenderer);

        setRenderMode(GLSurfaceView.RENDERMODE_CONTINUOUSLY);
    }

    @Override
    public boolean onTouchEvent(MotionEvent e) {
        float x = e.getX();
        float y = e.getY();

        // Update a location of a text to tell the rearview is not available.
        mRenderer.setTextLocation(x, y);

        return true;
    }

    public void clearBuffer() {
        mRenderer.clearBuffer();
    }
}
