/*
 * Copyright (c) 2011-2013, Qualcomm Technologies, Inc. All Rights Reserved.
 * Qualcomm Technologies Proprietary and Confidential.
 */
package com.allwinnertech.socs;


import java.util.ArrayList;
import java.util.List;
import java.util.Set;

import android.app.Activity;
import android.app.AlertDialog;
import android.bluetooth.BluetoothAdapter;
import android.bluetooth.BluetoothDevice;
import android.content.BroadcastReceiver;
import android.content.Context;
import android.content.DialogInterface;
import android.content.Intent;
import android.content.IntentFilter;
import android.os.Bundle;
import android.os.CountDownTimer;
import android.os.IBinder;
import android.os.ServiceManager;
import android.text.format.Time;
import android.util.Log;
import android.view.LayoutInflater;
import android.view.View;
import android.view.View.OnClickListener;
import android.view.ViewGroup;
import android.view.Window;
import android.widget.BaseAdapter;
import android.widget.Button;
import android.widget.ImageView;
import android.widget.ListView;
import android.widget.TextView;
import android.widget.Toast;

public class BluetoothTest extends Activity {

    final String TAG = "Bluetooth";
    private static Context mContext = null;
    ListView mListView = null;
    Button cancelButton = null;
    Button scanButton = null;

    LayoutInflater mInflater = null;
    BluetoothAdapter mBluetoothAdapter = null;
    List<DeviceInfo> mDeviceList = new ArrayList<DeviceInfo>();
    Set<BluetoothDevice> bondedDevices;
    Time time = new Time();
    long startTime;
    long endTime;
    boolean recordTime = false;
    private final static int MIN_COUNT = 1;
    boolean isUserCanncel = false;
    IntentFilter filter;
    private String testname;
    private boolean isPass = false;

    CountDownTimer mCountDownTimer = null;

    private long overitme = 10;

    @Override
    protected void onNewIntent(Intent intent) {

        super.onNewIntent(intent);
    }

    private void init(Context context)
    {
        mContext=context;
    }

    @Override
    public void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        init(this);
        mInflater = LayoutInflater.from(mContext);
        //requestWindowFeature(Window.FEATURE_INDETERMINATE_PROGRESS);
        setContentView(R.layout.bluetooth);

        // getService();
        bindView();

        mCountDownTimer = getOverTime(overitme, overitme);
        mCountDownTimer.start();

        mBluetoothAdapter = BluetoothAdapter.getDefaultAdapter();

        startScanAdapterUpdate();

        filter = new IntentFilter(BluetoothDevice.ACTION_FOUND);
        filter.addAction(BluetoothAdapter.ACTION_DISCOVERY_FINISHED);
        filter.addAction(BluetoothAdapter.ACTION_DISCOVERY_STARTED);
        filter.addAction(BluetoothAdapter.ACTION_STATE_CHANGED);

        if (mBluetoothAdapter.getState() == BluetoothAdapter.STATE_ON) {
               scanDevice();
        } else {
            if (mBluetoothAdapter.getState() != BluetoothAdapter.STATE_TURNING_ON) {
                time.setToNow();
                startTime = time.toMillis(true);
                recordTime = true;
                mBluetoothAdapter.enable();
            }
        }
    }


    void bindView() {

        mListView = (ListView) findViewById(R.id.devices_list);
        mListView.setAdapter(mAdapter);
    }

    private void scanDevice() {

        toast(getString(R.string.bluetooth_scan_start));
        setProgressBarIndeterminateVisibility(true);
        if (mBluetoothAdapter.isDiscovering()) {
            mBluetoothAdapter.cancelDiscovery();
        }
        mBluetoothAdapter.startDiscovery();
    }

    private void cancelScan() {

        setProgressBarIndeterminateVisibility(false);
        if (mBluetoothAdapter.isDiscovering()) {
            mBluetoothAdapter.cancelDiscovery();
        }
    }

    public void updateAdapter() {

        mAdapter.notifyDataSetChanged();
    }

    BroadcastReceiver mReceiver = new BroadcastReceiver() {

        @Override
        public void onReceive(Context context, Intent intent) {

            String action = intent.getAction();
            BluetoothDevice device = intent.getParcelableExtra(BluetoothDevice.EXTRA_DEVICE);

            if (BluetoothDevice.ACTION_FOUND.equals(action)) {
                mDeviceList.add(new DeviceInfo(device.getName(), device.getAddress()));
                updateAdapter();
                if (mDeviceList.size() >= MIN_COUNT){
                    StringBuilder sb = new StringBuilder(getString(R.string.bluetooth_info_head));
                    for (DeviceInfo di : mDeviceList) {
                        sb.append(di.getName()).append(";");
                    }
                    pass(sb.toString());
                }

            } else if (BluetoothAdapter.ACTION_DISCOVERY_FINISHED.equals(action)) {
                setProgressBarIndeterminateVisibility(false);
                if (mDeviceList.size() >= MIN_COUNT) {
                    StringBuilder sb = new StringBuilder(getString(R.string.bluetooth_info_head));
                    for (DeviceInfo di : mDeviceList) {
                        sb.append(di.getName()).append(";");
                    }
                    pass(sb.toString());
                } else if (!isUserCanncel)
                    toast(getString(R.string.bluetooth_scan_null));

            } else if (BluetoothAdapter.ACTION_DISCOVERY_STARTED.equals(action)) {
                startScanAdapterUpdate();
            } else if (BluetoothAdapter.ACTION_STATE_CHANGED.equals(action)) {

                if (BluetoothAdapter.STATE_ON == intent
                        .getIntExtra(BluetoothAdapter.EXTRA_STATE, 0)) {

                    scanDevice();

                    if (recordTime) {
                        time.setToNow();
                        endTime = time.toMillis(true);
                        recordTime = false;
                        toast("Turn on bluetooth cost " + (endTime - startTime) / 1000 + "S");
                    } else if (BluetoothAdapter.STATE_OFF == intent.getIntExtra(
                            BluetoothAdapter.EXTRA_STATE, 0))
                        mBluetoothAdapter.enable();
                } else if (BluetoothAdapter.STATE_TURNING_ON == intent.getIntExtra(
                        BluetoothAdapter.EXTRA_STATE, 0)) {
                    toast(getString(R.string.bluetooth_turning_on));
                    setProgressBarIndeterminateVisibility(true);
                }
            }

        }// onReceive

    };
    BaseAdapter mAdapter = new BaseAdapter() {

        public Object getItem(int arg0) {

            return null;
        }

        public long getItemId(int arg0) {

            return 0;
        }

        public View getView(int index, View convertView, ViewGroup parent) {

            if (convertView == null)
                convertView = mInflater.inflate(R.layout.bluetooth_item, null);
            ImageView image = (ImageView) convertView.findViewById(R.id.bluetooth_image);
            TextView text = (TextView) convertView.findViewById(R.id.bluetooth_text);
            text.setText(mDeviceList.get(index).getName() + "\n"
                    + mDeviceList.get(index).getAddress());
            image.setImageResource(android.R.drawable.stat_sys_data_bluetooth);
            return convertView;
        }

        public int getCount() {

            if (mDeviceList != null) {
                return mDeviceList.size();

            } else {
                return 0;
            }
        }
    };

    private void startScanAdapterUpdate() {

        mDeviceList.clear();
        bondedDevices = mBluetoothAdapter.getBondedDevices();
        for (BluetoothDevice device : bondedDevices) {
            DeviceInfo deviceInfo = new DeviceInfo(device.getName(), device.getAddress());
            mDeviceList.add(deviceInfo);
        }
        updateAdapter();

    }

    protected void onResume() {
        registerReceiver(mReceiver, filter);
        updateAdapter();
        super.onResume();
    };

    @Override
    protected void onPause() {

        unregisterReceiver(mReceiver);
        super.onPause();
    }

    void finishTest() {
        finish();
    }

    @Override
    public void finish() {
        mCountDownTimer.cancel();
        cancelScan();
        Intent returnIntend =new Intent();
        setResult(RESULT_OK,returnIntend);
        super.finish();
    }
    void fail(String msg) {
        AlertDialog.Builder builder = new AlertDialog.Builder(mContext);
        builder.setTitle("测试失败");
        builder.setMessage(msg);
        builder.setPositiveButton("确定", new DialogInterface.OnClickListener() {
            @Override
            public void onClick(DialogInterface dialog, int which) {
            }
        });
        builder.create().show();
    }

    void pass(String msg) {
        if (isPass)
            return;
        isPass = true;
        mCountDownTimer.cancel();
        mCountDownTimer = getShowTime(5, 5);
        mCountDownTimer.start();
    }

    public void toast(Object s) {

        if (s == null)
            return;
        Toast.makeText(this, s + "", Toast.LENGTH_SHORT).show();
    }

    private CountDownTimer getOverTime(long secondInFuture,long countDownInterval){
        return new CountDownTimer(secondInFuture*1000, countDownInterval*1000) {

            public void onTick(long arg0) {
            }

            public void onFinish() {
                fail("测试超时");
            }
        };
    }

    private CountDownTimer getShowTime(long secondInFuture,long countDownInterval){
        return new CountDownTimer(secondInFuture*1000, countDownInterval*1000) {

            public void onTick(long arg0) {
            }

            public void onFinish() {
                finishTest();
            }
        };
    }

}
