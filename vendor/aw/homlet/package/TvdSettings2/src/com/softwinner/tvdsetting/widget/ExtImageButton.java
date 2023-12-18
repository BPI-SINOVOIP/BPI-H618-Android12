package com.softwinner.tvdsetting.widget;

import com.softwinner.tvdsetting.R;

import android.content.Context;
import android.content.res.TypedArray;
import android.graphics.Canvas;
import android.graphics.Paint;
import android.graphics.Paint.Align;
import android.graphics.drawable.BitmapDrawable;
import android.util.AttributeSet;
import android.util.Log;
import android.view.KeyEvent;
import android.view.View;
import android.widget.ImageButton;

public class ExtImageButton extends ImageButton {
    private static final String TAG = "ExtImageButton";
	private String _text = "";
    private int _color = -1;
    private float _textsize = 0f;
    private boolean mPressed;
    private HomeView mHomeView;
    private KeyEventInterface mKeyEventInterface;
    
    public  interface KeyEventInterface{
    	boolean onDispatchKeyEvent(View view,KeyEvent event);
    }
    public ExtImageButton(Context context, AttributeSet attrs) {
        super(context, attrs);
        TypedArray a = context.obtainStyledAttributes(attrs,  
                R.styleable.ExtImageButton);  
          
        _color = a.getColor(R.styleable.ExtImageButton_textColor,  
                0XFFFFFFFF);  
        _textsize = a.getDimension(R.styleable.ExtImageButton_textSize, 50);  
          
        _text = a.getString(R.styleable.ExtImageButton_text)==null?"":a.getString(R.styleable.ExtImageButton_text);
        
        
        a.recycle();  
    }
    
    public void setText(String text){
        this._text = text;
    }
    
    public void setColor(int color){
        this._color = color;
    }
    
    public void setTextSize(float textsize){
        this._textsize = textsize;
    }
    
    public boolean isBtnPressed(){
    	return mPressed;
    }
    
    public void setBackgroudView(HomeView homeView){
    	mHomeView = homeView;
    }
    
    public void setOnDispatchKeyEvent(KeyEventInterface keyEventInterface){
    	mKeyEventInterface = keyEventInterface;
    }
    @Override  
    public boolean dispatchKeyEvent(KeyEvent event) {  
    	
        if (event.getKeyCode() == KeyEvent.KEYCODE_DPAD_CENTER){
        	if(event.getAction() == KeyEvent.ACTION_DOWN){
        		mPressed = true;
        	}else if(event.getAction() == KeyEvent.ACTION_UP){
        		mPressed = false;
        	}
        }  
        if(mHomeView!=null){
        	mHomeView.setState(mPressed);
        }
        Log.d(TAG,"dispatchKeyEvent mPressed = " + mPressed);
        
        if(mKeyEventInterface!=null){
        	return  mKeyEventInterface.onDispatchKeyEvent(this,event);
        }
        return super.dispatchKeyEvent(event);  
        
    }      
    
    @Override
    protected void onDraw(Canvas canvas) {
        super.onDraw(canvas);
        Paint paint = new Paint();
        paint.setTextAlign(Align.CENTER);
        paint.setColor(_color);
        paint.setTextSize(_textsize);
        canvas.drawText(_text, canvas.getWidth()/2, (canvas.getHeight()/2)+20, paint);
    }
}
