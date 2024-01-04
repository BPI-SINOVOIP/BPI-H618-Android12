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
 * ExtendImageButton
 * A button with text under
 * @author yuguoxu
 */
public class ExtendImageButton extends ImageButton {
    private static final String TAG = "ExtendImageButton";
	private String _text = "";
    private int _color = -1;
    private float _textsize = 0f;
    private Paint paint;
    
    public ExtendImageButton(Context context, AttributeSet attrs) {
        super(context, attrs);
        TypedArray a = context.obtainStyledAttributes(attrs,  
                R.styleable.TextImageView);  
          
        _color = a.getColor(R.styleable.TextImageView_textColor,  
                0XFFFFFFFF);  
        _textsize = a.getDimension(R.styleable.TextImageView_textSize, 50);  
          
        _text = a.getString(R.styleable.TextImageView_text)==null?"":a.getString(R.styleable.TextImageView_text);

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
        canvas.drawText(_text, canvas.getWidth()/2, (canvas.getHeight()/2)+15, paint);
    }
}
