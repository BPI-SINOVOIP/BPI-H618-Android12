package com.softwinner.tvdsetting.widget;

import android.content.Context;
import android.graphics.Canvas;
import android.graphics.Paint;
import android.graphics.Paint.Align;
import android.util.AttributeSet;
import android.util.Log;
import android.view.KeyEvent;
import android.widget.ImageButton;
import android.widget.LinearLayout;
import java.util.HashMap;
import com.softwinner.tvdsetting.R;
import android.content.res.TypedArray;
import android.widget.BaseAdapter;

public class ItemSelectView extends ImageButton {
	private static final String TAG = "ItemSelectView";
	Paint mPaint;
	int _color = -1;
	int _textSize = 50;
	String _text="";
	CharSequence[] mEntries;
	ItemSwitch mItemSwitch;
	int position;
	public interface ItemSwitch{
		void onItemSwitchPrev(int pos,String text);
		void onItemSwitchNext(int pos,String text);
		void onItemSwitchCenter(int pos,String text);
	}
	public ItemSelectView(Context context, AttributeSet attrs) {
		super(context, attrs);
		position = 0;
        TypedArray a = context.obtainStyledAttributes(attrs,  
                R.styleable.ItemSelectView);  
        mEntries = a.getTextArray(
                R.styleable.ItemSelectView_entries);  
        _color = a.getColor(R.styleable.ItemSelectView_isTextColor,  
                0XFFFFFFFF);  
        _textSize = (int) a.getDimension(R.styleable.ItemSelectView_isTextSize, 50);  
        
        position = (int) a.getInteger(R.styleable.ItemSelectView_isPosition, 1);
        a.recycle();
        if(mEntries!=null){
        	if(position<mEntries.length)
        		_text = mEntries[position].toString();
        }
		mPaint = new Paint();
		mPaint.setColor(_color);
		mPaint.setStyle(Paint.Style.STROKE);
		mPaint.setTextAlign(Align.CENTER);
		mPaint.setTextSize(_textSize);
	}
	
    public void setItemSwitch(ItemSwitch is){
        mItemSwitch = is;
    }
	public void setEntries(CharSequence[] entries){
		mEntries = entries;
	}
	
	public CharSequence[] getEntries(){
		return mEntries;
	}
	
	public int getPosition(){
		return position;
	}
	public void setPosition(int pos){
	    position = pos;
	    if(mEntries!=null){
        	if(position<mEntries.length)
        		_text = mEntries[position].toString();
        }
        this.invalidate();
	}
	String getText(){
		return mEntries[position].toString();
	}

	@Override
	public void onDraw(Canvas cvs){
		int w = cvs.getWidth();
		int h = cvs.getHeight();
		cvs.drawText(_text, cvs.getWidth()/2, (cvs.getHeight()/2)+17, mPaint);
	}
	
    @Override  
    public boolean dispatchKeyEvent(KeyEvent event) {
    	Log.d(TAG,"dispatchKeyEvent " + event.getKeyCharacterMap().toString());
    	Log.d(TAG,"position = " + position);
    	Log.d(TAG,"mEntries.length = " + mEntries.length);
    	boolean retval = false;
        if (event.getKeyCode() == KeyEvent.KEYCODE_DPAD_LEFT){
        	if(event.getAction() == KeyEvent.ACTION_DOWN){
        		if(position > 0){
        			position = position - 1;
        		} else {
        			position = position + mEntries.length - 1;
        		}
        		_text = mEntries[position].toString();
           		if(mItemSwitch!=null){
        			mItemSwitch.onItemSwitchPrev(position,_text);
        		}
           		retval = true;
        	}else if(event.getAction() == KeyEvent.ACTION_UP){

        	}
        }else if(event.getKeyCode() == KeyEvent.KEYCODE_DPAD_RIGHT){
        	if(event.getAction() == KeyEvent.ACTION_DOWN){
        		if(position < mEntries.length - 1){
        			position = position + 1;
        		} else {
        			position = 0;
        		}
        		_text = mEntries[position].toString();
           		if(mItemSwitch!=null){
        			mItemSwitch.onItemSwitchNext(position,_text);
        		}
           		retval = true;
        	}else if(event.getAction() == KeyEvent.ACTION_UP){

        	}      	
        }else if(event.getKeyCode() == KeyEvent.KEYCODE_DPAD_CENTER){
        	if(event.getAction() == KeyEvent.ACTION_DOWN){
           		if(mItemSwitch!=null){
        			mItemSwitch.onItemSwitchCenter(position,_text);
        		}        		
        	}else if(event.getAction() == KeyEvent.ACTION_UP){

        	}
        }
        this.invalidate();
		return retval;  
    }
}
