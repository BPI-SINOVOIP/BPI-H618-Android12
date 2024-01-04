package com.softwinner.firelauncher.section;

import java.util.ArrayList;
import java.util.List;

import junit.framework.Test;

import com.softwinner.animationview.AnimationView;
import com.softwinner.firelauncher.R;
import com.softwinner.firelauncher.section.AppSection.AppItemClass;
import com.softwinner.firelauncher.section.app.AppBean;
import com.softwinner.firelauncher.section.app.AppListBiz;
import com.softwinner.firelauncher.utils.AppConfig;
import com.softwinner.firelauncher.utils.Constants;
import com.softwinner.firelauncher.utils.Utils;

import android.annotation.SuppressLint;
import android.app.Activity;
import android.content.ActivityNotFoundException;
import android.content.BroadcastReceiver;
import android.content.ComponentName;
import android.content.ContentResolver;
import android.content.Context;
import android.content.Intent;
import android.content.IntentFilter;
import android.content.pm.ActivityInfo;
import android.content.pm.PackageManager;
import android.content.pm.ResolveInfo;
import android.database.Cursor;
import android.net.Uri;
import android.util.AttributeSet;
import android.util.Log;
import android.view.Gravity;
import android.view.KeyEvent;
import android.view.LayoutInflater;
import android.view.MotionEvent;
import android.view.View;
import android.view.WindowManager;
import android.widget.FrameLayout;
import android.widget.GridLayout;
import android.widget.ImageView;
import android.widget.TextView;
import android.widget.Toast;



public class v3_AppSection extends v3_SectionParent {
	private String TAG = "AppSection";
	private boolean DEBUG = true;
	private Context mContext;
	private GridLayout mGridLayout;
	public static ArrayList<AppBean> topApps;
	public static ArrayList<AppItemClass> appGridView;
	public static int APP_TYPE_NORMAL = 0;
	public static int APP_TYPE_SETTINGS = 1;
	public static int APP_TYPE_APPMARKET = 2;
	private int ItemMargin = 0;
	private static int[] BACKGROUND_COLOR = new int[]{R.drawable.bg_red, 
		R.drawable.bg_orange, R.drawable.bg_green, R.drawable.bg_bluegreen, 
		R.drawable.bg_blue, R.drawable.bg_darkgreen, R.drawable.bg_yellow, R.drawable.bg_purple};
	private AppDbDataChangeReceiver receiver;
	private AnimationView mAnimationView;
	private int addItemCount = 2;//1：没有应用市场，2：添加应用市场和应用管理
	private int CreateViewId = 0;
	int lastScrollerX = 0;
	int lastScrollerY = 0;

	public interface SectionInterface {
		public void setScrollTo(int x, int y);
	}

	public v3_AppSection(Context context) {
		super(context);
		mContext = context;
	}

	public v3_AppSection(Context context, AttributeSet attrs) {
		this(context, attrs, 0);
		mContext = context;
	}

	public v3_AppSection(Context context, AttributeSet attrs, int defStyle) {
		super(context, attrs, defStyle);
		mContext = context;
		receiver = new AppDbDataChangeReceiver();
		IntentFilter filter = new IntentFilter();
		filter.addAction(Constants.APP_DB_CHANGE);
		context.registerReceiver(receiver, filter);
		topApps = getAppList(new AppListBiz(mContext).getLauncherApps());
        ItemMargin = Utils.dip2px(context, 14);
	}
	
	public void onResume(){
		super.onResume();
		//从进入应用界面进入应用市场，并在应用市场中下载安装应用，返回主界面时，修复主界面UI
		if(mLastFouseView != null && mLastFouseView.getAnimation() == null){
			mLastFouseView.requestFocus();
			//mAnimationView.ViewOnFocused(mLastFouseView, true, null);
		}
	}
	
	@Override
	public void destroy() {
		// TODO Auto-generated method stub
		mContext.unregisterReceiver(receiver);
	}
	
	@Override
	public void initView() {
        mGridLayout = (GridLayout)findViewById(R.id.app_section_grid_layout);
        mGridLayout.setOnFocusChangeListener(this);
        updataUi(topApps);
		mAnimationView = new AnimationView(mContext, findViewById(R.id.animation_layout), false);
		mAnimationView.setShadowProcessor(this);
	}
	
	private ArrayList<AppBean> getAppList(ArrayList<AppBean> topApps){
		ArrayList<AppBean> appList = new ArrayList<AppBean>();
		//ContentResolver contentResolver = mContext.getContentResolver();
		//Uri uri = Uri.parse("content://com.atet.tvmarket.provider/appIsGame");
		//TODO:过滤不显示到应用界面的应用
		for (AppBean appBean : topApps) {
			if(appBean.getPackageName().equals("com.softwinner.firelauncher")
					//||appBean.getPackageName().equals("com.dangbeimarket")
					//||appBean.getPackageName().equals("com.guozi.appstore")
					/*||appBean.getPackageName().equals("com.atet.tvmarket")
					||appBean.getPackageName().equals("com.starcor.hunan")*/)
				continue;
			/*Cursor c=contentResolver.query(uri,null, "packageName='"+appBean.getPackageName()+"'", null, null);
			if(c!=null && c.getCount()>0){//该包名是ATET的游戏包名
				continue;
			}*/
			appList.add(appBean);
		}
		return appList;
	}
	
	
//	private int addItemCount = 2;//获取应用列表外的添加项目数1：没有应用市场，2：添加应用市场和应用管理
//	private void updataUi(ArrayList<AppBean> allApps){
//		isMeasured = false;
//		CreateViewId = 0;
//		View convertView = null;
//		appGridView = new ArrayList<AppItemClass>();
//		mGridLayout.removeAllViews();
//		PackageManager pm = mContext.getPackageManager();
//		int MarginRight = (int)mContext.getResources().getDimension(R.dimen.view_margin);
//		int endColum = (allApps.size()+addItemCount)/2;
//        for (int i = 0; i < allApps.size()+addItemCount; i++) {
//    		int MarginTop = 0;
//        	convertView = LayoutInflater.from(mContext).inflate(R.layout.app_grid_item, null);
//        	convertView.setId(CreateViewId++);
//			convertView.setClickable(true);
//			convertView.setFocusable(true);
//			convertView.setFocusableInTouchMode(true);
//			convertView.setOnFocusChangeListener(this);
//			convertView.setOnClickListener(this);
//			ImageView image = (ImageView) convertView.findViewById(R.id.app_icon);
//			TextView text = (TextView) convertView.findViewById(R.id.app_name);
//			
//        	GridLayout.LayoutParams gllpTv = new GridLayout.LayoutParams(); 
//        	MarginTop = (i%2 != 0 || i == 0)? ItemMargin:MarginRight;
//        	if(i > (addItemCount-1)){
//        		int j = i - addItemCount;
//        		gllpTv.rowSpec = GridLayout.spec(i%2 != 0? 0:1);
//        		gllpTv.columnSpec = GridLayout.spec((i+1)/2, 1); 
//        		gllpTv.width = (int)mContext.getResources().getDimension(R.dimen.square_side);
//        		gllpTv.height = gllpTv.width;
//    	       	gllpTv.setMargins(0, MarginTop, (i+1)/2 != endColum? MarginRight:ItemMargin, 0);
//   			image.setImageDrawable(allApps.get(j).getIcon());
//   			text.setText(allApps.get(j).getName());    	       	
//   			convertView.setBackgroundResource(BACKGROUND_COLOR[i%BACKGROUND_COLOR.length]);
//    			String packageName = allApps.get(j).getPackageName();
//    	       	appGridView.add(new AppItemClass(convertView, pm.getLaunchIntentForPackage(packageName)));
//        	}else if(i == 0){ //应用市场项
//        		FrameLayout.LayoutParams app_mark_layoutparams = new FrameLayout.LayoutParams(
//        				(int)mContext.getResources().getDimension(R.dimen.rectangle_vertical_width), 
//        				(int)mContext.getResources().getDimension(R.dimen.rectangle_vertical_height));
//        		image.setLayoutParams(app_mark_layoutparams);
//        		gllpTv.rowSpec = GridLayout.spec(0, 2);
//            	gllpTv.columnSpec = GridLayout.spec(0, 1);
//    	       	gllpTv.setMargins(ItemMargin, MarginTop, MarginRight, 0);
//    	       	convertView.setNextFocusRightId(CreateViewId);
//			   	if(AppConfig.getInstance(mContext).isEnableGame())
//			   		convertView.setNextFocusLeftId(R.id.poster_9);
//			   	else
//			   		convertView.setNextFocusLeftId(R.id.microfilm);
//   			image.setImageDrawable(mContext.getResources().getDrawable(R.drawable.ic_apps));
//   			text.setVisibility(View.GONE);
//    	       	//TODO 添加应用市场的Intent
//    	       	
//    	       	appGridView.add(new AppItemClass(convertView, null));
//        	}else{//应用管理项
//        		FrameLayout.LayoutParams all_app_layoutparams = new FrameLayout.LayoutParams(
//        				(int)mContext.getResources().getDimension(R.dimen.square_side), 
//        				(int)mContext.getResources().getDimension(R.dimen.square_side));
//        		image.setLayoutParams(all_app_layoutparams);
//        		gllpTv.rowSpec = GridLayout.spec(0, 1);
//            	gllpTv.columnSpec = GridLayout.spec(1, 1);
//        		gllpTv.width = (int)mContext.getResources().getDimension(R.dimen.square_side);
//        		gllpTv.height = gllpTv.width;
//    	       	gllpTv.setMargins(0, MarginTop, MarginRight, 0);
//   			image.setImageDrawable(mContext.getResources().getDrawable(R.drawable.all_apps));
//   			convertView.setBackgroundResource(BACKGROUND_COLOR[0]);
//   			text.setVisibility(View.GONE);
//    	       	Intent mIntent = new Intent();
//       		mIntent.setComponent(new ComponentName("com.softwinner.tvdsetting", "com.softwinner.tvdsetting.applications.AppManagerActivity"));
//    	       	appGridView.add(new AppItemClass(convertView, mIntent));
//        	}
//        	mGridLayout.addView(convertView, gllpTv);//把控件和布局参数添加到GridLayout 
//		}
//	}
	
	private void updataUi(ArrayList<AppBean> allApps){
		CreateViewId = 0;
		View convertView = null;
		appGridView = new ArrayList<AppItemClass>();
		mGridLayout.removeAllViews();
		PackageManager pm = mContext.getPackageManager();
		int MarginRight = (int)mContext.getResources().getDimension(R.dimen.v3_view_margin_item);
		int endColum = (allApps.size()+addItemCount)/2;
        for (int i = 0; i < allApps.size()+addItemCount; i++) {
    		int MarginTop = 0;
    		if(i > (addItemCount-2) && i < allApps.size()+addItemCount -1){
    			convertView = LayoutInflater.from(mContext).inflate(R.layout.app_grid_item, null);
    		}else if(i == allApps.size()+addItemCount -1){
    			convertView = LayoutInflater.from(mContext).inflate(R.layout.v3_app_grid_item_market, null);
    		}else{
    			convertView = LayoutInflater.from(mContext).inflate(R.layout.v3_app_grid_item_setting, null);
    		}
        	convertView.setId(CreateViewId++);
			convertView.setClickable(true);
			convertView.setFocusable(true);
			convertView.setFocusableInTouchMode(true);
			convertView.setOnFocusChangeListener(this);
			convertView.setOnClickListener(this);
			ImageView image = (ImageView) convertView.findViewById(R.id.app_icon);
			TextView text = (TextView) convertView.findViewById(R.id.app_name);
			
        	GridLayout.LayoutParams gllpTv = new GridLayout.LayoutParams(); 
        	MarginTop = (i%2 != 0 || i == 0)? ItemMargin:MarginRight;
        	if(i > (addItemCount-2) && i < allApps.size()+addItemCount -1){
        		int j = i - addItemCount + 1;
        		gllpTv.rowSpec = GridLayout.spec(i%2 != 0? 0:1);
        		gllpTv.columnSpec = GridLayout.spec((i+1)/2, 1); 
        		gllpTv.width = (int)mContext.getResources().getDimension(R.dimen.v3_square_side);
        		gllpTv.height = gllpTv.width;
    	       	gllpTv.setMargins(0, MarginTop, (i+1)/2 != endColum? MarginRight:ItemMargin, 0);
        		convertView.setBackgroundResource(BACKGROUND_COLOR[(i-1)%BACKGROUND_COLOR.length]);
    	       	image.setImageDrawable(allApps.get(j).getIcon());
    	       	text.setText(allApps.get(j).getName());
    	       	/*if(i == allApps.size()+addItemCount -2){
        	       	convertView.setNextFocusRightId(convertView.getId());
    	       	}*/
    			String packageName = allApps.get(j).getPackageName();
    	       	appGridView.add(new AppItemClass(convertView, pm.getLaunchIntentForPackage(packageName)));
        	}else if(i == allApps.size()+addItemCount -1){//应用市场
        		gllpTv.rowSpec = GridLayout.spec(i%2 != 0? 0:1);
        		gllpTv.columnSpec = GridLayout.spec((i+1)/2, 1); 
        		gllpTv.width = (int)mContext.getResources().getDimension(R.dimen.v3_square_side);
        		gllpTv.height = gllpTv.width;
    	       	gllpTv.setMargins(0, MarginTop, (i+1)/2 != endColum? MarginRight:ItemMargin, 0);
    	       	image.setImageDrawable(mContext.getResources().getDrawable(R.drawable.v3_ic_market));
    	       	text.setText(R.string.v3_appsc_market);
    	       	//convertView.setNextFocusRightId(convertView.getId());
    	       	Intent mIntent = new Intent("android.intent.action.MAIN");
       			//mIntent.setComponent(new ComponentName("com.guozi.appstore", "com.guozi.appstore.StartActivity"));
    	       	//mIntent.setComponent(new ComponentName("com.dangbeimarket", "com.dangbeimarket.activity.WelcomeActivity"));
    	       	Intent startintent = new Intent("android.intent.action.MAIN");//common intent for app market 
    	       	mIntent.addCategory("android.intent.category.APP_MARKET");  
    	       	mIntent.setFlags(Intent.FLAG_ACTIVITY_NEW_TASK);
    	       	appGridView.add(new AppItemClass(convertView, mIntent,APP_TYPE_APPMARKET));
        	}else { //应用管理
        		/*FrameLayout.LayoutParams app_mark_layoutparams = new FrameLayout.LayoutParams(
        				(int)mContext.getResources().getDimension(R.dimen.v3_app_section_icon_size), 
        				(int)mContext.getResources().getDimension(R.dimen.v3_app_section_icon_size));
        		image.setLayoutParams(app_mark_layoutparams);*/
        		gllpTv.rowSpec = GridLayout.spec(0, 2);
            	gllpTv.columnSpec = GridLayout.spec(0, 1);
            	gllpTv.width = (int)mContext.getResources().getDimension(R.dimen.v3_rectangle_vertical_width);
        		gllpTv.height = (int)mContext.getResources().getDimension(R.dimen.v3_rectangle_vertical_height);
            	gllpTv.setMargins(0, MarginTop, MarginRight, 0);
    	       	convertView.setNextFocusRightId(CreateViewId);
    	       	image.setImageDrawable(mContext.getResources().getDrawable(R.drawable.v3_ic_appmanager));
    	       	text.setText(R.string.app_manager);
    	       	text.setGravity(Gravity.CENTER);
    	       	Intent mIntent = new Intent();
       			mIntent.setComponent(new ComponentName("com.softwinner.tvdsetting", "com.softwinner.tvdsetting.applications.AppManagerActivity"));
    	       	appGridView.add(new AppItemClass(convertView, mIntent));
    	       	
        	}
        	mGridLayout.addView(convertView, gllpTv);//把控件和布局参数添加到GridLayout 
		}
	}
	
	//获取应用列表上最后一列上方的ICON的ID
	public int getLastFouseViewId(){
		return CreateViewId -((CreateViewId %2 == 0)? 1:2);
	}

	@Override
	public void reSetSectionSmooth(boolean isSmoothToHead) {
		if(DEBUG) Log.d(TAG, "updateData "+ isSmoothToHead); 
		mLastFouseView = null;
		if (appGridView.get(0).getView().getLeft() != 0) {
//			int dx = (int)(appGridView.get(getLastFouseViewId()).getView().getX() - appGridView.get(0).getView().getX());
			if(isSmoothToHead) {
				scrollTo(0, 0);
				mAnimationView.moveWhiteBorder(appGridView.get(0).getView());
			} else {
				scrollTo((int)appGridView.get(getLastFouseViewId()).getView().getX(), 0);
				mAnimationView.moveWhiteBorder(appGridView.get(getLastFouseViewId()).getView());
			}
		}
	}
	
	@Override
	public void updateFont() {
		for (AppItemClass item : appGridView) {
			TextView text = (TextView) item.view.findViewById(R.id.app_name);
			if(text != null)
				text.setTextSize(Utils.px2dip(mContext, mContext.getResources().getDimension(R.dimen.app_section_name_textsize)));
		}
	}

//	private int preRate = 0;
//	private int section_length = 0;
//	@Override
//	public int getDeltaX(View paramView) {
//		if (0 == section_length)
//			section_length = appGridView.get(7).getView().getLeft() - appGridView.get(0).getView().getLeft();
//		int rate = (paramView.getLeft() - appGridView.get(0).getView().getLeft()) / section_length;
//		int shift = (rate - preRate) * section_length;
//		preRate = rate;
//		return shift;
//	}

	@Override
	public void onClick(View paramView) {
		AppItemClass mAppItemClass= appGridView.get(paramView.getId());
		Intent intent = mAppItemClass.getIntent();
		if(null != intent){
			intent.addFlags(Intent.FLAG_ACTIVITY_NEW_TASK);
		}
		if(mAppItemClass.getAppType() == APP_TYPE_SETTINGS){
			if(null != pageChangeListioner){
				pageChangeListioner.onPageChange(1);
				mLastFouseView = appGridView.get(0).getView();
			}
		}else if(mAppItemClass.getAppType() == APP_TYPE_APPMARKET){
			try{
				mContext.startActivity(intent);
			}catch(ActivityNotFoundException ex){
				Toast.makeText(mContext, R.string.no_app_market_found, Toast.LENGTH_SHORT).show();
				ex.printStackTrace();
			}
			((Activity)mContext).overridePendingTransition(android.R.anim.fade_in,android.R.anim.fade_out);
		}else if(intent != null) {
			try{
				mContext.startActivity(intent);
			}catch(ActivityNotFoundException ex){
				ex.printStackTrace();
			}
			((Activity)mContext).overridePendingTransition(android.R.anim.fade_in,android.R.anim.fade_out);
		}
	}
	
	@SuppressLint("ServiceCast") @Override
	public void onFocusChange(View paramView, boolean paramBoolean) {
		mLastFouseView = paramView;
		if (DEBUG && paramBoolean)
			Log.d(TAG, "paramView id= " + paramView.getId() + 
					", paramView x= " + paramView.getX() + 
					", scroll x= " + getScrollX());
		mAnimationView.ViewOnFocused(paramView, paramBoolean, null);
		
		Log.d(TAG, "isOnKey ＝ "+isOnKey);
		Log.d(TAG, "mLastOnKey ＝ "+mLastOnKey+";"+System.currentTimeMillis());
		Log.d(TAG, "delta = "+ (mLastOnKey - System.currentTimeMillis()));
		if (paramBoolean && isOnKey){
			if(System.currentTimeMillis() - mLastOnKey < keyEffectDelay){
				smoothScrooll(paramView);
			}else{
				isOnKey = false;
			}
		}

	}
	
	@Override
	public int getShadowSelect(int onFocusViewId) {
		if(0 == onFocusViewId)
			return AnimationView.SHADOW_SIDE_HORIZONTAL;
		else
			return AnimationView.SHADOW_CENTER;
	}
	
	public ArrayList<AppItemClass> getAppGridView(){
		return appGridView;
	}
	
	private class AppDbDataChangeReceiver extends BroadcastReceiver {
		@Override
		public void onReceive(Context context, Intent intent) {
			topApps = getAppList(new AppListBiz(mContext).getLauncherApps());
			updataUi(topApps);
			invalidate();
			if(mLastFouseView != null)
				appGridView.get(0).getView().requestFocus();
		}
	}
	
	public class AppItemClass{
		private View view;
		private Intent intent;
		private int appType = APP_TYPE_NORMAL;
		
		public AppItemClass(View view, Intent intent) {
			super();
			this.view = view;
			this.intent = intent;
		}
		
		public AppItemClass(View view, Intent intent,int type) {
			super();
			this.view = view;
			this.intent = intent;
			this.appType = type;
		}

		public View getView() {
			return view;
		}
		
		public Intent getIntent(){
			return intent;
		}
		public void setAppType(int type){
			appType = type;
		}
		public int getAppType(){
			return appType;
		}
	}

	@Override
	public boolean isOnTop() {
		// TODO Auto-generated method stub
		for(int i = 0;i<appGridView.size();i++){
			if((i == 0) && (appGridView.get(i).getView().isFocused())){
				return true;
			}else if((i%2 == 1)&& (appGridView.get(i).getView().isFocused())){
				return true;
			}
		}
		return false;
	}

}
