package com.softwinner.firelauncher.utils;

import com.softwinner.firelauncher.LauncherApplication;
import com.softwinner.firelauncher.R;
import android.content.Context;
import android.content.SharedPreferences;
import android.content.SharedPreferences.Editor;
import android.preference.PreferenceManager;

public class AppConfig {
	private static AppConfig mInstance;
	private final Context mContext;
	private final SharedPreferences mConfigs;
	private final Editor mEditor; 
	
	public static final String TAG = "AppConfig";
	
	public static final String FIRST_TIME = "first_time";
	public static final String USE_UPDATE_UMENG = "use_update_umeng";
	public static final String SETTING_CITY_NAME = "setting_city_name";
	public static int mChipVersion;
	private static boolean mForceUpdate = false; 
	
	public static final String MANGO_RECOMMEND_IMG_BASE = "mango_recommend_img_url";
	public static final String MANGO_RECOMMEND_TITLE_BASE = "mango_recommend_title";
	public static final String RECOMMEND_ROLL_IMG_BASE = "recommend_roll_img_url";
	
	public static final String GAME_RECOMMEND_IMG_BASE = "game_recommend_img_url";
	public static final String GAME_RECOMMEND_TITLE_BASE = "game_recommend_title";
	
	public static final String XXX_CHANNEL_IMG_BASE = "xxx_channel_img_url";
	public static final String GAME_SECTION_ISENABLE = "game_section_isable";
	
	public static String UNKNOW_CITYCODE = "unknow";
	public static String DEFAULT_CITYCODE = "101010100";
	
	public static AppConfig getInstance(Context context) {
		if(mInstance == null) {
			mInstance = new AppConfig(context);
		}
		return mInstance;
	}
	
	private AppConfig(Context context) {
		mContext = context.getApplicationContext();
		mConfigs = PreferenceManager.getDefaultSharedPreferences(mContext);
		mEditor = mConfigs.edit();
//		initialize();
	}
	
//	private void initialize() {
////		extCfgParse();
////		mChipVersion = getInt(CHIP_VERSION, 31);
////		//mFirmwareVersion = getInt(FIRMWARE_VERSION, 10);
////		setAppMode(getInt(APP_MODE, APP_MODE_OPT));
////		bAthumbEnable = getBoolean(ATHUMB_ENABLE, true);
//	}
		
	public boolean isFirstTime() {
		 boolean ret = getBoolean(AppConfig.FIRST_TIME,  true);
		 setBoolean(AppConfig.FIRST_TIME, false);
		 
		 if(ret)
			setBoolean(GAME_SECTION_ISENABLE, getSystemGameProperty());
		 return ret;
	}
	
	private boolean getSystemGameProperty(){
		int v = ReflectionCall.getInstance(mContext).getInt("android.os.SystemProperties", 
				"getInt", "ro.business.game", 0);
		return v==1;
	}

	/**
	 * @return true:显示游戏  false：不显示游戏*/
	public boolean isEnableGame(){
		return getBoolean(GAME_SECTION_ISENABLE, false);
	}
	
	public boolean useUmengUpdate() {
		return getBoolean(AppConfig.USE_UPDATE_UMENG,  true);
	}
	
	public void setUmengUpdate(boolean v) {
		setBoolean(AppConfig.USE_UPDATE_UMENG,  v);
	}
	
	public static boolean isForceUpdate() {
		return mForceUpdate;
	}
	
	public static void setForceUpdate(boolean force) {
		mForceUpdate = force;
	}
	
	public void setSettingCityCode(String cityCode) {
		setString(SETTING_CITY_NAME, cityCode);
	}
	
	public String getSettingCityCode() {
		return getString(SETTING_CITY_NAME, UNKNOW_CITYCODE);//????????????��???
	}

	public void setRecommendURL(int index, String url) {
		setString(MANGO_RECOMMEND_IMG_BASE+index, url);
	}
	
	public String getRecommendURL(int index) {
		return getString(MANGO_RECOMMEND_IMG_BASE+index, "");
	}
	
	public void setRecommendRollURL(String url) {
		setString(RECOMMEND_ROLL_IMG_BASE, url);
	}
	
	public String getRecommendRollURL() {
		return getString(RECOMMEND_ROLL_IMG_BASE, "");
	}
	
	public void setRecommendTitle(int index, String title){
		setString(MANGO_RECOMMEND_TITLE_BASE+index, title);
	}
	
	public String getRecommendTitle(int index) {
		String def = LauncherApplication.getInstance().getResources().getString(R.string.title_no_network);
		return getString(MANGO_RECOMMEND_TITLE_BASE+index, def);
	}
	
	public void setGameImageURL(int index, String url) {
		setString(GAME_RECOMMEND_IMG_BASE+index, url);
	}
	
	public String getGameImageURL(int index) {
		return getString(GAME_RECOMMEND_IMG_BASE+index, "");
	}
	
	public void setGameTitle(int index, String title){
		setString(GAME_RECOMMEND_TITLE_BASE+index, title);
	}
	
	public String getGameTitle(int index) {
		String def = LauncherApplication.getInstance().getResources().getString(R.string.title_no_network);
		return getString(GAME_RECOMMEND_TITLE_BASE+index, def);
	}
	
	public void setChannelImgURL(int index, String url) {
		setString(XXX_CHANNEL_IMG_BASE+index, url);
	}
	public String getChannelImgURL(int index) {
		return getString(XXX_CHANNEL_IMG_BASE+index, "");
	}
	
	
	public boolean getBoolean(String key, boolean def) {
		return mConfigs.getBoolean(key, def);
	}
	
	public void setBoolean(String key, boolean value) {
		mEditor.putBoolean(key, value);
		mEditor.commit();
	}
	
	public int getInt(String key, int def) {
		return mConfigs.getInt(key, def);
	}
	
	public void setInt(String key, int value) {
		mEditor.putInt(key, value);
		mEditor.commit();
	}	
	
	public String getString(String key, String def) {
		return mConfigs.getString(key, def);
	}

	public void setString(String key, String value) {
		mEditor.putString(key, value);
		mEditor.commit();
	}

	public Float getFloat(String key, float def) {
		return mConfigs.getFloat(key, def);
	}

	public void setFloat(String key, float value) {
		mEditor.putFloat(key, value);
		mEditor.commit();
	}

	public long getLong(String key, Long def) {
		return mConfigs.getLong(key, def);
	}

	public void setLong(String key, long value) {
		mEditor.putLong(key, value);
		mEditor.commit();
	}
	
}