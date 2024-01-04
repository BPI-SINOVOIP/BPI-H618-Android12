package com.softwinner.dragonbox.manager;

import java.util.ArrayList;
import java.util.Collections;
import java.util.List;

import android.bluetooth.BluetoothAdapter;
import android.bluetooth.BluetoothDevice;
import android.content.BroadcastReceiver;
import android.content.Context;
import android.content.Intent;
import android.content.IntentFilter;
import android.os.Handler;
import android.util.Log;

import com.softwinner.dragonbox.entity.BluetoothDeviceInfo;

public class BluetoothManager {
    private static final String TAG = "DragonBox-BluetoothManager";
    private Context mContext;
    private List<BluetoothDeviceInfo> deviceList = new ArrayList<BluetoothDeviceInfo>();

    private OnBluetoothSearchListener mBuletoothSearchListener;
    private IntentFilter mFilter;

    private BroadcastReceiver mBluetoothReceiver = new BroadcastReceiver() {

        public void updateToList(BluetoothDeviceInfo deviceInfo) {
            boolean found = false;
            for (BluetoothDeviceInfo info : deviceList) {
                if (info.getmBuletoothDevice().equals(
                        deviceInfo.getmBuletoothDevice())) {
                    found = true;
                    info.setRSSI(deviceInfo.getRSSI());
                }
            }
            if (!found) {
                deviceList.add(deviceInfo);
            }
        }

        @Override
        public void onReceive(Context context, Intent intent) {

            String action = intent.getAction();
            if (BluetoothAdapter.ACTION_DISCOVERY_STARTED.equals(action)) {
                deviceList.clear();
                if (mBuletoothSearchListener != null) {
                    mBuletoothSearchListener.onSearchStart(deviceList);
                }

            }
            if (BluetoothDevice.ACTION_FOUND.equals(action)) {

                BluetoothDevice device = intent
                        .getParcelableExtra(BluetoothDevice.EXTRA_DEVICE);
                short rssi = intent.getExtras().getShort(
                        BluetoothDevice.EXTRA_RSSI);
                BluetoothDeviceInfo deviceInfo = new BluetoothDeviceInfo(
                        device, rssi);
                updateToList(deviceInfo);
                if (mBuletoothSearchListener != null) {
                    mBuletoothSearchListener.onSearching(deviceList);
                }
                if (device.getBondState() != BluetoothDevice.BOND_BONDED) {
                    // 信号强度。
                }
            } else if (BluetoothAdapter.ACTION_DISCOVERY_FINISHED
                    .equals(action)) {

                if (mBuletoothSearchListener != null) {
                    mBuletoothSearchListener.onSearchEnd(deviceList);
                }

            } else if (BluetoothAdapter.ACTION_STATE_CHANGED.equals(action)) {
            }
            Collections.sort(deviceList);
        }

    };

    public static final int WHAT_START_DISCOVERY = 1;
    public Handler mHandler = new Handler() {
        public void handleMessage(android.os.Message msg) {
            switch (msg.what) {
                case WHAT_START_DISCOVERY:
                    statDiscovery();
                    break;
            }
        };
    };

    public BluetoothManager(Context context) {
        mContext = context;
        mFilter = new IntentFilter();
        mFilter.addAction(BluetoothAdapter.ACTION_DISCOVERY_STARTED);
        mFilter.addAction(BluetoothDevice.ACTION_FOUND);
        mFilter.addAction(BluetoothAdapter.ACTION_DISCOVERY_FINISHED);
    }

    public void statDiscovery() {
        mContext.registerReceiver(mBluetoothReceiver, mFilter);

        mHandler.removeMessages(WHAT_START_DISCOVERY);
        final BluetoothAdapter adapter = BluetoothAdapter.getDefaultAdapter();
        Log.i(TAG,
                "statDiscovery " + 0 + " adapter.getState()="
                        + adapter.getState() + " isDiscovering="
                        + adapter.isDiscovering());
        if (!adapter.isEnabled() || adapter.isDiscovering()
                || adapter.getState() != BluetoothAdapter.STATE_ON) {
            if (!adapter.isEnabled()) {
                adapter.enable();
            }
            Log.i(TAG, "statDiscovery " + 1);

            mHandler.sendEmptyMessageDelayed(WHAT_START_DISCOVERY, 1000);
            return;
        }
        adapter.startDiscovery();
    }

    public void stopDiscovery(){
        mContext.unregisterReceiver(mBluetoothReceiver);
        mHandler.removeMessages(WHAT_START_DISCOVERY);
    }

    public void setOnBluetoothSearchListener(OnBluetoothSearchListener listener) {
        this.mBuletoothSearchListener = listener;
    }

    public interface OnBluetoothSearchListener {
        void onSearchStart(List<BluetoothDeviceInfo> deviceList);

        void onSearching(List<BluetoothDeviceInfo> deviceList);

        void onSearchEnd(List<BluetoothDeviceInfo> deviceList);

    }

}
