package com.allwinner.camera.ui;

import android.content.Intent;
import android.media.Image;
import android.media.ImageReader;
import android.os.AsyncTask;
import android.os.Build;
import android.os.Bundle;
import android.provider.MediaStore;
import android.support.annotation.RequiresApi;
import android.util.Log;
import android.view.View;
import android.widget.FrameLayout;
import android.widget.RelativeLayout;
import android.widget.SeekBar;
import android.widget.TextView;

import com.alibaba.android.mnnkit.actor.FaceDetector;
import com.allwinner.camera.R;
import com.allwinner.camera.SettingFragmentActivity;
import com.allwinner.camera.data.CameraData;
import com.allwinner.camera.data.Contants;
import com.allwinner.camera.utils.CameraUtils;
import com.allwinner.camera.utils.FaceExecutor;
import com.allwinner.camera.utils.SharedPreferencesUtils;
import com.allwinner.camera.views.CircleRotateImageView;
import com.allwinner.camera.views.RotateImageView;
import java.nio.ByteBuffer;
import java.util.Set;

public class AutoModeUI extends BaseModeUI implements View.OnClickListener , SeekBar.OnSeekBarChangeListener{
    private final String TAG = "AutoModeUI";
    private final RelativeLayout mTimerRelativeLayout;
    private final RelativeLayout mFlashRelativeLayout;
    private final RelativeLayout mBeautyRelativeLayout;
    private final RelativeLayout mFilterRelativeLayout;
    private final RelativeLayout mSettingRelativeLayout;
    private final RelativeLayout mFloatIconRelativeLayout;
    private final RelativeLayout mCancelRelativeLayout;
    private FaceDetector mFaceDetector;


    private CircleRotateImageView mSwicthcameraImageView;
    private CircleRotateImageView mCaptureImageView;
    private int mTimerCurrentImg = 0;
    private int mFlashCurrentImg = 0;
    private int mBeautyCurrentImg = 0;
    private RotateImageView mTimerImageView;
    private RotateImageView mFlashImageView;
    private RotateImageView mBeautyImageView;
    private RotateImageView mFilterImageView;
    private RotateImageView mSettingImageView;
    private RelativeLayout mCaptureRelativeLayout;
    private RelativeLayout mSwicthcameraRelativeLayout;
    private int[] mBeautyImageArray = {R.mipmap.ic_effects_faces_normol, R.mipmap.ic_video_effects_faces_big_eyes_holo_dark};
    private int mWidth;
    private int mHeight;
    private boolean mMirror = false;
    private ImageReader mImageReader = null;
    private int mCameraId;
    private int mJpegRotation;
    private FaceExecutor mExecutor;
    private boolean mDoingConvert = false;

    private boolean mBeautyMode = false;
    private byte[] mYuvbuffer;
    private int mPreBufferSize;
    private int mBufferSize;
    private RelativeLayout mFaceAdjustRelativeLayout;
    private SeekBar mSmoothSeekbar;
    private SeekBar mWhiteSeekbar;
    private SeekBar mBigeyeSeekbar;
    private SeekBar mFaceLiftSeekbar;
    private TextView mProgressSmoothText;
    private TextView mProgressWhiteText;
    private TextView mProgressBigeyeText;
    private TextView mProgressFaceliftText;


    public AutoModeUI(View rootView, UIManager uiManager) {
        super(rootView, uiManager);
        mCaptureRelativeLayout = rootView.findViewById(R.id.rl_capture);
        mCaptureRelativeLayout.setOnClickListener(this);
        mSwicthcameraRelativeLayout = rootView.findViewById(R.id.rl_switchCamera);
        mSwicthcameraRelativeLayout.setOnClickListener(this);
        mCaptureImageView = rootView.findViewById(R.id.iv_rotateCapture);
        mTimerImageView = rootView.findViewById(R.id.iv_timer);
        mTimerRelativeLayout = rootView.findViewById(R.id.rl_timer);
        mTimerRelativeLayout.setOnClickListener(this);
        mFlashImageView = rootView.findViewById(R.id.iv_flash);
        mFlashRelativeLayout = rootView.findViewById(R.id.rl_flash);

        mFlashRelativeLayout.setOnClickListener(this);
        mBeautyImageView = rootView.findViewById(R.id.iv_beauty);
        mBeautyRelativeLayout = rootView.findViewById(R.id.rl_beauty);
        mBeautyRelativeLayout.setOnClickListener(this);
        mFilterImageView = rootView.findViewById(R.id.iv_filter);
        mFilterRelativeLayout = rootView.findViewById(R.id.rl_filter);
        mFilterRelativeLayout.setOnClickListener(this);
        mSettingImageView = rootView.findViewById(R.id.iv_camera_settings);
        mSettingRelativeLayout = rootView.findViewById(R.id.rl_setting);
        mSettingRelativeLayout.setOnClickListener(this);
        mSwicthcameraImageView = rootView.findViewById(R.id.iv_switchCamera);
        mFaceAdjustRelativeLayout = rootView.findViewById(R.id.rl_face_adjust);
        mSmoothSeekbar = rootView.findViewById(R.id.seekbar_bar_smooth);
        mWhiteSeekbar = rootView.findViewById(R.id.seekbar_bar_white);
        mBigeyeSeekbar = rootView.findViewById(R.id.seekbar_bar_bigeye);
        mFaceLiftSeekbar = rootView.findViewById(R.id.seekbar_bar_facelift);
        mSmoothSeekbar.setOnSeekBarChangeListener(this);
        mWhiteSeekbar.setOnSeekBarChangeListener(this);
        mBigeyeSeekbar.setOnSeekBarChangeListener(this);
        mFaceLiftSeekbar.setOnSeekBarChangeListener(this);
        mProgressSmoothText = rootView.findViewById(R.id.progress_smooth);
        mProgressWhiteText = rootView.findViewById(R.id.progress_white);
        mProgressBigeyeText = rootView.findViewById(R.id.progress_bigeye);
        mProgressFaceliftText = rootView.findViewById(R.id.progress_facelift);
        mFloatIconRelativeLayout = rootView.findViewById(R.id.rl_float_icon);
        mFloatIconRelativeLayout.setOnClickListener(this);
        mCancelRelativeLayout = rootView.findViewById(R.id.cancel);
        mCancelRelativeLayout.setOnClickListener(this);
        initView();
        mUiManager.updateTimerView(mTimerImageView);
        mUiManager.updateFlashView(mFlashRelativeLayout,mFlashImageView);
        mExecutor = new FaceExecutor();
        resetFaceStatus();
    }

    private void initView() {
        if(!(CameraData.getInstance().getHasBackCamera() && CameraData.getInstance().getHasFrontCamera())){
            mSwicthcameraRelativeLayout.setVisibility(View.INVISIBLE);
        }
        int filterBottomMargin = CameraData.getInstance().getScreenHeight() - CameraData.getInstance().getScreenHeight_4_TO_3();
        if (mFaceAdjustRelativeLayout != null) {
            if (mFaceAdjustRelativeLayout.getLayoutParams() instanceof FrameLayout.LayoutParams) {
                FrameLayout.LayoutParams filterViewParams = (FrameLayout.LayoutParams)
                        mFaceAdjustRelativeLayout.getLayoutParams();

                filterViewParams.bottomMargin = filterBottomMargin;
                mFaceAdjustRelativeLayout.setLayoutParams(filterViewParams);
            }
        }
    }

    @Override
    public void onFlashStatus(final boolean isFlashAvaliable) {
            mUiManager.getActivity().runOnUiThread(new Runnable() {
                @Override
                public void run() {
                    mUiManager.updateFlashView(mFlashRelativeLayout,mFlashImageView);
                }
            });
    }

    @Override
    public void onCountDownEnd() {
    	super.onCountDownEnd();
    }

    public void onCountDownCancel() {
        mCaptureRelativeLayout.setEnabled(true);
    }

    @Override
    public void onOrientationChanged(int orientationCompensation) {
        mCaptureImageView.onOrientationChanged(orientationCompensation);
        mSwicthcameraImageView.onOrientationChanged(orientationCompensation);
        mTimerImageView.onOrientationChanged(orientationCompensation);
        mFlashImageView.onOrientationChanged(orientationCompensation);
        mBeautyImageView.onOrientationChanged(orientationCompensation);
        mFilterImageView.onOrientationChanged(orientationCompensation);
        mSettingImageView.onOrientationChanged(orientationCompensation);
        mUiManager.setFaceResult(null);
    }

    public void setCaptureEnable() {
        mCaptureRelativeLayout.setEnabled(true);
    }

    @Override
    public void onResume() {
        super.onResume();
    }
    @RequiresApi(api = Build.VERSION_CODES.M)
    public void handleVoiceTackPictrueIntent(){
        if(MediaStore.INTENT_ACTION_STILL_IMAGE_CAMERA.equals(mUiManager.getActivity().getIntent().getAction())){
            if(mUiManager.getActivity().isVoiceInteractionRoot()){
                if(mUiManager.getActivity().getIntent()!=null){
                    Bundle extras = mUiManager.getActivity().getIntent().getExtras();
                    boolean  extra = false;
                    if(extras.containsKey("com.google.assistant.extra.CAMERA_OPEN_ONLY")){
                        extra = extras.getBoolean("com.google.assistant.extra.CAMERA_OPEN_ONLY",false);
                    }
                    if( extra){
                        return;
                    }
                    final Set<String> categories = mUiManager.getActivity().getIntent().getCategories();
                    if ( categories != null&& categories.contains("android.intent.category.VOICE")){
                        startCountDown(0);
                    }
                }
            }

        }

    }

    @Override
    public void onClick(View v) {
        switch (v.getId()) {
            case R.id.rl_capture:
                mCaptureRelativeLayout.setEnabled(false);
                startCountDown((Integer) SharedPreferencesUtils.getParam(getContext(), "time", 0));
                break;
            case R.id.rl_flash:
                int flashIndex = ++mFlashCurrentImg % Contants.FLASHIMAGEARRAY.length;
                mFlashImageView.setImageResource(Contants.FLASHIMAGEARRAY[flashIndex]);
                CameraData.getInstance().setCaptureFlashMode(Contants.FLASHARRAY[flashIndex]);

                break;
            case R.id.rl_switchCamera:
                mUiManager.switchCamera();
                break;
            case R.id.rl_timer:
                int timeIndex = ++mTimerCurrentImg % Contants.TIMEARRAY.length;
                mTimerImageView.setImageResource(Contants.TIMEIMAGEARRAY[timeIndex]);
                SharedPreferencesUtils.setParam(getContext(), "time", Contants.TIMEARRAY[timeIndex]);
                break;
            case R.id.rl_setting:
                Intent intent = new Intent(getContext(), SettingFragmentActivity.class);
                intent.addFlags(Intent.FLAG_ACTIVITY_NEW_TASK);
                getContext().startActivity(intent);
                break;
            case R.id.rl_filter:
                mUiManager.setFilterVisibility();
                //mUiManager.setIsFling(false);
                break;
            case R.id.rl_beauty:
                //mUiManager.setIsFling(false);
                mBeautyMode = !mBeautyMode;
                mUiManager.setFaceBeautyOn(mBeautyMode);
                CameraData.getInstance().setFaceBeautyOn(mBeautyMode);
                setShareprefenceBeautystatus(mBeautyMode);
                int beautyIndex = 0;
                Log.e(TAG, "mBeautyMode:" + mBeautyMode);
                if (mBeautyMode) {
                    beautyIndex = 1;
                    mFloatIconRelativeLayout.setVisibility(View.VISIBLE);

                }else{
                    mFloatIconRelativeLayout.setVisibility(View.GONE);
                    setFaceAdjustInVisible();
                }
                mBeautyImageView.setImageResource(mBeautyImageArray[beautyIndex]);
                break;
            case R.id.rl_float_icon:
                if(mFaceAdjustRelativeLayout.isShown()){
                    setFaceAdjustInVisible();
                }else {
                    setFaceAdjustVisible();
                }
                break;
            case R.id.cancel:
                setFaceAdjustInVisible();
                break;

                    
            default:
                
                break;
        }

    }
    public void setFaceAdjustVisible(){
        if(mUiManager.getFilterViewVisbility()){
            mUiManager.setFilterViewVisible(false);
        }
        android.util.Log.e("setFaceAdjustVisible",android.util.Log.getStackTraceString(new Throwable()));
        mFaceAdjustRelativeLayout.setVisibility(View.VISIBLE);
        mUiManager.setIsFling(false);
        mUiManager.resetZoomView(true,mFaceAdjustRelativeLayout);
        CameraData.getInstance().setFaceAdjustVisibility(true);
    }
    public void setFaceAdjustInVisible(){
        mFaceAdjustRelativeLayout.setVisibility(View.GONE);
        mUiManager.setIsFling(true);
        mUiManager.resetZoomView(false,mFaceAdjustRelativeLayout);
        CameraData.getInstance().setFaceAdjustVisibility(false);
    }
    public void onModeChanged() {
        setFaceAdjustInVisible();
    }
    private boolean getShareprefenceBeautystatus() {
        return (boolean) SharedPreferencesUtils.getParam(mUiManager.getContext(), Contants.ISBEAUTY, false);
    }

    private void setShareprefenceBeautystatus(boolean value) {
        SharedPreferencesUtils.setParam(mUiManager.getContext(), Contants.ISBEAUTY, value);
    }

    public void createKitSuccess(FaceDetector faceDetector){
        mFaceDetector = faceDetector;
    }
    @Override
    public void release() {
        Log.e(TAG,"release");
        mCaptureRelativeLayout.setEnabled(true);
        saveFaceAdjustValue();
        setFaceAdjustInVisible();
        super.release();
    }

    @Override
    public void enter() {
        mBeautyImageView.setImageResource(mBeautyImageArray[0]);
        super.enter();
        mUiManager.updateTimerView(mTimerImageView);
        resetFaceStatus();
        mUiManager.updateFlashView(mFlashRelativeLayout,mFlashImageView);

    }


    public void resetFaceStatus() {
        mBeautyMode = getShareprefenceBeautystatus();
        mUiManager.setFaceBeautyOn(mBeautyMode);
        CameraData.getInstance().setFaceBeautyOn(mBeautyMode);
        int beautyIndex = 0;
        if (mBeautyMode) {
            beautyIndex = 1;
            mFloatIconRelativeLayout.setVisibility(View.VISIBLE);
        }
        mBeautyImageView.setImageResource(mBeautyImageArray[beautyIndex]);
        resetFaceAdjustValue();
    }
    private void saveFaceAdjustValue() {
        SharedPreferencesUtils.setParam(getContext(), Contants.KEY_SMOOTH_PROGRESS,CameraData.getInstance().getSmoothProgress());
        SharedPreferencesUtils.setParam(getContext(), Contants.KEY_WHITE_PROGRESS,CameraData.getInstance().getWhiteProgress());
        SharedPreferencesUtils.setParam(getContext(), Contants.KEY_BIGEYE_PROGRESS,CameraData.getInstance().getBigeyeProgress());
        SharedPreferencesUtils.setParam(getContext(), Contants.KEY_FACELIFT_PROGRESS,CameraData.getInstance().getFaceliftProgress());
    }

//    @Override
//    public Surface getPreviewSurface() {
//        if (mImageReader != null) {
//            synchronized(mImageReader) {
//                mImageReader.close();
//            }
//        }
//        mImageReader = ImageReader.newInstance(CameraData.getInstance().getPreviewWidth(),
//                CameraData.getInstance().getPreviewHeight(), ImageFormat.YUV_420_888, 3);
//        mImageReader.setOnImageAvailableListener(new ImageReader.OnImageAvailableListener() {
//            @Override
//            public void onImageAvailable(ImageReader reader) {
//                try (Image image = reader.acquireNextImage()) {
//                    mCameraId = CameraData.getInstance().getCameraId();
//                    if (mCameraId == CameraData.getInstance().getBackCameraId()) {
//                        //后置
//                        mMirror = false;
//                    } else {
//                        mMirror = true;
//                    }
//                    if (image.getFormat() == ImageFormat.JPEG) {
//                        //  onJpegDataTaken(image);
//                    } else {
//                        synchronized(mImageReader) {
//                            onYuvDataTaken(image);
//                        }
//                    }
//                }
//            }
//        }, CameraData.getInstance().getCameraHandler());
//        return mImageReader.getSurface();
//    }

    public void onYuvDataTaken(Image image) {
        mWidth = image.getWidth();
        mHeight = image.getHeight();
        int rowstride = image.getPlanes()[0].getRowStride();
        ByteBuffer ybuffer = image.getPlanes()[0].getBuffer();
        ByteBuffer uvbuffer = image.getPlanes()[2].getBuffer();
        //ByteBuffer vubuffer = image.getPlanes()[1].getBuffer();
        mBufferSize = rowstride * mHeight * 3 / 2;
        if(mYuvbuffer == null || mBufferSize != mPreBufferSize) {
            mYuvbuffer = new byte[mBufferSize];
        }
        mPreBufferSize = mBufferSize;
        ybuffer.get(mYuvbuffer, 0, rowstride * mHeight);
        uvbuffer.get(mYuvbuffer, rowstride * mHeight, uvbuffer.remaining());
        image.close();
        ybuffer.clear();
        uvbuffer.clear();
        mJpegRotation = CameraUtils.getPreviewRotation(mCameraId);
        mUiManager.setYuvData(mYuvbuffer, mWidth, mHeight, mJpegRotation, mMirror);
        if (CameraData.getInstance().getCurrentModeType() == Contants.ModeType.AutoMode) {
            if (mBeautyMode) {
                if (mDoingConvert) {
                    image.close();
                    return;
                }
                mDoingConvert = true;
                FaceTrackTask faceTrackTask = new FaceTrackTask(mYuvbuffer, mJpegRotation);
                faceTrackTask.execute();
            } else {
                mUiManager.setFilterType(CameraData.getInstance().getFilterType());
            }
        }

    }

    @Override
    public void onProgressChanged(SeekBar seekBar, int progress, boolean b) {
        switch (seekBar.getId()) {
            case R.id.seekbar_bar_smooth:
                mProgressSmoothText.setText(progress+"%");
                CameraData.getInstance().setSmoothProgress(progress);
                break;
            case R.id.seekbar_bar_white:
                mProgressWhiteText.setText(progress+"%");
                CameraData.getInstance().setWhiteProgress(progress);
                break;
            case R.id.seekbar_bar_bigeye:
                mProgressBigeyeText.setText(progress+"%");
                CameraData.getInstance().setBigeyeProgress(progress);
                break;
            case R.id.seekbar_bar_facelift:
                mProgressFaceliftText.setText(progress+"%");
                CameraData.getInstance().setFaceliftProgress(progress);
                break;
        }
    }


    private void resetFaceAdjustValue() {
        int smoothProgress=(int) SharedPreferencesUtils.getParam(getContext(),Contants.KEY_SMOOTH_PROGRESS,50);
        CameraData.getInstance().setSmoothProgress(smoothProgress);
        mSmoothSeekbar.setProgress(smoothProgress);
        int whiteProgress=(int) SharedPreferencesUtils.getParam(getContext(),Contants.KEY_WHITE_PROGRESS,50);
        CameraData.getInstance().setWhiteProgress(whiteProgress);
        mWhiteSeekbar.setProgress(whiteProgress);
        int bigeyeProgress=(int) SharedPreferencesUtils.getParam(getContext(),Contants.KEY_BIGEYE_PROGRESS,50);
        CameraData.getInstance().setBigeyeProgress(bigeyeProgress);
        mBigeyeSeekbar.setProgress(bigeyeProgress);
        int faceliftProgress=(int) SharedPreferencesUtils.getParam(getContext(),Contants.KEY_FACELIFT_PROGRESS,50);
        CameraData.getInstance().setFaceliftProgress(faceliftProgress);
        mFaceLiftSeekbar.setProgress(faceliftProgress);
    }
    @Override
    public void onStartTrackingTouch(SeekBar seekBar) {

    }

    @Override
    public void onStopTrackingTouch(SeekBar seekBar) {

    }

    private class FaceTrackTask extends AsyncTask<Void, Void, Void> {
        private byte[] mData = null;
        private int mJpegRotation = 0;

        public FaceTrackTask(byte[] data, int jpegRotation) {
            this.mData = data;
            this.mJpegRotation = jpegRotation;
        }

        @Override
        protected Void doInBackground(Void... voids) {

            int inAngle = CameraUtils.getInAngle();
            int outAngle = CameraUtils.getOutAngle();
            CameraUtils.doFaceDetector(mUiManager,mFaceDetector,mData, mWidth, mHeight,inAngle,outAngle,CameraData.getInstance().isCurrentFront(),false);
            mDoingConvert = false;
            return null;
        }
    }
}
