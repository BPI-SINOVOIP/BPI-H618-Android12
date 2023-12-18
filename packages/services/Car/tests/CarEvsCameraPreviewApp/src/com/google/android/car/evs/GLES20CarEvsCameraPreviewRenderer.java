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

import static android.opengl.GLU.gluErrorString;

import android.car.evs.CarEvsBufferDescriptor;
import android.content.Context;
import android.graphics.Bitmap;
import android.graphics.Canvas;
import android.graphics.Paint;
import android.graphics.drawable.Drawable;
import android.hardware.HardwareBuffer;
import android.opengl.GLES20;
import android.opengl.GLSurfaceView;
import android.opengl.GLUtils;
import android.util.Log;

import androidx.annotation.GuardedBy;

import java.nio.ByteBuffer;
import java.nio.ByteOrder;
import java.nio.FloatBuffer;
import java.util.Random;

import javax.microedition.khronos.egl.EGLConfig;
import javax.microedition.khronos.opengles.GL10;

/**
 * GLES20 SurfaceView Renderer
 */
public final class GLES20CarEvsCameraPreviewRenderer implements GLSurfaceView.Renderer {
    private static final String TAG = GLES20CarEvsCameraPreviewRenderer.class.getSimpleName();
    private static final int FLOAT_SIZE_BYTES = 4;

    private static final float[] sVertCarPosData = {
            -1.0f,  1.0f, 0.0f,
             1.0f,  1.0f, 0.0f,
            -1.0f, -1.0f, 0.0f,
             1.0f, -1.0f, 0.0f };

    private static final float[] sVertCarTexData = {
            0.0f, 0.0f,
            1.0f, 0.0f,
            0.0f, 1.0f,
            1.0f, 1.0f };

    private static final float[] sIdentityMatrix = {
            1.0f, 0.0f, 0.0f, 0.0f,
            0.0f, 1.0f, 0.0f, 0.0f,
            0.0f, 0.0f, 1.0f, 0.0f,
            0.0f, 0.0f, 0.0f, 1.0f };

    private final String mVertexShader =
        "#version 300 es                    \n" +
        "layout(location = 0) in vec4 pos;  \n" +
        "layout(location = 1) in vec2 tex;  \n" +
        "uniform mat4 cameraMat;            \n" +
        "out vec2 uv;                       \n" +
        "void main()                        \n" +
        "{                                  \n" +
        "   gl_Position = cameraMat * pos;  \n" +
        "   uv = tex;                       \n" +
        "}                                  \n";

    private final String mFragmentShader =
        "#version 300 es                    \n" +
        "precision mediump float;           \n" +
        "uniform sampler2D tex;             \n" +
        "in vec2 uv;                        \n" +
        "out vec4 color;                    \n" +
        "void main()                        \n" +
        "{                                  \n" +
        "    vec4 texel = texture(tex, uv); \n" +
        "    color = texel;                 \n" +
        "}                                  \n";

    private final Object mLock = new Object();

    private CarEvsCameraPreviewActivity mActivity;

    @GuardedBy("mLock")
    private CarEvsBufferDescriptor mBufferInUse = null;

    public GLES20CarEvsCameraPreviewRenderer(Context context,
            CarEvsCameraPreviewActivity activity) {

        mContext = context;
        mActivity = activity;

        mVertCarPos = ByteBuffer.allocateDirect(sVertCarPosData.length * FLOAT_SIZE_BYTES)
                .order(ByteOrder.nativeOrder()).asFloatBuffer();
        mVertCarPos.put(sVertCarPosData).position(0);
        mVertCarTex = ByteBuffer.allocateDirect(sVertCarTexData.length * FLOAT_SIZE_BYTES)
                .order(ByteOrder.nativeOrder()).asFloatBuffer();
        mVertCarTex.put(sVertCarTexData).position(0);
    }

    public void clearBuffer() {
        CarEvsBufferDescriptor bufferToReturn = null;
        synchronized (mLock) {
            if (mBufferInUse == null) {
                return;
            }

            bufferToReturn = mBufferInUse;
            mBufferInUse = null;
        }

        // bufferToReturn is not null here.
        mActivity.returnBuffer(bufferToReturn);
    }

    @Override
    public void onDrawFrame(GL10 glUnused) {
        // Use the GLES20 class's static methods instead of a passed GL10 interface.

        CarEvsBufferDescriptor bufferToRender = null;
        CarEvsBufferDescriptor bufferToReturn = null;
        CarEvsBufferDescriptor newFrame = mActivity.getNewFrame();
        synchronized (mLock) {
            if (newFrame != null) {
                // If a new frame has not been delivered yet, we're using a previous frame.
                if (mBufferInUse != null) {
                    bufferToReturn = mBufferInUse;
                }
                mBufferInUse = newFrame;
            }
            bufferToRender = mBufferInUse;
        }

        if (bufferToReturn != null) {
            mActivity.returnBuffer(bufferToReturn);
        }

        // Specify a shader program to use
        GLES20.glUseProgram(mProgram);

        // Set a cameraMat as 4x4 identity matrix
        int matrix = GLES20.glGetUniformLocation(mProgram, "cameraMat");
        if (matrix < 0) {
            throw new RuntimeException("Could not get a attribute location for cameraMat");
        }
        GLES20.glUniformMatrix4fv(matrix, 1, false, sIdentityMatrix, 0);

        if (bufferToRender == null) {
            // Show the default screen
            drawDefaultScreen();
        } else {
            // Retrieve a hardware buffer from a descriptor and update the texture
            HardwareBuffer buffer = bufferToRender.getHardwareBuffer();
            if (buffer == null) {
                Log.e(TAG, "HardwareBuffer is invalid.");
                drawDefaultScreen();
            } else {
                // Update the texture with a given hardware buffer
                if (!nUpdateTexture(buffer, mTextureId)) {
                    throw new RuntimeException(
                            "Failed to update the texture with the preview frame");
                }
            }
        }

        GLES20.glClearColor(1.0f, 0.0f, 0.0f, 1.0f);
        GLES20.glClear(GLES20.GL_DEPTH_BUFFER_BIT | GLES20.GL_COLOR_BUFFER_BIT);

        // Select active texture unit
        GLES20.glActiveTexture(GLES20.GL_TEXTURE0);

        // Bind a named texture to the target
        GLES20.glBindTexture(GLES20.GL_TEXTURE_2D, mTextureId);

        // Use a texture slot 0 as the source
        int sampler = GLES20.glGetUniformLocation(mProgram, "tex");
        if (sampler < 0) {
            throw new RuntimeException("Could not get a attribute location for tex");
        }
        GLES20.glUniform1i(sampler, 0);

        // We'll ignore the alpha value
        GLES20.glDisable(GLES20.GL_BLEND);

        // Define an array of generic vertex attribute data
        GLES20.glVertexAttribPointer(0, 3, GLES20.GL_FLOAT, false, 0, mVertCarPos);
        GLES20.glVertexAttribPointer(1, 2, GLES20.GL_FLOAT, false, 0, mVertCarTex);

        // Enable a generic vertex attribute array
        GLES20.glEnableVertexAttribArray(0);
        GLES20.glEnableVertexAttribArray(1);

        // Render primitives from array data
        GLES20.glDrawArrays(GLES20.GL_TRIANGLE_STRIP, 0, 4);

        GLES20.glDisableVertexAttribArray(0);
        GLES20.glDisableVertexAttribArray(1);

        // Wait until all GL execution is complete
        GLES20.glFinish();
    }

    @Override
    public void onSurfaceChanged(GL10 glUnused, int width, int height) {
        // Use the GLES20 class's static methods instead of a passed GL10 interface.
        GLES20.glViewport(0, 0, width, height);

        mWidth = width;
        mHeight = height;
    }

    @Override public void onSurfaceCreated(GL10 glUnused, EGLConfig config) {
        // Use the GLES20 class's static methods instead of a passed GL10 interface.
        mProgram = buildShaderProgram(mVertexShader, mFragmentShader);
        if (mProgram == 0) {
            Log.e(TAG, "Failed to build shader programs");
            return;
        }

        // Generate texture name
        int[] textures = new int[1];
        GLES20.glGenTextures(1, textures, 0);
        mTextureId = textures[0];
        if (mTextureId <= 0) {
            Log.e(TAG, "Did not get a texture handle");
            return;
        }

        GLES20.glBindTexture(GLES20.GL_TEXTURE_2D, mTextureId);
        // Use a linear interpolation to upscale the texture
        GLES20.glTexParameteri(GLES20.GL_TEXTURE_2D, GLES20.GL_TEXTURE_MAG_FILTER,
                GLES20.GL_LINEAR);
        // Use a nearest-neighbor to downscale the texture
        GLES20.glTexParameteri(GLES20.GL_TEXTURE_2D, GLES20.GL_TEXTURE_MIN_FILTER,
                GLES20.GL_NEAREST);
        // Clamp s, t coordinates at the edges
        GLES20.glTexParameteri(GLES20.GL_TEXTURE_2D, GLES20.GL_TEXTURE_WRAP_S,
                GLES20.GL_CLAMP_TO_EDGE);
        GLES20.glTexParameteri(GLES20.GL_TEXTURE_2D, GLES20.GL_TEXTURE_WRAP_T,
                GLES20.GL_CLAMP_TO_EDGE);
    }

    public void setTextLocation(float x, float y) {
        mTextX = x;
        mTextY = y;
    }

    private void drawDefaultScreen() {
        Drawable drawable = mContext.getResources().getDrawable(R.drawable.rearview);
        Bitmap bitmap = Bitmap.createBitmap(mWidth, mHeight, Bitmap.Config.ARGB_4444);
        Canvas canvas = new Canvas(bitmap);
        drawable.setBounds(0, 0, mWidth, mHeight);
        drawable.draw(canvas);

        Paint fontColor = new Paint();
        fontColor.setTextSize(Math.min(mWidth, mHeight) * 0.10f);
        fontColor.setAntiAlias(true);

        // Pick a font color randomly
        fontColor.setColor((0xFF << 24) + (mRandom.nextInt(0xFF) << 16)
                + (mRandom.nextInt(0xFF) << 8) + mRandom.nextInt(0xFF));

        // Set a location of the text relative to the surface size
        canvas.drawText("The rearview is not available.", mTextX, mTextY, fontColor);

        // Draw
        GLUtils.texImage2D(GLES20.GL_TEXTURE_2D, /* level = */ 0, bitmap, /* border = */ 0);
        bitmap.recycle();
    }

    private int loadShader(int shaderType, String source) {
        int shader = GLES20.glCreateShader(shaderType);
        if (shader == 0) {
            Log.e(TAG, "Failed to create a shader for " + source);
            return 0;
        }

        GLES20.glShaderSource(shader, source);
        GLES20.glCompileShader(shader);
        int[] compiled = new int[1];
        GLES20.glGetShaderiv(shader, GLES20.GL_COMPILE_STATUS, compiled, 0);
        if (compiled[0] == 0) {
            Log.e(TAG, "Could not compile shader " + shaderType + ": ");
            Log.e(TAG, GLES20.glGetShaderInfoLog(shader));
            GLES20.glDeleteShader(shader);
            return 0;
        }

        return shader;
    }

    private int buildShaderProgram(String vertexSource, String fragmentSource) {
        int vertexShader = loadShader(GLES20.GL_VERTEX_SHADER, vertexSource);
        if (vertexShader == 0) {
            Log.e(TAG, "Failed to load a vertex shader");
            return 0;
        }

        int fragmentShader = loadShader(GLES20.GL_FRAGMENT_SHADER, fragmentSource);
        if (fragmentShader == 0) {
            Log.e(TAG, "Failed to load a fragment shader");
            return 0;
        }

        int program = GLES20.glCreateProgram();
        if (program == 0) {
            Log.e(TAG, "Failed to create a program");
            return 0;
        }

        GLES20.glAttachShader(program, vertexShader);
        checkGlError("glAttachShader");
        GLES20.glAttachShader(program, fragmentShader);
        checkGlError("glAttachShader");
        GLES20.glLinkProgram(program);
        int[] linkStatus = new int[1];
        GLES20.glGetProgramiv(program, GLES20.GL_LINK_STATUS, linkStatus, 0);
        if (linkStatus[0] != GLES20.GL_TRUE) {
            Log.e(TAG, "Could not link a program: ");
            Log.e(TAG, GLES20.glGetProgramInfoLog(program));
            GLES20.glDeleteProgram(program);
            return 0;
        }

        return program;
    }

    private static void checkGlError(String op) {
        int error;
        while ((error = GLES20.glGetError()) != GLES20.GL_NO_ERROR) {
            Log.e(TAG, op + ": glError " + error);
            throw new RuntimeException(op + ": glError " + gluErrorString(error));
        }
    }

    private Context mContext;
    private int mProgram;
    private int mTextureId;
    private FloatBuffer mVertCarPos;
    private FloatBuffer mVertCarTex;
    private int mWidth;
    private int mHeight;
    private float mTextX;
    private float mTextY;
    private Random mRandom = new Random();

    static {
        System.loadLibrary("carevsglrenderer_jni");
    }

    // Native method to update the texture with a received frame buffer
    private native boolean nUpdateTexture(HardwareBuffer buffer, int textureId);
}
