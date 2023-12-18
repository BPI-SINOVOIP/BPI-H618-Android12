package com.softwinner.update;

import android.content.Context;
import android.content.SharedPreferences;

public class Preferences {
    private Context mContext;

    // access by download task
    public static final String PREFS_DOWNLOAD_SIZE = "download_size";

    // access by download task
    public static final String PREFS_DOWNLOAD_POSITION = "download_position";

    // access by checking task
    public static final String PREFS_DOWNLOAD_TARGET = "download_target";

    // access by checking task
    public static final String PREFS_DOWNLOAD_URL = "download_URL";

    // access by checking task
    public static final String PREFS_PACKAGE_DESCRIPTOR = "package_descriptor";

    // access by checking task
    public static final String PREFS_PACKAGE_MD5 = "package_md5";

    // access by loader
    public static final String PREFS_CHECK_TIME = "check_time";

    public static final String PREFS_NOTICE_TRUE = "notice_true";

    private SharedPreferences mPrefs;

    public Preferences(Context context) {
        mPrefs = context.getSharedPreferences("SHARE", Context.MODE_PRIVATE);
        mContext = context;

    }

    private void setString(String key, String Str) {
        SharedPreferences.Editor mEditor = mPrefs.edit();
        mEditor.putString(key, Str);
        mEditor.commit();
    }

    private void setInt(String key, int Int) {
        SharedPreferences.Editor mEditor = mPrefs.edit();
        mEditor.putInt(key, Int);
        mEditor.commit();
    }

    private void setLong(String key, long Long) {
        SharedPreferences.Editor mEditor = mPrefs.edit();
        mEditor.putLong(key, Long);
        mEditor.commit();
    }

    private void setBoolean(String key, Boolean bool) {
        SharedPreferences.Editor mEditor = mPrefs.edit();
        mEditor.putBoolean(key, bool);
        mEditor.commit();
    }

    public void setDownloadSize(long size) {
        setLong(PREFS_DOWNLOAD_SIZE, size);
    }

    private void setDownloadPos(long position) {
        setLong(PREFS_DOWNLOAD_POSITION, position);
    }

    public void setBreakpoint(long size, long position) {
        setLong(PREFS_DOWNLOAD_SIZE, size);
        setLong(PREFS_DOWNLOAD_POSITION, position);
    }

    public void setDownloadInfo(String url, String targetFile) {
        setString(PREFS_DOWNLOAD_URL, url);
        setString(PREFS_DOWNLOAD_TARGET, targetFile);
    }

    public void setMd5(String md5) {
        setString(PREFS_PACKAGE_MD5, md5);
    }

    public String getMd5() {
        return mPrefs.getString(PREFS_PACKAGE_MD5, "");
    }

    public void setCheckTime(long time) {
        setLong(PREFS_CHECK_TIME, time);
    }

    public void setPackageDescriptor(String str) {
        setString(PREFS_PACKAGE_DESCRIPTOR, str);
    }

    public void setDownloadTarget(String target) {
        setString(PREFS_DOWNLOAD_TARGET, target);
    }

    public void setDownloadURL(String URL) {
        setString(PREFS_DOWNLOAD_URL, URL);
    }


    public void setNotice(Boolean bool) {
        setBoolean(PREFS_NOTICE_TRUE, bool);
    }

    public String getPackageDescriptor() {
        return mPrefs.getString(PREFS_PACKAGE_DESCRIPTOR, "");
    }

    @SuppressWarnings("unused")
    public long getDownloadSize() {
        return mPrefs.getLong(PREFS_DOWNLOAD_SIZE, 0);
    }

    @SuppressWarnings("unused")
    public long getDownloadPos() {
        return mPrefs.getLong(PREFS_DOWNLOAD_POSITION, 0);
    }

    @SuppressWarnings("unused")
    public String getDownloadTarget() {
        return mPrefs.getString(PREFS_DOWNLOAD_TARGET, null);
    }

    @SuppressWarnings("unused")
    public String getDownloadURL() {
        return mPrefs.getString(PREFS_DOWNLOAD_URL, null);
    }

    public long getCheckTime() {
        return mPrefs.getLong(PREFS_CHECK_TIME, 0);
    }

    public boolean getNotice() {
        return mPrefs.getBoolean(PREFS_NOTICE_TRUE, true);
    }

    public void setDownloadInfo(long size, long position, String target, String URL) {
        setDownloadSize(size);
        setDownloadPos(position);
        setDownloadTarget(target);
        setDownloadURL(URL);
    }

    public int getID() {
        if (mPrefs.getInt("ID", 1001) == 1001) {
            int random = (int) (Math.random() * 1000);
            setInt("ID", random);
        }
        if (Configs.DEBUG) return 1000;
        else return mPrefs.getInt("ID", 0);

    }

}
