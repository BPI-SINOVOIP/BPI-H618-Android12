package com.softwinner.timerswitch.data;

import android.content.Context;
import android.database.sqlite.SQLiteDatabase;
import android.database.sqlite.SQLiteOpenHelper;
import android.util.Log;

import com.softwinner.timerswitch.utils.Constant;

public class AlarmDatabase extends SQLiteOpenHelper {

    private final static String TAG = "AlarmDatabase";

    public AlarmDatabase(Context context) {
        super(context, Constant.DB_NAME, null, Constant.DB_VERSION);
    }

    @Override
    public void onCreate(SQLiteDatabase db) {
        Log.d(TAG,"**************** create ClockDatabase !");
        db.execSQL("CREATE TABLE IF NOT EXISTS " + Constant.TABLE_NAME + " (" +
                Constant.COLUMN_ID + " INTEGER NOT NULL PRIMARY KEY AUTOINCREMENT," +
                Constant.COLUMN_CREATE_TIMESTAMP + " INTEGER NOT NULL," +
                Constant.COLUMN_DAYS_OF_WEEK + " TEXT, " +
                Constant.COLUMN_REPEAT_DAYS_OF_WEEK + " TEXT," +
                Constant.COLUMN_HOUR + " INTEGER NOT NULL, " +
                Constant.COLUMN_MINUTES + " INTEGER NOT NULL, " +
                Constant.COLUMN_ENABLED + " INTEGER  DEFAULT 0 , " +
                Constant.COLUMN_LABEL + " TEXT NOT NULL DEFAULT 'boot', " +
                Constant.COLUMN_ALERT_TIME + " INTEGER NOT NULL )"
        );
    }

    @Override
    public void onUpgrade(SQLiteDatabase db, int oldVersion, int newVersion) {
        Log.d(TAG,"Upgrading ClockDatabase from version " + oldVersion + " to " + newVersion);
    }


}
