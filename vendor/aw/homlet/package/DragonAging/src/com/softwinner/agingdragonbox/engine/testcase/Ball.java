package com.softwinner.agingdragonbox.engine.testcase;

import java.nio.ByteBuffer;
import java.nio.ByteOrder;
import java.nio.FloatBuffer;
import java.nio.IntBuffer;
import java.util.ArrayList;

import javax.microedition.khronos.opengles.GL10;

public class Ball {
    private static final int UNIT_SIZE = 10000;
    private IntBuffer   mVertexBuffer;
    private IntBuffer   mNormalBuffer;
    private FloatBuffer mTextureBuffer;
    public float        mAngleX;
    public float        mAngleY;
    public float        mAngleZ;
    int                 vCount = 0;
    int                 mTextId;
    

    public Ball(int scale, int texId) {
        this.mTextId = texId;
        final float angleSpan = 3.0f;

        float[] texCoorArray = generateTexCoor((int) (360 / angleSpan), (int) (180 / angleSpan));
        int tc = 0;
        int ts = texCoorArray.length;

        ArrayList<Integer> alVertix = new ArrayList<Integer>();
        ArrayList<Float> alTexture = new ArrayList<Float>();

        for (float vAngle = 90; vAngle > -90; vAngle = vAngle - angleSpan) {
            for (float hAngle = 360; hAngle > 0; hAngle = hAngle - angleSpan) {

                double xozLength = scale * UNIT_SIZE * Math.cos(Math.toRadians(vAngle));
                int x1 = (int) (xozLength * Math.cos(Math.toRadians(hAngle)));
                int z1 = (int) (xozLength * Math.sin(Math.toRadians(hAngle)));
                int y1 = (int) (scale * UNIT_SIZE * Math.sin(Math.toRadians(vAngle)));

                xozLength = scale * UNIT_SIZE * Math.cos(Math.toRadians(vAngle - angleSpan));
                int x2 = (int) (xozLength * Math.cos(Math.toRadians(hAngle)));
                int z2 = (int) (xozLength * Math.sin(Math.toRadians(hAngle)));
                int y2 = (int) (scale * UNIT_SIZE * Math.sin(Math.toRadians(vAngle - angleSpan)));

                xozLength = scale * UNIT_SIZE * Math.cos(Math.toRadians(vAngle - angleSpan));
                int x3 = (int) (xozLength * Math.cos(Math.toRadians(hAngle - angleSpan)));
                int z3 = (int) (xozLength * Math.sin(Math.toRadians(hAngle - angleSpan)));
                int y3 = (int) (scale * UNIT_SIZE * Math.sin(Math.toRadians(vAngle - angleSpan)));

                xozLength = scale * UNIT_SIZE * Math.cos(Math.toRadians(vAngle));
                int x4 = (int) (xozLength * Math.cos(Math.toRadians(hAngle - angleSpan)));
                int z4 = (int) (xozLength * Math.sin(Math.toRadians(hAngle - angleSpan)));
                int y4 = (int) (scale * UNIT_SIZE * Math.sin(Math.toRadians(vAngle)));

                alVertix.add(x1);
                alVertix.add(y1);
                alVertix.add(z1);
                alVertix.add(x2);
                alVertix.add(y2);
                alVertix.add(z2);
                alVertix.add(x4);
                alVertix.add(y4);
                alVertix.add(z4);
                alVertix.add(x4);
                alVertix.add(y4);
                alVertix.add(z4);
                alVertix.add(x2);
                alVertix.add(y2);
                alVertix.add(z2);
                alVertix.add(x3);
                alVertix.add(y3);
                alVertix.add(z3);

                alTexture.add(texCoorArray[tc++ % ts]);
                alTexture.add(texCoorArray[tc++ % ts]);
                alTexture.add(texCoorArray[tc++ % ts]);
                alTexture.add(texCoorArray[tc++ % ts]);
                alTexture.add(texCoorArray[tc++ % ts]);
                alTexture.add(texCoorArray[tc++ % ts]);
                alTexture.add(texCoorArray[tc++ % ts]);
                alTexture.add(texCoorArray[tc++ % ts]);
                alTexture.add(texCoorArray[tc++ % ts]);
                alTexture.add(texCoorArray[tc++ % ts]);
                alTexture.add(texCoorArray[tc++ % ts]);
                alTexture.add(texCoorArray[tc++ % ts]);
            }
        }

        vCount = alVertix.size() / 3;

        int[] vertices = new int[vCount * 3];
        for (int i = 0; i < alVertix.size(); i++) {
            vertices[i] = alVertix.get(i);
        }

        ByteBuffer vbb = ByteBuffer.allocateDirect(vertices.length * 4);
        vbb.order(ByteOrder.nativeOrder());
        mVertexBuffer = vbb.asIntBuffer();
        mVertexBuffer.put(vertices);
        mVertexBuffer.position(0);

        ByteBuffer nbb = ByteBuffer.allocateDirect(vertices.length * 4);
        nbb.order(ByteOrder.nativeOrder());
        mNormalBuffer = vbb.asIntBuffer();
        mNormalBuffer.put(vertices);
        mNormalBuffer.position(0);

        float[] textureCoors = new float[alTexture.size()];
        for (int i = 0; i < alTexture.size(); i++) {
            textureCoors[i] = alTexture.get(i);
        }

        ByteBuffer tbb = ByteBuffer.allocateDirect(textureCoors.length * 4);
        tbb.order(ByteOrder.nativeOrder());
        mTextureBuffer = tbb.asFloatBuffer();
        mTextureBuffer.put(textureCoors);
        mTextureBuffer.position(0);
    }

    public void drawSelf(GL10 gl) {
        gl.glRotatef(mAngleZ, 0, 0, 1);
        gl.glRotatef(mAngleX, 1, 0, 0);
        gl.glRotatef(mAngleY, 0, 1, 0);

        gl.glEnableClientState(GL10.GL_VERTEX_ARRAY);
        gl.glVertexPointer(3, GL10.GL_FIXED, 0, mVertexBuffer);

        gl.glEnableClientState(GL10.GL_NORMAL_ARRAY);
        gl.glNormalPointer(GL10.GL_FIXED, 0, mNormalBuffer);

        gl.glEnable(GL10.GL_TEXTURE_2D);
        gl.glEnableClientState(GL10.GL_TEXTURE_COORD_ARRAY);
        gl.glTexCoordPointer(2, GL10.GL_FLOAT, 0, mTextureBuffer);
        gl.glBindTexture(GL10.GL_TEXTURE_2D, mTextId);

        gl.glDrawArrays(GL10.GL_TRIANGLES, 0, vCount);
    }

    public float[] generateTexCoor(int bw, int bh) {
        float[] result = new float[bw * bh * 6 * 2];
        float sizew = 1.0f / bw;
        float sizeh = 1.0f / bh;
        int c = 0;
        for (int i = 0; i < bh; i++) {
            for (int j = 0; j < bw; j++) {
                float s = j * sizew;
                float t = i * sizeh;

                result[c++] = s;
                result[c++] = t;

                result[c++] = s;
                result[c++] = t + sizeh;

                result[c++] = s + sizew;
                result[c++] = t;

                result[c++] = s + sizew;
                result[c++] = t;

                result[c++] = s;
                result[c++] = t + sizeh;

                result[c++] = s + sizew;
                result[c++] = t + sizeh;
            }
        }
        return result;
    }
}
