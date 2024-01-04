/*
 * Copyright (C) 2011 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

package com.softwinner.tvdsetting.net.bluetooth;

import android.bluetooth.BluetoothAdapter;
import android.bluetooth.BluetoothDevice;
import android.bluetooth.BluetoothClass;
import android.bluetooth.BluetoothProfile;
import android.os.Bundle;
import android.preference.Preference;
import android.preference.PreferenceCategory;
import android.preference.PreferenceGroup;
import android.preference.PreferenceScreen;
import android.util.Log;

import java.util.Collection;
import java.util.WeakHashMap;
import android.app.Activity;
import java.util.ArrayList;
import android.content.Context;
import android.text.TextUtils;
import com.softwinner.tvdsetting.R;
import java.util.List;
import android.content.DialogInterface;
import android.text.Html;
import android.app.AlertDialog;
import android.preference.PreferenceActivity;
import android.os.Handler;
import android.os.Message;

public class BluetoothListUtils extends Activity implements BluetoothCallback {

    private static final String TAG = "BluetoothListUtils";

    LocalBluetoothAdapter mLocalAdapter;
    LocalBluetoothManager mLocalManager;

    private static final int RESCAN_TIME_MS = 5000;//5s
    private static final int MESSAGE_RESCAN = 0;
    
    private CachedBluetoothDevice mSelectedDevice;
    private Handler mHandler = new Handler() {
        @Override  
        public void handleMessage(android.os.Message msg) {
            if(msg.what == MESSAGE_RESCAN) {
                if(isBusy()) {
                    onScanningStateChanged(false);
                } else {
                    mLocalAdapter.startScanning(true);
                }
            }
        }; 
    };
    
    private boolean isBusy() {
        if(mSelectedDevice != null) {
            return mSelectedDevice.isBusy();
        }
        return false;
    }
    
    ArrayList<BluetoothDeviceUtils> mDeviceListGroup = new ArrayList();

    public void setDeviceListGroup(ArrayList<BluetoothDeviceUtils> listName) {
        mDeviceListGroup = listName;
    }

    final WeakHashMap<CachedBluetoothDevice, BluetoothDeviceUtils> mDeviceMap =
            new WeakHashMap<CachedBluetoothDevice, BluetoothDeviceUtils>();
    private AlertDialog mDisconnectDialog;
    
    public void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);

        mLocalManager = LocalBluetoothManager.getInstance(this);
        if (mLocalManager == null) {
            Log.e(TAG, "Bluetooth is not supported on this device");
            return;
        }
        mLocalAdapter = mLocalManager.getBluetoothAdapter();

    }

    @Override
    public void onResume() {
        super.onResume();
        if (mLocalManager == null) return;
        mLocalManager.getEventManager().registerCallback(this);
        addCachedDevices();
        mLocalAdapter.startScanning(true);
    }

    @Override
    public void onPause() {
        super.onPause();
        if (mLocalManager == null) return;
        mLocalManager.getEventManager().unregisterCallback(this);
    }

    @Override
    public void onDestroy() {
        super.onDestroy();
        removeAllDevices();
    }
    
    void removeAllDevices() {
        mLocalAdapter.stopScanning();
        mDeviceMap.clear();
        mDeviceListGroup.clear();
    }
    
    void addCachedDevices() {
        Collection<CachedBluetoothDevice> cachedDevices =
                mLocalManager.getCachedDeviceManager().getCachedDevicesCopy();
        for (CachedBluetoothDevice cachedDevice : cachedDevices) {
            onDeviceAdded(cachedDevice);
        }
    }


    public void onDeviceAdded(CachedBluetoothDevice cachedDevice) {
        if (mDeviceMap.get(cachedDevice) != null) {
            return;
        }
        if (mLocalAdapter.getBluetoothState() != BluetoothAdapter.STATE_ON) return;
            createBluetoothDeviceUtils(cachedDevice);
//        String name = cachedDevice.getName();
//        mDeviceListGroup.add(name);
//        mDeviceMap.put(cachedDevice, name);
//        updateDeviceView(name);
     }

    void createBluetoothDeviceUtils(CachedBluetoothDevice cachedDevice) {
        BluetoothDeviceUtils mDevice = new BluetoothDeviceUtils(this, cachedDevice);

//        initDevicePreference(preference);
        mDeviceListGroup.add(mDevice);
        mDeviceMap.put(cachedDevice, mDevice);
    }
    
    public void updateDeviceView(BluetoothDeviceUtils mDevice){
    }
    
    public void onDeviceDeleted(CachedBluetoothDevice cachedDevice) {
        BluetoothDeviceUtils mDevice = mDeviceMap.remove(cachedDevice);
        if(mDevice != null) {
            mDeviceListGroup.remove(mDevice);
            updateDeviceView(mDevice);
        }
    }
    
    public void onScanningStateChanged(boolean started) {
        if(!started) {
            Message reScanMsg = mHandler.obtainMessage(MESSAGE_RESCAN);
            mHandler.sendMessageDelayed(reScanMsg, RESCAN_TIME_MS);
        }
    }


    public void onBluetoothStateChanged(int bluetoothState) {

    }
    public void onDeviceBondStateChanged(CachedBluetoothDevice cachedDevice, int bondState) {

    }
    
    public class BluetoothDeviceUtils implements CachedBluetoothDevice.Callback {
        
        private final CachedBluetoothDevice mCachedDevice;
        private Context mContext;
        private int summaryResId;
        private int iconResId;
        
        public boolean isBonded() {
            return mCachedDevice.getBondState() == BluetoothDevice.BOND_BONDED;
        }
        
        public void unpairDevice() {
            mCachedDevice.unpair();
        }
    
        public String getName() {
            return mCachedDevice.getName();
        }
        
        public int getSummary() {
            return summaryResId;
        }
        
        public int getIcon() {
            return iconResId;
        }
        
        public BluetoothDeviceUtils(Context context, CachedBluetoothDevice cachedDevice) {
            mContext = context;
            mCachedDevice = cachedDevice;
            mCachedDevice.registerCallback(this);
            onDeviceAttributesChanged();
        }
        
        public void onDeviceAttributesChanged() {
            summaryResId = getConnectionSummary();
            iconResId = getBtClassDrawable();
            updateDeviceView(this);
        }

        public void onClicked() {
            mSelectedDevice = mCachedDevice;
            int bondState = mCachedDevice.getBondState();
            mLocalAdapter.stopScanning();
            if (mCachedDevice.isConnected()) {
                askDisconnect();
            } else if (bondState == BluetoothDevice.BOND_BONDED) {
                mCachedDevice.connect(true);
            } else if (bondState == BluetoothDevice.BOND_NONE) {
                pair();
            }
        }
        
        private void askDisconnect() {
            String name = mCachedDevice.getName();
            if (TextUtils.isEmpty(name)) {
                name = mContext.getString(R.string.bluetooth_device);
            }
            String message = mContext.getString(R.string.bluetooth_disconnect_all_profiles, name);
            String title = mContext.getString(R.string.bluetooth_disconnect_title);
    
            DialogInterface.OnClickListener disconnectListener = new DialogInterface.OnClickListener() {
                public void onClick(DialogInterface dialog, int which) {
                    mCachedDevice.disconnect();
                }
            };
    
            mDisconnectDialog = Utils.showDisconnectDialog(mContext,
                    mDisconnectDialog, disconnectListener, title, Html.fromHtml(message));
        }
        
        private void pair() {
            if (!mCachedDevice.startPairing()) {
                Utils.showError(mContext, mCachedDevice.getName(),
                        R.string.bluetooth_pairing_error_message);
            }
        }

        private int getConnectionSummary() {
            final CachedBluetoothDevice cachedDevice = mCachedDevice;
    
            boolean profileConnected = false;       // at least one profile is connected
            boolean a2dpNotConnected = false;       // A2DP is preferred but not connected
            boolean headsetNotConnected = false;    // Headset is preferred but not connected
    
            for (LocalBluetoothProfile profile : cachedDevice.getProfiles()) {
                int connectionStatus = cachedDevice.getProfileConnectionState(profile);
    
                switch (connectionStatus) {
                    case BluetoothProfile.STATE_CONNECTING:
                    case BluetoothProfile.STATE_DISCONNECTING:
                        return Utils.getConnectionStateSummary(connectionStatus);
    
                    case BluetoothProfile.STATE_CONNECTED:
                        profileConnected = true;
                        break;
    
                    case BluetoothProfile.STATE_DISCONNECTED:
                        if (profile.isProfileReady() && profile.isPreferred(cachedDevice.getDevice())) {
                            if (profile instanceof A2dpProfile) {
                                a2dpNotConnected = true;
                            } else if (profile instanceof HeadsetProfile) {
                                headsetNotConnected = true;
                            }
                        }
                        break;
                }
            }
    
            if (profileConnected) {
                if (a2dpNotConnected && headsetNotConnected) {
                    return R.string.bluetooth_connected_no_headset_no_a2dp;
                } else if (a2dpNotConnected) {
                    return R.string.bluetooth_connected_no_a2dp;
                } else if (headsetNotConnected) {
                    return R.string.bluetooth_connected_no_headset;
                } else {
                    return R.string.bluetooth_connected;
                }
            }
    
            switch (cachedDevice.getBondState()) {
                case BluetoothDevice.BOND_BONDING:
                    return R.string.bluetooth_pairing;
    
                case BluetoothDevice.BOND_BONDED:
                    return R.string.bluetooth_paired;
                case BluetoothDevice.BOND_NONE:
                default:
                    return R.string.bluetooth_unpaired;
            }
        }
    
        private int getBtClassDrawable() {
            BluetoothClass btClass = mCachedDevice.getBtClass();
            if (btClass != null) {
                switch (btClass.getMajorDeviceClass()) {
                    case BluetoothClass.Device.Major.COMPUTER:
                        return R.drawable.ic_bt_laptop;
    
                    case BluetoothClass.Device.Major.PHONE:
                        return R.drawable.ic_bt_cellphone;
    
                    case BluetoothClass.Device.Major.PERIPHERAL:
                        return HidProfile.getHidClassDrawable(btClass);
    
                    case BluetoothClass.Device.Major.IMAGING:
                        return R.drawable.ic_bt_imaging;
    
                    default:
                        // unrecognized device class; continue
                }
            } else {
                Log.w(TAG, "mBtClass is null");
            }
    
            List<LocalBluetoothProfile> profiles = mCachedDevice.getProfiles();
            for (LocalBluetoothProfile profile : profiles) {
                int resId = profile.getDrawableResource(btClass);
                if (resId != 0) {
                    return resId;
                }
            }
            if (btClass != null) {
                if (btClass.doesClassMatch(BluetoothClass.PROFILE_A2DP)) {
                    return R.drawable.ic_bt_headphones_a2dp;
    
                }
                if (btClass.doesClassMatch(BluetoothClass.PROFILE_HEADSET)) {
                    return R.drawable.ic_bt_headset_hfp;
                }
            }
            return 0;
        }
    }
}
