package com.softwinner.firelauncher.section;

import java.util.ArrayList;

import com.softwinner.animationview.AnimationView;
import com.softwinner.firelauncher.R;
import com.softwinner.firelauncher.utils.Constants;

import android.app.Activity;
import android.content.BroadcastReceiver;
import android.content.ComponentName;
import android.content.Context;
import android.content.Intent;
import android.content.IntentFilter;
import android.util.AttributeSet;
import android.util.Log;
import android.view.View;
import android.widget.ImageView;
import android.widget.TextView;

public class SettingSection extends SectionParent {
	private String TAG = "SettingSection";
	private boolean DEBUG = true;
	
	private Context mContext;
	private AppDbDataChangeReceiver receiver;
	
	private ArrayList<View> viewList;
	private TextView system_setting;
	private TextView network_settings;
	private TextView system_update;
	private TextView plot_setting;
	private TextView about_us;
	private AnimationView mAnimationView;
	
	public SettingSection(Context context) {
		super(context);
		mContext = context;
	}

	public SettingSection(Context context, AttributeSet attrs) {
		this(context, attrs, 0);
		mContext = context;
	}
	
	public SettingSection(Context context, AttributeSet attrs, int defStyle) {
		super(context, attrs, defStyle);
		mContext = context;
		
		receiver = new AppDbDataChangeReceiver();
		IntentFilter filter = new IntentFilter();
		filter.addAction(Constants.APP_DB_CHANGE);
		context.registerReceiver(receiver, filter);
	}
	
	@Override
	public void initView() {
		viewList = new ArrayList<View>();
		
		//*******************第一版面************************
		network_settings = (TextView) findViewById(R.id.network_settings);
		network_settings.setOnFocusChangeListener(this);
		network_settings.setOnClickListener(this);
		viewList.add(network_settings);
		
		system_update = (TextView) findViewById(R.id.system_update);
		system_update.setOnFocusChangeListener(this);
		system_update.setOnClickListener(this);
		viewList.add(system_update);
		
		system_setting = (TextView) findViewById(R.id.system_setting);
		system_setting.setOnFocusChangeListener(this);
		system_setting.setOnClickListener(this);
		viewList.add(system_setting);
		
		plot_setting = (TextView) findViewById(R.id.plot_setting);
		plot_setting.setOnFocusChangeListener(this);
		plot_setting.setOnClickListener(this);
		viewList.add(plot_setting);
		
		about_us = (TextView) findViewById(R.id.about_us);
		about_us.setOnFocusChangeListener(this);
		about_us.setOnClickListener(this);
		viewList.add(about_us);
		
		mAnimationView = new AnimationView(mContext, findViewById(R.id.animation_layout), false);
		mAnimationView.setShadowProcessor(this);
	}
	
	@Override
	public void reSetSectionSmooth(boolean isSmoothToHead) {
		// TODO Auto-generated method stub
		mLastFouseView = null;
	}
	
	@Override
	public void updateFont() {
		// TODO Auto-generated method stub
		
	}

	@Override
	public void destroy() {
		// TODO Auto-generated method stub
		mContext.unregisterReceiver(receiver);
	}

	@Override
	public void onClick(View paramView) {
//		isAnimationAble = false;
		int id = paramView.getId();
		Intent it = new Intent();
		it.setFlags(Intent.FLAG_ACTIVITY_NEW_TASK);
		switch (id) {
		case R.id.network_settings:
			it.setComponent(new ComponentName("com.softwinner.tvdsetting", "com.softwinner.tvdsetting.net.NetSettings"));
			break;
		case R.id.system_update:
			it.setComponent(new ComponentName("com.softwinner.update", "com.softwinner.update.HomeActivity"));
			break;
		case R.id.system_setting:
			it.setComponent(new ComponentName("com.softwinner.tvdsetting", "com.softwinner.tvdsetting.CommonSettings"));
			break;
		case R.id.plot_setting:
			it.setComponent(new ComponentName("com.softwinner.tvdsetting", "com.softwinner.tvdsetting.DisplaySetting"));
			break;
		case R.id.about_us:
			it.setComponent(new ComponentName("com.softwinner.tvdsetting", "com.softwinner.tvdsetting.about.AboutSetting"));
			break;
		default:
			break;
		}
		mContext.startActivity(it);
		((Activity)mContext).overridePendingTransition(android.R.anim.fade_in,android.R.anim.fade_out);
	}

	@Override
	public void onFocusChange(View paramView, boolean paramBoolean) {
		mLastFouseView = paramView;
		if(DEBUG && paramBoolean) Log.d(TAG, "paramView x "+ paramView.getX() + ", scroll x "+ getScrollX() );
		mAnimationView.ViewOnFocused(paramView, paramBoolean, null);
	}

	@Override
	public int getShadowSelect(int onFocusViewId) {
		if(R.id.network_settings == onFocusViewId || R.id.system_update == onFocusViewId)
			return AnimationView.SHADOW_SIDE_HORIZONTAL;
		else
			return AnimationView.SHADOW_CENTER;
	}
	
	private class AppDbDataChangeReceiver extends BroadcastReceiver {
		@Override
		public void onReceive(Context context, Intent intent) {
//			Log.d(TAG, "onReceive onReceive onReceive onReceive");
		}
	}

//	@Override
//	public int getDeltaX(View paramView) {
//		// TODO Auto-generated method stub
//		return 0;
//	}
	
}
