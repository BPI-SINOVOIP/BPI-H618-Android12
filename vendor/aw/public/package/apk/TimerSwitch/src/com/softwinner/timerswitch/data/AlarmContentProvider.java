package com.softwinner.timerswitch.data;

import android.content.ContentProvider;
import android.content.ContentResolver;
import android.content.ContentUris;
import android.content.ContentValues;
import android.content.UriMatcher;
import android.database.Cursor;
import android.database.sqlite.SQLiteDatabase;
import android.net.Uri;
import android.util.Log;
import androidx.annotation.NonNull;
import androidx.annotation.Nullable;
import com.softwinner.timerswitch.utils.Constant;
import java.util.Objects;

public class AlarmContentProvider extends ContentProvider {

    public static final String TAG = "TAG_AlarmContentProvider";
    private static final String AUTHORITY = "com.softwinner.timerswitch";

    public final static int CODE_URI_INSERT = 0;
    public final static int CODE_URI_DELETE = 1;
    public final static int CODE_URI_UPDATE = 2;
    public final static int CODE_URI_QUERY = 3;

    private static final UriMatcher mUriMatcher = new UriMatcher(UriMatcher.NO_MATCH);
    static {
        mUriMatcher.addURI(AUTHORITY,"alarm/insert", CODE_URI_INSERT);
        mUriMatcher.addURI(AUTHORITY,"alarm/delete", CODE_URI_DELETE);
        mUriMatcher.addURI(AUTHORITY,"alarm/update", CODE_URI_UPDATE);
        mUriMatcher.addURI(AUTHORITY,"alarm/query", CODE_URI_QUERY);
    }

    private AlarmDatabase mAlarmDatabase;

    public AlarmContentProvider(){ }

    @Override
    public boolean onCreate() {
        mAlarmDatabase = new AlarmDatabase(getContext());
        return false;
    }

    @Nullable
    @Override
    public String getType(@NonNull Uri uri) {
        return null;
    }

    @Nullable
    @Override
    public Uri insert(@NonNull Uri uri, @Nullable ContentValues values) {
        if (mUriMatcher.match(uri) == CODE_URI_INSERT) {
            SQLiteDatabase db = mAlarmDatabase.getWritableDatabase();
            long rowId = db.insert(Constant.TABLE_NAME, null, values);
            Uri insertUri = ContentUris.withAppendedId(uri, rowId);
            notifyChange(Objects.requireNonNull(getContext()).getContentResolver(), insertUri);
            return insertUri;
        }
        throw new IllegalArgumentException("illegal uri : " + uri);
    }

    @Override
    public int delete(@NonNull Uri uri, @Nullable String where, @Nullable String[] whereArgs) {
        if (mUriMatcher.match(uri) == CODE_URI_DELETE) {
            int count;
            SQLiteDatabase db = mAlarmDatabase.getWritableDatabase();
            count = db.delete(Constant.TABLE_NAME, where, whereArgs);
            if (count > 0) {
                String rowId = "";
                if (null != whereArgs) {
                    rowId = whereArgs[0];
                }
                Uri deleteUri = Uri.withAppendedPath(uri, rowId);
                notifyChange(Objects.requireNonNull(getContext()).getContentResolver(), deleteUri);
            }
            return count;
        }
        throw new IllegalArgumentException("illegal uri : " + uri);
    }

    @Override
    public int update(@NonNull Uri uri, @Nullable ContentValues values, @Nullable String selection, @Nullable String[] selectionArgs) {
        if (mUriMatcher.match(uri) == CODE_URI_UPDATE) {
            int count;
            SQLiteDatabase db = mAlarmDatabase.getWritableDatabase();
            count = db.update(Constant.TABLE_NAME, values, selection, selectionArgs);
            if (count > 0) {
                String rowId = "";
                if (null != selectionArgs) {
                    rowId = selectionArgs[0];
                }
                Uri updateUri = Uri.withAppendedPath(uri, rowId);
                notifyChange(Objects.requireNonNull(getContext()).getContentResolver(), updateUri);
            }
            return count;
        }
        throw new IllegalArgumentException("illegal uri : " + uri);
    }

    @Nullable
    @Override
    public Cursor query(@NonNull Uri uri, @Nullable String[] projection, @Nullable String selection, @Nullable String[] selectionArgs, @Nullable String sortOrder) {
        if (mUriMatcher.match(uri) == CODE_URI_QUERY) {
            SQLiteDatabase db = mAlarmDatabase.getReadableDatabase();
            Cursor cursor = db.query(Constant.TABLE_NAME, null, selection, selectionArgs, null, null, sortOrder);
            if (cursor != null) {
                cursor.setNotificationUri(Objects.requireNonNull(getContext()).getContentResolver(), uri);
            }
            return cursor;
        }
        throw new IllegalArgumentException("illegal uri : " + uri);
    }

    private void notifyChange(ContentResolver resolver, Uri uri){
        resolver.notifyChange(uri, null);
    }
}
