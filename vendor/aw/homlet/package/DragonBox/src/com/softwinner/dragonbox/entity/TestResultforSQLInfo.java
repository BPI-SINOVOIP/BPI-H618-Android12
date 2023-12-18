package com.softwinner.dragonbox.entity;

import android.annotation.SuppressLint;
import android.util.ArrayMap;
import android.util.Log;

public class TestResultforSQLInfo {
    public static final String TAG = "DragonBox-TestResultforSQLInfo";
    public String mPrimaryValue = "";
    @SuppressLint("NewApi")
    public ArrayMap<String, String> mArrResultInfo = new ArrayMap<String, String>(15);
    public final String[] mLegalKey = {"SN",//0:primary key
            "version","display","model","ddr","flash",//1-5:CaseVersion
            "rotpk","hdcp","widevine","mac","wifi_mac",//6-10:CaseTMSN
            "couple",//11:CaseCouple
            "aging",//12:CaseAging
            "cpu",//13:CaseCPU
            "usb",//14:CaseUsbVolume
            "wifi",//15:CaseWifi
            "ethernet",//16:CaseEthernet
            "ir",//17:CaseKey
            "hdmi",//18:CaseHdmi
            "cvbs",//19:CaseCvbs
            "sql",//20:CaseSql
            "reset",//21:CaseResetKey
            "result",//22:final result
            "led",//23
            "rflogname",//24
            "rf",//RF25
            //"utdid"//23:for ali
    };

    @SuppressLint("NewApi")
    public synchronized boolean putOneInfo(String key,String value) {
        for(String legalKey:mLegalKey) {
            if(legalKey.equals(key)) {
                mArrResultInfo.put(key, value);
                Log.w(TAG,"put ("+key+", "+value+")");
                return true;
            }
        }
        return false;
    }

    public ArrayMap<String, String> getALLResultInfo(){
        return mArrResultInfo;
    }

    public String getPrimaryKey() {
        return mLegalKey[0];
    }

    public void setPrimaryValue(String value) {
        mPrimaryValue = value;
    }

    public String getPrimaryValue() {
        return mPrimaryValue;
    }

}
