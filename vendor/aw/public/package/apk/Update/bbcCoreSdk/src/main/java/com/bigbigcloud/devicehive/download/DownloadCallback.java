package com.bigbigcloud.devicehive.download;

import org.xutils.common.Callback;
import org.xutils.common.util.LogUtil;
import org.xutils.ex.DbException;

import java.io.File;

/**
 * Created by wyouflf on 15/11/10.
 */
/*package*/ class DownloadCallback implements
        Callback.CommonCallback<File>,
        Callback.ProgressCallback<File>,
        Callback.Cancelable {

    private DownloadInfo downloadInfo;
    private DownloadStatusListener downloadStatusListener;
    private DownloadManager downloadManager;
    private boolean cancelled = false;
    private Cancelable cancelable;

    public DownloadCallback(DownloadStatusListener downloadStatusListener, DownloadInfo downloadInfo) {
        this.downloadStatusListener = downloadStatusListener;
        this.downloadInfo = downloadInfo;
    }


    public void setDownloadManager(DownloadManager downloadManager) {
        this.downloadManager = downloadManager;
    }

    public void setCancelable(Cancelable cancelable) {
        this.cancelable = cancelable;
    }

    public  boolean switchListner(DownloadStatusListener listener){
        if(listener == null) return false;
        synchronized (DownloadCallback.class) {
            if (downloadInfo != null) {
                if (this.isStopped()) {
                    return false;
                }
            }
            this.downloadStatusListener = listener;
        }
        return true;
    }

//    private DownloadViewHolder getViewHolder() {
//        if (viewHolderRef == null) return null;
//        DownloadViewHolder viewHolder = viewHolderRef.get();
//        if (viewHolder != null) {
//            DownloadInfo downloadInfo = viewHolder.getDownloadInfo();
//            if (this.downloadInfo != null && this.downloadInfo.equals(downloadInfo)) {
//                return viewHolder;
//            }
//        }
//        return null;
//    }

    @Override
    public void onWaiting() {
        try {
            downloadInfo.setState(DownloadState.WAITING);
            downloadManager.updateDownloadInfo(downloadInfo);
        } catch (DbException ex) {
            LogUtil.e(ex.getMessage(), ex);
        }
        if (downloadStatusListener != null) {
            downloadStatusListener.onWaiting();
        }
    }

    @Override
    public void onStarted() {
        try {
            downloadInfo.setState(DownloadState.STARTED);
            downloadManager.updateDownloadInfo(downloadInfo);
        } catch (DbException ex) {
            LogUtil.e(ex.getMessage(), ex);
        }
        if (downloadStatusListener != null) {
            downloadStatusListener.onStarted();
        }
    }

    public void autoPauseResume() {
        if (downloadInfo.getState() == DownloadState.WAITING) {
            onStarted();
        } else if (downloadInfo.getState() == DownloadState.STARTED) {
            onWaiting();
        }
    }
    @Override
    public void onLoading(long total, long current, boolean isDownloading) {
        if (isDownloading && downloadInfo.getState() == DownloadState.STARTED) {
            try {
                //downloadInfo.setState(DownloadState.STARTED);
                downloadInfo.setFileLength(total);
                downloadInfo.setProgress((int) (current * 100 / total));
                downloadManager.updateDownloadInfo(downloadInfo);
            } catch (DbException ex) {
                LogUtil.e(ex.getMessage(), ex);
            }
            if (downloadStatusListener != null) {
                downloadStatusListener.onLoading(total, current);
            }
        }
    }

    @Override
    public void onSuccess(File result) {
        synchronized (DownloadCallback.class) {
            try {
                downloadInfo.setState(DownloadState.FINISHED);
                downloadManager.updateDownloadInfo(downloadInfo);
            } catch (DbException ex) {
                LogUtil.e(ex.getMessage(), ex);
            }
            if (downloadStatusListener != null) {
                downloadStatusListener.onSuccess(result);
            }
        }
    }

    @Override
    public void onError(Throwable ex, boolean isOnCallback) {
        synchronized (DownloadCallback.class) {
            try {
                downloadInfo.setState(DownloadState.ERROR);
                downloadManager.updateDownloadInfo(downloadInfo);
            } catch (DbException e) {
                LogUtil.e(e.getMessage(), e);
            }
            if (downloadStatusListener != null) {
                downloadStatusListener.onError(ex, isOnCallback);
            }
        }
    }

    @Override
    public void onCancelled(CancelledException cex) {
        synchronized (DownloadCallback.class) {
            try {
                downloadInfo.setState(DownloadState.STOPPED);
                downloadManager.updateDownloadInfo(downloadInfo);
            } catch (DbException ex) {
                LogUtil.e(ex.getMessage(), ex);
            }
            if (downloadStatusListener != null) {
                downloadStatusListener.onCancelled(cex);
            }
        }
    }

    @Override
    public void onFinished() {
        cancelled = false;
    }

    private boolean isStopped() {
        DownloadState state = downloadInfo.getState();
        return isCancelled() || state.value() > DownloadState.STARTED.value();
    }

    @Override
    public void cancel() {
        cancelled = true;
        if (cancelable != null) {
            cancelable.cancel();
        }
    }

    @Override
    public boolean isCancelled() {
        return cancelled;
    }
}
