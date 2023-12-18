package com.softwinner.miracastReceiver;

import android.content.BroadcastReceiver;
import android.content.Context;
import android.content.Intent;
import android.app.Activity;
import android.util.Log;

public class SystemBootReceiver extends BroadcastReceiver{
    private static final String TAG = "SystemBootReceiver";

    @Override
    public void onReceive(Context context, Intent intent) {
        Log.d(TAG, "MiracastReceiver Received action = " + intent.getAction());

        try {
            Intent intentMiracast = new Intent(context, Miracast.class);
            intentMiracast.setFlags(Intent.FLAG_ACTIVITY_NEW_TASK); //TODO
            context.startActivity(intentMiracast);
        } catch (Exception e) {
            e.printStackTrace();
            Log.e(TAG, "open Miracast Activity error");
        }
    }
}