package com.softwinner.animationview;

import android.content.Context;
import android.util.AttributeSet;
import android.view.ViewDebug.ExportedProperty;
import android.widget.TextView;

public class AnimationText extends TextView {

	public AnimationText(Context context) {
		super(context);
		// TODO Auto-generated constructor stub
	}

	public AnimationText(Context context, AttributeSet attrs) {
		super(context, attrs);
		// TODO Auto-generated constructor stub
	}

	public AnimationText(Context context, AttributeSet attrs, int defStyle) {
		super(context, attrs, defStyle);
		// TODO Auto-generated constructor stub
	}

	@Override
	@ExportedProperty(category = "focus")
	public boolean isFocused() {
		// TODO Auto-generated method stub
		return true;
	}
	
	

}
