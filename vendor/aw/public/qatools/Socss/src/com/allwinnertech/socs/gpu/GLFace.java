package com.allwinnertech.socs.gpu;

import android.content.Context;
import android.opengl.GLES20;

import java.nio.ByteBuffer;
import java.nio.ByteOrder;
import java.nio.FloatBuffer;
import java.nio.ShortBuffer;
import java.util.Arrays;

public class GLFace {
    private Context mContext;
    private FloatBuffer vertexBuffer;
    private ShortBuffer drawListBuffer;
    private float faceCoords[] = new float[12];
    private float colors[];
    private short drawOrder[] = {0, 1, 2, 0, 2, 3};
    private final int program;
    private int positionHandle;
    private int colorHandle;
    private int vPMatrixHandle;
    static final int COORDS_PER_VERTEX = 3;
    private final int vertexStride = COORDS_PER_VERTEX * 4;

    public GLFace(Context context, GLVertex v1, GLVertex v2, GLVertex v3, GLVertex v4, float[] colors) {
        mContext = context;
        for (int i = 0; i < 3; i++) {
            faceCoords[i] = v1.getVertexCoords()[i];
        }
        for (int i = 3; i < 6; i++) {
            faceCoords[i] = v2.getVertexCoords()[i - 3];
        }
        for (int i = 6; i < 9; i++) {
            faceCoords[i] = v3.getVertexCoords()[i - 6];
        }
        for (int i = 9; i < 12; i++) {
            faceCoords[i] = v4.getVertexCoords()[i - 9];
        }
        this.colors = Arrays.copyOf(colors, colors.length);
        ByteBuffer bb = ByteBuffer.allocateDirect(faceCoords.length * 4);
        bb.order(ByteOrder.nativeOrder());
        vertexBuffer = bb.asFloatBuffer();
        vertexBuffer.put(faceCoords);
        vertexBuffer.position(0);

        ByteBuffer dlb = ByteBuffer.allocateDirect(drawOrder.length * 2);
        dlb.order(ByteOrder.nativeOrder());
        drawListBuffer = dlb.asShortBuffer();
        drawListBuffer.put(drawOrder);
        drawListBuffer.position(0);

        String vertexSource = ShaderUtil.loadFromAssetsFile("face_vertex.sh", mContext);
        String fragmentSource = ShaderUtil.loadFromAssetsFile("face_fragment.sh", mContext);
        program = ShaderUtil.createProgram(vertexSource, fragmentSource);
    }

    public void draw(float[] mvpMatrix) {
        GLES20.glUseProgram(program);
        positionHandle = GLES20.glGetAttribLocation(program, "vPosition");
        GLES20.glEnableVertexAttribArray(positionHandle);
        GLES20.glVertexAttribPointer(positionHandle, COORDS_PER_VERTEX, GLES20.GL_FLOAT,
                false, vertexStride, vertexBuffer);
        colorHandle = GLES20.glGetUniformLocation(program, "vColor");
        GLES20.glUniform4fv(colorHandle, 1, colors, 0);
        vPMatrixHandle = GLES20.glGetUniformLocation(program, "uMVPMatrix");
        GLES20.glUniformMatrix4fv(vPMatrixHandle, 1, false, mvpMatrix, 0);
        GLES20.glDrawElements(GLES20.GL_TRIANGLES, drawOrder.length,
                GLES20.GL_UNSIGNED_SHORT, drawListBuffer);
        GLES20.glDisableVertexAttribArray(positionHandle);
    }
}
