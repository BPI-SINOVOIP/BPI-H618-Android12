package com.softwinner.tvdsetting.weather;

import android.content.Context;
import android.database.Cursor;
import android.database.sqlite.SQLiteDatabase;
import android.util.Log;

public class CityCodeDB {
	public static final String TABLE_PROVINCE = "province";
	public static final String TABLE_CITY = "city";
	public static final String TABLE_AREA = "area";
	public static final String TABLE_CITY_CODE = "city_code";

	private Context context;

	public CityCodeDB(Context context) {
		this.context = context;
	}

	public SQLiteDatabase getDatabase(String dbname) {
		AssetsDatabaseManager.initManager(context);
		AssetsDatabaseManager mg = AssetsDatabaseManager.getManager();
		SQLiteDatabase db = mg.getDatabase(dbname);
		return db;
	}

	public Cursor getAllProvince(SQLiteDatabase db) {
		Log.e("WeatherLocationActivity", "database");
		if (db != null) {
			Log.e("WeatherLocationActivity", "database not null");
			Cursor cur = db
					.query(TABLE_PROVINCE, new String[] { "id", "name" }, null,
							null, null, null, null);
			return cur;
		} else {
			return null;
		}
	}

	public Cursor getCity(SQLiteDatabase db, String provinceid) {
		Log.e("WeatherLocationActivity", "database");
		if (db != null) {
			Log.e("WeatherLocationActivity", "database not null");
			Cursor cur = db.query(TABLE_CITY, new String[] {"id",
					"name" }, "p_id = ?", new String[] { provinceid }, null,
					null, null);
			return cur;
		} else {
			return null;
		}
	}
	
	public String getCityIdByName(SQLiteDatabase db, String cityname) {
		Log.e("WeatherLocationActivity", "database");
		if (db != null) {
			Log.e("WeatherLocationActivity", "database not null");
			Cursor cur = db.query(TABLE_CITY, new String[] {"id",
					"name" }, "name = ?", new String[] { cityname }, null,
					null, null);
			String cityid = null;
			if (cur.moveToFirst()) {
				cityid = cur.getString(cur.getColumnIndex("id"));
			}
			return cityid;
		} else {
			return null;
		}
	}

	Cursor getArea(SQLiteDatabase db, String cityid) {
		if (db != null) {
			Cursor cur = db.query(TABLE_AREA, new String[] { "id", "c_id",
					"name" }, "c_id = ?", new String[] { cityid }, null, null,
					null);
			return cur;
		} else {
			return null;
		}
	}

	String getCityCode(SQLiteDatabase db, String areaid) {
		if (db != null) {
			Log.e("WeatherLocationActivity", "database not null~~~");
			Cursor cur = db.query(TABLE_CITY_CODE, new String[] { "id", "code",
					"name" }, "id = ?", new String[] { areaid }, null, null,
					null);
			String citycode = null;
			if (cur.moveToFirst()) {
				citycode = cur.getString(cur.getColumnIndex("code"));
				Log.e("WeatherLocationActivity", "database not null" + citycode);
			}
			return citycode;
		} else {
			return null;
		}
	}

	String getCityCodeByName(SQLiteDatabase db, String areaname) {
		if (db != null) {
			Cursor cur = db.query(TABLE_CITY_CODE, new String[] { "code","name"
					}, "name = ?", new String[] { areaname }, null,
					null, null);
			String citycode = null;
			if (cur.moveToFirst()) {
				citycode = cur.getString(cur.getColumnIndex("code"));
			}
			return citycode;
		} else {
			return null;
		}
	}
}
