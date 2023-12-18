package com.bigbigcloud.devicehive.service;

import org.xutils.common.Callback;
import org.xutils.ex.HttpException;

/**
 * Created by Administrator on 2016/3/23.
 */
public abstract class RestCallback<ResultType> implements Callback.CommonCallback<ResultType> {
    public RestCallback() {
        super();
    }

    @Override
    public abstract void onSuccess(ResultType result);

    public abstract void onError(int responseCode,String message);

    @Override
    public void onError(Throwable ex, boolean isOnCallback) {
        if (ex instanceof HttpException) { // 网络错误
            HttpException httpEx = (HttpException) ex;
            int responseCode = httpEx.getCode();
            String responseMsg = httpEx.getMessage();
            onError(responseCode, responseMsg);
        }
    }

    @Override
    public void onCancelled(CancelledException cex) {

    }

    @Override
    public void onFinished() {

    }
}
