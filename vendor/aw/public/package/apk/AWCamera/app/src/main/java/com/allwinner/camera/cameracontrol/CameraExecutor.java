package com.allwinner.camera.cameracontrol;

import android.util.Log;

import com.allwinner.camera.utils.TaskExecutor;

import java.util.concurrent.ConcurrentLinkedQueue;

public class CameraExecutor {
    private ConcurrentLinkedQueue<Runnable> mTasks = new ConcurrentLinkedQueue();
    private final String TAG = "CameraExecutor";
    private Runnable mCurrentTask = null;
    private Object mSync = new Object();
    private boolean mIsRunning = false;

    public CameraExecutor() {

    }

    public synchronized void execute(final Runnable r, final String msg) {
        Log.i(TAG, "start offer: " + msg);
        mTasks.offer(new Runnable() {
            @Override
            public void run() {
                Log.i(TAG, "start exec Task " + msg);
                r.run();
                Log.i(TAG, "end exec Task " + msg);
                mIsRunning = false;
                synchronized (mSync) {
                    if (mIsRunning) {
                        return;
                    }
                    mCurrentTask = mTasks.poll();
                    if (mCurrentTask == null ) {
                        return;
                    }
                    Log.i(TAG, "mCurrentTask: " + mCurrentTask);
                    TaskExecutor.executeTask(mCurrentTask,msg);
                }
            }
        });
        Log.i(TAG, "end offer: " + msg);
        synchronized (mSync) {
            if (mCurrentTask == null) {
                Log.i(TAG, "mTasks  size: " + mTasks.size());
                mCurrentTask = mTasks.poll();
                Log.i(TAG, "mCurrentTask is null: " + (mCurrentTask == null));
                if (mCurrentTask != null) {
                    mIsRunning = true;
                    TaskExecutor.executeTask(mCurrentTask,msg);
                }
            }
        }

    }

    public void waitDone(final String msg) {
        final Object waitLock = new Object();
        Runnable runnable = new Runnable() {
            @Override
            public void run() {
                synchronized (waitLock) {
                    waitLock.notifyAll();
                    Log.i(TAG, "waitDone notifyAll");
                }
            }
        };
        execute(runnable, msg + " waitDone");
        try {
            synchronized (waitLock) {
                Log.i(TAG, "waitDone wait ");
                waitLock.wait();
            }
        } catch (InterruptedException e) {
            e.printStackTrace();
        }
    }
}
