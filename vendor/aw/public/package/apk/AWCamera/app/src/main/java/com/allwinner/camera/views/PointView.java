package com.allwinner.camera.views;

import android.content.Context;
import android.graphics.Canvas;
import android.graphics.Color;
import android.graphics.Paint;
import android.support.annotation.Nullable;
import android.util.AttributeSet;
import android.view.View;

import com.alibaba.android.mnnkit.entity.FaceDetectionReport;
import com.allwinner.camera.data.Contants;

public class PointView extends View {
    int mViewWidth;
    int mViewHeight;
    FaceDetectionReport[] mFaceResult;
    int mCameraOrientation;
    Paint mKeyPointsPaint = new Paint();
    Paint mPointOrderPaint = new Paint();
    private boolean mShowFacepointIndex ;

    public PointView(Context context) {
        super(context);
    }

    public PointView(Context context, @Nullable AttributeSet attrs) {
        super(context, attrs);
    }

    public PointView(Context context, @Nullable AttributeSet attrs, int defStyleAttr) {
        super(context, attrs, defStyleAttr);
    }

    public PointView(Context context, @Nullable AttributeSet attrs, int defStyleAttr, int defStyleRes) {
        super(context, attrs, defStyleAttr, defStyleRes);
    }

    public void init(){
        mKeyPointsPaint.setColor((Color.WHITE));
        mKeyPointsPaint.setStyle(Paint.Style.FILL);
        mKeyPointsPaint.setStrokeWidth(2);

        mPointOrderPaint.setColor(Color.GREEN);
        mPointOrderPaint.setStyle(Paint.Style.STROKE);
        mPointOrderPaint.setStrokeWidth(2f);
        mPointOrderPaint.setTextSize(18);
    }

    @Override
    protected void onMeasure(int widthMeasureSpec, int heightMeasureSpec) {
        super.onMeasure(widthMeasureSpec, heightMeasureSpec);
    }



    @Override
    protected void onDraw(Canvas canvas) {

        canvas.save();
        if (mFaceResult!= null && mFaceResult.length > 0){
            drawResult(mFaceResult[0].keyPoints, 1,mCameraOrientation,canvas,mShowFacepointIndex);
        }
        super.onDraw(canvas);
        canvas.restore();
    }
    public void setFaceResult(FaceDetectionReport[] result, int width, int height, int cameraOrientation, boolean showFacepointIndex) {
        mViewWidth = width;
        mViewHeight = height;
        mFaceResult = result;
        mCameraOrientation = cameraOrientation;
        mShowFacepointIndex = showFacepointIndex;
        invalidate();
    }
    public void drawResult(float[] facePoints,
                           int faceCount, int cameraOrientation, Canvas canvas, boolean showFacepointIndex) {
        try {
            if (canvas == null) {
                return;
            }

            float kx = 0.0f, ky = 0.0f;

            // 这里只写了摄像头正向为90/270度的一般情况
            /*if(90 == cameraOrientation || 270 == cameraOrientation){
                kx = ((float) mViewWidth)/ Contants.FACE_DATA_HEIGHT;
                ky = (float) mViewHeight/Contants.FACE_DATA_WIGHT;
            } else {
                kx = ((float) mViewWidth)/ Contants.FACE_DATA_WIGHT;
                ky = (float) mViewHeight/Contants.FACE_DATA_HEIGHT;
            }*/
            kx = ((float) mViewWidth)/(float) Contants.FACE_DATA_WIGHT;
            ky = ( (float)mViewHeight)/(float)Contants.FACE_DATA_HEIGHT;
            // 绘制人脸关键点
            for (int i=0; i<faceCount; i++) {
                for (int j=0; j<106; j++) {
                    float keyX = facePoints[i*106*2 + j*2];
                    float keyY = facePoints[i*106*2 + j*2 + 1];
                    canvas.drawCircle(keyX * kx, keyY * ky, 4.0f, mKeyPointsPaint);
                    if(showFacepointIndex) {
                        canvas.drawText(j + "", keyX * kx, keyY * ky, mPointOrderPaint); //标注106点的索引位置
                    }

                }
            }

        } catch (Throwable t) {
        } finally {

        }
    }
}
