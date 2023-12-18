package com.bigbigcloud.devicehive.service;

import android.text.TextUtils;
import android.util.Log;

import com.bigbigcloud.devicehive.api.IUiCallback;
import com.bigbigcloud.devicehive.entity.Device;
import com.bigbigcloud.devicehive.entity.DeviceCommand;
import com.bigbigcloud.devicehive.entity.DeviceNotification;
import com.bigbigcloud.devicehive.json.GsonFactory;
import com.bigbigcloud.devicehive.message.MessageDispather;
import com.bigbigcloud.devicehive.utils.Config;
import com.google.gson.Gson;
import com.google.gson.JsonElement;
import com.google.gson.JsonObject;
import com.google.gson.JsonParseException;
import com.google.gson.JsonParser;

import java.util.UUID;
import java.util.concurrent.ConcurrentHashMap;
import java.util.concurrent.ConcurrentMap;

/**
 * Created by Administrator on 2016/3/24.
 */
public class BbcDevcieWebsocketImpl extends BbcDeviceRestImpl implements WebsocketAgent.WebsocketCallback{

    private static final String TAG = "BbcDevcieWebsocketImpl";

    public final static String ACTION_MEMBER = "action";
    public final static String COMMAND_INSERT = "command/insert";
    public final static String NOTIFICATION_INSERT = "notification/insert";
    public final static String DEVICE_UNBINDUSER = "device/unbinduser";
    public final static String DEVICE_OTA_UPDATE = "device/otaupdate";
    public final static String DEVICE_GET_INFO = "device/get";
    public final static String DEVICE_UPDATE_INFO = "device/update";
    public final static String DEVICE_USERS = "device/users";
    public final static String COMMAND_SUBSCRIBE = "command/subscribe";
    public final static String COMMAND_MEMBER = "command";
    public final static String SUBSCRIPTION_ID = "subscriptionId";
    private static final String EXPECTED_RESPONSE_STATUS = "success";
    private static final String STATUS = "status";
    private static final String REQUEST_ID = "requestId";

    private ConcurrentMap<String, IUiCallback> websocketCallbackMap = new ConcurrentHashMap<String, IUiCallback>();

    private WebsocketAgent websocketAgent;
    private String subscribeId = null;

    private BbcDevcieWebsocketImpl(WebsocketAgent websocketAgent){
        super(websocketAgent);
        this.websocketAgent = websocketAgent;
        websocketAgent.setWebsocketCallback(this);
    }

    private static class SingletonHolder{
        public static BbcDevcieWebsocketImpl
                INSTANCE = new BbcDevcieWebsocketImpl(new WebsocketAgent(Config.getInstance().getRestfulUri(),
                Config.getInstance().getWebsocketUri(), "device"));
    }

    public static BbcDevcieWebsocketImpl getInstance(){
        return SingletonHolder.INSTANCE;
    }

    @Override
    public void setPrincipal(Principal principal) {
        websocketAgent.setPrincipal(principal);
    }

    @Override
    public  void subscribeForCommands(String timestamp, String deviceGuid,IUiCallback iUiCallback){
        if (TextUtils.isEmpty(deviceGuid)){
            iUiCallback.onError(-1, " params invalid !!");
            return;
        }
        if(subscribeId != null){
            unsubscribeFromCommands();
        }
        final JsonObject request = new JsonObject();
        request.addProperty(ACTION_MEMBER, "command/subscribe");
        request.addProperty("deviceGuid", deviceGuid);
        request.addProperty("timestamp", timestamp);
        final String requestId = UUID.randomUUID().toString();
        request.addProperty(REQUEST_ID, requestId);
        websocketCallbackMap.put(requestId, iUiCallback);
        websocketAgent.rawSendMessage(request);
    }
    @Override
    public  void unsubscribeFromCommands(IUiCallback iUiCallback){
        if(subscribeId == null) return;
        final JsonObject request = new JsonObject();
        request.addProperty(ACTION_MEMBER, "command/unsubscribe");
        request.addProperty(SUBSCRIPTION_ID, subscribeId);
        final String requestId = UUID.randomUUID().toString();
        request.addProperty(REQUEST_ID, requestId);
        websocketCallbackMap.put(requestId, iUiCallback);
        websocketAgent.rawSendMessage(request);
    }
    @Override
    public  void insertNotification(String deviceGuid,DeviceNotification deviceNotification, IUiCallback iUiCallback){
        if (deviceNotification == null || deviceGuid == null) {
            return;
        }
        JsonObject request = new JsonObject();
        request.addProperty("action", "notification/insert");
        request.addProperty("deviceGuid", deviceGuid);
        Gson gson = GsonFactory.createGson();
        request.add("notification", gson.toJsonTree(deviceNotification));
        final String requestId = UUID.randomUUID().toString();
        request.addProperty(REQUEST_ID, requestId);
        websocketCallbackMap.put(requestId, iUiCallback);
        websocketAgent.rawSendMessage(request);
    }

    @Override
    public void getDeviceInfo(String deviceGuid, IUiCallback iUiCallback) {
        if (TextUtils.isEmpty(deviceGuid)){
            iUiCallback.onError(-1, " params invalid !!");
            return;
        }
        JsonObject request = new JsonObject();
        request.addProperty("action", DEVICE_GET_INFO);
        request.addProperty("deviceGuid", deviceGuid);
        final String requestId = UUID.randomUUID().toString();
        request.addProperty(REQUEST_ID, requestId);
        websocketCallbackMap.put(requestId, iUiCallback);
        websocketAgent.rawSendMessage(request);
    }

    @Override
    public void updateDeviceInfo(String deviceGuid, JsonObject updateInfo, IUiCallback iUiCallback) {
        if (TextUtils.isEmpty(deviceGuid)){
            iUiCallback.onError(-1, " params invalid !!");
            return;
        }
        JsonObject request = new JsonObject();
        request.addProperty("action", DEVICE_UPDATE_INFO);
        request.addProperty("deviceGuid", deviceGuid);
        request.add("device", updateInfo);
        final String requestId = UUID.randomUUID().toString();
        request.addProperty(REQUEST_ID, requestId);
        websocketCallbackMap.put(requestId, iUiCallback);
        websocketAgent.rawSendMessage(request);
    }

    @Override
    public  void getBindedUsers(String deviceGuid, IUiCallback iUiCallback){
        if (TextUtils.isEmpty(deviceGuid)){
            iUiCallback.onError(-1, " params invalid !!");
            return;
        }
        JsonObject request = new JsonObject();
        request.addProperty("action", DEVICE_USERS);
        request.addProperty("deviceGuid", deviceGuid);
        final String requestId = UUID.randomUUID().toString();
        request.addProperty(REQUEST_ID, requestId);
        websocketCallbackMap.put(requestId, iUiCallback);
        websocketAgent.rawSendMessage(request);
    }
    @Override
    public  void unBindUser(String deviceGuid, Long userId, IUiCallback iUiCallback){
        if (TextUtils.isEmpty(deviceGuid) || userId == null){
            iUiCallback.onError(-1, " params invalid !!");
            return;
        }
        JsonObject request = new JsonObject();
        request.addProperty("action", DEVICE_UNBINDUSER);
        request.addProperty("deviceGuid", deviceGuid);
        request.addProperty("userId", userId);
        final String requestId = UUID.randomUUID().toString();
        request.addProperty(REQUEST_ID, requestId);
        websocketCallbackMap.put(requestId, iUiCallback);
        websocketAgent.rawSendMessage(request);
    }
    @Override
    public  void inquireForUpdate(String deviceGuid,Device device,String tgtVersion, IUiCallback iUiCallback){
        if (TextUtils.isEmpty(deviceGuid) || device == null){
            iUiCallback.onError(-1, " params invalid !!");
            return;
        }
        JsonObject request = new JsonObject();
        request.addProperty("action", "device/otaupdate");
        request.addProperty("deviceGuid", deviceGuid);
        request.addProperty("curVersion", device.getFirmwareVersion());
        request.addProperty("tgtVersion", tgtVersion);
        request.addProperty("vendor", device.getVendor());
        request.addProperty("romType", device.getRomType());
        request.addProperty("mac", device.getMac());
        request.addProperty("deviceId", device.getDeviceId());
        JsonObject deviceClass = new JsonObject();
        deviceClass.addProperty("name", device.getDeviceClass().getName());
        request.add("deviceClass", deviceClass);final String requestId = UUID.randomUUID().toString();
        request.addProperty(REQUEST_ID, requestId);
        websocketCallbackMap.put(requestId, iUiCallback);
        websocketAgent.rawSendMessage(request);
    }

    @Override
    public void unsubscribeFromCommands() {
        if(subscribeId == null) return;
        final JsonObject request = new JsonObject();
        request.addProperty(ACTION_MEMBER, "command/unsubscribe");
        request.addProperty(SUBSCRIPTION_ID, subscribeId);
        websocketAgent.rawSendMessage(request);
    }

    @Override
    public void close() {
        websocketAgent.disconnect();
    }

    @Override
    public void connect() {
        websocketAgent.connect();
    }

    @Override
    public void onOpen() {
       bbcResultCallback.onConnect();
    }

    @Override
    public void onReconnect() {
        MessageDispather.getInstance().notifyConnectionInfo(MessageDispather.ConnectionInfoType.RECONNECT);
    }

    @Override
    public void onClose() {
        MessageDispather.getInstance().notifyConnectionInfo(MessageDispather.ConnectionInfoType.CONECT_FAILED);
    }

    @Override
    public void handleMessage(String message) {
        JsonObject jsonMessage = null;
        try {
            jsonMessage = new JsonParser().parse(message).getAsJsonObject();
        } catch (JsonParseException ex) {
            Log.e(TAG,"Server sent incorrect message {}" + message);
        }catch (IllegalStateException ex){
            Log.e(TAG,"Server sent incorrect message {}" + message);
        }
        if(jsonMessage == null) return;
        Gson gson = GsonFactory.createGson();
        JsonElement reqestId = jsonMessage.get(REQUEST_ID);
        IUiCallback iUiCallback = null;
        if (reqestId != null && !reqestId.isJsonNull()) {
             iUiCallback = websocketCallbackMap.get(reqestId.getAsString());
        }
        final String action = jsonMessage.get(ACTION_MEMBER).getAsString();
        if (action.equals(COMMAND_INSERT)) {
            DeviceCommand deviceCommand = gson.fromJson(jsonMessage.getAsJsonObject(COMMAND_MEMBER),DeviceCommand.class);
            bbcResultCallback.onReceiveCommands(deviceCommand);
        }
        else if (action.equals(COMMAND_SUBSCRIBE)){
            String status = jsonMessage.get(STATUS).getAsString();
            boolean result = EXPECTED_RESPONSE_STATUS.equals(status);
            if(result) {
                subscribeId = jsonMessage.get(SUBSCRIPTION_ID).getAsString();
            }
            if(iUiCallback != null){
                if(result){
                    iUiCallback.onSuccess(jsonMessage);
                }else {
                    iUiCallback.onError(-1, "subscribe failed");
                }
            }
        }
        else if(action.equals(DEVICE_USERS) || action.equals(DEVICE_GET_INFO)
                || action.equals(DEVICE_UPDATE_INFO) ){
            if(iUiCallback != null){
                String status = jsonMessage.get(STATUS).getAsString();
                boolean result = EXPECTED_RESPONSE_STATUS.equals(status);
                if(result){
                    JsonElement dataElement = jsonMessage.get("data");
                    if(dataElement != null){
                        iUiCallback.onSuccess(dataElement.getAsJsonObject());
                    }else {
                        iUiCallback.onSuccess(new JsonObject());
                    }
                }else {
                    iUiCallback.onError(jsonMessage.get("errorCode").getAsInt(), jsonMessage.get("message").getAsString());
                }
            }
        }
        else if (action.equals(NOTIFICATION_INSERT) || action.equals(DEVICE_UNBINDUSER)){
            String status = jsonMessage.get(STATUS).getAsString();
            boolean result = EXPECTED_RESPONSE_STATUS.equals(status);
            if(iUiCallback != null){
                if(result){
                    iUiCallback.onSuccess(null);
                }else {
                    iUiCallback.onError(-1, "error message " +jsonMessage.get("message").getAsString());
                }
            }
        }
        else if(action.equals(DEVICE_OTA_UPDATE)){
            String status = jsonMessage.get(STATUS).getAsString();
            boolean result = EXPECTED_RESPONSE_STATUS.equals(status);
            if(iUiCallback != null){
                if(result){
                    JsonElement data = jsonMessage.get("data");
                    if(data == null || data.isJsonNull()){
                        iUiCallback.onError(-1, jsonMessage.get("message").getAsString());
                    }else {
                        iUiCallback.onSuccess(data.getAsJsonObject());
                    }
                }else {
                    iUiCallback.onError(jsonMessage.get("errorCode").getAsInt(), jsonMessage.get("message").getAsString());
                }
            }
        }
    }
}
