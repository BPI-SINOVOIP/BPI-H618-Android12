package com.allwinner.camera.utils;

import android.Manifest;
import android.annotation.SuppressLint;
import android.annotation.TargetApi;
import android.content.ContentResolver;
import android.content.ContentValues;
import android.content.Context;
import android.content.pm.PackageManager;
import android.content.res.AssetManager;
import android.graphics.Bitmap;
import android.graphics.BitmapFactory;
import android.graphics.PointF;
import android.hardware.Camera;
import android.media.ExifInterface;
import android.net.Uri;
import android.opengl.Matrix;
import android.os.Build;
import android.os.Environment;
import android.os.ParcelFileDescriptor;
import android.os.StatFs;
import android.provider.MediaStore;
import android.util.DisplayMetrics;
import android.util.Log;
import android.util.Size;
import android.view.OrientationEventListener;
import android.view.View;
import android.view.WindowManager;
import android.view.animation.AlphaAnimation;
import android.view.animation.Animation;

import com.alibaba.android.mnnkit.actor.FaceDetector;
import com.alibaba.android.mnnkit.entity.FaceDetectConfig;
import com.alibaba.android.mnnkit.entity.FaceDetectionReport;
import com.alibaba.android.mnnkit.entity.MNNCVImageFormat;
import com.alibaba.android.mnnkit.entity.MNNFlipType;
import com.allwinner.camera.data.CameraData;
import com.allwinner.camera.data.Contants;
import com.allwinner.camera.data.DynamicSticker;
import com.allwinner.camera.data.DynamicStickerData;
import com.allwinner.camera.data.DynamicStickerNormalData;
import com.allwinner.camera.ui.UIManager;

import org.json.JSONArray;
import org.json.JSONException;
import org.json.JSONObject;

import java.io.BufferedInputStream;
import java.io.BufferedOutputStream;
import java.io.BufferedReader;
import java.io.ByteArrayOutputStream;
import java.io.File;
import java.io.FileDescriptor;
import java.io.FileInputStream;
import java.io.FileOutputStream;
import java.io.IOException;
import java.io.InputStream;
import java.io.InputStreamReader;
import java.io.OutputStream;
import java.text.SimpleDateFormat;
import java.util.ArrayList;
import java.util.Date;
import java.util.Locale;
import java.util.concurrent.TimeUnit;
import java.util.concurrent.locks.ReentrantLock;

import static com.allwinner.camera.utils.ThumbnailUtils.readBitmapFromByteArray;

public class CameraUtils {
    private static final String TAG = "CameraUtils";
    public static String STORAGE_PATH = Environment.getExternalStorageDirectory().toString();
    public static ReentrantLock reentrantLock = new ReentrantLock();
    private final static int MAX_RESULT = 10;
    // 视椎体缩放倍数，具体数据与setLookAt 和 frustumM有关
    // 备注：setLookAt 和 frustumM 设置的结果导致了视点(eye)到近平面(near)和视点(eye)到贴纸(center)恰好是2倍的关系
    private static final float ProjectionScale = 2.0f;

    public static String generateDCIM() {
        File dir = new File(generateDCIMBase(),"/Camera");
        if (!dir.exists()) {
            dir.mkdirs();
        }
        return dir.toString();
    }
    public static String generateDCIMBase() {
        String path = new File(STORAGE_PATH, Environment.DIRECTORY_DCIM).toString();
        return path;
    }
    public static String generateVideoPath() {
        File dir = new File(generateDCIMBase(),"/Camera");
        if (!dir.exists()) {
            dir.mkdirs();
        }
        return dir.toString();
    }

    public static String generatePicName(long time) {
        SimpleDateFormat format = new SimpleDateFormat("'P'yMMdd-HHmmss");
        Date date = new Date(time);
        String name = format.format(date);
        return name;
    }

    public static String generateVideoTitle(long time) {
        SimpleDateFormat format = new SimpleDateFormat("yyyy-MM-dd-HH-mm-ss");
        String name = format.format(time);
        return name;
    }

    public static String generateFilePath(String title) {
        return generateDCIM() + '/' + title + ".jpg";
    }

    public static String generateFileYUVPath(String title) {
        return generateDCIM() + '/' + title + ".yuv";
    }

    public static String generateVideoFilePath(String title) {
        return generateVideoPath() + '/' + title + ".mp4";
    }

    public static String generateVideoFilePreviewPath(Context context, String title) {
        return context.getExternalFilesDir("Video").getAbsolutePath() + '/' + title + ".mp4";
    }

    public static String generateVideoFileTempPath(Context context, String title) {
        return context.getExternalFilesDir("Temp").getAbsolutePath() + '/' + title + ".mp4";
    }

    public static boolean checkImgDamage(String filePath) {
        BitmapFactory.Options options = null;
        if (options == null) {
            options = new BitmapFactory.Options();
        }
        options.inJustDecodeBounds = true;

        BitmapFactory.decodeFile(filePath, options);
        if (options.mCancel || options.outWidth == -1
                || options.outHeight == -1) {
            return true;
        }
        return false;
    }

    public static void CopyTempToVideo(ContentResolver resolver, Uri uri, String videoSavePath) {
        BufferedInputStream inputStream = null;
        OutputStream os = null;
        try {
            inputStream = new BufferedInputStream(new FileInputStream(videoSavePath));
            if (uri != null) {
                os = resolver.openOutputStream(uri);
            }
            if (os != null) {
                byte[] buffer = new byte[1024 * 4];
                int len;
                while ((len = inputStream.read(buffer)) != -1) {
                    os.write(buffer, 0, len);
                }
                os.flush();
            }
        } catch (IOException e) {
            Log.e(TAG, "Failed to copy data", e);
        } finally {
            try {
                os.close();
            } catch (IOException e) {
                e.printStackTrace();
            }
        }
    }

    @TargetApi(Build.VERSION_CODES.N)
    public static void addExif(Context context, Uri uri, long date) {
        ParcelFileDescriptor fileDescriptor = null;
        if(uri != null) {

            try {

                ExifInterface exifInterface = null;
                fileDescriptor = context.getContentResolver().openFileDescriptor(uri, "rw");
                exifInterface = new ExifInterface(fileDescriptor.getFileDescriptor());
                //  }
                SimpleDateFormat df = new SimpleDateFormat("yyyy:MM:dd kk:mm:ss", Locale.ENGLISH);
                String tagDate = df.format(date);
                exifInterface.setAttribute(ExifInterface.TAG_MAKE, "AW");
                exifInterface.setAttribute(ExifInterface.TAG_MODEL, "aw");
                exifInterface.setAttribute(ExifInterface.TAG_SOFTWARE, "a50");
                exifInterface.setAttribute(ExifInterface.TAG_DATETIME, tagDate);
                exifInterface.setAttribute(ExifInterface.TAG_DATETIME_DIGITIZED, tagDate);
                exifInterface.setAttribute(ExifInterface.TAG_DATETIME_ORIGINAL, tagDate);
                if (CameraData.getInstance().getIsSaveLocation()) {
                    if (LocationUtils.getInstance(context).getLocation() != null) {
                        Log.d(TAG, "Longitude:" + LocationUtils.getInstance(context).getLocation().getLongitude() + "Latitude:" + LocationUtils.getInstance(context).getLocation().getLatitude());
                        exifInterface.setAttribute(ExifInterface.TAG_GPS_LATITUDE, convert(LocationUtils.getInstance(context).getLocation().getLatitude()));
                        exifInterface.setAttribute(ExifInterface.TAG_GPS_LATITUDE_REF, latitudeRef(LocationUtils.getInstance(context).getLocation().getLatitude()));
                        exifInterface.setAttribute(ExifInterface.TAG_GPS_LONGITUDE, convert(LocationUtils.getInstance(context).getLocation().getLongitude()));
                        exifInterface.setAttribute(ExifInterface.TAG_GPS_LONGITUDE_REF, longitudeRef(LocationUtils.getInstance(context).getLocation().getLongitude()));
                    }
                }
                exifInterface.saveAttributes();
            } catch (IOException e) {
                Log.e(TAG, "failed to write ar exif: " + e.getMessage());
            }
            if(fileDescriptor!= null) {
                try {
                    fileDescriptor.close();
                } catch (IOException e) {
                    e.printStackTrace();
                }
                fileDescriptor = null;
            }
        }
    }

    private static final String convert(double latitude) {
        latitude = Math.abs(latitude);
        int degree = (int) latitude;
        latitude *= 60;
        latitude -= (degree * 60.0d);
        int minute = (int) latitude;
        latitude *= 60;
        latitude -= (minute * 60.0d);
        int second = (int) (latitude * 1000.0d);

        StringBuilder sb = new StringBuilder(20);
        sb.append(degree);
        sb.append("/1,");
        sb.append(minute);
        sb.append("/1,");
        sb.append(second);
        sb.append("/1000,");
        return sb.toString();
    }

    private static String latitudeRef(double latitude) {
        return latitude < 0.0d ? "S" : "N";
    }

    private static String longitudeRef(double longitude) {
        return longitude < 0.0d ? "W" : "E";
    }

    public static Uri insertMedia(ContentResolver resolver, String title, long date,
                                  int orientation, String path, int width, int height) {
        File file = new File(path);
        long dateModifiedSeconds = TimeUnit.MILLISECONDS.toSeconds(file.lastModified());

        // Insert into MediaStore.
        ContentValues values = new ContentValues(10);
        values.put(MediaStore.Images.ImageColumns.TITLE, title);
        values.put(MediaStore.Images.ImageColumns.DISPLAY_NAME, title + ".jpg");
        values.put(MediaStore.Images.ImageColumns.DATE_TAKEN, date);
        values.put(MediaStore.Images.ImageColumns.DATE_MODIFIED, dateModifiedSeconds);
        values.put(MediaStore.Images.ImageColumns.MIME_TYPE, "image/jpeg");
        values.put(MediaStore.Images.ImageColumns.ORIENTATION, orientation);
        //  values.put(MediaStore.Images.ImageColumns.DATA, values);

        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.Q) {
            //android Q中不再使用DATA字段，而用RELATIVE_PATH代替
            //RELATIVE_PATH是相对路径不是绝对路径
            //DCIM是系统文件夹，关于系统文件夹可以到系统自带的文件管理器中查看，不可以写没存在的名字
            values.put(MediaStore.Images.Media.RELATIVE_PATH, "DCIM/Camera");
        } else {
            //Android Q以下版本
            //   values.put(MediaStore.Images.ImageColumns.DATA, path);
        }
        values.put(MediaStore.Images.ImageColumns.DATA, path);
        values.put(MediaStore.Images.ImageColumns.WIDTH, width);
        values.put(MediaStore.Images.ImageColumns.HEIGHT, height);

        Uri uri = null;
        uri = CameraUtils.insertMedia(resolver, MediaStore.Images.Media.EXTERNAL_CONTENT_URI, values);
        CameraData.getInstance().setPhotoUri(uri);
        CameraData.getInstance().setPhotoPath(path);
        Log.i(TAG, "addImage path : " + path);
        Log.i(TAG, "addImage uri : " + uri + "date:" + date);
        return uri;
    }
    public static void writeFile(ContentResolver resolver, Uri insertUri, byte[] data) {
        Log.e(TAG, "writeFile : " + insertUri);
//        FileOutputStream out = null;
//        try {
//            out = new FileOutputStream(path);
//            out.write(data);
//        } catch (Exception e) {
//            Log.e(TAG, "Failed to write data", e);
//        } finally {
//            try {
//                out.close();
//            } catch (Exception e) {
//            }
//        }
        OutputStream os = null;
        try {
            if (insertUri != null) {
                os = resolver.openOutputStream(insertUri);
            }
            if (os != null) {
                os.write(data);
                os.flush();
            }

        } catch (IOException e) {
            Log.e(TAG, "Failed to write data", e);
        } finally {
            try {
                if(os!= null) {
                    os.close();
                }
            } catch (IOException e) {
                e.printStackTrace();
            }
        }
    }

    public static Bitmap getSquarePictrueBitmap(byte[] data, int width, int height) {
        Log.i(TAG, "getThumbnailBitmap");
        Bitmap bitmap = readBitmapFromByteArray(data, 1);
        Log.i(TAG, "getThumbnailBitmap rotation: " + bitmap.getWidth() + "x" + bitmap.getHeight());
        int finalsize = Math.min(bitmap.getWidth(), bitmap.getHeight());
        int startx = (int) (bitmap.getWidth() * 0.5f - finalsize * 0.5f);
        int starty = (int) (bitmap.getHeight() * 0.5f - finalsize * 0.5f);
        Bitmap bitmap2 = Bitmap.createBitmap(bitmap, startx, starty, finalsize, finalsize);
        Log.i(TAG, "getThumbnailBitmap end: " + bitmap2.getWidth() + "x" + bitmap2.getHeight() + "startx:" + startx + "starty:" + starty);
        bitmap.recycle();
        return bitmap2;
    }

    public static Uri insertMedia(ContentResolver resolver, Uri table, ContentValues values) {
        Uri uri = null;
        try {
            uri = resolver.insert(table, values);
        } catch (Throwable th) {
            Log.e(TAG, "Failed to write MediaStore" + th);
        }
        return uri;
    }
    public static int updateMedia(ContentResolver resolver,  ContentValues values,Uri uri) {
        int code = -1;
        try {
            code = resolver.update(uri, values,null,null);
        } catch (Throwable th) {
            Log.e(TAG, "Failed to write MediaStore" + th);
        }
        return code;
    }
    public static int getPreviewRotation(int cameraId) {
        int rotation = 0;
        //int orientation = CameraData.getInstance().getOrientation();
        int orientation = CameraData.getInstance().getDisplayRotation();
        Camera.CameraInfo info = new Camera.CameraInfo();
        if (orientation != OrientationEventListener.ORIENTATION_UNKNOWN) {
            if (CameraData.getInstance().getBackCameraId() == cameraId) {
                Camera.getCameraInfo(cameraId, info);
                rotation = (CameraData.getInstance().getBackCameraOrientation() - orientation + 360) % 360;
            } else { // FRONT-facing camera
                rotation = (CameraData.getInstance().getFrontCameraOrientation() + orientation) % 360;
            }
            //   Log.i(TAG, "orientation:"+orientation+"getJpegRotation rotation: " + rotation + " info.orientation " + info.orientation+ " getSensorOrientation:" + CameraData.getInstance().getSensorOrientation());
        }
        return rotation;
    }

    public static int getJpegRotation(int cameraId) {
        int rotation = 0;
        int orientation = CameraData.getInstance().getOrientation();
        //int orientation = CameraData.getInstance().getDisplayRotation();
        if (orientation != OrientationEventListener.ORIENTATION_UNKNOWN) {
            if (CameraData.getInstance().getFrontCameraId() == cameraId) {
                rotation = (CameraData.getInstance().getFrontCameraOrientation() - orientation + 360) % 360;
            } else { // FRONT-facing camera
                rotation = (CameraData.getInstance().getBackCameraOrientation() + orientation) % 360;
            }
            //Log.i(TAG, "orientation:"+orientation+"getJpegRotation rotation: " + rotation + " info.orientation " + info.orientation+ " getSensorOrientation:" + CameraData.getInstance().getSensorOrientation());
        }

        return rotation;
    }

    public static int getInAngle() {
        return CameraData.getInstance().isCurrentFront() ? (CameraData.getInstance().getFrontCameraOrientation() + 360 - CameraData.getInstance().getOrientation()) % 360 :
                (CameraData.getInstance().getBackCameraOrientation() + CameraData.getInstance().getOrientation()) % 360;
    }

    public static int getOutAngle() {
        return CameraData.getInstance().isCurrentFront() ? (360 - CameraData.getInstance().getOrientation()) % 360 : CameraData.getInstance().getOrientation() % 360;
    }

    public static boolean writeYuvFile(String path, byte[] data) {
        FileOutputStream out = null;
        try {
            File file = new File(path);
            File parent = file.getParentFile();
            if (parent != null && !parent.exists())
                parent.mkdirs();
            if (!file.exists()) {
                boolean isok = file.createNewFile();
                out = new FileOutputStream(path);
                out.write(data);
                FileDescriptor fd = out.getFD();
                fd.sync();
                return true;
            }
        } catch (Exception e) {
            e.printStackTrace();

        } finally {
            try {
                if (out != null)
                    out.close();
            } catch (Exception e) {
            }
        }
        return false;
    }


    public static void saveJpeg(ContentResolver contentResolver, Uri uri, Bitmap bitmap) {
        Log.i(TAG, "SaveJpeg:" + uri);
        BufferedOutputStream bos = null;
        OutputStream fileOutput = null;
        if (bitmap != null) {
            try {
                fileOutput = contentResolver.openOutputStream(uri);
                //fileOutput = new OutputStream(jpegpath);
                bos = new BufferedOutputStream(fileOutput);
                bitmap.compress(Bitmap.CompressFormat.JPEG, 95, bos);
            } catch (IOException e) {
                Log.e(TAG, "FileOutputStream fail to call the write() method");
                e.printStackTrace();
            } finally {
                if (bos != null) {
                    try {
                        if (fileOutput != null) {
                            fileOutput.close();
                        }
                        bos.flush();
                        bos.close();
                    } catch (IOException e) {
                        e.printStackTrace();
                        Log.e(TAG, "fail to close fops");
                    }
                }
                //bitmap.recycle();
            }
        }
        Log.i(TAG, "SaveJpeg end");
    }


    public static String loadFromAssets(Context c, String path) {
        InputStream in = null;
        String result = null;
        try {
            in = c.getResources().getAssets().open(path);
            int ch = 0;
            ByteArrayOutputStream baos = new ByteArrayOutputStream();
            while ((ch = in.read()) != -1) {
                baos.write(ch);
            }
            byte[] buff = baos.toByteArray();
            baos.close();
            in.close();
            result = new String(buff, "UTF-8");
            result = result.replaceAll("\\r\\n", "\n");
        } catch (Exception e) {
            e.printStackTrace();
        } finally {
            try {
                in.close();
            } catch (IOException e) {
                e.printStackTrace();
            }
        }
        return result;
    }

    public static float view2openglX(int x, int width) {
       /* float centerX = width/2.0f;
        float t = x - centerX;
        return t/centerX;*/
        return (float) x / (float) width;
    }

    public static float view2openglY(int y, int height) {
      /*  float centerY = height/2.0f;
        float s = centerY - y;
        return s/centerY;*/
        return (float) y / (float) height;
    }

    public static FaceDetectionReport[] doFaceDetector(UIManager uiManager, FaceDetector faceDetector, byte[] data, int width, int height, int inAngle, int outAngle, boolean front, boolean capture) {
        long detectConfig = FaceDetectConfig.ACTIONTYPE_EYE_BLINK | FaceDetectConfig.ACTIONTYPE_MOUTH_AH | FaceDetectConfig.ACTIONTYPE_HEAD_YAW | FaceDetectConfig.ACTIONTYPE_HEAD_PITCH | FaceDetectConfig.ACTIONTYPE_BROW_JUMP;
        FaceDetectionReport[] results = faceDetector.inference(data, width, height, MNNCVImageFormat.YUV_NV21, detectConfig, inAngle, outAngle,
                CameraData.getInstance().getCameraId() == CameraData.getInstance().getFrontCameraId() ? MNNFlipType.FLIP_Y : MNNFlipType.FLIP_NONE);
        uiManager.setFaceResult(results);
        return results;
    }

    /**
     * 求两点之间的距离
     * @param x1
     * @param y1
     * @param x2
     * @param y2
     * @return
     */
    public static double getDistance(float x1, float y1, float x2, float y2) {
        return Math.sqrt(Math.pow(x1 - x2, 2) + Math.pow(y1 - y2, 2));
    }

    static public PointF getRealPoint(float x, float y, int width, int height, boolean front, boolean isCapture) {
        if (isCapture) {
            Log.e(TAG, "CameraData.getInstance().getOrientation():" + CameraData.getInstance().getOrientation());
            if (CameraData.getInstance().getOrientation() == 90 || CameraData.getInstance().getOrientation() == 270) {
                return new PointF(x / (float) (Math.max(width, height)), y / (float) (Math.min(width, height)));
            } else {
                return new PointF(x / (float) (Math.min(width, height)), y / (float) (Math.max(width, height)));
            }
        } else if (front) {
            float rx = 1 - y / (float) height;
            float ry = 1 - x / (float) width;
            return new PointF(rx, ry);
        } else {
            float rx = y / (float) height;
            float ry = 1 - x / (float) width;
            return new PointF(rx, ry);
        }
    }

    static public PointF getRealPoint(FaceDetectionReport[] faceResult, int index, boolean isCapture, int faceWidth, int faceHeight) {
        return CameraUtils.getRealPoint(faceResult[0].keyPoints[index], faceResult[0].keyPoints[index + 1], faceWidth, faceHeight, CameraData.getInstance().isCurrentFront(), isCapture);
    }

    static public PointF  getStickFacePoint(float x, float y, int width, int height, boolean front, boolean isCapture) {
        if (isCapture) {
            Log.e(TAG, "CameraData.getInstance().getOrientation():" + CameraData.getInstance().getOrientation());
            if (CameraData.getInstance().getOrientation() == 90 || CameraData.getInstance().getOrientation() == 270) {
                return new PointF(x / (float) (Math.max(width, height)), 1 - y / (float) (Math.min(width, height)));
            } else {
                return new PointF(x / (float) (Math.min(width, height)), 1 - y / (float) (Math.max(width, height)));
            }
        } else  {
            float rx =x / (float) width;
            float ry = 1 - y / (float) height;
            return new PointF(rx, ry);
        }
    }
    static public PointF getStickFacePoint(FaceDetectionReport[] faceResult,int index,int faceWidth, int faceHeight, boolean isCapture){
        return CameraUtils.getStickFacePoint(faceResult[0].keyPoints[index],faceResult[0].keyPoints[index+1],faceWidth, faceHeight, CameraData.getInstance().isCurrentFront(), isCapture);
    }
    /**
     * Fade in a view from startAlpha to endAlpha during duration milliseconds
     *
     * @param view
     * @param startAlpha
     * @param endAlpha
     * @param duration
     */
    public static void fadeIn(View view, float startAlpha, float endAlpha, long duration) {
        if (view.getVisibility() == View.VISIBLE) {
            return;
        }

        view.setVisibility(View.VISIBLE);
        Animation animation = new AlphaAnimation(startAlpha, endAlpha);
        animation.setDuration(duration);
        view.startAnimation(animation);
    }



    public static boolean isSupprtFaceBeauty(Contants.ModeType modeType) {
        return modeType.equals(Contants.ModeType.AutoMode);
    }

    @SuppressLint("NewApi")
    public static Size getPreviewUiSize(Context context, Size previewSize) {
        WindowManager windowManager = (WindowManager) context.getSystemService(Context
                .WINDOW_SERVICE);
        DisplayMetrics metrics = new DisplayMetrics();
        windowManager.getDefaultDisplay().getMetrics(metrics);
        double ratio = previewSize.getWidth() / (double) previewSize.getHeight();
        int w = (int) Math.ceil(metrics.widthPixels * ratio);
        int h = metrics.widthPixels;
        return new Size(w, h);
    }
    public static long  getAvailableSpace(){
        String state = Environment.getExternalStorageState();
        Log.d(TAG, " External storage state=" + state);
        if (Environment.MEDIA_CHECKING.equals(state)) {
            return Contants.PREPARING;
        }
        if (!Environment.MEDIA_MOUNTED.equals(state)) {
            return Contants.UNAVAILABLE;
        }
        try {
            StatFs stat = new StatFs(generateDCIM());
            Log.d(TAG, "getAvailableSpace =" + stat.getAvailableBlocks() * (long) stat.getBlockSize());
            return stat.getAvailableBlocks() * (long) stat.getBlockSize();

        } catch (Exception e) {
            Log.i(TAG, "Fail to access external storage", e);
        }
        return Contants.UNKNOWN_SIZE;

    }

    /**
     * 从Stream中获取String
     * @param inputStream
     * @return
     * @throws IOException
     */
    public static String convertToString(InputStream inputStream)
            throws IOException {
        BufferedReader localBufferedReader = new BufferedReader(new InputStreamReader(inputStream));

        StringBuilder localStringBuilder = new StringBuilder();
        String str;
        while ((str = localBufferedReader.readLine()) != null) {
            localStringBuilder.append(str).append("\n");
        }
        return localStringBuilder.toString();
    }
    /**
     * 读取默认动态贴纸数据
     *
     * @return
     * @throws IOException
     * @throws JSONException
     */
    public static DynamicSticker decodeStickerData(Context context,String stickerName)
            throws IOException, JSONException {
        //获得assets资源管理器
        AssetManager assetManager = context.getAssets();
        String stickerJson = convertToString(assetManager.open(Contants.STICKER_ROOT_FOLDER+"/"+stickerName+"/json"));

        JSONObject jsonObject = new JSONObject(stickerJson);
        DynamicSticker dynamicSticker = new DynamicSticker();
        dynamicSticker.folderPath =Contants.STICKER_ROOT_FOLDER+ "/"+stickerName;
        if (dynamicSticker.dataList == null) {
            dynamicSticker.dataList = new ArrayList<>();
        }
        JSONArray stickerList = jsonObject.getJSONArray("stickerList");
        for (int i = 0; i < stickerList.length(); i++) {
            JSONObject jsonData = stickerList.getJSONObject(i);
            String type = jsonData.getString("type");
            DynamicStickerData data = null;
            if ("sticker".equals(type)) {
                data = new DynamicStickerNormalData();
                JSONArray centerIndexList = jsonData.getJSONArray("centerIndexList");
                ((DynamicStickerNormalData) data).centerIndexList = new int[centerIndexList.length()];
                for (int j = 0; j < centerIndexList.length(); j++) {
                    ((DynamicStickerNormalData) data).centerIndexList[j] = centerIndexList.getInt(j);
                }
                ((DynamicStickerNormalData) data).offsetX = (float) jsonData.getDouble("offsetX");
                ((DynamicStickerNormalData) data).offsetY = (float) jsonData.getDouble("offsetY");
                ((DynamicStickerNormalData) data).baseScale = (float) jsonData.getDouble("baseScale");
                ((DynamicStickerNormalData) data).startIndex = jsonData.getInt("startIndex");
                ((DynamicStickerNormalData) data).endIndex = jsonData.getInt("endIndex");
            }else if ("static".equals(type)) {//静态贴纸

            } else {
                // 如果不是贴纸又不是前景的话，则直接跳过
                if (!"frame".equals(type)) {
                    continue;
                }
            }
            DynamicStickerData stickerData = data;
            stickerData.width = jsonData.getInt("width");
            stickerData.height = jsonData.getInt("height");
            stickerData.frames = jsonData.getInt("frames");
            stickerData.action = jsonData.getInt("action");
            stickerData.childStickerName = jsonData.getString("stickerName");
            stickerData.duration = jsonData.getInt("duration");
            stickerData.stickerLooping = (jsonData.getInt("stickerLooping") == 1);
            stickerData.audioPath = jsonData.optString("audioPath");
            stickerData.audioLooping = (jsonData.optInt("audioLooping", 0) == 1);
            stickerData.maxCount = jsonData.optInt("maxCount", 5);
            dynamicSticker.dataList.add(stickerData);
        }

        return dynamicSticker;
    }
    /**
     * 更新贴纸顶点
     * @param stickerData
     */
    static public void calculateStickerVertices(DynamicStickerNormalData stickerData, FaceDetectionReport[] faceResult, int width, int height,
                                                float[] stickerVertices, float[] projectionMatrix,float[]  viewMatrix,float[] mVPMatrix, int faceWidth ,int faceHeight, boolean isCapture) {
        // 步骤一、计算贴纸的中心点和顶点坐标
        // 备注：由于frustumM设置的bottom 和top 为 -1.0 和 1.0，这里为了方便计算，直接用高度作为基准值来计算
        // 1.1、计算贴纸相对于人脸的宽高

        float stickerWidth = (float) CameraUtils.getDistance(
                CameraUtils.getStickFacePoint(faceResult,Contants.FACESTARTINDEX,
                        faceWidth,faceHeight,isCapture).x * width,
                CameraUtils.getStickFacePoint(faceResult,Contants.FACESTARTINDEX,
                        faceWidth,faceHeight,isCapture).y * height,
                CameraUtils.getStickFacePoint(faceResult,Contants.FACEENDINDEX,
                        faceWidth,faceHeight,isCapture).x * width,
                CameraUtils.getStickFacePoint(faceResult,Contants.FACEENDINDEX,
                        faceWidth,faceHeight,isCapture).y  * height) * stickerData.baseScale;
        float stickerHeight = stickerWidth * (float) stickerData.height / (float) stickerData.width;
        // 1.2、根据贴纸的参数计算出中心点的坐标
        float centerX = 0.0f;
        float centerY = 0.0f;
        for (int i = 0; i < stickerData.centerIndexList.length; i++) {
            centerX += CameraUtils.getStickFacePoint(faceResult,stickerData.centerIndexList[i]*2,faceWidth,faceHeight,isCapture).x * width;
            centerY += CameraUtils.getStickFacePoint(faceResult,stickerData.centerIndexList[i]*2,faceWidth,faceHeight,isCapture).y * height;
        }
        centerX /= (float) stickerData.centerIndexList.length;
        centerY /= (float) stickerData.centerIndexList.length;

        // 1.3、求出真正的中心点顶点坐标，这里由于frustumM设置了长宽比，以高度为1 来归一化。这里需要转换一下

         /*   --            1.0f
             ↑              |
                            |
                            |
          height ---------  |  ----------
                            |
             ↓              |
            ---            -1.f

        顶点坐标 -1 到 1 ，而长度为height ， 所以归一化后，真实的长度 1 代表 归一化后的 height/2 ,
        而坐标系的原点（0，0） 处于 （width/2, height/2） 处。
        故做出下面的转换，求出centerX，centerY 归一化后的坐标值*/

        float div = height / 2 ;
        float ndcCenterX = ((centerX - width / 2) / div);
        float ndcCenterY = ((centerY - height / 2) / div);

        // 1.4、贴纸的宽高在ndc坐标系中的长度
        float ndcStickerWidth = stickerWidth / div * ProjectionScale;;
        float ndcStickerHeight = ndcStickerWidth * (float) stickerData.height / (float) stickerData.width;

        // 1.5、根据贴纸参数求偏移的ndc坐标
        float offsetX = (stickerWidth * stickerData.offsetX) / div ;
        float offsetY = (stickerHeight * stickerData.offsetY) / div ;

        // 1.6、贴纸带偏移量的锚点的ndc坐标，即实际贴纸的中心点在OpenGL的顶点坐标系中的位置
        float anchorX = (ndcCenterX + offsetX) * ProjectionScale;
        float anchorY = (ndcCenterY + offsetY) * ProjectionScale;

        // 1.7、根据前面的锚点，计算出贴纸实际的顶点坐标
        stickerVertices[0] = anchorX - ndcStickerWidth/2; stickerVertices[1] = anchorY - ndcStickerHeight/2;
        stickerVertices[2] = anchorX + ndcStickerWidth/2; stickerVertices[3] = anchorY - ndcStickerHeight/2;
        stickerVertices[4] = anchorX - ndcStickerWidth/2; stickerVertices[5] = anchorY + ndcStickerHeight/2;
        stickerVertices[6] = anchorX + ndcStickerWidth/2; stickerVertices[7] = anchorY + ndcStickerHeight/2;

/*        stickerVertices[0] =  - ndcStickerWidth/2; stickerVertices[1] =  - ndcStickerHeight/2;
        stickerVertices[2] =  + ndcStickerWidth/2; stickerVertices[3] =  - ndcStickerHeight/2;
        stickerVertices[4] =  - ndcStickerWidth/2; stickerVertices[5] =  + ndcStickerHeight/2;
        stickerVertices[6] =  + ndcStickerWidth/2; stickerVertices[7] =  + ndcStickerHeight/2;*/
        // 贴纸变换矩阵

        float[] modelMatrix = new float[16];
        // 步骤二、根据人脸姿态角计算透视变换的总变换矩阵
        // 2.1、将Z轴平移到贴纸中心点，因为贴纸模型矩阵需要做姿态角变换
        // 平移主要是防止贴纸变形
        Matrix.setIdentityM(modelMatrix, 0);

        Matrix.translateM(modelMatrix, 0, anchorX, anchorY, 0);

        // 2.2、贴纸姿态角旋转
        float pitchAngle = -(float) (faceResult[0].pitch * 180f / Math.PI);
        float yawAngle = (float) (faceResult[0].yaw * 180f / Math.PI);
        float rollAngle = (float) (faceResult[0].roll * 180f / Math.PI);
        // 限定左右扭头幅度不超过50°，销毁人脸关键点SDK带来的偏差
        if (Math.abs(yawAngle) > 50) {
            yawAngle = (yawAngle / Math.abs(yawAngle)) * 50;
        }
        // 限定抬头低头最大角度，消除人脸关键点SDK带来的偏差
        if (Math.abs(pitchAngle) > 30) {
            pitchAngle = (pitchAngle / Math.abs(pitchAngle)) * 30;
        }
        // 贴纸姿态角变换，优先z轴变换，消除手机旋转的角度影响，否则会导致扭头、抬头、低头时贴纸变形的情况
        //Log.e("huihuixu4","rollAngle:"+rollAngle+"yawAngle:"+yawAngle+"pitchAngle:"+pitchAngle);
        Matrix.rotateM(modelMatrix, 0, rollAngle, 0, 0, 1);
        Matrix.rotateM(modelMatrix, 0, yawAngle, 0, 1, 0);
        Matrix.rotateM(modelMatrix, 0, pitchAngle, 1, 0, 0);

        // 2.4、将Z轴平移回到原来构建的视椎体的位置，即需要将坐标z轴平移回到屏幕中心，此时才是贴纸的实际模型矩阵
        //Matrix.scaleM(modelMatrix, 0, ProjectionScale, ProjectionScale, 0);
        Matrix.translateM(modelMatrix, 0, -anchorX, -anchorY, 0);

        // 2.5、计算总变换矩阵。MVPMatrix 的矩阵计算是 MVPMatrix = ProjectionMatrix * ViewMatrix * ModelMatrix
        // 备注：矩阵相乘的顺序不同得到的结果是不一样的，不同的顺序会导致前面计算过程不一致，这点希望大家要注意
        Matrix.setIdentityM(mVPMatrix, 0);
        Matrix.multiplyMM(mVPMatrix, 0, projectionMatrix, 0, viewMatrix, 0);
        Matrix.multiplyMM(mVPMatrix, 0, mVPMatrix, 0, modelMatrix, 0);
    }

    static public boolean needDoSticker(){
        boolean isNeed = !CameraData.getInstance().getStickerName().equals("")&&
                !CameraData.getInstance().getCurrentModeType().equals(Contants.ModeType.VideMode);
        return isNeed;
    }
    static public float range(final int percentage, final float start, final float end) {
        return (end - start) * percentage / 100.0f + start;
    }

    static public boolean checkHasNotLocationPermission(Context context) {
        return context.checkSelfPermission(Manifest.permission.ACCESS_FINE_LOCATION) != PackageManager.PERMISSION_GRANTED ||
        context.checkSelfPermission(Manifest.permission.ACCESS_COARSE_LOCATION) != PackageManager.PERMISSION_GRANTED;
    }

}
