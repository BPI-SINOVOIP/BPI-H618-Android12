package com.allwinner.camera.views;


import android.Manifest;
import android.app.Activity;
import android.app.AlertDialog;
import android.app.Dialog;
import android.app.DialogFragment;
import android.app.Fragment;
import android.content.Context;
import android.content.DialogInterface;
import android.graphics.Color;
import android.os.Bundle;
import android.provider.MediaStore;
import android.support.annotation.Nullable;
import android.support.design.widget.TabLayout;
import android.support.v13.app.FragmentStatePagerAdapter;
import android.support.v4.view.ViewPager;
import android.util.DisplayMetrics;
import android.util.Log;
import android.view.LayoutInflater;
import android.view.MotionEvent;
import android.view.ScaleGestureDetector;
import android.view.View;
import android.view.ViewGroup;
import android.view.ViewParent;
import android.view.WindowManager;
import android.widget.FrameLayout;

import com.allwinner.camera.R;
import com.allwinner.camera.cameracontrol.CameraManager;
import com.allwinner.camera.data.CameraData;
import com.allwinner.camera.data.Contants;
import com.allwinner.camera.modes.ModeManager;
import com.allwinner.camera.ui.UIManager;


public class PaperFragment extends Fragment implements ModeViewPaper.OnEventListener {
    private final String TAG = "PaperFragment";
    private ModeViewPaper mViewPaper;
    private SlideTabHost mTabLayout;
    private ModeManager modeManager;
    private int mCurrentItemIndex = Contants.AUTOMODEINDEX;
    private UIManager mUiManager;
    private static final int REQUEST_CAMERA_PERMISSION = 1;
    private float mSreenWidth;
    private WindowManager mWindowManager;
    private String mIntentAction;

    public void setUiManager(UIManager uiManager) {
        mUiManager = uiManager;
        Log.i("PaperFragment", "setUiManager: " + mUiManager);
    }

    @Nullable
    @Override
    public View onCreateView(LayoutInflater inflater, @Nullable ViewGroup container, Bundle savedInstanceState) {
        return inflater.inflate(R.layout.paperfragment, container, false);
    }

    @Override
    public void onViewCreated(View view, @Nullable Bundle savedInstanceState) {
        super.onViewCreated(view, savedInstanceState);
        mViewPaper = view.findViewById(R.id.viewPager);
        mTabLayout = view.findViewById(R.id.tabLayout);
        modeManager = new ModeManager(getActivity().getApplicationContext(), mUiManager);
        mIntentAction = getActivity().getIntent().getAction();
        String defaultTitle = "auto";
        if (isVideoIntent()) {
            defaultTitle = "video";
        } else if (isCaptureIntent()) {
            // Capture intent.
            mCurrentItemIndex = Contants.AUTOMODEINDEX;
        }
        mTabLayout.setTitles(modeManager.getModeTitle(), defaultTitle);
        mTabLayout.setOnTabSelectedListener(OnTabSelectedListener);

        initTabLayoutPosition();
        mViewPaper.setAdapter(new FragmentStatePagerAdapter(getFragmentManager()) {
            @Override
            public Fragment getItem(int position) {
                return modeManager.getFragment(position);
            }

            @Override
            public int getCount() {
                return modeManager.getModeCount();
            }
        });
        mViewPaper.addOnPageChangeListener(new ViewPager.OnPageChangeListener() {
            @Override
            public void onPageScrolled(int position, float positionOffset, int positionOffsetPixels) {

            }

            @Override
            public void onPageSelected(int position) {
                Log.e(TAG, "onPageSelected:" + position);
                //if(CameraData.getInstance().getIsPreviewDone()) {
                mTabLayout.setSelectedTab(position, true);
                mCurrentItemIndex = position;
                //}
            }

            @Override
            public void onPageScrollStateChanged(int state) {

            }
        });
        if (isVideoIntent()) {
            mCurrentItemIndex = Contants.VIDEOMODEINDEX;
        } else if (isCaptureIntent()) {
            mCurrentItemIndex = Contants.AUTOMODEINDEX;
        }

        Log.e(TAG, "mCurrentItemIndex:" + mCurrentItemIndex);
        mViewPaper.setCurrentItem(mCurrentItemIndex);
        mViewPaper.setListner(this);
        mViewPaper.setClickable(true);
        mViewPaper.setFocusable(true);
        mViewPaper.setLongClickable(true);

    }

    private boolean isCaptureIntent() {
        return MediaStore.ACTION_IMAGE_CAPTURE.equals(mIntentAction)
                || MediaStore.INTENT_ACTION_STILL_IMAGE_CAMERA.equals(mIntentAction);
    }

    private boolean isVideoIntent() {
        return MediaStore.INTENT_ACTION_VIDEO_CAMERA.equals(mIntentAction)
                || MediaStore.ACTION_VIDEO_CAPTURE.equals(mIntentAction);
    }

    private void initTabLayoutPosition() {
        int topMargin = CameraData.getInstance().getScreenHeight() - CameraData.getInstance().getScreenHeight_4_TO_3();
        Log.e(TAG, "topMargin:" + topMargin);
        if (mTabLayout.getLayoutParams() instanceof FrameLayout.LayoutParams) {
            FrameLayout.LayoutParams tabLayoutLayoutParams = (FrameLayout.LayoutParams)
                    mTabLayout.getLayoutParams();
            int textHeight = getContext().getResources().getDimensionPixelOffset(R.dimen.mode_text_fontsize)
                    + getContext().getResources().getDimensionPixelOffset(R.dimen.mode_text_padding_bottom);
            tabLayoutLayoutParams.bottomMargin = topMargin - textHeight;
            mTabLayout.setLayoutParams(tabLayoutLayoutParams);
        }
    }

    SlideTabHost.OnTabSelectedListener OnTabSelectedListener = new SlideTabHost.OnTabSelectedListener() {
        @Override
        public void onTabSelected(int position) {
            Log.e(TAG, "onTabSelected:" + position);
            if (!CameraData.getInstance().isImageCaptureIntent()&&!CameraData.getInstance().isVideoCaptureIntent() && CameraData.getInstance().getIsPreviewDone() && CameraData.getInstance().getCanChangeMode()) {
                mViewPaper.setCurrentItem(position);
                mCurrentItemIndex = position;
            }
        }
    };


    @Override
    public void onStart() {
        super.onStart();
    }

    @Override
    public void onResume() {
        super.onResume();
    }

    @Override
    public void onPause() {
        super.onPause();
    }

    @Override
    public void onStop() {
        super.onStop();
    }

    @Override
    public void onDestroy() {
        super.onDestroy();
    }

    @Override
    public void onDown(MotionEvent e) {

    }

    @Override
    public void onShowPress(MotionEvent e) {

    }

    @Override
    public void onSingleTapUp(MotionEvent e) {
        mUiManager.onSingleTapUp(e);
    }

    @Override
    public void onScroll(MotionEvent e1, MotionEvent e2, float distanceX, float distanceY) {
        mUiManager.onScroll(e1, e2, distanceX, distanceY);
    }

    @Override
    public void onLongPress(MotionEvent e) {
        mUiManager.onLongPress(e);
    }

    @Override
    public void onFling(MotionEvent e1, MotionEvent e2, float velocityX, float velocityY) {
        Log.e(TAG, "onFling");
        if (mUiManager.getIsFling() && !CameraData.getInstance().isImageCaptureIntent()&& !CameraData.getInstance().isVideoCaptureIntent() && CameraData.getInstance().getCanChangeMode() && CameraData.getInstance().getIsPreviewDone()) {
            if (e1.getX() - e2.getX() > 100
                    && Math.abs(velocityX) > 100 && CameraData.getInstance().getIsPreviewDone()) {
                // Fling left
                mUiManager.onFling(true);
                if (mCurrentItemIndex < modeManager.getModeCount()) {
                    mCurrentItemIndex = mCurrentItemIndex + 1;
                    mViewPaper.setCurrentItem(mCurrentItemIndex);
                    mTabLayout.setSelectedTab(mCurrentItemIndex, true);
                }
            } else if (e2.getX() - e1.getX() > 100
                    && Math.abs(velocityX) > 100 && CameraData.getInstance().getIsPreviewDone()) {
                // Fling right
                mUiManager.onFling(false);
                if (mCurrentItemIndex != 0) {
                    mCurrentItemIndex = mCurrentItemIndex - 1;
                    mViewPaper.setCurrentItem(mCurrentItemIndex);
                    mTabLayout.setSelectedTab(mCurrentItemIndex, true);
                }
            }
        }
    }

    @Override
    public void onScale(ScaleGestureDetector detector) {
        mUiManager.onScale(detector);
    }

    @Override
    public void onScaleBegin(ScaleGestureDetector detector) {
        mUiManager.onScaleBegin(detector);
    }

    @Override
    public void onScaleEnd(ScaleGestureDetector detector) {
        mUiManager.onScaleEnd(detector);
    }
}

