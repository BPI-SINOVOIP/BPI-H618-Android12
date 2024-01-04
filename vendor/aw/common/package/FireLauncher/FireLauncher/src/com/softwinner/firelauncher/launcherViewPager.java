package com.softwinner.firelauncher;

import java.lang.reflect.Field;
import java.util.ArrayList;
import java.util.List;

import com.softwinner.firelauncher.section.v3_SectionParent;
import com.softwinner.firelauncher.section.SettingSection;
import com.softwinner.firelauncher.section.v3_AppSection;
import com.softwinner.firelauncher.section.v3_SettingSection;

import android.annotation.SuppressLint;
import android.app.Activity;
import android.content.Context;
import android.support.v4.view.PagerAdapter;
import android.support.v4.view.ViewPager;
import android.util.AttributeSet;
import android.util.Log;
import android.view.InputDevice;
import android.view.MotionEvent;
import android.view.VelocityTracker;
import android.view.View;
import android.view.animation.AccelerateInterpolator;
import android.view.animation.Interpolator;
import android.widget.ImageView;
import android.widget.LinearLayout;
import android.widget.Scroller;
import android.widget.TextView;

public class launcherViewPager extends ViewPager implements v3_SectionParent.PageChangeInterface{
	
	private static String TAG = "launcherViewPager";
	private List<v3_SectionParent> lists = new ArrayList<v3_SectionParent>();
	private sectionAdapter myAdapter;
	private int curPosition = 0;
	private FixedSpeedScroller scroller = null;
	private ImageView[] dotViews;
	LinearLayout mDotGroup = null;
	Activity mActivity = null;
	private float preX;
	
	public launcherViewPager(Context context) {
		super(context);
		// TODO Auto-generated constructor stub
	}
	
	public launcherViewPager(Context context, AttributeSet attrs) {
		super(context, attrs);
		// TODO Auto-generated constructor stub
	}
	
	@Override
	public void onPageChange(int next) {
		// TODO Auto-generated method stub
		launcherViewPager.this.setCurrentItem(next);
	}

	public void setUp(Activity activity){
		mActivity = activity;
        lists.add((v3_AppSection)activity.getLayoutInflater().inflate(R.layout.v3_appsection, null));
        lists.add((v3_SettingSection)activity.getLayoutInflater().inflate(R.layout.v3_setting_section, null));
        for(int i = 0;i<lists.size();i++){
        	lists.get(i).initView();
        	lists.get(i).setPageChangeListioner(this);
        	if(activity instanceof v3_SectionParent.onTopInterface ){
        		Log.d(TAG, "setOnTopListioner");
        		lists.get(i).setOnTopListioner((v3_SectionParent.onTopInterface)activity);
        	}
        }
        setOverScrollMode(View.OVER_SCROLL_NEVER);
        myAdapter = new sectionAdapter(lists);
		this.setAdapter(myAdapter);
		this.setOnPageChangeListener(new sectionListioner());
		this.setCurrentItem(curPosition);
		this.setPageTransformer(true, new DepthPageTransformer());
		setScrollerTime(600);
	}
	
	public void setUp(Activity activity,LinearLayout dotGroup){
		setUp(activity);
		dotViews = new ImageView[lists.size()];
		mDotGroup = dotGroup;
        
        for (int i = 0; i < lists.size(); i++) {  
            ImageView iv = new ImageView(activity);
            int dot= getResources().getInteger(R.integer.v3_dot_size);
            LinearLayout.LayoutParams lp = new LinearLayout.LayoutParams(dot,dot);
            int dot_pading= getResources().getInteger(R.integer.v3_dot_padding);
            lp.setMargins(dot_pading, 0, dot_pading, 0);
            iv.setLayoutParams(lp);  
            //iv.setPadding(10, 0, 10, 0);
            dotViews[i] = iv;  
            if(i == 0){                    
            	dotViews[i].setBackgroundResource(R.drawable.dot_focus);  
            }else{  
            	dotViews[i].setBackgroundResource(R.drawable.dot_unfocus);  
            }  
            mDotGroup.addView(iv);  
        }
	}
	
	public boolean isEdge(){
		
		if(this.getCurrentItem()== 0 || this.getCurrentItem()== lists.size()-1)
			return true;
		else
			return false;	
	}
	
	public void onResume(){
		if(this.getCurrentItem()!=curPosition){
			this.setCurrentItem(curPosition);
		}
		lists.get(curPosition).onResume();
	}
	
	public void onPause(){
		for(int i = 0;i <lists.size();i++){
			lists.get(i).onPause();
		}
	}
	
	public void onDestroy(){
		for(int i = 0;i <lists.size();i++){
			lists.get(i).onDestroy();
		}
	}
	
	public class sectionAdapter extends PagerAdapter{

	    List<v3_SectionParent> viewLists;
	    
	    public sectionAdapter(List<v3_SectionParent> lists)
	    {
	        viewLists = lists;
	    }

	    @Override
	    public int getCount() {
	        // TODO Auto-generated method stub
	        return viewLists.size();
	    }

	    @Override
	    public boolean isViewFromObject(View arg0, Object arg1) {                         
	        // TODO Auto-generated method stub
	        return arg0 == arg1;
	    }
	    
	    @Override
	    public void destroyItem(View view, int position, Object object){
	    	((ViewPager) view).removeView(viewLists.get(position));
	    }
	    
	    @Override
	    public Object instantiateItem(View view, int position){
	        ((ViewPager) view).addView(viewLists.get(position), 0);
	        Log.d(TAG, "instantiateItem " + position);
	        return viewLists.get(position);
	    }
	    
	}

	public class sectionListioner implements ViewPager.OnPageChangeListener{
	    @Override
	    public void onPageSelected(int arg0) {
	        // TODO Auto-generated method stub
	        curPosition = arg0;
	        if(dotViews == null || dotViews.length < 1 || lists == null)
	        	return ;
	        if(arg0 > dotViews.length){  
	        	arg0 = arg0 % dotViews.length;  
	        }
	        //Log.d(TAG, " onPageSelected realItem = " + arg0);
	        for (int i = 0; i < dotViews.length; i++) {  
	        	if(arg0 == i)  
	        		dotViews[i].setBackgroundResource(R.drawable.dot_focus);  
	        	else
	        		dotViews[i].setBackgroundResource(R.drawable.dot_unfocus);
	        }  
	    }
	    
	    @Override
	    public void onPageScrolled(int arg0, float arg1, int arg2) {
	        // TODO Auto-generated method stub
	    }
	    
	    @Override
	    public void onPageScrollStateChanged(int arg0) {
	        // TODO Auto-generated method stub
	        
	    }
	}
	
	public class DepthPageTransformer implements ViewPager.PageTransformer  
	{  
	    private static final float MIN_SCALE = 0.75f;  
	  
	    public void transformPage(View view, float position)  
	    {  
	        int pageWidth = view.getWidth();  
	  
	        if (position < -1)  
	        { // [-Infinity,-1)  
	            // This page is way off-screen to the left.  
	            // view.setAlpha(0);  
	        	view.setAlpha(0);  
	        } else if (position <= 0)// a页滑动至b页 ； a页从 0.0 -1 ；b页从1 ~ 0.0  
	        { // [-1,0]  
	            // Use the default slide transition when moving to the left page  
	            // view.setAlpha(1);  
	        	view.setAlpha(1);  
	            // view.setTranslationX(0);  
	        	view.setTranslationX(0);  
	            // view.setScaleX(1);  
	        	view.setScaleX(1+position);  
	            // view.setScaleY(1);  
	        	view.setScaleY(1+position);  
	  
	        } else if (position <= 1)  
	        { // (0,1]  
	            // Fade the page out.  
	            // view.setAlpha(1 - position);  
	        	view.setAlpha(1 - position);  
	  
	            // Counteract the default slide transition  
	            // view.setTranslationX(pageWidth * -position);  
	        	view.setTranslationX(pageWidth * -position);  
	  
	            // Scale the page down (between MIN_SCALE and 1)  
	            float scaleFactor = MIN_SCALE + (1 - MIN_SCALE) * (1 - position);  
	            // view.setScaleX(scaleFactor);  
	            view.setScaleX(scaleFactor);  
	            // view.setScaleY(1);  
	            view.setScaleY(scaleFactor);  
	  
	        } else  
	        { // (1,+Infinity]  
	            // This page is way off-screen to the right.  
	            // view.setAlpha(0);  
	        	view.setAlpha(1);  
	        }  
	    }  
	}
	
	public class FixedSpeedScroller extends Scroller {
		 
	    private int mDuration =500;
	     
	    public void setTime(int scrollerTime){
	        mDuration=scrollerTime;
	    }
	    public FixedSpeedScroller(Context context) {
	        super(context);
	    }
	 
	    public FixedSpeedScroller(Context context, Interpolator interpolator) {
	        super(context, interpolator);
	    }
	 
	    @SuppressLint("NewApi") public FixedSpeedScroller(Context context, Interpolator interpolator, boolean flywheel) {
	        super(context, interpolator, flywheel);
	    }
	 
	 
	    @Override
	    public void startScroll(int startX, int startY, int dx, int dy, int duration) {
	        // Ignore received duration, use fixed one instead
	        super.startScroll(startX, startY, dx, dy, mDuration);
	    }
	 
	    @Override
	    public void startScroll(int startX, int startY, int dx, int dy) {
	        // Ignore received duration, use fixed one instead
	        super.startScroll(startX, startY, dx, dy, mDuration);
	    }
	}
	
    public void setScrollerTime(int scrollerTime){
        try {
            if(scroller!=null){
                 scroller.setTime(scrollerTime);
            }else{
                Field mScroller;
                mScroller = ViewPager.class.getDeclaredField("mScroller");
                mScroller.setAccessible(true); 
                scroller= new FixedSpeedScroller(this.getContext(),new AccelerateInterpolator());
                scroller.setTime(scrollerTime);
                mScroller.set(this, scroller);
            } 
        } catch (Exception e) {
        } 
    }
    
    @Override
    public boolean onGenericMotionEvent(MotionEvent event) {
    	if (0 != (event.getSource() & InputDevice.SOURCE_CLASS_POINTER)) {
    		switch (event.getAction()) {
    			case MotionEvent.ACTION_SCROLL:   
    				if( event.getAxisValue(MotionEvent.AXIS_VSCROLL) < 0.0f){
    					//获得垂直坐标上的滚动方向,也就是滚轮向下滚  
    					Log.i("fortest::onGenericMotionEvent", "down" );
    					int next = getCurrentItem()+1;
    					if(next < lists.size()&& next >= 0){
        					setCurrentItem(next);
    					}
    				}else{
        				//获得垂直坐标上的滚动方向,也就是滚轮向上滚
    					Log.i("fortest::onGenericMotionEvent", "up" );
    					int next = getCurrentItem()-1;
    					if(next < lists.size()&& next >= 0){
        					setCurrentItem(next);
    					}
    				}
                return true;
            }
    	}
    	return super.onGenericMotionEvent(event);
    }
    
    @Override
    public boolean onInterceptTouchEvent(MotionEvent event) {
    	boolean res = super.onInterceptTouchEvent(event);
    	if(event.getAction() == MotionEvent.ACTION_DOWN) {
    		preX = event.getX();
    		} else {
    			if( Math.abs(event.getX() - preX)> 10 ) {
    				return true;
    			} else {
    				preX = event.getX();
    			}
    	}
    	return res;
    }
}
