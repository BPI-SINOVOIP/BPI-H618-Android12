package com.allwinnertech.socs.gpu;

import android.content.Context;
import android.opengl.GLES20;
import android.opengl.GLSurfaceView;
import android.opengl.Matrix;
import android.util.AttributeSet;

import javax.microedition.khronos.egl.EGLConfig;
import javax.microedition.khronos.opengles.GL10;

public class KubeView extends GLSurfaceView {
    private KubeRenderer mRenderer;

    public KubeView(Context context) {
        this(context, null);
    }

    public KubeView(Context context, AttributeSet attrs) {
        super(context, attrs);
        setEGLContextClientVersion(2);
        mRenderer = new KubeRenderer(context);
        setRenderer(mRenderer);
    }

    public class KubeRenderer implements Renderer {
        private Kube mKube;
        private Context mContext;
        private float mAngle = 0;
        private float mWholeAngle = 0;

        private final float[] vpMatrix = new float[16];
        private final float[] vpLayerMatrix = new float[16];
        private final float[] projectionMatrix = new float[16];
        private final float[] viewMatrix = new float[16];
        private final float[] modelMatrix = new float[16];
        private final float[] currentAngleMatrix = new float[16];

        public KubeRenderer(Context context) {
            mContext = context;
        }

        @Override
        public void onSurfaceCreated(GL10 gl, EGLConfig eglConfig) {
            mKube = new Kube(mContext);
            mAngle = 0;
            mWholeAngle = 0;
            Matrix.setIdentityM(currentAngleMatrix, 0);
            mKube.rotateLayer();
        }

        @Override
        public void onSurfaceChanged(GL10 gl, int width, int height) {
            GLES20.glViewport(0, 0, width, height);
            float ratio = (float) width / height;
            Matrix.frustumM(projectionMatrix, 0, -ratio, ratio, -1, 1, 2, 6);
            GLES20.glDisable(GL10.GL_DITHER);
        }

        @Override
        public void onDrawFrame(GL10 gl) {
            mWholeAngle += 1.2f;
            GLES20.glClearColor(0.5f, 0.5f, 0.5f, 1);
            GLES20.glClear(GLES20.GL_COLOR_BUFFER_BIT | GLES20.GL_DEPTH_BUFFER_BIT);
            Matrix.setLookAtM(viewMatrix, 0, 1, 1, 3, 0f, 0f, 0f, 0f, 1.0f, 0.0f);
            Matrix.setIdentityM(modelMatrix, 0);
            Matrix.scaleM(modelMatrix, 0, 0.5f, 0.5f, 0.5f);
            Matrix.rotateM(viewMatrix, 0, mWholeAngle, 0, 1, 0);
            Matrix.rotateM(viewMatrix, 0, mWholeAngle * 0.25f, 1, 0, 0);
            Matrix.multiplyMM(vpMatrix, 0, projectionMatrix, 0, viewMatrix, 0);
            mAngle += 4.5f;
            int axis = mKube.currentLayer.getmAxis();
            Matrix.rotateM(currentAngleMatrix, 0, 4.5f, axis == Layer.sAxisX ? 1 : 0, axis == Layer.sAxisY ? 1 : 0, axis == Layer.sAxisZ ? 1 : 0);
            if (mAngle >= 90) {
                mKube.setModelMatrix(currentAngleMatrix);
                Matrix.setIdentityM(currentAngleMatrix, 0);
                mKube.updateLayer();
                mKube.rotateLayer();
                mAngle = 0;
            }
            Matrix.multiplyMM(vpLayerMatrix, 0, viewMatrix, 0, currentAngleMatrix, 0);
            Matrix.multiplyMM(vpLayerMatrix, 0, projectionMatrix, 0, vpLayerMatrix, 0);
            GLES20.glEnable(GLES20.GL_CULL_FACE);
            GLES20.glEnable(GLES20.GL_DEPTH_TEST);
            mKube.draw(modelMatrix, vpLayerMatrix, vpMatrix);
        }
    }
}
