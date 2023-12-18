package com.softwinner.dragonbox.manager;

import android.app.Activity;
import android.content.Context;
import android.os.Handler;
import android.os.Message;

import com.softwinner.dragonbox.utils.HttpUtil;

public class YSTLanManager {
    private Context mContext;

    public String mLan1IP;
    public String mLan2IP;
    private int mTimeOut;
    private onPingRespondListener mOnPingRespondListener;
    private boolean mPingLan = false;

    public YSTLanManager(Context context, String lan1IP, String lan2IP, int timeout) {
        mContext = context;
        mLan1IP = lan1IP;
        mLan2IP = lan2IP;
        mTimeOut = timeout;
    }

    private static final int MSG_STATE_REFLASH_WHAT = 2;
    private static final int MSG_CONN_TIME_OUT_WHAT = 4;

    Handler mHandler = new Handler() {
        public void handleMessage(Message msg) {
            if (mOnPingRespondListener == null) {
                return;
            }
            switch (msg.what) {
                case MSG_STATE_REFLASH_WHAT:
                    mOnPingRespondListener.onPingRespond(new boolean[]{msg.arg1 == 1, msg.arg2 == 1});
                    break;
                case MSG_CONN_TIME_OUT_WHAT:
                    mOnPingRespondListener.onTimeOut();
                    break;
            }
        }
    };

    public void startPingLan() {
        mHandler.sendEmptyMessageDelayed(MSG_CONN_TIME_OUT_WHAT, mTimeOut);
        mPingLan = true;
        new Thread() {
            @Override
            public void run() {
                while (mPingLan) {
                    final boolean lan1Result = HttpUtil.ping(mLan1IP);
                    final boolean lan2Result = HttpUtil.ping(mLan2IP);
                    Message msgReflash = mHandler.obtainMessage(
                            MSG_STATE_REFLASH_WHAT, lan1Result ? 1 : 0, lan2Result ? 1 : 0);
                    mHandler.sendMessage(msgReflash);
                }
            }
        }.start();
    }

    public void stopPingLan() {
        mPingLan = false;

    }

    public void setOnPingRespondListener(
            onPingRespondListener onPingRespondListener) {
        mOnPingRespondListener = onPingRespondListener;
    }

    public interface onPingRespondListener {
        public void onPingRespond(boolean[] result);
        public void onTimeOut();
    }

}
