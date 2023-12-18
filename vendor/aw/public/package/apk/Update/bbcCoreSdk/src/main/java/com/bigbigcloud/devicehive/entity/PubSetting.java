package com.bigbigcloud.devicehive.entity;

import java.io.Serializable;

/**
 * Created by Thomas.yang on 2016/7/24.
 */
public class PubSetting implements Serializable {
    private int update_type;// 1 静默升级; 2 用户确认升级; 3 强制升级;
    private int pub_area;
    private int notify_type;
    private int only_wifi;
    private String normal_date_start;
    private String normal_date_end;

    public String getNormal_date_end() {
        return normal_date_end;
    }

    public String getNormal_date_start() {
        return normal_date_start;
    }

    public int getNotify_type() {
        return notify_type;
    }

    public int getOnly_wifi() {
        return only_wifi;
    }

    public int getPub_area() {
        return pub_area;
    }

    public int getUpdate_type() {
        return update_type;
    }
}
