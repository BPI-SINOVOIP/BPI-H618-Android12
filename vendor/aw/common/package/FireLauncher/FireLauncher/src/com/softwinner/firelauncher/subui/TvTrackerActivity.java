package com.softwinner.firelauncher.subui;

import android.content.BroadcastReceiver;
import android.content.Context;
import android.content.Intent;
import android.content.IntentFilter;
import android.os.AsyncTask;
import android.text.TextUtils;
import android.util.Log;
import android.widget.TextView;

import com.softwinner.firelauncher.R;
import com.softwinner.firelauncher.utils.Constants;

public class TvTrackerActivity extends SubUiActivity  {
	
	private String TAG = "TvTrackerActivity";
	private boolean DEBUG = true;
	private TvTrackerRefreshReceiver mRefreshReceiver;
	private BroadcastReceiver mHomeKeyEventReceiver = null;
	
	@Override
	protected void onCreate(android.os.Bundle savedInstanceState) {
		// TODO Auto-generated method stub
		super.onCreate(savedInstanceState);
		
		TextView textImage_title = (TextView) findViewById(R.id.textImage_title);
		textImage_title.setText(R.string.recommand_tracker);
		
		mTable = TvTrackerTable.getInstance(this);
		mLoadTask.execute("");
		
		IntentFilter filter = new IntentFilter();
		filter.addAction("com.tv.follow");
		filter.addAction("com.tv.totcl.delsingle");
		mRefreshReceiver = new TvTrackerRefreshReceiver();
		registerReceiver(mRefreshReceiver, filter);
		HomeKeyEventRegisterReceiver();
	}

	@Override
	protected void onDestroy() {
		// TODO Auto-generated method stub
		super.onDestroy();
		
		if(mRefreshReceiver != null){
			unregisterReceiver(mRefreshReceiver);
			mRefreshReceiver = null;
		}
		
		if(mHomeKeyEventReceiver != null) {
			unregisterReceiver(mHomeKeyEventReceiver);
			mHomeKeyEventReceiver = null;
		}
	}

	class TvTrackerRrefreshTask extends AsyncTask<Intent, Void, Boolean> {

		private boolean addTvTracker(Intent intent){
			if(DEBUG) Log.d(TAG, "addTracker addTracker addTracker");
			MangoBean bean = new MangoBean();
			bean.videoId = intent.getStringExtra(Constants.INTENT_VIDEO_ID);
			bean.videoName = intent.getStringExtra(Constants.INTENT_VIDEO_NAME);
			bean.videoImgUrl = intent.getStringExtra(Constants.INTENT_VIDEO_IMG_URL);
			bean.episodeId = intent.getStringExtra(Constants.INTENT_EPISODE_ID);
			bean.episodeName = intent.getStringExtra(Constants.INTENT_EPISODE_NAME);
			bean.episodeCount = intent.getIntExtra(Constants.INTENT_EPISODE_COUNT, 0);
			bean.currentPosition = intent.getIntExtra(Constants.INTENT_CURRENT_POSITION, 0);
			bean.duration = intent.getIntExtra(Constants.INTENT_DURATION, 0);
			
			for(MangoBean b : mBeans) {
				if( b.videoId.equals(bean.videoId) ){
					if(DEBUG) Log.w(TAG, "same bean, do not add");
					return false;
				}
			}
			mBeans.add(bean);
			return true;
		}
		
		private boolean deleteTvTracker(Intent intent){
			if(DEBUG) Log.d(TAG, "deleteTracker deleteTracker deleteTracker");
			String type = intent.getStringExtra(Constants.ID_TYPE);
			if(type.equals(Constants.ID_TYPE_TVTRACKER)) {
				String videoId = intent.getStringExtra(Constants.INTENT_VIDEO_ID);
				for(MangoBean bean : mBeans ) {
					if(bean.videoId.equals(videoId)) {
						mBeans.remove(bean);
						return true;
					}
				}
			}
			return false;
		}
		@Override
		protected Boolean doInBackground(Intent... intents) {
			// TODO Auto-generated method stub
			if(null != intents[0] ) {
				if(intents[0].getAction().equals("com.tv.follow")) {
					return addTvTracker(intents[0]);
				} else if(intents[0].getAction().equals("com.tv.totcl.delsingle")){
					String type = intents[0].getStringExtra(Constants.ID_TYPE);
					if( type.equals(Constants.ID_TYPE_TVTRACKER))
						return deleteTvTracker(intents[0]);
				}
			}
			return false;
		}
		
		protected void onPostExecute(Boolean result) {
			if(DEBUG) Log.d(TAG, "onPostExecute onPostExecute onPostExecute "+result);
			if(result) {
				mFunctionalAdapter.notifyDataSetChanged();
			}
		}
		
	}

	class TvTrackerRefreshReceiver extends BroadcastReceiver {
		@Override
		public void onReceive(Context context, Intent intent) {
			if(DEBUG) Log.w(TAG, "onReceive onReceive onReceive");
			TvTrackerRrefreshTask task = new TvTrackerRrefreshTask();
			task.execute(intent);
		}
	}
	
	public void HomeKeyEventRegisterReceiver(){
		mHomeKeyEventReceiver = new BroadcastReceiver() {

			@Override
			public void onReceive(Context context, Intent intent) {

				String action = intent.getAction();
	            if (action.equals(Intent.ACTION_CLOSE_SYSTEM_DIALOGS)) {  
	                String reason = intent.getStringExtra("reason");  
	                if (TextUtils.equals(reason, "homekey")) {  
	                	finish();
	                } 
	            }
			}
		};
		IntentFilter intenFilter = new IntentFilter(Intent.ACTION_CLOSE_SYSTEM_DIALOGS);
		registerReceiver(mHomeKeyEventReceiver, intenFilter);
	}

}
