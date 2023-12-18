package com.softwinner.firelauncher.section.app;

import com.softwinner.firelauncher.utils.Constants;

import android.content.BroadcastReceiver;
import android.content.Context;
import android.content.Intent;
import android.util.Log;

public class AppChangeReceiver extends BroadcastReceiver {
	private String TAG = "AppChangeReceiver";
	
	@Override
	public void onReceive(Context context, Intent intent) {
		// TODO Auto-generated method stub
		String action = intent.getAction();
		if (action.equals(Intent.ACTION_PACKAGE_ADDED) ) {
			String packageName = intent.getDataString();
			Log.d(TAG, "ADD packageName "+ packageName);
//			appDb.insertApp(packageName);
		} else if( action.equals(Intent.ACTION_PACKAGE_REMOVED)) {
			String packageName = intent.getDataString();
//			boolean isDel = appDb.deleteApp(packageName);
			Log.d(TAG, "REMOVED packageName "+ packageName);
		} else if (action.equals(Intent.ACTION_EXTERNAL_APPLICATIONS_AVAILABLE)) {
			String packageName = intent.getDataString();
			Log.d(TAG, "sd mount,add packageName "+ packageName);
//			appDb.insertApp(packageName);
		} else if( action.equals(Intent.ACTION_EXTERNAL_APPLICATIONS_UNAVAILABLE)) {
			String packageName = intent.getDataString();
//			boolean isDel = appDb.deleteApp(packageName);
			Log.d(TAG, "sd unmount,remount packageName "+ packageName);
		}
		Intent newIntent = new Intent(Constants.APP_DB_CHANGE);
		context.sendBroadcast(newIntent);
	}

}
