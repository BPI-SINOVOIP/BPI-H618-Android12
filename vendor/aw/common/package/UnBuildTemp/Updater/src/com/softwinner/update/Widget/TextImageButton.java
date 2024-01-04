package com.softwinner.update.Widget;

import com.softwinner.update.R;

import android.view.View;
import android.widget.ImageButton;
import android.content.Context;
import android.content.res.TypedArray;
import android.graphics.Bitmap;
import android.graphics.Canvas;
import android.graphics.Paint;
import android.graphics.Paint.Align;
import android.graphics.drawable.BitmapDrawable;
import android.graphics.drawable.Drawable;
import android.util.AttributeSet;
import android.util.Log;
import android.widget.ImageButton;

/**
 * TextImageButton
 * A button with text under
 * @author yuguoxu
 */
public class TextImageButton extends ImageButton {
    private static final String TAG = "TextImageButton";
	private String _text = "";
    private int _color = -1;
    private float _textsize = 0f;
    private Bitmap _front;
    private Paint paint;
    
    public TextImageButton(Context context, AttributeSet attrs) {
        super(context, attrs);
        TypedArray a = context.obtainStyledAttributes(attrs,  
                R.styleable.TextImageView);  
          
        _color = a.getColor(R.styleable.TextImageView_textColor,  
                0XFFFFFFFF);  
        _textsize = a.getDimension(R.styleable.TextImageView_textSize, 50);  
          
        _text = a.getString(R.styleable.TextImageView_text)==null?"":a.getString(R.styleable.TextImageView_text);

        try{
	        BitmapDrawable bd =  (BitmapDrawable) a.getDrawable(R.styleable.TextImageView_src);
	
	        if(bd==null){
	        	Log.d(TAG,"can't find front drawable");
	        }
	        
	        _front = bd.getBitmap();
        }catch(Exception ex){
        	ex.printStackTrace();
        }
        
        paint = new Paint();
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
    
    @Override
    protected void onDraw(Canvas canvas) {
        super.onDraw(canvas);
        paint.setTextAlign(Align.CENTER);
        paint.setColor(_color);
        paint.setTextSize(_textsize);
        if(_front != null){
        	canvas.drawBitmap(_front, canvas.getWidth()/2 - _front.getWidth()/2, (canvas.getHeight()/2 - _front.getHeight()/2)*1/3, paint);
        }
        canvas.drawText(_text, canvas.getWidth()/2, (canvas.getHeight()*6/7), paint);

    }
}
