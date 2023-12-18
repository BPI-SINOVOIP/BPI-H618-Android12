package com.softwinner.agingdragonbox.engine.testcase;

import android.content.Context;
import android.opengl.GLSurfaceView;
import android.opengl.GLSurfaceView.Renderer;
import android.opengl.GLU;
import android.util.AttributeSet;
import android.util.Log;
import android.view.ViewGroup;

import java.nio.ByteBuffer;
import java.nio.ByteOrder;
import java.nio.FloatBuffer;
import java.nio.ShortBuffer;

import javax.microedition.khronos.egl.EGLConfig;
import javax.microedition.khronos.opengles.GL10;

/**
 * 3D测试界面
 */
public class ThreeDimensionalView extends GLSurfaceView implements Renderer {
    private static final String  TAG        = "3D";
    private ThreeDimensionalView mThreeDimensionalView;
    protected ViewGroup          mStage;
    private static final float   X          = .525731112119133606f;
    private static final float   Z          = .850650808352039932f;
    private static float[]       vertices   = {-X, 0.0f, Z, X, 0.0f, Z, -X, 0.0f, -Z,
            X, 0.0f, -Z, 0.0f, Z, X, 0.0f, Z, -X, 0.0f, -Z, X, 0.0f, -Z, -X, Z, X, 0.0f, -Z, X,
            0.0f, Z, -X, 0.0f, -Z, -X, 0.0f };
    private static short[]       indices    = {0, 4, 1, 0, 9, 4, 9, 5, 4, 4, 5, 8, 4,
            8, 1, 8, 10, 1, 8, 3, 10, 5, 3, 8, 5, 2, 3, 2, 7, 3, 7, 10, 3, 7, 6, 10, 7, 11, 6, 11,
            0, 6, 0, 1, 6, 6, 1, 10, 9, 0, 11, 9, 11, 2, 9, 2, 5, 7, 2, 11 };
    // The colors mapped to the vertices.
    private static float[]       colors     = {0f, 0f, 0f, 1f, 0f, 0f, 1f, 1f, 0f, 1f, 0f, 1f, 0f,
            1f, 1f, 1f, 1f, 0f, 0f, 1f, 1f, 0f, 1f, 1f, 1f, 1f, 0f, 1f, 1f, 1f, 1f, 1f, 1f, 0f, 0f,
            1f, 0f, 1f, 0f, 1f, 0f, 0f, 1f, 1f, 1f, 0f, 1f, 1f };

    // buffer
    private FloatBuffer          mVertexBuffer;
    private FloatBuffer          mColorBuffer;
    private ShortBuffer          mIndexBuffer;
    private int                  mAngle;
    private int                  x;

    public ThreeDimensionalView(Context context) {
        super(context);
        init();
    }

    public ThreeDimensionalView(Context context, AttributeSet attrs) {
        super(context, attrs);
        init();
    }

    private void init() {
        ByteBuffer vbb = ByteBuffer.allocateDirect(vertices.length * 4);
        vbb.order(ByteOrder.nativeOrder());
        mVertexBuffer = vbb.asFloatBuffer();
        mVertexBuffer.put(vertices);
        mVertexBuffer.position(0);

        ByteBuffer cbb = ByteBuffer.allocateDirect(colors.length * 4);
        cbb.order(ByteOrder.nativeOrder());
        mColorBuffer = cbb.asFloatBuffer();
        mColorBuffer.put(colors);
        mColorBuffer.position(0);

        ByteBuffer ibb = ByteBuffer.allocateDirect(indices.length * 2);
        ibb.order(ByteOrder.nativeOrder());
        mIndexBuffer = ibb.asShortBuffer();
        mIndexBuffer.put(indices);
        mIndexBuffer.position(0);

        mAngle = 0;
        this.setRenderer(this);
    }

    @Override
    public void onSurfaceCreated(GL10 gl, EGLConfig config) {
        gl.glDisable(GL10.GL_DITHER);
        // Set the background color to black ( rgba ).
        gl.glClearColor(0.0f, 0.0f, 0.0f, 0.5f);
        // Enable Smooth Shading, default not really needed.
        gl.glShadeModel(GL10.GL_SMOOTH);
        // Depth buffer setup.
        gl.glClearDepthf(1.0f);
        // Enables depth testing.
        gl.glEnable(GL10.GL_DEPTH_TEST);
        // The type of depth testing to do.
        gl.glDepthFunc(GL10.GL_LEQUAL);
        // Really nice perspective calculations.
        gl.glHint(GL10.GL_PERSPECTIVE_CORRECTION_HINT, GL10.GL_NEAREST);
        Log.d(TAG, "3D Test........");

    }

    @Override
    public void onDrawFrame(GL10 gl) {
        gl.glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
        // Clears the screen and depth buffer.
        gl.glClear(GL10.GL_COLOR_BUFFER_BIT | GL10.GL_DEPTH_BUFFER_BIT);

        gl.glColor4f(1.0f, 1.0f, 1.0f, 1.0f);
        gl.glLoadIdentity();
        gl.glTranslatef(2.2f, 1.0F, -6);

        for (int i = 0; i < 30; i++) {
            gl.glRotatef(mAngle, 0, -1, 0);
            gl.glRotatef(mAngle, 0, 0, 1);
            gl.glRotatef(mAngle, 1, 0, 0);
        }
        for (int i = 0; i < 30; i++) {
            gl.glRotatef(mAngle, 0, 1, 0);
            gl.glRotatef(mAngle, 0, 0, -1);
            gl.glRotatef(mAngle, -1, 0, 0);
        }

        // gl.glFrontFace(GL10.GL_CCW);
        // gl.glEnable(GL10.GL_CULL_FACE);
        // gl.glCullFace(GL10.GL_BACK);
        gl.glEnableClientState(GL10.GL_VERTEX_ARRAY);
        gl.glVertexPointer(3, GL10.GL_FLOAT, 0, mVertexBuffer);
        gl.glEnableClientState(GL10.GL_COLOR_ARRAY);
        gl.glColorPointer(4, GL10.GL_FLOAT, 0, mColorBuffer);
        gl.glDrawElements(GL10.GL_TRIANGLES, indices.length, GL10.GL_UNSIGNED_SHORT, mIndexBuffer);

        gl.glLoadIdentity();
        gl.glTranslatef(2.2f, -1.0F, -6);

        for (int i = 0; i < 10; i++) {
            gl.glRotatef(mAngle, 0, -1, 0);
            gl.glRotatef(mAngle, 0, 0, 1);
            gl.glRotatef(mAngle, 1, 0, 0);
        }
        for (int i = 0; i < 10; i++) {
            gl.glRotatef(mAngle, 0, 1, 0);
            gl.glRotatef(mAngle, 0, 0, -1);
            gl.glRotatef(mAngle, -1, 0, 0);
        }
        gl.glDrawElements(GL10.GL_TRIANGLES, indices.length, GL10.GL_UNSIGNED_SHORT, mIndexBuffer);

        gl.glLoadIdentity();
        gl.glTranslatef(-1.0f, 0, -4);
        // gl.glRotatef(mAngle, 0, 1, 0);
        for (int i = 0; i < 5; i++) {
            gl.glRotatef(mAngle, 0, -1, 0);
            gl.glRotatef(mAngle, 0, 0, 1);
            gl.glRotatef(mAngle, 1, 0, 0);
        }

        for (int j = 0; j < 5; j++) {
            gl.glRotatef(mAngle, 0, 1, 0);
            gl.glRotatef(mAngle, 0, 0, -1);
            gl.glRotatef(mAngle, -1, 0, 0);
        }
        // gl.glScalef(1.2f, 1.2f, 1f);
        gl.glDrawElements(GL10.GL_TRIANGLES, indices.length, GL10.GL_UNSIGNED_SHORT, mIndexBuffer);
        gl.glDisableClientState(GL10.GL_VERTEX_ARRAY);
        gl.glDisable(GL10.GL_CULL_FACE);
        mAngle++;
    }

    @Override
    public void onSurfaceChanged(GL10 gl, int width, int height) {
        // Sets the current view port to the new size.
        gl.glViewport(0, 0, width, height);
        // Select the projection matrix
        gl.glMatrixMode(GL10.GL_PROJECTION);
        // Reset the projection matrix
        gl.glLoadIdentity();
        // Calculate the aspect ratio of the window
        GLU.gluPerspective(gl, 45.0f, (float) width / (float) height, 0.1f, 100.0f);
        // Select the modelview matrix
        gl.glMatrixMode(GL10.GL_MODELVIEW);
        // Reset the modelview matrix
        gl.glLoadIdentity();
    }

}
