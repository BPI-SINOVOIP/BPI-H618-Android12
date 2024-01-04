package com.softwinner.firelauncher.subui;

import java.util.ArrayList;

import android.app.Activity;
import android.content.BroadcastReceiver;
import android.content.Context;
import android.content.Intent;
import android.content.IntentFilter;
import android.graphics.Bitmap;
import android.os.AsyncTask;
import android.os.Bundle;
import android.os.Handler;
import android.text.TextUtils;
import android.util.Log;
import android.view.KeyEvent;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;
import android.view.ViewTreeObserver;
import android.widget.AdapterView;
import android.widget.AdapterView.OnItemClickListener;
import android.widget.AdapterView.OnItemSelectedListener;
import android.widget.BaseAdapter;
import android.widget.GridView;
import android.widget.ImageView;
import android.widget.TextView;

import com.nostra13.universalimageloader.core.DisplayImageOptions;
import com.nostra13.universalimageloader.core.ImageLoader;
import com.nostra13.universalimageloader.core.assist.ImageScaleType;
import com.softwinner.animationview.AnimationGridView;
import com.softwinner.animationview.AnimationText;
import com.softwinner.firelauncher.R;
import com.softwinner.firelauncher.utils.Constants;

public class SubUiActivity extends Activity implements OnItemClickListener {

	private String TAG = "SubUiActivity";
	private boolean DEBUG = true;
	protected static Context mContext;
	private static Boolean isSendIntent = true;  
	private static int lastItemNumber = -1;
	private Handler mHandler = null;
	private long onKeyDown_lastTime = 0;
	private int lastKeyAction = KeyEvent.ACTION_UP;
	private BroadcastReceiver mHomeKeyEventReceiver = null;
	
	protected GridView mFunctionalGridView;
	protected FunctionalAdapter mFunctionalAdapter;
	private View lastView = null;
	protected AnimationGridView mAnimationView;
	
	private int clickDelayTime;
	protected ArrayList<MangoBean> mBeans = new ArrayList<MangoBean>();
	protected TvTable mTable;
	private boolean isMeasured = false;
	
	LoadTask mLoadTask = null;
	protected ImageLoader mImageLoader;
	protected static final int TrackerSuccess = 12345;
	Handler handler = new Handler();

	@Override
	protected void onCreate(Bundle savedInstanceState) {
		Log.w(TAG, "onCreate");
		super.onCreate(savedInstanceState);
		setContentView(R.layout.tv_tracker_gridview);

		mContext = getApplicationContext();
		mImageLoader = ImageLoader.getInstance();
		mHandler = new Handler();
		clickDelayTime = getResources().getInteger(R.integer.default_long_click_delay);
//		mConfig = AppConfig.getInstance(mContext);

		this.mLoadTask = new LoadTask();
		HomeKeyEventRegisterReceiver();
	}

	@Override
	protected void onResume() {
		super.onResume();
	}

	@Override
	protected void onDestroy() {
		super.onDestroy();
		if (this.mLoadTask != null && !(this.mLoadTask.isCancelled())) {
			this.mLoadTask.cancel(true);
			// this.mTrackersTask.execute("");
		}
		if(mHomeKeyEventReceiver != null) {
			unregisterReceiver(mHomeKeyEventReceiver);
			mHomeKeyEventReceiver = null;
		}
	}

	public void setViewObserver(final View selectView) {
		ViewTreeObserver observer = selectView.getViewTreeObserver();
		observer.addOnPreDrawListener(new ViewTreeObserver.OnPreDrawListener() {
			public boolean onPreDraw() {
				if (!isMeasured) {
					Log.d(TAG, "onPreDraw onPreDraw onPreDraw");
					isMeasured = true;
					handler.postDelayed(new Runnable() {
						
						@Override
						public void run() {
							// TODO Auto-generated method stub
							lastView = mFunctionalGridView.getSelectedView();
							mAnimationView.onItemSelected(mFunctionalGridView, lastView, 
										mFunctionalGridView.getSelectedItemPosition());
						}
					}, 500);
				}
				return true;
			}
		});
	}

	@Override
	public boolean dispatchKeyEvent(KeyEvent event) {
		// TODO Auto-generated method stub
    	boolean ret = true; 
        long current = System.currentTimeMillis();
        int keyAction = event.getAction();
        if(keyAction == lastKeyAction &&
        		current - onKeyDown_lastTime < clickDelayTime) {
        	if(DEBUG) Log.d(TAG, "OnLongClickDelta="+ (current - onKeyDown_lastTime)+" || ignored event="+event); 
            ret = true; //consume KeyEvent
        } else {
        	if(DEBUG) Log.d(TAG, "super.dispatchKeyEvent: delta "+ (current - onKeyDown_lastTime)+" || event="+event); 
            onKeyDown_lastTime = keyAction == lastKeyAction ? current:0;
            lastKeyAction = keyAction;
            ret = super.dispatchKeyEvent(event);
        }
        return ret;
	}
	
	@Override
	public boolean onKeyDown(int keyCode, KeyEvent event) {
    	boolean ret = true; 
        ret = super.onKeyDown(keyCode, event);
        nextSelectPosition(event.getKeyCode());//移动到一行的底部时跳转到下一行的开头
        return ret;
	}

	public void nextSelectPosition(int keyCode) {
		if(mFunctionalGridView == null) return;
		int count = mFunctionalGridView.getCount();
		int nextPosition = 0;
		if(count > 0){
	        if(keyCode == KeyEvent.KEYCODE_DPAD_RIGHT){
	        	nextPosition = Math.min(mFunctionalGridView.getSelectedItemPosition()+1, count-1);
	        	mFunctionalGridView.setSelection(nextPosition);
	        }else if(keyCode == KeyEvent.KEYCODE_DPAD_LEFT){
	        	nextPosition = Math.max(mFunctionalGridView.getSelectedItemPosition()-1, 0);
	        	mFunctionalGridView.setSelection(nextPosition);
	        }
		}
	}
	
	private Runnable filterClickRun = new Runnable() {
		@Override
		public void run() {
			isSendIntent = true;			
		}
	};
	
	/**
	 * 过滤双击事件
	 * @return true：通过      false：过滤*/
	private boolean filterClick(int position){
		mHandler.removeCallbacks(filterClickRun);
		if(lastItemNumber == position || lastItemNumber == -1){
			mHandler.postDelayed(filterClickRun, Constants.FILTER_CLICK_INTERVAL);
			if(!isSendIntent)return false;
		}
		isSendIntent = false;
		lastItemNumber = position;
		return true;
	}
	
	@Override
	public void onItemClick(AdapterView<?> parent, View item, int position,	long id) {
		if(!filterClick(position))return;
		
		Intent intent = new Intent();
		intent.setAction("com.starcor.hunan.mgtv");
		intent.putExtra("cmd_ex", "show_video_detail");
		intent.putExtra("video_id", mBeans.get(position).videoId);
		intent.putExtra("video_type", "0");
		intent.putExtra("video_ui_style", "0");
		sendBroadcast(intent);
	}
	
	class LoadTask extends AsyncTask<String, Void, Integer> {
		@Override
		protected void onPreExecute() {
			super.onPreExecute();
		}

		@Override
		protected Integer doInBackground(String... params) {
			Log.w(TAG, "load db");
			if(null != mTable) {
				mBeans = mTable.getItems();
				return TrackerSuccess;
			} else {
				return -1;
			}
		}

		protected void onPostExecute(Integer result) {
			if( -1 == result ) 
				return;
			
			mFunctionalGridView = (GridView) findViewById(R.id.functional_zone_gridview);
			mFunctionalAdapter = new FunctionalAdapter();
			mFunctionalGridView.setAdapter(mFunctionalAdapter);
			mFunctionalGridView.setOnItemSelectedListener(new OnItemSelectedListener() {
				
				@Override
				public void onItemSelected(AdapterView<?> parent, View view, int position, long id) {
					mAnimationView.onItemSelected(parent, view, position);
				}

				@Override
				public void onNothingSelected(AdapterView<?> parent) {
//					Log.d(TAG, ">>>>>>>>>onNothingSelected");
					mAnimationView.onNothingSelected();
				}
			});
			mFunctionalGridView.setOnItemClickListener(SubUiActivity.this);
		    mAnimationView = new AnimationGridView(mContext, findViewById(R.id.animation_layout), 
		    		mFunctionalGridView, R.id.animation_view);
			setViewObserver(mFunctionalGridView);
		}
	}

	public class FunctionalAdapter extends BaseAdapter {
		DisplayImageOptions optionsH;
		
		public FunctionalAdapter() {
			optionsH = new DisplayImageOptions.Builder()
			.showImageOnLoading(R.drawable.fail_h)
			.showImageForEmptyUri(R.drawable.fail_h)
			.showImageOnFail(R.drawable.fail_h)
			.resetViewBeforeLoading(true)
			.cacheInMemory(true)
			.cacheOnDisk(true)
			.imageScaleType(ImageScaleType.IN_SAMPLE_POWER_OF_2)
			.bitmapConfig(Bitmap.Config.RGB_565)
			.considerExifParams(true)
//			.displayer(new FadeInBitmapDisplayer(300))
			.build();
		}

		@Override
		public int getCount() {
			// return Integer.MAX_VALUE;
//			Log.d(TAG, "FunctionalAdapter getCount "+ mBeans.size());
			return mBeans.size();
		}

		@Override
		public Object getItem(int position) {
			return position; 
		}

		@Override
		public long getItemId(int position) {
			return position;
		}

		@Override
		public View getView(int position, View convertView, ViewGroup container) {
//			Log.d(TAG, "FunctionalAdapter getView "+ position);
			ViewHolder holder;
			
			if (convertView == null) {
				convertView = LayoutInflater.from(mContext).inflate(R.layout.tv_tracker_griditem, null);
				holder = new ViewHolder();
				holder.img = (ImageView) convertView.findViewById(R.id.animation_view);
				holder.detail = (TextView) convertView.findViewById(R.id.item_detail);
				holder.name = (AnimationText) convertView.findViewById(R.id.item_name);
				convertView.setTag(holder);
			} else {
				holder = (ViewHolder)convertView.getTag();
			}
			mImageLoader.displayImage(mBeans.get(position).videoImgUrl,	holder.img, optionsH);
			String episodeCountString1 = getResources().getString(R.string.episode_count1);
			String episodeCountString2 = getResources().getString(R.string.episode_count2);
			holder.detail.setText(episodeCountString1+mBeans.get(position).episodeCount+episodeCountString2);
			holder.name.setText(mBeans.get(position).videoName);
			
			return convertView;
		}
		
		class ViewHolder {
			ImageView img;
			TextView detail;
			AnimationText name;
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