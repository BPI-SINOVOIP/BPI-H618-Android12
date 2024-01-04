package com.softwinner.tvdsetting.weather;

import java.util.ArrayList;
import java.util.List;

import org.json.JSONException;
import org.json.JSONObject;

import com.softwinner.tvdsetting.R;
import com.softwinner.tvdsetting.widget.WheelView;
import android.app.Activity;
import android.app.ProgressDialog;
import android.content.Context;
import android.content.Intent;
import android.content.SharedPreferences;
import android.content.res.Resources;
import android.database.Cursor;
import android.database.sqlite.SQLiteDatabase;
import android.os.Bundle;
import android.os.Handler;
import android.os.Message;
import android.util.Log;
import android.view.View;
import android.widget.Button;
import android.widget.ImageView;
import android.widget.LinearLayout;
import android.widget.ProgressBar;
import android.widget.TextView;
import android.widget.Toast;

public class WeatherLocationActivity extends Activity implements
		View.OnClickListener, WheelView.WheelViewInterface {

	private Context mContext;
	private WheelView mPWheelView, mCWheelView, mAWheelView;
	private Button mPButton, mCButton, mAButton;
	private TextView mText1;
	private ImageView mImg1;
	private ProgressBar refreshpBar;
	private List<String> mProvince = new ArrayList<String>();
	private List<String> mCity = new ArrayList<String>();
	private List<String> mArea = new ArrayList<String>();
	private List<String> mProvinceId = new ArrayList<String>();
	private List<String> mCityID = new ArrayList<String>();
	private String mCurrentArea;
	private String mCurrentCity;
	private String mCurrentProvince;
	CityCodeDB citycodedb;
	SQLiteDatabase database;

	private LinearLayout city_choose,weather_message;
	private static final String LOCATION_INFO = "location_info";
	// private static final String WEATHER_INFO = "weather_info";
	private static final String PROVINCE = "province";
	private static final String CITY = "city";
	private static final String AREA = "area";
	private SharedPreferences location;
	// private SharedPreferences weather;
	private static final int MSG_UPDATE_CITY = 0;
	private static final int MSG_UPDATE_PROVINCE = 1;
	protected static final String TAG = "WeatherLocationActivity";
	protected static final int MSG_UPDATE_WEATHER = 2;

	interface WSCallback {
		void onGetWeatherCallback(String detail);
	}

	private String weatherToday = null;
	private int iconToday[] = new int[2];

	private Handler mHandler = new Handler() {
		@Override
		public void handleMessage(Message msg) {
			switch (msg.what) {
			case MSG_UPDATE_CITY:
				mCurrentCity = mCity.get(0);
				mCWheelView.setEntry(mCity, 0);
				mCWheelView.setEntry(mCity, searchNumber(mCity, mCurrentCity));
				// getCityWeather(mCurrentCity);
				break;
			case MSG_UPDATE_PROVINCE:
				Log.e("Ningxz", "toast begin");
				Toast.makeText(WeatherLocationActivity.this,"亲，现在网络不好，请退出，稍候再试...",Toast.LENGTH_SHORT).show();
				Log.e("Ningxz", "toast end");
				mPWheelView.setEntry(mProvince,
						searchNumber(mProvince, mCurrentProvince));
				break;
			case MSG_UPDATE_WEATHER:
				if (refreshpBar.isShown()) {
					refreshpBar.setVisibility(View.INVISIBLE);
				}
				mImg1.setImageResource(iconToday[0]);				
				mText1.setText(weatherToday);
				weather_message.setVisibility(View.VISIBLE);
//				city_choose.setEnabled(true);
				mAButton.setEnabled(true);
				mCButton.setEnabled(true);
				mPButton.setEnabled(true);
				mPButton.setFocusable(true);
				mPButton.requestFocus();
				break;
			default:
				break;
			}
		}
	};
	private WSCallback mWsCallback;
	private Thread mThread;

	@Override
	protected void onCreate(Bundle savedInstanceState) {
		mContext = this;
		super.onCreate(savedInstanceState);
		citycodedb = new CityCodeDB(WeatherLocationActivity.this);
		database = citycodedb.getDatabase("data.db");
		initProvinceList(database);
		Resources res = this.getResources();
		this.setContentView(R.layout.weatherlocationnew);

		refreshpBar = (ProgressBar)this.findViewById(R.id.downloadbar);
		
		city_choose = (LinearLayout)this.findViewById(R.id.city_choose);
		mAWheelView = (WheelView) this.findViewById(R.id.arealist);
		mCWheelView = (WheelView) this.findViewById(R.id.citylist);
		mPWheelView = (WheelView) this.findViewById(R.id.provicelist);
		mAWheelView.setOnKeyEventListener(this);
		mCWheelView.setOnKeyEventListener(this);
		mPWheelView.setOnKeyEventListener(this);
		
		weather_message = (LinearLayout)this.findViewById(R.id.weather_message);
		mText1 = (TextView) this.findViewById(R.id.text1);
		mImg1 = (ImageView) this.findViewById(R.id.today_weather1);
		
		mPButton = (Button) this.findViewById(R.id.provice);
		mCButton = (Button) this.findViewById(R.id.city);
		mAButton = (Button) this.findViewById(R.id.area);
		mPButton.setOnClickListener(this);
		mCButton.setOnClickListener(this);
		mAButton.setOnClickListener(this);

		location = getSharedPreferences(LOCATION_INFO, 0);
		mCurrentProvince = location.getString(PROVINCE,
				res.getString(R.string.beijing));
		mCurrentCity = location
				.getString(CITY, res.getString(R.string.beijing));
		mCurrentArea = location
				.getString(AREA, res.getString(R.string.beijing));
		
		String defaultProvince = location.getString(PROVINCE,"unknow");
		if(defaultProvince.equals("unknow")){
			String[] mLocFromIP = WeatherCityHelper.getRoughlyLocation();
			if(mLocFromIP[0] != null){
				String area_name = null;
				Log.e(TAG, "citycursor not null mLocFromIP[2]= " + mLocFromIP[2]);
				String myCityId = citycodedb.getCityIdByName(database, mLocFromIP[2]);
				Log.e(TAG, "citycursor not null myCityId= " + myCityId);
				if(myCityId != null){
					Cursor citycursor = citycodedb.getArea(database, myCityId);
					if (citycursor != null) {
						Log.e(TAG, "citycursor not null mLocFromIP[2]= " + mLocFromIP[2]);
						if (citycursor.moveToFirst()) {
							area_name = citycursor.getString(citycursor.getColumnIndex("name"));
						}
					}
					if(area_name != null){
						mCurrentProvince = mLocFromIP[1];
						mCurrentCity = mLocFromIP[2];
						mCurrentArea = area_name;
					}
				}
			}
		}

//		getCityList(database,
//				mProvinceId.get(searchNumber(mProvince, mCurrentProvince)));
//		mCurrentCity = mCity.get(0);
//		getAreaList(database, mCityID.get(searchNumber(mCity, mCurrentCity)));
//		mCurrentArea = mArea.get(0);

		mPButton.setText(mCurrentProvince);
		mCButton.setText(mCurrentCity);
		mAButton.setText(mCurrentArea);
		mCButton.setFocusable(false);
		mCButton.setFocusableInTouchMode(false);
		mAButton.setFocusable(false);
		mAButton.setFocusableInTouchMode(false);

		mPWheelView.setEntry(mProvince,
				searchNumber(mProvince, mCurrentProvince));
		mCWheelView.setEntry(mCity, searchNumber(mCity, mCurrentCity));
		mAWheelView.setEntry(mArea, searchNumber(mArea, mCurrentArea));
		mAWheelView.setVisibility(View.INVISIBLE);
		mCWheelView.setVisibility(View.INVISIBLE);
		mPWheelView.setVisibility(View.INVISIBLE);

		Log.e(TAG, "mCurrentArea = " + mCurrentArea);
		getCityWeather(mCurrentArea);

		mWsCallback = new WSCallback() {

			@Override
			public void onGetWeatherCallback(String detail) {

				Resources res = mContext.getResources();
				try {
					JSONObject jsonObject = new JSONObject(detail);
					JSONObject result = jsonObject.getJSONObject("weatherinfo");
					weatherToday = res.getString(R.string.weather)
							+ result.getString("weather");
					weatherToday += "\n" + res.getString(R.string.temp)
							+ result.getString("temp2") + "/"
							+ result.getString("temp1");

					iconToday = getWeaResByWeather(result.getString("weather"));
					Log.e(TAG, "iconToday ="+ iconToday.length);

				} catch (JSONException e) {
					e.printStackTrace();
				}
				mHandler.sendEmptyMessage(MSG_UPDATE_WEATHER);
			}

		};
	}

	@Override
	protected void onDestroy() {
		Log.e(TAG, "mThread will dead");
		super.onDestroy();
		mThread.interrupt();
	}
	
	public void initProvinceList(SQLiteDatabase database) {
		Log.e(TAG, "ININprovince");
		Cursor provincecursor = citycodedb.getAllProvince(database);
		if (provincecursor != null) {
			Log.e(TAG, "provincecursor not null");
			mProvince.clear();
			mProvinceId.clear();
			if (provincecursor.moveToFirst()) {
				do {
					String province_name = provincecursor
							.getString(provincecursor.getColumnIndex("name"));
					String province_id = provincecursor
							.getString(provincecursor.getColumnIndex("id"));
					mProvince.add(province_name);
					mProvinceId.add(province_id);
				} while (provincecursor.moveToNext());
			}
		}
		Log.e(TAG, mProvince.toString());
	}

	public void getCityList(SQLiteDatabase database, String proviceId) {
		Log.e(TAG, "ININcity");
		Cursor citycursor = citycodedb.getCity(database, proviceId);
		if (citycursor != null) {
			Log.e(TAG, "citycursor not null");
			mCity.clear();
			mCityID.clear();
			if (citycursor.moveToFirst()) {
				do {
					String city_name = citycursor.getString(citycursor
							.getColumnIndex("name"));
					String cityID = citycursor.getString(citycursor
							.getColumnIndex("id"));
					mCity.add(city_name);
					mCityID.add(cityID);
				} while (citycursor.moveToNext());
			}
		}
		Log.e(TAG, mCity.toString());
		Log.e(TAG, mCityID.toString());
		mCWheelView.setEntry(mCity, searchNumber(mCity, mCurrentCity));
	}

	public void getAreaList(SQLiteDatabase database, String cityId) {

		Cursor citycursor = citycodedb.getArea(database, cityId);
		if (citycursor != null) {
			Log.e(TAG, "citycursor not null");
			mArea.clear();
			if (citycursor.moveToFirst()) {
				do {
					String area_name = citycursor.getString(citycursor
							.getColumnIndex("name"));
					mArea.add(area_name);
				} while (citycursor.moveToNext());
			}
		}
		Log.e(TAG, mArea.toString());
		mAWheelView.setEntry(mArea, searchNumber(mArea, mCurrentArea));
	}

	private int searchNumber(List<String> ls, String str) {
		int retval = -1;
		for (int i = 0; i < ls.size(); i++) {
			if (ls.get(i).equals(str))
				retval = i;
		}
		return retval;
	}
	
	protected int[] getWeaResByWeather(String weather) {
		String[] strs = weather.split("转|到");
		int[] resIds = new int[strs.length];
		for (int i = 0; i < strs.length; i++) {
			resIds[i] = parseIcon(strs[i]);
			Log.e(TAG, "拆分后的天气：" + strs[i]);
		}
		if (resIds.length == 3) {
			if (resIds[0] == 0) {
				int[] newResids = new int[2];
				newResids[0] = resIds[1];
				newResids[1] = resIds[2];
				resIds = newResids;
			}
		} else if (resIds.length == 1) {
			int[] newResids = new int[2];
			newResids[0] = resIds[0];
			newResids[1] = 0;
			resIds = newResids;
		}
		return resIds;
	}

	protected int parseIcon(String weather) {
		if (weather.equals("阴")) {
			return R.drawable.ic_weather_cloudy_l;
		} else if (weather.equals("多云")) {
			return R.drawable.ic_weather_partly_cloudy_l;
		} else if (weather.equals("晴")) {
			return R.drawable.ic_weather_clear_day_l;
		} else if (weather.equals("小雨")) {
			return R.drawable.ic_weather_chance_of_rain_l;
		} else if (weather.equals("中雨")) {
			return R.drawable.ic_weather_chance_of_rain_l;
		} else if (weather.equals("大雨")) {
			return R.drawable.ic_weather_chance_of_rain_ml;
		} else if (weather.equals("暴雨")) {
			return R.drawable.ic_weather_chance_of_rain_xl;
		} else if (weather.equals("大暴雨")) {
			return R.drawable.ic_weather_chance_of_rain_xxl;
		} else if (weather.equals("特大暴雨")) {
			return R.drawable.ic_weather_chance_of_rain_xxxl;
		} else if (weather.equals("阵雨")) {
			return R.drawable.ic_weather_chance_storm_l;
		} else if (weather.equals("雷阵雨")) {
			return R.drawable.ic_weather_chance_of_rain_xxxl;
		} else if (weather.equals("小雪")) {
			return R.drawable.ic_weather_chance_snow_l;
		} else if (weather.equals("中雪")) {
			return R.drawable.ic_weather_snow_l;
		} else if (weather.equals("大雪")) {
			return R.drawable.ic_weather_snow_ml;
		} else if (weather.equals("暴雪")) {
			return R.drawable.ic_weather_snow_ml;
		} else if (weather.equals("冰雹")) {
			return R.drawable.ic_weather_icy_sleet_l;
		} else if (weather.equals("雨夹雪")) {
			return R.drawable.ic_weather_snow_rain_xl;
		} else if (weather.equals("风")) {
			return R.drawable.ic_weather_windy_l;
		} else if (weather.equals("龙卷风")) {
			return R.drawable.ic_weather_windy_l;
		} else if (weather.equals("雾")) {
			return R.drawable.ic_weather_fog_l;
		}
		return 0;
	}


	@Override
	public void onClick(View v) {
		switch (v.getId()) {
		case R.id.city:
			onPressKeyCenter(mPWheelView,mPWheelView.getPosition(),null);
			v.setVisibility(View.INVISIBLE);
			mCWheelView.setVisibility(View.VISIBLE);
			mPWheelView.setVisibility(View.INVISIBLE);
			mAWheelView.setVisibility(View.INVISIBLE);
			mPButton.setVisibility(View.VISIBLE);
			mAButton.setVisibility(View.VISIBLE);
			mPButton.setFocusable(false);
			mPButton.setFocusableInTouchMode(false);
			mAButton.setFocusable(false);
			mAButton.setFocusableInTouchMode(false);
			mCWheelView.setFocusable(true);
			mCWheelView.requestFocus();
			break;
		case R.id.provice:
			v.setVisibility(View.INVISIBLE);
			mPWheelView.setVisibility(View.VISIBLE);
			mCWheelView.setVisibility(View.INVISIBLE);
			mAWheelView.setVisibility(View.INVISIBLE);
			mCButton.setVisibility(View.VISIBLE);
			mAButton.setVisibility(View.VISIBLE);
			mCButton.setFocusable(false);
			mCButton.setFocusableInTouchMode(false);
			mAButton.setFocusable(false);
			mAButton.setFocusableInTouchMode(false);
			mPWheelView.setFocusable(true);
			mPWheelView.requestFocus();
			break;
		case R.id.area:
			onPressKeyCenter(mPWheelView,mPWheelView.getPosition(),null);
			onPressKeyCenter(mCWheelView,mCWheelView.getPosition(),null);
			v.setVisibility(View.INVISIBLE);
			mAWheelView.setVisibility(View.VISIBLE);
			mCWheelView.setVisibility(View.INVISIBLE);
			mPWheelView.setVisibility(View.INVISIBLE);
			mPButton.setVisibility(View.VISIBLE);
			mCButton.setVisibility(View.VISIBLE);
			mCButton.setFocusable(false);
			mCButton.setFocusableInTouchMode(false);
			mPButton.setFocusable(false);
			mPButton.setFocusableInTouchMode(false);
			mAWheelView.setFocusable(true);
			mAWheelView.requestFocus();
		default:
			break;
		}
	}

	private void getCityWeather(String area) {
		
		mHandler.post(new Runnable() {

			@Override
			public void run() {
				refreshpBar.setVisibility(View.VISIBLE);
				weather_message.setVisibility(View.INVISIBLE);
//				city_choose.setEnabled(false);
				mAButton.setEnabled(false);
				mCButton.setEnabled(false);
				mPButton.setEnabled(false);
			}
		});
		
		final String cityCode = citycodedb.getCityCodeByName(database, area);
		Log.e(TAG, "citycode = " + cityCode);
		mThread = new Thread(new Runnable() {
			@Override
			public void run() {
				String detail = WeatherCityHelper
						.getWeatherByCityCode1(cityCode);
				Log.d(TAG, "detail = " + detail);
				if (detail == null || detail.equals("")) {
					mHandler.sendEmptyMessage(MSG_UPDATE_PROVINCE);
				}else {
					mWsCallback.onGetWeatherCallback(detail);
				}
			}
		});
		mThread.setName("httpAccess");
		mThread.start();
	}

	@Override
	public boolean onPressKeyUp(View v, int position, String str) {
		return false;
	}

	@Override
	public boolean onPressKeyDown(View v, int position, String str) {
		return false;
	}

	private void sendLocationChangedBroadCast(String province, String city, String area) {
		Intent intent = new Intent();
		intent.setAction("com.softwinner.tvdsetting.LOCATION_CHANGE");
		intent.putExtra("province", province);
		intent.putExtra("city", city);
		intent.putExtra("area", area);
		this.sendBroadcast(intent);
	}

	@Override
	public boolean onPressKeyCenter(View v, int position, String str) {

		switch (v.getId()) {
		case R.id.provicelist:
			if (mProvince != null) {
				mCurrentProvince = mProvince.get(position);
				mPButton.setText(mCurrentProvince);

				getCityList(database, mProvinceId.get(position));
				Log.e(TAG, "MCITYID =" + mCityID.toString());
				getAreaList(database, mCityID.get(0));
				mCurrentCity = mCity.get(0);
				mCWheelView.setEntry(mCity, searchNumber(mCity, mCurrentCity));
				
				mCurrentArea = mArea.get(0);
				mAWheelView.setEntry(mArea, searchNumber(mArea, mCurrentArea));

			}
			mPButton.setVisibility(View.VISIBLE);
			mPButton.setFocusable(false);
			mPButton.setFocusableInTouchMode(false);
			mCButton.setText(mCurrentCity);
			mAButton.setText(mCurrentArea);
			v.setVisibility(View.INVISIBLE);
			mCButton.setFocusable(true);
			mCButton.requestFocus();
			break;

		case R.id.citylist:
			if (mCity != null) {
				mCurrentCity = mCity.get(position);
				mCButton.setText(mCurrentCity);

				getAreaList(database, mCityID.get(position));
				mCurrentArea = mArea.get(0);
				mAWheelView.setEntry(mArea, searchNumber(mArea, mCurrentArea));
			}
			mCButton.setVisibility(View.VISIBLE);
			mCButton.setFocusable(false);
			mCButton.setFocusableInTouchMode(false);
			mAButton.setText(mCurrentArea);
			v.setVisibility(View.INVISIBLE);
			mAButton.setFocusable(true);
			mAButton.requestFocus();
			break;

		case R.id.arealist:
			if (mArea != null) {
				mCurrentArea = mArea.get(position);
				mAButton.setText(mCurrentArea);
				SharedPreferences.Editor editor = location.edit();
				editor.putString(PROVINCE, mCurrentProvince);
				editor.putString(CITY, mCurrentCity);
				editor.putString(AREA, mCurrentArea);
				editor.commit();
				Log.e(TAG, "mCurrentArea press = " + mCurrentArea);
				getCityWeather(mCurrentArea);
			}
			mAButton.setVisibility(View.VISIBLE);
			mAButton.setFocusable(false);
			mAButton.setFocusableInTouchMode(false);
			v.setVisibility(View.INVISIBLE);
			sendLocationChangedBroadCast(mCurrentProvince, mCurrentCity, mCurrentArea);
		}
		return false;
	}

}
