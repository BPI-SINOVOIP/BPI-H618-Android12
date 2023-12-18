package com.allwinnertech.socs.gpu;

import android.content.Context;
import android.opengl.Matrix;

import java.util.Arrays;

public class Cube {
    private GLFace[] mGLFaces = new GLFace[6];

    float[] red = new float[]{1.0f, 0.0f, 0.0f, 1.0f};
    float[] green = new float[]{0.0f, 1.0f, 0.0f, 1.0f};
    float[] blue = new float[]{0.0f, 0.0f, 1.0f, 1.0f};
    float[] yellow = new float[]{1.0f, 1.0f, 0.0f, 1.0f};
    float[] orange = new float[]{1.0f, 0.5f, 0.0f, 1.0f};
    float[] white = new float[]{1.0f, 1.0f, 1.0f, 1.0f};
    float[] black = new float[]{0.0f, 0.0f, 0.0f, 1.0f};
    private final GLVertex leftBottomBack;
    private final GLVertex rightBottomBack;
    private final GLVertex leftTopBack;
    private final GLVertex rightTopBack;
    private final GLVertex leftBottomFront;
    private final GLVertex rightBottomFront;
    private final GLVertex leftTopFront;
    private final GLVertex rightTopFront;
    private float[] mModelMatrix = new float[16];

    public Cube(float left, float bottom, float back, float right, float top, float front) {
        leftBottomBack = new GLVertex(left, bottom, back);
        rightBottomBack = new GLVertex(right, bottom, back);
        leftTopBack = new GLVertex(left, top, back);
        rightTopBack = new GLVertex(right, top, back);
        leftBottomFront = new GLVertex(left, bottom, front);
        rightBottomFront = new GLVertex(right, bottom, front);
        leftTopFront = new GLVertex(left, top, front);
        rightTopFront = new GLVertex(right, top, front);
        Matrix.setIdentityM(mModelMatrix, 0);
    }

    public void setData(Context context, Kube kube) {
        //front
        if (Arrays.asList(kube.layers[4].getCubes()).contains(this) || Arrays.asList(kube.layers[5].getCubes()).contains(this)) {
            mGLFaces[0] = new GLFace(context, leftTopFront, leftBottomFront, rightBottomFront, rightTopFront, black);
        } else {
            mGLFaces[0] = new GLFace(context, leftTopFront, leftBottomFront, rightBottomFront, rightTopFront, green);
        }
        //left
        if (Arrays.asList(kube.layers[7].getCubes()).contains(this) || Arrays.asList(kube.layers[8].getCubes()).contains(this)) {
            mGLFaces[1] = new GLFace(context, leftTopBack, leftBottomBack, leftBottomFront, leftTopFront, black);
        } else {
            mGLFaces[1] = new GLFace(context, leftTopBack, leftBottomBack, leftBottomFront, leftTopFront, yellow);
        }
        //top
        if (Arrays.asList(kube.layers[1].getCubes()).contains(this) || Arrays.asList(kube.layers[2].getCubes()).contains(this)) {
            mGLFaces[2] = new GLFace(context, rightTopBack, leftTopBack, leftTopFront, rightTopFront, black);
        } else {
            mGLFaces[2] = new GLFace(context, rightTopBack, leftTopBack, leftTopFront, rightTopFront, orange);
        }
        //back
        if (Arrays.asList(kube.layers[3].getCubes()).contains(this) || Arrays.asList(kube.layers[4].getCubes()).contains(this)) {
            mGLFaces[3] = new GLFace(context, rightBottomBack, leftBottomBack, leftTopBack, rightTopBack, black);
        } else {
            mGLFaces[3] = new GLFace(context, rightBottomBack, leftBottomBack, leftTopBack, rightTopBack, blue);
        }
        //right
        if (Arrays.asList(kube.layers[6].getCubes()).contains(this) || Arrays.asList(kube.layers[7].getCubes()).contains(this)) {
            mGLFaces[4] = new GLFace(context, rightBottomFront, rightBottomBack, rightTopBack, rightTopFront, black);
        } else {
            mGLFaces[4] = new GLFace(context, rightBottomFront, rightBottomBack, rightTopBack, rightTopFront, white);
        }
        //bottom
        if (Arrays.asList(kube.layers[0].getCubes()).contains(this) || Arrays.asList(kube.layers[1].getCubes()).contains(this)) {
            mGLFaces[5] = new GLFace(context, leftBottomFront, leftBottomBack, rightBottomBack, rightBottomFront, black);
        } else {
            mGLFaces[5] = new GLFace(context, leftBottomFront, leftBottomBack, rightBottomBack, rightBottomFront, red);
        }
    }

    public void setModelMatrix(float[] angleMatrix) {
        Matrix.multiplyMM(mModelMatrix, 0, angleMatrix, 0, mModelMatrix, 0);
    }

    public void draw(float[] modelMatrix, float[] vpMatrix) {
        float[] mvpMatrix = new float[16];
        Matrix.setIdentityM(mvpMatrix, 0);
        Matrix.multiplyMM(mvpMatrix, 0, modelMatrix, 0, mModelMatrix, 0);
        Matrix.multiplyMM(mvpMatrix, 0, vpMatrix, 0, mvpMatrix, 0);
        for (GLFace glFace : mGLFaces) {
            if (glFace != null) {
                glFace.draw(mvpMatrix);
            }
        }
    }
}
