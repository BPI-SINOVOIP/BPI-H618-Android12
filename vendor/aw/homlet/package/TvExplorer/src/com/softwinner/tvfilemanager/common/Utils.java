/*
 * Copyright (C) 2014 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License"); you may not use this file except
 * in compliance with the License. You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software distributed under the License
 * is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express
 * or implied. See the License for the specific language governing permissions and limitations under
 * the License.
 */

package com.softwinner.tvfilemanager.common;

import android.app.Dialog;
import android.content.ContentResolver;
import android.content.ContentUris;
import android.content.Context;
import android.database.Cursor;
import android.graphics.Bitmap;
import android.graphics.BitmapFactory;
import android.graphics.Point;
import android.media.ExifInterface;
import android.media.ThumbnailUtils;
import android.net.Uri;
import android.os.ParcelFileDescriptor;
import android.provider.MediaStore;
import android.util.Log;
import android.view.Display;
import android.view.LayoutInflater;
import android.view.View;
import android.view.WindowManager;
import android.view.animation.Animation;
import android.view.animation.AnimationUtils;
import android.widget.ImageView;
import android.widget.LinearLayout;
import android.widget.TextView;
import android.widget.Toast;

import com.softwinner.tvfilemanager.MyApplication;
import com.softwinner.tvfilemanager.R;
import com.softwinner.tvfilemanager.entity.LocalMusic;
import com.softwinner.tvfilemanager.entity.LocalPicture;
import com.softwinner.tvfilemanager.entity.LocalVideo;

import java.io.File;
import java.io.FileDescriptor;
import java.io.FileInputStream;
import java.io.FileNotFoundException;
import java.io.FileOutputStream;
import java.io.IOException;
import java.io.InputStream;
import java.io.OutputStream;
import java.lang.ref.SoftReference;
import java.lang.ref.WeakReference;

/**
 * A collection of utility methods, all static.
 */
public class Utils {

    private final static String TAG = "TvExplorerUtils";
    private static Context mContext = MyApplication.getContext();
    private static SoftReference<Bitmap> srf = null;

    /*
     * Making sure public utility methods remain static
     */
    private Utils() {
    }

    /**
     * Returns the screen/display size
     */
    public static Point getDisplaySize(Context context) {
        WindowManager wm = (WindowManager) context.getSystemService(Context.WINDOW_SERVICE);
        Display display = wm.getDefaultDisplay();
        Point size = new Point();
        display.getSize(size);
        return size;
    }

    /**
     * Shows a (long) toast
     */
    public static void showToast(Context context, String msg) {
        Toast.makeText(context, msg, Toast.LENGTH_LONG).show();
    }

    /**
     * Shows a (long) toast.
     */
    public static void showToast(Context context, int resourceId) {
        Toast.makeText(context, context.getString(resourceId), Toast.LENGTH_LONG).show();
    }

    public static int convertDpToPixel(Context ctx, int dp) {
        float density = ctx.getResources().getDisplayMetrics().density;
        return Math.round((float) dp * density);
    }

    /**
     * Formats time in milliseconds to hh:mm:ss string format.
     */
    public static String formatMillis(int millis) {
        String result = "";
        int hr = millis / 3600000;
        millis %= 3600000;
        int min = millis / 60000;
        millis %= 60000;
        int sec = millis / 1000;
        if (hr > 0) {
            result += hr + ":";
        }
        if (min >= 0) {
            if (min > 9) {
                result += min + ":";
            } else {
                result += "0" + min + ":";
            }
        }
        if (sec > 9) {
            result += sec;
        } else {
            result += "0" + sec;
        }
        return result;
    }

    public static LocalVideo buildLocalVideo(String path, String name) {
        LocalVideo localVideo = new LocalVideo();
        localVideo.setFile_path(path);
        localVideo.setFile_name(name);
//        localVideo.setVideo_thumbnail(thumbnail);
        return localVideo;
    }

    public static LocalPicture buildLocalPicture(String path, String name) {
        LocalPicture localPicture = new LocalPicture();
        localPicture.setPicture_path(path);
        localPicture.setPicture_name(name);
        return localPicture;
    }

    public static LocalMusic buildLocalMusic(String path, String name) {
        LocalMusic localMusic = new LocalMusic();
        localMusic.setMusic_path(path);
        localMusic.setMusic_name(name);
        return localMusic;
    }

    public static String saveBitmap2file(Bitmap bmp, String filename) {
        Bitmap.CompressFormat format = Bitmap.CompressFormat.JPEG;
        String filePath = mContext.getApplicationContext().getFilesDir().getAbsolutePath() + "/";
        filePath = filePath + filename;
        File file = new File(filePath);
        if (!file.exists()) {
            try {
                file.createNewFile();
            } catch (Exception e) {
                e.printStackTrace();
            }
        }
        int quality = 60;
        OutputStream stream = null;
        try {
            stream = new FileOutputStream(file);
        } catch (FileNotFoundException e) {
            // TODO Auto-generated catch block
            e.printStackTrace();
        }

        bmp.compress(format, quality, stream);
        return filePath;
    }

    public static Bitmap decodedBitmapFromFile(String filePath,
                                               int reqWidth, int reqHeight) {
        // 第一次解析将inJustDecodeBounds设置为true，来获取图片大小
        final BitmapFactory.Options options = new BitmapFactory.Options();
        options.inJustDecodeBounds = true;
        BitmapFactory.decodeFile(filePath, options);
        // 调用上面定义的方法计算inSampleSize值
        options.inSampleSize = calculateInSampleSize(options, reqWidth, reqHeight);
        // 使用获取到的inSampleSize值再次解析图片
        options.inJustDecodeBounds = false;
        options.inPreferredConfig = Bitmap.Config.RGB_565;
        Bitmap bitmap = null;
        try {
            bitmap = BitmapFactory.decodeFile(filePath, options);
        } catch (OutOfMemoryError e) {
            e.printStackTrace();
        }
        return bitmap;
    }

    public static int calculateInSampleSize(BitmapFactory.Options options,
                                            int reqWidth, int reqHeight) {
        // 源图片的高度和宽度
        final int height = options.outHeight;
        final int width = options.outWidth;
        int inSampleSize = 1;
        if (height > reqHeight || width > reqWidth) {
            // 计算出实际宽高和目标宽高的比率
            final int heightRatio = Math.round((float) height / (float) reqHeight);
            final int widthRatio = Math.round((float) width / (float) reqWidth);
            // 选择宽和高中最小的比率作为inSampleSize的值，这样可以保证最终图片的宽和高
            // 一定都会大于等于目标的宽和高。
            inSampleSize = heightRatio > widthRatio ? heightRatio : widthRatio;
        }
        return inSampleSize;
    }

    public static WeakReference<Bitmap> createBitmapFromFile(String filePath,
                                                             int minSideLen, int maxNumPixels) {
        Bitmap bitmap = null;

        // 第一次解析将inJustDecodeBounds设置为true，来获取图片大小
        final BitmapFactory.Options options = new BitmapFactory.Options();
        options.inJustDecodeBounds = true;
        BitmapFactory.decodeFile(filePath, options);
        // 调用上面定义的方法计算inSampleSize值
        options.inSampleSize = computeSampleSize(options, minSideLen, maxNumPixels);
        // 使用获取到的inSampleSize值再次解析图片
        options.inJustDecodeBounds = false;
        options.inPreferredConfig = Bitmap.Config.RGB_565;
        Log.e("ningxzUtils", "inSampleSize = " + options.inSampleSize);
        try {
            bitmap = BitmapFactory.decodeFile(filePath, options);
        } catch (OutOfMemoryError e) {
            e.printStackTrace();
        }

        WeakReference<Bitmap> thumbnail = new WeakReference<Bitmap>(bitmap);

        bitmap = null;

        return thumbnail;

    }

    //第一个参数:原本Bitmap的options
    //第二个参数:希望生成的缩略图的宽高中的较小的值
    //第三个参数:希望生成的缩量图的总像素
    public static int computeSampleSize(BitmapFactory.Options options,
                                        int minSideLength, int maxNumOfPixels) {
        int initialSize = computeInitialSampleSize(options, minSideLength,maxNumOfPixels);
        int roundedSize;
        if (initialSize <= 8) {
            roundedSize = 1;
            while (roundedSize < initialSize) {
                roundedSize <<= 1;
            }
        } else {
            roundedSize = (initialSize + 7) / 8 * 8;
        }
        Log.e("ningxzUtils" , "roundedSize = " + roundedSize);
        return roundedSize;
    }

    private static int computeInitialSampleSize(BitmapFactory.Options options,
                                                int minSideLength, int maxNumOfPixels) {
        //原始图片的宽
        double w = options.outWidth;
        //原始图片的高
        double h = options.outHeight;
        System.out.println("========== w="+w+",h="+h);
        int lowerBound = (maxNumOfPixels == -1) ? 1 : (int) Math.ceil(Math
                .sqrt(w * h / maxNumOfPixels));
        int upperBound = (minSideLength == -1) ? 128 : (int) Math.min(
                Math.floor(w / minSideLength), Math.floor(h / minSideLength));

        if (upperBound < lowerBound) {
            // return the larger one when there is no overlapping zone.
            return lowerBound;
        }
        if ((maxNumOfPixels == -1) && (minSideLength == -1)) {
            return 1;
        } else if (minSideLength == -1) {
            return lowerBound;
        } else {
            return upperBound;
        }
    }

    public static SoftReference<Bitmap> createPictureThumbnail(String filePath,
                                                               int reqWidth, int reqHeight) {

        Bitmap bitmap = null;

        FileInputStream stream = null;

        try {
            stream = new FileInputStream(filePath);
            FileDescriptor fd = stream.getFD();
            BitmapFactory.Options options = new BitmapFactory.Options();
            options.inSampleSize = 1;
            options.inJustDecodeBounds = true;
            BitmapFactory.decodeFileDescriptor(fd, null, options);
            if (options.mCancel || options.outWidth == -1
                    || options.outHeight == -1) {
                return null;
            }
            options.inSampleSize = calculateInSampleSize(
                    options, reqWidth, reqHeight)*2;
            Log.e("ningxzUtils", "inSampleSize = " + options.inSampleSize);
            options.inJustDecodeBounds = false;
            options.inPreferredConfig = Bitmap.Config.RGB_565;
            bitmap = BitmapFactory.decodeFileDescriptor(fd, null, options);
        } catch (IOException ex) {
            Log.e(TAG, "", ex);
        } catch (OutOfMemoryError oom) {
            Log.e(TAG, "Unable to decode file " + filePath + ". OutOfMemoryError.", oom);
        }finally {
            try {
                if (stream != null) {
                    stream.close();
                }
            } catch (IOException ex) {
                Log.e(TAG, "", ex);
            }
        }

        /*bitmap = ThumbnailUtils.extractThumbnail(bitmap, reqWidth, reqHeight,
                ThumbnailUtils.OPTIONS_RECYCLE_INPUT);
*/
        srf = new SoftReference<Bitmap>(bitmap);
        return srf;
    }

    /**
     ┊* 创建缩略图
     ┊*
     ┊* @param imageSrc
     ┊* @return 返回null,如果这时无法获得缩略图
     ┊*/
    public static Bitmap createThumbnail(String imageSrc) {

        boolean isJPG = false;
        Bitmap thumbnail = null;
        try {
            String ext = imageSrc.substring(imageSrc.lastIndexOf(".") + 1);
            if (ext.equalsIgnoreCase("jpg") || ext.equalsIgnoreCase("jpeg")) {
                isJPG = true;
            }
        } catch (IndexOutOfBoundsException e) {
            e.printStackTrace();
            return null;
        }

        if (isJPG) {
            try {
                ExifInterface mExif = null;
                mExif = new ExifInterface(imageSrc);
                if (mExif != null) {
                    Log.e("ningxzExif" ,"exif");
                    byte[] thumbData = mExif.getThumbnail();

                    if (thumbData == null) {
                        thumbnail = createThumbnailByOptions(imageSrc);

                    } else {
                        thumbnail = BitmapFactory.decodeByteArray(thumbData, 0,
                                thumbData.length);
                        Log.e("ningxzExif","fileSize = " + thumbData.length);
                    }
                } else {
                    thumbnail = createThumbnailByOptions(imageSrc);
                }
            } catch (IOException e) {
                // TODO Auto-generated catch block
                e.printStackTrace();
                return null;
            }
        } else {
            thumbnail = createThumbnailByOptions(imageSrc);
        }
        return thumbnail;
    }

    private static Bitmap createThumbnailByOptions(String imageSrc) {
        Bitmap thumb = null;
        BitmapFactory.Options options = new BitmapFactory.Options();
        options.inJustDecodeBounds = true;

        BitmapFactory.decodeFile(imageSrc, options);
        /*int be = (int) (Math.min(options.outWidth / width, options.outHeight
                / height));
        if (be <= 0)
            be = 1;*/
        options.inSampleSize = calculateInSampleSize(options, Constants.THUMBNAIL_WIDTH,
                                                                Constants.THUMBNAIL_HEGITH)*2;
        options.inJustDecodeBounds = false;
        options.inPreferredConfig = Bitmap.Config.RGB_565;
//        thumb = BitmapFactory.decodeFile(imageSrc, options);
        thumb = ThumbnailUtils.extractThumbnail(BitmapFactory.decodeFile(imageSrc, options),
                Constants.THUMBNAIL_WIDTH, Constants.THUMBNAIL_HEGITH,
                ThumbnailUtils.OPTIONS_RECYCLE_INPUT);
        if (thumb == null) {
            /* 当不可decode时返回null */
            return null;
        }
//        thumb = Bitmap.createScaledBitmap(thumb, Constants.CARD_WIDTH, Constants.CARD_HEIGHT, false);
        /*Log.d("MediaProvider",
                "image:" + String.valueOf(width) + "*" + String.valueOf(height));*/
        return thumb;
    }


private static final Uri sArtworkUri = Uri.parse("content://media/external/audio/albumart");
    private static final BitmapFactory.Options sBitmapOptions = new BitmapFactory.Options();
//    private static Bitmap mCachedBit = null;

    public static Bitmap getArtwork(Context context, long song_id, long album_id, boolean allowdefault) {

        if (album_id < 0) {
            // This is something that is not in the database, so get the album art directly
            // from the file.

            if (song_id >= 0) {
                Bitmap bm = getArtworkFromFile(context, song_id, -1);
                if (bm != null) {
                    return bm;
                }
            }
            if (allowdefault) {
                return getDefaultArtwork(context);
            }
            return null;
        }

        ContentResolver res = context.getContentResolver();
        Uri uri = ContentUris.withAppendedId(sArtworkUri, album_id);

        if (uri != null) {
            InputStream in = null;

            try {
                in = res.openInputStream(uri);
                return BitmapFactory.decodeStream(in, null, sBitmapOptions);
            } catch (FileNotFoundException ex) {

                // The album art thumbnail does not actually exist. Maybe the user deleted it, or
                // maybe it never existed to begin with.

                Bitmap bm = getArtworkFromFile(context, song_id, album_id);
                if (bm != null) {
                    if (bm.getConfig() == null) {
                        bm = bm.copy(Bitmap.Config.RGB_565, false);
                        if (bm == null && allowdefault) {
                            return getDefaultArtwork(context);
                        }
                    }
                } else if (allowdefault) {
                    bm = getDefaultArtwork(context);
                }
                return bm;

            } finally {
                try {
                    if (in != null) {
                        in.close();
                    }
                } catch (IOException ex) {
                    ex.printStackTrace();
                }
            }
        }
        return null;
    }

    //1、获取音乐文件专辑封面作为缩略图，在loadLocalMusicData里调用，这个可能会影响加载速度，
    // 后续考虑和video/picture一样单独开个线程去生成
    //这个方法：java.io.FileNotFoundException: No entry for content://media/external/audio/albumart/16
    public static Bitmap getMusic_thumbnail(String filePath) {

        String path = null;
        Long album_id = 0l;
        Long songid = 0l;
        Long albumid = 0l;
        Bitmap bmp = null;
        Cursor c = mContext.getContentResolver().query(
                MediaStore.Audio.Media.EXTERNAL_CONTENT_URI,
                new String[] { MediaStore.Audio.Media.TITLE,
                        MediaStore.Audio.Media.DURATION,
                        MediaStore.Audio.Media.ARTIST,
                        MediaStore.Audio.Media._ID,
                        MediaStore.Audio.Media.ALBUM,
                        MediaStore.Audio.Media.DISPLAY_NAME,
                        MediaStore.Audio.Media.DATA,
                        MediaStore.Audio.Media.ALBUM_ID}, null,null, null);

        if (c.moveToFirst()) {
            do {
                // 通过Cursor 获取路径，如果路径相同则break；
                System.out.println("////////"+filePath);
                path = c.getString(c
                        .getColumnIndexOrThrow(MediaStore.Audio.Media.DATA));
                System.out.println("?????????"+path);
                // 查找到相同的路径则返回，此时cursorPosition 便是指向路径所指向的Cursor 便可以返回了
                if (path.equals(filePath)) {

                    songid = c.getLong(3);
                    albumid = c.getLong(7);
                    Log.e(TAG, "songid: " + songid);
                    Log.e(TAG, "albumid: " + albumid);
                    bmp = Utils.getArtwork(mContext, songid, albumid,true);

                    break;
                }
            } while (c.moveToNext());
        }
        return bmp;
    }

    private static Bitmap getArtworkFromFile(Context context, long songid, long albumid) {

        Bitmap bm = null;
        byte[] art = null;
        String path = null;

        if (albumid < 0 && songid < 0) {
            throw new IllegalArgumentException("Must specify an album or a song id");
        }

        try {
            if (albumid < 0) {

                Uri uri = Uri.parse("content://media/external/audio/media/" + songid + "/albumart");
                ParcelFileDescriptor pfd = context.getContentResolver().openFileDescriptor(uri, "r");
                if (pfd != null) {
                    FileDescriptor fd = pfd.getFileDescriptor();
                    bm = BitmapFactory.decodeFileDescriptor(fd);
                }

            } else {
                Uri uri = ContentUris.withAppendedId(sArtworkUri, albumid);
                ParcelFileDescriptor pfd = context.getContentResolver().openFileDescriptor(uri, "r");
                if (pfd != null) {
                    FileDescriptor fd = pfd.getFileDescriptor();
                    bm = BitmapFactory.decodeFileDescriptor(fd);
                }
            }

        } catch (FileNotFoundException ex) {
            ex.printStackTrace();
        }

        return bm;
    }

    private static Bitmap getDefaultArtwork(Context context) {
        BitmapFactory.Options opts = new BitmapFactory.Options();
        opts.inPreferredConfig = Bitmap.Config.RGB_565;
        return BitmapFactory.decodeStream(
                context.getResources().openRawResource(R.drawable.thumbnail_music), null, opts);

    }

    //2、获取音乐文件专辑封面作为缩略图,
    //这个方法获取到的albumArt为null，考虑是mUriAlbums错误；
    private Bitmap getImage(String filePath){

        String path = null;
        Long album_id = 0l;

        Cursor c = mContext.getContentResolver().query(MediaStore.Audio.Media.EXTERNAL_CONTENT_URI,
                null, null, null, MediaStore.Audio.Media.DEFAULT_SORT_ORDER);

        if (c.moveToFirst()) {
            do {
                // 通过Cursor 获取路径，如果路径相同则break；
                System.out.println("////////"+filePath);
                path = c.getString(c
                        .getColumnIndexOrThrow(MediaStore.Audio.Media.DATA));
                System.out.println("?????????"+path);
                // 查找到相同的路径则返回，此时cursorPosition 便是指向路径所指向的Cursor 便可以返回了
                if (path.equals(filePath)) {
                    album_id  = c.getLong(c.getColumnIndexOrThrow(
                            MediaStore.Audio.Media.ALBUM_ID));

                    break;
                }
            } while (c.moveToNext());
        }

        Log.e(TAG, "album_id: " + album_id);
        String albumArt = getAlbumArt(album_id);
        Log.e(TAG, "albumArt: " + albumArt);
        Bitmap bmp = null;
        if (albumArt != null) {
            bmp = BitmapFactory.decodeFile(albumArt);
        }
        return bmp;
    }

    private String getAlbumArt(Long album_id) {
        String mUriAlbums = "content://media/external/audio/albums";
        String[] projection = new String[] { "album_art" };
        Cursor cur = mContext.getContentResolver().query(
                Uri.parse(mUriAlbums + "/" + Long.toString(album_id)),
                projection, null, null, null);
        String album_art = null;
        if (cur.getCount() > 0 && cur.getColumnCount() > 0) {
            cur.moveToNext();
            album_art = cur.getString(0);
        }
        cur.close();
        cur = null;
        return album_art;
    }


    /**
     * 自定义的progressDialog
     *
     * @param context
     * @param msg
     * @return
     */
    public static Dialog createLoadingDialog(Context context, String msg) {

        LayoutInflater inflater = LayoutInflater.from(context);
        View v = inflater.inflate(R.layout.processing_dialog, null);// 得到加载view
        LinearLayout layout = (LinearLayout) v.findViewById(R.id.dialog_view);// 加载布局
        // main.xml中的ImageView
        ImageView spaceshipImage = (ImageView) v.findViewById(R.id.img);
        TextView tipTextView = (TextView) v.findViewById(R.id.tipTextView);// 提示文字
        // 加载动画
        Animation hyperspaceJumpAnimation = AnimationUtils.loadAnimation(
                context, R.anim.process_animation);
        // 使用ImageView显示动画
        spaceshipImage.startAnimation(hyperspaceJumpAnimation);
        tipTextView.setText(msg);// 设置加载信息

        Dialog loadingDialog = new Dialog(context, R.style.loading_dialog);// 创建自定义样式dialog

        loadingDialog.setCancelable(false);// 不可以用“返回键”取消
        loadingDialog.setContentView(layout, new LinearLayout.LayoutParams(
                LinearLayout.LayoutParams.MATCH_PARENT,
                LinearLayout.LayoutParams.MATCH_PARENT));// 设置布局
        return loadingDialog;

    }
}
