package com.allwinner.camera.ui;

import android.content.Intent;
import android.util.Log;
import android.view.View;
import android.widget.FrameLayout;
import android.widget.RelativeLayout;

import com.allwinner.camera.R;
import com.allwinner.camera.SettingFragmentActivity;
import com.allwinner.camera.data.CameraData;
import com.allwinner.camera.data.Contants;
import com.allwinner.camera.utils.SharedPreferencesUtils;
import com.allwinner.camera.views.CircleRotateImageView;
import com.allwinner.camera.views.RotateImageView;

public class SquareModeUI extends BaseModeUI implements View.OnClickListener {
    private final String TAG = "SquareModeUI";
    private final RelativeLayout mFlashRelativeLayout;
    private FrameLayout mTopCover;
    private FrameLayout mBottomCover;
    private int mTimerCurrentImg = 0;
    private int mFlashCurrentImg = 0;
    private CircleRotateImageView mSwicthcameraImageView;
    private CircleRotateImageView mCaptureImageView;
    private int mCurrentImg = 0;
    private final RelativeLayout mTimerRelativeLayout;
    private final RelativeLayout mBeautyRelativeLayout;
    private final RelativeLayout mFilterRelativeLayout;
    private final RelativeLayout mSettingRelativeLayout;
    private RotateImageView mTimerImageView;
    private RotateImageView mBeautyImageView;
    private RotateImageView mFilterImageView;
    private RotateImageView mSettingImageView;
    private RotateImageView mFlashImageView;
    private RelativeLayout mCaptureRelativeLayout;
    private RelativeLayout mSwicthcameraRelativeLayout;


    public SquareModeUI(View rootView, UIManager uiManager) {
        super(rootView, uiManager);
        mCaptureRelativeLayout = rootView.findViewById(R.id.rl_capture);
        mCaptureRelativeLayout.setOnClickListener(this);
        mSwicthcameraRelativeLayout = rootView.findViewById(R.id.rl_switchCamera);
        mSwicthcameraRelativeLayout.setOnClickListener(this);
        mCaptureImageView = rootView.findViewById(R.id.iv_rotateCapture);
        mTimerImageView = rootView.findViewById(R.id.iv_timer);
        mTimerRelativeLayout = rootView.findViewById(R.id.rl_timer);
        mTimerRelativeLayout.setOnClickListener(this);
        mBeautyImageView = rootView.findViewById(R.id.iv_beauty);
        mBeautyRelativeLayout = rootView.findViewById(R.id.rl_beauty);
        mBeautyRelativeLayout.setOnClickListener(this);
        mBeautyRelativeLayout.setVisibility(View.GONE);
        mFilterImageView = rootView.findViewById(R.id.iv_filter);
        mFilterRelativeLayout = rootView.findViewById(R.id.rl_filter);
        mFilterRelativeLayout.setOnClickListener(this);
        mSettingImageView = rootView.findViewById(R.id.iv_camera_settings);
        mSettingRelativeLayout = rootView.findViewById(R.id.rl_setting);
        mSettingRelativeLayout.setOnClickListener(this);
        mSwicthcameraImageView = rootView.findViewById(R.id.iv_switchCamera);
        mFlashRelativeLayout = rootView.findViewById(R.id.rl_flash);
        mFlashImageView = rootView.findViewById(R.id.iv_flash);
        mUiManager.updateFlashView(mFlashRelativeLayout,mFlashImageView);
        mFlashRelativeLayout.setOnClickListener(this);
        mTopCover = rootView.findViewById(R.id.top_cover);
        mBottomCover = rootView.findViewById(R.id.bottom_cover);
        initView();
        mUiManager.updateTimerView(mTimerImageView);
    }

    private void initView() {
        if(!(CameraData.getInstance().getHasBackCamera() && CameraData.getInstance().getHasFrontCamera())){
            mSwicthcameraRelativeLayout.setVisibility(View.INVISIBLE);
        }
        int margin = (CameraData.getInstance().getScreenHeight_4_TO_3() - CameraData.getInstance().getSreenWidth()) / 2;
        FrameLayout.LayoutParams topCoverLayoutParams = (FrameLayout.LayoutParams)
                mTopCover.getLayoutParams();
        topCoverLayoutParams.topMargin = 0;
        topCoverLayoutParams.height = margin;
        mTopCover.setLayoutParams(topCoverLayoutParams);
        FrameLayout.LayoutParams bottomCoverLayoutParams = (FrameLayout.LayoutParams)
                mBottomCover.getLayoutParams();
        bottomCoverLayoutParams.topMargin = margin + CameraData.getInstance().getSreenWidth();
        bottomCoverLayoutParams.height = CameraData.getInstance().getScreenHeight_4_TO_3();
        mBottomCover.setLayoutParams(bottomCoverLayoutParams);
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
        mCaptureRelativeLayout.setEnabled(false);

    }

    public void onCountDownCancel() {
        mCaptureRelativeLayout.setEnabled(true);
    }

    @Override
    public void onOrientationChanged(int orientationCompensation) {
        mCaptureImageView.onOrientationChanged(orientationCompensation);
        mSwicthcameraImageView.onOrientationChanged(orientationCompensation);
        mTimerImageView.onOrientationChanged(orientationCompensation);
        mBeautyImageView.onOrientationChanged(orientationCompensation);
        mFilterImageView.onOrientationChanged(orientationCompensation);
        mSettingImageView.onOrientationChanged(orientationCompensation);
        mFlashImageView.onOrientationChanged(orientationCompensation);
    }

    public void setCaptureEnable() {
        mCaptureRelativeLayout.setEnabled(true);
    }

    @Override
    public void release() {
        super.release();
        mCaptureRelativeLayout.setEnabled(true);
    }

    @Override
    public void enter() {
        super.enter();
        mUiManager.updateTimerView(mTimerImageView);
        mUiManager.updateFlashView(mFlashRelativeLayout,mFlashImageView);
    }

    @Override
    public void onClick(View v) {
        switch (v.getId()) {

            case R.id.rl_capture:
                //cancelCountDown();
                startCountDown((Integer) SharedPreferencesUtils.getParam(getContext(), "time", 0));
                break;
            case R.id.rl_switchCamera:
                mUiManager.switchCamera();
                break;
            case R.id.rl_flash:
                int flashIndex = ++mFlashCurrentImg % Contants.FLASHIMAGEARRAY.length;
                mFlashImageView.setImageResource(Contants.FLASHIMAGEARRAY[flashIndex]);
                CameraData.getInstance().setCaptureFlashMode(Contants.FLASHARRAY[flashIndex]);
                break;
            case R.id.rl_timer:
                int timeIndex = ++mCurrentImg % Contants.TIMEARRAY.length;
                mTimerImageView.setImageResource(Contants.TIMEIMAGEARRAY[timeIndex]);
                SharedPreferencesUtils.setParam(getContext(), "time",Contants.TIMEARRAY[timeIndex]);
                break;
            case R.id.rl_setting:
                Intent intent = new Intent(getContext(), SettingFragmentActivity.class);
                intent.addFlags(Intent.FLAG_ACTIVITY_NEW_TASK);
                getContext().startActivity(intent);
                break;
            case R.id.rl_filter:
                Log.e(TAG, "filter click");
                mUiManager.setFilterVisibility();
                //mUiManager.setIsFling(false);
                break;
            default:
                break;
        }
    }

    
}
