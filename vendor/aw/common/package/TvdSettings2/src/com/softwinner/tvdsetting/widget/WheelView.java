package com.softwinner.tvdsetting.widget;

import java.util.List;
import com.softwinner.tvdsetting.R;
import android.content.Context;
import android.content.res.TypedArray;
import android.graphics.Canvas;
import android.graphics.Paint;
import android.graphics.Paint.Align;
import android.graphics.Rect;
import android.util.AttributeSet;
import android.view.InputDevice;
import android.view.KeyEvent;
import android.view.LayoutInflater;
import android.view.MotionEvent;
import android.view.View;
import android.widget.LinearLayout;
import android.widget.TextView;

public class WheelView extends LinearLayout {

	private List<String> mEntries;
	private int position;
	private WheelViewInterface mWheelViewInterface;
	private TextView mTextView1, mTextView2, mTextView3;
	private String _text;

	public interface WheelViewInterface {
		boolean onPressKeyUp(View v, int position, String str);

		boolean onPressKeyDown(View v, int position, String str);

		boolean onPressKeyCenter(View v, int position, String str);
	}

	public WheelView(Context context, AttributeSet attrs) {
		super(context, attrs);
		LayoutInflater inflater = (LayoutInflater) context
				.getSystemService(Context.LAYOUT_INFLATER_SERVICE);
		inflater.inflate(R.layout.wheelview, this, true);
		mTextView1 = (TextView) this.findViewById(R.id.text1);
		mTextView2 = (TextView) this.findViewById(R.id.text2);
		mTextView3 = (TextView) this.findViewById(R.id.text3);
		// TODO Auto-generated constructor stub
		position = 0;
		mTextView2.setOnClickListener(new View.OnClickListener() {
			
			@Override
			public void onClick(View arg0) {
				// TODO Auto-generated method stub
				if (mWheelViewInterface != null) {
					mWheelViewInterface.onPressKeyCenter(WheelView.this, position, _text);
				}
			}
		});
	}

	public void setOnKeyEventListener(WheelViewInterface wheelViewInterface) {
		mWheelViewInterface = wheelViewInterface;
	}

	public WheelView(Context context, AttributeSet attrs, List<String> list) {
		this(context, attrs);
		// TODO Auto-generated constructor stub
		mEntries = list;
	}

	public void setEntry(List<String> list, int pos) {
		mEntries = list;
		int len = mEntries.size();
		position = pos >= 0 && pos < (len - 1) ? pos : 0;
		if (len > 2) {
			mTextView1.setText(mEntries.get(len - 1));
			mTextView2.setText(mEntries.get(position));
			mTextView3.setText(mEntries.get(position + 1));
		} else if (len == 2) {
			mTextView1.setText(mEntries.get(position + 1));
			mTextView2.setText(mEntries.get(position));
			mTextView3.setText("");
		} else if (len == 1) {
			mTextView1.setText("");
			mTextView2.setText(mEntries.get(position));
			mTextView3.setText("");
		} else {

		}
		// mTextView2.setBackgroundResource(R.drawable.inputbox_focus);
	}

	@Override
	public boolean dispatchKeyEvent(KeyEvent event) {
		boolean retval = false;
		int len = 0;
		if (mEntries != null)
			len = mEntries.size();
		if (event.getKeyCode() == KeyEvent.KEYCODE_DPAD_UP) {
			if (event.getAction() == KeyEvent.ACTION_DOWN) {
				if (position > 0) {
					position = position - 1;
				} else {
					position = position + len - 1;
				}
				if (len >= 3) {
					if (position > 0 && position < len - 1) {
						mTextView1.setText(mEntries.get(position - 1));
						mTextView2.setText(mEntries.get(position));
						mTextView3.setText(mEntries.get(position + 1));
					} else if (position == 0) {
						mTextView1.setText(mEntries.get(len - 1));
						mTextView2.setText(mEntries.get(position));
						mTextView3.setText(mEntries.get(position + 1));
					} else if (position == len - 1) {
						mTextView1.setText(mEntries.get(position - 1));
						mTextView2.setText(mEntries.get(position));
						mTextView3.setText(mEntries.get(0));
					}
				} else if (len == 2) {
					if (position == 0) {
						mTextView1.setText(mEntries.get(1));
						mTextView2.setText(mEntries.get(0));
						mTextView3.setText("");
					} else if (position == 1) {
						mTextView1.setText(mEntries.get(0));
						mTextView2.setText(mEntries.get(1));
						mTextView3.setText("");
					}
				}
				// mTextView2.setBackgroundResource(R.drawable.inputbox_focus);
				if (mWheelViewInterface != null) {
					mWheelViewInterface.onPressKeyUp(this, position, _text);
				}
				retval = true;
			} else if (event.getAction() == KeyEvent.ACTION_UP) {

			}
		} else if (event.getKeyCode() == KeyEvent.KEYCODE_DPAD_DOWN) {
			if (event.getAction() == KeyEvent.ACTION_DOWN) {
				if (position < len - 1) {
					position = position + 1;
				} else {
					position = 0;
				}
				if (len >= 3) {
					if (position > 0 && position < len - 1) {
						mTextView1.setText(mEntries.get(position - 1));
						mTextView2.setText(mEntries.get(position));
						mTextView3.setText(mEntries.get(position + 1));
					} else if (position == 0) {
						mTextView1.setText(mEntries.get(len - 1));
						mTextView2.setText(mEntries.get(position));
						mTextView3.setText(mEntries.get(position + 1));
					} else if (position == len - 1) {
						mTextView1.setText(mEntries.get(position - 1));
						mTextView2.setText(mEntries.get(position));
						mTextView3.setText(mEntries.get(0));
					}
				} else if (len == 2) {
					if (position == 0) {
						mTextView1.setText(mEntries.get(1));
						mTextView2.setText(mEntries.get(0));
						mTextView3.setText("");
					} else if (position == 1) {
						mTextView1.setText(mEntries.get(0));
						mTextView2.setText(mEntries.get(1));
						mTextView3.setText("");
					}
				}
				// mTextView2.setBackgroundResource(R.drawable.inputbox_focus);
				if (mWheelViewInterface != null) {
					mWheelViewInterface.onPressKeyDown(this, position, _text);
				}
				retval = true;
			} else if (event.getAction() == KeyEvent.ACTION_UP) {

			}
		} else if (event.getKeyCode() == KeyEvent.KEYCODE_DPAD_CENTER ||
				event.getKeyCode() == KeyEvent.KEYCODE_ENTER) {
			if (event.getAction() == KeyEvent.ACTION_DOWN) {
				if (mWheelViewInterface != null) {
					mWheelViewInterface.onPressKeyCenter(this, position, _text);
				}
			} else if (event.getAction() == KeyEvent.ACTION_UP) {

			}
		}
		this.invalidate();
		return false;
	}
	
    @Override
    public boolean onGenericMotionEvent(MotionEvent event) {
		int len = 0;
		if (mEntries != null)
			len = mEntries.size();
    	if (0 != (event.getSource() & InputDevice.SOURCE_CLASS_POINTER)) {
    		switch (event.getAction()) {
    			case MotionEvent.ACTION_SCROLL:   
    				if( event.getAxisValue(MotionEvent.AXIS_VSCROLL) < 0.0f){
    					if (position > 0) {
    						position = position - 1;
    					} else {
    						position = position + len - 1;
    					}
    					if (len >= 3) {
    						if (position > 0 && position < len - 1) {
    							mTextView1.setText(mEntries.get(position - 1));
    							mTextView2.setText(mEntries.get(position));
    							mTextView3.setText(mEntries.get(position + 1));
    						} else if (position == 0) {
    							mTextView1.setText(mEntries.get(len - 1));
    							mTextView2.setText(mEntries.get(position));
    							mTextView3.setText(mEntries.get(position + 1));
    						} else if (position == len - 1) {
    							mTextView1.setText(mEntries.get(position - 1));
    							mTextView2.setText(mEntries.get(position));
    							mTextView3.setText(mEntries.get(0));
    						}
    					} else if (len == 2) {
    						if (position == 0) {
    							mTextView1.setText(mEntries.get(1));
    							mTextView2.setText(mEntries.get(0));
    							mTextView3.setText("");
    						} else if (position == 1) {
    							mTextView1.setText(mEntries.get(0));
    							mTextView2.setText(mEntries.get(1));
    							mTextView3.setText("");
    						}
    					}
    					// mTextView2.setBackgroundResource(R.drawable.inputbox_focus);
    					if (mWheelViewInterface != null) {
    						mWheelViewInterface.onPressKeyUp(this, position, _text);
    					}
    				}else{
    					if (position < len - 1) {
    						position = position + 1;
    					} else {
    						position = 0;
    					}
    					if (len >= 3) {
    						if (position > 0 && position < len - 1) {
    							mTextView1.setText(mEntries.get(position - 1));
    							mTextView2.setText(mEntries.get(position));
    							mTextView3.setText(mEntries.get(position + 1));
    						} else if (position == 0) {
    							mTextView1.setText(mEntries.get(len - 1));
    							mTextView2.setText(mEntries.get(position));
    							mTextView3.setText(mEntries.get(position + 1));
    						} else if (position == len - 1) {
    							mTextView1.setText(mEntries.get(position - 1));
    							mTextView2.setText(mEntries.get(position));
    							mTextView3.setText(mEntries.get(0));
    						}
    					} else if (len == 2) {
    						if (position == 0) {
    							mTextView1.setText(mEntries.get(1));
    							mTextView2.setText(mEntries.get(0));
    							mTextView3.setText("");
    						} else if (position == 1) {
    							mTextView1.setText(mEntries.get(0));
    							mTextView2.setText(mEntries.get(1));
    							mTextView3.setText("");
    						}
    					}
    					// mTextView2.setBackgroundResource(R.drawable.inputbox_focus);
    					if (mWheelViewInterface != null) {
    						mWheelViewInterface.onPressKeyDown(this, position, _text);
    					}
    				}
                return true;
            }
    	}
    	return super.onGenericMotionEvent(event);
    }
    
    public int getPosition(){
    	return position;
    }
}
