package com.bigbigcloud.devicehive.api;

import com.google.gson.JsonObject;

/**
 * Created by Thomas on 2016/5/24.
 */
public interface IUiCallback {
    /*
    * api访问成功时的回调函数
    * @param data 为rest api 中的data字段的json对象，如果为空，则data= null
     */
    void onSuccess(JsonObject data);

    /*
    * api访问失败的回调函数
    * @param errorCode 错误码
    * @param message 错误信息
     */
    void onError(int errorCode, String message);

    void onCancel();

}
