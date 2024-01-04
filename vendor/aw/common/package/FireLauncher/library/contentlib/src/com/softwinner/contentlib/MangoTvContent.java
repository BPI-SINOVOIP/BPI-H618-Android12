package com.softwinner.contentlib;

import android.content.Intent;


public class MangoTvContent {
	public static Intent getMangoIntent() {
		Intent intent = new Intent();
		intent.addFlags(Intent.FLAG_ACTIVITY_NEW_TASK);
		intent.setAction("com.starcor.hunan.mgtv");
		return intent;
	}
	
	public static Intent getSearchIntent() {
		Intent intent = new Intent();
		intent.addFlags(Intent.FLAG_ACTIVITY_NEW_TASK);
		intent.setAction("com.starcor.hunan.mgtv");
		intent.putExtra("cmd_ex", "show_search");
		return intent;
	}
	
	public static Intent getRollingIntent() {
		Intent intent = new Intent();
		intent.addFlags(Intent.FLAG_ACTIVITY_NEW_TASK);
		intent.setAction("com.starcor.hunan.mgtv"); 
		intent.putExtra("cmd_ex", "show_timeshift_list");
		return intent;
	}
	
	public static Intent getVideoIntent(String id, String style) {
		Intent intent = new Intent();
		intent.addFlags(Intent.FLAG_ACTIVITY_NEW_TASK);
		intent.setAction("com.starcor.hunan.mgtv");
		intent.putExtra("cmd_ex", "show_video_detail");
		intent.putExtra("video_id",	id);
		intent.putExtra("video_type", "0");
		intent.putExtra("video_ui_style", style);
		return intent;
	}
}
