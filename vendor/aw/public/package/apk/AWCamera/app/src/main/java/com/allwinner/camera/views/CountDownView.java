package com.allwinner.camera.views;

import android.content.Context;
import android.os.Build;
import android.os.Handler;
import android.os.Message;
import android.util.AttributeSet;
import android.util.Log;
import android.widget.FrameLayout;
import android.widget.TextView;

import com.allwinner.camera.R;

import java.util.Locale;

public class CountDownView extends FrameLayout {


    public interface  CountDownListener {
        void onCountDownStart();
        void onCountDownEnd();
        void onCountDownCancel();
    }

    private CountDownListener mListener;
    private RotateTextView mTextView;
    private Handler mHandler = new CountDownHandler();
    private final static int COUNTDOWN = 1;
    private int mRemainTime = 0;

    public CountDownView(Context context) {
        super(context);
    }

    public CountDownView(Context context, AttributeSet attrs) {
        super(context, attrs);
    }

    @Override
    protected void onFinishInflate() {
        super.onFinishInflate();
        mTextView = findViewById(R.id.countdowntext);
    }

    public void setListener(CountDownListener listener){
        mListener = listener;
    }

    public void startCountDown(int time){
        setVisibility(VISIBLE);
        mRemainTime = time;
        if (mListener!= null && time == 0 ){
            setVisibility(GONE);
            mListener.onCountDownEnd();
            return;
        }
        if (mListener!= null ){
            mListener.onCountDownStart();
        }
        doCountDown();
    }

    public void cancelCountDown(){
        setVisibility(GONE);
        mHandler.removeCallbacksAndMessages(null);
        if (mListener!= null ) {
            mListener.onCountDownCancel();
        }
    }

    private void doCountDown(){
        Locale locale;
        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.N) {
            locale = getResources().getConfiguration().getLocales().get(0);
        } else {
            locale = getResources().getConfiguration().locale;
        }
        String value = String.format(locale, "%d", mRemainTime);
        mTextView.setText(value);
        mHandler.sendEmptyMessageDelayed(COUNTDOWN,1000);
    }


    public void onOrientationChanged(int orientation) {
        mTextView.onOrientationChanged(orientation);
    }

    public void release() {

    }

    private class CountDownHandler extends Handler{
        @Override
        public void handleMessage(Message msg) {
            if (msg.what == COUNTDOWN) {
                mRemainTime = mRemainTime -1;
                if (mRemainTime != 0) {
                    doCountDown();
                } else {
                    setVisibility(GONE);
                    if (mListener != null) {
                        mListener.onCountDownEnd();
                    }
                }
            }
        }
    }

}
