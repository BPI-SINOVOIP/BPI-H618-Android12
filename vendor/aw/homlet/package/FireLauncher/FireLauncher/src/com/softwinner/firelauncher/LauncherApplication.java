package com.softwinner.firelauncher;

import com.android.volley.RequestQueue;
import com.android.volley.toolbox.Volley;
import com.nostra13.universalimageloader.cache.disc.naming.Md5FileNameGenerator;
import com.nostra13.universalimageloader.core.DisplayImageOptions;
import com.nostra13.universalimageloader.core.ImageLoader;
import com.nostra13.universalimageloader.core.ImageLoaderConfiguration;
import com.nostra13.universalimageloader.core.assist.ImageScaleType;
import com.nostra13.universalimageloader.core.assist.QueueProcessingType;
import com.nostra13.universalimageloader.cache.disc.impl.UnlimitedDiscCache;

import android.app.Application;
import android.content.Context;
import android.graphics.Bitmap;

import java.io.File;

public class LauncherApplication extends Application {
	private static LauncherApplication instance;
	private long mCurrentTime;
	private RequestQueue mRequestQueue;
	private boolean mWeatherAnimation = true;
	
	@Override
	public void onCreate() {
		// TODO Auto-generated method stub
		super.onCreate();
		
		setInstance(this);
		initImageLoader(getApplicationContext());
		initVolleyQueue();
	}
	 
	public static LauncherApplication getInstance() {
		return instance;
	}
	
	public static void setInstance(LauncherApplication instance) {
		LauncherApplication.instance = instance;
	}
	
	private void initImageLoader(Context context) {
		// This configuration tuning is custom. You can tune every option, you may tune some of them,
		// or you can create default configuration by
		//  ImageLoaderConfiguration.createDefault(this);
		// method.
	File cacheDir = context.getCacheDir();
    	DisplayImageOptions defaultOptions = new DisplayImageOptions.Builder()
												.showImageOnLoading(R.drawable.empty_thumb)
												.showImageForEmptyUri(R.drawable.empty_thumb)
												.showImageOnFail(R.drawable.empty_thumb)
												.resetViewBeforeLoading(true)
												.cacheInMemory(true)
												.cacheOnDisk(true)
												.imageScaleType(ImageScaleType.IN_SAMPLE_POWER_OF_2)
												.bitmapConfig(Bitmap.Config.RGB_565)
												.considerExifParams(true)
//												.displayer(new FadeInBitmapDisplayer(300))
												.build();
    	
//    	File cacheDir = StorageUtils.getCacheDirectory(context);
		ImageLoaderConfiguration config = new ImageLoaderConfiguration.Builder(context)
				.threadPoolSize(3)
				.threadPriority(Thread.NORM_PRIORITY - 4)
				.denyCacheImageMultipleSizesInMemory()
				.diskCacheFileNameGenerator(new Md5FileNameGenerator())
				.tasksProcessingOrder(QueueProcessingType.LIFO)
//				.memoryCacheSizePercentage(40)
				.diskCacheFileCount(1000)
				.diskCache(new UnlimitedDiscCache(cacheDir))
				.defaultDisplayImageOptions(defaultOptions)
//				.writeDebugLogs() // Remove for release app
				.build();
		// Initialize ImageLoader with configuration.
		ImageLoader.getInstance().init(config);
	}
	
	private void initVolleyQueue() {
		mRequestQueue = Volley.newRequestQueue(getApplicationContext());
	}
	
	public long getCurrentTime() { 
		return mCurrentTime;
	}
	public void setCurrentTime(long time) {
		mCurrentTime = time;
	}
	
	public RequestQueue getRequestQueue() {
		return mRequestQueue;
	}

	public boolean getmWeatherAnimation() {
		return mWeatherAnimation;
	}

	public void setmWeatherAnimation(Boolean mWeatherAnimation) {
		this.mWeatherAnimation = mWeatherAnimation;
	}
	
}
