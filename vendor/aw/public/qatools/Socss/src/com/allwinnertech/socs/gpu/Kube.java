package com.allwinnertech.socs.gpu;

import android.content.Context;

import java.util.Arrays;
import java.util.Random;

public class Kube {
    private Cube[] mCubes = new Cube[27];
    public Layer[] layers = new Layer[9];
    public Layer currentLayer;
    int[] mCurrentLayerPermutation;
    int[] mPermutation;

    float c0 = -1.0f;
    float c1 = -0.38f;
    float c2 = -0.31f;
    float c3 = 0.31f;
    float c4 = 0.38f;
    float c5 = 1.0f;

    public Kube(Context context) {
        // top front, left to right
        mCubes[0] = new Cube(c0, c4, c4, c1, c5, c5);
        mCubes[1] = new Cube(c2, c4, c4, c3, c5, c5);
        mCubes[2] = new Cube(c4, c4, c4, c5, c5, c5);
        // top middle, left to right
        mCubes[3] = new Cube(c0, c4, c2, c1, c5, c3);
        mCubes[4] = new Cube(c2, c4, c2, c3, c5, c3);
        mCubes[5] = new Cube(c4, c4, c2, c5, c5, c3);
        // top back, left to right
        mCubes[6] = new Cube(c0, c4, c0, c1, c5, c1);
        mCubes[7] = new Cube(c2, c4, c0, c3, c5, c1);
        mCubes[8] = new Cube(c4, c4, c0, c5, c5, c1);
        // middle front, left to right
        mCubes[9] = new Cube(c0, c2, c4, c1, c3, c5);
        mCubes[10] = new Cube(c2, c2, c4, c3, c3, c5);
        mCubes[11] = new Cube(c4, c2, c4, c5, c3, c5);
        // middle middle, left to right
        mCubes[12] = new Cube(c0, c2, c2, c1, c3, c3);
        mCubes[13] = null;
        mCubes[14] = new Cube(c4, c2, c2, c5, c3, c3);
        // middle back, left to right
        mCubes[15] = new Cube(c0, c2, c0, c1, c3, c1);
        mCubes[16] = new Cube(c2, c2, c0, c3, c3, c1);
        mCubes[17] = new Cube(c4, c2, c0, c5, c3, c1);
        // bottom front, left to right
        mCubes[18] = new Cube(c0, c0, c4, c1, c1, c5);
        mCubes[19] = new Cube(c2, c0, c4, c3, c1, c5);
        mCubes[20] = new Cube(c4, c0, c4, c5, c1, c5);
        // bottom middle, left to right
        mCubes[21] = new Cube(c0, c0, c2, c1, c1, c3);
        mCubes[22] = new Cube(c2, c0, c2, c3, c1, c3);
        mCubes[23] = new Cube(c4, c0, c2, c5, c1, c3);
        // bottom back, left to right
        mCubes[24] = new Cube(c0, c0, c0, c1, c1, c1);
        mCubes[25] = new Cube(c2, c0, c0, c3, c1, c1);
        mCubes[26] = new Cube(c4, c0, c0, c5, c1, c1);
        layers[0] = new Layer(Layer.sAxisY);
        layers[1] = new Layer(Layer.sAxisY);
        layers[2] = new Layer(Layer.sAxisY);
        layers[3] = new Layer(Layer.sAxisZ);
        layers[4] = new Layer(Layer.sAxisZ);
        layers[5] = new Layer(Layer.sAxisZ);
        layers[6] = new Layer(Layer.sAxisX);
        layers[7] = new Layer(Layer.sAxisX);
        layers[8] = new Layer(Layer.sAxisX);
        for (int i = 0; i < 3; i++) {
            for (int j = 0; j < 3; j++) {
                layers[0].getCubes()[i * 3 + j] = mCubes[i * 3 + j];
                layers[1].getCubes()[i * 3 + j] = mCubes[i * 3 + j + 9];
                layers[2].getCubes()[i * 3 + j] = mCubes[i * 3 + j + 18];
                layers[3].getCubes()[i * 3 + j] = mCubes[i * 9 + j];
                layers[4].getCubes()[i * 3 + j] = mCubes[i * 9 + j + 3];
                layers[5].getCubes()[i * 3 + j] = mCubes[i * 9 + j + 6];
                layers[6].getCubes()[i * 3 + j] = mCubes[i * 9 + j * 3];
                layers[7].getCubes()[i * 3 + j] = mCubes[i * 9 + j * 3 + 1];
                layers[8].getCubes()[i * 3 + j] = mCubes[i * 9 + j * 3 + 2];
            }
        }
        mPermutation = new int[27];
        for (int i = 0; i < 27; i++) {
            mPermutation[i] = i;
            if (mCubes[i] != null) {
                mCubes[i].setData(context, this);
            }
        }
    }

    public void rotateLayer() {
        Random random = new Random(System.currentTimeMillis());
        int layerID = random.nextInt(9);
        currentLayer = layers[layerID];
        mCurrentLayerPermutation = mLayerPermutations[layerID];
    }

    static int[][] mLayerPermutations = {
            // layer[0]
            {6, 3, 0, 7, 4, 1, 8, 5, 2, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25, 26},
            // layer[1]
            {0, 1, 2, 3, 4, 5, 6, 7, 8, 15, 12, 9, 16, 13, 10, 17, 14, 11, 18, 19, 20, 21, 22, 23, 24, 25, 26},
            // layer[2]
            {0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 24, 21, 18, 25, 22, 19, 26, 23, 20},
            // layer[3]
            {2, 11, 20, 3, 4, 5, 6, 7, 8, 1, 10, 19, 12, 13, 14, 15, 16, 17, 0, 9, 18, 21, 22, 23, 24, 25, 26},
            // layer[4]
            {0, 1, 2, 5, 14, 23, 6, 7, 8, 9, 10, 11, 4, 13, 22, 15, 16, 17, 18, 19, 20, 3, 12, 21, 24, 25, 26},
            // layer[5]
            {0, 1, 2, 3, 4, 5, 8, 17, 26, 9, 10, 11, 12, 13, 14, 7, 16, 25, 18, 19, 20, 21, 22, 23, 6, 15, 24},
            // layer[6]
            {6, 1, 2, 15, 4, 5, 24, 7, 8, 3, 10, 11, 12, 13, 14, 21, 16, 17, 0, 19, 20, 9, 22, 23, 18, 25, 26},
            // layer[7]
            {0, 7, 2, 3, 16, 5, 6, 25, 8, 9, 4, 11, 12, 13, 14, 15, 22, 17, 18, 1, 20, 21, 10, 23, 24, 19, 26},
            // layer[8]
            {0, 1, 8, 3, 4, 17, 6, 7, 26, 9, 10, 5, 12, 13, 14, 15, 16, 23, 18, 19, 2, 21, 22, 11, 24, 25, 20}
    };

    public void updateLayer() {
        int[] newPermutation = new int[27];
        for (int i = 0; i < 27; i++) {
            newPermutation[i] = mPermutation[mCurrentLayerPermutation[i]];
        }
        mPermutation = newPermutation;
        for (int i = 0; i < 3; i++) {
            for (int j = 0; j < 3; j++) {
                layers[0].getCubes()[i * 3 + j] = mCubes[mPermutation[i * 3 + j]];
                layers[1].getCubes()[i * 3 + j] = mCubes[mPermutation[i * 3 + j + 9]];
                layers[2].getCubes()[i * 3 + j] = mCubes[mPermutation[i * 3 + j + 18]];
                layers[3].getCubes()[i * 3 + j] = mCubes[mPermutation[i * 9 + j]];
                layers[4].getCubes()[i * 3 + j] = mCubes[mPermutation[i * 9 + j + 3]];
                layers[5].getCubes()[i * 3 + j] = mCubes[mPermutation[i * 9 + j + 6]];
                layers[6].getCubes()[i * 3 + j] = mCubes[mPermutation[i * 9 + j * 3]];
                layers[7].getCubes()[i * 3 + j] = mCubes[mPermutation[i * 9 + j * 3 + 1]];
                layers[8].getCubes()[i * 3 + j] = mCubes[mPermutation[i * 9 + j * 3 + 2]];
            }
        }
    }

    public void draw(float[] modelMatrix, float[] vpLayerMatrix, float[] vpMatrix) {
        for (Cube cube : mCubes) {
            if (cube != null) {
                if (Arrays.asList(currentLayer.getCubes()).contains(cube)) {
                    cube.draw(modelMatrix, vpLayerMatrix);
                } else {
                    cube.draw(modelMatrix, vpMatrix);
                }
            }
        }
    }

    public void setModelMatrix(float[] angleMatrix) {
        for (Cube cube : mCubes) {
            if (cube != null) {
                if (Arrays.asList(currentLayer.getCubes()).contains(cube)) {
                    cube.setModelMatrix(angleMatrix);
                }
            }
        }
    }
}
