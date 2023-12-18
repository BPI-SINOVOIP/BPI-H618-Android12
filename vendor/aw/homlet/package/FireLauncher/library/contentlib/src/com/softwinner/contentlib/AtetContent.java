package com.softwinner.contentlib;

import android.content.Intent;

public class AtetContent {
	public static Intent getGameMarketIntent() {
		Intent intent = new Intent();
		intent.addFlags(Intent.FLAG_ACTIVITY_NEW_TASK);
		intent.setAction("com.atet.tvmarket.MAIN");
		return intent;
	}
	
	public static Intent getStartGameIntent(String id, String name, int type){
		Intent intent = new Intent();
		intent.addFlags(Intent.FLAG_ACTIVITY_NEW_TASK);
		intent.setAction("com.atet.tvmarket.DETAILGAME");
		intent.putExtra("gameId", id);// 游戏id
		intent.putExtra("packageName", name);// 包名
		intent.putExtra("type", type);//类型
		return intent;
	}
}
