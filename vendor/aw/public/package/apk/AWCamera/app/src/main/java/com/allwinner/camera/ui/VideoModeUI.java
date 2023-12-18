package com.allwinner.camera.ui;

import android.annotation.TargetApi;
import android.content.ContentResolver;
import android.content.ContentValues;
import android.content.Intent;
import android.graphics.Bitmap;
import android.graphics.Matrix;
import android.media.AudioManager;
import android.media.CamcorderProfile;
import android.media.MediaCodec;
import android.media.MediaMetadataRetriever;
import android.media.MediaRecorder;
import android.net.Uri;
import android.os.AsyncTask;
import android.os.Build;
import android.os.ParcelFileDescriptor;
import android.os.SystemClock;
import android.os.Handler;
import android.os.Message;
import android.provider.MediaStore;
import android.support.annotation.RequiresApi;
import android.util.Log;
import android.view.Surface;
import android.view.View;
import android.widget.Chronometer;
import android.widget.RelativeLayout;
import android.widget.Toast;

import com.allwinner.camera.R;
import com.allwinner.camera.SettingFragmentActivity;
import com.allwinner.camera.cameracontrol.CameraLock;
import com.allwinner.camera.cameracontrol.CameraManager;
import com.allwinner.camera.data.CameraData;
import com.allwinner.camera.data.Contants;
import com.allwinner.camera.utils.CameraUtils;
import com.allwinner.camera.utils.SharedPreferencesUtils;
import com.allwinner.camera.utils.SoundPlayerUtil;
import com.allwinner.camera.utils.ThumbnailUtils;
import com.allwinner.camera.views.CircleRotateImageView;
import com.allwinner.camera.views.RotateImageView;

import org.greenrobot.eventbus.EventBus;

import java.io.File;
import java.io.IOException;
import java.text.SimpleDateFormat;

import static android.hardware.Camera.ACTION_NEW_VIDEO;

public class VideoModeUI extends BaseModeUI implements View.OnClickListener,MediaRecorder.OnInfoListener,MediaRecorder.OnErrorListener{
    private final Chronometer mTimeChronometer;
    private final CameraLock mCameraLock;
    private final RotateImageView mFlashImageView;
    private CircleRotateImageView mVideoControlImageView;
    private RelativeLayout mVideoControlRelativeLayout;
    private CircleRotateImageView mSwitchVideoImageView;
    private RotateImageView mSettingImageView;
    private CircleRotateImageView mRecordImageView;
    private RelativeLayout mSwicthVideoRelativeLayout;
    private MediaRecorder mMediaRecorder;
    private long mRecordTime;
    private ContentValues mCurrentVideoValues;
    private Surface mVideoSurface;
    private static final String TAG = "VideoModeUI";
    private RelativeLayout mRecordRelativeLayout;
    private Boolean mIsRecording = false;
    public static final String VIDEO_MEDIA_URI = "content://media/external/video/media";
    private String mVideoSaveTempPath;
    private String mVideoSavePreviewPath;
    private boolean mPauseRecord = true;
    private long mRecordingTime = 0;
    private String mPath;
    private long mCaptureTime;
    private boolean mIsStopFinish = false;
    private boolean mIsStartFinish = false;
    private ParcelFileDescriptor mVideoFileDescriptor;
    private int mFlashCurrentImg = 0;
    private int[] mFlashImageArray = {R.mipmap.ic_flash_off_normal, R.mipmap.ic_flash_on_normal};
    private int[] mFlashArray = {Contants.FlashMode.FLASH_OFF, Contants.FlashMode.FLASH_TORCH};
    private Uri mUri;

    private boolean mVideoSurfaceStatus = true;
    private final long DELAY_RECORD_TIME = 3 * 100; // 300ms delay
    private final int START_DELAY_RECORD = 999;

    public VideoModeUI(View rootView, UIManager uiManager) {
        super(rootView, uiManager);
        mRecordRelativeLayout = (RelativeLayout) rootView.findViewById(R.id.record);
        mRecordImageView = (CircleRotateImageView) rootView.findViewById(R.id.iv_rotateRecord);
        mSettingImageView = (RotateImageView) rootView.findViewById(R.id.iv_video_settings);
        mFlashImageView = (RotateImageView) rootView.findViewById(R.id.iv_flash);
        mSwicthVideoRelativeLayout = (RelativeLayout) rootView.findViewById(R.id.rl_switchVideo);
        mSwitchVideoImageView = (CircleRotateImageView) rootView.findViewById(R.id.iv_switchVideo);
        mVideoControlRelativeLayout = (RelativeLayout) rootView.findViewById(R.id.rl_video_control);
        mVideoControlImageView = (CircleRotateImageView) rootView.findViewById(R.id.iv_video_control);
        mTimeChronometer = (Chronometer) rootView.findViewById(R.id.timer);
        mRecordRelativeLayout.setOnClickListener(this);
        mVideoControlRelativeLayout.setOnClickListener(this);
        mSwicthVideoRelativeLayout.setOnClickListener(this);
        mSettingImageView.setOnClickListener(this);
        mFlashImageView.setOnClickListener(this);
        if(hasOneCamera()){
            mSwicthVideoRelativeLayout.setVisibility(View.INVISIBLE);
        }
        mCameraLock = new CameraLock();
        updateFlashView();
        if (CameraData.getInstance().isVideoCaptureIntent()) {
            mSettingImageView.setVisibility(View.GONE);
            mVideoControlRelativeLayout.setVisibility(View.GONE);
        }
    }

    @Override
    public void onFlashStatus(final boolean isFlashAvaliable) {
        mUiManager.getActivity().runOnUiThread(new Runnable() {
            @Override
            public void run() {
                updateFlashView();
            }
        });
    }
    @Override
    public void enter() {
        updateFlashView();
    }
    public void updateFlashView(){
        if (CameraData.getInstance().getIsFlashAvailable() ) {
            mFlashImageView.setVisibility(View.VISIBLE);
            int index = 0;
            switch (CameraData.getInstance().getVideoFlashMode()) {
                case Contants.FlashMode.FLASH_OFF:
                    index = 0;
                    break;
                case Contants.FlashMode.FLASH_TORCH:
                    index = 1;
                    break;
            }
            Log.e(TAG, "index:" + index);

            mFlashImageView.setImageResource(mFlashImageArray[index]);
        } else {
            mFlashImageView.setVisibility(View.GONE);
        }

    }
    @Override
    public void onOrientationChanged(int orientationCompensation) {
        mRecordImageView.onOrientationChanged(orientationCompensation);
        mSettingImageView.onOrientationChanged(orientationCompensation);
        mFlashImageView.onOrientationChanged(orientationCompensation);
        mSwitchVideoImageView.onOrientationChanged(orientationCompensation);
        mVideoControlImageView.onOrientationChanged(orientationCompensation);
    }

    private Handler mHandler = new Handler() {
        @Override
        public void handleMessage(Message msg) {
            super.handleMessage(msg);
            switch(msg.what) {
                case START_DELAY_RECORD:
                    if (mVideoSurface == null){
                        sendEmptyMessageDelayed(START_DELAY_RECORD, DELAY_RECORD_TIME);
                    } else {
                        startRecord();
                        mVideoSurfaceStatus = true;
                    }
                    break;
                default: break;
            }
        }
    };

    @RequiresApi(api = Build.VERSION_CODES.N)
    @Override
    public void onClick(View v) {
        switch (v.getId()) {
            case R.id.record:
                SoundPlayerUtil.playSound(SoundPlayerUtil.TYPE_VIDEO_SOUND);
                if (CameraData.getInstance().getIsPreviewDone()) {
                    if (mIsRecording) {
                        mUiManager.startCoverAnimation();
                        stopRecord();
                    } else {
                        if (mVideoSurface == null){
                            if (!mVideoSurfaceStatus) return;
                            mVideoSurfaceStatus = false;
                            Toast.makeText(mUiManager.getContext(), R.string.click_too_frequently_tips,
                                    Toast.LENGTH_LONG).show();
                            mUiManager.onModeChanged();
                            mHandler.sendEmptyMessageDelayed(START_DELAY_RECORD, DELAY_RECORD_TIME);
                        } else {
                            startRecord();
                        }
                    }
                }
                break;
            case R.id.rl_video_control:
                if (mIsRecording) {
                    if (mPauseRecord) {
                        Log.e(TAG, "PauseRecord");
                        mVideoControlImageView.setImageResource(R.mipmap.pause_normol);
                        if (mMediaRecorder != null) {
                            mMediaRecorder.pause();
                        }
                        mPauseRecord = false;
                        mTimeChronometer.stop();
                        mRecordingTime = SystemClock.elapsedRealtime() - mTimeChronometer.getBase();// 保存这次记录了的时间
                    } else {
                        mVideoControlImageView.setImageResource(R.mipmap.play_normal);
                        if (mMediaRecorder != null) {
                            mMediaRecorder.resume();
                        }
                        mTimeChronometer.setBase(SystemClock.elapsedRealtime() - mRecordingTime);// 跳过已经记录了的时间，起到继续计时的作用
                        mTimeChronometer.start();
                        mPauseRecord = true;
                    }
                }
                break;
            case R.id.iv_video_settings:
                Intent intent = new Intent(getContext(), SettingFragmentActivity.class);
                intent.addFlags(Intent.FLAG_ACTIVITY_NEW_TASK);
                getContext().startActivity(intent);
                break;
            case R.id.rl_switchVideo:
                if (mIsRecording) {
                    mCaptureTime = System.currentTimeMillis();
                    CaptureTask captureTask = new CaptureTask();
                    captureTask.execute();
                } else {
                    release();
                    mUiManager.switchCamera();
                }
                break;
            case R.id.iv_flash:
                int flashIndex = ++mFlashCurrentImg % mFlashImageArray.length;
                mFlashImageView.setImageResource(mFlashImageArray[flashIndex]);
                CameraData.getInstance().setVideoFlashMode(mFlashArray[flashIndex]);
                mUiManager.setPreviewFlashRequest(mFlashArray[flashIndex]);
                break;
            default:
                break;
        }
    }

    private void startCountTime() {
        mTimeChronometer.setBase(SystemClock.elapsedRealtime());//计时器清零
        int hour = (int) ((SystemClock.elapsedRealtime() - mTimeChronometer.getBase()) / 1000 / 60);
        mTimeChronometer.setFormat("0" + String.valueOf(hour) + ":%s");
        mTimeChronometer.start();
    }

    private void startRecord() {
        Log.i(TAG, "startRecord");
        releaseVideo();
        mUiManager.updateStorageSpaceAndHint(new UIManager.OnStorageUpdateDoneListener() {
            @TargetApi(Build.VERSION_CODES.M)
            @Override
            public void onStorageUpdateDone(long bytes) {
                 if(bytes <= Contants.LOW_STORAGE_THRESHOLD_BYTES){
                     Log.w(TAG, "Storage issue, ignore the start request");
                 }else{
                     mRecordRelativeLayout.setEnabled(false);
                     mVideoControlRelativeLayout.setVisibility(View.VISIBLE);
                     startCountTime();
                     mUiManager.mThumbnailView.setVisibility(View.GONE);
                     mRecordImageView.setImageResource(R.mipmap.ic_recording_indicator);
                     mSwicthVideoRelativeLayout.setVisibility(View.VISIBLE);
                     mSwitchVideoImageView.setImageResource(R.mipmap.ic_camera_normal);
                     mSettingImageView.setVisibility(View.GONE);
                     mIsRecording = true;
                     mUiManager.getActivity().enableKeepScreenOn(true);
                     CameraData.getInstance().setCanChangeMode(false);
                     mRecordTime = System.currentTimeMillis();
                     setupMediaRecorder(false);
                     if (mVideoSavePreviewPath != null) {
                         File f = new File(mVideoSavePreviewPath);
                         if (f.length() == 0 && f.delete()) {
                             mVideoSavePreviewPath = null;
                         }
                     }
                     CameraManager.getInstance().prepareVideoCamera(mVideoSurface);
                     try {
                         mMediaRecorder.start();
                     } catch (RuntimeException e) {
                         Log.e(TAG, "mMediaRecorder.start() fail!");
                         releaseVideo();
                         setupMediaRecorder(false);
                         CameraManager.getInstance().releaseVideoCamera();
                         CameraManager.getInstance().prepareVideoCamera(null);
                     }
                     mRecordRelativeLayout.setEnabled(true);
                     mUiManager.getAudioManager().requestAudioFocus(null, AudioManager.STREAM_MUSIC, AudioManager.AUDIOFOCUS_GAIN);
                 }
            }
        });

        Log.i(TAG, "startRecord end ");
    }

    private void stopRecord() {
        Log.i(TAG, "stopRecord");
        mVideoControlRelativeLayout.setVisibility(View.GONE);
        mRecordRelativeLayout.setEnabled(false);
        mUiManager.getActivity().enableKeepScreenOn(false);
        mPauseRecord = true;
        mRecordingTime = 0;
        mMediaRecorder.setOnErrorListener(null);
        mMediaRecorder.setOnInfoListener(null);
        CameraManager.getInstance().releaseVideoCamera();
        CameraManager.getInstance().prepareVideoCamera(null);
        try {
            VideoSaveTask task = new VideoSaveTask();
            task.execute();
        } catch (RuntimeException e) {
            mRecordRelativeLayout.setEnabled(true);
        }
        mVideoControlImageView.setImageResource(R.mipmap.play_normal);
        CameraData.getInstance().setCanChangeMode(true);
        setStopRecordUI();
        mIsRecording = false;
        mUiManager.updateStorageSpaceAndHint(null);
    }

    @RequiresApi(api = Build.VERSION_CODES.M)
    public void setVideoSurface() {
        if (mVideoSurface == null) {
            Log.e(TAG, "mVideoSurface == null");
            mVideoSurface = MediaCodec.createPersistentInputSurface();
        }
    }

    @RequiresApi(api = Build.VERSION_CODES.M)
    public Surface getVideoSurface() {
        setVideoSurface();
        mCameraLock.lock("setupMediaRecorder");
        setupMediaRecorder(true);
        mCameraLock.unlock("setupMediaRecorder");
        return mVideoSurface;
    }
    private boolean hasOneCamera(){

        return !(CameraData.getInstance().getHasBackCamera() && CameraData.getInstance().getHasFrontCamera());
    }

    public void setStopRecordUI() {
        if (mTimeChronometer != null) {
            mTimeChronometer.setBase(SystemClock.elapsedRealtime());
            mTimeChronometer.stop();
        }
        mUiManager.mThumbnailView.setVisibility(View.VISIBLE);
        if (mRecordImageView != null) {
            mRecordImageView.setImageResource(R.mipmap.ic_capture_video);
        }
        if (mSettingImageView != null) {
            mSettingImageView.setVisibility(View.VISIBLE);
        }
        if (mSwitchVideoImageView != null) {
            if(hasOneCamera()) {
                mSwicthVideoRelativeLayout.setVisibility(View.INVISIBLE);
                return;
            }
            mSwitchVideoImageView.setImageResource(R.mipmap.ic_switch_video_facing_holo_light_xlarge);
        }

    }

    public void releaseVideo() {
        Log.e(TAG, "releaseVideo");
        if (mIsRecording) {
            stopRecord();

        }
        if (mVideoSavePreviewPath != null) {
            File f = new File(mVideoSavePreviewPath);
            if (f.length() == 0 && f.delete()) {
                mVideoSavePreviewPath = null;
            }
        }
        if (mMediaRecorder != null) {
            mMediaRecorder.reset();
            mMediaRecorder.release();
            mMediaRecorder = null;
        }
        Log.e(TAG, "releaseVideo end");
    }

    @Override
    public void release() {
        mCameraLock.lock("releaseVideo");

        Log.i(TAG, "release");
        releaseVideo();
        mRecordRelativeLayout.setEnabled(true);
        mCameraLock.unlock("releaseVideo");
    }

    public void releaseSurface() {
        if (mVideoSurface != null) {
            mVideoSurface.release();
            Log.e(TAG, "releaseSurface");
            mVideoSurface = null;
        }
    }

    public int updateMeida() {
        long duration = System.currentTimeMillis() - mRecordTime;
        // mCurrentVideoValues.put(MediaStore.Video.Media.SIZE, new File(mVideoSaveTempPath).length());
        mCurrentVideoValues.put(MediaStore.Video.Media.DURATION, duration);
        mCurrentVideoValues.put(MediaStore.Video.Media.IS_PENDING, "0");
        int code = CameraUtils.updateMedia(getUiManager().getContext().getContentResolver(), mCurrentVideoValues, mUri);

        //CameraData.getInstance().setPhotoPath(mVideoSaveTempPath);
        return code;
    }

    @RequiresApi(api = Build.VERSION_CODES.M)
    private void setupMediaRecorder(boolean isForPreview) {
        mRecordTime = System.currentTimeMillis();
        long requestedSizeLimit = 0;
        if (mMediaRecorder != null) {
            mMediaRecorder.reset();
        }
        if (mMediaRecorder == null) {
            mMediaRecorder = new MediaRecorder();
        }
        Log.i(TAG, "mRecordTime: " + mRecordTime);

        String title = CameraUtils.generateVideoTitle(mRecordTime);
        mCurrentVideoValues = new ContentValues(9);
        mCurrentVideoValues.put(MediaStore.Video.Media.TITLE, title);
        mCurrentVideoValues.put(MediaStore.Video.Media.DISPLAY_NAME, title);
        mCurrentVideoValues.put(MediaStore.Video.Media.DATE_TAKEN, mRecordTime);
        mCurrentVideoValues.put(MediaStore.MediaColumns.DATE_MODIFIED, mRecordTime / 1000);
        mCurrentVideoValues.put(MediaStore.Video.Media.MIME_TYPE, "video/mp4");
        //  mCurrentVideoValues.put(MediaStore.Video.Media.DATA, mVideoSaveTempPath);
        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.Q) {
            //android Q中不再使用DATA字段，而用RELATIVE_PATH代替
            //RELATIVE_PATH是相对路径不是绝对路径
            //DCIM是系统文件夹，关于系统文件夹可以到系统自带的文件管理器中查看，不可以写没存在的名字
            mCurrentVideoValues.put(MediaStore.Images.Media.RELATIVE_PATH, "DCIM/Camera");
        } else {
            //Android Q以下版本
            //  mCurrentVideoValues.put(MediaStore.Images.ImageColumns.DATA, mVideoSaveTempPath);
        }
        mCurrentVideoValues.put(
                MediaStore.Video.Media.RESOLUTION, CameraData.getInstance().getPreviewWidth() + "x" + CameraData.getInstance().getPreviewHeight());
        mCurrentVideoValues.put("orientation", CameraUtils.getJpegRotation(CameraData.getInstance().getCameraId()));

        mMediaRecorder.setAudioSource(MediaRecorder.AudioSource.MIC);
        mMediaRecorder.setVideoSource(MediaRecorder.VideoSource.SURFACE);
        mMediaRecorder.setInputSurface(mVideoSurface);
        mMediaRecorder.setProfile(CamcorderProfile.get(selectRecVideoProfile()));
        if (isForPreview) {
            mVideoSavePreviewPath = CameraUtils.generateVideoFilePreviewPath(getUiManager().getContext(), title);
            mMediaRecorder.setOutputFile(mVideoSavePreviewPath);
        } else if (CameraData.getInstance().isVideoCaptureIntent()) {
            if(CameraData.getInstance().getIntentUri() != null){
                setOutputFilebyUri(CameraData.getInstance().getIntentUri());
            }else{
                Uri uri = CameraUtils.insertMedia(getUiManager().getContext().getContentResolver(), MediaStore.Video.Media.EXTERNAL_CONTENT_URI, mCurrentVideoValues);
                CameraData.getInstance().setPhotoUri(uri);
                setOutputFilebyUri(uri);
                CameraData.getInstance().setIntentUri(uri);
            }

            if (CameraData.getInstance().getIntentSize() >= 0) {
                requestedSizeLimit = CameraData.getInstance().getIntentSize();
            }
            if (CameraData.getInstance().getIntentDuration() >= 0) {
                mMediaRecorder.setMaxDuration(CameraData.getInstance().getIntentDuration() * 1000);
            }
            if (CameraData.getInstance().getIntentQuality() >= 0) {
            }
        } else {
            mCurrentVideoValues.put(MediaStore.Video.Media.IS_PENDING, "1");
            mUri = CameraUtils.insertMedia(getUiManager().getContext().getContentResolver(), MediaStore.Video.Media.EXTERNAL_CONTENT_URI, mCurrentVideoValues);
            CameraData.getInstance().setPhotoUri(mUri);
            setOutputFilebyUri(mUri);
        }
        long maxFileSize = mUiManager.getStorageSpaceBytes() - Contants.LOW_STORAGE_THRESHOLD_BYTES;
        if (requestedSizeLimit > 0 && requestedSizeLimit < maxFileSize) {
            maxFileSize = requestedSizeLimit;
        }
        Log.e(TAG,"requestedSizeLimit:"+requestedSizeLimit+"maxFileSize:"+maxFileSize);
        try {
            mMediaRecorder.setMaxFileSize(maxFileSize);
        } catch (RuntimeException exception) {
        }
        mMediaRecorder.setVideoSize(CameraData.getInstance().getPreviewWidth(), CameraData.getInstance().getPreviewHeight());
        mMediaRecorder.setOrientationHint(CameraUtils.getJpegRotation(CameraData.getInstance().getCameraId()));
        try {
            mMediaRecorder.prepare();
        } catch (IOException e) {
            Log.i(TAG, "mMediaRecorder prepare fail ");
            e.printStackTrace();
            mMediaRecorder.release();
            mVideoSurface.release();
            throw new RuntimeException();
        }
        mMediaRecorder.setOnErrorListener(this);
        mMediaRecorder.setOnInfoListener(this);
    }

    private int selectRecVideoProfile() {
        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.HONEYCOMB) {
            int frameHeight =  Math.min(CameraData.getInstance().getPreviewWidth(), CameraData.getInstance().getPreviewHeight());
            if (CamcorderProfile.hasProfile(CamcorderProfile.QUALITY_1080P)
                    && frameHeight == 1080)
                return CamcorderProfile.QUALITY_1080P;
            if (CamcorderProfile.hasProfile(CamcorderProfile.QUALITY_720P)
                    && frameHeight == 720)
                return CamcorderProfile.QUALITY_720P;
            if (CamcorderProfile.hasProfile(CamcorderProfile.QUALITY_480P)
                    && frameHeight == 480)
                return CamcorderProfile.QUALITY_480P;
        }

        if (CamcorderProfile.hasProfile(CamcorderProfile.QUALITY_LOW))
            return CamcorderProfile.QUALITY_LOW;

        return CamcorderProfile.QUALITY_LOW;
    }

    private void setOutputFilebyUri(Uri uri) {
        if(uri ==  null){
            return;
        }
        closeVideoFileDescriptor();
        ContentResolver contentResolver = getContext().getContentResolver();
        try {
            mVideoFileDescriptor = contentResolver.openFileDescriptor(uri, "rw");
            if (mVideoFileDescriptor != null && mVideoFileDescriptor.getFileDescriptor()!= null) {
                mMediaRecorder.setOutputFile(mVideoFileDescriptor.getFileDescriptor());
            }
        } catch (java.io.FileNotFoundException ex) {

            Log.e(TAG, ex.toString());
        }
    }

    //    @Override
//    public void onCountDownEnd() {
//        Log.e(TAG,"onCountDownEnd");
//        startRecord();
//        mIsRecording = true;
//        startCountTime();
//        mRecordImageView.setImageResource(R.mipmap.ic_recording_indicator);
//    }
    private void closeVideoFileDescriptor() {
        if (mVideoFileDescriptor != null) {
            try {
                mVideoFileDescriptor.close();

            } catch (IOException e) {
                Log.e(TAG, "Fail to close fd", e);
            }
            mVideoFileDescriptor = null;
        }
    }

    @Override
    public void onError(MediaRecorder mediaRecorder, int what, int extra) {
        Log.e(TAG, "MediaRecorder error. what=" + what + ". extra=" + extra);
        if (what == MediaRecorder.MEDIA_RECORDER_ERROR_UNKNOWN) {
            // We may have run out of space on the sdcard.
            stopRecord();
            mUiManager.updateStorageSpaceAndHint(null);
        }
    }

    @Override
    public void onInfo(MediaRecorder mediaRecorder, int what, int extra) {
        if (what == MediaRecorder.MEDIA_RECORDER_INFO_MAX_DURATION_REACHED) {
            if (mIsRecording) {
                stopRecord();
            }
        } else if (what == MediaRecorder.MEDIA_RECORDER_INFO_MAX_FILESIZE_REACHED) {
            if (mIsRecording) {
                stopRecord();
            }

            // Show the toast.
            Toast.makeText(mUiManager.getContext(), R.string.video_reach_size_limit,
                    Toast.LENGTH_LONG).show();
        }
    }

    private class VideoSaveTask extends AsyncTask<Void, Void, Bitmap> {

        @Override
        protected void onPostExecute(Bitmap bitmap) {
            getUiManager().updateThumbnail(bitmap);
            mRecordRelativeLayout.setEnabled(true);
            Log.i(TAG, "stopRecord end");
        }

        @Override
        protected Bitmap doInBackground(Void... voids) {
            Bitmap bitmap = null;
            try {
              if (mMediaRecorder != null) {
                  mMediaRecorder.stop();
              }
            }catch (RuntimeException e){
                Log.e(TAG, "stop fail",  e);
            }
            if (CameraData.getInstance().isVideoCaptureIntent()) {
//                getUiManager().generatePrevieBitmap();
//                Bitmap bitmap = getUiManager().getPreviewBitmap();

                MediaMetadataRetriever retriever = new MediaMetadataRetriever();
                try {
                    if (mVideoFileDescriptor.getFileDescriptor() != null) {
                        retriever.setDataSource(mVideoFileDescriptor.getFileDescriptor());
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
                Contants.EventObject object = new Contants.EventObject(Contants.MsgType.MSG_ON_UPDATE_BITMAP, bitmap);
                EventBus.getDefault().post(object);
            } else {
                int code = updateMeida();
                Log.i(TAG, "addVideo uri : " + mUri + "code:" + code);
                bitmap = ThumbnailUtils.getVideoThumbnailBitmapByfd(mVideoFileDescriptor.getFileDescriptor(), CameraUtils.getJpegRotation(CameraData.getInstance().getCameraId()));
                getUiManager().updateFile(mUri);
                CameraData.getInstance().setPhotoUri(mUri);
                mUiManager.getContext().sendBroadcast(new Intent(ACTION_NEW_VIDEO, mUri));
            }
            mCurrentVideoValues = null;
            closeVideoFileDescriptor();
            return bitmap;
        }
    }


    private class CaptureTask extends AsyncTask<Void, Void, Void> {
        @Override
        protected Void doInBackground(Void... voids) {
            Log.i(TAG, "BlurTask doInBackground");
            getUiManager().generatePrevieBitmap();
            Bitmap bitmap = getUiManager().getPreviewBitmap();
            SimpleDateFormat format = new SimpleDateFormat("yyyy-MM-dd-HH-mm-ss");
            String name = format.format(mCaptureTime);
            mPath = CameraUtils.generateFilePath(name);
            Matrix matrix = new Matrix();
            matrix.setRotate(CameraData.getInstance().getOrientation(), bitmap.getWidth() * 0.5f, bitmap.getHeight() * 0.5f);
            Bitmap bitmap2 = Bitmap.createBitmap(bitmap, 0, 0, bitmap.getWidth(), bitmap.getHeight(), matrix, true);
            Uri uri = CameraUtils.insertMedia(getContext().getContentResolver(), name, mCaptureTime, 0, mPath, bitmap.getWidth(), bitmap.getHeight());
            CameraUtils.saveJpeg(getContext().getContentResolver(), uri, bitmap2);
            CameraUtils.addExif(getContext(), uri, mCaptureTime);
            bitmap.recycle();
            bitmap2.recycle();
            Contants.EventObject object = new Contants.EventObject(Contants.MsgType.MSG_ON_PICTURETAKEN_FINISH, uri);
            EventBus.getDefault().post(object);
            return null;
        }

    }

    private class ThumSaveTask extends AsyncTask<Void, Void, Bitmap> {
        private Bitmap mBitmap = null;
        private int mJpegRotation = 0;

        public ThumSaveTask(Bitmap bitmap, int jpegRotation) {
            this.mBitmap = bitmap;
            this.mJpegRotation = jpegRotation;
        }

        @Override
        protected Bitmap doInBackground(Void... voids) {
            Bitmap bitmap = ThumbnailUtils.getThumbnailBitmap(mBitmap, mJpegRotation);
            return bitmap;
        }

        @Override
        protected void onPostExecute(Bitmap bitmap) {
            Contants.EventObject object = new Contants.EventObject(Contants.MsgType.MSG_ON_UPDATE_THUMBNAIL, bitmap);
            EventBus.getDefault().post(object);
        }
    }
}
