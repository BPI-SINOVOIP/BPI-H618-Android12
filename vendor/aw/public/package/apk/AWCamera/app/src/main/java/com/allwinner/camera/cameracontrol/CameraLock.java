package com.allwinner.camera.cameracontrol;

import android.util.Log;

import java.util.concurrent.Semaphore;
import java.util.concurrent.TimeUnit;

public class CameraLock {

    private Semaphore mSemaphore = new Semaphore(1,true);
    private static  final  String  TAG= "CameraLock";
    private static long TIMEOUT = 2500;
    public CameraLock(){

    }

    public void lock(String msg) {
        Log.i(TAG, msg + " try lock");
        try {
            if (!mSemaphore.tryAcquire(TIMEOUT, TimeUnit.MILLISECONDS)) {
                throw new RuntimeException(msg + "timeout");
            }
            Log.i(TAG, msg + " lock"+"availablePermits():"+mSemaphore.availablePermits());
        } catch (InterruptedException e) {
            Log.i(TAG, msg + " InterruptedException unlock");
            mSemaphore.release();
            e.printStackTrace();
        }
    }

    public void unlock(String msg){

        mSemaphore.release();
        Log.i(TAG, msg + " unlock"+ mSemaphore.availablePermits());
    }
}
