package com.softwinner.tvdsetting;

import java.util.ArrayList;
import java.util.HashMap;

import com.softwinner.tvdsetting.applications.AppManagerActivity;
import com.softwinner.tvdsetting.net.NetSettings;
import com.softwinner.tvdsetting.widget.HomeView;

import android.os.Bundle;
import android.app.Activity;
import android.content.ComponentName;
import android.content.Intent;
import android.content.pm.ApplicationInfo;
import android.content.pm.PackageManager;
import android.content.pm.PackageManager.NameNotFoundException;
import android.content.res.Resources;
import android.graphics.Canvas;
import android.graphics.Paint;
import android.graphics.Rect;
import android.graphics.drawable.BitmapDrawable;
import android.util.DisplayMetrics;
import android.util.Log;
import android.view.Menu;
import android.view.View;
import android.widget.GridView;
import android.widget.ImageButton;
import android.widget.SimpleAdapter;

import com.softwinner.tvdsetting.widget.ExtImageButton;

public class Settings extends Activity implements View.OnClickListener,View.OnFocusChangeListener{

	private static final String TAG = "Settings";
	ExtImageButton mNetworkBtn;
	ExtImageButton mDisplayBtn;
	ExtImageButton mPlayerBtn;
	ExtImageButton mAppSettingBtn;
	ExtImageButton mUpdateBtn;
	ExtImageButton mAdvancedBtn;
	HomeView mHomeView;
	
	//GridView mGV = null;
	
	@Override
	protected void onCreate(Bundle savedInstanceState) {
		super.onCreate(savedInstanceState);
		setContentView(R.layout.main);
		initUI();
		
	}
	
	private void initUI(){
		
		mHomeView = (HomeView)findViewById(R.id.homeview);
		mNetworkBtn = (ExtImageButton) findViewById(R.id.networksetting);
		mDisplayBtn = (ExtImageButton) findViewById(R.id.displaysetting);
		mPlayerBtn  = (ExtImageButton) findViewById(R.id.playersetting);
		mAppSettingBtn = (ExtImageButton) findViewById(R.id.appsetting);
		mUpdateBtn = (ExtImageButton) findViewById(R.id.updatesetting);
		mAdvancedBtn = (ExtImageButton)findViewById(R.id.advancesetting);
		mNetworkBtn.setOnClickListener(this);
		mNetworkBtn.setOnFocusChangeListener(this);
		mNetworkBtn.setBackgroudView(mHomeView);
		
		mDisplayBtn.setOnClickListener(this);
		mDisplayBtn.setOnFocusChangeListener(this);
		mDisplayBtn.setBackgroudView(mHomeView);
		
		mPlayerBtn.setOnClickListener(this);
		mPlayerBtn.setOnFocusChangeListener(this);
		mPlayerBtn.setBackgroudView(mHomeView);
		
		mAppSettingBtn.setOnClickListener(this);
		mAppSettingBtn.setOnFocusChangeListener(this);
		mAppSettingBtn.setBackgroudView(mHomeView);
		
		mUpdateBtn.setOnClickListener(this);
		mUpdateBtn.setOnFocusChangeListener(this);
		mUpdateBtn.setBackgroudView(mHomeView);
		
		mAdvancedBtn.setOnClickListener(this);
		mAdvancedBtn.setOnFocusChangeListener(this);
		mAdvancedBtn.setBackgroudView(mHomeView);

		
        DisplayMetrics metric = new DisplayMetrics();
        getWindowManager().getDefaultDisplay().getMetrics(metric);
        int width = metric.widthPixels;
        int height = metric.heightPixels;
        
        Log.d(TAG,"width = " + width);
        Log.d(TAG,"height = " + height);
        
        
	}

	@Override
	public boolean onCreateOptionsMenu(Menu menu) {
		// Inflate the menu; this adds items to the action bar if it is present.
		getMenuInflater().inflate(R.menu.main, menu);
		return true;
	}

	@Override
	public void onClick(View arg0) {
		// TODO Auto-generated method stub
		Log.d(TAG,"onClick id = " + arg0.getId());
		Log.d(TAG,"pressed is " + ((ExtImageButton) arg0).isBtnPressed());
		switch(arg0.getId()){
		case R.id.networksetting:
			onNetworkSettingClicked();
			break;
		case R.id.displaysetting:
			onDisplaySettingClicked();
			break;
		case R.id.playersetting:
			onPlayerSettingClicked();
			break;
		case R.id.appsetting:
			onAppSettingClicked();
			break;
		case R.id.updatesetting:
			onUpdateSettingClicked();
			break;
		case R.id.advancesetting:
			onAdvanceSettingClicked();
			break;
		default:
			break;
		}
	}

	private void onAdvanceSettingClicked() {
		// TODO Auto-generated method stub
		if(checkPackage("com.android.settings")){
			Log.d(TAG,"apk exist!");
	        Intent mIntent = new Intent( );   
	        ComponentName comp = new ComponentName("com.android.settings", "com.android.settings.Settings");  
	        mIntent.setComponent(comp);
	        startActivity(mIntent); 
		}else{
			Log.d(TAG,"ota not exist");

		}		
	}

	private void onUpdateSettingClicked() {
		// TODO Auto-generated method stub
		startOtaUpdate();
	}

	private void onAppSettingClicked() {
		// TODO Auto-generated method stub
		Intent intent = new Intent();
		intent.setClass(Settings.this, AppManagerActivity.class);
		startActivity(intent);
	}

	private void onPlayerSettingClicked() {
		// TODO Auto-generated method stub
//		Intent intent = new Intent();
//		intent.setClass(Settings.this, VideoSetting.class);
//		startActivity(intent);	
		
		Intent intent = new Intent();
		intent.setClass(Settings.this, CommonSettings.class);
		startActivity(intent);				
	}

	private void onDisplaySettingClicked() {
		// TODO Auto-generated method stub
		Intent intent = new Intent();
		intent.setClass(Settings.this, DisplaySetting.class);
		startActivity(intent);
	}

	private void onNetworkSettingClicked() {
		// TODO Auto-generated method stub
		Intent intent = new Intent();
		intent.setClass(Settings.this, NetSettings.class);
		startActivity(intent);
	}
    public boolean checkPackage(String packageName) {  
        if (packageName == null || "".equals(packageName))  
            return false;  
        try {  
            ApplicationInfo info = getPackageManager().getApplicationInfo(  
                    packageName, PackageManager.GET_UNINSTALLED_PACKAGES);  
            return true;  
        } catch (NameNotFoundException e) {  
            return false;  
        }  
    }  
    
	private void startOtaUpdate(){
		if(checkPackage("com.softwinner.update")){
			Log.d(TAG,"apk exist!");
	        Intent mIntent = new Intent( );   
	        ComponentName comp = new ComponentName("com.softwinner.update", "com.softwinner.update.HomeActivity");  
	        mIntent.setComponent(comp);    
	        startActivity(mIntent); 
		}else{
			Log.d(TAG,"ota not exist");

		}
	}
	
	@Override
	public void onFocusChange(View arg0, boolean arg1) {
		Log.d(TAG,"onFocusChange arg1 = " + arg1);
		// TODO Auto-generated method stub
		switch(arg0.getId()){
		case R.id.networksetting:
			if(arg1)
				mHomeView.setSelectState(0, false);
			break;
		case R.id.displaysetting:
			if(arg1)
				mHomeView.setSelectState(1, false);
			break;
		case R.id.playersetting:
			if(arg1)
				mHomeView.setSelectState(2, false);
			break;
		case R.id.appsetting:
			if(arg1)
				mHomeView.setSelectState(3, false);
			break;
		case R.id.updatesetting:
			if(arg1)
				mHomeView.setSelectState(4, false);
			break;
		case R.id.advancesetting:
			if(arg1)
				mHomeView.setSelectState(5, false);
			break;
		default:
			break;
		}
	}

}
