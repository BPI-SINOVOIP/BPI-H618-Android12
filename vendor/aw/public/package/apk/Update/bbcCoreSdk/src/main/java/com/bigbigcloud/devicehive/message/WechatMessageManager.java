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
import com.google.gson.JsonObject;

import org.xutils.common.Callback;
import org.xutils.common.util.MD5;

import java.io.File;
import java.util.ArrayList;
import java.util.Collections;
import java.util.List;

/**
 * Created by Administrator on 2015/11/10.
 */
public class WechatMessageManager implements MessageDispather.CommandMessageListener{

    private final String TAG = "WechatMessageManager";

    private List<BaseMessage> messageList;
    private List<BaseMessage> imageList;
    private List<MessageReciever> recieverList;
    private volatile static WechatMessageManager instance;

    private BbcDevice bbcDevice;
    private Property property;

    private String cachedir;

    private List<String> wxMessageTypes = new ArrayList<String>(){
        {
            add(MessageType.WX_TEXT);
            add(MessageType.WX_IMAGE);
            add(MessageType.WX_VOICE);
            add(MessageType.WX_VIDEO);
            add(MessageType.WX_SHORT_VIDEO);
        }
    };

    public static WechatMessageManager getInstance(){
        if(instance == null){
            synchronized (WechatMessageManager.class){
                if(instance == null){
                    instance = new WechatMessageManager();
                }
            }
        }
        return instance;
    }

    protected WechatMessageManager(){
        if(messageList == null){
            messageList = Collections.synchronizedList(new ArrayList<BaseMessage>());
        }
        if (imageList == null){
            imageList = Collections.synchronizedList(new ArrayList<BaseMessage>());
        }
        if(recieverList == null){
            recieverList = Collections.synchronizedList(new ArrayList<MessageReciever>());
        }
        MessageDispather.getInstance().registerMessageListener(this, wxMessageTypes);
    }

    public void init(Context context, BbcDevice bbcDevice){
        cachedir = context.getCacheDir().getAbsolutePath();
        this.bbcDevice = bbcDevice;
        property = new Property(context);
    }

    public void likeImage(String deviceId, String vendor, String picurl,
                          String mediaId, boolean liked, IUiCallback iUiCallback){
        JsonObject parameters = new JsonObject();
        parameters.addProperty("deviceGuid", property.getDeviceGuid());
        parameters.addProperty("deviceId", deviceId);
        parameters.addProperty("vendor", vendor);
        parameters.addProperty("picurl", picurl);
        parameters.addProperty("mediaId", mediaId);
        if(liked){
            parameters.addProperty("like", 1);
        }else {
            parameters.addProperty("like", 0);
        }
        DeviceNotification deviceNotification = new DeviceNotification("wechat/piclike", parameters.toString());
        bbcDevice.insertNotification(property.getDeviceGuid(), deviceNotification, iUiCallback);
    }

    public void commentOnImage(String deviceId, String vendor, String picurl,
                               String mediaId, String comments, IUiCallback iUiCallback){
        JsonObject parameters = new JsonObject();
        parameters.addProperty("deviceGuid", property.getDeviceGuid());
        parameters.addProperty("deviceId", deviceId);
        parameters.addProperty("vendor", vendor);
        parameters.addProperty("picurl", picurl);
        parameters.addProperty("mediaId", mediaId);
        parameters.addProperty("content", comments);
        DeviceNotification deviceNotification = new DeviceNotification("wechat/piccomment", parameters.toString());
        bbcDevice.insertNotification(property.getDeviceGuid(), deviceNotification, iUiCallback);
    }

    public void requestWxQrcode(String deviceId, String vendor, String mac, IUiCallback iUiCallback){
        JsonObject parameters = new JsonObject();
        parameters.addProperty("deviceGuid", property.getDeviceGuid());
        parameters.addProperty("deviceId", deviceId);
        parameters.addProperty("vendor", vendor);
        parameters.addProperty("mac", mac);
        DeviceNotification deviceNotification = new DeviceNotification("wechat/qrcode", parameters.toString());
        bbcDevice.insertNotification(property.getDeviceGuid(), deviceNotification, iUiCallback);
    }

    public void registerWechatReceiver(MessageReciever reciever){
        recieverList.add(reciever);
    }

    public  void unRegisterWechatReceiver(MessageReciever reciever){
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
        Gson gson = new Gson();
        String messageType = message.getMessageType();
        BaseMessage baseMessage = null;
        if(messageType.equals(MessageType.WX_TEXT)){
            baseMessage = gson.fromJson(message.getParameters(), TextMessage.class);
        }else if (messageType.equals(MessageType.WX_IMAGE)){
            baseMessage = gson.fromJson(message.getParameters(), ImageMessage.class);
            imageList.add(baseMessage);
            downLoadMessage(baseMessage);
        }else if (messageType.equals(MessageType.WX_VOICE)){
            baseMessage = gson.fromJson(message.getParameters(), VoiceMessage.class);
            downLoadMessage(baseMessage);
        }else if (messageType.equals(MessageType.WX_VIDEO) || messageType.equals(MessageType.WX_SHORT_VIDEO)){
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

}
