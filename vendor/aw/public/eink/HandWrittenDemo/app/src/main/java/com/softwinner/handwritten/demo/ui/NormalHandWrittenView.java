package com.softwinner.handwritten.demo.ui;

import android.content.Context;
import android.graphics.Canvas;
import android.graphics.Color;
import android.graphics.Paint;
import android.graphics.Path;
import android.graphics.PixelFormat;
import android.graphics.PorterDuff;
import android.graphics.PorterDuffXfermode;
import android.os.Trace;
import android.util.AttributeSet;
import android.util.DisplayMetrics;
import android.util.Log;
import android.view.MotionEvent;
import android.view.Surface;
import android.view.SurfaceHolder;
import android.view.SurfaceControl;
import android.view.SurfaceView;
import android.view.WindowManager;

import java.util.ArrayList;
import java.util.List;
import java.util.concurrent.locks.Condition;
import java.util.concurrent.locks.ReentrantLock;

public class NormalHandWrittenView extends SurfaceView implements SurfaceHolder.Callback, Runnable {

    private final static String TAG = "hand-view";

    private final static int MAX_POINT_CACHE = 100;
    private final static float DEFAULT_STROKE_WIDTH = 5f;

    private List<MotionEvent> mPointList;
    private List<MotionEvent> mPointTmpList;
    private ReentrantLock mPointListLock;
    private Condition mPointListFull;
    private Condition mPointListEmpty;

    private Surface mSurface;
    private Canvas mCanvas;
    private Paint mDrawPaint;
    private Paint mRubberPaint;
    private Path mDrawPath;
    private Path mRubberPath;

    private boolean mIsRubberMode;

    private boolean mIsHandWriteStop;
    private Thread mRenderThread;

    public NormalHandWrittenView(Context context) {
        this(context, null);
    }

    public NormalHandWrittenView(Context context, AttributeSet attrs) {
        this(context, attrs, 0);
    }

    public NormalHandWrittenView(Context context, AttributeSet attrs, int defStyle) {
        super(context, attrs, defStyle);
        init();
    }

    private void init() {
        mPointList = new ArrayList<MotionEvent>();
        mPointTmpList = new ArrayList<MotionEvent>();
        mPointListLock = new ReentrantLock();
        mPointListFull = mPointListLock.newCondition();
        mPointListEmpty = mPointListLock.newCondition();

        mIsHandWriteStop = true;
        mIsRubberMode = false;

        mDrawPaint = new Paint();
        mDrawPaint.setStrokeWidth(DEFAULT_STROKE_WIDTH);
        mDrawPaint.setColor(Color.BLACK);
        mDrawPaint.setStyle(Paint.Style.STROKE);
        mRubberPaint = new Paint();
        mRubberPaint.setStrokeWidth(DEFAULT_STROKE_WIDTH);
        mRubberPaint.setColor(Color.RED);
        mRubberPaint.setStyle(Paint.Style.STROKE);
        mRubberPaint.setXfermode(new PorterDuffXfermode(PorterDuff.Mode.DST_OUT));

        mDrawPath = new Path();
        mRubberPath = new Path();

        mIsHandWriteStop = false;

        mRenderThread = new Thread(this);
        mRenderThread.setPriority(Thread.MAX_PRIORITY);

        getHolder().addCallback(this);
        getHolder().setFormat(PixelFormat.RGBA_8888);
    }

    public void release() {
        stopHandWrite();

        getHolder().removeCallback(this);
    }

    public void clearPanel() {
        mPointListLock.lock();
        try {
            mPointList.clear();
            mDrawPath.reset();
            mRubberPath.reset();
            mPointListEmpty.signal();
        } finally {
            mPointListLock.unlock();
        }
    }

    public void setRubberMode(boolean isRubber) {
        mIsRubberMode = isRubber;
    }

    public boolean isRubberMode() {
        return mIsRubberMode;
    }

    private void startHandWrit() {
        /*SurfaceControl sc = getSurfaceControl();
        sc.openTransaction();
        try {
            // due SurfaceView has it's owner surface, need setRefreshMode alone
            sc.setRefreshMode(
                WindowManager.LayoutParams.EINK_DU_MODE);
        } catch (Error e) {
			Log.d(TAG, "error:", e);
		} finally {
            sc.closeTransaction();
        }*/

        //int[] point = new int[2];
        //getLocationOnScreen(point);

        WindowManager wm = (WindowManager) getContext()
            .getSystemService(Context.WINDOW_SERVICE);
        DisplayMetrics outMetrics = new DisplayMetrics();
        wm.getDefaultDisplay().getMetrics(outMetrics);
        int w = outMetrics.widthPixels;
        int h = outMetrics.heightPixels;

        mSurface = getHolder().getSurface();

        mDrawPath.reset();
        mRubberPath.reset();

        //mDrawPath.moveTo(100, 100);
        //mDrawPath.lineTo(500, 100);

        //mRubberPath.moveTo(200, 100);
        //mRubberPath.lineTo(300, 100);

        mIsHandWriteStop = false;
        mRenderThread.start();
    }

    private void stopHandWrite() {
        mIsHandWriteStop = true;
        try {
            mPointListEmpty.signal();
            mPointListFull.signal();
        } catch (Exception e) {
            Log.w(TAG, "error:", e);
        }
        try {
            //mRenderThread.stop();
            mRenderThread.join(500);
        } catch (InterruptedException e) {
            Log.e(TAG, "error:", e);
        }

        mPointList.clear();
        mPointTmpList.clear();

        mDrawPath.reset();
        mRubberPath.reset();
    }

    @Override
    public boolean onTouchEvent(MotionEvent event) {
        return postInputEvent(event);
    }

    @Override
    public void surfaceCreated(SurfaceHolder holder) {
        Log.d(TAG, "surfaceCreated ... ");
        startHandWrit();
    }

    @Override
    public void surfaceChanged(SurfaceHolder holder, int format, int width, int height) {
        Log.d(TAG, "surfaceChanged: size: " + width + "x" + height + ", format: " + format);
    }

    @Override
    public void surfaceDestroyed(SurfaceHolder holder) {
        Log.d(TAG, "surfaceDestroyed ... ");
    }

    @Override
    public void run() {
        threadLoopInRenderBk();
    }

    @Override
    protected void onAttachedToWindow() {
        super.onAttachedToWindow();
        Log.d(TAG, "hmm onAttachedToWindow");
    }

    @Override
    protected void onWindowVisibilityChanged(int visibility) {
        super.onWindowVisibilityChanged(visibility);
        Log.d(TAG, "hmm onWindowVisibilityChanged: " + visibility);
    }

    private boolean postInputEvent(MotionEvent event) {
        if (mIsHandWriteStop) {
            return false;
        }

        Trace.beginSection("HandWritten#postInputEvent");

        mPointListLock.lock();
        try {
            if (mPointList.size() > MAX_POINT_CACHE) {
                mPointListFull.await();
            }

            mPointList.add(MotionEvent.obtain(event));
            mPointListEmpty.signal();

        } catch (InterruptedException e) {
            Log.e(TAG, "error:", e);
        } finally {
            mPointListLock.unlock();
        }

        Trace.endSection();

        return true;
    }

    private void threadLoopInRenderBk() {
        clearInRenderBk();
        do {
            getDrawPointsInRenderBk();
            drawPointsInRenderBk();
        } while (!mIsHandWriteStop);
    }

    private void getDrawPointsInRenderBk() {
        mPointListLock.lock();
        try {
            if (mPointList.isEmpty()) {
                mPointListEmpty.await();
            }

            mPointTmpList.clear();
            mPointTmpList.addAll(mPointList);
            mPointList.clear();

        } catch (InterruptedException e) {
            Log.e(TAG, "error:", e);
        } finally {
            mPointListLock.unlock();
        }

        Path path = mIsRubberMode ? mRubberPath : mDrawPath;

        for (MotionEvent ev : mPointTmpList) {
            switch (ev.getAction()) {
                case MotionEvent.ACTION_DOWN:
                    //mPath.reset();
                    path.moveTo(ev.getX(), ev.getY());
                    break;

                case MotionEvent.ACTION_UP:
                case MotionEvent.ACTION_MOVE:
                    path.lineTo(ev.getX(), ev.getY());
                    break;

                default:
                    break;
            }
        }

        mPointTmpList.clear();
    }

    private void drawPointsInRenderBk() {
        Trace.beginSection("HandWritten#draw");

        mCanvas = mSurface.lockCanvas(null);

        mCanvas.drawColor(Color.WHITE);

        //mDrawPath.reset();
        //mDrawPath.moveTo(100, 100);
        //mDrawPath.lineTo(500, 100);

        mCanvas.drawPath(mDrawPath, mDrawPaint);
        mCanvas.drawPath(mRubberPath, mRubberPaint);

        /*mDrawPaint.setColor(0xffff0000);
        mDrawPaint.setStyle(Paint.Style.FILL);
        mDrawPaint.setXfermode(null);
        mCanvas.drawRect(100, 100, 200, 200, mDrawPaint);

        mDrawPaint.setColor(0xff00ff00);
        mDrawPaint.setStyle(Paint.Style.FILL);
        mDrawPaint.setXfermode(new PorterDuffXfermode(PorterDuff.Mode.CLEAR));
        mCanvas.drawRect(150, 150, 250, 250, mDrawPaint);*/

        mSurface.unlockCanvasAndPost(mCanvas);

        Trace.endSection();
    }

    private void clearInRenderBk() {
        mCanvas = mSurface.lockCanvas(null);
        mCanvas.drawColor(Color.WHITE);
        mSurface.unlockCanvasAndPost(mCanvas);
    }

}
