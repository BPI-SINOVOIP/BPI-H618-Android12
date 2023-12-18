package com.softwinner.firelauncher.otherwidgets;

import java.io.IOException;

import android.content.Context;
import android.content.res.Resources;
import android.database.Cursor;
import android.database.sqlite.SQLiteDatabase;

import com.softwinner.firelauncher.R;
import com.softwinner.firelauncher.utils.Utils;

public class WeatherUtil {

	private static Context mContext;
	private static WeatherUtil weatherUtil;
	private int[] weatherTxtIdList = {R.string.cloudy, R.string.partly_cloudy, R.string.clear_day, R.string.rain_s,
			R.string.rain_m, R.string.rain_l, R.string.rain_storm_m, R.string.rain_storm_l,	R.string.storm_xl,
			R.string.chance_of_rain_normal, R.string.chance_of_rain_storm, R.string.snow_s,	R.string.snow_m,
			R.string.snow_l, R.string.snow_xl, R.string.icy_sleet, R.string.rain_snow, R.string.windy,
			R.string.windy_storm, R.string.fog};
	
	private int[] weatherIconIdList ={R.drawable.ic_weather_cloudy_l, R.drawable.ic_weather_partly_cloudy_l,
			 R.drawable.ic_weather_clear_day_l, R.drawable.ic_weather_chance_of_rain_l, R.drawable.ic_weather_chance_of_rain_l,
			 R.drawable.ic_weather_chance_of_rain_ml, R.drawable.ic_weather_chance_of_rain_xl, R.drawable.ic_weather_chance_of_rain_xxl,
			 R.drawable.ic_weather_chance_of_rain_xxxl, R.drawable.ic_weather_chance_storm_l, R.drawable.ic_weather_chance_of_rain_xxxl,
			 R.drawable.ic_weather_chance_snow_l, R.drawable.ic_weather_snow_l, R.drawable.ic_weather_snow_ml,
			 R.drawable.ic_weather_snow_ml, R.drawable.ic_weather_icy_sleet_l, R.drawable.ic_weather_snow_rain_xl,
			 R.drawable.ic_weather_windy_l, R.drawable.ic_weather_windy_l, R.drawable.ic_weather_fog_l};

	
	public static WeatherUtil getInstance(Context context) {
		if(null == weatherUtil) {
			weatherUtil = new WeatherUtil();
			mContext = context;
		}
		return weatherUtil;
	}
	
	public int[] getWeaResByWeather(String weather) {
		String[] strs = weather.split("转|到");
		int[] resIds = new int[Math.max(2, strs.length)];
		int j = 0;
		for (int i = 0; i < strs.length; i++) {
			int IconId = mateIconId(strs[i]);
			if(IconId != 0){
				resIds[j] = IconId;
				j++;
			}
//			Log.i("info", "拆分后的天气：" + strs[i]);
		}
		return resIds;
	}

	private int mateIconId(String weather) {
		Resources res = mContext.getResources();
		for (int i = 0; i < weatherTxtIdList.length; i++) {
			if(weather.equals(res.getString(weatherTxtIdList[i])))
				return weatherIconIdList[i];
		}
		return 0;
	}
	
	public String getCityCode(String cityName){
		if(!mContext.getDatabasePath("data.db").exists())
			copyWetherData();
		SQLiteDatabase weatherDb = SQLiteDatabase.openDatabase(
				mContext.getDatabasePath("data.db").toString(), null, SQLiteDatabase.OPEN_READONLY);
		// 查询城市编号
		Cursor cursor = null;
		cursor = weatherDb.query("city_code", new String[] { "code" },
				"name=?", new String[] { cityName }, null, null, null);
		if (cursor.getCount() > 0 && cursor.moveToFirst()) {
			String citycode = cursor.getString(cursor.getColumnIndex("code"));
			cursor.close();
			weatherDb.close();
			return citycode;
		}
		return null;
	}
	
	private void copyWetherData() {
		try {
			Utils.writeFile(mContext.openOrCreateDatabase("data.db", 0, null).getPath(), 
					mContext.getAssets().open("data.db"));
		} catch (IOException e) {
			e.printStackTrace();
		}
	}

}
