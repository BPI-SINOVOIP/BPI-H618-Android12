package com.allwinner.camera.data;

import android.content.Context;
import android.content.SharedPreferences;
import android.graphics.PixelFormat;
import android.graphics.Point;
import android.hardware.camera2.CameraManager;
import android.net.Uri;
import android.os.Handler;
import android.preference.PreferenceManager;
import android.util.DisplayMetrics;
import android.util.Log;
import android.view.OrientationEventListener;
import android.view.Surface;
import android.view.WindowManager;

import com.allwinner.camera.R;
import com.allwinner.camera.utils.CameraUtils;
import com.allwinner.camera.utils.SharedPreferencesUtils;

import java.util.ArrayList;
import java.util.HashMap;
import java.util.List;

public class CameraData {
    private static final String TAG = "CameraData";
    private static CameraData mInstance = null;
    private int mSreenWidth;
    private int mScreenHeight;
    private int mViewVissableHeight_4_TO_3;
    private int mViewVissableHeight_16_TO_9;
    private int mViewVissableHeight_18_TO_9;
    private int mViewVissableHeight_1_TO_1;
    private float mDensity;
    private int mPreviewWidth = 640;
    private int mPreviewHieght = 480;
    Point mPreviewSize;
    Point mPictureSize;
    private HashMap<Integer, List<Point>> mSupportedPictureSizes = new HashMap();
    private HashMap<Integer, List<Point>> mSupportedPreviewSizes = new HashMap();
    private HashMap<Integer, List<Point>> mSupportedVideoSizes = new HashMap();
    private Contants.ModeType mModeType = Contants.ModeType.AutoMode;
    private int mFlashMode = Contants.FlashMode.FLASH_OFF;
    private WindowManager mWindowManager;
    private int mOrientation = OrientationEventListener.ORIENTATION_UNKNOWN;
    private int mDisplayRotation = 0;
    private int mThumbnailSize;
    private int mSensorOrientation = 0;
    private int mVideoQuality = 0;
    private SharedPreferences mSp;
    private Contants.FilterType mFilterType = Contants.FilterType.YUV;
    private boolean mFaceBeautyOn = false;
    private String mPath;
    private SharedPreferences mSharedPreferences;
    private SharedPreferences.Editor editor;
    private boolean mIsFaceHandle = false;
    private Context mContext;
    private double[] mRatioPictrue = {(double) (4f / 3f), (double) (16f / 9f), (double) (18f / 9f), (double) (1f / 1f)};
    private String[] mRatioStrPictrue = {Contants.PictureRatioStr.PICTURE_RATIO_STR_4_TO_3, Contants.PictureRatioStr.PICTURE_RATIO_STR_16_TO_9,
            Contants.PictureRatioStr.PICTURE_RATIO_STR_18_TO_9, Contants.PictureRatioStr.PICTURE_RATIO_STR_1_TO_1};
    private String[] mVideoStrRatio = {Contants.VideoRatioStr.VIDEO_RATIO_STR_480P, Contants.VideoRatioStr.VIDEO_RATIO_STR_720P,
            Contants.VideoRatioStr.VIDEO_RATIO_STR_1080P, Contants.VideoRatioStr.VIDEO_RATIO_STR_4K};
    private List<String> mSupportedPictureRatio = new ArrayList<>();
    private List<String> mSupportedVideoQuality = new ArrayList<>();
    private Uri mUri;
    private boolean mIsFlashAvailable;
    private boolean mIsCaoturePano = false;
    private boolean mMosaicFrameProcessorInitialized = false;
    private float mMaxZoom = 400f;
    private boolean mIsSupportZoom = false;
    private Handler mCameraHandler;
    private CameraManager mCameraManager;
    private int mFrontCameraId =-1;
    private int mBackCameraId =-1;
    private boolean mIsPreviewStart = false;
    private boolean mCanChangeMode = true;
    private int mFrontCameraOrientation;
    private int mBackCameraOrientation;
    private int mIntentQuality = -1;
    private int mSeconds = -1;
    private long mRequestedSizeLimit = -1;
    private Uri mPhotoUri;
    private boolean mIsStorageError;
    private int mOrientationCompensation;
    private long mCaptureTime = 0;
    private String mCustomText="";
    private boolean mIsImageCaptureIntent;
    private boolean mIsVideoCaptureIntent;
    private boolean mSecureCamera;
    private String mStickerName ="";
    private int mSmoothProgress;
    private int mWhiteProgress;
    private int mBigeyeProgress;
    private int mFaceliftProgress;
    private boolean mFaceAdjustVisibility;
    private boolean mHasFrontCamera;
    private boolean mHasBackCamera;

    private CameraData() {

    }

    public synchronized static CameraData getInstance() {
        if (mInstance == null) {
            mInstance = new CameraData();
            return mInstance;
        } else {
            return mInstance;
        }
    }

    public void initData(Context context) {
        mContext = context;
        DisplayMetrics metrics = new DisplayMetrics();
        mWindowManager = (WindowManager) context.getSystemService(Context.WINDOW_SERVICE);
        mWindowManager.getDefaultDisplay().getMetrics(metrics);
        mSp = PreferenceManager.getDefaultSharedPreferences(context);
        mSharedPreferences = context.getSharedPreferences("data", Context.MODE_PRIVATE);
        editor = mSharedPreferences.edit();
        mDensity = metrics.density;
        mThumbnailSize = (int) ((float) context.getResources().getDimensionPixelOffset(R.dimen.thumbnail_size));
        mSreenWidth = Math.min(metrics.heightPixels, metrics.widthPixels);
        mScreenHeight = Math.max(metrics.heightPixels, metrics.widthPixels);
        Log.i(TAG, "mSreenSize" + mSreenWidth + "x" + mScreenHeight + " mThumbnailSize " + mThumbnailSize + "mDensity" + mDensity);
        mViewVissableHeight_4_TO_3 = (int) (mSreenWidth * (4 / 3f));
        mViewVissableHeight_16_TO_9 = (int) (mSreenWidth * (16 / 9f));
        mViewVissableHeight_18_TO_9 = (int) (mSreenWidth * (18 / 9f));
        mViewVissableHeight_1_TO_1 = mSreenWidth;
        //updateCameraRatio(getCurrentRadioStr());
    }

    public int getSreenWidth() {
        return mSreenWidth;
    }

    public int getScreenHeight() {
        return mScreenHeight;
    }

    public int getScreenHeight_4_TO_3() {
        return mViewVissableHeight_4_TO_3;
    }

    public int getViewVissableHeight() {
        int cameraRatio;
        cameraRatio = getCameraRatio();
        if (cameraRatio == Contants.CameraRatio.RATIO_4_TO_3) {
            return mViewVissableHeight_4_TO_3;
        } else if (cameraRatio == Contants.CameraRatio.RATIO_16_TO_9) {
            return mViewVissableHeight_16_TO_9;
        } else if (cameraRatio == Contants.CameraRatio.RATIO_18_TO_9) {
            return mViewVissableHeight_18_TO_9;
        } else if (cameraRatio == Contants.CameraRatio.RATIO_1_TO_1) {
            return mViewVissableHeight_1_TO_1;
        } else {
            return mViewVissableHeight_4_TO_3;
        }
    }

    public boolean showReferenceLine() {
        return mSp.getBoolean("reference_line", false);
    }

    public boolean getIsSaveLocation() {
        if (CameraUtils.checkHasNotLocationPermission(mContext)) {
            return false;
        }
        return mSp.getBoolean(Contants.KEY_SAVE_POSITION, true);
    }
    public boolean getIsTimeWaterSign() {
        return mSp.getBoolean(Contants.KEY_TIME_WATERSIGN, false);
    }
    public String getModelWaterSignType() {
        return mSp.getString(Contants.KEY_MODEL_WATERSIGN, Contants.MODEL_WATERSIGN_DEFAULT_VALUE);
    }
    public boolean getFirstOpenCamera() {
        return mSharedPreferences.getBoolean("first_open_camera", true);
    }

    public void setFirstOpenCamera(boolean isFirstOpenCamera) {
        editor.putBoolean("first_open_camera", isFirstOpenCamera);
        editor.commit();
    }

    public void setCaptureFlashMode(int flashMode) {
        mFlashMode = flashMode;
        SharedPreferencesUtils.setParam(mContext, "captureflashmode",flashMode);
    }

    public int getCaptureFlashMode() {
        return (int) SharedPreferencesUtils.getParam(mContext, "captureflashmode", 0);
    }
    public void setVideoFlashMode(int flashMode) {
        mFlashMode = flashMode;
        SharedPreferencesUtils.setParam(mContext, "videoflashmode",flashMode);
    }

    public int getVideoFlashMode() {
        return (int) SharedPreferencesUtils.getParam(mContext, "videoflashmode", 0);
    }
    public void setIsFaceHandle(boolean isFaceHandle) {

        mIsFaceHandle = isFaceHandle;
    }

    public boolean getIsFaceHandle() {
        return mIsFaceHandle;
    }

    public void setIsFlashAvailable(boolean isFlashAvailable) {
        mIsFlashAvailable = isFlashAvailable;
    }

    public boolean getIsFlashAvailable() {
        return mIsFlashAvailable;
    }

    public float getDensity() {
        return mDensity;
    }

    public void setPreviewSize(int width, int height) {
        mPreviewWidth = width;
        mPreviewHieght = height;
        if (mPreviewSize == null) {
            mPreviewSize = new Point();
        }
        mPreviewSize.x = mPreviewWidth;
        mPreviewSize.y = mPreviewHieght;
        Log.i(TAG, "setPreviewSize: " + mPreviewSize);
    }

    public int getPreviewBufSize() {
        PixelFormat pixelInfo = new PixelFormat();
        PixelFormat.getPixelFormatInfo(PixelFormat.RGB_888, pixelInfo);
        // TODO: remove this extra 32 byte after the driver bug is fixed.
        return (mPreviewWidth * mPreviewHieght * pixelInfo.bitsPerPixel / 8) + 32;
    }

    public void setPictureSize(int width, int height) {
        if (mPictureSize == null) {
            mPictureSize = new Point();
        }
        mPictureSize.x = width;
        mPictureSize.y = height;
        Log.i(TAG, "setPictureSize: " + mPictureSize);
    }

    public void setPhotoUri(Uri uri) {
        mPhotoUri = uri;
    }

    public Uri getPhotoUri() {
        return mPhotoUri;
    }
    public void setPhotoPath(String path) {
        mPath = path;
    }

    public String getPhotoPath() {
        return mPath;
    }
    public int getPreviewWidth() {
        return mPreviewWidth;
    }

    public int getPreviewHeight() {
        return mPreviewHieght;
    }

    public Point getPreviewSize() {
        return mPreviewSize;
    }

    public Point getPictureSize() {
        return mPictureSize;
    }

    public void setOrientation(int rotation) {
        mOrientation = roundOrientation(rotation, mOrientation);

    }

    public void setIsCapturePano(boolean isCapturePano) {
        mIsCaoturePano = isCapturePano;
    }

    public boolean getIsCapturePano() {
        return mIsCaoturePano;
    }

    public static int roundOrientation(int orientation, int orientationHistory) {
        boolean changeOrientation = false;
        if (orientationHistory == OrientationEventListener.ORIENTATION_UNKNOWN) {
            changeOrientation = true;
        } else {

            int dist = Math.abs(orientation - orientationHistory);
            dist = Math.min(dist, 360 - dist);
            changeOrientation = (dist >= 60 + 5);
        }
        if (changeOrientation) {
            return ((orientation + 30) / 90 * 90) % 360;
        }
        return orientationHistory;
    }

    public int getOrientation() {
        // Log.i(TAG, "getOrientation: " + mOrientation);
        if (mOrientation == OrientationEventListener.ORIENTATION_UNKNOWN) {
            return 0;
        } else {
            return mOrientation;
        }

    }

    public int getDisplayRotation() {
        mDisplayRotation = mWindowManager.getDefaultDisplay().getRotation();
        switch (mDisplayRotation) {
            case Surface.ROTATION_0:
                mDisplayRotation = 0;
                break;
            case Surface.ROTATION_180:

                mDisplayRotation = 180;
                break;
            case Surface.ROTATION_90:
                mDisplayRotation = 90;
                break;
            case Surface.ROTATION_270:
                mDisplayRotation = 270;
                break;
            default:
                break;
        }
        return mDisplayRotation;

    }

    /**
     * 保存List
     *
     * @param tag
     */
    public void setDataList(String tag, List<String> environmentList) {
        editor.putInt("Nums" + tag, environmentList.size());
        for (int i = 0; i < environmentList.size(); i++) {
            editor.putString(tag + i, environmentList.get(i));
        }
        editor.commit();

    }

    /**
     * 获取List
     *
     * @param tag
     * @return
     */
    public List<String> getDataList(String tag) {
        int environNums = mSharedPreferences.getInt("Nums" + tag, 0);
        List<String> environmentList = new ArrayList<>(environNums);
        for (int i = 0; i < environNums; i++) {
            String str = mSharedPreferences.getString(tag + i, "");
            environmentList.add(str);
        }
        return environmentList;

    }
    public void setCustomText(String text) {
        mCustomText = text;
        editor.putString("custom_text",text );
        editor.commit();
    }

    public String getCustomText() {
        if(mCustomText.equals("")){
            return (String) mSharedPreferences.getString("custom_text","");
        }
        return mCustomText;
    }
    public Point getPictureSizebyRatio(List<Point> pointList,
                                       double targetRatio) {
        final double ASPECT_TOLERANCE = 0.1;
        Point picturesize = null;

        int optimalSizeIndex = -1;
        double minDiff = Double.MAX_VALUE;

        for (Point point : pointList) {
            double ratio = (double) point.x / point.y;
            if (Math.abs(ratio - targetRatio) > ASPECT_TOLERANCE * 2) {
                continue;
            }

            if (picturesize == null) {
                picturesize = point;
            }

            if (compareSize(point, picturesize)) {
                picturesize = point;
            }
        }
        return picturesize;
    }

    public Point getPreviewSizebyRatio(List<Point> pointList,
                                       double targetRatio) {
        final double ASPECT_TOLERANCE = 0.01;
        Point previewsize = null;

        int optimalSizeIndex = -1;
        double minDiff = Double.MAX_VALUE;
        float tagetHeight = Math.min(getSreenWidth(), getScreenHeight());
        for (Point point : pointList) {
            double ratio = (double) point.x / point.y;
            if (Math.abs(ratio - targetRatio) > ASPECT_TOLERANCE)
                continue;
            if (previewsize == null) {
                previewsize = point;
            }
            if (Math.abs(point.y - tagetHeight) < minDiff) {
                previewsize = point;
                minDiff = Math.abs(point.y - tagetHeight);
            }
        }
        return previewsize;
    }

    public boolean compareSize(Point size1, Point size2) {
        if (size1 == null || size2 == null) {
            return false;
        }
        float fsize1 = (float) size1.x * (float) size1.y / 1000000;
        float fsize2 = (float) size2.x * (float) size2.y / 1000000;
        if (fsize1 > fsize2) {
            return true;
        }
        return false;
    }

    public HashMap<Integer, List<Point>> getSurpportedPreviewSizes() {
        return mSupportedPreviewSizes;
    }

    public HashMap<Integer, List<Point>> getSurpportedPictureSizes() {
        return mSupportedPictureSizes;
    }


    public HashMap<Integer, List<Point>> getSurpportedVideoSizes() {
        return mSupportedVideoSizes;
    }

    public void addPictureSizetoList(List<Point> list, int cameraid) {
        mSupportedPictureRatio.clear();
        if (list.size() != 0) {
            mSupportedPictureSizes.put(cameraid, list);
            for (int i = 0; i < mRatioPictrue.length; i++) {
                if (getPictureSizebyRatio(list, mRatioPictrue[i]) != null) {
                    Log.e(TAG, "mRatioStrPictrue[" + i + "]:" + mRatioStrPictrue[i]);
                    mSupportedPictureRatio.add(mRatioStrPictrue[i]);
                }
            }
            setDataList("picturesize" + cameraid, mSupportedPictureRatio);
        }
    }

    public void addVideoSizetoList(List<Point> list, int cameraid) {
        mSupportedVideoQuality.clear();
        if (list.size() != 0) {
            mSupportedVideoSizes.put(cameraid, list);
            for (Point point : list) {
                if (point.equals(640, 480)) {
                    mSupportedVideoQuality.add(mVideoStrRatio[0]);
                } else if (point.equals(1280, 720)) {
                    mSupportedVideoQuality.add(mVideoStrRatio[1]);
                } else if (point.equals(1920, 1080)) {
                    mSupportedVideoQuality.add(mVideoStrRatio[2]);
                } else if (point.equals(3840, 2160)) {
                    mSupportedVideoQuality.add(mVideoStrRatio[3]);
                }
            }
            setDataList("videoquality" + cameraid, mSupportedVideoQuality);
        }
    }

    public void addPreviewSizetoList(List<Point> list, int cameraid) {
        if (list.size() != 0) {
            mSupportedPreviewSizes.put(cameraid, list);
        }
    }

    public void setCurrentMode(Contants.ModeType type) {
        mModeType = type;
    }

    public Contants.ModeType getCurrentModeType() {
        return mModeType;
    }

    public int getCameraRatio() {
        String radioStr;
        if (getCurrentModeType().equals(Contants.ModeType.SquareMode)) {
            return Contants.CameraRatio.RATIO_4_TO_3;
        }
        if (getCurrentModeType().equals(Contants.ModeType.VideMode)) {
            if (CameraData.getInstance().getCameraId() == getBackCameraId()) {
                radioStr = mSp.getString("back_video_scale", Contants.VIDEO_SCALE_DEFAULT_VALUE);
            } else {
                radioStr = mSp.getString("front_video_scale", Contants.VIDEO_SCALE_DEFAULT_VALUE);
            }
            Log.v(TAG, "video radioStr:" + radioStr);
            if (radioStr.equals(Contants.VideoRatioStr.VIDEO_RATIO_STR_480P)) {
                return Contants.CameraRatio.RATIO_4_TO_3;
            } else if (radioStr.equals(Contants.VideoRatioStr.VIDEO_RATIO_STR_720P)) {
                return Contants.CameraRatio.RATIO_16_TO_9;
            } else if (radioStr.equals(Contants.VideoRatioStr.VIDEO_RATIO_STR_4K)) {
                return Contants.CameraRatio.RATIO_16_TO_9;
            } else if (radioStr.equals(Contants.VideoRatioStr.VIDEO_RATIO_STR_1080P)) {
                return Contants.CameraRatio.RATIO_16_TO_9;
            } else {
                return Contants.CameraRatio.RATIO_4_TO_3;
            }
        }
        if (CameraData.getInstance().getCameraId() == getBackCameraId()) {
            radioStr = mSp.getString(Contants.KEY_BACK_PICTURE_SCALE, Contants.PICTURE_SCALE_DEFAULT_VALUE);
        } else {
            radioStr = mSp.getString(Contants.KEY_FRONT_PICTURE_SCALE, Contants.PICTURE_SCALE_DEFAULT_VALUE);
        }
        Log.v(TAG, "radioStr:" + radioStr);
        if (radioStr.equals(Contants.PictureRatioStr.PICTURE_RATIO_STR_4_TO_3)) {
            return Contants.CameraRatio.RATIO_4_TO_3;
        } else if (radioStr.equals(Contants.PictureRatioStr.PICTURE_RATIO_STR_16_TO_9)) {
            return Contants.CameraRatio.RATIO_16_TO_9;
        } else if (radioStr.equals(Contants.PictureRatioStr.PICTURE_RATIO_STR_1_TO_1)) {
            return Contants.CameraRatio.RATIO_1_TO_1;
        } else if (radioStr.equals(Contants.PictureRatioStr.PICTURE_RATIO_STR_18_TO_9)) {
            return Contants.CameraRatio.RATIO_18_TO_9;
        } else {
            return Contants.CameraRatio.RATIO_4_TO_3;
        }
        // return mCameraRatio;
    }

    public int getThumbnailSize() {
        return mThumbnailSize;
    }

    public int getCameraId() {
        int cameraId = mSharedPreferences.getInt("cameraid", mBackCameraId);
        if(cameraId < 0){
            if(getHasFrontCamera()){
                cameraId = mFrontCameraId;
            }
        }
        return mSharedPreferences.getInt("cameraid", cameraId);
    }

    public void setCameraId(int cameraId) {
        Log.i(TAG, "setCameraId: " + cameraId);
        editor.putInt("cameraid", cameraId);
        editor.commit();
    }

    public void setSensorOrientation(int sensorOrientation) {
        mSensorOrientation = sensorOrientation;
    }

    public int getSensorOrientation() {
        return mSensorOrientation;
    }

    public void setIntentUri(Uri uri) {
        mUri = uri;
    }

    public Uri getIntentUri() {
        return mUri;
    }

    public void setVideoQuality(int quality, int ratio) {
        mVideoQuality = quality;
        //?sharePerenfence
    }

    public int getVideoQuality() {
        String radioStr;
        if (CameraData.getInstance().getCameraId() == getBackCameraId()) {

            radioStr = mSp.getString("back_video_scale", Contants.VIDEO_SCALE_DEFAULT_VALUE);
        } else {

            radioStr = mSp.getString("front_video_scale", Contants.VIDEO_SCALE_DEFAULT_VALUE);
        }
        if (radioStr.equals(Contants.VideoRatioStr.VIDEO_RATIO_STR_480P)) {
            return Contants.VideoQuality.Q480P;
        } else if (radioStr.equals(Contants.VideoRatioStr.VIDEO_RATIO_STR_720P)) {
            return Contants.VideoQuality.Q720P;
        } else if (radioStr.equals(Contants.VideoRatioStr.VIDEO_RATIO_STR_4K)) {
            return Contants.VideoQuality.Q4K;
        } else if (radioStr.equals(Contants.VideoRatioStr.VIDEO_RATIO_STR_1080P)) {
            return Contants.VideoQuality.Q1080P;
        } else {
            return Contants.VideoQuality.Q480P;
        }
    }

    public void setFilterType(Contants.FilterType filtertype) {
        mFilterType = filtertype;
    }

    public Contants.FilterType getFilterType() {
        return mFilterType;
    }

    public void setFaceBeautyOn(boolean fbOn) {
        mFaceBeautyOn = fbOn;
    }

    public boolean getFaceBeautyStatus() {
        return mFaceBeautyOn;
    }

    public boolean isImageCaptureIntent() {
        return mIsImageCaptureIntent;
    }

    public boolean isVideoCaptureIntent() {
        return mIsVideoCaptureIntent;
    }

    public void setImageCaptureIntent(boolean isImageCaptureIntent) {
        mIsImageCaptureIntent = isImageCaptureIntent;
    }

    public void setVideoCaptureIntent(boolean isVideoCaptureIntent) {
        mIsVideoCaptureIntent = isVideoCaptureIntent;
    }

    public void setMosaicFrameProcessorInitialized(boolean isMosaicFrameProcessorInitialized) {
        mMosaicFrameProcessorInitialized = isMosaicFrameProcessorInitialized;
    }

    public boolean getMosaicFrameProcessorInitialized() {
        return mMosaicFrameProcessorInitialized;
    }

    public void setMaxZoom(float maxZoom) {
        mMaxZoom = maxZoom;
    }

    public float getMaxZoom() {
        return mMaxZoom;
    }

    public void setIsSupportZoom(boolean isSupportZoom) {
        mIsSupportZoom = isSupportZoom;
    }

    public boolean getIsSupportZoom() {
        return mIsSupportZoom;
    }

    public void setCameraHandler(Handler cameraHandler) {
        mCameraHandler = cameraHandler;
    }

    public Handler getCameraHandler() {
        return mCameraHandler;
    }

    public void setCameraManager(CameraManager cameraManager) {
        mCameraManager = cameraManager;
    }


    public void setFrontCameraId(int frontCameraId) {
        mFrontCameraId = frontCameraId;
    }

    public int getFrontCameraId() {
        return mFrontCameraId;
    }

    public boolean isCurrentFront() {
        return getCameraId() == getFrontCameraId() ;
    }

    public void setBackCameraId(int backCameraId) {
        mBackCameraId = backCameraId;
    }

    public int getBackCameraId() {
        return mBackCameraId;
    }

    public void setIsPreviewDone(boolean isPreviewStart) {
        Log.e(TAG,"setIsPreviewDone:"+isPreviewStart);
        mIsPreviewStart = isPreviewStart;
    }

    public boolean getIsPreviewDone() {
        Log.e(TAG,"getIsPreviewDone:"+mIsPreviewStart);

        return mIsPreviewStart;
    }
    public void setIsStorageError(boolean isStorageError) {
        Log.e(TAG,"setIsStorageError:"+mIsStorageError);
        mIsStorageError = isStorageError;
    }

    public boolean getIsStorageError() {
        Log.e(TAG,"getIsStorageError:"+mIsStorageError);

        return mIsStorageError;
    }
    public void setCanChangeMode(boolean canChangeMode) {
      mCanChangeMode = canChangeMode;
    }

    public boolean getCanChangeMode() {
        return mCanChangeMode;
    }

    public void setFrontCameraOrientation(int orientation) {
        mFrontCameraOrientation = orientation;
    }
    public int getFrontCameraOrientation(){
        return mFrontCameraOrientation;
    }

    public void setBackCameraOrientation(int orientation) {
        mBackCameraOrientation = orientation;
    }
    public int getBackCameraOrientation(){
        return mBackCameraOrientation;
    }

    public void setIntentDuration(int seconds) {
        mSeconds = seconds;
    }
    public int getIntentDuration(){
        return  mSeconds;
    }
    public void setIntentSize(long requestedSizeLimit) {
        mRequestedSizeLimit = requestedSizeLimit;
    }
    public long getIntentSize(){
        return  mRequestedSizeLimit;
    }
    public void setIntentQuality(int intentQuality) {
        mIntentQuality = intentQuality;
    }

    public int getIntentQuality() {
        return mIntentQuality;
    }

    public void setOrientationCompensation(int orientationCompensation) {
        mOrientationCompensation = orientationCompensation;
    }
    public int getOrientationCompensation() {
       return mOrientationCompensation ;
    }

    public void setCaptureTime(long captureTime) {
        mCaptureTime = captureTime;
    }
    public long getCaptureTime() {
       return mCaptureTime;
    }

    public void setSecureCameraIntent(boolean isSecure) {
        mSecureCamera = isSecure;
    }
    public boolean getIsSecureCameraIntent() {
        return mSecureCamera ;
    }

    public void setStickerName(String stickerName) {
        mStickerName = stickerName;
    }
    public String getStickerName() {
        return mStickerName ;
    }

    public void setSmoothProgress(int smoothProgress) {
        mSmoothProgress = smoothProgress;
    }
    
    public int getSmoothProgress(){
        return mSmoothProgress;
    }
    
    public void setWhiteProgress(int whiteProgress) {
        mWhiteProgress = whiteProgress;
    }
    
    public int getWhiteProgress(){
        return mWhiteProgress;
    }

    public void setBigeyeProgress(int bigeyeProgress) {
        mBigeyeProgress = bigeyeProgress;
    }
    
    public int getBigeyeProgress(){
        return mBigeyeProgress;
    }
    
    public void setFaceliftProgress(int faceliftProgress) {
        mFaceliftProgress = faceliftProgress;
    }
    
    public int getFaceliftProgress(){
        return mFaceliftProgress;
    }

    public void setFaceAdjustVisibility(boolean visibility) {
        mFaceAdjustVisibility = visibility;
    }
    public boolean getFaceAdjustVisibility(){
        return mFaceAdjustVisibility;
    }

    public void setHasFrontCamera(boolean hasFrontCamera) {
        mHasFrontCamera = hasFrontCamera;
    }

    public boolean getHasFrontCamera(){
        return mHasFrontCamera;
    }

    public void setHasBackCamera(boolean hasBackCamera) {
        mHasBackCamera = hasBackCamera;
    }

    public boolean getHasBackCamera(){
        return mHasBackCamera;
    }
}
