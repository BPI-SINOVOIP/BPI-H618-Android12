package com.allwinner.camera.views;

import android.content.Context;
import android.content.SharedPreferences;
import android.content.res.TypedArray;
import android.graphics.Canvas;
import android.graphics.Color;
import android.graphics.Paint;
import android.preference.PreferenceManager;
import android.util.AttributeSet;
import android.util.Log;
import android.view.View;

import com.allwinner.camera.R;

public class GridlineView extends View {
    private Paint mPaint=null;
    private int width;
    private int height;
    public GridlineView(Context context) {
        this(context,null);
        init();

    }

    public GridlineView(Context context, AttributeSet attrs) {
        this(context, attrs,0);
        init();
    }

    public GridlineView(Context context, AttributeSet attrs, int defStyleAttr) {
        super(context, attrs, defStyleAttr);
        init();
    }




    private void init() {//关于paint类
        mPaint=new Paint();
        mPaint.setColor(Color.GRAY);
        mPaint.setStyle(Paint.Style.STROKE);//设置画笔的类型是填充，还是描边，还是描边且填充
        mPaint.setStrokeWidth(1);//设置笔刷的粗细
    }
    @Override
    protected void onMeasure(int widthMeasureSpec, int heightMeasureSpec) {
        super.onMeasure(widthMeasureSpec, heightMeasureSpec);
        width= MeasureSpec.getSize(widthMeasureSpec);
        height= MeasureSpec.getSize(heightMeasureSpec);
    }

    @Override
    protected void onDraw(Canvas canvas) {
        super.onDraw(canvas);
        int x = width/3;
        int y = height/3;
        canvas.drawLine(x, 0, x, height, mPaint);//绘制直线的起始(x,y)与终止(x1,y1)与画笔。
        canvas.drawLine(2*x, 0, 2*x, height, mPaint);
        canvas.drawLine(0, y, width, y, mPaint);
        canvas.drawLine(0, 2*y, width, 2*y, mPaint);


    }


}
