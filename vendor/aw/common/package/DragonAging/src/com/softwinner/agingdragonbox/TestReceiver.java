package com.softwinner.agingdragonbox;

import android.content.BroadcastReceiver;
import android.content.Context;
import android.content.Intent;
import android.util.Log;

public class TestReceiver extends BroadcastReceiver {
    public void onReceive(Context context, Intent intent) {
        String message = intent.getStringExtra("message");
        Log.i("TestReceiver", message);
        System.exit(0);

    }
}
