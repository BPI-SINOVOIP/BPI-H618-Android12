package com.softwinner.TvdFileManager.net;

import android.content.ContentValues;
import android.content.Context;
import android.database.Cursor;
import android.database.sqlite.SQLiteDatabase;
import android.util.Log;

import java.io.File;

/**
 * Store samba server and login information Database: login.db Table: login
 * ---------------------------------------------------------------------- |
 * smbFilePath(TEXT) | domain(TEXT) | username(TEXT) | passwrod(TEXT) |
 * ----------------------------------------------------------------------
 * |smb://ETHANSHAN-PC/| ? | ethanshan | wanran |
 * ----------------------------------------------------------------------
 */
public class SmbLoginDB {
    private static final String TAG = "LoginDatabase";
    private Context mContext = null;
    private SQLiteDatabase mLoginDB = null;
    private static final String DB_DIR = "database";
    private static final String DB_NAME = "login.db";
    private static final String DB_TABLE = "login";
    public static final String SMB_PATH = "smbFilePath";
    public static final String DOMAIN = "domain";
    public static final String USER_NAME = "username";
    public static final String PASSWORD = "password";
    public static final String TABLE_CREATE = "CREATE TABLE IF NOT EXISTS " + DB_TABLE + " ("
            + SMB_PATH + " TEXT," + DOMAIN + " TEXT," + USER_NAME + " TEXT," + PASSWORD + " TEXT)";

    public SmbLoginDB(Context context) {
        mContext = context;
        init();
    }

    private void init() {
        File dir = mContext.getDir(DB_DIR, 0);
        File dbFile = new File(dir, DB_NAME);
        String dbPath = dbFile.getAbsolutePath();
        if (mLoginDB == null) {
            createDB(dbPath);
        } else {
            if (!dbFile.exists()) {
                createDB(dbPath);
            }
        }
    }

    private void createDB(String path) {

        mLoginDB = SQLiteDatabase.openOrCreateDatabase(path, null);
        mLoginDB.execSQL(TABLE_CREATE);
    }

    public void insert(String smbPath, String domain, String username, String password) {
        init();
        ContentValues cv = new ContentValues();
        cv.put(SMB_PATH, smbPath);
        cv.put(DOMAIN, domain);
        cv.put(USER_NAME, username);
        cv.put(PASSWORD, password);
        mLoginDB.insert(DB_TABLE, null, cv);
    }

    public void delete(String smbPath) {
        init();
        String[] args = {smbPath};
        String clause = SMB_PATH + "=?";
        mLoginDB.delete(DB_TABLE, clause, args);
    }

    public Cursor query(String[] columes, String selection, String[] selectionArgs,
                            String orderBy) {
        init();
        Cursor c = null;
        try {
            c = mLoginDB.query(DB_TABLE, columes, selection, selectionArgs, null, null, orderBy);
        } catch (IllegalStateException e) {
            Log.e(TAG, "database not open");
        }
        return c;
    }

    public void closeDB() {
        mLoginDB.close();
    }
}
