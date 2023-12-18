package com.allwinner.camera.views;

import android.content.Context;
import android.graphics.Bitmap;
import android.graphics.Canvas;
import android.graphics.Color;
import android.graphics.ImageFormat;
import android.graphics.Matrix;
import android.graphics.Paint;
import android.graphics.PorterDuff;
import android.graphics.PorterDuffXfermode;
import android.util.AttributeSet;
import android.util.Log;
import android.view.View;
import android.widget.ImageView;

import com.allwinner.camera.data.CameraData;

public class PanoPreviewView extends View {

    private Bitmap mShowBitmap = null;
    private Object mSync = new Object();
    private Matrix mMatrix = new Matrix();
    private Paint mPaint = null;
    private int mTranY = 0;
    private int mShowHeight = 0;

    public PanoPreviewView(Context context, AttributeSet attrs, int defStyleAttr) {
        super(context, attrs, defStyleAttr);
    }

    public PanoPreviewView(Context context) {
        super(context);
    }

    public PanoPreviewView(Context context, AttributeSet attrs) {
        super(context, attrs);
    }

    public void setBitmap(Bitmap bitmap){
        synchronized (mSync) {
            if (mShowBitmap != null && !mShowBitmap.isRecycled()) {
                mShowBitmap.recycle();
            }
            mShowBitmap = bitmap;
        }
        invalidate();
    }

    public void intMatrix(){
        mShowHeight = CameraData.getInstance().getScreenHeight() /6 ;
        float previewHeight = CameraData.getInstance().getPreviewWidth();
        float scale =  mShowHeight/previewHeight;
        mTranY =(int)( CameraData.getInstance().getScreenHeight() * 3 /8 - mShowHeight /2);
        mMatrix.postScale(scale,scale);
        mPaint = new Paint(Paint.ANTI_ALIAS_FLAG);
        mPaint.setFilterBitmap(true);
        mPaint.setDither(true);
    }

    public int getTranY() { return mTranY; }

    public int getShowHeight() { return mShowHeight; }

    @Override
    protected void onMeasure(int widthMeasureSpec, int heightMeasureSpec) {
        super.onMeasure(widthMeasureSpec, heightMeasureSpec);
    }

    @Override
    protected void onDraw(Canvas canvas) {
        super.onDraw(canvas);
        synchronized (mSync) {
            try {
                if (mShowBitmap != null) {
                    canvas.save();
                    Paint paint = new Paint();
                    paint.setXfermode(new PorterDuffXfermode(PorterDuff.Mode.CLEAR));
                    canvas.drawPaint(paint);
                    paint.setXfermode(new PorterDuffXfermode(PorterDuff.Mode.SRC));
                    canvas.translate(0,mTranY );
                    canvas.drawBitmap(mShowBitmap,mMatrix,paint);
                    canvas.restore();
                }
            }catch (Exception e) {
                e.printStackTrace();
            }

        }





    }
}
