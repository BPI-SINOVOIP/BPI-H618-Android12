/*
 * add by chenjd, chenjd@allwinnertech.com  20110919
 * when a file is created,modify or delete,it will used this class
 * to notify the MediaScanner to refresh the media database
 */

package com.softwinner.TvdFileManager;

import android.content.ContentResolver;
import android.content.ContentUris;
import android.content.Context;
import android.content.Intent;
import android.database.Cursor;
import android.net.Uri;
import android.provider.MediaStore.Audio;
import android.provider.MediaStore.Images;
import android.provider.MediaStore.Video;
import android.util.Log;

import java.io.File;
import java.lang.SecurityException;

public class RefreshMedia {
    private Context mContext;

    static final String EXTERNAL_VOLUME = "external";

    private static final String TAG = "RefreshMedia";

    public RefreshMedia(Context c) {
        this.mContext = c;
    }

    public void notifyMediaAdd(String file) {
        File mfile = new File(file);
        if (mfile.exists()) {
            try {
                /*
                 * notify the media to scan
                 */
                Uri mUri = Uri.fromFile(mfile);
                Intent mIntent = new Intent();
                mIntent.setData(mUri);
                mIntent.setAction(Intent.ACTION_MEDIA_SCANNER_SCAN_FILE);
                mContext.sendBroadcast(mIntent);
            } catch (Exception e) {
                Log.d("RefreshMedia", e.getMessage());
            }
        }
    }

    public void notifyMediaDelete(String file) throws SecurityException{
        final int idAudioColumnIndex = 0;
        final int pathAudioColumnIndex = 1;
        String[] projection = new String[] {Audio.Media._ID, Audio.Media.DATA, };
        Uri[] mediatypes = new Uri[] {Audio.Media.getContentUri(EXTERNAL_VOLUME),
                Video.Media.getContentUri(EXTERNAL_VOLUME),
                Images.Media.getContentUri(EXTERNAL_VOLUME), };
        ContentResolver cr = mContext.getContentResolver();
        Cursor c = null;

        for (int i = 0; i < mediatypes.length; i++) {
            try{
                c = cr.query(mediatypes[i], projection, null, null, null);
            }catch(Exception e){
            }
            if (c != null) {
                try {
                    while (c.moveToNext()) {
                        long rowId = c.getLong(idAudioColumnIndex);
                        String path = c.getString(pathAudioColumnIndex);
                         //获取该目录下的所有文件
                        if ( path.equals(file)|| path.startsWith(file+"/")) {
                            Log.d(TAG, "delete row " + rowId + "in table " + mediatypes[i]+"path:"+path);
                            cr.delete(ContentUris.withAppendedId(mediatypes[i], rowId), null, null);
                        }
                    }
                } finally {
                    c.close();
                    c = null;
                }
            }
        }
    }
}
