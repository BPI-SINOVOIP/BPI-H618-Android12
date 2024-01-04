
package com.android.tv.launcher.device.bluetooth;

import com.android.tv.launcher.R;

import android.app.NotificationManager;
import android.bluetooth.BluetoothAdapter;
import android.bluetooth.BluetoothDevice;
import android.bluetooth.BluetoothDevicePicker;
import android.content.BroadcastReceiver;
import android.content.ContentUris;
import android.content.ContentValues;
import android.content.Context;
import android.content.Intent;
import android.database.Cursor;
import android.net.Uri;
import android.util.Log;
import android.widget.Toast;


public class BluetoothFileConfirmReceiver extends BroadcastReceiver {
    private static final String TAG = "BluetoothFileConfirmReceiver";


    private static final String EXTRA_DEVICE_NAME = "opp_device_name";
    private static final String EXTRA_FILE_NAME = "opp_file_name";
    private static final String EXTRA_FILE_SIZE = "opp_file_size";


    public static final String ACTION_OPP_FILE_CONFIRM = "tv.launcher.bluetooth.file.confirm.receiver";


    @Override
    public void onReceive(Context context, Intent intent) {
        Log.v(TAG, "onReceive().");

        String action = intent.getAction();


        if (action.equals(ACTION_OPP_FILE_CONFIRM)) {



        }
    }


}