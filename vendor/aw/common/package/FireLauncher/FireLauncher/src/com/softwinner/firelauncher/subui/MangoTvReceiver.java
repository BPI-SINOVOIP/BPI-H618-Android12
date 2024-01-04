package com.softwinner.firelauncher.subui;

import android.content.BroadcastReceiver;
import android.content.Context;
import android.content.Intent;
import android.util.Log;

import com.softwinner.firelauncher.utils.Constants;

public class MangoTvReceiver extends BroadcastReceiver {
	private final String TAG = "MangoTvReceiver";
	
	@Override
	public void onReceive(Context context, Intent intent) {
		// TODO Auto-generated method stub
		if(intent.getAction().equals("com.tv.follow")) {
			Log.d("", "MangoTvReceiver follow add ,onReceive onReceive onReceive onReceive");
			MangoBean bean = new MangoBean();
			bean.videoId = intent.getStringExtra(Constants.INTENT_VIDEO_ID);
			bean.videoName = intent.getStringExtra(Constants.INTENT_VIDEO_NAME);
			bean.videoImgUrl = intent.getStringExtra(Constants.INTENT_VIDEO_IMG_URL);
			bean.episodeId = intent.getStringExtra(Constants.INTENT_EPISODE_ID);
			bean.episodeName = intent.getStringExtra(Constants.INTENT_EPISODE_NAME);
			bean.episodeCount = intent.getIntExtra(Constants.INTENT_EPISODE_COUNT, 0);
			bean.currentPosition = intent.getIntExtra(Constants.INTENT_CURRENT_POSITION, 0);
			bean.duration = intent.getIntExtra(Constants.INTENT_DURATION, 0);
			
			TvTrackerTable db = TvTrackerTable.getInstance(context);
			db.addItem(bean);
		} else if(intent.getAction().equals("com.tv.collection")){
			Log.d("", "MangoTvReceiver collection add ,onReceive onReceive onReceive onReceive");
			MangoBean bean = new MangoBean();
			bean.videoId = intent.getStringExtra(Constants.INTENT_VIDEO_ID);
			bean.videoName = intent.getStringExtra(Constants.INTENT_VIDEO_NAME);
			bean.videoImgUrl = intent.getStringExtra(Constants.INTENT_VIDEO_IMG_URL);
			bean.episodeId = intent.getStringExtra(Constants.INTENT_EPISODE_ID);
			bean.episodeName = intent.getStringExtra(Constants.INTENT_EPISODE_NAME);
			bean.episodeCount = intent.getIntExtra(Constants.INTENT_EPISODE_COUNT, 0);
			bean.currentPosition = intent.getIntExtra(Constants.INTENT_CURRENT_POSITION, 0);
			bean.duration = intent.getIntExtra(Constants.INTENT_DURATION, 0);
			
			TvCollectionTable db = TvCollectionTable.getInstance(context);
			db.addItem(bean);
		} else if(intent.getAction().equals("com.starcor.hunan.mgtv")){
			String cmdEx = intent.getStringExtra(Constants.ID_CMD);
			if(cmdEx!=null && cmdEx.equals(Constants.ID_CMD_ADD_PLAY)) {
				Log.d(TAG, "MangoTvReceiver play add ,onReceive onReceive onReceive onReceive");
				MangoBean bean = new MangoBean();
				bean.videoId = intent.getStringExtra(Constants.INTENT_VIDEO_ID2);
				bean.videoName = intent.getStringExtra(Constants.INTENT_VIDEO_NAME2);
				bean.videoImgUrl = intent.getStringExtra(Constants.INTENT_VIDEO_IMG_URL2);
				bean.videoIndex = intent.getIntExtra(Constants.INTENT_VIDEO_INDEX, 0);
				bean.videoIndexName = intent.getStringExtra(Constants.INTENT_VIDEO_INDEX_NAME);
				bean.videoIndexCount = intent.getIntExtra(Constants.INTENT_VIDEO_INDEX_COUNT, 0);
				bean.currentPosition = intent.getIntExtra(Constants.INTENT_CURRENT_POSITION2, 0);
				bean.duration = intent.getIntExtra(Constants.INTENT_DURATION2, 0);
				
				TvHistoryTable db = TvHistoryTable.getInstance(context);
				db.addItem(bean);
				
				//test addedTime of db record
//				ArrayList<MangoBean> beans;
//				beans = db.getToday();
//				for(MangoBean bean2 : beans) {
//					Log.w(TAG, "getToday " + bean2.videoName);
//				}
//				beans = db.getYesterday(); 
//				for(MangoBean bean2 : beans) {
//					Log.w(TAG, "getYesterday " + bean2.videoName);
//				}
//				beans = db.getDayBeforeYesterday();
//				for(MangoBean bean2 : beans) {
//					Log.w(TAG, "getDayBeforeYesterday " + bean2.videoName);
//				}
//				beans = db.getWeek();
//				for(MangoBean bean2 : beans) {
//					Log.w(TAG, "getWeek " + bean2.videoName);
//				}
//				beans = db.getRestOfWeek();
//				for(MangoBean bean2 : beans) {
//					Log.w(TAG, "getRestOfWeek " + bean2.videoName);
//				}
//				beans = db.getLastWeek();
//				for(MangoBean bean2 : beans) {
//					Log.w(TAG, "getLastWeek " + bean2.videoName);
//				}
//				beans = db.getEarly();
//				for(MangoBean bean2 : beans) {
//					Log.w(TAG, "getEarly " + bean2.videoName);
//				}
				
			}
			
		} else if(intent.getAction().equals("com.tv.totcl.delsingle")){
			Log.d("", "MangoTvReceiver del ,onReceive onReceive onReceive onReceive");
			String type = intent.getStringExtra(Constants.ID_TYPE);
			String videoId = intent.getStringExtra(Constants.INTENT_VIDEO_ID);
			if(type!=null && type.equals(Constants.ID_TYPE_TVTRACKER)) {
				TvTrackerTable db = TvTrackerTable.getInstance(context);
				db.delItem(videoId);
			} else if(type!=null && type.equals(Constants.ID_TYPE_COLLECT)) {
				TvCollectionTable db = TvCollectionTable.getInstance(context);
				db.delItem(videoId);
			} else if(type!=null && type.equals(Constants.ID_TYPE_RECENT)) {
				TvHistoryTable db = TvHistoryTable.getInstance(context);
				db.delItem(videoId);
			}
		} else if(intent.getAction().equals("com.tv.totcl.delall")) {
			Log.d("", "MangoTvReceiver del all ,onReceive onReceive onReceive onReceive");
			String type = intent.getStringExtra(Constants.ID_TYPE);
			Log.d("", "~~~~ "+type);
			if(type!=null && type.equals(Constants.ID_TYPE_RECENT)) {
				TvHistoryTable db = TvHistoryTable.getInstance(context);
				db.delALLItems();
			}
//			if(type.equals(Constants.ID_TYPE_TVTRACKER)) {
//				TvTrackerTable db = TvTrackerTable.getInstance(context);
//				db.delALLItems();
//			} else if(type.equals(Constants.ID_TYPE_COLLECT)) {
//				TvCollectionTable db = TvCollectionTable.getInstance(context);
//				db.delALLItems();
//			} else if(type.equals(Constants.ID_TYPE_RECENT)) {
//				TvHistoryTable db = TvHistoryTable.getInstance(context);
//				db.delALLItems();
//			}
				
		}
	}

}
