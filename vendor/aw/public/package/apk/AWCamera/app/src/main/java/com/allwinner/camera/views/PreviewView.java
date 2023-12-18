package com.allwinner.camera.views;

import android.content.Context;
import android.graphics.Bitmap;
import android.util.AttributeSet;
import android.util.Log;
import android.view.TextureView;


/**
 * A {@link TextureView} that can be adjusted to a specified aspect ratio.
 */
public class PreviewView extends TextureView {

    private int mRatioWidth = 0;
    private int mRatioHeight = 0;
    private final Object mSyncObject = new Object();
    private final static String TAG = "MzTextureView";
    private Bitmap mBitmap = null;
    private int mFactor = 4;
    private int[] mRGBData = new int[3000 * 2000];
    private int[] mRGBsize = new int[2];
    int mWidth = 0;
    int mHeight = 0;
    int mDisplayWidth = 0;
    int mDisplayHeight = 0;

    public PreviewView(Context context) {
        this(context, null);
    }

    public PreviewView(Context context, AttributeSet attrs) {
        this(context, attrs, 0);
    }

    public PreviewView(Context context, AttributeSet attrs, int defStyle) {
        super(context, attrs, defStyle);
    }

    @Override
    public void layout(int l, int t, int r, int b) {
        super.layout(l, t, r, b);
        int width = getWidth();
        int height = getHeight();
        Log.v(TAG, "layout:" + width + "x" + height);
        if (getSurfaceTexture() != null) {
            if (mRatioHeight <=0 || mRatioWidth <= 0) {
                Log.v(TAG, "layout:size zero");
            } else {
                getSurfaceTexture().setDefaultBufferSize(mRatioHeight, mRatioWidth);
            }

        }
//        if (mSurfaceTexture != null) {
//            mSurfaceTexture.setDefaultBufferSize(mViewWidth, mViewHeight);
//            if (mListener != null) {
//                mListener.onSurfaceTextureSizeChanged(mSurfaceTexture, mViewWidth, mViewHeight);
//            }
//        }
        invalidate();
    }


    public void setLayoutSize(int width, int height) {
        Log.i(TAG, "setLayoutSize: " + width + "x" + height);
        mWidth = width;
        mHeight = height;
    }


    public void setPreviewSize(int width, int height) {
        Log.i(TAG, "setPreviewSize: " + width + "x" + height);
        if (width < 0 || height < 0) {
            throw new IllegalArgumentException("Size cannot be negative.");
        }
        mRatioWidth = width;
        mRatioHeight = height;
    }

    @Override
    protected void onMeasure(int widthMeasureSpec, int heightMeasureSpec) {
        super.onMeasure(widthMeasureSpec, heightMeasureSpec);
        int width = MeasureSpec.getSize(widthMeasureSpec);
        int height = MeasureSpec.getSize(heightMeasureSpec);
        Log.i(TAG, "onMeasure: " + width + "x" + height);
        if (0 == mRatioWidth || 0 == mRatioHeight) {
            setMeasuredDimension(width, height);
        } else {
            if (width < height * mRatioWidth / mRatioHeight) {
                setMeasuredDimension(width, width * mRatioHeight / mRatioWidth);
            } else {
                setMeasuredDimension(height * mRatioWidth / mRatioHeight, height);
            }
        }
    }

}
