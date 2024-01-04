package com.softwinner.dragonbox.receiver;


import android.bluetooth.BluetoothDevice;
import android.content.BroadcastReceiver;
import android.content.Context;
import android.content.Intent;
import android.text.TextUtils;
import android.bluetooth.BluetoothHidHost;
import android.bluetooth.BluetoothProfile;
import com.softwinner.dragonbox.platform.BTPairCallback;
import android.util.Log;

public class BluetoothReceiver extends BroadcastReceiver {

    private static final String TAG = "DragonBox-BluetoothReceiver";
    public static final String RCU_NAME = "alibaba";
    private BTPairCallback mCallback;

    public void setBTPairCallback(BTPairCallback btPairCallback){
        this.mCallback = btPairCallback;
    }

    @Override
    public void onReceive(Context context, Intent intent) {
        String action = intent.getAction();
        if(TextUtils.isEmpty(action)) return;

        if(action.equals(BluetoothHidHost.ACTION_CONNECTION_STATE_CHANGED)){
            final BluetoothDevice device = intent.getParcelableExtra(BluetoothDevice.EXTRA_DEVICE);
            int new_state = intent.getIntExtra(BluetoothProfile.EXTRA_STATE,-1);
            int pre_state = intent.getIntExtra(BluetoothProfile.EXTRA_PREVIOUS_STATE,-1);
            String os_mac = intent.getStringExtra("BD_ADDR");
            String name = device.getName();
            Log.d(TAG,"device name: " + name + " new_state: " + new_state + " pre_state: " + pre_state + " type: " + device.getType() +" ,os_mac:"+os_mac);
            if((new_state==2) && (((!TextUtils.isEmpty(name)) && name.equals(RCU_NAME)) ||
                device.getType()==BluetoothDevice.DEVICE_TYPE_LE || device.getType()==BluetoothDevice.DEVICE_TYPE_DUAL)) {
                //connected
                Log.d(TAG,"***connect "+name+" device success***");
                if(mCallback!=null){
                    mCallback.onDeviceConnectedEvent(os_mac);
                }
            } else if((new_state == 0) && (((!TextUtils.isEmpty(name)) && name.equals(RCU_NAME)) ||
                device.getType()==BluetoothDevice.DEVICE_TYPE_LE )){
                //disconnected
                Log.d(TAG,"***device "+name+" disconnect***");
                if(mCallback!=null){
                    mCallback.onDeviceDisConnectEvent();
                }
            }
        }
    }
}