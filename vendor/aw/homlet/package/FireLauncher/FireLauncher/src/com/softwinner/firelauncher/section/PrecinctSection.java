package com.softwinner.firelauncher.section;

import com.softwinner.animationview.AnimationView;
import com.softwinner.firelauncher.R;
import com.softwinner.firelauncher.network.HttpJsonLoader.onJsonLoaderFinishListener;

import android.content.Context;
import android.util.AttributeSet;
import android.util.Log;
import android.view.View;
import android.widget.ImageView;


public class PrecinctSection extends SectionParent implements onJsonLoaderFinishListener{
	private String TAG = "PrecinctSection";
	private boolean DEBUG = true;
	
	private Context mContext;
	private ImageView precinct_3d;
	private View precinct_1080p;
	private View precinct_blu_ray;
	private ImageView precinct_mango;
	private AnimationView mAnimationView;
	
	public PrecinctSection(Context context) {
		super(context);
		mContext = context;
	}

	public PrecinctSection(Context context, AttributeSet attrs) {
		this(context, attrs, 0);
		mContext = context;
	}
	
	public PrecinctSection(Context context, AttributeSet attrs, int defStyle) {
		super(context, attrs, defStyle);
		mContext = context;
	}
	
	@Override
	public void initView() {
		//*******************第一版面************************
		precinct_1080p = findViewById(R.id.precinct_1080p);
		precinct_1080p.setOnFocusChangeListener(this);
		precinct_1080p.setOnClickListener(this);
		
		precinct_blu_ray = findViewById(R.id.precinct_blu_ray);
		precinct_blu_ray.setOnFocusChangeListener(this);
		precinct_blu_ray.setOnClickListener(this);
		
		precinct_3d = (ImageView) findViewById(R.id.precinct_3d);
		precinct_3d.setOnFocusChangeListener(this);
		precinct_3d.setOnClickListener(this);
		
		precinct_mango = (ImageView) findViewById(R.id.precinct_mango);
		precinct_mango.setOnFocusChangeListener(this);
		precinct_mango.setOnClickListener(this);
		
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
		
	}

	@Override
	public void onClick(View paramView) {
//		isAnimationAble = false;
	}

	@Override
	public void onFocusChange(View paramView, boolean paramBoolean) {
		mLastFouseView = paramView;
		if(DEBUG && paramBoolean) Log.d(TAG, "paramView x "+ paramView.getX() + ", scroll x "+ getScrollX() );
		
		mAnimationView.ViewOnFocused(paramView, paramBoolean, null);
	}

	@Override
	public int getShadowSelect(int onFocusViewId) {
		if(R.id.precinct_1080p == onFocusViewId || R.id.precinct_blu_ray == onFocusViewId)
			return AnimationView.SHADOW_SIDE_HORIZONTAL;
		else
			return AnimationView.SHADOW_CENTER;
	}

	@Override
	public void onJsonLoaderFinish(int type, int addedSize) {
		// TODO Auto-generated method stub
		
	}

	@Override
	public void onNetworkError(String message) {
		// TODO Auto-generated method stub
		
	}

//	@Override
//	public int getDeltaX(View paramView) {
//		// TODO Auto-generated method stub
//		return 0;
//	}
	
}
