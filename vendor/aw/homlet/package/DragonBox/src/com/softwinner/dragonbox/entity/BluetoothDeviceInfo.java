package com.softwinner.dragonbox.entity;

import android.bluetooth.BluetoothDevice;

public class BluetoothDeviceInfo implements Comparable<BluetoothDeviceInfo>{
    private BluetoothDevice mBuletoothDevice;
    private short RSSI;

    public BluetoothDeviceInfo(BluetoothDevice mBuletoothDevice, short rSSI) {
        super();
        this.mBuletoothDevice = mBuletoothDevice;
        RSSI = rSSI;
    }

    public BluetoothDevice getmBuletoothDevice() {
        return mBuletoothDevice;
    }

    public void setmBuletoothDevice(BluetoothDevice mBuletoothDevice) {
        this.mBuletoothDevice = mBuletoothDevice;
    }

    public short getRSSI() {
        return RSSI;
    }

    public void setRSSI(short rSSI) {
        RSSI = rSSI;
    }

    @Override
    public int compareTo(BluetoothDeviceInfo another) {
        return another.getRSSI() - getRSSI();
    }
}
