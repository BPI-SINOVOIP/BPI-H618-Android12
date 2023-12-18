package com.bigbigcloud.devicehive;

import android.app.Application;
import android.content.pm.ApplicationInfo;
import android.content.pm.PackageManager;
import android.util.Log;

import com.bigbigcloud.devicehive.service.Property;

import org.xutils.x;

/**
 * Created by Administrator on 2016/1/27.
 */
public class BaseDeviceApplication extends Application {
    protected String license;
    @Override
    public void onCreate() {
        super.onCreate();
        x.Ext.init(this);
        x.Ext.setDebug(true);
        try{
            ApplicationInfo applicationInfo = getPackageManager().getApplicationInfo(getPackageName(), PackageManager.GET_META_DATA);
            license = applicationInfo.metaData.getString("license");
        }catch (PackageManager.NameNotFoundException e){
            e.printStackTrace();
        }
        Property property = new Property(getApplicationContext());
        Log.d("yang", " run on environment " + property.getEnvironment());
    }

    @Override
    public void onTerminate() {
        super.onTerminate();
    }

    public String getLicense(){
        return license;
    }
}
