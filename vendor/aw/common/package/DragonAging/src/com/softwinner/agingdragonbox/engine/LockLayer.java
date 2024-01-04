package com.softwinner.agingdragonbox.engine;

import android.app.Activity;
import android.graphics.PixelFormat;
import android.view.Gravity;
import android.view.View;
import android.view.WindowManager;
import android.view.WindowManager.LayoutParams;

public class LockLayer {
    private Activity      mActivty;
    private WindowManager mWindowManager;
    private WindowManager mTopWindowManager;
    private View          mLockView;
    private LayoutParams  mLockViewLayoutParams;

    public LockLayer(Activity act) {
        mActivty = act;
        initialize();
    }

    private void initialize() {
        mWindowManager = mActivty.getWindowManager();
        mTopWindowManager = (WindowManager) mActivty.getApplicationContext().getSystemService(
                "window");
        mLockViewLayoutParams = new LayoutParams();
        mLockViewLayoutParams.width = LayoutParams.MATCH_PARENT;
        mLockViewLayoutParams.height = LayoutParams.MATCH_PARENT;
        mLockViewLayoutParams.flags = LayoutParams.FLAG_KEEP_SCREEN_ON
                | LayoutParams.FLAG_FULLSCREEN;
        // mLockViewLayoutParams.systemUiVisibility =
        // View.SYSTEM_UI_FLAG_HIDE_NAVIGATION;
        // mLockViewLayoutParams.type = LayoutParams.FLAG_HARDWARE_ACCELERATED |
        // LayoutParams.TYPE_PHONE;
    }

    public void lock() {
        if (mLockView != null) {
            mWindowManager.addView(mLockView, mLockViewLayoutParams);
        }
    }

    public void unlock() {
        if (mWindowManager != null) {
            try {
                mWindowManager.removeView(mLockView);
                // WindowManager.LayoutParams params =
                // mActivty.getWindow().getAttributes();
                // params.systemUiVisibility = View.SYSTEM_UI_FLAG_VISIBLE;
                // mActivty.getWindow().setAttributes(params);
            } catch (Exception e) {
                e.printStackTrace();
            }
        }
    }

    public View getView() {
        return mLockView;
    }

    public void setLockView(View v) {
        mLockView = v;
    }

    public void setLockView(int resId) {
        View v = View.inflate(mActivty, resId, null);
        setLockView(v);
    }

    public void lockView(int resId) {
        View v = View.inflate(mActivty, resId, null);
        lockView(v);
    }

    public void lockTopView(int resId) {
        View v = View.inflate(mActivty, resId, null);
        lockTopView(v);
    }

    public void lockTopView(View v) {
        setLockView(v);
        WindowManager.LayoutParams wParams = new WindowManager.LayoutParams();

        wParams.type = LayoutParams.TYPE_PHONE;
        wParams.format = PixelFormat.RGBA_8888;

        wParams.flags = LayoutParams.FLAG_NOT_TOUCH_MODAL | LayoutParams.FLAG_NOT_FOCUSABLE;

        wParams.gravity = Gravity.RIGHT | Gravity.BOTTOM;
        wParams.width = LayoutParams.WRAP_CONTENT;
        wParams.height = LayoutParams.WRAP_CONTENT;

        lockTop(v, wParams);
    }

    public void lockTop(View v, WindowManager.LayoutParams wp) {
        if (mTopWindowManager != null) {
            mTopWindowManager.addView(v, wp);
        }
    }

    public void unlockTop() {
        if (mTopWindowManager != null) {
            mTopWindowManager.removeView(mLockView);
        }
    }

    public void lockView(View v, boolean enableBack) {
        mLockViewLayoutParams.type = LayoutParams.TYPE_SYSTEM_ALERT;
        mLockViewLayoutParams.flags = LayoutParams.FLAG_NOT_FOCUSABLE;
        setLockView(v);
        lock();
    }

    public void lockView(View v) {
        setLockView(v);
        lock();
    }
}

