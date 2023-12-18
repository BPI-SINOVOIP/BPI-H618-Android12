package com.softwinner.agingdragonbox.engine.testcase;

import android.content.Context;
import android.graphics.Bitmap;
import android.graphics.BitmapFactory;
import android.opengl.GLSurfaceView;
import android.opengl.GLUtils;

import java.io.IOException;
import java.io.InputStream;

import javax.microedition.khronos.egl.EGLConfig;
import javax.microedition.khronos.opengles.GL10;

import com.softwinner.agingdragonbox.R;

public class EarthAndMoonView extends GLSurfaceView {

    private static final float TOUCH_SCALE_FACTOR = 180.0f / 320;
    private SceneRenderer mRenderer;
    private float         mPreviousY;
    private float         mPreviousX;

    public int            earthTextureId;
    public int            moonTextureId;

    public EarthAndMoonView(Context context) {
        super(context);
        mRenderer = new SceneRenderer();
        setRenderer(mRenderer);
        setRenderMode(GLSurfaceView.RENDERMODE_CONTINUOUSLY);
    }

    private class SceneRenderer implements GLSurfaceView.Renderer {
        Ball      earth;
        Ball      moon;
        Celestial celestialSmall;
        Celestial celestialBig;

        public void onDrawFrame(GL10 gl) {
            gl.glClear(GL10.GL_COLOR_BUFFER_BIT | GL10.GL_DEPTH_BUFFER_BIT);
            gl.glMatrixMode(GL10.GL_MODELVIEW);
            gl.glLoadIdentity();

            gl.glTranslatef(0, 0f, -3.6f);

            gl.glEnable(GL10.GL_LIGHTING);
            gl.glPushMatrix();
            gl.glMaterialf(GL10.GL_FRONT_AND_BACK, GL10.GL_SHININESS, 3.5f);
            earth.drawSelf(gl);
            gl.glTranslatef(0, 0f, 1.5f);
            gl.glMaterialf(GL10.GL_FRONT_AND_BACK, GL10.GL_SHININESS, 1.0f);
            moon.drawSelf(gl);
            gl.glPopMatrix();
            gl.glDisable(GL10.GL_LIGHTING);

            gl.glPushMatrix();
            gl.glTranslatef(0, -8.0f, 0.0f);
            celestialSmall.drawSelf(gl);
            celestialBig.drawSelf(gl);
            gl.glPopMatrix();

        }

        public void onSurfaceChanged(GL10 gl, int width, int height) {
            gl.glViewport(0, 0, width, height);
            gl.glMatrixMode(GL10.GL_PROJECTION);
            gl.glLoadIdentity();
            float ratio = (float) width / height;
            gl.glFrustumf(-ratio * 0.5f, ratio * 0.5f, -0.5f, 0.5f, 1, 100);
        }

        public void onSurfaceCreated(GL10 gl, EGLConfig config) {
            gl.glDisable(GL10.GL_DITHER);
            gl.glHint(GL10.GL_PERSPECTIVE_CORRECTION_HINT, GL10.GL_FASTEST);
            gl.glClearColor(0, 0, 0, 0);
            gl.glShadeModel(GL10.GL_SMOOTH);
            gl.glEnable(GL10.GL_DEPTH_TEST);
            gl.glEnable(GL10.GL_CULL_FACE);

            gl.glEnable(GL10.GL_LIGHTING);
            initSunLight(gl);
            initMaterial(gl);

            earthTextureId = initTexture(gl, R.drawable.earth);
            moonTextureId = initTexture(gl, R.drawable.moon);

            earth = new Ball(6, earthTextureId);
            moon = new Ball(2, moonTextureId);

            celestialSmall = new Celestial(0, 0, 1, 0, 750);
            celestialBig = new Celestial(0, 0, 2, 0, 200);

            new Thread() {
                public void run() {
                    while (true) {
                        mRenderer.earth.mAngleY += 2 * TOUCH_SCALE_FACTOR;
                        mRenderer.moon.mAngleY += 2 * TOUCH_SCALE_FACTOR;
                        requestRender();
                        try {
                            Thread.sleep(50);
                        } catch (Exception e) {
                            e.printStackTrace();
                        }
                    }
                }
            }.start();

            new Thread() {
                public void run() {
                    while (true) {
                        celestialSmall.mYAngle += 0.5;
                        if (celestialSmall.mYAngle >= 360) {
                            celestialSmall.mYAngle = 0;
                        }
                        celestialBig.mYAngle += 0.5;
                        if (celestialBig.mYAngle >= 360) {
                            celestialBig.mYAngle = 0;
                        }
                        try {
                            Thread.sleep(100);
                        } catch (InterruptedException e) {
                            e.printStackTrace();
                        }
                    }
                }
            }.start();
        }
    }

    private void initSunLight(GL10 gl) {
        gl.glEnable(GL10.GL_LIGHT0);

        float[] ambientParams = {0.05f, 0.05f, 0.025f, 1.0f };
        gl.glLightfv(GL10.GL_LIGHT0, GL10.GL_AMBIENT, ambientParams, 0);

        float[] diffuseParams = {1f, 1f, 0.5f, 1.0f };
        gl.glLightfv(GL10.GL_LIGHT0, GL10.GL_DIFFUSE, diffuseParams, 0);

        float[] specularParams = {1f, 1f, 0.5f, 1.0f };
        gl.glLightfv(GL10.GL_LIGHT0, GL10.GL_SPECULAR, specularParams, 0);

        float[] positionParamsGreen = {-14.14f, 8.28f, 6f, 0 };
        gl.glLightfv(GL10.GL_LIGHT0, GL10.GL_POSITION, positionParamsGreen, 0);
    }

    private void initMaterial(GL10 gl) {
        float[] ambientMaterial = {0.7f, 0.7f, 0.7f, 1.0f };
        gl.glMaterialfv(GL10.GL_FRONT_AND_BACK, GL10.GL_AMBIENT, ambientMaterial, 0);
        float[] diffuseMaterial = {1.0f, 1.0f, 1.0f, 1.0f };
        gl.glMaterialfv(GL10.GL_FRONT_AND_BACK, GL10.GL_DIFFUSE, diffuseMaterial, 0);
        float[] specularMaterial = {1f, 1f, 1f, 1.0f };
        gl.glMaterialfv(GL10.GL_FRONT_AND_BACK, GL10.GL_SPECULAR, specularMaterial, 0);
    }

    public int initTexture(GL10 gl, int drawableId) {
        int[] textures = new int[1];
        gl.glGenTextures(1, textures, 0);
        int textureId = textures[0];
        gl.glBindTexture(GL10.GL_TEXTURE_2D, textureId);
        gl.glTexParameterf(GL10.GL_TEXTURE_2D, GL10.GL_TEXTURE_MIN_FILTER, GL10.GL_NEAREST);
        gl.glTexParameterf(GL10.GL_TEXTURE_2D, GL10.GL_TEXTURE_MAG_FILTER, GL10.GL_LINEAR);
        gl.glTexParameterf(GL10.GL_TEXTURE_2D, GL10.GL_TEXTURE_WRAP_S, GL10.GL_CLAMP_TO_EDGE);
        gl.glTexParameterf(GL10.GL_TEXTURE_2D, GL10.GL_TEXTURE_WRAP_T, GL10.GL_CLAMP_TO_EDGE);

        InputStream is = this.getResources().openRawResource(drawableId);
        Bitmap bitmapTmp;
        try {
            bitmapTmp = BitmapFactory.decodeStream(is);
        } finally {
            try {
                is.close();
            } catch (IOException e) {
                e.printStackTrace();
            }
        }
        GLUtils.texImage2D(GL10.GL_TEXTURE_2D, 0, bitmapTmp, 0);
        bitmapTmp.recycle();

        return textureId;
    }
}
