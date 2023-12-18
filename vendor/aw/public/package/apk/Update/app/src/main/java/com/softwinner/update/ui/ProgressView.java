package com.softwinner.update.ui;

import android.content.Context;
import android.content.res.TypedArray;
import android.graphics.Canvas;
import android.graphics.Paint;
import android.graphics.Paint.Style;
import android.util.AttributeSet;
import android.view.View;

import com.softwinner.update.R;

public class ProgressView extends View {

    private Paint textPaint, numPaint, progressPaint, backPaint;
    private float h, w;
    private int textSize = 20;
    private String str1 = "", str2 = "", str3 = "";
    private Context mContext;

    public ProgressView(Context context, AttributeSet attrs) {
        super(context, attrs);
        mContext = context;
        parseAttributes(context.obtainStyledAttributes(attrs,
                R.styleable.ProgressView));

        textPaint = new Paint();
        textPaint.setTextSize(textSize);
        textPaint.setAntiAlias(true);
        textPaint.setColor(0xff3079d8);
        textPaint.setTextAlign(Paint.Align.CENTER);

        numPaint = new Paint();
        numPaint.setTextSize(textSize);
        numPaint.setAntiAlias(true);
        numPaint.setColor(0xffffffff);
        numPaint.setTextAlign(Paint.Align.CENTER);

        progressPaint = new Paint();
        progressPaint.setStyle(Style.FILL);
        progressPaint.setAntiAlias(true);
        progressPaint.setColor(0xff3079d8);

        backPaint = new Paint();
        backPaint.setStyle(Style.FILL);
        backPaint.setAntiAlias(true);
        backPaint.setColor(0xffa8a8a8);

    }

    @Override
    protected void onDraw(Canvas canvas) {
        super.onDraw(canvas);
        w = this.getWidth();
        float x, y, textY;
        x = w / 6;
        y = mContext.getResources().getDimensionPixelSize(R.dimen.progress_circle_radius);
        textY = mContext.getResources().getDimensionPixelSize(R.dimen.progress_text_y_paddingtop);
        android.util.Log.i("tag", ">>>>>> " + y + " " + textY);
        canvas.drawCircle(x, y, y, progressPaint);
        canvas.drawCircle(3 * x, y, y, backPaint);
        canvas.drawCircle(5 * x, y, y, backPaint);

        canvas.drawCircle(9, y, 9, progressPaint);
        canvas.drawCircle(w - 9, y, 9, backPaint);

        canvas.drawRect(9, y - 9, 2 * x, y + 9, progressPaint);
        canvas.drawRect(2 * x, y - 9, w - 9, y + 9, backPaint);

        numPaint.setTextSize(y * 3 / 2);
        canvas.drawText("1", x, 6 * y / 4, numPaint);
        canvas.drawText("2", 3 * x, 6 * y / 4, numPaint);
        canvas.drawText("3", 5 * x, 6 * y / 4, numPaint);

        canvas.drawText(str1, x, textY, textPaint);
        canvas.drawText(str2, 3 * x, textY, textPaint);
        canvas.drawText(str3, 5 * x, textY, textPaint);
    }

    private void parseAttributes(TypedArray a) {
        textSize = (int) a.getDimension(R.styleable.ProgressView_textsize,
                textSize);

        if (a.hasValue(R.styleable.ProgressView_text1)) {
            str1 = (String) a.getString(R.styleable.ProgressView_text1);
            str2 = (String) a.getString(R.styleable.ProgressView_text2);
            str3 = (String) a.getString(R.styleable.ProgressView_text3);
        }

        // Recycle
        a.recycle();
    }
}
