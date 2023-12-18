package com.allwinner.camera.utils;

import android.content.ContentResolver;
import android.content.ContentUris;
import android.database.Cursor;
import android.graphics.Bitmap;
import android.graphics.BitmapFactory;
import android.graphics.Matrix;
import android.media.MediaMetadataRetriever;
import android.net.Uri;
import android.os.Build;
import android.os.ParcelFileDescriptor;
import android.provider.MediaStore;
import android.util.Log;
import android.util.Size;

import com.allwinner.camera.data.CameraData;
import com.allwinner.camera.data.Contants;

import java.io.File;
import java.io.FileDescriptor;
import java.io.FileNotFoundException;
import java.io.IOException;
import java.util.Locale;

public class ThumbnailUtils {
    private static final String TAG = "ThumbnailUtils";
    public static Bitmap readBitmapFromByteArray(byte[] data,int inSampleSize) {
        Log.d(TAG, "readBitmapFromByteArray");
        if (data == null) {
            return null;
        }
        BitmapFactory.Options options = new BitmapFactory.Options();
        options.inJustDecodeBounds = false;
        options.inSampleSize = inSampleSize;
        Bitmap bitmap = BitmapFactory.decodeByteArray(data, 0, data.length, options);
        Log.d(TAG, "readBitmapFromByteArray end: " + bitmap.getWidth() + " x " + bitmap.getHeight());
        return bitmap;
    }

    public static Bitmap getThumbnailBitmap(byte[] data,int width, int height,int orientation){
        Log.i(TAG, "getThumbnailBitmap");
        int ratio = (int) Math
                .ceil((double)  Math.min(width,height) / CameraData.getInstance().getThumbnailSize());
        int inSampleSize = Integer.highestOneBit(ratio);
        Bitmap bitmap= readBitmapFromByteArray(data,inSampleSize);
        Matrix matrix = new Matrix();
        //90
        matrix.setRotate(orientation, bitmap.getWidth() * 0.5f, bitmap.getHeight() * 0.5f);
        Bitmap bitmap2 =  Bitmap.createBitmap(bitmap, 0, 0,bitmap.getWidth(),bitmap.getHeight() ,matrix,true);
        Log.i(TAG, "getThumbnailBitmap rotation: " + bitmap.getWidth() + "x" + bitmap.getHeight() + "orientation " + orientation);
        int finalsize =Math.min(bitmap2.getWidth(),bitmap2.getHeight());
        int startx =(int)( bitmap2.getWidth()*0.5f-finalsize*0.5f);
        int starty = (int)( bitmap2.getHeight()*0.5f-finalsize*0.5f);
        Bitmap bitmap3 = Bitmap.createBitmap(bitmap2, startx, starty,finalsize,finalsize);
        Log.i(TAG, "getThumbnailBitmap end: " + bitmap3.getWidth() + "x" + bitmap3.getHeight());
        bitmap.recycle();
        bitmap2.recycle();
        return bitmap3;
    }
    public static Bitmap getThumbnailBitmap(Bitmap bitmap,int orientation){
        Log.i(TAG, "getThumbnailBitmap");
        Matrix matrix = new Matrix();
        //90
        matrix.setRotate(orientation, bitmap.getWidth() * 0.5f, bitmap.getHeight() * 0.5f);
        Bitmap bitmap2 =  Bitmap.createBitmap(bitmap, 0, 0,bitmap.getWidth(),bitmap.getHeight() ,matrix,true);
        Log.i(TAG, "getThumbnailBitmap rotation: " + bitmap.getWidth() + "x" + bitmap.getHeight() + "orientation " + orientation);
        int finalsize =Math.min(bitmap2.getWidth(),bitmap2.getHeight());
        int startx =(int)( bitmap2.getWidth()*0.5f-finalsize*0.5f);
        int starty = (int)( bitmap2.getHeight()*0.5f-finalsize*0.5f);
        Bitmap bitmap3 = Bitmap.createBitmap(bitmap2, startx, starty,finalsize,finalsize);
        Log.i(TAG, "getThumbnailBitmap end: " + bitmap3.getWidth() + "x" + bitmap3.getHeight());
        bitmap.recycle();
        bitmap2.recycle();
        return bitmap3;
    }
    public static Bitmap getVideoThumbnailBitmap(String filePath,int orientation) {
        Log.i(TAG, "getVideoThumbnailBitmap");
        Bitmap bitmap = null;
        MediaMetadataRetriever retriever = new MediaMetadataRetriever();
        try {
            if (filePath != null) {
                if (!new File(filePath).exists()) {
                    filePath = filePath.substring(0, filePath.lastIndexOf("."));
                }
                retriever.setDataSource(filePath);
            }
            bitmap = retriever.getFrameAtTime(-1);
        } catch (IllegalArgumentException ex) {
            // Assume this is a corrupt video file
        } catch (RuntimeException ex) {
            // Assume this is a corrupt video file.
        } finally {
            try {
                retriever.release();
            } catch (RuntimeException ex) {
                // Ignore failures while cleaning up.
            }
        }
        if (bitmap == null)
            return null;

  /*      Matrix matrix = new Matrix();
        matrix.setRotate(orientation, bitmap.getWidth() * 0.5f, bitmap.getHeight() * 0.5f);
        Bitmap bitmap2 =  Bitmap.createBitmap(bitmap, 0, 0,bitmap.getWidth(),bitmap.getHeight() ,matrix,true);*/
        Log.i(TAG, "getVideoThumbnailBitmap rotation: " + bitmap.getWidth() + "x" + bitmap.getHeight() + "orientation " + orientation);
        Bitmap bitmap3 = getSquareBitmap(bitmap);
        Log.i(TAG, "getVideoThumbnailBitmap end: " + bitmap3.getWidth() + "x" + bitmap3.getHeight());
        bitmap.recycle();
        return bitmap3;
    }


    private static Contants.MediaData getLastPictureMediaData(ContentResolver resolver, long timeLimit) {
        Uri baseUri = MediaStore.Images.Media.EXTERNAL_CONTENT_URI;

        Uri query = baseUri.buildUpon().appendQueryParameter("limit", "1").build();
        String[] projection = new String[]{MediaStore.Images.ImageColumns._ID, MediaStore.Images.ImageColumns.ORIENTATION,
                MediaStore.Images.ImageColumns.DATE_TAKEN, MediaStore.Images.ImageColumns.DATA, MediaStore.Images.ImageColumns.BUCKET_ID};
        // if the image is deleted in the gallery, they will just add description "glr_dlt" or "glr_dlt_sub" to the photo,
        // but the photo doesn't been deleted in media database yet.
        // so we add ImageColumns.DESCRIPTION filter when getting the latest image/video from database.
        // "glr_dlt" means photo has been deleted in gallery, "glr_dlt_sub" is used for burst picture.
        String selection = "(" + MediaStore.Images.ImageColumns.DESCRIPTION + " IS NULL OR " +
                MediaStore.Images.ImageColumns.DESCRIPTION + " NOT IN ('glr_dlt', 'glr_dlt_sub')) AND ("
                + MediaStore.Images.ImageColumns.MIME_TYPE + "='image/jpeg') AND ("
                + MediaStore.Images.ImageColumns.BUCKET_ID + '=' + String.valueOf(CameraUtils.generateDCIM().toLowerCase(Locale.ENGLISH).hashCode())  + " )"
                + (timeLimit == -1 ? "" : " AND (" + MediaStore.Images.ImageColumns.DATE_TAKEN + ">=" + timeLimit + ")");
        String order = MediaStore.Images.ImageColumns.DATE_TAKEN + " DESC," + MediaStore.Images.ImageColumns._ID + " DESC";
        Cursor cursor = null;
        try {
            cursor = resolver.query(query, projection, selection, null, order);
            if (cursor != null && cursor.moveToFirst()) {

                long id = cursor.getLong(0);
                return new Contants.MediaData(cursor.getLong(0), cursor.getInt(1), cursor.getLong(2),
                        ContentUris.withAppendedId(baseUri, id), true, cursor.getString(3));
            }
        } catch (Exception e) {
            Log.e(TAG, "getLastPictureMedia Exception");
        } finally {
            if (cursor != null) {
                cursor.close();
            }
        }
        return null;
    }

    private static Contants.MediaData getLastRecordMediaData(ContentResolver resolver, long timeLimit) {
        Uri baseUri = MediaStore.Video.Media.EXTERNAL_CONTENT_URI;

        Uri query = baseUri.buildUpon().appendQueryParameter("limit", "1").build();
        String[] projection = new String[]{MediaStore.Video.VideoColumns._ID, MediaStore.MediaColumns.DATA,
                MediaStore.Video.VideoColumns.DATE_TAKEN, MediaStore.Video.VideoColumns.DATA};
        String selection = "(" + MediaStore.Images.ImageColumns.DESCRIPTION + " IS NULL OR " +
                MediaStore.Images.ImageColumns.DESCRIPTION + " NOT IN ('glr_dlt', 'glr_dlt_sub')) AND ("
                + MediaStore.Video.VideoColumns.BUCKET_ID + '=' + String.valueOf(CameraUtils.generateVideoPath().toLowerCase(Locale.ENGLISH).hashCode())  + ")"
                + (timeLimit == -1 ? "" : " AND (" + MediaStore.Video.VideoColumns.DATE_TAKEN + ">=" + timeLimit + ")");
        String order = MediaStore.Video.VideoColumns.DATE_TAKEN + " DESC," + MediaStore.Video.VideoColumns._ID + " DESC";

        Cursor cursor = null;
        try {
            cursor = resolver.query(query, projection, selection, null, order);
            if (cursor != null && cursor.moveToFirst()) {
                long id = cursor.getLong(0);
                return new Contants.MediaData(id, 0, cursor.getLong(2), ContentUris.withAppendedId(baseUri, id),
                        false, cursor.getString(3));
            }
        } catch (Exception e) {
            Log.e(TAG, "getLastRecordMediaData Exception");
        } finally {
            if (cursor != null) {
                cursor.close();
            }
        }
        return null;
    }

    private static Contants.MediaData getLastMedia(ContentResolver resolver, long timeLimit) {
        Contants.MediaData pictureMedia = getLastPictureMediaData(resolver, timeLimit);
        Contants.MediaData recordMedia = getLastRecordMediaData(resolver, timeLimit);
        if (pictureMedia != null || recordMedia != null) {
            if (pictureMedia != null && recordMedia != null) {
                if (pictureMedia.dateTaken > recordMedia.dateTaken) {
                    return pictureMedia;
                } else {
                    return recordMedia;
                }
            } else if (pictureMedia != null) {
                return pictureMedia;
            } else {
                return recordMedia;
            }
        }
        return null;
    }
    public static Bitmap getVideoThumbnailBitmapByfd(FileDescriptor fd,int orientation) {
        Log.i(TAG, "getVideoThumbnailBitmap");
        Bitmap bitmap = null;
        MediaMetadataRetriever retriever = new MediaMetadataRetriever();
        try {

            retriever.setDataSource(fd);
            bitmap = retriever.getFrameAtTime(-1);
        } catch (IllegalArgumentException ex) {
            // Assume this is a corrupt video file
        } catch (RuntimeException ex) {
            // Assume this is a corrupt video file.
        } finally {
            try {
                retriever.release();
            } catch (RuntimeException ex) {
                // Ignore failures while cleaning up.
            }
        }
        if (bitmap == null)
            return null;

  /*      Matrix matrix = new Matrix();
        matrix.setRotate(orientation, bitmap.getWidth() * 0.5f, bitmap.getHeight() * 0.5f);
        Bitmap bitmap2 =  Bitmap.createBitmap(bitmap, 0, 0,bitmap.getWidth(),bitmap.getHeight() ,matrix,true);*/
        Log.i(TAG, "getVideoThumbnailBitmap rotation: " + bitmap.getWidth() + "x" + bitmap.getHeight() + "orientation " + orientation);
        Bitmap bitmap3 = getSquareBitmap(bitmap);
        Log.i(TAG, "getVideoThumbnailBitmap end: " + bitmap3.getWidth() + "x" + bitmap3.getHeight());
        bitmap.recycle();
        return bitmap3;
    }
    public static Bitmap getLastThumbnailFromCR(ContentResolver resolver,long secureTimeLimit) {
        Contants.MediaData lastMedia = getLastMedia(resolver, secureTimeLimit);
        if (lastMedia == null) {
            return null;
        }
        Log.d(TAG, "get thumbnail from CR uri : " + lastMedia.uri +  "orientation: " +  lastMedia.orientation+lastMedia.path+"lastMedia.id:"+lastMedia.id);
        CameraData.getInstance().setPhotoUri(lastMedia.uri);
        CameraData.getInstance().setPhotoPath(lastMedia.path);
        Bitmap bitmap =null;
        ParcelFileDescriptor fileDescriptor;
        try {
                if (lastMedia.isPicture) {
//                    bitmap = MediaStore.Images.Thumbnails.getThumbnail(resolver, lastMedia.id,
//                            MediaStore.Images.Thumbnails.MICRO_KIND, null);
                    try {
                        fileDescriptor = resolver.openFileDescriptor(lastMedia.uri,"r");
                        bitmap = BitmapFactory.decodeFileDescriptor(fileDescriptor.getFileDescriptor());
                    } catch (FileNotFoundException e) {
                        e.printStackTrace();
                    }
                } else {
//                    bitmap = MediaStore.Video.Thumbnails.getThumbnail(resolver, lastMedia.id,
//                            MediaStore.Video.Thumbnails.MICRO_KIND, null);
                    try {
                        fileDescriptor = resolver.openFileDescriptor(lastMedia.uri,"r");
                        bitmap = getVideoThumbnailBitmapByfd(fileDescriptor.getFileDescriptor(),lastMedia.orientation);
                    } catch (FileNotFoundException e) {
                        e.printStackTrace();
                    }
                }

        } catch (OutOfMemoryError e) {
            Log.e(TAG, "getLastThumbnailFromCR OutOfMemoryError");
        } catch (RuntimeException e) {
            Log.e(TAG, "getLastThumbnailFromCR RuntimeException:" + e.getMessage());
        }
        if(bitmap == null){
            return null;
        }
        Matrix matrix = new Matrix();
        matrix.setRotate(lastMedia.orientation, bitmap.getWidth() * 0.5f, bitmap.getHeight() * 0.5f);
        Bitmap bitmap2 =  Bitmap.createBitmap(bitmap, 0, 0,bitmap.getWidth(),bitmap.getHeight() ,matrix,true);
        Bitmap bitmap3 = getSquareBitmap(bitmap2);
        Log.i(TAG, "getThumbnailBitmap end: " + bitmap3.getWidth() + "x" + bitmap3.getHeight());
        //bitmap.recycle();
        //bitmap2.recycle();
        return bitmap3;
    }

    public static Bitmap getSquareBitmap(Bitmap origin){
        int finalsize =Math.min(origin.getWidth(),origin.getHeight());
        int startx =(int)( origin.getWidth()*0.5f-finalsize*0.5f);
        int starty = (int)( origin.getHeight()*0.5f-finalsize*0.5f);
        Bitmap bitmap = Bitmap.createBitmap(origin, startx, starty,finalsize,finalsize);
        return bitmap;
    }
}
