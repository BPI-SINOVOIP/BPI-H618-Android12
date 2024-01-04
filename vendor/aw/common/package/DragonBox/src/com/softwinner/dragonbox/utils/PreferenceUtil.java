package com.softwinner.dragonbox.utils;

import android.content.Context;
import android.content.SharedPreferences;
import android.content.SharedPreferences.Editor;
import android.preference.PreferenceManager;
import android.util.Log;
import com.softwinner.dragonbox.DragonBoxApplication;

public class PreferenceUtil {

    private static final String TAG = "PreferenceUtil";
 public static final String FACTORY_SUCCESS = "1";
 public static final String FACTORY_FAIL = "0";

 private static final String PREF_FACTORY_FILE = "factory_pass";
 private static final String PREF_FACTORY_RESULT_KEY = "pass";

 public static final String KEY_UPLOAD_MES_URL = "key_upload_mes_url";

 private static SharedPreferences mSharedPreferences = PreferenceManager
                .getDefaultSharedPreferences(DragonBoxApplication.getAppContext());
    private static Editor mEditor = mSharedPreferences.edit();

 public static void writeFactory(Context context, boolean success) {
  SharedPreferences sharedPreferences = context.getSharedPreferences(PREF_FACTORY_FILE, Context.MODE_WORLD_READABLE);
  Editor editor = sharedPreferences.edit();
  editor.putString(PREF_FACTORY_RESULT_KEY, success ? FACTORY_SUCCESS : FACTORY_FAIL);
  editor.commit();
  Log.i(TAG, "==============================" + sharedPreferences.getString("pass", "0"));
 }

 public static String readFactory(Context context){
  SharedPreferences sharedPreferences = context.getSharedPreferences(PREF_FACTORY_FILE, Context.MODE_WORLD_READABLE);
  return sharedPreferences.getString(PREF_FACTORY_RESULT_KEY, FACTORY_FAIL);
 }

 public static boolean getBoolean(String key, boolean def) {
        return mSharedPreferences.getBoolean(key, def);
    }

    public static void setBoolean(String key, boolean value) {
        mEditor.putBoolean(key, value);
        mEditor.commit();
    }

    public static int getInt(String key, int def) {
        return mSharedPreferences.getInt(key, def);
    }

    public static void setInt(String key, int value) {
        mEditor.putInt(key, value);
        mEditor.commit();
    }

    public static String getString(String key, String def) {
        return mSharedPreferences.getString(key, def);
    }

    public static void setString(String key, String value) {
        mEditor.putString(key, value);
        mEditor.commit();
    }

    public static Float getFloat(String key, float def) {
        return mSharedPreferences.getFloat(key, def);
    }

    public static void setFloat(String key, float value) {
        mEditor.putFloat(key, value);
        mEditor.commit();
    }

    public static long getLong(String key, Long def) {
        return mSharedPreferences.getLong(key, def);
    }

    public static void setLong(String key, long value) {
        mEditor.putLong(key, value);
        mEditor.commit();
    }

}
