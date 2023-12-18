package com.softwinner.shared;

import android.content.Context;
import android.graphics.Bitmap;
import android.graphics.Canvas;
import android.graphics.Paint;
import android.graphics.drawable.BitmapDrawable;
import android.os.Handler;
import android.os.Message;
import android.util.AttributeSet;
import android.view.View;

import com.softwinner.update.R;

public class CircleRotate extends View {

    private Paint paint;
    private boolean isSpinning = false;
    private float mRadius;
    private Bitmap mbitmap;
    private float x1, y1, x2, y2;

    private Handler spinHandler = new Handler() {
        /**
         * This is the code that will increment the progress variable and so
         * spin the wheel
         */
        @Override
        public void handleMessage(Message msg) {
            invalidate();
            if (isSpinning) {
                progress += 4;
                if (progress > 360) {
                    progress = 0;
                }
                spinHandler.sendEmptyMessageDelayed(0, 0);
            }
            // super.handleMessage(msg);
        }
    };
    int progress = 270;

    public CircleRotate(Context context) {
        super(context);

        paint = new Paint();
        paint.setAntiAlias(true);
        mbitmap = ((BitmapDrawable) getResources().getDrawable(
                R.drawable.ic_check_dot)).getBitmap();
    }

    public CircleRotate(Context context, AttributeSet attrs) {
        super(context, attrs);
    }

    protected void onDraw(Canvas canvas) {
        super.onDraw(canvas);
        x2 = (mRadius) * (float) Math.cos(progress * Math.PI / 180) + x1;
        y2 = (mRadius) * (float) Math.sin(progress * Math.PI / 180) + y1;
        canvas.drawBitmap(mbitmap, x2 - mbitmap.getWidth() / 2, y2 - mbitmap.getHeight() / 2, paint);
    }

    public void setPonit(float x, float y) {
        x1 = x;
        y1 = y;
    }

    public void setRadius(float radius) {
        this.mRadius = radius;
    }

    public void spin() {
        isSpinning = true;
        spinHandler.sendEmptyMessage(0);
    }

    public void stopSpinning() {
        isSpinning = false;
        progress = 0;
        spinHandler.removeMessages(0);
    }

    public boolean isSpinning() {
        if (isSpinning) {
            return true;
        } else {
            return false;
        }
    }
}
