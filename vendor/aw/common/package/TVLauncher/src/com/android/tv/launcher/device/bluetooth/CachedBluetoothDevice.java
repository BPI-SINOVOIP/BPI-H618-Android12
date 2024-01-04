
package com.android.tv.launcher.device.bluetooth;


import android.bluetooth.BluetoothDevice;



public class CachedBluetoothDevice {

    private BluetoothDevice mBluetoothDevice;
    private short mRssi;

    CachedBluetoothDevice(BluetoothDevice dev) {
        mBluetoothDevice = dev;
    }

    public BluetoothDevice getDevice() {
        return mBluetoothDevice;
    }

    public void setRssi(short rssi) {
        mRssi = rssi;
    }

    public short getRssi() {
        return mRssi;
    }

}
