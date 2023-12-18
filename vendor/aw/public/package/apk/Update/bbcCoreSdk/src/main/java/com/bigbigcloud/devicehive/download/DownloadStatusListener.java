package com.bigbigcloud.devicehive.download;

import org.xutils.common.Callback;

import java.io.File;

/**
 * Created by Administrator on 2016/1/27.
 */
public interface DownloadStatusListener {
    public void onWaiting();

    public void onStarted();

    public void onLoading(long total, long current);

    public void onSuccess(File result);

    public void onError(Throwable ex, boolean isOnCallback);

    public void onCancelled(Callback.CancelledException cex);
}
