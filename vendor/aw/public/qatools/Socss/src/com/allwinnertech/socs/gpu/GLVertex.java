package com.allwinnertech.socs.gpu;

public class GLVertex {
    private float vertexCoords[] = new float[3];

    public float[] getVertexCoords() {
        return vertexCoords;
    }

    public GLVertex(float x, float y, float z) {
        vertexCoords[0] = x;
        vertexCoords[1] = y;
        vertexCoords[2] = z;
    }
}
