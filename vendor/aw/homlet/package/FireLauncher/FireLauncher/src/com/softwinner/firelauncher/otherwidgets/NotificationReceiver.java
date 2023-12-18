package com.softwinner.firelauncher.otherwidgets;

import android.content.BroadcastReceiver;
import android.content.Context;
import android.content.Intent;
import android.os.Bundle;
import android.util.Log;

public class NotificationReceiver extends BroadcastReceiver {
	private final static String TAG = "NotificationReceiver";
	private final static String ACTION_NAME = "system.ui.notification.count";
	public static int notificationCount = 0;
	@Override
	public void onReceive(Context context, Intent intent) {
		String action = intent.getAction();
		if(action.equals(ACTION_NAME)){
			NotificationReceiver.notificationCount = ((Bundle)intent.getExtras()).getInt("nitify.count");
			Log.d(TAG, "NotificationReceiver:NotificationCount="+NotificationReceiver.notificationCount);
		}
	}	
}
