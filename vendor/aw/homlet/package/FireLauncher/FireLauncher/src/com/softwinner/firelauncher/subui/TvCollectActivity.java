package com.softwinner.firelauncher.subui;

import android.content.BroadcastReceiver;
import android.content.Context;
import android.content.Intent;
import android.content.IntentFilter;
import android.os.AsyncTask;
import android.text.TextUtils;
import android.util.Log;
import android.widget.ImageView;
import android.widget.TextView;

import com.softwinner.firelauncher.R;
import com.softwinner.firelauncher.utils.Constants;

public class TvCollectActivity extends SubUiActivity {
	
	private String TAG = "CollectActivity";
	private TvCollectionRefreshReceiver mRefreshReceiver;

	@Override
	protected void onCreate(android.os.Bundle savedInstanceState) {
		// TODO Auto-generated method stub
		super.onCreate(savedInstanceState);
		
		TextView textImage_title = (TextView) findViewById(R.id.textImage_title);
		textImage_title.setText(R.string.recommand_collect);
		
		mTable = TvCollectionTable.getInstance(this);
		mLoadTask.execute("");
		
		IntentFilter filter = new IntentFilter();
		filter.addAction("com.tv.collection");
		filter.addAction("com.tv.totcl.delsingle");
		mRefreshReceiver = new TvCollectionRefreshReceiver();
		registerReceiver(mRefreshReceiver, filter);
		HomeKeyEventRegisterReceiver();
	}

	@Override
	protected void onDestroy() {
		// TODO Auto-generated method stub
		super.onDestroy();
		
		unregisterReceiver(mRefreshReceiver);
		if(mHomeKeyEventReceiver != null) {
			unregisterReceiver(mHomeKeyEventReceiver);
			mHomeKeyEventReceiver = null;
		}
	}
	
	class TvCollectionRefreshTask extends AsyncTask<Intent, Void, Boolean> {

		private boolean addTvCollection(Intent intent){
			Log.d(TAG, "addTvCollection addTvCollection addTvCollection");
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
					Log.w(TAG, "same bean, do not add");
					return false;
				}
			}
			mBeans.add(bean);
			return true;
		}
		private boolean deleteTvColletion(Intent intent){
			Log.d(TAG, "deleteTvColletion deleteTvColletion deleteTvColletion");
			String type = intent.getStringExtra(Constants.ID_TYPE);
			if(type.equals(Constants.ID_TYPE_COLLECT)) {
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
					return addTvCollection(intents[0]);
				} else if(intents[0].getAction().equals("com.tv.totcl.delsingle")){
					String type = intents[0].getStringExtra(Constants.ID_TYPE);
					if( type.equals(Constants.ID_TYPE_COLLECT))
						return deleteTvColletion(intents[0]);
				}
			}
			return false;
		}
		
		protected void onPostExecute(Boolean result) {
			Log.d(TAG, "onPostExecute onPostExecute onPostExecute "+result);
			if(result) {
				mFunctionalAdapter.notifyDataSetChanged();
			}
		}
		
	}

	class TvCollectionRefreshReceiver extends BroadcastReceiver {
		@Override
		public void onReceive(Context context, Intent intent) {
			Log.w(TAG, "onReceive onReceive onReceive");
			TvCollectionRefreshTask task = new TvCollectionRefreshTask();
			task.execute(intent);
		}
	}
	
	private BroadcastReceiver mHomeKeyEventReceiver = null;
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
