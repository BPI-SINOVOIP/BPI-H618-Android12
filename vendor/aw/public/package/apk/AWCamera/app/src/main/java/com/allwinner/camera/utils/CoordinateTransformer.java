package com.allwinner.camera.utils;

import android.graphics.Matrix;
import android.graphics.Rect;
import android.graphics.RectF;
import android.hardware.camera2.CameraCharacteristics;
import android.util.Log;

import com.allwinner.camera.data.CameraData;

/**
 * Transform coordinates to and from preview coordinate space and camera driver
 * coordinate space.
 */
public class CoordinateTransformer {
    private final static String TAG ="CoordinateTransformer";
    private final Matrix mPreviewToCameraTransform;
    private RectF mDriverRectF;

    /**
     * Convert rectangles to / from camera coordinate and preview coordinate space.
     * @param chr camera characteristics
     * @param previewRect the preview rectangle size and position.
     */
    public CoordinateTransformer(CameraCharacteristics chr, RectF previewRect) {
        if (!hasNonZeroArea(previewRect)) {
            throw new IllegalArgumentException("previewRect");
        }
        Rect rect = chr.get(CameraCharacteristics.SENSOR_INFO_ACTIVE_ARRAY_SIZE);
        Integer sensorOrientation = chr.get(CameraCharacteristics.SENSOR_ORIENTATION);
        int rotation = sensorOrientation == null ? 90 : CameraUtils.getPreviewRotation(CameraData.getInstance().getCameraId());
        mDriverRectF = new RectF(rect);
        boolean mirrorX ;
        if(CameraData.getInstance().getBackCameraId()==CameraData.getInstance().getCameraId() ){
            Log.e(TAG,"back camera");
            mirrorX = false;
        }else{
            Log.e(TAG,"front camera");
            mirrorX = true;
        }
        mPreviewToCameraTransform = previewToCameraTransform(mirrorX, rotation, previewRect);
    }

    /**
     * Transform a rectangle in preview view space into a new rectangle in
     * camera view space.
     * @param source the rectangle in preview view space
     * @return the rectangle in camera view space.
     */
    public RectF toCameraSpace(RectF source) {
        RectF result = new RectF();
        mPreviewToCameraTransform.mapRect(result, source);
        return result;
    }

    private Matrix previewToCameraTransform(boolean mirrorX, int sensorOrientation,
          RectF previewRect) {
        Matrix transform = new Matrix();
        // Need mirror for front camera.
        transform.setScale(mirrorX ? -1 : 1, 1);
        // Because preview orientation is different  form sensor orientation,
        // rotate to same orientation, Counterclockwise.
        transform.postRotate(-sensorOrientation);
        // Map rotated matrix to preview rect
        transform.mapRect(previewRect);
        // Map  preview coordinates to driver coordinates
        Matrix fill = new Matrix();
        fill.setRectToRect(previewRect, mDriverRectF, Matrix.ScaleToFit.FILL);
        // Concat the previous transform on top of the fill behavior.
        transform.setConcat(fill, transform);
        // finally get transform matrix
        return transform;
    }

    private boolean hasNonZeroArea(RectF rect) {
        return rect.width() != 0 && rect.height() != 0;
    }
}
