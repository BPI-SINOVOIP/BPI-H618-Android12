package com.softwinner.tvdsetting.widget;

import com.softwinner.tvdsetting.R;

import android.content.Context;
import android.content.res.Resources;
import android.content.res.TypedArray;
import android.graphics.Bitmap;
import android.graphics.Canvas;
import android.graphics.Matrix;
import android.graphics.Paint;
import android.graphics.PixelFormat;
import android.graphics.Rect;
import android.graphics.drawable.BitmapDrawable;
import android.graphics.drawable.Drawable;
import android.util.AttributeSet;
import android.util.Log;
import android.view.View;

public class HomeView extends View {
	
	private static final String TAG = "HomeView";
	Paint mPaint;
	int mWidth,mHeight;

	BitmapDrawable mVline;
	BitmapDrawable mHline;
	BitmapDrawable mSelectNor;
	BitmapDrawable mSelectPress;
	BitmapDrawable mCurrentSelect;
	boolean mPress;
	
	int _borderWidth = 36;
	int _borderHeight = 39;
	int mSelectItem = 0;

	public HomeView(Context context, AttributeSet attrs) {
		super(context, attrs);
		// TODO Auto-generated constructor stub

		Resources resources=getResources(); 
        TypedArray a = context.obtainStyledAttributes(attrs,  
                R.styleable.HomeView);  
		mVline = (BitmapDrawable) resources.getDrawable(R.drawable.vline);
		mHline = (BitmapDrawable) resources.getDrawable(R.drawable.hline);
		mSelectPress = (BitmapDrawable) resources.getDrawable(R.drawable.ic_select_box_press);
		mSelectNor = (BitmapDrawable) resources.getDrawable(R.drawable.ic_select_box_nor);
		_borderWidth = (int) a.getDimension(R.styleable.HomeView_broderx, 36);
		_borderHeight = (int) a.getDimension(R.styleable.HomeView_brodery, 39);
		a.recycle();
		mCurrentSelect = mSelectNor;
	}
	
    private Bitmap drawableToBitmap(Drawable drawable){
         int width = drawable.getIntrinsicWidth();
         int height = drawable.getIntrinsicHeight();
         Bitmap.Config config = drawable.getOpacity() != PixelFormat.OPAQUE ? Bitmap.Config.ARGB_8888:Bitmap.Config.RGB_565;
         Bitmap bitmap = Bitmap.createBitmap(width, height, config);
         Canvas canvas = new Canvas(bitmap);
         drawable.setBounds(0, 0, width, height);
         drawable.draw(canvas);
         return bitmap;
    }	
    
	private BitmapDrawable zoomDrawable(Drawable drawable, int w, int h){
	         int width = drawable.getIntrinsicWidth();
	         int height= drawable.getIntrinsicHeight();
	         Bitmap oldbmp = drawableToBitmap(drawable);
	         Matrix matrix = new Matrix();
	         float scaleWidth = ((float)w / width);
	         float scaleHeight = ((float)h / height);
	         matrix.postScale(scaleWidth, scaleHeight);
	         Bitmap newbmp = Bitmap.createBitmap(oldbmp, 0, 0, width, height, matrix, true);
	         return new BitmapDrawable(newbmp);
	}	
	
	public void setSelectState(int item,boolean state){
		mSelectItem = item;
		mPress = state;
		if(mPress){
			mCurrentSelect = mSelectPress;
		}else{
			mCurrentSelect = mSelectNor;
		}
		this.invalidate();
	}
	
	public void setState(boolean state){
		mPress = state;
		if(mPress){
			mCurrentSelect = mSelectPress;
		}else{
			mCurrentSelect = mSelectNor;
		}
		this.invalidate();
	}
	
	@Override
	public void onDraw(Canvas cvs){
		mWidth = this.getWidth();
		mHeight = this.getHeight();
		cvs.drawBitmap(mVline.getBitmap(), mWidth/3 - mVline.getBitmap().getWidth()/2, mHeight/2 - mVline.getBitmap().getHeight()/2, mPaint);
		cvs.drawBitmap(mVline.getBitmap(), mWidth*2/3 - mVline.getBitmap().getWidth()/2, mHeight/2 - mVline.getBitmap().getHeight()/2, mPaint);
		cvs.drawBitmap(mHline.getBitmap(), mWidth/2 - mHline.getBitmap().getWidth()/2, mHeight/2-mHline.getBitmap().getHeight()/2, mPaint);
		Rect src = null,dest = null;
		if(mSelectItem== 0){ //the fuck ui give me the fuck resource,so i have to do this
			src = new Rect(_borderWidth,_borderHeight,mCurrentSelect.getBitmap().getWidth(),mCurrentSelect.getBitmap().getHeight());
			dest = new Rect(0,0,mWidth/3 + _borderWidth,mHeight/2 + _borderHeight);
		
		}else if(mSelectItem== 1){
			src = new Rect(0,_borderHeight,mCurrentSelect.getBitmap().getWidth(),mCurrentSelect.getBitmap().getHeight());
			dest = new Rect(mWidth/3-_borderWidth,0,mWidth*2/3 + _borderWidth,mHeight/2 + _borderHeight);
			
		}else if(mSelectItem == 2){
			src = new Rect(0,_borderHeight,mCurrentSelect.getBitmap().getWidth(),mCurrentSelect.getBitmap().getHeight());
			dest = new Rect(mWidth*2/3-_borderWidth,0,mWidth + _borderWidth,mHeight/2 + _borderHeight);
		}else if(mSelectItem == 3){
			src = new Rect(_borderWidth,0,mCurrentSelect.getBitmap().getWidth(),mCurrentSelect.getBitmap().getHeight() + _borderHeight);
			dest = new Rect(0,mHeight/2 - _borderHeight,mWidth/3 + _borderWidth,mHeight + _borderHeight);
		}else if(mSelectItem == 4){
			src = new Rect(0,0,mCurrentSelect.getBitmap().getWidth() + _borderWidth,mCurrentSelect.getBitmap().getHeight() + _borderHeight);
			dest = new Rect(mWidth/3 - _borderWidth ,mHeight/2 - _borderHeight,mWidth*2/3 + _borderWidth,mHeight + _borderHeight);
		}else if(mSelectItem == 5){
			src = new Rect(0,0,mCurrentSelect.getBitmap().getWidth() + _borderWidth,mCurrentSelect.getBitmap().getHeight() + _borderHeight);
			dest = new Rect(mWidth*2/3 - _borderWidth ,mHeight/2 - _borderHeight,mWidth + _borderWidth,mHeight + _borderHeight);
		}else{
			src = new Rect(_borderWidth,_borderHeight,mCurrentSelect.getBitmap().getWidth(),mCurrentSelect.getBitmap().getHeight());
			dest = new Rect(0,0,mWidth/3 + _borderWidth,mHeight/2 + _borderHeight);
		}
		cvs.drawBitmap(mCurrentSelect.getBitmap(), src,dest, mPaint);

	}

}
