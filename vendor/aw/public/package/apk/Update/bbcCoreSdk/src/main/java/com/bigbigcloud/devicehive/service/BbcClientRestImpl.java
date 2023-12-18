package com.bigbigcloud.devicehive.service;

import android.content.Context;
import android.os.Handler;
import android.os.Message;
import android.text.TextUtils;
import android.util.Log;

import com.bigbigcloud.devicehive.api.BbcClient;
import com.bigbigcloud.devicehive.api.IUiCallback;
import com.bigbigcloud.devicehive.entity.DeviceCommand;
import com.bigbigcloud.devicehive.utils.Config;
import com.google.gson.JsonElement;
import com.google.gson.JsonObject;
import com.google.gson.JsonParser;

import org.xutils.common.Callback;
import org.xutils.http.HttpMethod;

import java.util.HashMap;
import java.util.Map;

/**
 * Created by Administrator on 2016/5/23.
 */
public class BbcClientRestImpl implements BbcClient{

    private static String TAG = "BbcClientRestImpl";

    private String appId;
    private String appSecret;
    private RestAgent restAgent;
    private Context context;

    private static class SingletonHolder{
        public static BbcClientRestImpl INSTANCE = new BbcClientRestImpl();
    }

    private BbcClientRestImpl(){}

    public static BbcClientRestImpl getInstance(){
        return SingletonHolder.INSTANCE;
    }

    public void setPrinciple(Principal principle){
        restAgent.setPrincipal(principle);
    }

    public void init(Context context,String appId, String appSecret){
        this.context = context;
        this.appId = appId;
        this.appSecret = appSecret;
        restAgent = new RestAgent(Config.getInstance().getRestfulUri());
        restAgent.setPrincipal(Principal.createAppClient(appId, appSecret));
    }

    @Override
    public void getSmsCode(String phoneNum, String action, IUiCallback uiCallback){
        if (TextUtils.isEmpty(phoneNum)){
            uiCallback.onError(-1, " params invalid !!");
            return;
        }
        String path = "/user/smscode";
        Map<String, String> params = new HashMap<>();
        params.put("phone", phoneNum);
        params.put("action", action);
        restAgent.execute(path, HttpMethod.POST, null, params, new TemptCallback(context, uiCallback));
    }
    @Override
    public void getEmailCode(String email,String action,IUiCallback uiCallback){
        if (TextUtils.isEmpty(email)){
            uiCallback.onError(-1, " params invalid !!");
            return;
        }
        String path = "/user/emailcode";
        Map<String, String> params = new HashMap<>();
        params.put("email", email);
        params.put("action", action);
        restAgent.execute(path, HttpMethod.POST, null, params, new TemptCallback(context, uiCallback));
    }
    @Override
    public void getUser(long id, IUiCallback uiCallback) {
        if (id < 0){
            uiCallback.onError(-1, " params invalid !!");
            return;
        }
        String path = "/user/" + id;
        restAgent.execute(path,HttpMethod.GET, new TemptCallback(context, uiCallback));
    }

    @Override
    public void phoneRegister(String phone, String password, String verifyCode, IUiCallback uiCallback){
        if (TextUtils.isEmpty(phone)){
            uiCallback.onError(-1, " params invalid !!");
            return;
        }
        String path = "/user";
        Map<String, String> params = new HashMap<>();
        params.put("phone", phone);
        params.put("password", password);
        params.put("verifyCode", verifyCode);
        restAgent.execute(path, HttpMethod.POST, null, params, new TemptCallback(context,uiCallback));
    }
    @Override
    public void emailRegister(String email, String password, String verifyCode, IUiCallback uiCallback){
        if (TextUtils.isEmpty(email)){
            uiCallback.onError(-1, " params invalid !!");
            return;
        }
        String path = "/user";
        Map<String, String> params = new HashMap<>();
        params.put("email", email);
        params.put("password", password);
        params.put("verifyCode", verifyCode);
        restAgent.execute(path, HttpMethod.POST, null, params, new TemptCallback(context,uiCallback));
    }
    @Override
    public void thirdTypeRegister(String nickName,String headImgurl,
                                  String thirdLogin, String accessToken,
                                  String thirdType, IUiCallback uiCallback){
        String path = "/user";
        Map<String, String> params = new HashMap<>();
        params.put("nickName", nickName);
        params.put("headImgurl", headImgurl);
        params.put("thirdLogin", thirdLogin);
        params.put("accessToken",accessToken);
        params.put("thirdType", thirdType);
        restAgent.execute(path, HttpMethod.POST, null, params, new TemptCallback(context, uiCallback));
    }
    @Override
    public void userLogin(String userName, String password, IUiCallback uiCallback){
        String path = "/user/login";
        Map<String, String> params = new HashMap<>();
        params.put("userName", userName);
        params.put("password", password);
        restAgent.execute(path, HttpMethod.POST, null, params, new TemptCallback(context, uiCallback));
    }
    @Override
    public void verifiedCodeLogin(String phone, String code, IUiCallback uiCallback){
        String path = "/user/login";
        Map<String, String> params = new HashMap<>();
        params.put("phone", phone);
        params.put("verifyCode", code);
        restAgent.execute(path, HttpMethod.POST, null, params, new TemptCallback(context, uiCallback));
    }
    @Override
    public void updateUser(long userId,String nickName, String headImgurl, int sex, IUiCallback uiCallback) {
        if(userId < 0){
            uiCallback.onError(-1, " params invalid !!");
            return;
        }
        Map<String, String> params = new HashMap<>();
        Log.d("yang", " userid " + userId + " nick name " + nickName + "head url " + headImgurl);
        if(nickName != null){
            params.put("nickName", nickName);
        }
        if(headImgurl != null){
            params.put("headImgurl", headImgurl);
        }
        params.put("sex", String.valueOf(sex));
        String path = "/user/" + userId;
        restAgent.execute(path,HttpMethod.PUT,null, params, new TemptCallback(context, uiCallback));
    }
    @Override
    public void bindToPhone(long userId,String phone, String verifyCode, String password,IUiCallback uiCallback){
        if(userId < 0){
            uiCallback.onError(-1, " params invalid !!");
            return;
        }
        String path = "/user/" + userId;
        Map<String, String> params = new HashMap<>();
        params.put("phone", phone);
        params.put("verifyCode", verifyCode);
        params.put("password", password);
        restAgent.execute(path, HttpMethod.PUT, null, params, new TemptCallback(context, uiCallback));
    }
    @Override
    public void bindToEmail(long userId, String email, String verifyCode, String password, IUiCallback uiCallback){
        if(userId < 0 || TextUtils.isEmpty(email)){
            uiCallback.onError(-1, " params invalid !!");
            return;
        }
        String path = "/user/" + userId;
        Map<String, String> params = new HashMap<>();
        params.put("email", email);
        params.put("verifyCode", verifyCode);
        params.put("password", password);
        restAgent.execute(path, HttpMethod.PUT, null, params, new TemptCallback(context, uiCallback));
    }
    @Override
    public void changePassWord(long userId,String oldPass, String newPass,IUiCallback uiCallback){
        if(userId < 0){
            uiCallback.onError(-1, " params invalid !!");
            return;
        }
        String path = "/user/" + userId;
        Map<String, String> params = new HashMap<>();
        params.put("password", newPass);
        params.put("oldPassword", oldPass);
        restAgent.execute(path, HttpMethod.PUT, null, params, new TemptCallback(context, uiCallback));
    }
    @Override
    public void  resetPasswordByPhone(String phone, String verifyCode,String password,IUiCallback uiCallback){
        String path = "/user/resetpwd";
        Map<String, String> params = new HashMap<>();
        params.put("phone", phone);
        params.put("verifyCode", verifyCode);
        params.put("password", password);
        restAgent.execute(path, HttpMethod.POST, null, params, new TemptCallback(context, uiCallback));
    }
    @Override
    public void resetPasswordByEmail(String email, String verifyCode,String password,IUiCallback uiCallback){
        String path = "/user/resetpwd";
        Map<String, String> params = new HashMap<>();
        params.put("email", email);
        params.put("verifyCode", verifyCode);
        params.put("password", password);
        restAgent.execute(path, HttpMethod.POST, null, params, new TemptCallback(context, uiCallback));
    }

    @Override
    public void bindDevice(long userId, String deviceGuid, IUiCallback uiCallback){
        String path = "/user/" + userId + "/device/" + deviceGuid;
        restAgent.execute(path,  HttpMethod.POST, new TemptCallback(context,uiCallback));
    }

    @Override
    public void bindDevice(long userId, String deviceId, String vendor, String deviceClassName, IUiCallback uiCallback) {
        String path = "/user/" + userId + "/device";
        JsonObject jsonObject = new JsonObject();
        jsonObject.addProperty("deviceId", deviceId);
        jsonObject.addProperty("vendor", vendor);
        JsonObject jdeviceClass = new JsonObject();
        jdeviceClass.addProperty("name", deviceClassName);
        jsonObject.add("deviceClass", jdeviceClass);
        restAgent.execute(path, HttpMethod.POST, null, jsonObject, new TemptCallback(context,uiCallback));
    }

    @Override
    public void unBindDevice(long userId, String deviceGuid, IUiCallback uiCallback){
        String path = "/user/" + userId + "/device/" + deviceGuid;
        restAgent.execute(path,  HttpMethod.DELETE, new TemptCallback(context,uiCallback));
    }
    @Override
    public void getBindDevices(long userId, IUiCallback uiCallback){
        String path = "/user/" + userId + "/devices";
        restAgent.execute(path,  HttpMethod.GET, new TemptCallback(context,uiCallback));
    }

    public void getDeviceInfo(String deviceGuid, IUiCallback uiCallback){
        String path = "/device/" + deviceGuid;
        restAgent.execute(path, HttpMethod.GET, new TemptCallback(context,uiCallback));
    }
    @Override
    public void sendCommand(String deviceGuid, DeviceCommand deviceCommand, IUiCallback uiCallback){
        String path = "/device/" + deviceGuid + "/command";
        restAgent.execute(path, HttpMethod.POST, null, deviceCommand, new TemptCallback(context, uiCallback));
    }



    public class TemptCallback implements Callback.CommonCallback<String>{
        private Handler handler;
        private IUiCallback uiCallback;

        public  TemptCallback(Context context,IUiCallback uiCallback){
            this.uiCallback = uiCallback;
            handler = new Handler(context.getMainLooper(), new Handler.Callback() {
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
            Log.d(TAG,"onSuccess result " + result);
            Message message = handler.obtainMessage();
            if(result == null){
                message.what = 1;
                message.obj = "return result null";
                message.arg1 = 50001;//返回空值
                handler.sendMessage(message);
                Log.d(TAG, "onSuccess result null ");
                return;
            }else {
                Log.d(TAG,"onSuccess result -------1----- ");
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
