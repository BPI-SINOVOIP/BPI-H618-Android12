package com.softwinner.firelauncher.section;

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

import com.nostra13.universalimageloader.core.DisplayImageOptions;
import com.nostra13.universalimageloader.core.ImageLoader;
import com.nostra13.universalimageloader.core.assist.FailReason;
import com.nostra13.universalimageloader.core.assist.ImageScaleType;
import com.nostra13.universalimageloader.core.listener.ImageLoadingListener;
import com.softwinner.animationview.AnimationView;
import com.softwinner.firelauncher.R;
import com.softwinner.firelauncher.network.HttpJsonLoader;
import com.softwinner.firelauncher.network.HttpJsonLoader.onJsonLoaderFinishListener;
import com.softwinner.firelauncher.utils.AppConfig;
import com.softwinner.firelauncher.utils.Constants;
import com.softwinner.firelauncher.utils.Utils;


public class ChannelSection extends SectionParent implements onJsonLoaderFinishListener{
	private String TAG = "ChannelSection";
	private boolean DEBUG = true;
	private final int IMAGE_COUNT = 7;
	
	private Context mContext;
	private View education;
	private TextView IVmusic;
	private View tv_serial;
	private View variety;
	private TextView life;
	private TextView cartoon;
	private TextView documentary;	
	private View movie;
	private TextView sports;
	private View hd1080p;
	private TextView economics;
	private View microfilm;
	private AnimationView mAnimationView;
	
	private ImageView[] networkImages = new ImageView[IMAGE_COUNT];
	private ImageLoader mImageLoader;
	private AppConfig mConfig;
	private HttpJsonLoader mChannelImages;
	private final long INTERVAL = 5*1000;
	private boolean isLoaded = false;
	
	private Handler mHandler = new Handler() {
		@SuppressLint("HandlerLeak")
		public void handleMessage(android.os.Message msg) {
			switch (msg.what) {
			case Constants.START_LOAD_CHANNEL:
				if(!isLoaded) {
					if( Utils.isNetworkConnected(mContext) ) {
						Log.d(TAG, "real load");
						mChannelImages.loadUrl(Utils.appendChannelsUrl());
					} else {
						Log.w(TAG, "no network, no load");
					}
					sendEmptyMessageDelayed(Constants.START_LOAD_CHANNEL, INTERVAL);
				}
				break;
			case Constants.FINISH_LOAD_CHANNLE:
				Log.d(TAG, "FINISH_LOAD_CHANNLE FINISH_LOAD_CHANNLE FINISH_LOAD_CHANNLE");
				isLoaded = true;
//				initView();				
				DisplayImageOptions optionsH = new DisplayImageOptions.Builder()
					.showImageOnLoading(R.drawable.fail_h)
					.showImageForEmptyUri(R.drawable.fail_h)
					.showImageOnFail(R.drawable.fail_h)
					.resetViewBeforeLoading(true)
					.cacheInMemory(true)
					.cacheOnDisk(true) 
					.imageScaleType(ImageScaleType.IN_SAMPLE_POWER_OF_2)
					.bitmapConfig(Bitmap.Config.RGB_565)
					.considerExifParams(true)
					.build();
				DisplayImageOptions optionsW = new DisplayImageOptions.Builder()
					.showImageOnLoading(R.drawable.fail_w)
					.showImageForEmptyUri(R.drawable.fail_w)
					.showImageOnFail(R.drawable.fail_w)
					.resetViewBeforeLoading(true)
					.cacheInMemory(true)
					.cacheOnDisk(true) 
					.imageScaleType(ImageScaleType.IN_SAMPLE_POWER_OF_2)
					.bitmapConfig(Bitmap.Config.RGB_565)
					.considerExifParams(true)
					.build();
			
				for(int i=0; i<mChannelImages.getSize(); i++) {
					DisplayImageOptions option = optionsH;
					switch (i) {
					case 0:
					case 3:
					case 5:
						option = optionsH;
						break;
					case 1:
					case 2:
					case 4:
						option = optionsW;
						break;
					default:
						break;
					}
					ImageLoadingListener loadingListener = new loader(i, mChannelImages.getChannelImageItem(i).img, option);
					mImageLoader.displayImage(mChannelImages.getChannelImageItem(i).img, networkImages[i], option, loadingListener);
					mConfig.setChannelImgURL(i, mChannelImages.getChannelImageItem(i).img);
				}
				 
				break;
			case Constants.RELOAD_CHANNLE:  
				Log.w(TAG, "RELOAD_CHANNLE RELOAD_CHANNLE RELOAD_CHANNLE");
				removeMessages(Constants.RELOAD_CHANNLE);
				isLoaded = false;
				sendEmptyMessageDelayed(Constants.START_LOAD_CHANNEL, 5000);
				break;
			default:
				break;
			}
		}
	};
	
	class loader implements ImageLoadingListener {

		int i = -1;
		String url;
		DisplayImageOptions option;
		
		public loader(int i, String url, DisplayImageOptions option) {
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
			// TODO Auto-generated method stub
			Log.e(TAG, "~~~~~~~~~~~~ onLoadingFailed onLoadingFailed onLoadingFailed "+ this.url + " "+ i);
			mHandler.sendEmptyMessage(Constants.RELOAD_CHANNLE);
		}

		@Override
		public void onLoadingStarted(String arg0, View arg1) {
			// TODO Auto-generated method stub
			
		}
		
	}
		
	public ChannelSection(Context context) {
		super(context);
		mContext = context;
	}

	public ChannelSection(Context context, AttributeSet attrs) {
		this(context, attrs, 0);
		mContext = context;
	}
	
	public ChannelSection(Context context, AttributeSet attrs, int defStyle) {
		super(context, attrs, defStyle);
		mContext = context;
		
		mChannelImages = new HttpJsonLoader(mContext, HttpJsonLoader.JSON_CHANNEL);
		mChannelImages.setJsonLoaderFinishListener(this);
		mHandler.sendEmptyMessage(Constants.START_LOAD_CHANNEL);
		mImageLoader = ImageLoader.getInstance();
		mConfig = AppConfig.getInstance(mContext);
		
	}

	private void loadImage(ImageView imageView, int index) {
		String url = mConfig.getChannelImgURL(index);
		if(!url.equals("")) {
			mImageLoader.displayImage(url, imageView);
		}
	}
	
	@Override
	public void initView() {
		//*******************第一版面************************
		tv_serial = findViewById(R.id.tv_serial);
		tv_serial.setOnFocusChangeListener(this);
		tv_serial.setOnClickListener(this);
		networkImages[0] = (ImageView)findViewById(R.id.tv_serial_image);
		loadImage(networkImages[0], 0);
		
		variety = findViewById(R.id.variety);
		variety.setOnFocusChangeListener(this);
		variety.setOnClickListener(this);
		networkImages[1] = (ImageView)findViewById(R.id.variety_image);
		loadImage(networkImages[1], 1);
		
		cartoon = (TextView) findViewById(R.id.cartoon);
		cartoon.setOnFocusChangeListener(this);
		cartoon.setOnClickListener(this);
		
		documentary = (TextView) findViewById(R.id.documentary);
		documentary.setOnFocusChangeListener(this);
		documentary.setOnClickListener(this);
		
		IVmusic = (TextView) findViewById(R.id.IVmusic);
		IVmusic.setOnFocusChangeListener(this);
		IVmusic.setOnClickListener(this);
		
		life = (TextView) findViewById(R.id.life);
		life.setOnFocusChangeListener(this);
		life.setOnClickListener(this);
		
		movie = findViewById(R.id.movie);
		movie.setOnFocusChangeListener(this);
		movie.setOnClickListener(this);
		networkImages[2] = (ImageView)findViewById(R.id.movie_image);
		loadImage(networkImages[2], 2);
		
		//*******************第二版面************************
		hd1080p = findViewById(R.id.hd1080p);
		hd1080p.setOnFocusChangeListener(this);
		hd1080p.setOnClickListener(this);
		networkImages[3] = (ImageView)findViewById(R.id.hd1080p_image);
		loadImage(networkImages[3], 3);
		
		education = findViewById(R.id.education);
		education.setOnFocusChangeListener(this);
		education.setOnClickListener(this);
		networkImages[4] = (ImageView)findViewById(R.id.education_image);
		loadImage(networkImages[4], 4);
		
		economics = (TextView) findViewById(R.id.economics);
		economics.setOnFocusChangeListener(this);
		economics.setOnClickListener(this);
		
		sports = (TextView) findViewById(R.id.sports);
		sports.setOnFocusChangeListener(this);
		sports.setOnClickListener(this);
		
		microfilm = findViewById(R.id.microfilm);
		microfilm.setOnFocusChangeListener(this);
		microfilm.setOnClickListener(this);
		networkImages[5] = (ImageView)findViewById(R.id.microfilm_image);
		loadImage(networkImages[5], 5);
		
		mAnimationView = new AnimationView(mContext, findViewById(R.id.animation_layout), false);
		mAnimationView.setShadowProcessor(this);
	}
	
	@Override
	public void reSetSectionSmooth(boolean isSmoothToHead) {
		mLastFouseView = null;
		if( tv_serial.getLeft()!=0) {
//			int dx = (int)(microfilm.getX() - tv_serial.getX() );
			if(isSmoothToHead) {
				scrollTo(0, 0);
				mAnimationView.moveWhiteBorder(tv_serial);
			}
			else {
				scrollTo((int)microfilm.getX(), 0);
				mAnimationView.moveWhiteBorder(microfilm);
			}
		}
	}
	
	@Override
	public void updateFont() {
		// TODO Auto-generated method stub
		
	}

	@Override
	public void destroy() {
		// TODO Auto-generated method stub
		mHandler.removeMessages(Constants.START_LOAD_CHANNEL);
	}

//	private int preRate = 0; 
//	private int section_length = 0;
//	@Override
//	public int getDeltaX(View paramView){
//		if(0 == section_length)
//			section_length = hd1080p.getLeft() - tv_serial.getLeft();
//		int rate = (paramView.getLeft() - tv_serial.getLeft()) / section_length;
//		int shift = (rate - preRate) * section_length;
//		preRate = rate;
//		return shift;
//	}

	private void startMangoActivity(String id) {
		Intent intent = new Intent();
		intent.setAction("com.starcor.hunan.mgtv"); 
		intent.putExtra("cmd_ex", "show_category");
		intent.putExtra("packet_id", id);
		intent.putExtra("category_id", "1000"); 
		mContext.sendBroadcast(intent);
		((Activity)mContext).overridePendingTransition(android.R.anim.fade_in,android.R.anim.fade_out);
	}
	
	@Override
	public void onClick(View paramView) {
//		isAnimationAble = false;
		int id = paramView.getId();
		switch (id) {
		case R.id.tv_serial:
			startMangoActivity("TVseries");
			break;
		case R.id.variety:
			startMangoActivity("variety");
			break;
		case R.id.cartoon:
			startMangoActivity("animation");
			break;
		case R.id.documentary:
			startMangoActivity("documentary");
			break;
		case R.id.IVmusic:
			startMangoActivity("music");
			break;
		case R.id.life:
			startMangoActivity("fashion");
			break;
		case R.id.movie:
			startMangoActivity("movie");
			break;
		case R.id.hd1080p:
			startMangoActivity("FullHD");
			break;
		case R.id.education:
			startMangoActivity("Education");
			break;
		case R.id.economics:
			startMangoActivity("Finance");
			break;
		case R.id.sports:
			startMangoActivity("Sports");
			break;
		case R.id.microfilm:
			startMangoActivity("MicroFilm");
			break;
		default:
			break;
		}
	}

	@Override
	public void onFocusChange(View paramView, boolean paramBoolean) {
		mLastFouseView = paramView;
		if(DEBUG && paramBoolean) Log.d(TAG, "paramView x "+ paramView.getX() + ", scroll x "+ getScrollX() );
		mAnimationView.ViewOnFocused(paramView, paramBoolean, null);
		
		if (paramBoolean)
			smoothScrooll(paramView);
	}
	
	@Override
	public int getShadowSelect(int onFocusViewId) {
		if(R.id.tv_serial == onFocusViewId || R.id.hd1080p == onFocusViewId
				|| R.id.microfilm == onFocusViewId)
			return AnimationView.SHADOW_SIDE_HORIZONTAL;
		else
			return AnimationView.SHADOW_CENTER;
	}

	@Override
	public void onJsonLoaderFinish(int type, int addedSize) {
		Log.d(TAG, "mChannelImages.getSize() "+ mChannelImages.getSize());
		if(HttpJsonLoader.JSON_CHANNEL == type 
				&& mChannelImages.getSize() > 0) {
			mHandler.sendEmptyMessage(Constants.FINISH_LOAD_CHANNLE);
		}
	}

	@Override
	public void onNetworkError(String message) {
		// TODO Auto-generated method stub
		
	}
	
}
