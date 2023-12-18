package com.softwinner.agingdragonbox.engine.testcase;

import java.nio.ByteBuffer;
import java.nio.ByteOrder;
import java.nio.FloatBuffer;
import java.nio.IntBuffer;

import javax.microedition.khronos.opengles.GL10;

public class Celestial {

    private static final float         UNIT_SIZE = 20.0f;
    private FloatBuffer mVertexBuffer;
    private IntBuffer   mColorBuffer;
    int                 mVCount    = 0;
    float               mYAngle;
    int                 mXOffset;
    int                 mZOffset;
    float               mScale;

    public Celestial(int xOffset, int zOffset, float scale, float yAngle, int vCount) {
        mXOffset = xOffset;
        mZOffset = zOffset;
        mYAngle = yAngle;
        mScale = scale;
        mVCount = vCount;

        float[] vertices = new float[vCount * 3];
        for (int i = 0; i < vCount; i++) {
            double angleTempJD = Math.PI * 2 * Math.random();
            double angleTempWD = Math.PI / 2 * Math.random();
            vertices[i * 3] = (float) (UNIT_SIZE * Math.cos(angleTempWD) * Math.sin(angleTempJD));
            vertices[i * 3 + 1] = (float) (UNIT_SIZE * Math.sin(angleTempWD));
            vertices[i * 3 + 2] = (float) (UNIT_SIZE * Math.cos(angleTempWD) * Math
                    .cos(angleTempJD));
        }

        ByteBuffer vbb = ByteBuffer.allocateDirect(vertices.length * 4);
        vbb.order(ByteOrder.nativeOrder());
        mVertexBuffer = vbb.asFloatBuffer();
        mVertexBuffer.put(vertices);
        mVertexBuffer.position(0);

        final int one = 65535;
        int[] colors = new int[vCount * 4];
        for (int i = 0; i < vCount; i++) {
            colors[i * 4] = one;
            colors[i * 4 + 1] = one;
            colors[i * 4 + 2] = one;
            colors[i * 4 + 3] = 0;
        }

        ByteBuffer cbb = ByteBuffer.allocateDirect(colors.length * 4);
        cbb.order(ByteOrder.nativeOrder());
        mColorBuffer = cbb.asIntBuffer();
        mColorBuffer.put(colors);
        mColorBuffer.position(0);
    }

    public void drawSelf(GL10 gl) {
        gl.glEnableClientState(GL10.GL_VERTEX_ARRAY);
        gl.glEnableClientState(GL10.GL_COLOR_ARRAY);

        gl.glDisable(GL10.GL_LIGHTING);
        gl.glPointSize(mScale);
        gl.glPushMatrix();
        gl.glTranslatef(mXOffset * UNIT_SIZE, 0, 0);
        gl.glTranslatef(0, 0, mZOffset * UNIT_SIZE);
        gl.glRotatef(mYAngle, 0, 1, 0);

        gl.glVertexPointer(3, GL10.GL_FLOAT, 0, mVertexBuffer);

        gl.glColorPointer(4, GL10.GL_FIXED, 0, mColorBuffer);

        gl.glDrawArrays(GL10.GL_POINTS, 0, mVCount);

        gl.glPopMatrix();
        gl.glPointSize(1);
        gl.glEnable(GL10.GL_LIGHTING);
    }
}
