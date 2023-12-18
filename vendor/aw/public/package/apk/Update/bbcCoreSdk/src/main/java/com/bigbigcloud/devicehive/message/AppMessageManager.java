package com.bigbigcloud.devicehive.message;

import android.content.Context;
import android.util.Log;

import com.bigbigcloud.devicehive.api.BbcDevice;
import com.bigbigcloud.devicehive.api.IUiCallback;
import com.bigbigcloud.devicehive.download.DownloadManager;
import com.bigbigcloud.devicehive.download.DownloadStatusListener;
import com.bigbigcloud.devicehive.entity.DeviceNotification;
import com.bigbigcloud.devicehive.service.Property;
import com.google.gson.Gson;
import com.google.gson.JsonArray;
import com.google.gson.JsonObject;

import org.xutils.common.Callback;
import org.xutils.common.util.MD5;

import java.io.File;
import java.util.ArrayList;
import java.util.Collections;
import java.util.List;

/**
 * Created by Thomas.yang on 2016/6/18.
 */
public class AppMessageManager implements MessageDispather.CommandMessageListener {

    private final String TAG = "WechatMessageManager";

    private List<BaseMessage> messageList;
    private List<BaseMessage> imageList;
    private List<MessageReciever> recieverList;
    private volatile static AppMessageManager instance;

    private BbcDevice bbcDevice;
    private Property property;

    private String cachedir;
    private List<String> appMessageTypes = new ArrayList<String>(){
        {
            add(MessageType.APP_TEXT);
            add(MessageType.APP_IMAGE);
            add(MessageType.APP_VOICE);
            add(MessageType.APP_VIDEO);
        }
    };

    public static AppMessageManager getInstance(){
        if(instance == null){
            synchronized (AppMessageManager.class){
                if(instance == null){
                    instance = new AppMessageManager();
                }
            }
        }
        return instance;
    }

    protected AppMessageManager(){
        if(messageList == null){
            messageList = Collections.synchronizedList(new ArrayList<BaseMessage>());
        }
        if (imageList == null){
            imageList = Collections.synchronizedList(new ArrayList<BaseMessage>());
        }
        if(recieverList == null){
            recieverList = Collections.synchronizedList(new ArrayList<MessageReciever>());
        }
        MessageDispather.getInstance().registerMessageListener(this, appMessageTypes);
    }

    public void init(Context context, BbcDevice bbcDevice){
        cachedir = context.getCacheDir().getAbsolutePath();
        this.bbcDevice = bbcDevice;
        property = new Property(context);
    }

    public void registerMessageReceiver(MessageReciever reciever){
        recieverList.add(reciever);
    }

    public  void unRegisterMessageReceiver(MessageReciever reciever){
        recieverList.remove(reciever);
    }

    public List<BaseMessage> getMessageList(){
        return messageList;
    }

    public List<BaseMessage> getImageList(){
        return  imageList;
    }

    public void downLoadMessage(BaseMessage message){
        String downLoadUrl = null;
        String filePath = null;
        final BaseMessage baseMessage = message;
        switch (message.getType()){
            case IMAGE:
                downLoadUrl = ((ImageMessage) baseMessage).getPicurl();
                filePath = cachedir + "/image/"+ MD5.md5(downLoadUrl);
                break;
            case VOICE:
                downLoadUrl = ((VoiceMessage) baseMessage).getVoiceurl();
                filePath = cachedir + "/voice/"+ MD5.md5(downLoadUrl);
                break;
            case VIDEO:
                downLoadUrl = ((VideoMessage) baseMessage).getVideourl();
                filePath = cachedir + "/video/"+ MD5.md5(downLoadUrl);
                break;
        }
        if (downLoadUrl == null){
            return;
        }
        try {
            DownloadManager.getInstance().startDownload(downLoadUrl,
                    filePath, true, false, new DownloadStatusListener() {
                        @Override
                        public void onWaiting() {

                        }

                        @Override
                        public void onStarted() {
                            baseMessage.setStatus(BaseMessage.Status.INPROGRESS);
                        }

                        @Override
                        public void onLoading(long total, long current) {
                            int progress = (int)(100 * current / total);
                            if(baseMessage.getMessageCallBack() != null) {
                                baseMessage.getMessageCallBack().onProgress(progress);
                            }
                        }

                        @Override
                        public void onSuccess(File result) {
                            Log.d(TAG, " download success " + result);
                            baseMessage.setStatus(BaseMessage.Status.SUCCESS);
                            baseMessage.setFilePath(result.getAbsolutePath());
                            if(baseMessage.getMessageCallBack() != null) {
                                baseMessage.getMessageCallBack().onSuccess();
                            }
                        }

                        @Override
                        public void onError(Throwable ex, boolean isOnCallback) {
                            baseMessage.setStatus(BaseMessage.Status.FAIL);
                            if(baseMessage.getMessageCallBack() != null) {
                                baseMessage.getMessageCallBack().onError();
                            }
                        }

                        @Override
                        public void onCancelled(Callback.CancelledException cex) {

                        }
                    });
        }catch (Exception e){

        }
    }

    @Override
    public void onMessage(BbcMessage message) {
        Log.d("yang", " app message on message " + message.getParameters());
        Gson gson = new Gson();
        String messageType = message.getMessageType();
        BaseMessage baseMessage = null;
        if(messageType.equals(MessageType.APP_TEXT)){
            baseMessage = gson.fromJson(message.getParameters(), TextMessage.class);
        }else if (messageType.equals(MessageType.APP_IMAGE)){
            baseMessage = gson.fromJson(message.getParameters(), ImageMessage.class);
            imageList.add(baseMessage);
            downLoadMessage(baseMessage);
        }else if (messageType.equals(MessageType.APP_VOICE)){
            baseMessage = gson.fromJson(message.getParameters(), VoiceMessage.class);
            downLoadMessage(baseMessage);
        }else if (messageType.equals(MessageType.APP_VIDEO)){
            baseMessage = gson.fromJson(message.getParameters(), VideoMessage.class);
            Log.d(TAG, " video url " + ((VideoMessage) baseMessage).getVideourl());
            downLoadMessage(baseMessage);
        }
        if(baseMessage != null){
            messageList.add(baseMessage);
            for(MessageReciever reciever : recieverList){
                reciever.onReceive(baseMessage);
            }
        }

    }

    /*
    * @param deviceGuid 设备的唯一Guid
    * @param params 通知的json格式字符串内容
     */
    public void sendDeviceNotification(String deviceGuid, JsonObject params){
        DeviceNotification deviceNotification = new DeviceNotification("device/out", params.toString());
        bbcDevice.insertNotification(deviceGuid, deviceNotification, new IUiCallback() {
            @Override
            public void onSuccess(JsonObject data) {

            }

            @Override
            public void onError(int errorCode, String message) {

            }

            @Override
            public void onCancel() {

            }
        });
    }

    /*
   * @param deviceGuid 设备的唯一Guid
   * @param params 通知的json格式字符串内容
    */
    public void sendDeviceNotification(String deviceGuid, JsonArray users, JsonObject params){
        DeviceNotification deviceNotification = new DeviceNotification("device/out",users,params.toString());
        bbcDevice.insertNotification(deviceGuid, deviceNotification, new IUiCallback() {
            @Override
            public void onSuccess(JsonObject data) {

            }

            @Override
            public void onError(int errorCode, String message) {

            }

            @Override
            public void onCancel() {

            }
        });
    }


}
