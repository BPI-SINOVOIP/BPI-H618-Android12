package com.bigbigcloud.devicehive.entity;

/**
 * Created by Administrator on 2016/1/2.
 */
public class OtaUpdateNotification {
    private String pushId;
    private int updateType;// 1 静默升级; 2 用户确认升级; 3 强制升级;
    private int wifiOnly;
    private int notify;   //
    private int area;  //
    private String dateStart; //
    private String dateEnd;  //
    private String curVersion;
    private String tgtVersion; //


    public OtaUpdateNotification(){
    }

    public String getPushId() {
        return pushId;
    }

    public int getUpdateType() {
        return updateType;
    }

    public int getWifiOnly() {
        return wifiOnly;
    }

    public int getNotify() {
        return notify;
    }

    public int getArea() {
        return area;
    }

    public String getDateStart() {
        return dateStart;
    }

    public String getDateEnd() {
        return dateEnd;
    }

    public String getCurVersion() {
        return curVersion;
    }

    public String getTgtVersion() {
        return tgtVersion;
    }

    public String toString(){
        return "updateType : " + updateType + " wifionly : "+ wifiOnly
                + " notify : " + notify + " area : " + area
                + " datestart : " + dateStart + " dateend : " + dateEnd
                + " preversion : " + curVersion + " tgversion : " + tgtVersion;
    }
}
