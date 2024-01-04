package com.softwinner.tvdsetting.about;

import com.softwinner.tvdsetting.R;

import android.app.Activity;
import android.content.Context;
import android.content.Intent;
import android.content.SharedPreferences;
import android.content.SharedPreferences.Editor;
import android.os.Bundle;
import android.util.Log;
import android.view.KeyEvent;
import android.widget.EditText;
import android.widget.ImageView;
import android.widget.LinearLayout;
import android.widget.TextView;
import android.view.View;
import android.view.inputmethod.EditorInfo;
import android.view.inputmethod.InputMethodManager;


public class UserDefined extends Activity implements View.OnFocusChangeListener , EditText.OnEditorActionListener {
	
	private LinearLayout mDeviceNameLayout;
	private EditText userdefined;
    static String device_name_defined = "";
	@Override
	protected void onCreate(Bundle savedInstanceState) {
		super.onCreate(savedInstanceState);
		setContentView(R.layout.userdefined);
		mDeviceNameLayout = (LinearLayout)this.findViewById(R.id.devicename_layout);
		userdefined = (EditText)findViewById(R.id.userinput);
		userdefined.setText(device_name_defined);	
		userdefined.setSelection(userdefined.length());
		userdefined.setOnFocusChangeListener(this);
		userdefined.setOnEditorActionListener(this);

	}
	
	@Override
	protected void onResume() {
		// TODO Auto-generated method stub
		super.onResume();
		userdefined.setText(device_name_defined);
		userdefined.setSelection(userdefined.length());
	}
	
	@Override
	public boolean onKeyDown(int keyCode, KeyEvent event) {
		TextView title = (TextView) this.findViewById(R.id.title);
		switch (keyCode) {
		case KeyEvent.KEYCODE_BACK:
			title.setText(R.string.setting_name);
			device_name_defined = userdefined.getText().toString();
			break;
		}
		return false;
	}

	@Override
	public boolean onKeyUp(int keyCode, KeyEvent event) {
		TextView title = (TextView) this.findViewById(R.id.title);
		int passPosition = getIntent().getIntExtra("recodePosition", 1);
		
    	Intent data=new Intent();
		switch (keyCode) {
		case KeyEvent.KEYCODE_BACK:
			
			title.setText(R.string.setting_name);
		                      
            if(device_name_defined.equals("")){  
    			if(passPosition != 4){
    				data.putExtra("passPosition", passPosition);
    			}
            }else {
            	SharedPreferences sharedPreferences = getSharedPreferences("device_name", MODE_PRIVATE);
                Editor editor = sharedPreferences.edit();
                editor.putString("device_name_defined", device_name_defined);
                editor.commit();
            }
            
            setResult(RESULT_OK, data);
			finish();
			break;
		}
		return false;
	}
	
	@Override
	public void onFocusChange(View arg0, boolean hasFocus) {
		switch(arg0.getId()){
		case R.id.userinput:
			if (hasFocus) {
				mDeviceNameLayout.setBackgroundResource(R.drawable.input_btn_focus);
			} else {
				mDeviceNameLayout.setBackgroundResource(R.drawable.list_btn_nor);
			}
			break;
		default:
			break;
		}
	}

	@Override
	public boolean onEditorAction(TextView v, int actionId, KeyEvent event) {
		// TODO Auto-generated method stub
        if(actionId == EditorInfo.IME_ACTION_DONE){  
            InputMethodManager imm = (InputMethodManager) v  
                    .getContext().getSystemService(  
                            Context.INPUT_METHOD_SERVICE);  
            if (imm.isActive()) {  
                imm.hideSoftInputFromWindow(  
                        v.getApplicationWindowToken(), 0);  
            }  
            device_name_defined = userdefined.getText().toString();
            int passPosition = getIntent().getIntExtra("recodePosition", 1); 
            Intent data=new Intent(); 
            if(device_name_defined.equals("")){  
    			if(passPosition != 4){
    				data.putExtra("passPosition", passPosition);
    			}
            }else {
            	SharedPreferences sharedPreferences = getSharedPreferences("device_name", MODE_PRIVATE);
                Editor editor = sharedPreferences.edit();
                editor.putString("device_name_defined", device_name_defined);
                editor.commit();
            }
            setResult(RESULT_OK, data);
			finish();
            return true;  
        }  
        return false;  
	}
}
