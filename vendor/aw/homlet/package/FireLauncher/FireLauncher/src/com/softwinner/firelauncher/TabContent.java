package com.softwinner.firelauncher;

import java.util.ArrayList;

import android.content.Context;
import android.util.AttributeSet;
import android.widget.HorizontalScrollView;

import com.android.launcher2.PagedView;

public class TabContent extends PagedView {
//	private String TAG = "TabContent";
	
	private ArrayList<OnPageMovingListener> mPageMovingListenerList = null;

	public interface OnPageMovingListener {
		void onPageBeginMoving(int curPage, int aimPage);
		void onPageEndMoving(int curPage);
	}
	
	public TabContent(Context context) {
		this(context, null);
    }

    public TabContent(Context context, AttributeSet attrs) {
    	this(context, attrs, 0);
    }


	public TabContent(Context context, AttributeSet attrs, int defStyle) {
		super(context, attrs, defStyle);
		mContentIsRefreshable = false;
//		mFadeInAdjacentScreens = false;
		setDataIsReady();
		setPageSpacing(getResources().getDimensionPixelSize(R.dimen.page_space));
		mPageMovingListenerList = new ArrayList<OnPageMovingListener>();
	}
	

	@Override
	public void syncPages() {
		// TODO Auto-generated method stub
	}

	@Override
	public void syncPageItems(int page, boolean immediate) {
		// TODO Auto-generated method stub

	}
	
	public void snapToPage2(int page) {
		if(page == mCurrentPage)return;
		snapToPage(page);
		HorizontalScrollView view = (HorizontalScrollView)getChildAt(page);
		view.smoothScrollTo(0, 0);
	}

	public void setOnPageMovingListener(OnPageMovingListener listener) {
		if(listener == null) return;
		mPageMovingListenerList.add(listener);
	}
	
	public int getCurrentPageIndex(){
		return mCurrentPage;
	}
	
	@Override
	protected void onPageBeginMoving() {
		// TODO Auto-generated method stub
		if(mPageMovingListenerList.size() > 0 && mNextPage != INVALID_PAGE) {
			for (OnPageMovingListener mPageMovingListener : mPageMovingListenerList) {
				mPageMovingListener.onPageBeginMoving(mCurrentPage,mNextPage);
			}
		}
	}
	
	@Override
	protected void onPageEndMoving() {
		// TODO Auto-generated method stub
		if(mPageMovingListenerList.size() > 0) {
			for (OnPageMovingListener mPageMovingListener : mPageMovingListenerList) {
				mPageMovingListener.onPageEndMoving(mCurrentPage);
			}
		}
	}


}
