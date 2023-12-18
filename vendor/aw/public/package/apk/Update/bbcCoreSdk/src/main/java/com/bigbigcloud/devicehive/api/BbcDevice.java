package com.bigbigcloud.devicehive.api;


import android.os.Handler;
import android.os.Looper;
import android.os.Message;
import android.text.TextUtils;
import android.util.Log;

import com.bigbigcloud.devicehive.entity.Device;
import com.bigbigcloud.devicehive.entity.DeviceCommand;
import com.bigbigcloud.devicehive.entity.DeviceNotification;
import com.bigbigcloud.devicehive.entity.UpdateFailedInfo;
import com.bigbigcloud.devicehive.json.GsonFactory;
import com.bigbigcloud.devicehive.service.Principal;
import com.bigbigcloud.devicehive.service.RestAgent;
import com.bigbigcloud.devicehive.utils.Config;
import com.bigbigcloud.devicehive.utils.Utils;
import com.google.gson.JsonElement;
import com.google.gson.JsonObject;
import com.google.gson.JsonParser;

import org.xutils.common.Callback;
import org.xutils.http.HttpMethod;
import org.xutils.http.RequestParams;
import org.xutils.x;

/**
 * Created by Administrator on 2016/2/19.
 */
public abstract class BbcDevice {

    private static String TAG = "BbcDevice";

    protected BbcResultCallback bbcResultCallback;


    public void setBbcResultCallback(BbcResultCallback callback){
        bbcResultCallback = callback;
    }


    /*
    * 注册设备
    * @Param device  Device对象，存放设备的信息
    * @Param 设备对应的license
     */
    public void regesterDevice(Device device, String license, IUiCallback iUiCallback){
        if(device == null || TextUtils.isEmpty(device.getDeviceId())){
            iUiCallback.onError(-1, "device params not right");
            return;
        }
        long timestamp = System.currentTimeMillis()/1000;
        String strToEncode = device.getDeviceId() + device.getMac() + device.getVendor()
                + device.getDeviceClass().getName() + timestamp + license;
        String signature = Utils.SHA1(strToEncode);
        Log.d(TAG, " strToEncode " + strToEncode + " signature " + signature);
        RequestParams requestParams = new RequestParams(Config.getInstance().getRestfulUri() + "/device");
        requestParams.addHeader(RestAgent.AUTH_SIGNATURE_HEADER, signature);
        requestParams.addHeader(RestAgent.AUTH_TIMESTAMP_HEADER, String.valueOf(timestamp));
        String deviceStr = GsonFactory.createGson().toJson(device);
        JsonObject jsonObject = new JsonParser().parse(deviceStr).getAsJsonObject();
        jsonObject.addProperty("sdkVersion", Config.SDK_VERSION);
        requestParams.setBodyContent(jsonObject.toString());
        requestParams.setAsJsonContent(true);
        x.http().request(HttpMethod.PUT, requestParams, new TemptCallback(iUiCallback));
    }
    /*
    *同步设备信息
    *  @Param device  Device对象，存放设备的信息
    *  @Param 设备对应的license
    *  @param updateFailedInfo 如果设备升级失败，此参数存放升级失败信息。没有升级失败则为null
     */
    public void syncDeviceInfo(Device device, String deviceGuid,String license, UpdateFailedInfo updateFailedInfo, IUiCallback iUiCallback){
        if(device == null || TextUtils.isEmpty(deviceGuid)){
            iUiCallback.onError(-1, "device params not right");
            return;
        }
        device.setDeviceGuid(deviceGuid);
        long timestamp = System.currentTimeMillis()/1000;
        String strToEncode = device.getDeviceGuid() + timestamp + license;
        String signature = Utils.SHA1(strToEncode);
        RequestParams requestParams = new RequestParams(Config.getInstance().getRestfulUri() + "/device/" + device.getDeviceGuid());
        requestParams.addHeader(RestAgent.AUTH_SIGNATURE_HEADER, signature);
        requestParams.addHeader(RestAgent.AUTH_TIMESTAMP_HEADER, String.valueOf(timestamp));
        String deviceStr = GsonFactory.createGson().toJson(device);
        JsonObject jsonObject = new JsonParser().parse(deviceStr).getAsJsonObject();
        jsonObject.addProperty("sdkVersion", Config.SDK_VERSION);
        if(updateFailedInfo != null){
            JsonObject upgrade = new JsonObject();
            JsonObject updateInfo = new JsonObject();
            updateInfo.addProperty("curVersion", updateFailedInfo.getCurVersion());
            updateInfo.addProperty("errcode", updateFailedInfo.getErrcode());
            upgrade.add("upgradefail", updateInfo);
            jsonObject.add("upgrade", upgrade);
        }
        Log.d(TAG, " sync device str " + jsonObject.toString());
        requestParams.setBodyContent(jsonObject.toString());
        requestParams.setAsJsonContent(true);
        x.http().request(HttpMethod.POST, requestParams, new TemptCallback(iUiCallback));
    }

    /*
    * 从sdk端创建厂商
    * @param name 厂商名
    * @param accessKey 平台给的accesskey
     */
    public void createVendor(String name, String accessKey, IUiCallback iUiCallback){
        if(TextUtils.isEmpty(accessKey)){
            return;
        }
        RequestParams requestParams = new RequestParams(Config.getInstance().getRestfulUri() + "/vendor");
        requestParams.addHeader("Authorization", accessKey);
        JsonObject jsonObject = new JsonObject();
        jsonObject.addProperty("status", 0);
        jsonObject.addProperty("name", name);
        jsonObject.addProperty("source", "sdk");
        requestParams.setBodyContent(jsonObject.toString());
        requestParams.setAsJsonContent(true);
        x.http().request(HttpMethod.POST, requestParams, new TemptCallback(iUiCallback));
    }

    /*
    * 创建deviceClass
    * @param vendorId 厂商Id
    * @param accessKey 平台给的accesskey
    * @param name deviceClass 名字
     */
    public void createDeviceClass(int vendorId, String accessKey, String name,IUiCallback iUiCallback){
        RequestParams requestParams = new RequestParams(Config.getInstance().getRestfulUri() + "/vendor/" + vendorId + "/deviceclass");
        requestParams.addHeader("Authorization", accessKey);
        JsonObject jsonObject = new JsonObject();
        jsonObject.addProperty("name", name);
        jsonObject.addProperty("source", "sdk");
        requestParams.setBodyContent(jsonObject.toString());
        requestParams.setAsJsonContent(true);
        x.http().request(HttpMethod.POST, requestParams, new TemptCallback(iUiCallback));
    }
    /*
     * 获取设备信息
     */
    public abstract  void getDeviceInfo(String deviceGuid, IUiCallback iUiCallback);

    /*
     * 更新设备信息
     */
    public abstract void updateDeviceInfo(String deviceGuid, JsonObject updateInfo, IUiCallback iUiCallback);

    /*
    * 订阅消息
    * @param timestamp 上一次收到消息的时间，格式为UTC，如果为空，则用服务器时间
    * @deviceGuid 设备的Guid
     */
    public abstract void subscribeForCommands(String timestamp, String deviceGuid,IUiCallback iUiCallback);

    /*
    * 取消订阅
     */
    public abstract void unsubscribeFromCommands(IUiCallback iUiCallback);

    /*
    * 发送消息到服务器，到绑定的用户
    * @param deviceGuid 设备Guid
    * @deviceNotification 发送的消息结构体，参考DeviceNotification 类
     */
    public abstract void insertNotification(String deviceGuid,DeviceNotification deviceNotification, IUiCallback iUiCallback);

    /*
    * 获取绑定的用户列表
    *  @param deviceGuid 设备Guid
     */
    public abstract void getBindedUsers(String deviceGuid, IUiCallback iUiCallback);

    /*
    * 解除绑定
    *  @param deviceGuid 设备Guid
    *  @param userId  用户id
     */
    public abstract void unBindUser(String deviceGuid, Long userId, IUiCallback iUiCallback);

    /*
    * 查询ota更新
    * @param deviceGuid 设备Guid
    * @param tgtVersion 目标版本，如果为空，则获取最新版本
     */
    public abstract void inquireForUpdate(String deviceGuid,Device device,String tgtVersion, IUiCallback iUiCallback);

    /*
    * 查询ota更新
    * @param deviceGuid 设备Guid
     */
    public void inquireForUpdate(String deviceGuid,Device device, IUiCallback iUiCallback){
         inquireForUpdate(deviceGuid, device, "", iUiCallback);
    }


    public abstract void connect();

    public abstract void close();

    public abstract void setPrincipal(Principal principal);

    public interface BbcResultCallback{
        void onConnect();
        void onReceiveCommands(DeviceCommand deviceCommand);
    }

    public class TemptCallback implements Callback.CommonCallback<String>{
        private Handler handler;
        private IUiCallback uiCallback;

        public  TemptCallback(IUiCallback uiCallback){
            this.uiCallback = uiCallback;
            handler = new Handler(Looper.getMainLooper(), new Handler.Callback() {
                @Override
                public boolean handleMessage(Message msg) {
                    switch (msg.what){
                        case 0:
                            TemptCallback.this.uiCallback.onSuccess((JsonObject)msg.obj);
                            break;
                        case 1:
                            TemptCallback.this.uiCallback.onError(msg.arg1, (String)msg.obj);
                            break;
                        case 2:
                            TemptCallback.this.uiCallback.onCancel();
                            break;
                    }
                    return true;
                }
            });
        }
        @Override
        public void onCancelled(CancelledException cex) {
            Message message = handler.obtainMessage();
            message.what = 2;
        }

        @Override
        public void onSuccess(String result) {
            Log.d(TAG, "onSuccess result " + result);
            Message message = handler.obtainMessage();
            if(result == null){
                message.what = 1;
                message.obj = "return result null";
                message.arg1 = 50001;//返回空值
                handler.sendMessage(message);
                Log.d(TAG, "onSuccess result null ");
                return;
            }else {
                JsonObject jsonObject = null;
                try {
                    jsonObject = new JsonParser().parse(result).getAsJsonObject();
                }catch (Exception e){
                    message.what = 1;
                    message.obj = "json parse exception";
                    message.arg1 = 50002;//json 解释错误
                    handler.sendMessage(message);
                    Log.d(TAG, "onSuccess result -------2----- ");
                    return;
                }
                String status = jsonObject.get("status").getAsString();
                if(status != null && status.equals("success")){
                    Log.d(TAG,"onSuccess result -------3----- " +  jsonObject.toString());
                    message.what = 0;
                    JsonElement dataElement = jsonObject.get("data");
                    if(dataElement.isJsonNull()){
                        message.obj = null;
                        Log.d(TAG,"onSuccess result -------4----- ");
                    }else {
                        message.obj = dataElement.getAsJsonObject();
                        Log.d(TAG,"onSuccess result -------5----- ");
                    }
                }else {
                    message.what = 1;
                    message.obj = jsonObject.get("message").getAsString();
                    message.arg1 = jsonObject.get("errorCode").getAsInt();
                }
                Log.d(TAG, " send message what " + message.what);
                handler.sendMessage(message);
            }
        }

        @Override
        public void onError(Throwable ex, boolean isOnCallback) {
            Log.d(TAG,"--1---onError result " + ex.getMessage() + "is on callback " + isOnCallback);
            Message message = handler.obtainMessage();
            message.what = 1;
            message.obj = ex.getMessage();
            message.arg1 = 50004;//网络错误
            handler.sendMessage(message);
        }

        @Override
        public void onFinished() {

        }
    }
}
