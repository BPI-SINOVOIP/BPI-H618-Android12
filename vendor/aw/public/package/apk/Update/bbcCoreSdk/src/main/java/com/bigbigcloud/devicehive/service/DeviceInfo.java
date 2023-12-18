package com.bigbigcloud.devicehive.service;

import android.content.Context;

import java.io.Serializable;

/**
 * Created by Administrator on 2015/12/24.
 */
public abstract class DeviceInfo implements Serializable {
    protected String id; //设备ID
    protected String key;
    protected String name;//设备名称
    protected String mac;//设备mac地址
    protected String ip;//设备ip
    protected String vendor;//设备厂商名，必须与厂商后台注册的设备厂商一致

    protected String model;//设备型号，必须与厂商后台注册的设备型号一致
    protected String firmwareVersion;//设备固件版本号
    protected String romType;

    public DeviceInfo(Context context){
        initDeviceInfo(context);
    }

    public abstract void initDeviceInfo(Context context);

    public String getId() {
        return id;
    }

    public String getKey() {
        return key;
    }

    public String getName() {
        return name;
    }

    public String getMac() {
        return mac;
    }

    public String getVendor() {
        return vendor;
    }

    public String getIp() {
        return ip;
    }

    public String getModel() {
        return model;
    }

    public String getFirmwareVersion() {
        return firmwareVersion;
    }

    public String getRomType() {
        return romType;
    }

    @Override
    public boolean equals(Object o) {
        if (this == o) return true;
        if (o == null || getClass() != o.getClass()) return false;

        DeviceInfo that = (DeviceInfo) o;

        if (!id.equals(that.id)) return false;
        if (key != null ? !key.equals(that.key) : that.key != null) return false;
        if (name != null ? !name.equals(that.name) : that.name != null) return false;
        if (!mac.equals(that.mac)) return false;
        if (!ip.equals(that.ip)) return false;
        if (vendor != null ? !vendor.equals(that.vendor) : that.vendor != null) return false;
        if (model != null ? !model.equals(that.model) : that.model != null) return false;
        return firmwareVersion.equals(that.firmwareVersion);

    }

    @Override
    public int hashCode() {
        int result = id.hashCode();
        result = 31 * result + (key != null ? key.hashCode() : 0);
        result = 31 * result + (name != null ? name.hashCode() : 0);
        result = 31 * result + mac.hashCode();
        result = 31 * result + ip.hashCode();
        result = 31 * result + (vendor != null ? vendor.hashCode() : 0);
        result = 31 * result + (model != null ? model.hashCode() : 0);
        result = 31 * result + firmwareVersion.hashCode();
        return result;
    }
}
