package com.android.tv.launcher.device.bluetooth;

import android.app.Activity;
import android.app.FragmentManager;
import android.content.Context;
import android.content.Intent;
import android.content.res.Resources;
import android.os.Bundle;
import android.os.Handler;
import android.os.SystemClock;
import android.text.TextUtils;
import android.util.Pair;
import android.util.Log;

import android.content.BroadcastReceiver;
import android.content.IntentFilter;

import android.bluetooth.BluetoothAdapter;
import android.bluetooth.BluetoothDevice;
import android.bluetooth.BluetoothClass;
import android.bluetooth.BluetoothPan;
import android.bluetooth.BluetoothProfile;
import android.bluetooth.BluetoothDevicePicker;

import android.net.ConnectivityManager;

import android.widget.Toast;
import android.graphics.BitmapFactory;

import com.android.tv.launcher.dialog.SettingsLayoutActivity;
import com.android.tv.launcher.dialog.SettingsLayoutFragment;
import com.android.tv.launcher.dialog.Layout;
import com.android.tv.launcher.dialog.Layout.Header;
import com.android.tv.launcher.dialog.Layout.Action;
import com.android.tv.launcher.dialog.Layout.Status;
import com.android.tv.launcher.dialog.Layout.Static;
import com.android.tv.launcher.dialog.Layout.StringGetter;
import com.android.tv.launcher.dialog.Layout.LayoutGetter;
import com.android.tv.launcher.widget.SettingsToast;
import com.android.tv.launcher.R;

import java.util.ArrayList;
import java.util.Collections;
import java.util.HashMap;
import java.util.List;
import java.util.Set;
import java.lang.ref.WeakReference;
import java.util.concurrent.atomic.AtomicReference;

import static android.net.ConnectivityManager.TETHERING_BLUETOOTH;

public class BluetoothActivity extends SettingsLayoutActivity {
    private static final String TAG = "BluetoothActivity";

    private static final int REQUEST_CODE_ADVANCED_OPTIONS = 1;
    private Resources mRes;

    private final static int LAYOUT_BT_OVERVIEW = 0;
    private final static int LAYOUT_BT_STATUS = 1;
    private final static int LAYOUT_BT_SEARCH = 2;
    private final static int LAYOUT_BT_DEVICE_LIST = 3;
    private final static int LAYOUT_BT_DEVICE_RENAME = 4;
    private final static int LAYOUT_BT_TETHER = 5;

    private int currentLayout = LAYOUT_BT_OVERVIEW;

    private final static int ACTION_ID_BT_STATUS_ON = 10;
    private final static int ACTION_ID_BT_STATUS_OFF = 11;
    private final static int ACTION_ID_BT_SEARCH_ON = 12;
    private final static int ACTION_ID_BT_SEARCH_OFF = 13;

    private final static int ACTION_ID_CREATE_BOND = 14;
    private final static int ACTION_ID_REMOVE_BOND = 15;

    private final static int ACTION_ID_RENAME = 16;

    private final static int ACTION_ID_TETHER_STATUS_ON = 17;
    private final static int ACTION_ID_TETHER_STATUS_OFF = 18;

    private final static String KEY_DEVICE = "bt_device";

    private boolean showDiscoveryEndToast = false;

    private BluetoothAdapter mBluetoothAdapter = null;
    private Handler mHandler = new Handler();
    private ArrayList<CachedBluetoothDevice> mFoundBluetoothDeviceList = null;

    private ArrayList<DeviceStringGetter> mAvaliableDeviceDesStringList = null;

    private AtomicReference<BluetoothPan> mBluetoothPan = new AtomicReference<BluetoothPan>();
    private ConnectivityManager mConnectivityManager;
    private String[] mBluetoothRegexs;
    private boolean bluetoothTetherAvailable;
    private OnStartTetheringCallback mStartTetheringCallback;

    private BluetoothDevice mTargetDevice = null;

    private boolean mNeedAuth = false;
    private String mLaunchPackage = null;
    private String mLaunchClass = null;
    private boolean needPickDevice = false;

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        Log.v(TAG, "onCreate().");
        mRes = getResources();
        
        Intent intent = getIntent();
        String action = intent.getAction();
        if (action != null && action.equals(BluetoothDevicePicker.ACTION_LAUNCH)) {
            Log.v(TAG, "Created for picking device.");
            needPickDevice = true;
            mNeedAuth = intent.getBooleanExtra(BluetoothDevicePicker.EXTRA_NEED_AUTH, false);
            mLaunchPackage = intent.getStringExtra(BluetoothDevicePicker.EXTRA_LAUNCH_PACKAGE);
            mLaunchClass = intent.getStringExtra(BluetoothDevicePicker.EXTRA_LAUNCH_CLASS);
        } else {
            needPickDevice = false;
        }

        mBluetoothAdapter = BluetoothAdapter.getDefaultAdapter();
        //mBluetoothAdapter.setDiscoverableTimeout(30000);
        mFoundBluetoothDeviceList = new ArrayList<CachedBluetoothDevice>();
        mAvaliableDeviceDesStringList = new ArrayList<DeviceStringGetter>();

        mConnectivityManager = (ConnectivityManager) getSystemService(Context.CONNECTIVITY_SERVICE);

        if (mBluetoothAdapter != null) {
            mBluetoothAdapter.getProfileProxy(this, mProfileServiceListener,
                    BluetoothProfile.PAN);
        }

        //mConnectivityManager.getTetheringManager().getTetherableBluetoothRegexs();
        bluetoothTetherAvailable = mBluetoothRegexs.length != 0;
        super.onCreate(savedInstanceState);
    }

    @Override
    public void onStart() {
        super.onStart();

        mStartTetheringCallback = new OnStartTetheringCallback(this);
    }

    @Override
    public void onResume() {
        Log.v(TAG, "onResume().");

        registerDeviceReceiver();

        if (mPairedDeviceListLayout != null)
            mPairedDeviceListLayout.onDeviceListInvalidated();
        if (mAvaliableDeviceListLayout != null)
            mAvaliableDeviceListLayout.onDeviceListInvalidated();

        updateOverView();
        super.onResume();
    }

    @Override
    protected void onPause() {
        Log.v(TAG, "onPause().");

        unRegisterDeviceReceiver();

        super.onPause();
    }

    @Override
    protected void onDestroy() {
        Log.v(TAG, "onDestroy().");

        if (mBluetoothPan.get() != null)
            mBluetoothAdapter.closeProfileProxy(BluetoothProfile.PAN, mBluetoothPan.get());
        super.onDestroy();
    }

    private boolean isBluetoothEnable() {
        return mBluetoothAdapter.isEnabled();
    }

    private boolean isBluetoothCanBeDiscovery() {
        int mode = mBluetoothAdapter.getScanMode();
        if (mode == BluetoothAdapter.SCAN_MODE_CONNECTABLE_DISCOVERABLE)
            return true;
        else
            return false;
    }

    private boolean isBluetoothTetherEnabled() {
        BluetoothPan pan = mBluetoothPan.get();
        if (pan != null && pan.isTetheringOn()) {
            return true;
        } else {
            return false;
        }
    }

    private void startTethering() {
        if (bluetoothTetherAvailable && !isBluetoothTetherEnabled()) {
            mConnectivityManager.startTethering(TETHERING_BLUETOOTH, true, mStartTetheringCallback, mHandler);
        } else {
            Log.e(TAG, "Bluetooth tether is not avaliable!!!");
        }
    }

    private void stopTethering() {
        if (bluetoothTetherAvailable && isBluetoothTetherEnabled()) {
            mConnectivityManager.stopTethering(TETHERING_BLUETOOTH);
        } else {
            Log.e(TAG, "Bluetooth tether is not avaliable!!!");
        }
    }

    private static final class OnStartTetheringCallback extends
            ConnectivityManager.OnStartTetheringCallback {
        final WeakReference<BluetoothActivity> mBluetoothActivity;

        OnStartTetheringCallback(BluetoothActivity settings) {
            mBluetoothActivity = new WeakReference<BluetoothActivity>(settings);
        }

        @Override
        public void onTetheringStarted() {
            Log.v(TAG, "onTetheringStarted().");
            update();
        }

        @Override
        public void onTetheringFailed() {
            Log.v(TAG, "onTetheringFailed().");
            update();
        }

        private void update() {
            BluetoothActivity settings = mBluetoothActivity.get();
            if (settings != null) {
                //settings.updateState();
            }
        }
    }


    private void updateOverView() {
        mBluetoothStateString.refreshView();
        mBluetoothCanBeDiscoveryString.refreshView();
        mBluetoothTetherStatusString.refreshView();
    }

    private void refreshOverView() {
        Layout layout = mBluetoothOverViewLayout.get();
        layout.navigateToRoot();
        final FragmentManager fm = getFragmentManager();
        SettingsLayoutFragment mSettingsLayoutFragment = (SettingsLayoutFragment) fm.findFragmentByTag(
                SettingsLayoutFragment.TAG_LEAN_BACK_DIALOG_FRAGMENT);
        if (mSettingsLayoutFragment != null) {
            mSettingsLayoutFragment.setLayout(layout);
            mSettingsLayoutFragment.onRefreshView();
        }
    }

    private void connectToDevice(BluetoothDevice dev) {
        int majorDeviceClass = dev.getBluetoothClass().getMajorDeviceClass();
        switch (majorDeviceClass) {
            case BluetoothClass.Device.Major.PERIPHERAL:
                (new BluetoothHidConnector(BluetoothActivity.this, dev)).openConnection(mBluetoothAdapter);
            case BluetoothClass.Device.Major.AUDIO_VIDEO:
                (new BluetoothA2dpConnector(BluetoothActivity.this, dev)).openConnection(mBluetoothAdapter);
            default:
                Log.d(TAG, "Unhandle device class: " + majorDeviceClass);
                break;
        }
    }


    private final BroadcastReceiver mReceiver = new BroadcastReceiver() {

        @Override
        public void onReceive(Context context, Intent intent) {
            String action = intent.getAction();
            Log.v(TAG, "mReceiver got " + action);

            if (BluetoothDevice.ACTION_FOUND.equals(action)) {
                BluetoothDevice mDev = intent.getParcelableExtra(BluetoothDevice.EXTRA_DEVICE);
                short rssi = intent.getShortExtra(BluetoothDevice.EXTRA_RSSI, Short.MIN_VALUE);

                Log.v(TAG, "found device: " + mDev.getAddress());

                if (mFoundBluetoothDeviceList.size() > 0) {
                    for (CachedBluetoothDevice dev : mFoundBluetoothDeviceList) {
                        if (dev.getDevice().getAddress().equals(mDev.getAddress())) {
                            return;
                        }
                        if (dev.getDevice().getBondState() == BluetoothDevice.BOND_BONDED) {
                            return;
                        }
                    }
                }

                CachedBluetoothDevice cDev = new CachedBluetoothDevice(mDev);
                cDev.setRssi(rssi);
                mFoundBluetoothDeviceList.add(cDev);

                BluetoothDeviceComparator cp = new BluetoothDeviceComparator();
                Collections.sort(mFoundBluetoothDeviceList, cp);

                mPairedDeviceListLayout.onDeviceListInvalidated();
                mAvaliableDeviceListLayout.onDeviceListInvalidated();
            } else if (BluetoothDevice.ACTION_NAME_CHANGED.equals(action)) {
                BluetoothDevice mDev = intent.getParcelableExtra(BluetoothDevice.EXTRA_DEVICE);

                Log.v(TAG, "device name changed: " + mDev.getAddress());

                for (int i = 0; i < mFoundBluetoothDeviceList.size(); i++) {
                    if (mFoundBluetoothDeviceList.get(i).getDevice().getAddress().equals(mDev.getAddress())) {
                        mFoundBluetoothDeviceList.remove(i);
                        mFoundBluetoothDeviceList.add(i, new CachedBluetoothDevice(mDev));
                    }
                }

                mPairedDeviceListLayout.onDeviceListInvalidated();
                mAvaliableDeviceListLayout.onDeviceListInvalidated();
            } else if (BluetoothAdapter.ACTION_DISCOVERY_STARTED.equals(action)) {
                //Toast.makeText(BluetoothActivity.this, "Discoverying...", Toast.LENGTH_SHORT).show();

                SettingsToast t = new SettingsToast(BluetoothActivity.this, getString(R.string.accessory_bluetooth_discoverying),
                                                    BitmapFactory.decodeResource(mRes, R.drawable.ic_settings_bluetooth));
                t.show();

                showDiscoveryEndToast = true;
                //if (mFoundBluetoothDeviceList != null)
                //    mFoundBluetoothDeviceList.clear();
            } else if (BluetoothAdapter.ACTION_DISCOVERY_FINISHED.equals(action)) {
                //Toast.makeText(BluetoothActivity.this, "Discovery end...", Toast.LENGTH_SHORT).show();

                if (showDiscoveryEndToast) {
                    SettingsToast t = new SettingsToast(BluetoothActivity.this, getString(R.string.accessory_bluetooth_discoverying_end),
                                                    BitmapFactory.decodeResource(mRes, R.drawable.ic_settings_bluetooth));
                    t.show();
                    showDiscoveryEndToast = false;
                }

                //if (mFoundBluetoothDeviceList != null)
                //    mFoundBluetoothDeviceList.clear();
            } else if (BluetoothDevice.ACTION_BOND_STATE_CHANGED.equals(action)) {
                int bondState = intent.getIntExtra(BluetoothDevice.EXTRA_BOND_STATE, BluetoothDevice.ERROR);
                BluetoothDevice mDev = intent.getParcelableExtra(BluetoothDevice.EXTRA_DEVICE);

                if (bondState == BluetoothDevice.BOND_BONDED && needPickDevice) {
                    if (mDev.equals(mTargetDevice)) {
                        mHandler.post(new Runnable() {
                            @Override
                            public void run() {
                                sendDevicePickedIntent(mDev);
                                finish();
                            }
                        });
                    }
                }

                if (bondState == BluetoothDevice.BOND_BONDED) {
                    for (int i = 0; i < mFoundBluetoothDeviceList.size(); i++) {
                        BluetoothDevice dev = mFoundBluetoothDeviceList.get(i).getDevice();
                        if (dev.getAddress().equals(mDev.getAddress())) {
                            mFoundBluetoothDeviceList.remove(i);
                            break;
                        }
                    }
                }

                if ((bondState == BluetoothDevice.BOND_BONDED) && (mDev.equals(mTargetDevice))) {
                    connectToDevice(mDev);
                }

                if (bondState == BluetoothDevice.BOND_NONE) {
                    int reason = intent.getIntExtra(BluetoothDevice.EXTRA_REASON, BluetoothDevice.ERROR);
                    Log.v(TAG, "Bond none reason: " + reason);
                }

                if (bondState == BluetoothDevice.BOND_BONDING) {
                    //mPairedDeviceListLayout.onDeviceListInvalidated();
                    //mAvaliableDeviceListLayout.onDeviceListInvalidated();

                    for (DeviceStringGetter sg : mAvaliableDeviceDesStringList) {
                        if (sg.getDevice().getAddress().equals(mDev.getAddress())) {
                            sg.refreshView();
                            break;
                        }
                    }

                } else {
                    mPairedDeviceListLayout.onDeviceListInvalidated();
                    mAvaliableDeviceListLayout.onDeviceListInvalidated();
                }
            } else if (BluetoothDevice.ACTION_ACL_CONNECTED.equals(action)) {
                mPairedDeviceListLayout.onDeviceListInvalidated();
            } else if (BluetoothDevice.ACTION_ACL_DISCONNECTED.equals(action)) {
                mPairedDeviceListLayout.onDeviceListInvalidated();
            } else if (BluetoothAdapter.ACTION_STATE_CHANGED.equals(action)) {
                //int state = intent.getIntExtra(BluetoothAdapter.EXTRA_STATE, BluetoothAdapter.STATE_OFF);

                mBluetoothStateString.refreshView();
                mHandler.post(new Runnable() {
                    @Override
                    public void run() {
                        refreshOverView();
                    }
                });
            } else if (BluetoothAdapter.ACTION_SCAN_MODE_CHANGED.equals(action)) {

                mBluetoothCanBeDiscoveryString.refreshView();
            }
        }
    };

    private void registerDeviceReceiver() {
        IntentFilter filter = new IntentFilter();
        filter.addAction(BluetoothDevice.ACTION_FOUND);
        filter.addAction(BluetoothDevice.ACTION_NAME_CHANGED);
        filter.addAction(BluetoothAdapter.ACTION_DISCOVERY_STARTED);
        filter.addAction(BluetoothAdapter.ACTION_DISCOVERY_FINISHED);
        filter.addAction(BluetoothDevice.ACTION_BOND_STATE_CHANGED);
        filter.addAction(BluetoothDevice.ACTION_ACL_CONNECTED);
        filter.addAction(BluetoothDevice.ACTION_ACL_DISCONNECTED);
        filter.addAction(BluetoothAdapter.ACTION_STATE_CHANGED);
        filter.addAction(BluetoothAdapter.ACTION_SCAN_MODE_CHANGED);
        registerReceiver(mReceiver, filter);
    }

    private void unRegisterDeviceReceiver() {
        unregisterReceiver(mReceiver);
    }






    private void sendDevicePickedIntent(BluetoothDevice device) {
        Intent intent = new Intent(BluetoothDevicePicker.ACTION_DEVICE_SELECTED);
        intent.putExtra(BluetoothDevice.EXTRA_DEVICE, device);
        if (mLaunchPackage != null && mLaunchClass != null) {
            intent.setClassName(mLaunchPackage, mLaunchClass);
        }
        sendBroadcast(intent);
    }



    StringGetter mBluetoothStateString = new StringGetter() {

        @Override
        public String get() {
            int state = mBluetoothAdapter.getState();

            if (state == BluetoothAdapter.STATE_OFF)
                return mRes.getString(R.string.accessory_bluetooth_status_off);
            else if (state == BluetoothAdapter.STATE_TURNING_ON)
                return mRes.getString(R.string.accessory_bluetooth_status_oning);
            else if (state == BluetoothAdapter.STATE_ON)
                return mRes.getString(R.string.accessory_bluetooth_status_on);
            else if (state == BluetoothAdapter.STATE_TURNING_OFF)
                return mRes.getString(R.string.accessory_bluetooth_status_offing);
            else
                return mRes.getString(R.string.accessory_bluetooth_status_off);
        }

        @Override
        public void refreshView() {
            super.refreshView();
        }
    };



    StringGetter mBluetoothCanBeDiscoveryString = new StringGetter() {

        @Override
        public String get() {
            if (isBluetoothCanBeDiscovery())
                return mRes.getString(R.string.accessory_bluetooth_open_search_on);
            else
                return mRes.getString(R.string.accessory_bluetooth_open_search_off);
        }

        @Override
        public void refreshView() {
            super.refreshView();
        }
    };

    StringGetter mBluetoothDeviceListString = new StringGetter() {

        @Override
        public String get() {
            return mRes.getString(R.string.accessory_bluetooth_device_list_tip);
        }
    };

    StringGetter mBluetoothDeviceNameString = new StringGetter() {

        @Override
        public String get() {
            return mBluetoothAdapter.getName();
        }
    };

    StringGetter mBluetoothTetherStatusString = new StringGetter() {

        @Override
        public String get() {
            if (isBluetoothTetherEnabled())
                return mRes.getString(R.string.accessory_bluetooth_status_on);
            else
                return mRes.getString(R.string.accessory_bluetooth_status_off);
        }

        @Override
        public void refreshView() {
            super.refreshView();
        }
    };


    @Override
    protected void onActivityResult(int requestCode, int resultCode, Intent data) {
        if (requestCode == REQUEST_CODE_ADVANCED_OPTIONS && resultCode == RESULT_OK) {
            //TODO make sure view reflects model deltas
        } else {
            super.onActivityResult(requestCode, resultCode, data);
        }
    }

    @Override
    public void onBackPressed() {
    	Log.d(TAG, "onBackPressed in");

        switch (currentLayout) {
            case LAYOUT_BT_STATUS:
            case LAYOUT_BT_SEARCH:
            case LAYOUT_BT_DEVICE_LIST:
            case LAYOUT_BT_DEVICE_RENAME:
            case LAYOUT_BT_TETHER:
                currentLayout = LAYOUT_BT_OVERVIEW;
                break;
        }

        if (currentLayout == LAYOUT_BT_OVERVIEW) {
            if (mBluetoothAdapter != null) {
                if (mBluetoothAdapter.isDiscovering())
                    mBluetoothAdapter.cancelDiscovery();
            }
        }

        super.onBackPressed();
    }


    @Override
    public void onActionFocused(Layout.LayoutRow item) {
        int resId = item.getContentIconRes();
        if (resId != 0) {
            setIcon(resId);
        }
    }

    @Override
    public void onActionClicked(Action action) {
        Bundle bundle;
        BluetoothDevice dev;

        switch (action.getId()) {
            case Action.ACTION_INTENT:
                startActivityForResult(action.getIntent(), REQUEST_CODE_ADVANCED_OPTIONS);
                break;
            case ACTION_ID_BT_STATUS_ON:
                if (!isBluetoothEnable())
                    mBluetoothAdapter.enable();
                onBackPressed();
                break;
            case ACTION_ID_BT_STATUS_OFF:
                if (isBluetoothEnable())
                    mBluetoothAdapter.disable();
                onBackPressed();
                break;
            case ACTION_ID_BT_SEARCH_ON:
                if (!isBluetoothCanBeDiscovery())
                    mBluetoothAdapter.setScanMode(BluetoothAdapter.SCAN_MODE_CONNECTABLE_DISCOVERABLE);
                onBackPressed();
                break;
            case ACTION_ID_BT_SEARCH_OFF:
                if (isBluetoothCanBeDiscovery())
                    mBluetoothAdapter.setScanMode(BluetoothAdapter.SCAN_MODE_CONNECTABLE);
                onBackPressed();
                break;
            case ACTION_ID_CREATE_BOND:
                bundle = action.getData();
                dev = bundle.getParcelable(KEY_DEVICE);
                if (mBluetoothAdapter.isDiscovering())
                    mBluetoothAdapter.cancelDiscovery();

                if (needPickDevice) {
                    if (!mNeedAuth) {
                        sendDevicePickedIntent(dev);
                        finish();
                        return;
                    }
                }

                dev.createBond();
                mTargetDevice = dev;
                break;
            case ACTION_ID_REMOVE_BOND:
                bundle = action.getData();
                dev = bundle.getParcelable(KEY_DEVICE);
                //dev.removeBond();

                if (needPickDevice) {
                    sendDevicePickedIntent(dev);
                    finish();
                    return;
                }

                Intent i = new Intent();
                i.putExtra(BluetoothDevice.EXTRA_DEVICE, dev);
                i.setAction(BluetoothUnPairActivity.ACTION_UNPAIR_REQUEST);
                startActivity(i);

                break;
            case ACTION_ID_RENAME:
                //new BluetoothNameDialogFragment().show(getFragmentManager(), "rename device");
                
                Intent it = new Intent();
                it.setAction(BluetoothRenameActivity.ACTION_RENAME_REQUEST);
                //it.setAction(BluetoothFileReceiveActivity.ACTION_FILE_RECEIVE_CONFIRM);
                startActivity(it);
                
                //onBackPressed();
                break;
            case ACTION_ID_TETHER_STATUS_ON:
                startTethering();
                onBackPressed();
                break;
            case ACTION_ID_TETHER_STATUS_OFF:
                stopTethering();
                onBackPressed();
                break;
        }
    }








    private class DeviceStringGetter extends StringGetter {
        private BluetoothDevice mDev;

        DeviceStringGetter(BluetoothDevice dev) {
            mDev = dev;
        }

        public BluetoothDevice getDevice() {
            return mDev;
        }

        public String get() {
            BluetoothDevice dev = getDevice();
            if (dev != null) {
                if (dev.getBondState() == BluetoothDevice.BOND_BONDING) {
                    return mRes.getString(R.string.accessory_state_pairing);
                } else {
                    return dev.getAddress();
                }
            } else {
                return " ";
            }
        }

        @Override
        public void refreshView() {
            super.refreshView();
        }
    }

/*
    DeviceStringGetter mDeviceDesStringGetter = new DeviceStringGetter() {

        public String get() {
            BluetoothDevice dev = getDevice();
            if (dev != null) {
                if (dev.getBondState() == BluetoothDevice.BOND_BONDING) {
                    return mRes.getString(R.string.accessory_state_pairing);
                } else {
                    return dev.getAddress();
                }
            } else {
                return " ";
            }
        }
    };
*/

    public int getHidClassDrawable(BluetoothClass btClass) {
        switch (btClass.getDeviceClass()) {
            case BluetoothClass.Device.PERIPHERAL_KEYBOARD:
            case BluetoothClass.Device.PERIPHERAL_KEYBOARD_POINTING:
                return R.drawable.ic_lockscreen_ime;
            case BluetoothClass.Device.PERIPHERAL_POINTING:
                return R.drawable.ic_bt_pointing_hid;
            default:
                return R.drawable.ic_bt_misc_hid;
        }
    }

    private int getBtClassDrawableId(BluetoothDevice dev) {
        BluetoothClass btClass = dev.getBluetoothClass();

        if (btClass != null) {
            switch (btClass.getMajorDeviceClass()) {
                case BluetoothClass.Device.Major.COMPUTER:
                    return R.drawable.ic_bt_laptop;

                case BluetoothClass.Device.Major.PHONE:
                    return R.drawable.ic_bt_cellphone;

                case BluetoothClass.Device.Major.PERIPHERAL:
                    return getHidClassDrawable(btClass);

                case BluetoothClass.Device.Major.IMAGING:
                    return R.drawable.ic_bt_imaging;

                default:
                    // unrecognized device class; continue
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

        Log.w(TAG, "mBtClass is null");

        return R.drawable.ic_settings_bluetooth;
    }




    private final PairedDeviceListLayout mPairedDeviceListLayout = new PairedDeviceListLayout();

    private class PairedDeviceListLayout extends LayoutGetter {

        private final Runnable mRefreshViewRunnable = new Runnable() {
            @Override
            public void run() {
                
                refreshView();
            }
        };

        @Override
        public Layout get() {
            mHandler.removeCallbacks(mRefreshViewRunnable);
            return getPairedDevices();
        }

        public void onDeviceListInvalidated() {
            mHandler.removeCallbacks(mRefreshViewRunnable);
            mHandler.post(mRefreshViewRunnable);
        }


        private Layout getPairedDevices() {
            Layout layout = new Layout();
            Set<BluetoothDevice> devSet = mBluetoothAdapter.getBondedDevices();
            String name;
            boolean isConnected;
            String connectedString = mRes.getString(R.string.accessory_connected);

            if (devSet != null) {
                for (BluetoothDevice dev : devSet) {
                    name = dev.getAlias();
                    isConnected = dev.isConnected();

                    Bundle bundle = new Bundle();
                    bundle.putParcelable(KEY_DEVICE, dev);

                    if (TextUtils.isEmpty(name)) {
                        layout.add(new Action.Builder(mRes, ACTION_ID_REMOVE_BOND)
                                        .description(isConnected?connectedString:dev.getAddress())
                                        .data(bundle)
                                        .icon(getBtClassDrawableId(dev))
                                        .build());
                    } else {
                        layout.add(new Action.Builder(mRes, ACTION_ID_REMOVE_BOND)
                                        .title(name)
                                        .description(isConnected?connectedString:dev.getAddress())
                                        .data(bundle)
                                        .icon(getBtClassDrawableId(dev))
                                        .build());
                    }
                }
            } else {
                layout.add(new Action.Builder(mRes, 0)
                       .title(R.string.accessibility_none).build());
            }

            return layout;
        }

    };


    private final AvaliableDeviceListLayout mAvaliableDeviceListLayout = new AvaliableDeviceListLayout();

    private class AvaliableDeviceListLayout extends LayoutGetter {

        private final Runnable mRefreshViewRunnable = new Runnable() {
            @Override
            public void run() {
                
                refreshView();
            }
        };

        @Override
        public Layout get() {
            mHandler.removeCallbacks(mRefreshViewRunnable);
            return getAvaliableDevices();
        }

        public void onDeviceListInvalidated() {
            mHandler.removeCallbacks(mRefreshViewRunnable);
            mHandler.post(mRefreshViewRunnable);
        }


        private Layout getAvaliableDevices() {
            Layout layout = new Layout();
            String name;
            int bondState;
            boolean isBonding;
            BluetoothDevice dev;
            String bondingString = mRes.getString(R.string.accessory_state_pairing);
            
            mAvaliableDeviceDesStringList.clear();

            if (mFoundBluetoothDeviceList.size() > 0) {
                for (CachedBluetoothDevice d : mFoundBluetoothDeviceList) {
                    dev = d.getDevice();
                    isBonding = false;
                    name = dev.getAlias();
                    bondState = dev.getBondState();

                    if (bondState == BluetoothDevice.BOND_BONDING)
                        isBonding = true;


                    Bundle bundle = new Bundle();
                    bundle.putParcelable(KEY_DEVICE, dev);

                    DeviceStringGetter mDeviceDesStringGetter = new DeviceStringGetter(dev);
                    mAvaliableDeviceDesStringList.add(mDeviceDesStringGetter);

                    if (TextUtils.isEmpty(name)) {
                        layout.add(new Action.Builder(mRes, ACTION_ID_CREATE_BOND)
                                        //.description(isBonding?bondingString:(dev.getAddress()))
                                        .description(mDeviceDesStringGetter)
                                        .data(bundle)
                                        .icon(getBtClassDrawableId(dev))
                                        .build());

                    } else {
                        layout.add(new Action.Builder(mRes, ACTION_ID_CREATE_BOND)
                                        .title(name)
                                        //.description(isBonding?bondingString:(dev.getAddress()))
                                        .description(mDeviceDesStringGetter)
                                        .data(bundle)
                                        .icon(getBtClassDrawableId(dev))
                                        .build());
                    }
                }
            } else {
                layout.add(new Action.Builder(mRes, 0)
                        .title("")
                        .build());
            }

            return layout;
        }

    };



    private final Runnable mUpdateOverViewRunnable = new Runnable() {
        @Override
        public void run() {
            updateOverView();
        }
    };

    private BluetoothProfile.ServiceListener mProfileServiceListener =
            new BluetoothProfile.ServiceListener() {
        public void onServiceConnected(int profile, BluetoothProfile proxy) {
            mBluetoothPan.set((BluetoothPan) proxy);
            mHandler.post(mUpdateOverViewRunnable);
        }
        public void onServiceDisconnected(int profile) {
            mBluetoothPan.set(null);
        }
    };




    LayoutGetter mBluetoothStatusLayout = new LayoutGetter() {
        public Layout get() {

            Log.v(TAG, "mBluetoothStatusLayout in");

            currentLayout = LAYOUT_BT_STATUS;

            return new Layout()
                .add(new Action.Builder(mRes, ACTION_ID_BT_STATUS_ON)
                        .title(R.string.on)
                        .checked(isBluetoothEnable())
                        .build())
                .add(new Action.Builder(mRes, ACTION_ID_BT_STATUS_OFF)
                        .title(R.string.off)
                        .checked(!isBluetoothEnable())
                        .build());
        }
    };

    LayoutGetter mBluetoothOpenSearchLayout = new LayoutGetter() {
        public Layout get() {

            Log.v(TAG, "mBluetoothOpenSearchLayout in");

            currentLayout = LAYOUT_BT_SEARCH;

            return new Layout()
                .add(new Action.Builder(mRes, ACTION_ID_BT_SEARCH_ON)
                        .title(R.string.on)
                        .checked(isBluetoothCanBeDiscovery())
                        .build())
                .add(new Action.Builder(mRes, ACTION_ID_BT_SEARCH_OFF)
                        .title(R.string.off)
                        .checked(!isBluetoothCanBeDiscovery())
                        .build());
        }
    };

    LayoutGetter mBluetoothDeviceListLayout = new LayoutGetter() {
        public Layout get() {

            Log.v(TAG, "mBluetoothDeviceListLayout in");
            if ((mBluetoothAdapter != null) && (currentLayout == LAYOUT_BT_OVERVIEW)) {
                if (!mBluetoothAdapter.isDiscovering())
                    mBluetoothAdapter.startDiscovery();
            }

            if (currentLayout == LAYOUT_BT_OVERVIEW) {
                if (mFoundBluetoothDeviceList != null) {
                    Log.v(TAG, "mFoundBluetoothDeviceList is clear.");
                    mFoundBluetoothDeviceList.clear();
                }
            }

            currentLayout = LAYOUT_BT_DEVICE_LIST;

            return new Layout()
                .add(new Static.Builder(mRes)
                        .title(R.string.accessory_bluetooth_device_paired)
                        .build())
                .add(mPairedDeviceListLayout)
                .add(new Static.Builder(mRes)
                        .title(R.string.accessory_bluetooth_device_avaliable)
                        .build())
                .add(mAvaliableDeviceListLayout);
        }
    };



    LayoutGetter mBluetoothDeviceReNameLayout = new LayoutGetter() {
        public Layout get() {

            Log.v(TAG, "mBluetoothDeviceReNameLayout in");

            currentLayout = LAYOUT_BT_DEVICE_RENAME;

            return new Layout()
                .add(new Action.Builder(mRes, ACTION_ID_RENAME)
                        .title(R.string.accessory_rename)
                        .checked(true)
                        .defaultSelection()
                        //.description(mBluetoothAdapter.getName())
                        .build());
        }
    };

    LayoutGetter mBluetoothTetherStatusLayout = new LayoutGetter() {
        public Layout get() {

            Log.v(TAG, "mBluetoothTetherStatusLayout in");

            currentLayout = LAYOUT_BT_TETHER;

            return new Layout()
                .add(new Action.Builder(mRes, ACTION_ID_TETHER_STATUS_ON)
                        .title(R.string.on)
                        .checked(isBluetoothTetherEnabled())
                        .build())
                .add(new Action.Builder(mRes, ACTION_ID_TETHER_STATUS_OFF)
                        .title(R.string.off)
                        .checked(!isBluetoothTetherEnabled())
                        .build());
        }
    };









    LayoutGetter mBluetoothOverViewLayout = new LayoutGetter() {

        public Layout get() {

            BluetoothAdapter adapter = BluetoothAdapter.getDefaultAdapter();

            if (adapter != null && adapter.isEnabled()) {
                return new Layout()
                    .breadcrumb(getString(R.string.header_category_device))
                    .add(new Header.Builder(mRes)
                            .icon(R.drawable.ic_settings_bluetooth)
                            .title(R.string.accessory_bluetooth)
                            .description(R.string.accessory_bluetooth_devices_list)
                            .build()
                        .add(new Header.Builder(mRes)
                                .title(R.string.accessory_bluetooth_status)
                                .contentIconRes(R.drawable.ic_settings_bluetooth)
                                .description(mBluetoothStateString)
                                .build()
                            .add(mBluetoothStatusLayout))
                        .add(new Header.Builder(mRes)
                                .title(R.string.accessory_bluetooth_open_search)
                                .contentIconRes(R.drawable.ic_settings_bluetooth)
                                .description(mBluetoothCanBeDiscoveryString)
                                .build()
                            .add(mBluetoothOpenSearchLayout))
                        .add(new Header.Builder(mRes)
                                .title(R.string.accessory_bluetooth_devices_list)
                                .contentIconRes(R.drawable.ic_settings_bluetooth)
                                .description(mBluetoothDeviceListString)
                                .build()
                            .add(mBluetoothDeviceListLayout))
                        .add(new Header.Builder(mRes)
                                .title(R.string.device_name)
                                .contentIconRes(R.drawable.ic_settings_bluetooth)
                                .description(mBluetoothDeviceNameString)
                                .build()
                            .add(mBluetoothDeviceReNameLayout))
                        .add(new Header.Builder(mRes)
                                .title(R.string.accessory_bluetooth_tether)
                                .contentIconRes(R.drawable.ic_settings_bluetooth)
                                .description(mBluetoothTetherStatusString)
                                .build()
                            .add(mBluetoothTetherStatusLayout))
                    );
            } else {
                return new Layout()
                    .breadcrumb(getString(R.string.header_category_device))
                    .add(new Header.Builder(mRes)
                            .icon(R.drawable.ic_settings_bluetooth)
                            .title(R.string.accessory_bluetooth)
                            .description(R.string.accessory_bluetooth_devices_list)
                            .build()
                        .add(new Header.Builder(mRes)
                                .title(R.string.accessory_bluetooth_status)
                                .contentIconRes(R.drawable.ic_settings_bluetooth)
                                .description(mBluetoothStateString)
                                .build()
                            .add(mBluetoothStatusLayout))
                    );
            }
        }
    };


    @Override
    public Layout createLayout() {
        Log.v(TAG, "createLayout()");

        currentLayout = LAYOUT_BT_OVERVIEW;

        return mBluetoothOverViewLayout.get();
    }


/*
    @Override
    public Layout createLayout() {
        Log.v(TAG, "createLayout()");

        currentLayout = LAYOUT_BT_OVERVIEW;

        return new Layout()
                .breadcrumb(getString(R.string.header_category_device))
                .add(new Header.Builder(mRes)
                        .icon(R.drawable.ic_settings_bluetooth)
                        .title(R.string.accessory_bluetooth)
                        .description(R.string.accessory_bluetooth_devices_list)
                        .build()
                    .add(new Header.Builder(mRes)
                            .title(R.string.accessory_bluetooth_status)
                            .contentIconRes(R.drawable.ic_settings_bluetooth)
                            .description(mBluetoothEnabledString)
                            .build()
                        .add(mBluetoothStatusLayout))
                    .add(new Header.Builder(mRes)
                            .title(R.string.accessory_bluetooth_open_search)
                            .contentIconRes(R.drawable.ic_settings_bluetooth)
                            .description(mBluetoothCanBeDiscoveryString)
                            .build()
                        .add(mBluetoothOpenSearchLayout))
                    .add(new Header.Builder(mRes)
                            .title(R.string.accessory_bluetooth_devices_list)
                            .contentIconRes(R.drawable.ic_settings_bluetooth)
                            .description(mBluetoothDeviceListString)
                            .build()
                        .add(mBluetoothDeviceListLayout))
                );

    }
*/
}
