package com.android.music;

import android.app.Application;
import android.content.Context;
import android.content.BroadcastReceiver;
import android.content.Intent;
import android.content.IntentFilter;
import android.os.Message;
import android.os.Handler;

import java.util.List;
import java.util.ArrayList;

import com.android.music.utils.LogHelper;

public class MusicApp extends Application {

    private static final String TAG = LogHelper.makeLogTag(MusicApp.class);

    private List<MountCallback> mMountCallbackList;
    private MediaMountReceiver mMountReceiver = new MediaMountReceiver();

    // handle removeable storage mount unmount events
    public interface MountCallback {
        void onAppMountChanged(boolean isMounted);
    }

    @Override
    public void onCreate() {
        super.onCreate();
        init();
    }

    private void init() {
        mMountCallbackList = new ArrayList<MountCallback> ();

        IntentFilter filter = new IntentFilter();
        filter.addDataScheme("file");
        filter.addAction(Intent.ACTION_MEDIA_UNMOUNTED);
        filter.addAction(Intent.ACTION_MEDIA_MOUNTED);
        registerReceiver(mMountReceiver, filter);
    }

    public void addMountCallback(MountCallback callback) {
        synchronized(MusicApp.this) {
            if (!mMountCallbackList.contains(callback)) {
                LogHelper.d(TAG, "add mount callback: " + callback.toString());
                mMountCallbackList.add(callback);
            }
        }
    }

    public void removeMountCallback(MountCallback callback) {
        synchronized(MusicApp.this) {
            if (mMountCallbackList.contains(callback)) {
                LogHelper.d(TAG, "remove mount callback: " + callback.toString());
                mMountCallbackList.remove(callback);
            }
        }
    }

    private class MediaMountReceiver extends BroadcastReceiver {
        @Override
        public void onReceive(Context context, Intent intent) {
            final String action = intent.getAction();
            LogHelper.d(TAG, "received action: " + action);
            boolean isMounted = true;
            if (Intent.ACTION_MEDIA_UNMOUNTED.equals(action)) {
                isMounted = false;
            }
            synchronized(MusicApp.this) {
                for (MountCallback callback : mMountCallbackList) {
                    callback.onAppMountChanged(isMounted);
                }
            }
        }
    }

}
