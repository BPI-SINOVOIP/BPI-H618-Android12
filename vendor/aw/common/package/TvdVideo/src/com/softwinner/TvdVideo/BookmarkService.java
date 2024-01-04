package com.softwinner.TvdVideo;

import android.content.Context;
import android.database.Cursor;
import android.database.sqlite.SQLiteDatabase;

public class BookmarkService {
    private final int maxRecord = 100;
    private MangerDatabase dbmanger;

    public BookmarkService(Context context) {
        dbmanger = new MangerDatabase(context);
    }

    public void save(String path, int bookmark, int subsave, int tracksave, int subcolorsave,
            int subsizesave) {
        long time = System.currentTimeMillis();

        SQLiteDatabase database = dbmanger.getWritableDatabase();
        if (getCount() >= maxRecord) {
            long oldestTime = time;
            Cursor cursor = database.query(MangerDatabase.NAME, null, null, null, null, null, null);
            if (cursor != null) {
                try {
                    while (cursor.moveToNext()) {
                        int mIdIndex = cursor.getColumnIndexOrThrow(MangerDatabase.TIME);
                        long recordTime = cursor.getLong(mIdIndex);
                        if (recordTime < oldestTime) {
                            oldestTime = recordTime;
                        }
                    }
                } finally {
                    cursor.close();
                }
            }
            if (oldestTime < time) {
                database.execSQL("delete from " + MangerDatabase.NAME + " where "
                        + MangerDatabase.TIME + "=?", new Object[] {oldestTime});
            }
        }

        database.execSQL("insert into " + MangerDatabase.NAME + "(" + MangerDatabase.PATH + ","
                + MangerDatabase.BOOKMARK + "," + MangerDatabase.SUBSAVE + ","
                + MangerDatabase.TRACKSAVE + "," + MangerDatabase.SUBCOLORSAVE + ","
                + MangerDatabase.SUBSIZESAVE + "," + MangerDatabase.TIME
                + ") values(?,?,?,?,?,?,?)", new Object[] {path, bookmark, subsave, tracksave,
                subcolorsave, subsizesave, time});
    }

    public boolean delete(String path) {
        boolean ret = false;
        SQLiteDatabase database = dbmanger.getWritableDatabase();

        Cursor cursor = database.rawQuery("select * from " + MangerDatabase.NAME + " where "
                + MangerDatabase.PATH + "=?", new String[] {path});
        if (cursor != null) {
            database.execSQL("delete from " + MangerDatabase.NAME + " where " + MangerDatabase.PATH
                    + "=?", new Object[] {path});
            cursor.close();

            ret = true;
        }

        return ret;
    }

    public void update(String path, int bookmark, int subsave, int tracksave, int subcolorsave,
            int subsizesave) {
        long time = System.currentTimeMillis();
        SQLiteDatabase database = dbmanger.getWritableDatabase();

        database.execSQL("update " + MangerDatabase.NAME + " set " + MangerDatabase.PATH + "=?,"
                + MangerDatabase.BOOKMARK + "=?," + MangerDatabase.SUBSAVE + "=?,"
                + MangerDatabase.TRACKSAVE + "=?," + MangerDatabase.SUBCOLORSAVE + "=?,"
                + MangerDatabase.SUBSIZESAVE + "=? where " + MangerDatabase.TIME + "=?",
                new Object[] {path, bookmark, subsave, tracksave, subcolorsave, subsizesave, time});

    }

    public int findByPathReturnSeek(String path) {
        int ret = 0;
        SQLiteDatabase database = dbmanger.getWritableDatabase();

        Cursor cursor = database.rawQuery("select * from " + MangerDatabase.NAME + " where "
                + MangerDatabase.PATH + "=?", new String[] {path});
        if (cursor != null) {
            try {
                if (cursor.moveToNext()) {
                    int mIdIndex = cursor.getColumnIndexOrThrow(MangerDatabase.BOOKMARK);
                    ret = cursor.getInt(mIdIndex);
                }
            } finally {
                cursor.close();
            }
        }
        return ret;
    }

    public int findByPathReturnSubSave(String path) {
        int sub = 0;
        SQLiteDatabase database = dbmanger.getWritableDatabase();

        Cursor cursor = database.rawQuery("select * from " + MangerDatabase.NAME + " where "
                + MangerDatabase.PATH + "=?", new String[] {path});
        if (cursor != null) {
            try {
                if (cursor.moveToNext()) {
                    sub = cursor.getInt(cursor.getColumnIndex(MangerDatabase.SUBSAVE));
                }
            } finally {
                cursor.close();
            }
        }

        return sub;
    }

    public int findByPathReturnTrackSave(String path) {
        int track = 0;
        SQLiteDatabase database = dbmanger.getWritableDatabase();
        Cursor cursor = database.rawQuery("select * from " + MangerDatabase.NAME + " where "
                + MangerDatabase.PATH + "=?", new String[] {path});
        if (cursor != null) {
            try {
                if (cursor.moveToNext()) {
                    track = cursor.getInt(cursor.getColumnIndex(MangerDatabase.TRACKSAVE));
                }
            } finally {
                cursor.close();
            }
        }

        return track;
    }

    public int findByPathReturnSubColorSave(String path) {
        int subColor = -1;
        SQLiteDatabase database = dbmanger.getWritableDatabase();
        Cursor cursor = database.rawQuery("select * from " + MangerDatabase.NAME + " where "
                + MangerDatabase.PATH + "=?", new String[] {path});
        if (cursor != null) {
            try {
                if (cursor.moveToNext()) {
                    subColor = cursor.getInt(cursor.getColumnIndex(MangerDatabase.SUBCOLORSAVE));
                }
            } finally {
                cursor.close();
            }
        }

        return subColor;
    }

    public int findByPathReturnSubSizeSave(String path) {
        int subSize = 32;
        SQLiteDatabase database = dbmanger.getWritableDatabase();
        Cursor cursor = database.rawQuery("select * from " + MangerDatabase.NAME + " where "
                + MangerDatabase.PATH + "=?", new String[] {path});
        if (cursor != null) {
            try {
                if (cursor.moveToNext()) {
                    subSize = cursor.getInt(cursor.getColumnIndex(MangerDatabase.SUBSIZESAVE));
                }
            } finally {
                cursor.close();
            }
        }

        return subSize;
    }

    public int getCount() {
        long count = 0;

        SQLiteDatabase database = dbmanger.getWritableDatabase();

        Cursor cursor = database.rawQuery("select count(*) from " + MangerDatabase.NAME, null);
        if (cursor != null) {
            try {
                if (cursor.moveToLast()) {
                    count = cursor.getLong(0);
                }
            } finally {
                cursor.close();
            }
        }

        return (int) count;
    }

    public void close() {
        dbmanger.close();
    }
}
