package com.softwinner.handwritten.demo.ui;

import android.content.Context;
import android.graphics.Bitmap;
import android.graphics.Canvas;
import android.graphics.PixelFormat;
import android.graphics.Point;
import android.graphics.PorterDuff;
import android.graphics.Rect;
import android.graphics.Region;
import android.view.WindowManager;
import android.view.Surface;
import android.view.SurfaceHolder;
import android.view.SurfaceView;
import android.util.DisplayMetrics;
import android.util.AttributeSet;
import android.util.Log;
import android.util.Slog;

import java.util.ArrayList;
import java.util.concurrent.locks.ReentrantLock;

import android.os.RemoteException;
import android.os.ServiceManager;
import com.softwinner.EinkManager;

public class HandWrittenView extends SurfaceView implements SurfaceHolder.Callback {

    private static final String TAG = "HandWrittenView";
	private static final boolean DEBUG = false;

    private int mWidth;
    private int mHeight;
    private boolean mIsHandWrittenStart;
    private Bitmap mBg;
    private boolean mHasWindowFocus = false;


    private Surface mSurface;

    public HandWrittenView(Context context) {
        this(context, null);
    }

    public HandWrittenView(Context context, AttributeSet attrs) {
        this(context, attrs, 0);
    }

    public HandWrittenView(Context context, AttributeSet attrs, int defStyleAttr) {
        this(context, attrs, defStyleAttr, 0);
    }

    public HandWrittenView(Context context, AttributeSet attrs, int defStyleAttr, int defStyleRes) {
        super(context, attrs, defStyleAttr, defStyleRes);
        init();
    }

    private void init() {
        mWidth = 0;
        mHeight = 0;
        mIsHandWrittenStart = false;


        getHolder().addCallback(this);
    }

    @Override
    protected void onWindowVisibilityChanged(int visibility) {
        super.onWindowVisibilityChanged(visibility);
        Slog.d(TAG, "onWindowVisibilityChanged: visibility=" + visibility);
    }

    @Override
    protected void onAttachedToWindow() {
        super.onAttachedToWindow();
        Slog.d(TAG, "onAttachedToWindow");
    }

    @Override
    public void onWindowFocusChanged(boolean hasWindowFocus) {
        super.onWindowFocusChanged(hasWindowFocus);
        mHasWindowFocus = hasWindowFocus;
        Log.d(TAG, "onWindowFocusChanged: " + hasWindowFocus);
        if(mSurface == null){
            Log.e(TAG, "onWindowFocusChanged: Surface is not available");
            return;
        }
        EinkManager.setWindowFocus(hasWindowFocus);
    }

    @Override
    public void surfaceCreated(SurfaceHolder holder) {
        Slog.d(TAG, "surfaceCreated: holder=" + holder.getSurface());

        mWidth = 0;
        mHeight = 0;
        mSurface = holder.getSurface();

    }

    @Override
    public void surfaceChanged(SurfaceHolder holder, int format, int width, int height) {
        Slog.d(TAG, "surfaceChanged: w=" + width + ", h=" + height + ", format=" + format);
        if (mSurface == holder.getSurface()) {
            boolean needCreateNew = ((width > 0) && (height > 0)
                    && ((mWidth != width) || (mHeight != height)));
            mWidth = width;
            mHeight = height;
            if (needCreateNew) {
                Slog.d(TAG, "surfaceChanged: size changed reset hand write");
                mSurface = holder.getSurface();
                stopHandWritten();
                startHandWritten();
            }
        }
    }

    @Override
    public void surfaceDestroyed(SurfaceHolder holder) {
        Slog.d(TAG, "surfaceDestroyed: mSurface=" + mSurface + ", holder=" + holder.getSurface());
        if (mSurface == holder.getSurface()) {
            stopHandWritten();
            mSurface = null;
        }
    }

    private void native_onInputEvent(int type, int x, int y, int pressure) {
        Slog.d(TAG, "onInputEvent: type=" + type + ", x=" + x + ", y=" + y + ", pressure=" + pressure);
    }

    // ----------------------------------------------------------------------------

    private void startHandWritten() {
        if (mIsHandWrittenStart) {
            Slog.i(TAG, "startHandWritten: hand write already start, ignore it");
            return;
        }
        EinkManager.init(mSurface, getLocationOnScreen());
        EinkManager.start();
        EinkManager.setWindowFocus(mHasWindowFocus);
        Slog.d(TAG, "startHandWritten: start");

        mIsHandWrittenStart = true;
        updateBg();
    }

    private void stopHandWritten() {
        if (!mIsHandWrittenStart) {
            Slog.i(TAG, "stopHandWritten: hand write not start, ignore it");
            return;
        }
        EinkManager.stop();

        Slog.d(TAG, "stopHandWritten: stop");

        mIsHandWrittenStart = false;
    }

    public void setBg(Bitmap background) {
        mBg = background;
        updateBg();
    }

    private void updateBg() {
        if (mIsHandWrittenStart) {
            EinkManager.setBackground(mBg);
        }
    }
}
