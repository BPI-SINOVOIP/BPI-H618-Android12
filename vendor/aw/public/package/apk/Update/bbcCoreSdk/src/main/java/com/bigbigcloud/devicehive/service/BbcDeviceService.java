package com.bigbigcloud.devicehive.service;

import android.app.Service;
import android.content.BroadcastReceiver;
import android.content.Context;
import android.content.Intent;
import android.content.IntentFilter;
import android.net.ConnectivityManager;
import android.os.Binder;
import android.os.IBinder;
import android.support.annotation.Nullable;
import android.text.TextUtils;
import android.util.Log;

import com.bigbigcloud.devicehive.BaseDeviceApplication;
import com.bigbigcloud.devicehive.api.BbcDevice;
import com.bigbigcloud.devicehive.api.IUiCallback;
import com.bigbigcloud.devicehive.entity.Device;
import com.bigbigcloud.devicehive.entity.DeviceCommand;
import com.bigbigcloud.devicehive.entity.UpdateFailedInfo;
import com.bigbigcloud.devicehive.message.BbcMessage;
import com.bigbigcloud.devicehive.message.MessageDispather;
import com.bigbigcloud.devicehive.utils.Utils;
import com.google.gson.JsonObject;

/**
 * Created by Administrator on 2016/3/27.
 */
public abstract class BbcDeviceService extends Service implements BbcDevice.BbcResultCallback {

    private static final String TAG = "BbcDeviceService";
    private String license;
    private String timestamp;
    protected Property property;
    protected BbcDevice bbcDevice;
    protected Device device;
    protected boolean waitForNetwork = false;

    protected ServiceBinder binder = new ServiceBinder();
    protected ConnectionReceiver connectionReceiver;


    @Override
    public void onCreate() {
        Log.d(TAG, " BbcDeviceService onCreate");
        super.onCreate();
        BaseDeviceApplication baseDeviceApplication = (BaseDeviceApplication)getApplication();
        license = baseDeviceApplication.getLicense();
        property = new Property(this);
        timestamp = property.getTimeStamp();
        connectionReceiver = new ConnectionReceiver();
        IntentFilter intentFilter = new IntentFilter(ConnectivityManager.CONNECTIVITY_ACTION);
        registerReceiver(connectionReceiver, intentFilter);
        initBbcDevice();
        if(Utils.isNetworkConnected(this)){
            Log.d(TAG, " register device");
            device = getDeviceInfo();
            if(device == null || TextUtils.isEmpty(device.getDeviceId())){
                Log.d(TAG, " device invalidated ");
                return;
            }
            registerOrUpdateDevice(device);
        }else {
            waitForNetwork = true;
        }
    }

    @Override
    public void onDestroy() {
        super.onDestroy();
        unregisterReceiver(connectionReceiver);
        if(bbcDevice != null) {
            bbcDevice.close();
        }
    }

    @Nullable
    @Override
    public IBinder onBind(Intent intent) {
        return binder;
    }

    @Override
    public void onConnect() {
        Log.d(TAG, " onconnect ,then subscribe ");
        if (isRealtimeConnection()) {
            bbcDevice.subscribeForCommands(timestamp, property.getDeviceGuid(), new IUiCallback() {
                @Override
                public void onSuccess(JsonObject data) {
                    Log.d(TAG, " async subscribe success");
                    MessageDispather.getInstance().notifyConnectionInfo(MessageDispather.ConnectionInfoType.CONECT_SUCESS);
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

    @Override
    public void onReceiveCommands(DeviceCommand deviceCommand) {
        if(deviceCommand == null){
            return;
        }
        timestamp = deviceCommand.getTimestamp();
        property.setTimeStamp(timestamp);
        String msgType = deviceCommand.getCommand();
        String parameters = (String)deviceCommand.getParameters();
        Log.d(TAG, "parameters : " + parameters);
        BbcMessage message = new BbcMessage(msgType, parameters);
        MessageDispather.getInstance().notifyCommandMessageReceive(message);
    }

    private void initBbcDevice(){
        if (isWebsocketPrefered()){
            bbcDevice = BbcDevcieWebsocketImpl.getInstance();
        }else {
            bbcDevice = BbcDeviceRestImpl.getInstance();
        }
        bbcDevice.setBbcResultCallback(this);
    }


    public void registerOrUpdateDevice(Device device){
        if(device == null){
            return;
        }
        if(TextUtils.isEmpty(property.getDeviceGuid())){
            bbcDevice.regesterDevice(device, license, new IUiCallback(){
                @Override
                public void onCancel() {

                }

                @Override
                public void onSuccess(JsonObject data) {
                    property.setDeviceGuid(data.get("deviceGuid").getAsString());
                    Principal principal = Principal.createDevice(property.getDeviceGuid(), license);
                    bbcDevice.setPrincipal(principal);
                    syncDeviceInfo();
                }

                @Override
                public void onError(int errorCode, String message) {

                }
            });
        }else {
            Principal principal = Principal.createDevice(property.getDeviceGuid(), license);
            bbcDevice.setPrincipal(principal);
            syncDeviceInfo();
        }
    }

    private void syncDeviceInfo(){
        UpdateFailedInfo updateFailedInfo = null;
        if(property.getUpdateReboot()){
            property.setUpdateReboot(false);
            JsonObject updateInfo = new JsonObject();
            updateFailedInfo = new UpdateFailedInfo(property.getCurVersion(), property.getUpdateErrorCode());
        }
        bbcDevice.syncDeviceInfo(device, property.getDeviceGuid(),license, updateFailedInfo, new IUiCallback() {
            @Override
            public void onSuccess(JsonObject data) {
                Log.d(TAG, " sync device info success data " + data.toString());
                onBbcDeviceReady(bbcDevice);
                bbcDevice.connect();
            }

            @Override
            public void onError(int errorCode, String message) {
                Log.d(TAG, " sync device info error " + message);
            }

            @Override
            public void onCancel() {

            }
        });
    }

    abstract protected boolean isRealtimeConnection();

    abstract protected boolean isWebsocketPrefered();

    abstract protected void onBbcDeviceReady(BbcDevice bbcDevice);

    abstract  protected Device getDeviceInfo();

    public
    class ServiceBinder extends Binder {
        public Service getService(){
            return BbcDeviceService.this;
        }

        public BbcDevice getBbcDevice(){return bbcDevice;}
    }

    class ConnectionReceiver extends BroadcastReceiver {
        @Override
        public void onReceive(Context context, Intent intent) {
            if(waitForNetwork & Utils.isNetworkConnected(BbcDeviceService.this)){
                Log.d(TAG, " network connected !!!!!!");
                waitForNetwork = false;
                device = getDeviceInfo();
                if(device == null || TextUtils.isEmpty(device.getDeviceId())){
                    Log.d(TAG, " device invalidated ");
                    return;
                }
                registerOrUpdateDevice(device);
            }
        }
    }


}
