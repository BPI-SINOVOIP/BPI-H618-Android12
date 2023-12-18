package com.bigbigcloud.devicehive;

import android.app.Application;
import android.content.pm.ApplicationInfo;
import android.content.pm.PackageManager;

import com.bigbigcloud.devicehive.service.BbcClientRestImpl;
import com.bigbigcloud.devicehive.service.Principal;

import org.xutils.x;

/**
 * Created by Thomas.yang on 2016/7/15.
 */
public class BaseClientApplication extends Application {

    protected String appId;
    protected String appSecret;
    protected BbcClientRestImpl bbcClient;

    @Override
    public void onCreate() {
        super.onCreate();
        x.Ext.init(this);
        x.Ext.setDebug(true);
        try{
            ApplicationInfo applicationInfo = getPackageManager().getApplicationInfo(getPackageName(), PackageManager.GET_META_DATA);
            appId = applicationInfo.metaData.getString("appId");
            appSecret = applicationInfo.metaData.getString("appSecret");
        }catch (PackageManager.NameNotFoundException e){
            e.printStackTrace();
        }
        bbcClient = BbcClientRestImpl.getInstance();
        bbcClient.init(getApplicationContext(),appId, appSecret);
    }

    public void setToken(String token) {
        bbcClient.setPrinciple(Principal.createAccessToken(appId, token));
    }

}
