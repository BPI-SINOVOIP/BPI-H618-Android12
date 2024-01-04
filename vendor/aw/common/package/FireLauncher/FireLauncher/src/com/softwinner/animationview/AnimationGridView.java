package com.softwinner.animationview;

import com.softwinner.firelauncher.R;

import android.animation.Animator;
import android.animation.Animator.AnimatorListener;
import android.content.Context;
import android.os.Bundle;
import android.os.Message;
import android.text.TextUtils;
import android.util.Log;
import android.view.View;
import android.view.animation.Animation;
import android.view.animation.Animation.AnimationListener;
import android.widget.AdapterView;
import android.widget.GridView;

public class AnimationGridView extends AnimationParent {
	private final String TAG = "AnimationGridView";
	private View selectViewPre = null;

	public AnimationGridView(Context context, View ParentView, GridView parent, int itemAnimationIconId) {
		super(context, ParentView);
		ItemAnimationIconId = itemAnimationIconId;
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
	
	public void onItemSelected(AdapterView<?> parent, View view, int position){
		if(null == view) return;
		mParentView.bringToFront();//控件前置（阴影部分才能显示）	
		selectViewCurr = view;
		AnimationText curr_item_name = (AnimationText)selectViewCurr.findViewById(R.id.item_name);
		if(curr_item_name != null)
			curr_item_name.setEllipsize(TextUtils.TruncateAt.valueOf("MARQUEE"));
		sendMessage(MSG_FLY_BORDER, selectViewCurr, 0);
		if(selectViewPre != null && selectViewPre != selectViewCurr){
			recoverLooseFocusAinimation(selectViewPre);
			AnimationText pre_item_name = (AnimationText)selectViewPre.findViewById(R.id.item_name);
			if(pre_item_name != null)
				pre_item_name.setEllipsize(TextUtils.TruncateAt.valueOf("END"));
		}
		selectViewPre = selectViewCurr;
	}
	
	public void onNothingSelected(){
		selectViewCurr = null;
		selectViewPre = null;
		resetAnimation();
	} 
	
	//TODO 添加相框背景图时，需要在这里匹配相关的选择原则
	public View getshadowView(View view){
		return shadowCenterBlue;
	}

	@Override
	public AnimatorListener setAnimatorListener(final boolean isRecover) {
		// TODO Auto-generated method stub
		return new AnimatorListener() {
			@Override
			public void onAnimationStart(Animator animator) {
				// TODO Auto-generated method stub
//				whiteBorder.setVisibility(View.INVISIBLE);	
			}
			
			@Override
			public void onAnimationRepeat(Animator animator) {
				// TODO Auto-generated method stub
				
			}
			
			@Override
			public void onAnimationEnd(Animator animator) {
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
			public void onAnimationEnd(Animation animation) {
				// TODO Auto-generated method stub
			}

			@Override
			public void onAnimationRepeat(Animation animation) {
				// TODO Auto-generated method stub
			}

			@Override
			public void onAnimationStart(Animation animation) {
//				getshadowView(selectViewCurr).startAnimation(animEffect.alphaAnimation(0.0F, 1.0F, 250L, 0L));
				getshadowView(selectViewCurr).setVisibility(View.VISIBLE);
			}
			
		};
	}
	
}
