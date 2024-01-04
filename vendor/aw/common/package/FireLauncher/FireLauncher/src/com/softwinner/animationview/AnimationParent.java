package com.softwinner.animationview;

import java.io.Serializable;

import com.softwinner.firelauncher.R;
import android.animation.Animator;
import android.animation.Animator.AnimatorListener;
import android.annotation.SuppressLint;
import android.content.Context;
import android.os.Bundle;
import android.os.Handler;
import android.os.Message;
import android.util.Log;
import android.view.View;
import android.view.ViewPropertyAnimator;
import android.view.animation.AccelerateInterpolator;
import android.view.animation.AlphaAnimation;
import android.view.animation.Animation;
import android.view.animation.ScaleAnimation;
import android.view.animation.Animation.AnimationListener;
import android.widget.FrameLayout;
import android.widget.ImageView;

public abstract class AnimationParent {
	private final static String TAG = "AnimationParent";

	public static final int SHADOW_CENTER = 1;
	public static final int SHADOW_SIDE_HORIZONTAL = 2;
	public static final int SHADOW_CENTER_BLUE = 3;
	public static final int SHADOW_CENTER_ORANGE = 4;
	public static final int MSG_FLY_BORDER = 0;
	public static final int MSG_SCALE_VIEW = 1;
	public static final int MSG_LOOP_ANIMATION = 2;
	public final long FLY_DURATION; // 50L
	public final long SHADOW_DURATION;
	public final int DELAY_SCALE_TIME;
	public final int SCALE_DURATION;
	public boolean isDeleteMode = false;
	public boolean isFlyFinished = true;
	public Context mContext;
	public ScaleAnimEffect animEffect;
	public View mParentView;
	public ImageView whiteBorder;
	public ImageView shadowSideHorizontal;
	public ImageView shadowCenter;
	public ImageView shadowCenterBlue;
	public ImageView shadowCenterOrange;
	public ImageView trashIcon;
	public int ItemAnimationIconId = -1;
	public float magnification = 1.025F;// 动画放大倍数
	public float frame_magnification = 1.68F;// TODO 配置相框与动画的放大倍数
	public float tabindicator_height = -1F;
	public View selectViewCurr = null;
	public int defaultBorderWidth = 150;
	public int defaultBorderHeight = 150;
	public int mAimWidth = -1;
	public int mAimHeight = -1;
	public int[] mAimSetXY;
//	public int mAimSetX = -1;
//	public int mAimSetY = -1;
	private OnShadowProcessor mOnShadowProcessor;

	public abstract AnimatorListener setAnimatorListener(final boolean isRecover);
	public abstract AnimationListener setAnimationListener();
	public interface OnShadowProcessor {
		/**
		 * 阴影选择机制：只能在当前有效的阴影中选择
		 * 
		 * @param onFocusViewId 聚焦的View
		 * @return 1 shadow_center; 2 shadow_side_horizontal
		 * */
		public int getShadowSelect(int onFocusViewId);
	}

	@SuppressLint("HandlerLeak")
	public Handler mHandler = new Handler() {
		@Override
		public void handleMessage(Message msg) {
			int width, height;
			int[] setXY;
			switch (msg.what) {
			case MSG_FLY_BORDER:
				width = msg.getData().getInt("width");
				height = msg.getData().getInt("height");
				setXY = msg.getData().getIntArray("SetXY");
				flyWhiteBorder(width, height, setXY, selectViewCurr, false);
				break;
			case MSG_SCALE_VIEW:
				ViewSerializable mScaleView = (ViewSerializable) msg.getData().getSerializable("selectView");
				scaleOnFocusAnimation(mScaleView.selectView);
				break;
			default:
				break;
			}
		}
	};

	public AnimationParent(Context context, View ParentView) {
		mContext = context;
		mParentView = ParentView;
		FLY_DURATION = mContext.getResources().getInteger(R.integer.default_animation_flywhiteborder_duration);
		SHADOW_DURATION = mContext.getResources().getInteger(R.integer.default_animation_flyshadow_duration);
		DELAY_SCALE_TIME = mContext.getResources().getInteger(R.integer.default_animation_scaleview_delay);
		SCALE_DURATION = mContext.getResources().getInteger(R.integer.default_animation_scaleview_duration);
		whiteBorder = (ImageView) mParentView.findViewById(R.id.white_boder);
		shadowSideHorizontal = (ImageView) mParentView.findViewById(R.id.shadow_side_horizontal);
		shadowCenter = (ImageView) mParentView.findViewById(R.id.shadow_center);
		shadowCenterBlue = (ImageView) mParentView.findViewById(R.id.shadow_center_blue);
		shadowCenterOrange = (ImageView) mParentView.findViewById(R.id.shadow_center_orange);
		trashIcon = (ImageView) mParentView.findViewById(R.id.trash_icon);	
		animEffect = new ScaleAnimEffect();

		initAnimationView();
	}

	private void initAnimationView() {
		int defaultBorderX = (int) mContext.getResources().getDimension(R.dimen.layout_margin_left);
		int defaultBorderY = (int) mContext.getResources().getDimension(R.dimen.layout_margin_top);
		FrameLayout.LayoutParams border_layoutparams = new FrameLayout.LayoutParams(defaultBorderWidth, defaultBorderHeight);
		border_layoutparams.leftMargin = defaultBorderX;
		border_layoutparams.topMargin = defaultBorderY;
		whiteBorder.setLayoutParams(border_layoutparams);
		whiteBorder.setVisibility(View.INVISIBLE);

		FrameLayout.LayoutParams shadow_layoutparams = new FrameLayout.LayoutParams(defaultBorderWidth, defaultBorderHeight);
		shadow_layoutparams.leftMargin = defaultBorderX;
		shadow_layoutparams.topMargin = defaultBorderY;
		
		// TODO 所有的相框背景都需要匹配这个参数
		shadowSideHorizontal.setLayoutParams(shadow_layoutparams);
		shadowSideHorizontal.setVisibility(View.INVISIBLE);
		shadowCenter.setLayoutParams(shadow_layoutparams);
		shadowCenter.setVisibility(View.INVISIBLE);
		shadowCenterBlue.setLayoutParams(shadow_layoutparams);
		shadowCenterBlue.setVisibility(View.INVISIBLE);
		shadowCenterOrange.setLayoutParams(shadow_layoutparams);
		shadowCenterOrange.setVisibility(View.INVISIBLE);
		
	}
	
	public void setDeleteMode(boolean isEnable){
		isDeleteMode = isEnable;
		if(!isEnable)trashIcon.setVisibility(View.GONE);
	}
	
	public boolean isDeleteMode(){
		return isDeleteMode;
	}
	
	//TODO 添加相框背景图时，需要在这里匹配相关的选择原则
	public View getshadowView(View view){
		if(mOnShadowProcessor != null){
			int selectShadowNo = mOnShadowProcessor.getShadowSelect(view.getId());
			switch (selectShadowNo) {
			case SHADOW_CENTER:
				return shadowCenter;
			case SHADOW_SIDE_HORIZONTAL:
				return shadowSideHorizontal;
			case SHADOW_CENTER_BLUE:
				return shadowCenterBlue;
			case SHADOW_CENTER_ORANGE:
				return shadowCenterOrange;
			default:
				Log.w(TAG, "undefinition shadow mode had been chosen!!");
				break;
			}
		}
		return shadowCenter;
	}

	public void setAllShadowViewVisibility(int visibility) {
		shadowCenter.setVisibility(visibility);
		shadowSideHorizontal.setVisibility(visibility);
		shadowCenterBlue.setVisibility(visibility);
		shadowCenterOrange.setVisibility(visibility);
	}

	public void recoverLooseFocusAinimation(View selectView) {
		getshadowView(selectView).setVisibility(View.GONE);
		recoverWhiteBorder();
		selectView.clearAnimation();
	}

	public void scaleOnFocusAnimation(final View selectView) {
		if (selectView != selectViewCurr) return;
		scaleWhiteBorder(mAimWidth, mAimHeight, mAimSetXY);
		Animation localAnimation = this.animEffect.createAnimation(1.0F, magnification, 1.0F, magnification, SCALE_DURATION);
		localAnimation.setAnimationListener(setAnimationListener());
		selectView.startAnimation(localAnimation);
	}

	public void flyWhiteBorder(int width, int height, int[] setXY, View selectView, final boolean isRecover) {
		 Log.d(TAG,  "width="+width+"  height="+height+"  setX="+setXY[0]+"  setY="+setXY[1]);
		if ((this.whiteBorder != null)) {
			setAllShadowViewVisibility(View.GONE);

			int mWidth = this.whiteBorder.getWidth();
			int mHeight = this.whiteBorder.getHeight();
			if (mWidth == 0 || mHeight == 0) {
				mWidth = 1;
				mHeight = 1;
			}

			ViewPropertyAnimator localViewPropertyAnimator = this.whiteBorder.animate();
			localViewPropertyAnimator.setDuration(isRecover ? 1L : FLY_DURATION);
			localViewPropertyAnimator.x(setXY[0]);
			localViewPropertyAnimator.y(setXY[1]);
			localViewPropertyAnimator.scaleX((float) width / (float) mWidth);
			localViewPropertyAnimator.scaleY((float) height / (float) mHeight);
			localViewPropertyAnimator.setListener(setAnimatorListener(isRecover));

			ViewPropertyAnimator shadowViewPropertyAnimator = this.getshadowView(selectView).animate();
			shadowViewPropertyAnimator.setDuration(isRecover ? 1L: SHADOW_DURATION);
			shadowViewPropertyAnimator.x(setXY[0]);
			shadowViewPropertyAnimator.y(setXY[1]);
			shadowViewPropertyAnimator.scaleX((float) (width * frame_magnification)	/ (float) mWidth);
			shadowViewPropertyAnimator.scaleY((float) (height * frame_magnification)/ (float) mHeight);

			isFlyFinished = false;
			localViewPropertyAnimator.start();
			shadowViewPropertyAnimator.start();

		}
	}
	
	public void scaleWhiteBorder(int width, int height, int[] setXY) {
		if ((this.whiteBorder != null)) {
			int mWidth = this.whiteBorder.getWidth();
			int mHeight = this.whiteBorder.getHeight();
			if (mWidth == 0 || mHeight == 0) {
				mWidth = 1;
				mHeight = 1;
			}
			float enlarge_width = width * magnification;
			float enlarge_height = height * magnification;
			ViewPropertyAnimator localViewPropertyAnimator = this.whiteBorder
					.animate();
			localViewPropertyAnimator.setDuration(SCALE_DURATION);
			localViewPropertyAnimator.scaleX((float) enlarge_width
					/ (float) mWidth);
			localViewPropertyAnimator.scaleY((float) enlarge_height
					/ (float) mHeight);

			ViewPropertyAnimator shadowViewPropertyAnimator = this
					.getshadowView(selectViewCurr).animate();
			shadowViewPropertyAnimator.setDuration(SCALE_DURATION);
			shadowViewPropertyAnimator.scaleX((float) (enlarge_width * frame_magnification) / (float) mWidth);
			shadowViewPropertyAnimator.scaleY((float) (enlarge_height * frame_magnification) / (float) mHeight);
			
			if(isDeleteMode){
				trashIcon.setX(setXY[0] + (defaultBorderWidth - trashIcon.getWidth())/2);
				trashIcon.setY(setXY[1] + (defaultBorderHeight - trashIcon.getHeight())/2);
			}
			localViewPropertyAnimator.start();
			shadowViewPropertyAnimator.start();
		}
	}
	
	public void moveWhiteBorder(View selectView){
		int aimWidth = 0;
		int aimHeight = 0;
		int[] setXY = getSetiingXY(selectView);
		if(ItemAnimationIconId != -1){
			aimWidth = selectView.findViewById(ItemAnimationIconId).getWidth();
			aimHeight = selectView.findViewById(ItemAnimationIconId).getHeight();
		}else{
			aimWidth = selectView.getWidth();
			aimHeight = selectView.getHeight();
		}
		flyWhiteBorder(aimWidth, aimHeight, setXY, selectView, true);
	}

	public void recoverWhiteBorder() {
		flyWhiteBorder(mAimWidth, mAimHeight, mAimSetXY, selectViewCurr, true);
	}
	
	public void sendMessage(int messageWhat, View selectView, int onDelay) {
		Message message = new Message();
		message.what = messageWhat;
		Bundle bundle = new Bundle();
		switch (messageWhat) {
		case MSG_FLY_BORDER:
			mAimSetXY = getSetiingXY(selectView);
			if(ItemAnimationIconId != -1){
				mAimWidth = selectView.findViewById(ItemAnimationIconId).getWidth();
				mAimHeight = selectView.findViewById(ItemAnimationIconId).getHeight();
			}else{
				mAimWidth = selectView.getWidth();
				mAimHeight = selectView.getHeight();
			}
			bundle.putInt("width", mAimWidth);
			bundle.putInt("height", mAimHeight);
			bundle.putIntArray("SetXY", mAimSetXY);
			break;

		case MSG_SCALE_VIEW:
			bundle.putSerializable("selectView", new ViewSerializable(
					selectView));
			break;

		default:
			break;
		}
		message.setData(bundle);
		if (onDelay > 0)
			mHandler.sendMessageDelayed(message, onDelay);
		else
			mHandler.sendMessage(message);
	}

	public void resetAnimation() {
		initAnimationView();
	}

	public void setShadowProcessor(OnShadowProcessor processor) {
		mOnShadowProcessor = processor;
	}
	
	public void fadeIn(View signView, boolean isRight, final View animationView){
		ViewPropertyAnimator mAnimator = animationView.animate();
		mAnimator.setDuration(FLY_DURATION);
		mAnimator.setInterpolator(new AccelerateInterpolator((float) 1.5));
		mAnimator.x(isRight?signView.getLeft()+signView.getWidth():
						signView.getLeft()-animationView.getWidth());
		mAnimator.setListener(new AnimatorListener() {
			
			@Override
			public void onAnimationStart(Animator arg0) {
				animationView.startAnimation(animEffect.alphaAnimation(0.0F, 1.0F, FLY_DURATION, 0L));
				animationView.setVisibility(View.VISIBLE);
			}
			
			@Override
			public void onAnimationRepeat(Animator arg0) {
				// TODO Auto-generated method stub
				
			}
			
			@Override
			public void onAnimationEnd(Animator arg0) {
				// TODO Auto-generated method stub
				
			}
			
			@Override
			public void onAnimationCancel(Animator arg0) {
				// TODO Auto-generated method stub
				
			}
		});
		mAnimator.start();
	}
	
	public void fadeOut(final View animationView, int SetX){
		ViewPropertyAnimator mAnimator = animationView.animate();
		mAnimator.setDuration(FLY_DURATION - 40L);
		mAnimator.setInterpolator(new AccelerateInterpolator((float) 1.5));
		mAnimator.x(SetX);
		mAnimator.setListener(new AnimatorListener() {
			
			@Override
			public void onAnimationStart(Animator arg0) {
				if(animationView.getVisibility() == View.GONE) return;
				animationView.startAnimation(animEffect.alphaAnimation(1.0F, 0.0F, FLY_DURATION - 40L, 0L));
				animationView.setVisibility(View.VISIBLE);
			}
			
			@Override
			public void onAnimationRepeat(Animator arg0) {
				// TODO Auto-generated method stub
				
			}
			
			@Override
			public void onAnimationEnd(Animator arg0) {
				animationView.setVisibility(View.INVISIBLE);
			}
			
			@Override
			public void onAnimationCancel(Animator arg0) {
				// TODO Auto-generated method stub
				
			}
		});
		mAnimator.start();
	}
	
	/**
	 * 获取图片的左上角需要移动到XY轴上的位置坐标（放大中心不变原则）
	 * @return setXY[0] 移动到X轴上的位置坐标;
	 * 		   setXY[1] 移动到Y轴上的位置坐标
	 * */
	protected int[] getSetiingXY(View imageView){
		int[] setXY = new int[2];
		setXY[0] = (int)(imageView.getLeft() + (imageView.getWidth() - defaultBorderWidth)/2);
		setXY[1] = (int)(imageView.getTop() + (imageView.getHeight() - defaultBorderHeight) / 2);
		return setXY;
	}
	
//	/**
//	 * 获取图片的左上角需要移动到X轴上的位置坐标（放大中心不变原则）
//	 * @param imageViewsX 目标控件在父控件中的x轴位置
//	 * @param imageViewsWidth 目标控件的宽度
//	 * */
//	protected int getSetiingX(float imageViewsX, int imageViewsWidth){
//		return (int)(imageViewsX + (imageViewsWidth - defaultBorderWidth)/2);
//	}
//
//	/**
//	 * 获取图片的左上角需要移动到Y轴上的位置坐标（放大中心不变原则）
//	 * @param imageViewsY 目标控件在父控件中的y轴位置
//	 * @param imageViewsHeight 目标控件的高度
//	 * */
//	protected int getSetiingY(float imageViewsY, int imageViewsHeight) {
//		return (int)(imageViewsY + (imageViewsHeight - defaultBorderHeight) / 2);
//	}

	public class ViewSerializable implements Serializable {
		private static final long serialVersionUID = 1L;
		public View selectView;

		public ViewSerializable(View selectScaleView) {
			super();
			this.selectView = selectScaleView;
		}
	}

	public class ScaleAnimEffect {

		// 渐变透明度动画效果
		public Animation alphaAnimation(float fromAlpha, float toAlpha, long duration, long delayMs) {
			AlphaAnimation localAlphaAnimation = new AlphaAnimation(fromAlpha, toAlpha);
			localAlphaAnimation.setDuration(duration);
			localAlphaAnimation.setStartOffset(delayMs);
			return localAlphaAnimation;
		}

		// 渐变尺寸伸缩动画效果
		public Animation createAnimation(float fromXScale, float toXScale, float fromYScale, 
				float toYScale, long duration) {
			ScaleAnimation localScaleAnimation = new ScaleAnimation(fromXScale,	toXScale, 
					fromYScale, toYScale, Animation.RELATIVE_TO_SELF, 0.5F, Animation.RELATIVE_TO_SELF, 0.5F);
			localScaleAnimation.setFillAfter(true);
			localScaleAnimation.setDuration(duration);
			return localScaleAnimation;
		}
	}

}
