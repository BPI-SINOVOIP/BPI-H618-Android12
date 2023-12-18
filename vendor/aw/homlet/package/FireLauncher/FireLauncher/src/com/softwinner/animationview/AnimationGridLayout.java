package com.softwinner.animationview;

import com.softwinner.firelauncher.R;

import android.animation.Animator;
import android.animation.Animator.AnimatorListener;
import android.content.Context;
import android.os.Bundle;
import android.os.Message;
import android.util.Log;
import android.view.View;
import android.view.ViewTreeObserver;
import android.view.animation.Animation.AnimationListener;
import android.widget.FrameLayout;
import android.widget.TextView;

public class AnimationGridLayout extends AnimationParent {
	private final String TAG = "AnimationDemo";
	
	private static final int CHECK_ONfOCUS_DELAY = 10;
	private Context mContext;
	private boolean isItemNameAble = false;
	private boolean isViewFocused = false;
	private boolean isMeasured = false;
	private TextView item_name;
	private View mParentView;
	private View selectViewPre = null;
	private float tabindicator_height = -1F;

	public AnimationGridLayout(Context context, View ParentView, int itemAnimationIconId, boolean isEnableItemName) {
		super(context, ParentView);
		mContext = context;
		mParentView = ParentView;
		ItemAnimationIconId = itemAnimationIconId;
		isItemNameAble = isEnableItemName;
		if(isItemNameAble){
			item_name = (TextView) mParentView.findViewById(R.id.item_name);
			tabindicator_height = mContext.getResources().getDimension(R.dimen.tabindicator_height);
		}
	}
	
	@Override
	public void sendMessage(int messageWhat, View selectView, int onDelay){
		Message message = new Message();
		message.what = messageWhat;
		Bundle bundle = new Bundle(); 
		switch (messageWhat) {
		case MSG_FLY_BORDER:
			mAimSetXY = getSetiingXY(selectView);
			mAimWidth = selectView.findViewById(ItemAnimationIconId).getWidth();
			mAimHeight = selectView.findViewById(ItemAnimationIconId).getHeight();
			bundle.putInt("width", mAimWidth);
			bundle.putInt("height", mAimHeight);
			bundle.putIntArray("SetXY", mAimSetXY);
			break;
			
		case MSG_SCALE_VIEW:
			bundle.putSerializable("selectView",new ViewSerializable(selectView));
			break;

		default:
			break;
		}
		message.setData(bundle);
		if(onDelay > 0)
			mHandler.sendMessageDelayed(message, onDelay);
		else 
			mHandler.sendMessage(message);
	}
	
	public void scaleWhiteBorder(int width, int height, int[] setXY){
		super.scaleWhiteBorder(width, height, setXY);
		if(isItemNameAble){
			FrameLayout.LayoutParams text_layoutparams = new FrameLayout.LayoutParams((int) (width * magnification), (int)tabindicator_height);
			item_name.setLayoutParams(text_layoutparams);
			item_name.setX(setXY[0] + (defaultBorderWidth - width * magnification)/2);
			item_name.setY(setXY[1] - tabindicator_height + (defaultBorderHeight + height * magnification)/2);
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
			selectViewCurr.bringToFront();
			if(isItemNameAble)
				item_name.setText(itemName);
			if(selectViewCurr.getWidth() != 0)
				sendMessage(MSG_FLY_BORDER, selectViewCurr, 0);
			else{
				isMeasured = false;
				setViewObserver(selectViewCurr);
			}				
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
				if(!isViewFocused){
					selectViewPre.clearAnimation();
					getshadowView(selectViewPre).setVisibility(View.INVISIBLE);
					return;
				}
				whiteBorder.setVisibility(View.VISIBLE);					
				if(isFlyFinished == false && !isRecover){
					sendMessage(MSG_SCALE_VIEW, selectViewCurr, DELAY_SCALE_TIME);
					isFlyFinished = true;
				}
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
				if(isItemNameAble && "" != item_name.getText()){
					item_name.setVisibility(View.VISIBLE);
				}
			}
			
		};
	}


}
