package com.allwinner.camera.utils;

import android.content.Intent;
import android.support.annotation.NonNull;
import android.util.Log;

import java.util.concurrent.BlockingQueue;
import java.util.concurrent.Executor;
import java.util.concurrent.LinkedBlockingDeque;
import java.util.concurrent.ThreadFactory;
import java.util.concurrent.ThreadPoolExecutor;
import java.util.concurrent.TimeUnit;
import java.util.concurrent.atomic.AtomicInteger;

public class TaskExecutor {
    private static BlockingQueue<Runnable> workQueue = new LinkedBlockingDeque<>(128);
    private static ThreadFactory threadFactory = new ThreadFactory() {
        private AtomicInteger mCount = new AtomicInteger(0);
        @Override
        public Thread newThread(@NonNull Runnable r) {
            return new Thread(r,"ThreadTask: " + mCount.incrementAndGet());
        }
    };

    public static Executor threadPool = new ThreadPoolExecutor(Runtime.getRuntime().availableProcessors(),8,0,TimeUnit.MILLISECONDS,workQueue,threadFactory);


    public static void executeTask(Runnable r,String msg) {
        Log.i("TaskExecutor","executeTask: " + msg);
        if (r != null) {
            threadPool.execute(r);
        }
    }
}