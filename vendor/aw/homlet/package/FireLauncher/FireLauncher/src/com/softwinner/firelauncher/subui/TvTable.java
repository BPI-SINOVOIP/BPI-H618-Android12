package com.softwinner.firelauncher.subui;

import java.util.ArrayList;

import android.content.ContentValues;
import android.content.Context;
import android.database.Cursor;
import android.database.sqlite.SQLiteDatabase;
import android.database.sqlite.SQLiteOpenHelper;
import android.provider.BaseColumns;
import android.util.Log;

import com.softwinner.firelauncher.utils.Constants;
import com.softwinner.firelauncher.utils.DateUtil;

public class TvTable extends SQLiteOpenHelper {

	private String TAG = "TvTable";
	private boolean DEBUG = true;

	// private static TvTable instance = null;
	// private Context context;
	private String tableName;
	private static final int VERSION = 3;
	private static final String _ID = BaseColumns._ID;

	// public static TvTable getInstance(Context context, String name) {
	// if(null == instance) {
	// instance = new TvTable(context, name);
	// }
	// return instance;
	// }

	public TvTable(Context context, String name) {
		super(context, Constants.DBNAME + name, null, VERSION);
		// this.context = context;
		this.tableName = name;
	}

	@Override
	public void onCreate(SQLiteDatabase db) {
		// TODO Auto-generated method stub
		if(DEBUG) Log.d(TAG, "onCreate onCreate onCreate onCreate onCreate onCreate");
		String sql = "create table if not exists " + tableName + " ( " + _ID
				+ " INTEGER primary key autoincrement,"
				+ Constants.INTENT_VIDEO_ID + " text not null,"
				+ Constants.INTENT_VIDEO_NAME + " text not null,"
				+ Constants.INTENT_VIDEO_IMG_URL + " text not null,"
				+ Constants.INTENT_EPISODE_ID + " text not null,"
				+ Constants.INTENT_EPISODE_NAME + " text not null,"
				+ Constants.INTENT_EPISODE_COUNT + " INTEGER not null,"
				+ Constants.INTENT_VIDEO_INDEX + " INTEGER not null,"
				+ Constants.INTENT_VIDEO_INDEX_NAME + " text not null,"
				+ Constants.INTENT_VIDEO_INDEX_COUNT + " INTEGER not null,"
				+ Constants.INTENT_CURRENT_POSITION + " INTEGER not null,"
				+ Constants.INTENT_DURATION + " INTEGER not null,"
				+ Constants.XXXXXX_ADDED_DATE + " text not null)";
		db.execSQL(sql);
	}

	@Override
	public void onUpgrade(SQLiteDatabase db, int oldVersion, int newVersion) {
		// TODO Auto-generated method stub
		db.execSQL("drop table if exists " + tableName);
		onCreate(db);
	}

	private boolean hasItem(String videoId) {
		SQLiteDatabase db = getReadableDatabase();
		Cursor cursor = db.query(tableName, null, Constants.INTENT_VIDEO_ID
				+ "=?", new String[] { videoId }, null, null, null);
		if(DEBUG) Log.d("",
				"hasTvTracker: " + cursor.toString() + ", count "
						+ cursor.getCount() + ", id " + videoId);
		boolean ret;
		if (null == cursor || 0 == cursor.getCount()) {
			ret = false;
		} else {
			ret = true;
		}
		cursor.close();
		db.close();
		return ret;
	}

	public void addItem(MangoBean bean) {
		// if(hasItem(bean.videoId))
		// return;
		
		// db.beginTransaction();
		ContentValues values = new ContentValues();

		values.put(Constants.INTENT_VIDEO_ID, bean.videoId);
		values.put(Constants.INTENT_VIDEO_NAME, bean.videoName);
		values.put(Constants.INTENT_VIDEO_IMG_URL, bean.videoImgUrl);
		values.put(Constants.INTENT_EPISODE_ID, bean.episodeId);
		values.put(Constants.INTENT_EPISODE_NAME, bean.episodeName);
		values.put(Constants.INTENT_EPISODE_COUNT, bean.episodeCount);
		values.put(Constants.INTENT_VIDEO_INDEX, bean.videoIndex);
		values.put(Constants.INTENT_VIDEO_INDEX_NAME, bean.videoIndexName);
		values.put(Constants.INTENT_VIDEO_INDEX_COUNT, bean.videoIndexCount);
		values.put(Constants.INTENT_CURRENT_POSITION, bean.currentPosition);
		values.put(Constants.INTENT_DURATION, bean.duration);
		values.put(Constants.XXXXXX_ADDED_DATE, DateUtil.getDateDayFormat());
		if (!hasItem(bean.videoId)) {
			SQLiteDatabase db = getWritableDatabase();
			long count = db.insert(tableName, null, values);
			if(DEBUG) Log.d("", "insert count " + count);
			db.close();
		} else {
			SQLiteDatabase db = getWritableDatabase();
			int count = db.update(tableName, values, Constants.INTENT_VIDEO_ID
					+ "=?", new String[] { bean.videoId });
			if(DEBUG) Log.d("", "update count " + count);
			db.close();
		}
		

	}

	public void delItem(String videoId) {
		if (!hasItem(videoId))
			return;

		SQLiteDatabase db = getWritableDatabase();
		// db.beginTransaction();
		long count = db.delete(tableName, Constants.INTENT_VIDEO_ID + "=\'"
				+ videoId + "\'", null/* new String[] {videoId} */);
		if(DEBUG) Log.d("", "delete count " + count);
		// db.endTransaction();
		db.close();
	}
	
	public void delALLItems() {
		SQLiteDatabase db = getWritableDatabase();
		long count = db.delete(tableName, null, null/* new String[] {videoId} */);
		if(DEBUG) Log.d("", "delete count " + count);
		db.close();
	}

	public ArrayList<MangoBean> getItems() {
		ArrayList<MangoBean> beanList = new ArrayList<MangoBean>();
		SQLiteDatabase db = getReadableDatabase();
		Cursor cursor = db.query(tableName, null, null, null, null, null, null);
		if(DEBUG) Log.d("", cursor.toString() + ", count " + cursor.getCount());
		if (cursor != null && cursor.getCount() > 0) {
			cursor.moveToFirst();
			do {
				MangoBean bean = new MangoBean();
				bean.videoId = cursor.getString(cursor
						.getColumnIndex(Constants.INTENT_VIDEO_ID));
				bean.videoName = cursor.getString(cursor
						.getColumnIndex(Constants.INTENT_VIDEO_NAME));
				bean.videoImgUrl = cursor.getString(cursor
						.getColumnIndex(Constants.INTENT_VIDEO_IMG_URL));
				bean.episodeId = cursor.getString(cursor
						.getColumnIndex(Constants.INTENT_EPISODE_ID));
				bean.episodeName = cursor.getString(cursor
						.getColumnIndex(Constants.INTENT_EPISODE_NAME));
				bean.episodeCount = cursor.getInt(cursor
						.getColumnIndex(Constants.INTENT_EPISODE_COUNT));
				bean.videoIndex = cursor.getInt(cursor
						.getColumnIndex(Constants.INTENT_VIDEO_INDEX));
				bean.videoIndexName = cursor.getString(cursor
						.getColumnIndex(Constants.INTENT_VIDEO_INDEX_NAME));
				bean.videoIndexCount = cursor.getInt(cursor
						.getColumnIndex(Constants.INTENT_VIDEO_INDEX_COUNT));
				bean.currentPosition = cursor.getInt(cursor
						.getColumnIndex(Constants.INTENT_CURRENT_POSITION));
				bean.duration = cursor.getInt(cursor
						.getColumnIndex(Constants.INTENT_DURATION));
				bean.addedDate = cursor.getString(cursor
						.getColumnIndex(Constants.XXXXXX_ADDED_DATE));
				beanList.add(bean);
			} while (cursor.moveToNext());
		}
		cursor.close();
		db.close();
		return beanList;
	}
}
