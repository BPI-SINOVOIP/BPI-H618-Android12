package com.softwinner.tvdsetting;

import java.util.HashMap;
import java.io.FileReader;
import java.io.IOException;
import android.os.SystemProperties;

import com.softwinner.tvdsetting.DisplayAdjuestDialog.DisplayDialogInterface;
import com.softwinner.tvdsetting.widget.ExtImageButton;

import android.app.Activity;
import android.content.BroadcastReceiver;
import android.content.Context;
import android.content.Intent;
import android.content.IntentFilter;
import android.content.res.Resources;
import android.graphics.drawable.Drawable;
import android.hardware.display.DisplayManager;
import android.os.Bundle;
import android.util.Log;
import android.view.InputDevice;
import android.view.KeyEvent;
import android.view.MotionEvent;
import android.view.View;
import android.widget.ImageView;
import android.widget.TextView;
import android.widget.Toast;
import android.provider.Settings;
import android.provider.Settings.SettingNotFoundException;


public class DisplaySetting extends Activity implements View.OnClickListener, ExtImageButton.KeyEventInterface, View.OnFocusChangeListener{
	
	private static final String TAG = "DisplaySetting";

	private TextView mResTextView;
	private TextView mRecommendTextView;
	private ImageView mUpImgView;
	private ImageView mDownImgView;
	private Drawable mUpPress,mUpNor;
	private Drawable mDownPress,mDownNor;
	private HashMap<Integer,String> mDisplayMode = new HashMap<Integer,String>();
	private HashMap<Integer,Integer> mDisplaySerial = new HashMap<Integer,Integer>();
	String[] mode_entries ;
	int[] mode_value ;
	Resources res;
	boolean plugged = true;
	private int cnt = 0;
    private boolean isSupport = false;
    private boolean isSameMode = false;
    private int oldValue;
    private int newValue;
    private DisplayManager mDisplayManager;
	ExtImageButton mScaleSetting;
	ExtImageButton mResresolution;
    private BroadcastReceiver mDisplayPluggedListener = new BroadcastReceiver(){

        @Override
        public void onReceive(Context context, Intent intent) {
            int value = mDisplayManager.getDisplayOutput(android.view.Display.TYPE_BUILT_IN);
            Log.d(TAG,"value === " + value + "mDisplayMode = " + mDisplayMode.toString());
            
            mResTextView.setText(mDisplayMode.get((Integer)value));

            if(mDisplaySerial.containsKey(value)){
            	cnt = mDisplaySerial.get(value);
				if(oldValue==0){
					oldValue = value;
					newValue = oldValue;
				}
            }
			
        }
    };    
	@Override
	protected void onCreate(Bundle savedInstanceState) {
		super.onCreate(savedInstanceState);
		setContentView(R.layout.displaysetting);
		
	    res =getResources();
		
		mScaleSetting = (ExtImageButton)this.findViewById(R.id.scaleandshift);
		mResresolution = (ExtImageButton)this.findViewById(R.id.resolutionsetting);
		mUpImgView = (ImageView)this.findViewById(R.id.ic_up);
		mDownImgView = (ImageView)this.findViewById(R.id.ic_down);
		mUpNor = res.getDrawable(R.drawable.displaysetting_arrow_up);
		mUpPress = res.getDrawable(R.drawable.displaysetting_arrow_up);
		mDownNor = res.getDrawable(R.drawable.displaysetting_arrow_down);
		mDownPress = res.getDrawable(R.drawable.displaysetting_arrow_down);
		mResTextView = (TextView)this.findViewById(R.id.resolutions); 
		mRecommendTextView = (TextView)this.findViewById(R.id.recommends); 
		mScaleSetting.setOnClickListener(this);
		mResresolution.setOnClickListener(this);
		mResresolution.setOnDispatchKeyEvent(this);
		mResresolution.setOnFocusChangeListener(this);
        mDisplayManager = (DisplayManager)getSystemService(
                Context.DISPLAY_SERVICE);		
		
		if(isHdmiMode()) {
			mode_entries = res.getStringArray(R.array.hdmi_output_mode_entries);
			mode_value = res.getIntArray(R.array.hdmi_output_mode_values);
			
			if (mResTextView != null) {
            registerReceiver(mDisplayPluggedListener, new IntentFilter(
                         Intent.ACTION_HDMISTATUS_CHANGED));
			}		
		}else {
			mode_entries = res.getStringArray(R.array.cvbs_output_mode_entries);
			mode_value = res.getIntArray(R.array.cvbs_output_mode_values);
		}
		
		for(int i=0;i<mode_value.length;i++){
			mDisplayMode.put(mode_value[i],mode_entries[i]);
			mDisplaySerial.put(mode_value[i],i);
			Log.d(TAG,"mode_value[i] = " + mode_value[i]);
		}
		
		if(!isHdmiMode()){
//			String platform = SystemProperties.get("ro.board.platform");
			int value = 0;
//			if(platform.equal(dolphin)) {
//				value = mDisplayManager.getDisplayOutput(android.view.Display.TYPE_BUILT_IN);
//            }else if(platform.equal(eagle)) {
//				value = mDisplayManager.getDisplayOutput(android.view.Display.BUILT_HDMI);
//			}
			if(DisplayManager.DISPLAY_OUTPUT_TYPE_TV == mDisplayManager.getDisplayOutputType(android.view.Display.TYPE_HDMI)) {
				value = mDisplayManager.getDisplayOutput(android.view.Display.TYPE_HDMI);
			} else {
				value = mDisplayManager.getDisplayOutput(android.view.Display.TYPE_BUILT_IN);
			}
			Log.d(TAG,"value11 === " + value + "mDisplayMode = " + mDisplayMode.toString());
            mResTextView.setText(mDisplayMode.get((Integer)value));
			mRecommendTextView.setVisibility(View.INVISIBLE);
		}
		mUpImgView.setOnClickListener(new View.OnClickListener() {
			
			@Override
			public void onClick(View arg0) {
				// TODO Auto-generated method stub
				resOnUp();
			}
		});
		
		mDownImgView.setOnClickListener(new View.OnClickListener() {
			
			@Override
			public void onClick(View arg0) {
				// TODO Auto-generated method stub
				resOnDown();
			}
		});
		
		
	}

	@Override
	public void onClick(View arg0) {
		// TODO Auto-generated method stub
		if(arg0.getId() == R.id.scaleandshift){
			Intent intent = new Intent();
			intent.setClass(DisplaySetting.this, DisplayScaleSetting.class);
			startActivity(intent);
		}else if(arg0.getId() == R.id.resolutionsetting){
			if(newValue!=oldValue){
				showChangeDialog(mode_entries[cnt],oldValue,newValue);
				switchDispFormat(mode_value[cnt],false);
			}else{
				mUpImgView.setVisibility(View.VISIBLE);
				mDownImgView.setVisibility(View.VISIBLE);
				mResresolution.setBackgroundResource(R.drawable.displaysetting_btn_focus);
			}
		}
	}

	@Override
    public void onPause() {
        super.onPause();
    }	
	
	protected void onDestroy() {
		super.onDestroy();
		unregisterReceiver(mDisplayPluggedListener);
	}
	@Override
	public boolean onDispatchKeyEvent(View view,KeyEvent event) {
		// TODO Auto-generated method stub
		boolean retval = false;
		if(event.getKeyCode()==KeyEvent.KEYCODE_DPAD_UP){
        	if(event.getAction() == KeyEvent.ACTION_DOWN){
        		Log.d(TAG,"onDispatchKeyEvent 1");
        		resOnUp();
        	}else if(event.getAction() == KeyEvent.ACTION_UP){
        		Log.d(TAG,"onDispatchKeyEvent 2");
        		mUpImgView.setImageDrawable(mUpNor);
        	}
        	retval = true;
		}else if(event.getKeyCode()==KeyEvent.KEYCODE_DPAD_DOWN){
        	if(event.getAction() == KeyEvent.ACTION_DOWN){
        		Log.d(TAG,"onDispatchKeyEvent 3");
        		resOnDown();
        	}else if(event.getAction() == KeyEvent.ACTION_UP){
        		Log.d(TAG,"onDispatchKeyEvent 4");
        		mDownImgView.setImageDrawable(mDownNor);
        	}
        	retval = true;
		}else if(event.getKeyCode()==KeyEvent.KEYCODE_DPAD_CENTER ||
				event.getKeyCode()==KeyEvent.KEYCODE_ENTER){
			if(event.getAction() == KeyEvent.ACTION_DOWN){
				if(newValue!=oldValue){
					showChangeDialog(mode_entries[cnt],oldValue,newValue);
					switchDispFormat(mode_value[cnt],false);
				}
			}
		}
		return retval;
	}
	
	void resOnUp(){
		//oldValue = mode_value[cnt];
		if(cnt<mode_value.length - 1){
			cnt += 1;
		} else {
			cnt = 0;
		}
		newValue = mode_value[cnt];
		mResTextView.setText(mode_entries[cnt]);
		mUpImgView.setImageDrawable(mUpPress);
		//switchDispFormat(mode_value[cnt],false);
		//showChangeDialog(mode_entries[cnt],oldValue,newValue);
	}
	
	void resOnDown(){
		//oldValue = mode_value[cnt];
		if(cnt>0){
			cnt -= 1;
		} else {
			cnt = mode_value.length - 1;
		}
		newValue = mode_value[cnt];
		mResTextView.setText(mode_entries[cnt]);
		mDownImgView.setImageDrawable(mDownPress);
		//switchDispFormat(mode_value[cnt],false);
		//showChangeDialog(mode_entries[cnt],oldValue,newValue);
	}
	
    @Override
    public boolean onGenericMotionEvent(MotionEvent event) {
    	if (0 != (event.getSource() & InputDevice.SOURCE_CLASS_POINTER)) {
    		switch (event.getAction()) {
    			case MotionEvent.ACTION_SCROLL:   
    				if( event.getAxisValue(MotionEvent.AXIS_VSCROLL) < 0.0f){
    					resOnUp();
    				}else{
    					resOnDown();
    				}
    				mUpImgView.setVisibility(View.VISIBLE);
    				mDownImgView.setVisibility(View.VISIBLE);
                return true;
            }
    	}
    	return super.onGenericMotionEvent(event);
    }
	
	private boolean isHdmiMode(){
		final String filename = "/sys/class/switch/hdmi/state";
            FileReader reader = null;
            try {
                reader = new FileReader(filename);
                char[] buf = new char[15];
                int n = reader.read(buf);
                if (n > 1) {
                    plugged = 0 != Integer.parseInt(new String(buf, 0, n-1));
                }
            } catch (IOException ex) {
                Log.e(TAG, "Couldn't read hdmi state from " + filename + ": " + ex);
				return false;
            } catch (NumberFormatException ex) {
                Log.e(TAG, "Couldn't read hdmi state from " + filename + ": " + ex);
				return false;
            } finally {
                if (reader != null) {
                    try {
                        reader.close();
                    } catch (IOException ex) {
                    }
                }
            }
		return plugged;
	}
	
	private void showChangeDialog(String text,int oldvalue,int newvalue){
		final int valueNew = newvalue;
		final int valueOld = oldvalue;
		final DisplayAdjuestDialog dd = new DisplayAdjuestDialog(this,R.style.CommonDialog,text);
		DisplayDialogInterface ddinterface = new DisplayDialogInterface(){

			@Override
			public void onButtonYesClick() {
				
				switchDispFormat(valueNew,true);
				dd.dismiss();
				setOldValue(valueNew);
			}

			@Override
			public void onButtonNoClick() {

				newValue = valueOld;
				switchDispFormat(valueOld,false);
				dd.dismiss();
			}

			@Override
			public void onTimeOut() {

				if(dd.isShowing()){
					Log.d(TAG,"switchDispFormat to old mode " + valueOld);
					switchDispFormat(valueOld,false);
					dd.dismiss();
				}
			}
			
		};
		dd.setDisplayDialogInterface(ddinterface);
		dd.show();
		
	}
	private void setOldValue(int value){
		oldValue = value;
	}
	private void switchDispFormat(int value, boolean save) {
	    try {
	        int format = value;
	        final DisplayManager dm = (DisplayManager)getSystemService(Context.DISPLAY_SERVICE);
            int dispformat = dm.getDisplayModeFromFormat(format);
            int mCurType = dm.getDisplayOutputType(android.view.Display.TYPE_BUILT_IN);
//            isSupport = mDisplayManager.isSupportHdmiMode(android.view.Display.TYPE_BUILT_IN, dispformat);
            
//			if(isHdmiMode()){
//				if(isSupport){
//					dm.setDisplayOutput(android.view.Display.TYPE_BUILT_IN, format);
//				}else {
//					Toast.makeText(this, res.getString(R.string.unsupport),Toast.LENGTH_SHORT).show();
//				}
//			}else {
				dm.setDisplayOutput(android.view.Display.TYPE_BUILT_IN, format);			
//            }
			
			if(save){
				Settings.System.putString(getContentResolver(), Settings.System.DISPLAY_OUTPUT_FORMAT, Integer.toHexString(value));
			}
			Log.d(TAG,"switchDispFormat = " + mDisplaySerial.get(value));
			mResTextView.setText(mDisplayMode.get(value));
			cnt = mDisplaySerial.get(value);
			
        } catch (NumberFormatException e) {
            Log.w(TAG, "Invalid display output format!");
        }
	}

	@Override
	public void onFocusChange(View arg0, boolean hasFocus) {
		
		switch(arg0.getId()){
		case R.id.resolutionsetting:
			if (hasFocus) {
				mUpImgView.setVisibility(View.VISIBLE);
				mDownImgView.setVisibility(View.VISIBLE);
			} else {
				mUpImgView.setVisibility(View.INVISIBLE);
				mDownImgView.setVisibility(View.INVISIBLE);
			}
			break;
		default:
			break;
		}
		
	}	
	@Override
	public boolean onKeyDown (int keyCode, KeyEvent event){
		TextView title = (TextView) this.findViewById(R.id.title);
		switch(keyCode){
		case KeyEvent.KEYCODE_BACK:
    		title.setText(R.string.setting_plot);
			break; 
		}
		return false;
	}
	@Override
	public boolean onKeyUp (int keyCode, KeyEvent event){
		TextView title = (TextView) this.findViewById(R.id.title);
		switch(keyCode){
		case KeyEvent.KEYCODE_BACK:
			title.setText(R.string.setting_plot);
			finish();
			break;
		}
		return false;
	}
}
