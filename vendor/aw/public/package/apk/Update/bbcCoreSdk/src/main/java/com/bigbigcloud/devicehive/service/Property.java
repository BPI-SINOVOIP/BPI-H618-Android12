package com.bigbigcloud.devicehive.service;

import android.content.Context;
import android.content.SharedPreferences;

/**
 * Created by Administrator on 2015/12/24.
 */
public class Property {

    private static final String REGISTER = "register";
    private static final String TIME_STAMP = "timestamp";
    private static final String CUR_VERSION = "cur_version";
    private static final String SERVER_URL = "server_url";
    private static final String DEVICE_GUID = "device_guid";
    private static final String UPDATE_REBOOT = "update_reboot";
    private static final String UPDATE_ERROR_CODE ="update_error_code";
    private static final String ENVIRONMENT = "environment";

    SharedPreferences preferences;
    SharedPreferences.Editor editor;

    public Property(Context context){
        preferences = context.getSharedPreferences(
                context.getPackageName() + "_devicehiveprefs",
                Context.MODE_PRIVATE);
        editor = preferences.edit();
    }

    public boolean getRegister(){
        return preferences.getBoolean(REGISTER, false);
    }

    public void setRegister(boolean isRegister){
        editor.putBoolean(REGISTER, isRegister);
        editor.commit();
    }

    public boolean getUpdateReboot(){
        return preferences.getBoolean(UPDATE_REBOOT, false);
    }

    public void setUpdateReboot(boolean isUpdateReboot){
        editor.putBoolean(UPDATE_REBOOT, isUpdateReboot);
        editor.commit();
    }

    public int getUpdateErrorCode(){
        return preferences.getInt(UPDATE_ERROR_CODE, 100);
    }

    public void setUpdateErrorCode(int errorCode){
        editor.putInt(UPDATE_ERROR_CODE, errorCode);
        editor.commit();
    }

    public String getDeviceGuid(){
        return preferences.getString(DEVICE_GUID, "");
    }

    public void setDeviceGuid(String deviceGuid){
        editor.putString(DEVICE_GUID, deviceGuid);
        editor.commit();
    }


    public String getTimeStamp(){
        return preferences.getString(TIME_STAMP, "");
    }

    public void setTimeStamp(String timeStamp){
        editor.putString(TIME_STAMP, timeStamp);
        editor.commit();
    }

    public String getCurVersion(){
        return preferences.getString(CUR_VERSION,"0.0.1");
    }

    public void setCurVersion(String version){
        editor.putString(CUR_VERSION, version);
        editor.commit();
    }

    public String getServerUrl(){
        return preferences.getString(SERVER_URL, null);
    }

    public void setServerUrl(String serverUrl){
        editor.putString(SERVER_URL,serverUrl);
        editor.commit();
    }

    public String getEnvironment(){
        return preferences.getString(ENVIRONMENT, "Product");
    }

    public void setEnviroment(String environment){
        editor.putString(ENVIRONMENT, environment);
        editor.commit();
    }
}
