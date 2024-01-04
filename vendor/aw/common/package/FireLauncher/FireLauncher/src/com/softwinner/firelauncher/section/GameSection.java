package com.softwinner.firelauncher.section;

import java.util.ArrayList;
import java.util.List;
import android.annotation.SuppressLint;
import android.app.Activity;
import android.content.BroadcastReceiver;
import android.content.ContentResolver;
import android.content.Context;
import android.content.Intent;
import android.content.IntentFilter;
import android.content.pm.PackageManager;
import android.database.Cursor;
import android.graphics.Bitmap;
import android.net.Uri;
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
import com.softwinner.contentlib.AtetContent;
import com.softwinner.firelauncher.R;
import com.softwinner.firelauncher.network.HttpJsonLoader;
import com.softwinner.firelauncher.network.HttpJsonLoader.onJsonLoaderFinishListener;
import com.softwinner.firelauncher.section.app.AppBean;
import com.softwinner.firelauncher.section.app.AppListBiz;
import com.softwinner.firelauncher.subui.InstallGameActivity;
import com.softwinner.firelauncher.utils.AppConfig;
import com.softwinner.firelauncher.utils.Constants;
import com.softwinner.firelauncher.utils.Utils;
import com.softwinner.fireplayer.remotemedia.returnitem.ReturnGameItem;
import com.softwinner.fireplayer.remotemedia.returnitem.ReturnGameItem.GameInfo;

public class GameSection extends SectionParent implements
		onJsonLoaderFinishListener {
	private String TAG = "GameSection";
	private boolean DEBUG = true;

	private Context mContext;
	private HttpJsonLoader mGameLoader;
	private ImageLoader mImageLoader;
	private List<GameInfo> mGameItemList;
	private AppConfig mConfig;
	public static ArrayList<AppBean> gameApps;
	private AppDbDataChangeReceiver receiver;
	private ArrayList<View> viewList;
	private ArrayList<ImageView> networkImages;
	private ArrayList<ImageView> installSinges;
	private View poster_0;
	private View poster_1;
	private View poster_2;
	private View poster_3;
	private View poster_4;
	private View poster_5;
	private View poster_6;
	private View poster_7;
	private View poster_8;
	private View poster_9;
	private View poster_10;
	private View poster_11;

	private TextView install_game;
	private TextView game_market;
	private DisplayImageOptions optionsH;
	private DisplayImageOptions optionsW;
	private DisplayImageOptions optionsZ;
	private AnimationView mAnimationView;
	private PackageManager mPackageManager;

	private static int[] POSTER_LAYOUT = new int[] { R.id.poster_0_item,
			R.id.poster_1_item, R.id.poster_2_item, R.id.poster_3_item,
			R.id.poster_4_item, R.id.poster_5_item, R.id.poster_6_item,
			R.id.poster_7_item, R.id.poster_8_item, R.id.poster_9_item,
			R.id.poster_10_item, R.id.poster_11_item };

	private int section_length = 0;
	private final long INTERVAL = 5 * 1000;
	private boolean isLoaded = false;
	@SuppressLint("HandlerLeak")
	private Handler mHandler = new Handler() {
		public void handleMessage(android.os.Message msg) {
			switch (msg.what) {
			case Constants.START_LOAD_RECOMMEND:
				if (!isLoaded) {
					if (Utils.isNetworkConnected(mContext)) {
						if (DEBUG)
							Log.d(TAG, "real load");
						mGameLoader.loadUrl(Utils.appendGameUrl());
					} else {
						Log.w(TAG, "no network, no load");
					}
					sendEmptyMessageDelayed(Constants.START_LOAD_RECOMMEND,	INTERVAL);
				}
				break;
			case Constants.FINISH_LOAD_RECOMMEND:
				if (DEBUG)
					Log.d(TAG, "FINISH_LOAD_RECOMMEND " + isLoaded);
				removeMessages(Constants.START_LOAD_RECOMMEND);
				isLoaded = true;
				for (int i = 0; i < mGameItemList.size(); i++) {
					if (!mConfig.getGameImageURL(i).equals(mGameItemList.get(i).url)) {
						if (DEBUG)
							Log.d(TAG, "~~~~~~~~~ new url " + i + " " + mGameItemList.get(i).url);
						DisplayImageOptions option = getOption(i);
						ImageLoadingListener loadingListener = new loader(i, mGameItemList.get(i).url, option);
						mImageLoader.displayImage(mGameItemList.get(i).url,	networkImages.get(i), option, loadingListener);
						mConfig.setGameImageURL(i, mGameItemList.get(i).url);
						mConfig.setGameTitle(i, mGameItemList.get(i).gameName);
					} else {
						if (DEBUG)
							Log.d(TAG, "~~~~~~~~~ old url " + i + " " + mConfig.getRecommendURL(i));
					}
				}
				checkAllGameInstall();
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

	private DisplayImageOptions newDisplayImageOptions(int imageRes) {
		return new DisplayImageOptions.Builder().showImageOnLoading(imageRes)
				.showImageForEmptyUri(imageRes).showImageOnFail(imageRes)
				.resetViewBeforeLoading(true).cacheInMemory(true)
				.cacheOnDisk(true)
				.imageScaleType(ImageScaleType.IN_SAMPLE_POWER_OF_2)
				.bitmapConfig(Bitmap.Config.RGB_565).considerExifParams(true)
				.build();
	}
	
	private DisplayImageOptions getOption(int index){
		if(optionsH == null)
			optionsH = newDisplayImageOptions(R.drawable.fail_h);
		if(optionsW == null)
			optionsW = newDisplayImageOptions(R.drawable.fail_w);
		if(optionsZ == null)
			optionsZ = newDisplayImageOptions(R.drawable.fail_z);
		DisplayImageOptions option = optionsZ;
		switch (index) {
		case 0:
		case 5:
			option = optionsH;
			break;
		case 1:
		case 3:
		case 7:
		case 9:
			option = optionsW;
			break;
		case 2:
		case 4:
		case 6:
		case 8:
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
		int index = -1;
		String url;
		DisplayImageOptions option;

		public loader(int index, String url, DisplayImageOptions option) {
			this.index = index;
			this.url = url;
			this.option = option;
		}

		@Override
		public void onLoadingCancelled(String imageUri, View view) {
			// TODO Auto-generated method stub

		}

		@Override
		public void onLoadingComplete(String imageUri, View view, Bitmap loadedImage) {
			// TODO Auto-generated method stub

		}

		@Override
		public void onLoadingFailed(String imageUri, View view,	FailReason failReason) {
			Log.e(TAG, "ImageLoadingFailed!!! onLoadingFailed:index=" + index + " url=" + imageUri);
			if(mGameItemList.size() > 0 && !mGameItemList.get(index).url.equals(this.url))
				this.url = mGameItemList.get(index).url;
			mImageLoader.displayImage(this.url,	networkImages.get(index), option, loader.this);
		}

		@Override
		public void onLoadingStarted(String imageUri, View view) {
			// TODO Auto-generated method stub

		}

	}

	public GameSection(Context context) {
		super(context);
		mContext = context;
	}

	public GameSection(Context context, AttributeSet attrs) {
		this(context, attrs, 0);
		mContext = context;
	}

	public GameSection(Context context, AttributeSet attrs, int defStyle) {
		super(context, attrs, defStyle);
		mContext = context;

		mGameItemList = new ArrayList<ReturnGameItem.GameInfo>();
		mGameLoader = new HttpJsonLoader(mContext, HttpJsonLoader.JSON_GAME);
		mGameLoader.setJsonLoaderFinishListener(this);
		mHandler.sendEmptyMessage(Constants.START_LOAD_RECOMMEND);
		mImageLoader = ImageLoader.getInstance();

		mConfig = AppConfig.getInstance(mContext);
		mPackageManager = mContext.getPackageManager();
		receiver = new AppDbDataChangeReceiver();
		IntentFilter filter = new IntentFilter();
		filter.addAction(Constants.APP_DB_CHANGE);
		mContext.registerReceiver(receiver, filter);
		gameApps = getGameAppList(new AppListBiz(mContext).getLauncherApps());
	}

	private void loadImage(int index) {
		String url = mConfig.getGameImageURL(index);
		ImageView imageView = networkImages.get(index);
		Log.d(TAG, "loadImage:index=" + index + " url=" + url);
		if (!url.equals("")) {
			mImageLoader.displayImage(url, imageView, new loader(index, url, getOption(index)));
		}
	}

	@Override
	public void initView() {
		viewList = new ArrayList<View>();
		networkImages = new ArrayList<ImageView>();
		installSinges = new ArrayList<ImageView>();
		// *******************第一版面************************
		poster_0 = findViewById(R.id.poster_0);
		poster_0.setOnFocusChangeListener(this);
		poster_0.setOnClickListener(this);
		viewList.add(poster_0);

		poster_1 = findViewById(R.id.poster_1);
		poster_1.setOnFocusChangeListener(this);
		poster_1.setOnClickListener(this);
		viewList.add(poster_1);

		poster_2 = findViewById(R.id.poster_2);
		poster_2.setOnFocusChangeListener(this);
		poster_2.setOnClickListener(this);
		viewList.add(poster_2);

		poster_3 = findViewById(R.id.poster_3);
		poster_3.setOnFocusChangeListener(this);
		poster_3.setOnClickListener(this);
		viewList.add(poster_3);

		poster_4 = findViewById(R.id.poster_4);
		poster_4.setOnFocusChangeListener(this);
		poster_4.setOnClickListener(this);
		viewList.add(poster_4);

		// *******************第二版面************************
		poster_5 = findViewById(R.id.poster_5);
		poster_5.setOnFocusChangeListener(this);
		poster_5.setOnClickListener(this);
		viewList.add(poster_5);

		poster_6 = findViewById(R.id.poster_6);
		poster_6.setOnFocusChangeListener(this);
		poster_6.setOnClickListener(this);
		viewList.add(poster_6);

		poster_7 = findViewById(R.id.poster_7);
		poster_7.setOnFocusChangeListener(this);
		poster_7.setOnClickListener(this);
		viewList.add(poster_7);

		poster_8 = findViewById(R.id.poster_8);
		poster_8.setOnFocusChangeListener(this);
		poster_8.setOnClickListener(this);
		viewList.add(poster_8);

		poster_9 = findViewById(R.id.poster_9);
		poster_9.setOnFocusChangeListener(this);
		poster_9.setOnClickListener(this);
		viewList.add(poster_9);

		poster_10 = findViewById(R.id.poster_10);
		poster_10.setOnFocusChangeListener(this);
		poster_10.setOnClickListener(this);
		viewList.add(poster_10);

		poster_11 = findViewById(R.id.poster_11);
		poster_11.setOnFocusChangeListener(this);
		poster_11.setOnClickListener(this);
		viewList.add(poster_11);

		for (int i = 0; i < POSTER_LAYOUT.length; i++) {
			networkImages.add((ImageView) findViewById(POSTER_LAYOUT[i]).findViewById(R.id.poster_image));
			installSinges.add((ImageView) findViewById(POSTER_LAYOUT[i]).findViewById(R.id.install_signe));
			loadImage(i);
		}

		// *************************固定功能按键区**************************
		install_game = (TextView) findViewById(R.id.install_game);
		install_game.setOnFocusChangeListener(this);
		install_game.setOnClickListener(this);
		viewList.add(install_game);

		game_market = (TextView) findViewById(R.id.game_market);
		game_market.setOnFocusChangeListener(this);
		game_market.setOnClickListener(this);
		viewList.add(game_market);

		mAnimationView = new AnimationView(mContext, findViewById(R.id.animation_layout), true);
		mAnimationView.setShadowProcessor(this);

	}

	// 翻页调用
	@Override
	public void reSetSectionSmooth(boolean isSmoothToHead) {
		mLastFouseView = null;
		if (poster_0.getLeft() != 0) {
			if (isSmoothToHead) {
				scrollTo(0, 0);
				mAnimationView.moveWhiteBorder(poster_0);
			} else {
				scrollTo((int) poster_9.getX(), 0);
				mAnimationView.moveWhiteBorder(poster_9);
			}
		}
	}
	
	@Override
	public void updateFont() {
		mAnimationView.refreshFont();
	}

	@Override
	public void destroy() {
		mHandler.removeMessages(Constants.START_LOAD_RECOMMEND);
		mContext.unregisterReceiver(receiver);
	}

	private int preRate = 0;

	public int getDeltaX(View paramView) {
		if (0 == section_length) {
			section_length = poster_5.getLeft() - poster_0.getLeft();
		}
		int rate = (paramView.getLeft() - poster_0.getLeft()) / section_length;
		int shift = (rate - preRate) * section_length;
		preRate = rate;
		return shift;
	}

	@Override
	public void onClick(View paramView) {
		Intent intent;
		int ItemNumber = viewList.indexOf(paramView);
		if (ItemNumber == -1)
			return;
		switch (paramView.getId()) {
		case R.id.install_game:// 已安装 ATET 游戏
			intent = new Intent();
			intent.addFlags(Intent.FLAG_ACTIVITY_NEW_TASK);
			intent.setClass(mContext, InstallGameActivity.class);
			mContext.startActivity(intent);
			break;
		case R.id.game_market:// ATET 游戏大厅
			intent = AtetContent.getGameMarketIntent();
			 mContext.startActivity(intent);
			break;
		default:
			if (mGameItemList.size() <= 0) {
				Toast.makeText(mContext, R.string.can_not_download,
						Toast.LENGTH_SHORT).show();
				return;
			}
			if (ItemNumber < networkImages.size()) {// 游戏推荐控件
				if (isAppInstall(mGameItemList.get(ItemNumber).packageName)) {// 已安装游戏
					Intent appIntent = mPackageManager.getLaunchIntentForPackage(mGameItemList.get(ItemNumber).packageName);
					appIntent.addFlags(Intent.FLAG_ACTIVITY_NEW_TASK);
					mContext.startActivity(appIntent);
					intent = null;
				} else {// 未安装游戏
//					Log.d(TAG, "未安装游戏:Action="
//							+ mGameItemList.get(ItemNumber).actionUrl.action
//							+ " gameId=" + mGameItemList.get(ItemNumber).gameId
//							+ " packageName=" + mGameItemList.get(ItemNumber).packageName
//							+ " type=" + mGameItemList.get(ItemNumber).type);
					intent = AtetContent.getStartGameIntent(
							mGameItemList.get(ItemNumber).gameId,
							mGameItemList.get(ItemNumber).packageName,
							mGameItemList.get(ItemNumber).type);
					mContext.startActivity(intent);
				}
			}
			break;
		}
		((Activity) mContext).overridePendingTransition(android.R.anim.fade_in,	android.R.anim.fade_out);
	}

	@Override
	public void onFocusChange(View paramView, boolean paramBoolean) {
		mLastFouseView = paramView;
		if (DEBUG && paramBoolean)
			Log.d(TAG, "paramView x " + paramView.getX() + ", scroll x " + getScrollX());

		int ItemNumber = viewList.indexOf(paramView);
		if (ItemNumber >= networkImages.size())// 倒数2个对象为控件（install_game/game_market）不显示标题
			mAnimationView.ViewOnFocused(paramView, paramBoolean, "");
		else 
			mAnimationView.ViewOnFocused(paramView, paramBoolean, mConfig.getGameTitle(ItemNumber));
		
		if (paramBoolean)
			smoothScrooll(paramView);
	}

	private ArrayList<AppBean> getGameAppList(ArrayList<AppBean> topApps) {
		ArrayList<AppBean> gameList = new ArrayList<AppBean>();
		ContentResolver contentResolver = mContext.getContentResolver();
		Uri uri = Uri.parse("content://com.atet.tvmarket.provider/appIsGame");
		for (AppBean appBean : topApps) {
			Cursor c = contentResolver.query(uri, null, "packageName='"
					+ appBean.getPackageName() + "'", null, null);
			if (c != null && c.getCount() > 0) {// 该包名是ATET的游戏包名
				gameList.add(appBean);
			}
		}
		return gameList;
	}

	@Override
	public int getShadowSelect(int onFocusViewId) {
		if (R.id.poster_0 == onFocusViewId || R.id.poster_5 == onFocusViewId)
			return AnimationView.SHADOW_SIDE_HORIZONTAL;
		else
			return AnimationView.SHADOW_CENTER;
	}

	private boolean isAppInstall(String packageName) {
		for (AppBean app : gameApps) {
			if (app.getPackageName().equals(packageName))
				return true;
		}
		return false;
	}

	private void checkAllGameInstall() {
		for (int i = 0; i < mGameItemList.size(); i++) {
			if (isAppInstall(mGameItemList.get(i).packageName))
				installSinges.get(i).setVisibility(View.VISIBLE);
			else
				installSinges.get(i).setVisibility(View.INVISIBLE);
		}
	}

	private class AppDbDataChangeReceiver extends BroadcastReceiver {
		@Override
		public void onReceive(Context context, Intent intent) {
			gameApps = getGameAppList(new AppListBiz(mContext)
					.getLauncherApps());
			checkAllGameInstall();
		}
	}

	@Override
	public void onJsonLoaderFinish(int type, int addedSize) {
		if (HttpJsonLoader.JSON_GAME == type) {
			mGameItemList.clear();
			mGameItemList.addAll(mGameLoader.getGameItemList());
			if (mGameItemList.size() > 0) {
				mHandler.sendEmptyMessage(Constants.FINISH_LOAD_RECOMMEND);
			} else {
				mHandler.sendEmptyMessage(Constants.RELOAD_RECOMMEND);
			}
		}
	}

	@Override
	public void onNetworkError(String message) {
		// TODO Auto-generated method stub

	}

}
