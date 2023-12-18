package com.allwinnertech.socs.gpu;

public class Layer {
    private int mAxis;
    public Cube[] cubes = new Cube[9];
    public static final int sAxisX = 0;
    public static final int sAxisY = 1;
    public static final int sAxisZ = 2;

    public Layer(int axis) {
        mAxis = axis;
    }

    public int getmAxis() {
        return mAxis;
    }

    public Cube[] getCubes() {
        return cubes;
    }
}
