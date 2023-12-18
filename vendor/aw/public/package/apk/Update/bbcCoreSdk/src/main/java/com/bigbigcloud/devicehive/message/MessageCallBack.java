package com.bigbigcloud.devicehive.message;

/**
 * Created by Administrator on 2015/12/31.
 */
public interface MessageCallBack {
    public void onSuccess();
    public void onProgress(final int progress);
    public void onError();
}
