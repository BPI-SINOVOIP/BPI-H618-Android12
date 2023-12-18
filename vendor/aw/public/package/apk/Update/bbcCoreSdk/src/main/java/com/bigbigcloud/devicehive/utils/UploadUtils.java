package com.bigbigcloud.devicehive.utils;

import android.content.Context;
import android.util.Log;

import com.alibaba.sdk.android.oss.ClientException;
import com.alibaba.sdk.android.oss.OSS;
import com.alibaba.sdk.android.oss.OSSClient;
import com.alibaba.sdk.android.oss.ServiceException;
import com.alibaba.sdk.android.oss.callback.OSSCompletedCallback;
import com.alibaba.sdk.android.oss.callback.OSSProgressCallback;
import com.alibaba.sdk.android.oss.common.auth.OSSCredentialProvider;
import com.alibaba.sdk.android.oss.common.auth.OSSPlainTextAKSKCredentialProvider;
import com.alibaba.sdk.android.oss.model.PutObjectRequest;
import com.alibaba.sdk.android.oss.model.PutObjectResult;

import java.io.File;
import java.util.Calendar;

/**
 * Created by Thomas.yang on 2016/7/15.
 */
public class UploadUtils {
    private static final String ENDPOINT = "http://oss-cn-shenzhen.aliyuncs.com";
    private static final String ACCESS_KEY = "n1IDusKhycytheZp";
    private static final String ACCESS_SECRET = "WWH9f2iG1z0npX6BK2TUed8BCSyufr";
    private OSS oss;

    public static enum FileType{
        PICTURE("picture"),
        VOICE("voice"),
        VIDEO("video"),
        UNKNOW("unknow");
        private String value;
        private FileType(String value){
            this.value = value;
        }
        public String value(){
            return this.value;
        }
    }

    private static class SingletonHolder{
        public static UploadUtils Instance = new UploadUtils();
    }
    public static UploadUtils getInstance(){
        return SingletonHolder.Instance;
    }

    public void init(Context context){
        OSSCredentialProvider credentialProvider = new OSSPlainTextAKSKCredentialProvider(ACCESS_KEY, ACCESS_SECRET);
        oss = new OSSClient(context, ENDPOINT, credentialProvider);
    }

    public void uploadFile(String filePath,FileType fileType , final UploadCallback uploadCallback){
        if(!(new File(filePath).exists()) || uploadCallback == null){
            uploadCallback.onError("params invalid");
            return;
        }
        Calendar calendar = Calendar.getInstance();
        String object = "bbcAppclient/" + calendar.get(Calendar.YEAR) + "/" + calendar.get(Calendar.MONTH) + "/" + fileType.value() + calendar.getTimeInMillis();
        final String url = "http://sz-d-bbc.oss-cn-shenzhen.aliyuncs.com" + "/" + object;
        PutObjectRequest put = new PutObjectRequest("sz-d-bbc", object, filePath);
        // 异步上传时可以设置进度回调
        put.setProgressCallback(new OSSProgressCallback<PutObjectRequest>() {
            @Override
            public void onProgress(PutObjectRequest request, long currentSize, long totalSize) {
                Log.d("PutObject", "currentSize: " + currentSize + " totalSize: " + totalSize);
                int progress = (int) (100 * currentSize / totalSize);
                uploadCallback.onProgress(progress);
            }
        });
        oss.asyncPutObject(put, new OSSCompletedCallback<PutObjectRequest, PutObjectResult>() {
            @Override
            public void onSuccess(PutObjectRequest putObjectRequest, PutObjectResult putObjectResult) {
                uploadCallback.onSuccess(url);
            }

            @Override
            public void onFailure(PutObjectRequest putObjectRequest, ClientException e, ServiceException e1) {
                uploadCallback.onError(e1.getMessage());
            }
        });
    }

    public interface UploadCallback{
        public void onSuccess(String resultUrl);
        public void onProgress(int progress);
        public void onError(String message);
    }


}
