package com.softwinner.firelauncher.section;

import java.util.ArrayList;
import android.annotation.SuppressLint;
import android.app.Activity;
import android.content.Context;
import android.content.Intent;
import android.graphics.Bitmap;
import android.os.Handler;
import android.util.AttributeSet;
import android.util.Log;
import android.view.View;
import android.widget.ImageView;
import android.widget.TextView;
import android.widget.Toast;

import com.nostra13.universalimageloader.core.DisplayImageOptions;
import com.nostra13.universalimageloader.core.ImageLoader;
import com.nostra13.universalimageloader.core.assist.FailReason;
import com.nostra13.universalimageloader.core.assist.ImageScaleType;
import com.nostra13.universalimageloader.core.listener.ImageLoadingListener;
import com.softwinner.animationview.AnimationView;
import com.softwinner.contentlib.MangoTvContent;
import com.softwinner.firelauncher.R;
import com.softwinner.firelauncher.network.HttpJsonLoader;
import com.softwinner.firelauncher.network.HttpJsonLoader.onJsonLoaderFinishListener;
import com.softwinner.firelauncher.subui.TvCollectActivity;
import com.softwinner.firelauncher.subui.TvHistoryActivity;
import com.softwinner.firelauncher.subui.TvTrackerActivity;
import com.softwinner.firelauncher.utils.AppConfig;
import com.softwinner.firelauncher.utils.Constants;
import com.softwinner.firelauncher.utils.Utils;


public class RecommendSection extends SectionParent implements onJsonLoaderFinishListener{
	private String TAG = "RecommendSection";
	private boolean DEBUG = true;
	private static final int ENABLE_SEND_MANGO_INTENT = 11119;
	private static Boolean isSendIntent = true;  
	private static int lastItemNumber = -1;
	
	private Context mContext;
	private HttpJsonLoader mRecommendVideo;
	private HttpJsonLoader mRecommendRoll;
	private ImageLoader mImageLoader;
	private AppConfig mConfig;
	private ArrayList<View> viewList;
	private ArrayList<View> posterList;
	private View rolling_broadcast;
	private ImageView rolling_image;
	private ImageView featured_poster_0;
	private ImageView featured_poster_1;
	private ImageView featured_poster_2;
	private ImageView featured_poster_3;
	private ImageView featured_poster_4;
	private ImageView featured_poster_5;	
	private ImageView featured_poster_6;
	private ImageView featured_poster_7;
	private ImageView featured_poster_8;	
	private ImageView featured_poster_9;
	private ImageView featured_poster_10;
	private ImageView featured_poster_11;
	private ImageView mango;
	private TextView collect;
	private TextView search;
	private TextView history;
	private TextView tracker;
	private DisplayImageOptions optionsH;
	private DisplayImageOptions optionsW;
	private DisplayImageOptions optionsZ;
	private AnimationView mAnimationView;	
	
	private int section_length = 0;
	private int preRate = 0; 
	private final long INTERVAL = 5*1000;
	private final int ROLL_INDEX = 11116;
	private boolean isLoaded = false;
	private boolean isRollLoaded = false;
	
	@SuppressLint("HandlerLeak")
	private Handler mHandler = new Handler() {
		public void handleMessage(android.os.Message msg) {
			switch (msg.what) {
			case ENABLE_SEND_MANGO_INTENT:
				isSendIntent =true;
				break;
			case Constants.START_LOAD_RECOMMEND:
				if(!isLoaded) {
					if( Utils.isNetworkConnected(mContext) ) {
						if(DEBUG) Log.d(TAG, "~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ real load");
						mRecommendVideo.loadUrl(Utils.appendRecommendUrl());
					} else {
						Log.w(TAG, "no network, no load");
					}
					sendEmptyMessageDelayed(Constants.START_LOAD_RECOMMEND, INTERVAL);
				}
				break;
				
			case Constants.START_LOAD_RECOMMEND_ROLL:
				if(!isRollLoaded) {
					if( Utils.isNetworkConnected(mContext) ) {
						if(DEBUG) Log.d(TAG, "~~~~~~~~~~~RecommendRollUrl="+Utils.appendRecommendRollUrl());
						mRecommendRoll.loadUrl(Utils.appendRecommendRollUrl());
					} else {
						Log.w(TAG, "no network, no load");
					}
					sendEmptyMessageDelayed(Constants.START_LOAD_RECOMMEND_ROLL, INTERVAL);
				}
				break;
				
			case Constants.FINISH_LOAD_RECOMMEND_ROLL:
				String roolUrl = mRecommendRoll.getRecommendRollImageUrl();
				if(roolUrl == null || roolUrl.equals("")) return;
				removeMessages(Constants.START_LOAD_RECOMMEND_ROLL);
				isRollLoaded = true;
				if(!mConfig.getRecommendRollURL().equals(roolUrl)){
					if(DEBUG) Log.d(TAG, "~~~~~~~~~ RecommendRollImageUrl=" + roolUrl);
					DisplayImageOptions optionsR = getImageOption(R.drawable.fail_r);
					ImageLoadingListener loadingListener = new loader(ROLL_INDEX, roolUrl, optionsR);
					mImageLoader.displayImage(roolUrl, rolling_image, optionsR, loadingListener);
					mConfig.setRecommendRollURL(roolUrl);
				} else {
					if(DEBUG) Log.d(TAG, "~~~~~~~~~ old RollImageUrl "+ mConfig.getRecommendRollURL());
				}
				break;
			case Constants.FINISH_LOAD_RECOMMEND:
				if(DEBUG) Log.d(TAG, "FINISH_LOAD_RECOMMEND  FINISH_LOAD_RECOMMEND FINISH_LOAD_RECOMMEND "+ isLoaded);
				removeMessages(Constants.START_LOAD_RECOMMEND);
				isLoaded = true;
				for(int i=0; i< mRecommendVideo.getSize(); i++) {
					if( !mConfig.getRecommendURL(i).equals(mRecommendVideo.getRecommendVideoItem(i).video_img_list.video_img_url_1) ) {
						String url = mRecommendVideo.getRecommendVideoItem(i).video_img_list.video_img_url_1;
						String title = mRecommendVideo.getRecommendVideoItem(i).video_name;
						DisplayImageOptions option = getOption(i);
						if(DEBUG) Log.d(TAG, "~~~~~~~~~ new url " +i + " " + url);
						ImageLoadingListener loadingListener = new loader(i, url, option);
						mImageLoader.displayImage(url, (ImageView)viewList.get(i), option, loadingListener);
						mConfig.setRecommendURL(i, url);
						mConfig.setRecommendTitle(i, title);
					} else {
						if(DEBUG) Log.d(TAG, "~~~~~~~~~ old url "+ i +" "+ mConfig.getRecommendURL(i));
					}
				}
				break; 
			case Constants.RELOAD_RECOMMEND:  
				Log.w(TAG, "RELOAD_RECOMMEND RELOAD_RECOMMEND RELOAD_RECOMMEND");
				removeMessages(Constants.START_LOAD_RECOMMEND);
				isLoaded = false;
				sendEmptyMessageDelayed(Constants.START_LOAD_RECOMMEND, 5000);
				break;
			default:
				break;
			}
		};
	};
	
	private DisplayImageOptions getImageOption(int imageRes){
		return new DisplayImageOptions.Builder()
			.showImageOnLoading(imageRes)
			.showImageForEmptyUri(imageRes)
			.showImageOnFail(imageRes)
			.resetViewBeforeLoading(true)
			.cacheInMemory(true)
			.cacheOnDisk(true) 
			.imageScaleType(ImageScaleType.IN_SAMPLE_POWER_OF_2)
			.bitmapConfig(Bitmap.Config.RGB_565)
			.considerExifParams(true)
			.build();
	}
	
	private DisplayImageOptions getOption(int index){
		if(optionsH == null)
			optionsH = getImageOption(R.drawable.fail_h);
		if(optionsW == null)
			optionsW = getImageOption(R.drawable.fail_w);
		if(optionsZ == null)
			optionsZ = getImageOption(R.drawable.fail_z);
		DisplayImageOptions option = optionsZ;
		switch (index) {
		case 0:
		case 5:
			option = optionsH;						
			break;
		case 1:
		case 4:
		case 8:
		case 9:
			option = optionsW;
			break;
		case 2:
		case 3:
		case 6:
		case 7:
		case 10:
		case 11:
			option = optionsZ;
			break;
		default:
			break;
		}
		return option;
	}
	
	class loader implements ImageLoadingListener {

		int i = -1;
		String url;
		DisplayImageOptions option;
		
		public loader(int i, String url, DisplayImageOptions option) {
			Log.d(TAG, "ImageLoadingListener:i="+i+" url="+url);
			this.i = i;
			this.url = url;
			this.option = option;
		}
		
		@Override
		public void onLoadingCancelled(String arg0, View arg1) {
			// TODO Auto-generated method stub
			
		}

		@Override
		public void onLoadingComplete(String arg0, View arg1, Bitmap arg2) {
			// TODO Auto-generated method stub
			
		}

		@Override
		public void onLoadingFailed(String arg0, View arg1, FailReason arg2) {
			Log.e(TAG, "~~~~~~~~~~~~ onLoadingFailed onLoadingFailed onLoadingFailed "+ this.url + " "+ i);
			if(ROLL_INDEX == i){
				mImageLoader.displayImage(this.url, rolling_image, this.option, this);
			}else{
				mHandler.sendEmptyMessage(Constants.RELOAD_RECOMMEND);
			}
			
		}

		@Override
		public void onLoadingStarted(String arg0, View arg1) {
			// TODO Auto-generated method stub
			
		}
		
	}
	
	public RecommendSection(Context context) {
		super(context);
		mContext = context;
	}

	public RecommendSection(Context context, AttributeSet attrs) {
		this(context, attrs, 0);
		mContext = context;
	}
	
	public RecommendSection(Context context, AttributeSet attrs, int defStyle) {
		super(context, attrs, defStyle);
		mContext = context;
		
		mRecommendVideo = new HttpJsonLoader(mContext, HttpJsonLoader.JSON_RECOMMEND);
		mRecommendVideo.setJsonLoaderFinishListener(this);
		mHandler.sendEmptyMessage(Constants.START_LOAD_RECOMMEND);
		mRecommendRoll = new HttpJsonLoader(mContext, HttpJsonLoader.JSON_RECOMMEND_ROLL);
		mRecommendRoll.setJsonLoaderFinishListener(this);
		mHandler.sendEmptyMessage(Constants.START_LOAD_RECOMMEND_ROLL);
		mImageLoader = ImageLoader.getInstance();
		
		mConfig = AppConfig.getInstance(mContext);
	}
	
	private void loadImage(ImageView imageView, int index) {
		String url = mConfig.getRecommendURL(index);
		if(!url.equals("")) {
			mImageLoader.displayImage(url, imageView, new loader(index, url, getOption(index)));
		}
	}
	
	private void loadRollingImage() {
		String url = mConfig.getRecommendRollURL();
		if(!url.equals("")) {
			mImageLoader.displayImage(url, rolling_image);
		}
	}

	@Override
	public void initView() {
		viewList = new ArrayList<View>();
		posterList = new ArrayList<View>();
		
		//*******************第一版面************************
		featured_poster_0 = (ImageView) findViewById(R.id.featured_poster_0);
		featured_poster_0.setOnFocusChangeListener(this);
		featured_poster_0.setOnClickListener(this);
		loadImage(featured_poster_0, 0);
		viewList.add(featured_poster_0);
		posterList.add(featured_poster_0);
		
		featured_poster_1 = (ImageView) findViewById(R.id.featured_poster_1);
		featured_poster_1.setOnFocusChangeListener(this);
		featured_poster_1.setOnClickListener(this);
		loadImage(featured_poster_1, 1);
		viewList.add(featured_poster_1);
		posterList.add(featured_poster_1);
		
		featured_poster_2 = (ImageView) findViewById(R.id.featured_poster_2);
		featured_poster_2.setOnFocusChangeListener(this);
		featured_poster_2.setOnClickListener(this);
		loadImage(featured_poster_2, 2);
		viewList.add(featured_poster_2);
		posterList.add(featured_poster_2);
		
		featured_poster_3 = (ImageView) findViewById(R.id.featured_poster_3);
		featured_poster_3.setOnFocusChangeListener(this);
		featured_poster_3.setOnClickListener(this);
		loadImage(featured_poster_3, 3);
		viewList.add(featured_poster_3);
		posterList.add(featured_poster_3);
		
		featured_poster_4 = (ImageView) findViewById(R.id.featured_poster_4);
		featured_poster_4.setOnFocusChangeListener(this);
		featured_poster_4.setOnClickListener(this);
		loadImage(featured_poster_4, 4);
		viewList.add(featured_poster_4);
		posterList.add(featured_poster_4);
		
		//*******************第二版面************************
		featured_poster_5 = (ImageView) findViewById(R.id.featured_poster_5);
		featured_poster_5.setOnFocusChangeListener(this);
		featured_poster_5.setOnClickListener(this);
		loadImage(featured_poster_5, 5);
		viewList.add(featured_poster_5);
		posterList.add(featured_poster_5);
		
		featured_poster_6 = (ImageView) findViewById(R.id.featured_poster_6);
		featured_poster_6.setOnFocusChangeListener(this);
		featured_poster_6.setOnClickListener(this);
		loadImage(featured_poster_6, 6);
		viewList.add(featured_poster_6);
		posterList.add(featured_poster_6);
		
		featured_poster_7 = (ImageView) findViewById(R.id.featured_poster_7);
		featured_poster_7.setOnFocusChangeListener(this);
		featured_poster_7.setOnClickListener(this);
		loadImage(featured_poster_7, 7);
		viewList.add(featured_poster_7);
		posterList.add(featured_poster_7);
		
		featured_poster_8 = (ImageView) findViewById(R.id.featured_poster_8);
		featured_poster_8.setOnFocusChangeListener(this);
		featured_poster_8.setOnClickListener(this);
		loadImage(featured_poster_8, 8);
		viewList.add(featured_poster_8);
		posterList.add(featured_poster_8);
		
		featured_poster_9 = (ImageView) findViewById(R.id.featured_poster_9);
		featured_poster_9.setOnFocusChangeListener(this);
		featured_poster_9.setOnClickListener(this);
		loadImage(featured_poster_9, 9);
		viewList.add(featured_poster_9);
		posterList.add(featured_poster_9);
		
		featured_poster_10 = (ImageView) findViewById(R.id.featured_poster_10);
		featured_poster_10.setOnFocusChangeListener(this);
		featured_poster_10.setOnClickListener(this);
		loadImage(featured_poster_10, 10);
		viewList.add(featured_poster_10);
		posterList.add(featured_poster_10);
		
		featured_poster_11 = (ImageView) findViewById(R.id.featured_poster_11);
		featured_poster_11.setOnFocusChangeListener(this);
		featured_poster_11.setOnClickListener(this);
		loadImage(featured_poster_11, 11);
		viewList.add(featured_poster_11);
		posterList.add(featured_poster_11);

		//*********************固定功能按键*********************************
		//轮播
		rolling_broadcast = findViewById(R.id.rolling_broadcast);
		rolling_broadcast.setOnFocusChangeListener(this);
		rolling_broadcast.setOnClickListener(this);
		rolling_image = (ImageView)findViewById(R.id.rolling_broadcast_image);
		loadRollingImage();
		viewList.add(rolling_broadcast);
		
		mango = (ImageView) findViewById(R.id.mango);
		mango.setOnFocusChangeListener(this);
		mango.setOnClickListener(this);
		viewList.add(mango);
		
		collect = (TextView) findViewById(R.id.collect);
		collect.setOnFocusChangeListener(this);
		collect.setOnClickListener(this);
		viewList.add(collect);
		
		search = (TextView) findViewById(R.id.search);
		search.setOnFocusChangeListener(this);
		search.setOnClickListener(this);
		viewList.add(search);
		
		history = (TextView) findViewById(R.id.history);
		history.setOnFocusChangeListener(this);
		history.setOnClickListener(this);
		viewList.add(history);
		
		tracker = (TextView) findViewById(R.id.tv_tracker);
		tracker.setOnFocusChangeListener(this);
		tracker.setOnClickListener(this);
		viewList.add(tracker);
		
		mAnimationView = new AnimationView(mContext, findViewById(R.id.animation_layout), true);
		mAnimationView.setShadowProcessor(this);
		
	}

	@Override
	public void reSetSectionSmooth(boolean isSmoothToHead) {
		mLastFouseView = null;
		if(rolling_broadcast.getLeft()!=0) {
			if(isSmoothToHead) {
				scrollTo(0, 0);
				mAnimationView.moveWhiteBorder(rolling_broadcast);
			}else {
				scrollTo((int)featured_poster_9.getX(), 0);
				mAnimationView.moveWhiteBorder(featured_poster_9);
			}
		}
	}
	
	@Override
	public void updateFont() {
		mAnimationView.refreshFont();
	}


	@Override
	public void destroy() {
		// TODO Auto-generated method stub
		mHandler.removeMessages(Constants.START_LOAD_RECOMMEND);
	}

	public int getDeltaX(View paramView){
		if(0 == section_length) {
			section_length = featured_poster_5.getLeft() - rolling_broadcast.getLeft();
		}
		int rate = (paramView.getLeft() - rolling_broadcast.getLeft()) / section_length;
		int shift = (rate - preRate) * section_length;
		preRate = rate;
		return shift;
	}

	@Override
	public void onClick(View paramView) {
		Intent intent;
		int ItemNumber = viewList.indexOf(paramView);
		if (ItemNumber >= posterList.size()) {//mango服务器返回数据之外的控件
			switch (paramView.getId()) {
			case R.id.mango:
				intent = MangoTvContent.getMangoIntent();
				mContext.startActivity(intent);
				break;
			case R.id.collect:
				intent = new Intent();
				intent.addFlags(Intent.FLAG_ACTIVITY_NEW_TASK);
				intent.setClass(mContext, TvCollectActivity.class);
				mContext.startActivity(intent);
				break;
			case R.id.search:
				intent = MangoTvContent.getSearchIntent();
				mContext.sendBroadcast(intent);
				break; 
			case R.id.history:
				intent = new Intent();
				intent.addFlags(Intent.FLAG_ACTIVITY_NEW_TASK);
				intent.setClass(mContext, TvHistoryActivity.class);
				mContext.startActivity(intent);
				break;
			case R.id.tv_tracker:
				intent = new Intent();
				intent.addFlags(Intent.FLAG_ACTIVITY_NEW_TASK);
				intent.setClass(mContext, TvTrackerActivity.class);
				mContext.startActivity(intent);
				break;
			case R.id.rolling_broadcast:
				intent = MangoTvContent.getRollingIntent();
				mContext.sendBroadcast(intent);
				break;
			default:
				return;
			}
		} else {
			if(mRecommendVideo.getSize()<=0){
				Toast.makeText(mContext, R.string.can_not_download, Toast.LENGTH_SHORT).show();
				return;
			}
			if(!filterClick(ItemNumber))return;
			
			intent = MangoTvContent.getVideoIntent(mRecommendVideo.getRecommendVideoItem(ItemNumber).video_id, 
						mRecommendVideo.getRecommendVideoItem(ItemNumber).video_ui_style);
			mContext.sendBroadcast(intent);
		}
		
		((Activity)mContext).overridePendingTransition(android.R.anim.fade_in,android.R.anim.fade_out);
	}
	
	/**
	 * 过滤双击事件
	 * @return true：通过      false：过滤*/
	private boolean filterClick(int position){
		mHandler.removeMessages(ENABLE_SEND_MANGO_INTENT);
		if(lastItemNumber == position || lastItemNumber == -1){
			mHandler.sendEmptyMessageDelayed(ENABLE_SEND_MANGO_INTENT, Constants.FILTER_CLICK_INTERVAL);
			if(!isSendIntent)return false;
		}
		isSendIntent = false;
		lastItemNumber = position;
		return true;
	}

	@Override
	public void onFocusChange(View paramView, boolean paramBoolean) {
		mLastFouseView = paramView;
		if(DEBUG && paramBoolean) Log.d(TAG, "paramView x "+ paramView.getX() + ", scroll x "+ getScrollX());
		
		int ItemNumber = viewList.indexOf(paramView);
		if(ItemNumber >= posterList.size()){//不显示标题
			mAnimationView.ViewOnFocused(paramView, paramBoolean, "");
		}else {
			mAnimationView.ViewOnFocused(paramView, paramBoolean, mConfig.getRecommendTitle(ItemNumber));
		}
		if (paramBoolean)
//			smoothScrooll(paramView, Utils.dip2px(mContext, 900));
			smoothScrooll(paramView);
	}
	
	@Override
	public int getShadowSelect(int onFocusViewId) {
		if(R.id.featured_poster_0 == onFocusViewId || R.id.featured_poster_5== onFocusViewId )
			return AnimationView.SHADOW_SIDE_HORIZONTAL;
		else
			return AnimationView.SHADOW_CENTER;
	}

	@Override
	public void onJsonLoaderFinish(int type, int addedSize) {
//		Log.d(TAG, "mRecommendVideo.getSize() "+ mRecommendVideo.getSize());
		if(HttpJsonLoader.JSON_RECOMMEND == type && mRecommendVideo.getSize() > 0 ) {
			mHandler.sendEmptyMessage(Constants.FINISH_LOAD_RECOMMEND);
		}else if(HttpJsonLoader.JSON_RECOMMEND_ROLL == type){
			mHandler.sendEmptyMessage(Constants.FINISH_LOAD_RECOMMEND_ROLL);
		}
	}

	@Override
	public void onNetworkError(String message) {
		// TODO Auto-generated method stub
		
	}
	
}
