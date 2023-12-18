package com.softwinner.tvdsetting;

import java.io.File;
import java.io.FileWriter;
import java.io.IOException;
import java.util.HashMap;
import java.util.Locale;

import com.softwinner.tvdsetting.SystemRecoveryDialog.SystemRecoveryDialogInterface;
import com.softwinner.tvdsetting.about.AboutSetting;
import com.softwinner.tvdsetting.weather.WeatherLocationActivity;

import android.app.Activity;
import android.content.ComponentName;
import android.content.Context;
import android.content.Intent;
import android.content.pm.ApplicationInfo;
import android.content.pm.PackageManager;
import android.content.pm.PackageManager.NameNotFoundException;
import android.content.res.Resources;
import android.os.Bundle;
import android.os.PowerManager;
import android.os.UserManager;
import android.util.Log;
import android.view.KeyEvent;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;
import android.widget.AdapterView;
import android.widget.BaseAdapter;
import android.widget.ImageView;
import android.widget.ListView;
import android.widget.TextView;
import android.provider.Settings;
import static android.provider.Settings.System.SCREEN_OFF_TIMEOUT;

public class CommonSettings extends Activity  implements ListView.OnKeyListener , ListView.OnItemClickListener{
	private static final String TAG = "CommonSettings";
	Context mContext;
	ListView mListView;
	private final int ENABLE_SUSPEND = 0;
	private final int WETHER_LOCATION = 1;
	private final int INSTALL_APPLICATION = 2;
	private final int FACTORY_RECOVERY = 3;
//	private final int ABOUT_PRODUCT = 4;
	private final int SENIOR_SETTING = 4;
	
	private String [] suspend_time;
	private int [] screen_timeout;
	private int cnt = 0;
	private boolean allowNonMarketApk = true;
	private int screenTimeout = -1;
	private VideoSettingAdapter mVideoSettingAdapter;
	private HashMap<Integer,Integer> mMap = new HashMap();
	@Override
	protected void onCreate(Bundle savedInstanceState) {
		super.onCreate(savedInstanceState);
		mContext = this;
		Resources res = this.getResources();
		suspend_time = res.getStringArray(R.array.suspend_time);
		screen_timeout = res.getIntArray(R.array.screen_timeout);
		for(int i=0;i<screen_timeout.length;i++)
			mMap.put(screen_timeout[i], i);
		allowNonMarketApk = isNonMarketAppsAllowed();
		screenTimeout = getScreenTimeoutValue();
		if(mMap.containsKey(screenTimeout)){
			cnt = mMap.get(screenTimeout);
		}else{
			screenTimeout = 1800000;
			setScreenTimeoutValue(screenTimeout);
			cnt = 2;
		}
		this.setContentView(R.layout.common_setting);
		mListView = (ListView)this.findViewById(R.id.common_setting_list);
		mVideoSettingAdapter = new VideoSettingAdapter();
		mListView.setAdapter(mVideoSettingAdapter);
		mListView.setOnItemClickListener(this);
		mListView.setOnKeyListener(this);
	}
	
	class VideoSettingAdapter extends BaseAdapter{

		@Override
		public int getCount() {
			// TODO Auto-generated method stub
			return SENIOR_SETTING + 1;
		}

		@Override
		public Object getItem(int position) {
			// TODO Auto-generated method stub
			return null;
		}

		@Override
		public long getItemId(int position) {
			// TODO Auto-generated method stub
			return 0;
		}

		@Override
		public View getView(int position, View convertView, ViewGroup parent) {
			// TODO Auto-generated method stub
			convertView=LayoutInflater.from(mContext).inflate(R.layout.commonsettingitem, null);
			TextView text = (TextView)convertView.findViewById(R.id.text1);
			TextView state = (TextView)convertView.findViewById(R.id.state);
			ImageView iconLeft = (ImageView)convertView.findViewById(R.id.icon1);
			ImageView iconRight = (ImageView)convertView.findViewById(R.id.icon2);
			switch(position){
			case ENABLE_SUSPEND:
				text.setText(R.string.enable_suspend);
				state.setText(suspend_time[cnt]);
				iconLeft.setOnClickListener(new View.OnClickListener(){

					@Override
					public void onClick(View arg0) {
						// TODO Auto-generated method stub
						if(cnt>0) {
							cnt--;
						} else {
							cnt = suspend_time.length - 1;
						}
						setScreenTimeoutValue(screen_timeout[cnt]);
						mVideoSettingAdapter.notifyDataSetChanged();
					}
					
				});
				
				iconRight.setOnClickListener(new View.OnClickListener(){

					@Override
					public void onClick(View arg0) {
						// TODO Auto-generated method stub
						if(cnt<suspend_time.length - 1) {
							cnt++;
						} else {
							cnt = 0;
						}
						setScreenTimeoutValue(screen_timeout[cnt]);
						mVideoSettingAdapter.notifyDataSetChanged();
					}
					
				});
				break;
			case WETHER_LOCATION:
				iconLeft.setVisibility(View.INVISIBLE);
				iconRight.setImageResource(R.drawable.right_icon_btn);
				text.setText(R.string.wether_location);
				break;
			case INSTALL_APPLICATION:
				text.setText(R.string.install_application);
				if(allowNonMarketApk)
					state.setText(R.string.allow_install_apk);
				else
					state.setText(R.string.not_allow_install_apk);
				iconLeft.setOnClickListener(new View.OnClickListener(){

					@Override
					public void onClick(View arg0) {
						// TODO Auto-generated method stub
						if(allowNonMarketApk) {
							allowNonMarketApk = false;
						} else {
							allowNonMarketApk = true;
						}
						setNonMarketAppsAllowed(allowNonMarketApk);
						mVideoSettingAdapter.notifyDataSetChanged();
					}
					
				});
				
				iconRight.setOnClickListener(new View.OnClickListener(){

					@Override
					public void onClick(View arg0) {
						// TODO Auto-generated method stub
						if(allowNonMarketApk) {
							allowNonMarketApk = false;
						} else {
							allowNonMarketApk = true;
						}
						setNonMarketAppsAllowed(allowNonMarketApk);
						mVideoSettingAdapter.notifyDataSetChanged();
					}
					
				});
				break;
			case FACTORY_RECOVERY:
				iconLeft.setVisibility(View.INVISIBLE);
				iconRight.setImageResource(R.drawable.right_icon_btn);
				text.setText(R.string.factory_recovery);
				break;
//			case ABOUT_PRODUCT:
//				iconLeft.setVisibility(View.INVISIBLE);
//				text.setText(R.string.about_product);
//				iconRight.setImageResource(R.drawable.right_icon_btn);
//				state.setText(R.string.open);
			case SENIOR_SETTING:
				iconLeft.setVisibility(View.INVISIBLE);
				iconRight.setImageResource(R.drawable.right_icon_btn);
				text.setText(R.string.senior_setting);
				break;
			default:
				break;
			}
			return convertView;
		}
		
	}

	@Override
	public boolean onKey(View v, int keyCode, KeyEvent event) {
		// TODO Auto-generated method stub
		int index = mListView.getSelectedItemPosition();
		if(index<0 || index > mVideoSettingAdapter.getCount()) {
			return false;
		}
		boolean down = event.getAction()==KeyEvent.ACTION_DOWN?true:false;
		boolean left = keyCode==KeyEvent.KEYCODE_DPAD_LEFT?true:false;
		boolean right = keyCode==KeyEvent.KEYCODE_DPAD_RIGHT?true:false;
			switch(index){
			case ENABLE_SUSPEND:
				if(!down){
					if(left){
						if(cnt>0) {
							cnt--;
						} else {
							cnt = suspend_time.length - 1;
						}
					}else if(right){
						if(cnt<suspend_time.length - 1) {
							cnt++;
						} else {
							cnt = 0;
						}
					}
				}
				setScreenTimeoutValue(screen_timeout[cnt]);
				break;
			case WETHER_LOCATION:
				if(!down){
					if(right){
						Intent intent = new Intent();
						intent.setClass(CommonSettings.this, WeatherLocationActivity.class);
						startActivity(intent);	
					}
				}
				break;
			case INSTALL_APPLICATION:
				if(down){
					if(left){
						if(allowNonMarketApk) {
							allowNonMarketApk = false;
						} else {
							allowNonMarketApk = true;
						}
					}else if(right){
						if(allowNonMarketApk) {
							allowNonMarketApk = false;
						} else {
							allowNonMarketApk = true;
						}
					}
					setNonMarketAppsAllowed(allowNonMarketApk);
				}
				break;
			case FACTORY_RECOVERY:
				if(!down){
					if(right){
						startSystemRecovery();
					}
				}
				break;
			case SENIOR_SETTING:
				if(down){
					if(right){
						startSeniorSetting();
					}
				}
				break;
			default:
				break;
			}
			mVideoSettingAdapter.notifyDataSetChanged();
		return false;
	}
	
    public boolean checkPackage(String packageName) {  
        if (packageName == null || "".equals(packageName))  
            return false;  
        try {  
            getPackageManager().getApplicationInfo(  
                    packageName, PackageManager.GET_UNINSTALLED_PACKAGES);  
            return true;  
        } catch (NameNotFoundException e) {  
            return false;  
        }  
    }  
    
	private void startSeniorSetting() {
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

	@Override
	public boolean onKeyDown (int keyCode, KeyEvent event){
		TextView title = (TextView) this.findViewById(R.id.title);
		switch(keyCode){
		case KeyEvent.KEYCODE_BACK:
				title.setText(R.string.setting_common);
			break;
		}
		return false;
	}
	
	@Override
	public boolean onKeyUp (int keyCode, KeyEvent event){
		TextView title = (TextView) this.findViewById(R.id.title);
		switch(keyCode){
		case KeyEvent.KEYCODE_BACK:
				title.setText(R.string.setting_common);
				finish();
			break;
		}
		return false;
	}
	@Override
	public void onItemClick(AdapterView<?> parent, View view, int position,
			long id) {
		// TODO Auto-generated method stub
		switch(position){
		case WETHER_LOCATION:
			Intent intent = new Intent();
			intent.setClass(CommonSettings.this, WeatherLocationActivity.class);
			startActivity(intent);
			break;
		case FACTORY_RECOVERY:
			startSystemRecovery();
			break;
//		case ABOUT_PRODUCT:
//			startAboutSet();
//			break;
		case SENIOR_SETTING:
			startSeniorSetting();
			break;
		}
	}
	
	private int getScreenTimeoutValue(){
		return  Settings.System.getInt(getContentResolver(), SCREEN_OFF_TIMEOUT, -1);
		
	}
	
	private void setScreenTimeoutValue(int value){
		Settings.System.putInt(getContentResolver(), SCREEN_OFF_TIMEOUT, value);
	}
    private boolean isNonMarketAppsAllowed() {
        return Settings.Global.getInt(getContentResolver(),
                                      Settings.Global.INSTALL_NON_MARKET_APPS, 0) > 0;
    }

    private void setNonMarketAppsAllowed(boolean enabled) {
        final UserManager um = (UserManager) getSystemService(Context.USER_SERVICE);
        if (um.hasUserRestriction(UserManager.DISALLOW_INSTALL_UNKNOWN_SOURCES)) {
            return;
        }
        // Change the system setting
        Settings.Global.putInt(getContentResolver(), Settings.Global.INSTALL_NON_MARKET_APPS,
                                enabled ? 1 : 0);
    }
    
	private void startSystemRecovery() {
		// TODO Auto-generated method stub
		
		final SystemRecoveryDialog  srDialog = new SystemRecoveryDialog(mContext,R.style.CommonDialog,getString(R.string.warning),getString(R.string.system_reocvery));
		
		SystemRecoveryDialogInterface srInterface = new SystemRecoveryDialogInterface(){

			@Override
			public void onButtonYesClick() {
				// TODO Auto-generated method stub
				//rebootRecovery();
				mContext.sendBroadcast(new Intent("android.intent.action.MASTER_CLEAR"));
			}

			@Override
			public void onButtonNoClick() {
				// TODO Auto-generated method stub
				srDialog.dismiss();
			}
			
		};
		srDialog.setSystemRecoveryDialogInterface(srInterface);
		srDialog.show();		
	}
}
