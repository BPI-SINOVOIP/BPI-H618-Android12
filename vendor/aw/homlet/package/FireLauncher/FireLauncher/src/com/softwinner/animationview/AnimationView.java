package com.softwinner.animationview;

import java.util.ArrayList;

import com.softwinner.firelauncher.R;
import com.softwinner.firelauncher.utils.Utils;

import android.animation.Animator;
import android.animation.Animator.AnimatorListener;
import android.content.Context;
import android.util.Log;
import android.view.SoundEffectConstants;
import android.view.View;
import android.view.ViewTreeObserver;
import android.view.animation.Animation.AnimationListener;
import android.widget.FrameLayout;

public class AnimationView extends AnimationParent {
	private final String TAG = "AnimationView";
	
	public static final int VIEW_TOP = 1;
	public static final int VIEW_CENTER = 2;
	public static final int VIEW_BOTTOM = 3;
	private static final int CHECK_ONfOCUS_DELAY = 10;
	private Context mContext;
	private boolean isItemNameAble = false;
	private boolean isViewFocused = false;
	private boolean isMeasured = false;
	private AnimationText item_name;
	private View mParentView;
	private View selectViewPre = null;
	private float tabindicator_height = -1F;
	private ArrayList<AddView> addViewInAnimation;

	public AnimationView(Context context, View ParentView, boolean isEnableItemName) {
		super(context, ParentView);
		mContext = context;
		mParentView = ParentView;
		isItemNameAble = isEnableItemName;
		addViewInAnimation = new ArrayList<AddView>();
		if(isItemNameAble){
			item_name = (AnimationText) mParentView.findViewById(R.id.item_name);
			tabindicator_height = mContext.getResources().getDimension(R.dimen.tabindicator_height);
		}
	}
	
	public void refreshFont(){
		if(isItemNameAble && item_name != null){
			item_name.setTextSize(Utils.px2dip(mContext, mContext.getResources().getDimension(R.dimen.app_section_name_textsize)));
		}
	}
	
	public AddView addViewOnAnimation(View view, int width, int height, int position){
		AddView mAddView = new AddView();
		mAddView.aimView = view;
//		mAddView.aimWidth = width;
//		mAddView.aimHeight = height;
		mAddView.position = position;
		addViewInAnimation.add(mAddView);
		return mAddView;
	}
	
	public void setitemAnimationIconId(int itemAnimationIconId) {
		this.ItemAnimationIconId = itemAnimationIconId;
	}
	
	public void scaleWhiteBorder(int width, int height, int[] setXY){
		super.scaleWhiteBorder(width, height, setXY);
		if(isItemNameAble){
			FrameLayout.LayoutParams text_layoutparams = new FrameLayout.LayoutParams((int) (width * magnification), (int)tabindicator_height);
			item_name.setLayoutParams(text_layoutparams);
			item_name.setX(setXY[0] + (defaultBorderWidth - width * magnification)/2);
			item_name.setY(setXY[1] - tabindicator_height + (defaultBorderHeight + height * magnification)/2);
		}
		moveAddView(width, height, setXY);
	}
	
	public void moveAddView(int width, int height, int[] setXY){
		if(addViewInAnimation.size()>0)
			for (AddView addView : addViewInAnimation) {
				addView.move(width, height, setXY);
			}
	}
	
	Runnable mAutoCheckFocusRunnable = new Runnable(){
		@Override
		public void run() {
			if(!isViewFocused){
				if(selectViewCurr != null && selectViewCurr.getAnimation()!=null){
					whiteBorder.setVisibility(View.VISIBLE);
					getshadowView(selectViewCurr).setVisibility(View.VISIBLE);
				}else
					whiteBorder.setVisibility(View.INVISIBLE);
			}
		}
    };
    
	private void resetutoCheckFocus() {
    	mHandler.removeCallbacks(mAutoCheckFocusRunnable);
    	mHandler.postDelayed(mAutoCheckFocusRunnable, CHECK_ONfOCUS_DELAY);
    }
	
	public void recoverLooseFocusAinimation(View selectView) {
		super.recoverLooseFocusAinimation(selectView);
		for (AddView addView : addViewInAnimation) {
			addView.aimView.setVisibility(View.GONE);
		}
		if(isDeleteMode)
			trashIcon.setVisibility(View.GONE);
		if(isItemNameAble)
			item_name.setVisibility(View.GONE);
		resetutoCheckFocus();
	}

	public void setViewObserver(final View selectView){
		ViewTreeObserver observer = selectView.getViewTreeObserver();
		observer.addOnPreDrawListener(new ViewTreeObserver.OnPreDrawListener() {
		    public boolean onPreDraw() {
		        if (!isMeasured) {
		        	sendMessage(MSG_FLY_BORDER, selectView, 0);
		          isMeasured = true;
		        }
		        return true;
		    }
		});
	}
	
	//聚焦监听函数
	public void ViewOnFocused(View view, boolean paramBoolean, String itemName){
		if(null == view) return ;
//		Log.d(TAG, ">>>>Receive:"+" ViewOnFocused ("+paramBoolean+", "+itemName+")");
		isViewFocused = paramBoolean;
		if (paramBoolean) {
			mParentView.bringToFront();//控件前置（阴影部分才能显示）		
			selectViewCurr = view;
			//selectViewCurr.bringToFront();
			if(isItemNameAble){
				item_name.setText(itemName);
			}
			if(selectViewCurr.getWidth() != 0)
				sendMessage(MSG_FLY_BORDER, selectViewCurr, 0);
			else{
				isMeasured = false;
				setViewObserver(selectViewCurr);
			}	
			selectViewCurr.playSoundEffect(SoundEffectConstants.CLICK);//添加按键声音
		} else {
			selectViewPre = view;
			recoverLooseFocusAinimation(selectViewPre);
		}
	}

	@Override
	public AnimatorListener setAnimatorListener(final boolean isRecover) {
		// TODO Auto-generated method stub
		return new AnimatorListener() {
			@Override
			public void onAnimationStart(Animator animator) {
				// TODO Auto-generated method stub
			}
			
			@Override
			public void onAnimationRepeat(Animator animator) {
				// TODO Auto-generated method stub
				
			}
			
			@Override
			public void onAnimationEnd(Animator animator) {
				if(!isViewFocused && selectViewPre != null){
					selectViewPre.clearAnimation();
					getshadowView(selectViewPre).setVisibility(View.INVISIBLE);
					return;
				}
				whiteBorder.setVisibility(View.VISIBLE);					
				if(isFlyFinished == false && !isRecover){
					sendMessage(MSG_SCALE_VIEW, selectViewCurr, DELAY_SCALE_TIME);
					isFlyFinished = true;
				}
				resetutoCheckFocus();
			}
			
			@Override
			public void onAnimationCancel(Animator animator) {
				// TODO Auto-generated method stub
				Log.d(TAG, ">>>>>>>>>onAnimationCancel!!!!!!");
			}
		};
	}

	@Override
	public AnimationListener setAnimationListener() {
		// TODO Auto-generated method stub
		return new AnimationListener() {
			@Override
			public void onAnimationEnd(android.view.animation.Animation arg0) {
				// TODO Auto-generated method stub
				
			}

			@Override
			public void onAnimationRepeat(android.view.animation.Animation arg0) {
				// TODO Auto-generated method stub
				
			}

			@Override
			public void onAnimationStart(android.view.animation.Animation arg0) {
				getshadowView(selectViewCurr).setVisibility(View.VISIBLE);
				if(isDeleteMode)
					trashIcon.setVisibility(View.VISIBLE);
				if(isItemNameAble && "" != item_name.getText()){
					item_name.setVisibility(View.VISIBLE);
				}
			}
			
		};
	}
	
	public class AddView {
		private View aimView;
		private int position;
//		private int aimWidth;
//		private int aimHeight;
//		private boolean isVisibilit = true;
		
		
		private void move(int width, int height, int[] setXY){
			switch (this.position) {
			case VIEW_TOP:
				
				break;
			case VIEW_CENTER:
				//没有改变view的大小
				this.aimView.setX(setXY[0] + (defaultBorderWidth - width * magnification)/2);
				this.aimView.setY(setXY[1] - tabindicator_height + (defaultBorderHeight + height * magnification)/2);
				break;
			case VIEW_BOTTOM:

				break;

			default:
				break;
			}
		}
		
		
	}

}
