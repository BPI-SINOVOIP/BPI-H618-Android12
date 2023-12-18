package com.softwinner.view;

import android.content.Context;
import android.graphics.Canvas;
import android.graphics.Color;
import android.graphics.Paint;
import android.graphics.Rect;
import android.util.AttributeSet;
import android.widget.Button;

public class MyButton extends Button {
    private int height = 6;
    private static final String TAG = "MyButton";

    public MyButton(Context context) {
        super(context);
    }

    public MyButton(Context context, AttributeSet attrs) {
        super(context, attrs);
    }

    public MyButton(Context context, AttributeSet attrs, int defStyle) {
        super(context, attrs, defStyle);
    }

    @Override
    protected void onDraw(Canvas canvas) {
        super.onDraw(canvas);
        if (this.isSelected()) {
            Paint mPaint = new Paint();
            mPaint.setColor(0xFF33B5E5);
            Rect rect = new Rect(0, this.getHeight() - height, this.getWidth(), this.getHeight());
            canvas.drawRect(rect, mPaint);
            this.setTextColor(0xFF33B5E5);
        } else {
            this.setTextColor(Color.WHITE);
        }
    }
}
