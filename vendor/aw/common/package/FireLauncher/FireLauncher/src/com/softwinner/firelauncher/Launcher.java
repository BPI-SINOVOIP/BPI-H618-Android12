package com.softwinner.firelauncher;

import java.lang.reflect.Method;
import android.annotation.SuppressLint;
import android.app.Activity;
import android.bluetooth.BluetoothAdapter;
import android.content.BroadcastReceiver;
import android.content.ComponentName;
import android.content.Context;
import android.content.Intent;
import android.content.IntentFilter;
import android.content.res.Configuration;
import android.net.ConnectivityManager;
import android.net.NetworkInfo;
import android.net.NetworkInfo.State;
import android.net.wifi.WifiManager;
import android.os.Bundle;
import android.os.Environment;
import android.os.Handler;
import android.os.StrictMode;
import android.os.storage.StorageManager;
import android.text.TextUtils;
import android.util.Log;
import android.view.KeyEvent;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup.MarginLayoutParams;
import android.view.animation.AlphaAnimation;
import android.view.animation.Animation;
import android.view.animation.AnimationUtils;
import android.view.animation.Animation.AnimationListener;
import android.widget.ImageView;
import android.widget.LinearLayout;
import android.widget.TextView;
import android.widget.Toast;

import com.android.launcher2.FocusOnlyTabWidget;
import com.softwinner.firelauncher.network.HttpJsonLoader;
import com.softwinner.firelauncher.network.HttpJsonLoader.onJsonLoaderFinishListener;
import com.softwinner.firelauncher.otherwidgets.LauncherDialog;
import com.softwinner.firelauncher.otherwidgets.NotificationReceiver;
import com.softwinner.firelauncher.otherwidgets.WeatherUtil;
import com.softwinner.firelauncher.LauncherApplication;
import com.softwinner.firelauncher.utils.AppConfig;
import com.softwinner.firelauncher.utils.Utils;
import com.softwinner.fireplayer.remotemedia.returnitem.ReturnWeatherInfo.WeatherInfo;

public class Launcher extends Activity implements onJsonLoaderFinishListener{

	static final String TAG = "Launcher";
	private boolean DEBUG = true;

	static final boolean PROFILE_STARTUP = false;
	static final boolean DEBUG_STRICT_MODE = false;
	public static final int LOAD_WEATHER_INFO = 666665;
	public static final int REFRESH_WEATHER_UI = 666666;
	public static final int REFRESH_WEATHER_CITY = 666667;
	public static final int REFRESH_SD_USB_STATE = 666668;
	public static final String LAUNCHER_FIRST_START_FLAG = "com.softwinner.tvdsetting.WifiSettingActivity.FIRST_START_FLAG";
	public static final String WeacherInfo = "com.softwinner.firelauncher.otherwidgets.MyWeacherHttUtil";
	public static final String LOCATION_CHANGE = "com.softwinner.tvdsetting.LOCATION_CHANGE";
	public static final String NOTIFICASTION_EXPAND = "com.softwinner.systemui.expand";
	public static final String[] MNT_USB_PATH = {"/mnt/usbhost"};//TODO:可添加监听的设备节点
	public static final String[] MNT_SD_PATH = {"/mnt/extsd"};
	public static final String SINA_GET_LOCAL_INFO_URL = "http://int.dpool.sina.com.cn/iplookup/iplookup.php?format=json&ip=";
	public static String weacherResult = "";
	public static int ONRESUME = 1;
	public static int ONCREATE = 2;
	public static int ONPAUSE = 3;
	public static int ONDESTROY = 4;
	private static LauncherDialog mDialog;
	private final long INTERVAL = 5*1000;
	private int ActivityState = 0;// 0:初始状态  1:onResume 2:onCreate 3:onPause 4:onDestroy
	private int clickDelayTime;
	private LauncherTabHost tabHost;
	private FocusOnlyTabWidget tabWidget;
	private AppConfig mConfig;
	private HttpJsonLoader mWeatherLoader;
	private HttpJsonLoader mCityLoader;
	private String cityCode = null;

	// OtherWidgets
	public static View LauncherView;
	public static View other_info_view;
	private TextView mTimeClock;
	private TextView weatherCity;
	private ImageView weatherIcon1;
	private ImageView weatherIcon2;
	private TextView weatherInfo;
	private ImageView WifiStatus;
	private ImageView imageUSB;
	private ImageView imageSD;
	private ImageView mBluetoothStatus;
	private BroadcastReceiver launcherStateReceiver = null;
	private String info = null;
	private String temp = null;
	private boolean isLoaded = false;
	private boolean isCityLoaded = false;
	
	private int connect_state = LauncherDialog.CONNECT_NO;
	private int lastKeyAction = KeyEvent.ACTION_UP;
	private long onKeyDown_lastTime = 0;
    
	@SuppressLint("HandlerLeak")
	Handler mHandler = new Handler() {
		public void handleMessage(android.os.Message msg) {
			switch (msg.what) {
			case WifiManager.WIFI_STATE_ENABLED:
			case WifiManager.WIFI_STATE_DISABLED:
				// 没有连接到wifi，更新UI
				NetworkConnect();
				break;
			case LOAD_WEATHER_INFO:
				if(!isLoaded) {
					if(Utils.isNetworkConnected(Launcher.this)) {
						String myCode = null;
						if(cityCode.equals(AppConfig.UNKNOW_CITYCODE)){
							myCode = AppConfig.DEFAULT_CITYCODE;
						}else{
							myCode = cityCode;
						}
						mWeatherLoader.loadUrl(Utils.appendWeatherUrl(myCode));
					} else {
						Log.w(TAG, "no network, no load");
					}
					removeMessages(LOAD_WEATHER_INFO);
					sendEmptyMessageDelayed(LOAD_WEATHER_INFO, INTERVAL);
				}
				break;
			case REFRESH_WEATHER_CITY:
				if(!isCityLoaded) {
					if(Utils.isNetworkConnected(Launcher.this)) {
						mCityLoader.loadUrl(SINA_GET_LOCAL_INFO_URL);
						Log.d(TAG, "mCityLoader.loadUrl(SINA_GET_LOCAL_INFO_URL)");
					} else {
						Log.w(TAG, "no network, no load city");
					}
					removeMessages(REFRESH_WEATHER_CITY);
					sendEmptyMessageDelayed(REFRESH_WEATHER_CITY, INTERVAL);
				}
				break;
			case REFRESH_WEATHER_UI:
				// 获取到天气，更新UI
				isLoaded = true;
				removeMessages(LOAD_WEATHER_INFO);
				refreshWeacherUI();
				break;
				
			case REFRESH_SD_USB_STATE:
				//有些设备开机硬盘事件起来特别慢，延时5秒再刷新一次
				if(imageSD != null){
					imageSD.setVisibility(isMntStorage(MNT_SD_PATH)?View.VISIBLE:View.GONE);
				}
				if(imageUSB != null){
					imageUSB.setVisibility(isMntStorage(MNT_USB_PATH)?View.VISIBLE:View.GONE);
				}
				break;

			default:
				break;
			}
		};
	};

	@Override
	protected void onCreate(Bundle savedInstanceState) {
		ActivityState = ONCREATE;
		if (DEBUG_STRICT_MODE) {
			StrictMode.setThreadPolicy(new StrictMode.ThreadPolicy.Builder()
					.detectDiskReads()
					.detectDiskWrites()
					.detectNetwork() // or .detectAll() for all detectable problems
					.penaltyLog()
					.build());
			StrictMode.setVmPolicy(new StrictMode.VmPolicy.Builder()
					.detectLeakedSqlLiteObjects()
					.detectLeakedClosableObjects()
					.penaltyLog()
					.penaltyDeath()
					.build());
		}

		LauncherApplication.getInstance().setCurrentTime(System.currentTimeMillis());
		mConfig = AppConfig.getInstance(this);

		if (mConfig.isFirstTime()) {
			Log.d(TAG, "first time");
			Intent it = new Intent();
			it.putExtra(LAUNCHER_FIRST_START_FLAG, true);
			it.setComponent(new ComponentName("com.softwinner.tvdsetting",
					"com.softwinner.tvdsetting.net.wifi.WifiSettingActivity"));
			startActivity(it);
		}
		super.onCreate(savedInstanceState);
		//检测系统设置字体大小
		if(getResources().getDisplayMetrics().scaledDensity > 1.15f){
			getResources().getDisplayMetrics().scaledDensity = 1.15f;
		}
		// UmengUpdateAgent.update(this);
		if (PROFILE_STARTUP) {
			android.os.Debug.startMethodTracing(Environment.getExternalStorageDirectory() + "/firelauncher");
		}
		clickDelayTime = getResources().getInteger(R.integer.default_long_click_delay);
		// process that need to measure
		setupViews();
		if (PROFILE_STARTUP) {
			android.os.Debug.stopMethodTracing();
		}
		
		/*//H3  Build.MODEL == dolphin;  A80 Build.MODEL == jaws;   H8  Build.MODEL == eagle
		String device_model = Build.MODEL; // 设备型号   
		//H3 去掉天气文字动画
		if (null!=device_model&&"dolphin".equals(device_model)){
			((LauncherApplication) this.getApplicationContext()).setmWeatherAnimation(false);
			animation = false;
		}*/
	}

	@Override
	protected void onDestroy() {
		ActivityState = ONDESTROY;
		tabHost.onDestroy();
		if (launcherStateReceiver != null) {
			unregisterReceiver(launcherStateReceiver);
			launcherStateReceiver = null;
		}
		super.onDestroy();
	}

	@Override
	protected void onResume() {
		ActivityState = ONRESUME;
		super.onResume();
		tabHost.onResume();
		registerLauncherStateReceiver();

		// MobclickAgent.setDebugMode( true );
		// MobclickAgent.onResume(this);
	}

	@Override
	protected void onPause() {
		ActivityState = ONPAUSE;
		super.onPause();
		tabHost.onPause();
		// MobclickAgent.onPause(this);
	}
	
	@Override
	public void onBackPressed() {
		// 说明：注销返回键事件
		// super.onBackPressed();
	}
	
	@Override
	public void onConfigurationChanged(Configuration newConfig) {
		super.onConfigurationChanged(newConfig);
		resetTab();
	}

	@Override
	public boolean onKeyDown(int keyCode, KeyEvent event) {
		boolean ret = true;
		long current = System.currentTimeMillis();
		int keyAction = event.getAction();
		// Log.d(TAG, "onKeyDown_onKey:keyCode="+keyCode+"  KeyEvent="+event);
		// if(event.getKeyCode() == KeyEvent.KEYCODE_MENU && event.getAction() == KeyEvent.ACTION_DOWN) {
		// Log.d(TAG, "left message dialog ");
		// //显示左部对话框
		// showDialogLeft();
		// return true;
		// }
		if (isIgnoreKeyCode(keyCode, keyAction, current)) {
			if (DEBUG)
				Log.d(TAG, "OnLongClickDelta=" + (current - onKeyDown_lastTime)	+ " || ignored event=" + event);
			ret = true; // consume KeyEvent
		} else {
			if (DEBUG)
				Log.d(TAG, "super.dispatchKeyEvent: delta "	+ (current - onKeyDown_lastTime) + " || event=" + event);
			onKeyDown_lastTime = (keyAction == lastKeyAction ? current : 0);
			lastKeyAction = keyAction;
			if (tabWidget.hasFocus() && keyCode == KeyEvent.KEYCODE_DPAD_UP	&& keyAction == KeyEvent.ACTION_DOWN) {
				Log.d(TAG, "up message dialog");
				showDialogTop(connect_state, NotificationReceiver.notificationCount, 
						isMntStorage(MNT_USB_PATH), isMntStorage(MNT_SD_PATH));
				ret = true;
			} else {
				ret = super.onKeyDown(keyCode, event);
			}
		}
		return ret;
	}

	private boolean isIgnoreKeyCode(int keyCode, int keyAction, long current) {
		switch (keyCode) {
		case KeyEvent.KEYCODE_DPAD_DOWN:
		case KeyEvent.KEYCODE_DPAD_UP:
		case KeyEvent.KEYCODE_DPAD_RIGHT:
		case KeyEvent.KEYCODE_DPAD_LEFT:
			return (keyAction == lastKeyAction && current - onKeyDown_lastTime < clickDelayTime);
		default:
			return false;
		}
	}
	
	private void resetTab(){
		if(getResources().getDisplayMetrics().scaledDensity > 1.15f){
			getResources().getDisplayMetrics().scaledDensity = 1.15f;
		}
		int width = (int)(getResources().getDimension(R.dimen.tabindicator_width));
		int height = (int) (getResources().getDimension(R.dimen.tabindicator_height)*
				getResources().getDisplayMetrics().scaledDensity);
		tabHost.updateFont();
		LinearLayout.LayoutParams tab_layoutparams = new LinearLayout.LayoutParams(MarginLayoutParams.WRAP_CONTENT, height);
		for(int i=0; i<tabWidget.getChildCount(); i++) {
			tabWidget.getChildAt(i).setLayoutParams(tab_layoutparams);
			TextView name = (TextView) tabWidget.getChildAt(i).findViewById(R.id.tabindicator_name);
			name.setTextSize(Utils.px2dip(this, getResources().getDimension(R.dimen.tabindicator_text_size_normal)));
			if(i == tabWidget.getSelectedTabIndex()) {
				if(name.getAnimation() != null)continue;
				Animation tabScaleAnimation = AnimationUtils.loadAnimation(this, R.anim.tab_scale_anim);  
				tabScaleAnimation.setFillAfter(true);
				name.startAnimation(tabScaleAnimation);
			} else {
				name.clearAnimation();
			}
		}
	}

	private void setupViews() {
		LayoutInflater inflater = LayoutInflater.from(this);
		//TODO:加载不同的布局，更改主界面的section页面项
		if(!AppConfig.getInstance(this).isEnableGame())
			LauncherView = inflater.inflate(R.layout.launcher_no_game, null);
		else
			LauncherView = inflater.inflate(R.layout.launcher_all, null);
		setContentView(LauncherView);
		other_info_view = (View) findViewById(R.id.other_info);
		
		weatherCity = (TextView) findViewById(R.id.top_weather_city);
		weatherIcon1 = (ImageView) findViewById(R.id.top_weather_icon_1);
		weatherIcon2 = (ImageView) findViewById(R.id.top_weather_icon_2);
		weatherInfo = (TextView) findViewById(R.id.top_weather_info);
		cityCode = mConfig.getSettingCityCode();
		if(cityCode.equals(AppConfig.UNKNOW_CITYCODE)){
			mHandler.sendEmptyMessage(REFRESH_WEATHER_CITY);
		}
		mWeatherLoader = new HttpJsonLoader(this, HttpJsonLoader.JSON_WEATHER);
		mWeatherLoader.setJsonLoaderFinishListener(this);
		mCityLoader = new HttpJsonLoader(this, HttpJsonLoader.JSON_GET_CITY);
		mCityLoader.setJsonLoaderFinishListener(this);
		mHandler.sendEmptyMessage(LOAD_WEATHER_INFO);
		
		mTimeClock = (TextView) findViewById(R.id.top_timeclock);
		setCurrtime();
		
		WifiStatus = (ImageView) findViewById(R.id.WifiStatus);
		imageSD = (ImageView) findViewById(R.id.imageSD);
		imageSD.setVisibility(isMntStorage(MNT_SD_PATH)?View.VISIBLE:View.GONE);
		imageUSB = (ImageView) findViewById(R.id.imageUSB);
		imageUSB.setVisibility(isMntStorage(MNT_USB_PATH)?View.VISIBLE:View.GONE);
		mHandler.sendEmptyMessageDelayed(REFRESH_SD_USB_STATE,INTERVAL);
		mBluetoothStatus = (ImageView) findViewById(R.id.BluetoothStatus);
		tabHost = (LauncherTabHost) findViewById(R.id.main_content_tabhost);
		tabWidget = (FocusOnlyTabWidget) tabHost.getTabWidget();
	}
	
	private void setCurrtime(){
		String time = Utils.getCurrTime(this);
		if(time != null && !time.equals("")){
	        mTimeClock.setText(time);
			//定时更新天气预报
			if(time.equals("12:00") || time.equals("00:00")){
				isLoaded = false;
				mHandler.sendEmptyMessage(LOAD_WEATHER_INFO);
			}
		}
	}
	
	private void NetworkHandler() {
		NetworkConnect();
		if (connect_state != LauncherDialog.CONNECT_NO) {
			if (mDialog != null && mDialog.isShow()) {
				mDialog.dismiss();
			}
		} else {
			if(isLoaded)
				showDialogBottom();
			else{
				mHandler.postDelayed(new Runnable() {
					@Override
					public void run() {
						if(connect_state == LauncherDialog.CONNECT_NO)
							showDialogBottom();
					}
				}, 16000);
			}
		}
	}
	
	private int NetworkConnect() {
		ConnectivityManager mConnectivityManager = (ConnectivityManager) getSystemService(Context.CONNECTIVITY_SERVICE);
		NetworkInfo mNetworkInfo = mConnectivityManager.getActiveNetworkInfo();
		if (mNetworkInfo != null && mNetworkInfo.isAvailable()) {
			NetworkInfo networkInfoWifi = mConnectivityManager.getNetworkInfo(ConnectivityManager.TYPE_WIFI);
			NetworkInfo networkInfoEth = mConnectivityManager.getNetworkInfo(ConnectivityManager.TYPE_ETHERNET);

			if (networkInfoEth.getState() == State.CONNECTED || networkInfoEth.getState() == State.CONNECTING) {
				WifiStatus.setImageResource(R.drawable.icon_ethernet);
				connect_state = LauncherDialog.CONNECT_ETHERNET;
			} else if (networkInfoWifi.getState() == State.CONNECTED || networkInfoWifi.getState() == State.CONNECTING) {
				setWifiIcon();
				connect_state = LauncherDialog.CONNECT_WIFI;
			} else {
				WifiStatus.setImageResource(R.drawable.icon_wifi_no);
				connect_state = LauncherDialog.CONNECT_NO;
			}
		}else{
			WifiStatus.setImageResource(R.drawable.icon_wifi_no);
			connect_state = LauncherDialog.CONNECT_NO;
		}
		return connect_state;
	}

	private void registerLauncherStateReceiver() {
		launcherStateReceiver = new BroadcastReceiver() {

			@Override
			public void onReceive(Context context, Intent intent) {
				String action = intent.getAction();
				//TODO:添加响应action的动作
				if (Intent.ACTION_TIME_TICK.equals(action) || 
						Intent.ACTION_TIME_CHANGED.equals(action)) {
					setCurrtime();
				}else if(Intent.ACTION_CLOSE_SYSTEM_DIALOGS.equals(action)){
					if (ActivityState == ONPAUSE) return;
					String reason = intent.getStringExtra("reason");
					if (TextUtils.equals(reason, "homekey")) tabHost.setSelectPage(0);
				}else if(ConnectivityManager.CONNECTIVITY_ACTION.equals(action) ||
						WifiManager.WIFI_STATE_CHANGED_ACTION.equals(action)){
//					if(ConnectivityManager.CONNECTIVITY_ACTION.equals(action)){
//				        if (intent.getBooleanExtra(ConnectivityManager.EXTRA_NO_CONNECTIVITY, false)) 
//				            Log.e(TAG, "netWork has lost");  
//				        NetworkInfo tmpInfo = (NetworkInfo) intent.getExtras().get(ConnectivityManager.EXTRA_NETWORK_INFO);  
//				        Log.e(TAG, tmpInfo.toString() + " {isConnected = " + tmpInfo.isConnected() + "}");  
//					}else{
//						int wifistate = intent.getIntExtra(WifiManager.EXTRA_WIFI_STATE, WifiManager.WIFI_STATE_UNKNOWN);
//					}
					NetworkHandler();
				}else if(BluetoothAdapter.ACTION_STATE_CHANGED.equals(action)){
					BluetoothStatusChanged();
				}else if(Intent.ACTION_MEDIA_MOUNTED.equals(action)){
					String path = intent.getData().toString();
					if(path.contains("usbhost")){
						USBChange(true);
					}else if(path.contains("extsd")){
						SDChange(true);
					}
				}else if(Intent.ACTION_MEDIA_REMOVED.equals(action)){
					String path = intent.getData().toString();
					if(path.contains("usbhost")){
						USBChange(false);
					}else if(path.contains("extsd")){
						SDChange(false);
					}
				}else if(NOTIFICASTION_EXPAND.equals(action) && 
						intent.getBooleanExtra("expand", false)){
					if (mDialog.isShow()) mDialog.dismiss();
				}else if(LOCATION_CHANGE.equals(action)){
					String cityName = intent.getStringExtra("city");
					String code = WeatherUtil.getInstance(Launcher.this).getCityCode(cityName);
					if(code == null) return; 
					cityCode = code;
					mConfig.setSettingCityCode(cityCode);
					isLoaded = false;
					mHandler.sendEmptyMessage(LOAD_WEATHER_INFO);
				}
			}
		};
		
		//TODO:添加广播监听action
		IntentFilter intenFilter = new IntentFilter(Intent.ACTION_TIME_TICK);
		intenFilter.addAction(Intent.ACTION_TIME_CHANGED);
		intenFilter.addAction(Intent.ACTION_CLOSE_SYSTEM_DIALOGS);
		intenFilter.addAction(ConnectivityManager.CONNECTIVITY_ACTION);
		intenFilter.addAction(WifiManager.WIFI_STATE_CHANGED_ACTION);
		intenFilter.addAction(BluetoothAdapter.ACTION_STATE_CHANGED);
		intenFilter.addAction(NOTIFICASTION_EXPAND);
		intenFilter.addAction(LOCATION_CHANGE);
		
		registerReceiver(launcherStateReceiver, intenFilter);
		
		//需要提供特殊权限或优先级的action监听
		IntentFilter mediaDeviceIntent = new IntentFilter(Intent.ACTION_MEDIA_MOUNTED);
		mediaDeviceIntent.addAction(Intent.ACTION_MEDIA_REMOVED);
		mediaDeviceIntent.setPriority(1000);//设置优先级
		mediaDeviceIntent.addDataScheme("file");
		registerReceiver(launcherStateReceiver, mediaDeviceIntent);
	}
	
	private void USBChange(boolean state) {
		LayoutInflater.from(Launcher.this).inflate(R.layout.launcher_dialog_layout, null);
		showDialogRigth(LauncherDialog.USB_SHOW, state);
		if(isMntStorage(MNT_USB_PATH)){
			imageUSB.setVisibility(View.VISIBLE);
		}else{
			imageUSB.setVisibility(View.GONE);
			if(state)
				Toast.makeText(this, R.string.media_unmount, Toast.LENGTH_LONG).show();
		}
	}

	private void SDChange(boolean state) {
		LayoutInflater.from(Launcher.this).inflate(R.layout.launcher_dialog_layout, null);
//		imageSD.setVisibility(isMntStorage(MNT_SD_PATH)? View.VISIBLE:View.GONE);
		showDialogRigth(LauncherDialog.SD_SHOW, state);
		if(isMntStorage(MNT_SD_PATH)){
			imageSD.setVisibility(View.VISIBLE);
		}else{
			imageSD.setVisibility(View.GONE);
			if(state)
				Toast.makeText(this, R.string.media_unmount, Toast.LENGTH_LONG).show();
		}
	}
	
	private void BluetoothStatusChanged() {
		BluetoothAdapter adapter = BluetoothAdapter.getDefaultAdapter();
		mBluetoothStatus.setVisibility(adapter != null && adapter.isEnabled()? View.VISIBLE:View.GONE);
	}
	
	private boolean isMntStorage(String[] mountPoints){
		try {
			StorageManager storageManager = (StorageManager) getSystemService(Context.STORAGE_SERVICE);
	        Method getVolumePaths = StorageManager.class.getDeclaredMethod("getVolumePaths");
	        getVolumePaths.setAccessible(true);
	        Object result = getVolumePaths.invoke(storageManager);
	        if (result != null && result instanceof String[]) {
	            String[] pathes = (String[]) result;
	            for (String path : pathes) {
	            	for (String mountPoint : mountPoints) {
	                	if(!path.contains(mountPoint)) continue;
//                		StatFs statFs = new StatFs(path);
//	                    if (statFs.getBlockCount() * statFs.getBlockSize() != 0) {
//	                    	return true;
//	                	}
	                    Method getVolumeState = StorageManager.class.getDeclaredMethod("getVolumeState",String.class);
	                    getVolumeState.setAccessible(true); 
		             	String state = (String) getVolumeState.invoke(storageManager, path);
		                if (state != null && state.equals(Environment.MEDIA_MOUNTED)) {
		                	return true;
		            	}
					}
	            }
	        }
		} catch (Exception e) {
			e.printStackTrace();
		}
		return false;
	}
	
	private void refreshWeacherUI() {
		WeatherInfo mWeatherInfo = mWeatherLoader.getWeatherInfo();
		info = mWeatherInfo.weather;
		temp = mWeatherInfo.temp1 + "~" + mWeatherInfo.temp2;
		//updata weacher widget
		weatherCity.setText(mWeatherInfo.city);
		weatherInfo.setText(info);
		int[] ids = WeatherUtil.getInstance(this).getWeaResByWeather(info);
		weatherIcon1.setVisibility(ids[0]!= 0? View.VISIBLE:View.GONE);
		if(ids[0] == 0 && ids[1] == 0){
			Log.d(TAG, "info= "+ info + "temp =" + temp);
			isLoaded = false;
			mHandler.sendEmptyMessage(LOAD_WEATHER_INFO);  
			info = "";
			temp = "";
			weatherCity.setText(R.string.weather_info_error);
		}
		if (ids[0] != 0) {
			weatherIcon1.setImageResource(ids[0]);
			weatherIcon1.setVisibility(View.VISIBLE);
		} else {
			weatherIcon1.setVisibility(View.GONE);
		}
		if (ids[1] != 0) {
			weatherIcon2.setImageResource(ids[1]);
			weatherIcon2.setVisibility(View.VISIBLE);
		} else if (weatherIcon2 != null) {
			weatherIcon2.setVisibility(View.GONE);
		}
		setLoopAnimation();
	}
	
	private void setLoopAnimation(){
		final AlphaAnimation anim = new AlphaAnimation(0, 1);
		anim.setDuration(1000);
		anim.setAnimationListener(new AnimationListener() {
			private boolean isInfo = true;
			@Override
			public void onAnimationStart(Animation animation) {
				if(!isInfo){
					weatherInfo.setText(info);
					isInfo = true;
				}else{
					weatherInfo.setText(temp);
					isInfo = false;
				}
			}
			
			@Override
			public void onAnimationRepeat(Animation animation) {
				// TODO Auto-generated method stub

			}
			
			@Override
			public void onAnimationEnd(final Animation animation) {
				mHandler.postDelayed(new Runnable() {
					@Override
					public void run() {
						weatherInfo.startAnimation(animation);
					}
				}, 15000);
			}
		});
		weatherInfo.startAnimation(anim);
	}

	private void setWifiIcon() {
		int level = Math.abs(((WifiManager) getSystemService(WIFI_SERVICE)).getConnectionInfo().getRssi());
		WifiStatus.setImageResource(R.drawable.wifi_sel);
		WifiStatus.setImageLevel(level);
	}
	
	private void resetDialog(){
		if(mDialog == null)	mDialog = LauncherDialog.getInstance(this);
		if (mDialog.isShow()) mDialog.dismiss();
	}
	
	private void showDialogTop(int connectState, int notifCount, boolean usbState, boolean sdState){
		if(mDialog == null || mDialog.getShowPosition() != LauncherDialog.TOP){
			resetDialog();
			mDialog.showDialogTop(connectState, notifCount, usbState, sdState);
		}
	}
	
	private void showDialogBottom(){
		if(mDialog == null || mDialog.getShowPosition() != LauncherDialog.BOTTOM){
			resetDialog();
			mDialog.showDialogBottom();
		}
	}
	
	private void showDialogRigth(int contentId, boolean state){
		if(mDialog == null ||  mDialog.getShowPosition() != LauncherDialog.RIGHT){
			resetDialog();
			mDialog.showDialogRigth(contentId, state);
		}
	}
	
//	private void showDialogLeft(){
//		if(mDialog == null || mDialog.getShowPosition() != LauncherDialog.LEFT){
//			resetDialog();
//			mDialog.showDialogLeft();
//		}
//	}

	@Override
	public void onJsonLoaderFinish(int type, int addedSize) {
		if(HttpJsonLoader.JSON_WEATHER == type && mWeatherLoader.getSize() > 0 ) {
			mHandler.sendEmptyMessage(REFRESH_WEATHER_UI);
		}
		if(HttpJsonLoader.JSON_GET_CITY == type && mCityLoader.getSize() > 0 ) {
			String cityName = mCityLoader.getCityInfo().city;
			String code = WeatherUtil.getInstance(Launcher.this).getCityCode(cityName);
			if(code == null) return; 
			cityCode = code;
			mConfig.setSettingCityCode(cityCode);
			isLoaded = false;
			isCityLoaded = true;
			mHandler.sendEmptyMessage(LOAD_WEATHER_INFO);
		}
	}
	
	@Override
	public void onNetworkError(String message) {
		// TODO Auto-generated method stub
		
	}

}
