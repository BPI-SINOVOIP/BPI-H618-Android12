package com.softwinner.tvdsetting.widget;

import android.content.Context;
import android.graphics.Canvas;
import android.graphics.Paint;
import android.graphics.Paint.Align;
import android.graphics.RectF;
import android.util.AttributeSet;
import android.view.View;

public class CycleView extends View {

	Paint mPaint;
	int _color = -1;
	int mWidth,mHeight;
    int _r;
    int _padding;

	public CycleView(Context context, AttributeSet attrs) {
		super(context, attrs);
		// TODO Auto-generated constructor stub
		mPaint = new Paint();
		mPaint.setColor(_color);
		mPaint.setStyle(Paint.Style.STROKE);
		mPaint.setStrokeWidth(10);
		mPaint.setAntiAlias(true);
		mPaint.setTextAlign(Align.CENTER);
		mPaint.setTextSize(100);
		_padding = 40;
	}
	
	@Override
	public void onDraw(Canvas cvs){
		mWidth = this.getWidth();
		mHeight = this.getHeight();
		_r = mWidth/2 - _padding;
		RectF oval = new RectF(_padding,_padding,mWidth-_padding,mHeight-_padding);
		cvs.drawArc(oval, -240,  300, false, mPaint);
		cvs.drawText("97%", mWidth/2, mHeight/2 + 100/2 , mPaint);
		mPaint.setTextSize(60);
		//cvs.drawText("+", (float)(_r - _r*Math.cos(60*Math.PI/180)), (float)(_r + _r*Math.sin(60*Math.PI/180)), mPaint);
		//cvs.drawText("-", (float)(_r + _r*Math.cos(60*Math.PI/180)), (float)(_r + _r*Math.sin(60*Math.PI/180)), mPaint);
		cvs.drawText("+", (float)(_r - _r*Math.cos(60*Math.PI/180)) + 30, (float)(_r + _r*Math.sin(60*Math.PI/180)) + 100, mPaint);
		cvs.drawText("-", (float)(_r + _r*Math.cos(60*Math.PI/180)) + 30, (float)(_r + _r*Math.sin(60*Math.PI/180)) + 100, mPaint);
	}

}
