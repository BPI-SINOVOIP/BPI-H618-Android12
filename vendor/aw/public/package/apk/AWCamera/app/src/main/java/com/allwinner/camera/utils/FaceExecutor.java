package com.allwinner.camera.utils;

import android.util.Log;

import java.util.ArrayDeque;

public class FaceExecutor {
    private ArrayDeque<Runnable> mTasks = new ArrayDeque();
    private final String TAG = "FaceExecutor";
    private Runnable mCurrentTask = null;

    public FaceExecutor(){

    }

    public  synchronized void execute(final Runnable r, final String msg) {
        mTasks.offer(new Runnable() {
            @Override
            public void run() {
                Log.i(TAG,"start exec Task " + msg);
                r.run();
                Log.i(TAG,"end exec Task " + msg);
                mCurrentTask = mTasks.poll();
                TaskExecutor.executeTask(mCurrentTask,msg);
            }
        });
        if (mCurrentTask == null) {
            mCurrentTask = mTasks.poll();
            TaskExecutor.executeTask(mCurrentTask,msg);
        }

    }

    public void waitDone(final String msg){
        final Object waitLock = new Object();
        Runnable runnable = new Runnable() {
            @Override
            public void run() {
                synchronized(waitLock){
                    waitLock.notifyAll();
                    Log.i(TAG,"waitDone notifyAll");
                }
            }
        };
        execute(runnable,"waitDone");
        try {
            synchronized(waitLock){
                Log.i(TAG,"waitDone wait ");
                waitLock.wait();
            }
        } catch (InterruptedException e) {
            e.printStackTrace();
        }
    }
}
