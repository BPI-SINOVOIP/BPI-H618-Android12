package com.softwinner.firelauncher.section;

import android.app.Activity;
import android.content.Context;
import android.util.AttributeSet;
import android.util.DisplayMetrics;
import android.util.Log;
import android.view.KeyEvent;
import android.view.View;
import android.widget.HorizontalScrollView;
import android.widget.Scroller;

import com.softwinner.animationview.AnimationParent.OnShadowProcessor;
import com.softwinner.firelauncher.R;

public abstract class v3_SectionParent extends HorizontalScrollView implements View.OnFocusChangeListener, 
						View.OnClickListener, OnShadowProcessor{
	private final String TAG = "SectionParent";
	private boolean DEBUG = true;
	private Context mContext;
	public int screenWidth;
	public Scroller mScroller = null;
	public View mLastFouseView;
	public boolean isAnimationAble = true;
	private int lastScrollerX = 0;
	private int lastScrollerY = 0;
	private long onKeyDown_lastTime = 0;
	private int lastKeyAction = KeyEvent.ACTION_UP;
	private int clickDelayTime = 0;
	private static View lastSrcollView = null;
	public PageChangeInterface pageChangeListioner = null;
	public onTopInterface mOnTop = null;
	public boolean isOnKey = false;
	public long mLastOnKey;
	public static long keyEffectDelay = 800;
	
	/**
	 * Init view of section_sub:初始化子视图
	 **/
	public abstract void initView();
	
	/**
	 * reset scroll the section_sub after PageView snapped:翻页时更新子视图的显示位置
	 * @param isSmoothToHead
	 * 		是否滑动的section的头部
	 * */
	public abstract void reSetSectionSmooth(boolean isSmoothToHead);
	
	/**
	 * update TextView Font while configChanges be changed by fontScale:修改系统字体大小时调用   
	 * */
	public abstract void updateFont();
	
	public abstract void destroy();
	
	/**
	 * focus will leaving this section on top,top dialog will show:是否顶弹出窗口 
	 * */
	public abstract boolean isOnTop();
	
//	public abstract int getDeltaX(View paramView);
	
	public v3_SectionParent(Context context) {
		super(context);
		mContext = context;
	}

	public v3_SectionParent(Context context, AttributeSet attrs) {
		this(context, attrs, 0);
		mContext = context;
	}

	public v3_SectionParent(Context context, AttributeSet attrs, int defStyle) {
		super(context, attrs, defStyle);
		mContext = context;
		
		DisplayMetrics metric = new DisplayMetrics();
        ((Activity)context).getWindowManager().getDefaultDisplay().getMetrics(metric);
        screenWidth = metric.widthPixels;  
        clickDelayTime = getResources().getInteger(R.integer.default_long_click_delay);
        setOverScrollMode(View.OVER_SCROLL_NEVER);
	}
	
	public void onResume(){
		// TODO Auto-generated method stub
		isAnimationAble = true;
	}
	
	public void onPause(){
		// TODO Auto-generated method stub
	}
	
	public void onDestroy(){
		// TODO Auto-generated method stub
	}
	
	public View getLastFouseView(){
		return mLastFouseView;
	}
	
	public void setLastFouseView(View view){
		mLastFouseView = view;
	}
	
	@Override
	public boolean dispatchKeyEvent(KeyEvent event) {
		if(!isAnimationAble) return true;
    	boolean ret = true; 
        long current = System.currentTimeMillis();
        int keyAction = event.getAction();
		if((event.getKeyCode() == KeyEvent.KEYCODE_DPAD_UP) && (event.getAction() == KeyEvent.ACTION_DOWN)
		           &&isOnTop() && mOnTop != null){
            mOnTop.onTop();
		}
        if(isIgnoreKeyCode(event.getKeyCode(),keyAction,current)) {
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
	
	private boolean isIgnoreKeyCode(int keyCode, int keyAction, long current){
		switch (keyCode) {
		case KeyEvent.KEYCODE_DPAD_DOWN:
		case KeyEvent.KEYCODE_DPAD_UP:
		case KeyEvent.KEYCODE_DPAD_RIGHT:
		case KeyEvent.KEYCODE_DPAD_LEFT:
			isOnKey = true;
			mLastOnKey = current;
			Log.d(TAG, "isOnKey ＝ "+isOnKey);
			return keyAction == lastKeyAction 
				&& current - onKeyDown_lastTime < clickDelayTime;
		default:
			isOnKey = false;
			return false;
		}
	}
	
	public void smoothScrollBy2(int dx, int dy, int duration) {
		if(0 == dx && 0 == dy )
			return;
		if(null == mScroller)
			mScroller = new Scroller(mContext);
		
		lastScrollerX = 0;
		lastScrollerY = 0;
		mScroller.startScroll(0, 0, dx, dy, duration);
		invalidate();
	}
	
	@Override
	public void computeScroll() {
		if(null == mScroller) return;
		if(mScroller.computeScrollOffset()) {
			int dx = mScroller.getCurrX() - lastScrollerX;
			int dy = mScroller.getCurrY() - lastScrollerY;
//				Log.w(TAG, "X "+mScroller.getCurrX() + ", dx "+ dx);
			lastScrollerX = mScroller.getCurrX();
			lastScrollerY = mScroller.getCurrY();
			scrollBy(dx, dy);
			invalidate();
		} else {
			mScroller = null;
		}
	}
	
	/**
	 * @param paramView
	 * 		聚焦的view
	 * @param delta
	 * 		开始移动page的位置(< screenWidth)，默认为screenWidth/2*/
	public void smoothScrooll(View paramView,int delta){
		if(paramView.equals(lastSrcollView))return;
		int scrollX = getScrollX();
		int viewRight = paramView.getRight();
		int viewLeft = paramView.getLeft();
		if(DEBUG) Log.d(TAG, "scrollX " +scrollX + ", view left " + viewLeft + ", right "+ viewRight+", delta "+delta);
		if( scrollX+screenWidth < viewRight+delta ) {
			smoothScrollBy2(viewRight-(scrollX+screenWidth)+delta, 0, 300);
		} else if( scrollX > viewLeft-delta ) {
			smoothScrollBy2(viewLeft-scrollX-delta, 0, 300);
		}
		lastSrcollView = paramView;
	}
	
	public void smoothScrooll(View paramView){
		smoothScrooll(paramView, screenWidth/2);
	}
	
	public void setPageChangeListioner(PageChangeInterface l){
		pageChangeListioner = l;
	}
	
	public interface PageChangeInterface {
		public void onPageChange(int next);
	}
	
	public void setOnTopListioner(onTopInterface l){
		mOnTop = l;
	}
	
	public interface onTopInterface {
		public void onTop();
	}
}
