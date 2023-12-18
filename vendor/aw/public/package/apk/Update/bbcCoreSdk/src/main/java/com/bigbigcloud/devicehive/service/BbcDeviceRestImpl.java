package com.bigbigcloud.devicehive.service;

import android.text.TextUtils;
import android.util.Log;

import com.bigbigcloud.devicehive.api.BbcDevice;
import com.bigbigcloud.devicehive.api.IUiCallback;
import com.bigbigcloud.devicehive.entity.Device;
import com.bigbigcloud.devicehive.entity.DeviceCommand;
import com.bigbigcloud.devicehive.entity.DeviceNotification;
import com.bigbigcloud.devicehive.json.GsonFactory;
import com.bigbigcloud.devicehive.utils.Config;
import com.google.gson.Gson;
import com.google.gson.JsonElement;
import com.google.gson.JsonObject;
import com.google.gson.JsonParser;
import com.google.gson.reflect.TypeToken;

import org.xutils.http.HttpMethod;

import java.lang.reflect.Type;
import java.util.HashMap;
import java.util.List;
import java.util.Map;
import java.util.concurrent.ExecutorService;
import java.util.concurrent.Executors;
import java.util.concurrent.Future;

/**
 * Created by Administrator on 2016/3/23.
 */
public class BbcDeviceRestImpl extends BbcDevice {

    private static final String TAG = "BbcDeviceRestImpl";

    private static final int TIMEOUT = 60;
    private static final String TIMESTAMP = "timestamp";
    private static final String WAIT_TIMEOUT_PARAM = "waitTimeout";

    protected RestAgent restAgent;
    protected final ExecutorService subscriptionExecutor = Executors.newFixedThreadPool(2);
    private  Future<?> commandSubscriptionsResults = null;

    protected BbcDeviceRestImpl(RestAgent restAgent){
        this.restAgent = restAgent;
    }

    private static class SingletonHolder{
        public static BbcDeviceRestImpl INSTANCE = new BbcDeviceRestImpl(new RestAgent(Config.getInstance().getRestfulUri()));
    }

    public static BbcDeviceRestImpl getInstance(){
        return SingletonHolder.INSTANCE;
    }

    @Override
    public void setPrincipal(Principal principal) {
        restAgent.setPrincipal(principal);
    }

    @Override
    public void getDeviceInfo(String deviceGuid, IUiCallback iUiCallback) {
        String path = "/device/" + deviceGuid;
        restAgent.execute(path, HttpMethod.GET, new TemptCallback(iUiCallback));
    }

    @Override
    public void updateDeviceInfo(String deviceGuid, JsonObject updateInfo, IUiCallback iUiCallback) {
        String path = "/device/" + deviceGuid;
        restAgent.execute(path, HttpMethod.PUT, null, updateInfo, new TemptCallback(iUiCallback));
    }

    @Override
    public void getBindedUsers(String deviceGuid, IUiCallback iUiCallback) {
        if(TextUtils.isEmpty(deviceGuid)){
            return;
        }
        String path = "/device/" + deviceGuid + "/users";
        restAgent.execute(path, HttpMethod.GET, new TemptCallback(iUiCallback));
    }

    @Override
    public void subscribeForCommands(String timestamp, String deviceGuid, IUiCallback iUiCallback) {
        if(TextUtils.isEmpty(deviceGuid)){
            return;
        }
        subscribeForCommands(timestamp, deviceGuid);
        iUiCallback.onSuccess(null);
    }

    @Override
    public void unsubscribeFromCommands(IUiCallback iUiCallback) {
        unsubscribeFromCommands();
        iUiCallback.onSuccess(null);
    }

    @Override
    public void insertNotification(String deviceGuid, DeviceNotification deviceNotification, IUiCallback iUiCallback) {
        if(TextUtils.isEmpty(deviceGuid) || deviceNotification == null){
            return;
        }
        String path = "/device/" + deviceGuid + "/notification";
        restAgent.execute(path, HttpMethod.POST, null, deviceNotification, new TemptCallback(iUiCallback));
    }

    @Override
    public void unBindUser(String deviceGuid, Long userId, IUiCallback iUiCallback) {
        if(TextUtils.isEmpty(deviceGuid) || userId == null){
            return;
        }
        String path = "/device/" + deviceGuid + "/users/" + userId;
        restAgent.execute(path, HttpMethod.DELETE, new TemptCallback(iUiCallback));
    }

    @Override
    public void inquireForUpdate(String deviceGuid, Device device, String tgtVersion, IUiCallback iUiCallback) {
        if(TextUtils.isEmpty(deviceGuid) || device == null){
            return;
        }
        String path = "/device/" + deviceGuid + "/otaupdate";
        JsonObject request = new JsonObject();
        request.addProperty("deviceGuid", deviceGuid);
        request.addProperty("curVersion", device.getFirmwareVersion());
        request.addProperty("tgtVersion", tgtVersion);
        request.addProperty("vendor", device.getVendor());
        request.addProperty("romType", device.getRomType());
        request.addProperty("mac", device.getMac());
        request.addProperty("deviceId", device.getDeviceId());
        JsonObject deviceClass = new JsonObject();
        deviceClass.addProperty("name", device.getDeviceClass().getName());
        request.add("deviceClass", deviceClass);
        restAgent.execute(path, HttpMethod.POST, null,request, new TemptCallback(iUiCallback));
    }

    private void subscribeForCommands(final String timestamp, String deviceGuid) {
        if(TextUtils.isEmpty(deviceGuid)){
            return;
        }
        if(commandSubscriptionsResults != null){
            unsubscribeFromCommands();
        }
        final String path = String.format("/device/%s/command/poll", deviceGuid);
        final Map<String, String> params = new HashMap<>();
        params.put(WAIT_TIMEOUT_PARAM, String.valueOf(TIMEOUT));
        if(timestamp != null) {
            params.put(TIMESTAMP, timestamp);
        }
        final Type responseType = new TypeToken<List<DeviceCommand>>() {
        }.getType();
        final Gson gson = GsonFactory.createGson();
        Runnable restSubscription = new Runnable() {
            @Override
            public void run() {
                while (!Thread.currentThread().isInterrupted()){
                    String result = null;
                    try {
                        result = restAgent.executeSync(path, HttpMethod.GET, null, params, String.class);
                    }catch (Throwable e){
                        Log.d(TAG, " subscribeForCommands eception " + e.getMessage() );
                    }
                    if(result != null){
                        JsonElement jsonElement = new JsonParser().parse(result);
                        final List<DeviceCommand> responses = gson.fromJson(jsonElement, responseType);
                        for (final DeviceCommand response : responses) {
                            bbcResultCallback.onReceiveCommands(response);
                        }
                        if (!responses.isEmpty()) {
                            final String newTimestamp = responses.get(responses.size() - 1).getTimestamp();
                            params.put(TIMESTAMP, newTimestamp);
                        }
                    }

                }
            }
        };
        commandSubscriptionsResults = subscriptionExecutor.submit(restSubscription);
    }

    public void unsubscribeFromCommands() {
        if(commandSubscriptionsResults != null){
            commandSubscriptionsResults.cancel(true);
            commandSubscriptionsResults = null;
        }
    }


    @Override
    public void connect() {
        bbcResultCallback.onConnect();
    }

    @Override
    public void close() {
        unsubscribeFromCommands();
    }
}
