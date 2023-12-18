package com.allwinner.camera.views;

import android.content.Context;
import android.graphics.Canvas;
import android.util.AttributeSet;
import android.view.animation.AnimationUtils;
import android.widget.ImageView;

public class RotateImageView extends ImageView implements RotateView{
    private int mCurrentAngle = 0;
    private int mStartAngle = 0;
    private long mStartAnimTime = 0;
    private long mEndAnimTime = 0;
    private int mTargetAngle = 0;
    private long mStepAngle = 0;
    public RotateImageView(Context context) {
        super(context);
    }

    public RotateImageView(Context context, AttributeSet attrs) {
        super(context, attrs);
    }


    @Override
    public void onOrientationChanged(int orientation) {
        mTargetAngle = orientation;
        mStartAnimTime = AnimationUtils.currentAnimationTimeMillis();
        mEndAnimTime = mStartAnimTime + ANIM_DURATION;
        mStartAngle = mCurrentAngle;

        int diff = mTargetAngle - mStartAngle;
        diff = diff < 0 ? diff + 360 : diff;
        diff = (diff > 180) ? diff - 360 : diff;
        mStepAngle = (diff * 1000) / ANIM_DURATION;
        invalidate();
    }

    @Override
    protected void onMeasure(int widthMeasureSpec, int heightMeasureSpec) {
        super.onMeasure(widthMeasureSpec, heightMeasureSpec);
    }

    @Override
    protected void onDraw(Canvas canvas) {
        if (mCurrentAngle != mTargetAngle) {
            long time = AnimationUtils.currentAnimationTimeMillis();
            if (time < mEndAnimTime) {
                long duration = time - mStartAnimTime;
                mCurrentAngle = mStartAngle + (int) (duration * mStepAngle) / 1000;
                invalidate();
            } else {
                mCurrentAngle = mTargetAngle;
            }

        }
        canvas.save();
        canvas.translate(getPaddingLeft(), getPaddingTop());
        canvas.rotate(-this.mCurrentAngle, this.getWidth() / 2f, this.getHeight() / 2f);
        super.onDraw(canvas);
        canvas.restore();
    }

}
