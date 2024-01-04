package com.softwinner.TvdFileManager;

import android.content.ContentValues;
import android.content.Context;
import android.database.Cursor;
import android.database.sqlite.SQLiteDatabase;
import android.util.Log;

import java.io.File;

public class ImageDatabase {
    private static final String TAG = "ImageDatabase";
    private static final String APP_DIR_NAME = ".fileManager";
    private static final String DB_NAME = "fileManager.db";

    private static final String IMAGE_TABLE = "image";
    public static final String ORIG_PATH = "orignalPath";
    public static final String THUMB_PATH = "thumbPath";
    public static final String CREATE_TIME = "created_time";
    public static final String KEY_ID = "_id";
    public static final String KEY_NAME = "name";
    public static final String TABLE_CREATE = "CREATE TABLE IF NOT EXISTS " + IMAGE_TABLE + " ("
            + ORIG_PATH + " TEXT," + THUMB_PATH + " TEXT," + CREATE_TIME + " INTEGER)";
    private SQLiteDatabase imageDatabase;
    private Context mContext;
    private String appDir;

    public ImageDatabase(Context context) {
        mContext = context;
        init();
        appDir = mContext.getDir(APP_DIR_NAME, 0).getAbsolutePath();
    }

    public String getAppDir() {
        return appDir;
    }

    /* 为了防止在文件浏览时，有人无意删除掉该文件夹内内容，导致数据库访问错误，每次操作前都要进行初始化 */
    private void init() {
        File dir = mContext.getDir(APP_DIR_NAME, 0);
        File dbFile = new File(dir, DB_NAME);
        String dbPath = dbFile.getAbsolutePath();
        if (imageDatabase == null) {
            createDB(dbPath);
        } else {

            if (!dbFile.exists()) {
                createDB(dbPath);
            }
        }
    }

    private void createDB(String path) {
        // 打开或创建指定的图片数据库
        imageDatabase = SQLiteDatabase.openOrCreateDatabase(path, null);
        imageDatabase.execSQL(TABLE_CREATE);
        Log.d(TAG, "open database success");
    }

    /**
     * 插入一项数据
     *
     * @param origPath
     *            :源图片文件路径
     * @param thumbPath
     *            :缩略图文件路径
     */
    public void insert(String origPath, String thumbPath) {
        init();
        File thumb = new File(thumbPath);
        long time = thumb.lastModified();
        ContentValues cv = new ContentValues();
        cv.put(ORIG_PATH, origPath);
        cv.put(THUMB_PATH, thumbPath);
        cv.put(CREATE_TIME, time);
        imageDatabase.insert(IMAGE_TABLE, null, cv);
    }

    /**
     * 删除数据库中源文件路径所在的数据
     *
     * @param origPath
     */
    public void delete(String origPath) {
        init();
        String[] args = {origPath};
        String clause = ORIG_PATH + "=?";
        imageDatabase.delete(IMAGE_TABLE, clause, args);
    }

    /**
     * 查询数据库中的图片信息
     *
     * @param columns
     *            :查询的列
     * @param selection
     *            :条件
     * @param selctionArgs
     * @param orderBy
     *            :整理类型
     * @return
     */
    public Cursor query(String[] columns, String selection, String[] selectionArgs,
                              String orderBy) {
        init();
        Cursor c = null;
        try {
            c = imageDatabase.query(IMAGE_TABLE, columns, selection, selectionArgs, null, null,
                    orderBy);
        } catch (IllegalStateException e) {
            Log.e(TAG, "database not open");
        }
        return c;
    }

    public void closeDatabase() {
        imageDatabase.close();
    }
}
