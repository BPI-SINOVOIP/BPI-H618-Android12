package com.softwinner.dragonbox.manager;

import java.util.ArrayList;
import java.util.List;
import java.util.Map;

import org.apache.http.NameValuePair;
import org.apache.http.message.BasicNameValuePair;

import android.content.Context;
import android.os.Bundle;
import android.os.Handler;
import android.os.Message;

import com.softwinner.dragonbox.entity.YSTWanConfigInfo;
import com.softwinner.dragonbox.utils.HttpUtil;

public class YSTWanManager {
    public Context mContext;
    public YSTWanConfigInfo mConfigInfo;
    public WanConnStatListener mConnStatListener;

    private boolean relash = false;

    public YSTWanManager(Context context, YSTWanConfigInfo configInfo) {
        mContext = context;
        mConfigInfo = configInfo;
    }

    public void setConnStatListener(WanConnStatListener connStatListener) {
        mConnStatListener = connStatListener;
    }

    private static final int MSG_SENG_RESULT_WHAT = 1;
    private static final int MSG_STATE_REFLASH_WHAT = 2;
    private static final int MSG_CONN_END_WHAT = 3;
    private static final int MSG_CONN_TIME_OUT_WHAT = 4;

    private static final String MSG_GET_WAN_IP = "wan_ipaddr";

    Handler mHandler = new Handler() {
        public void handleMessage(Message msg) {
            if (mConnStatListener == null) {
                return;
            }
            switch (msg.what) {
                case MSG_SENG_RESULT_WHAT:
                    mConnStatListener.onCommandSendResult(msg.arg1 == 1);
                    break;
                case MSG_STATE_REFLASH_WHAT:
                    mConnStatListener.onWanStateReflash(msg.arg1 == 1);
                    break;
                case MSG_CONN_END_WHAT:
                    String ip = msg.getData().getString(MSG_GET_WAN_IP);
                    mConnStatListener.onWanConnEnd(msg.arg1 == 1, ip);
                    removeMessages(MSG_CONN_TIME_OUT_WHAT);
                    break;
                case MSG_CONN_TIME_OUT_WHAT:
                    mConnStatListener.onTimeOut();
                    break;
            }
        }
    };

    public void startConnWan() {
        mHandler.sendEmptyMessageDelayed(MSG_CONN_TIME_OUT_WHAT,
                mConfigInfo.getTimeOut());
        new Thread() {
            @Override
            public void run() {
                relash = true;

                while (relash && !HttpUtil.ping(mConfigInfo.getRouterIP())) {
                    try {
                        Thread.sleep(200);
                    } catch (InterruptedException e) {
                        e.printStackTrace();
                    }
                }

                List<NameValuePair> params = new ArrayList<NameValuePair>();
                params.add(new BasicNameValuePair("data", mConfigInfo
                        .getWanConnPara()));
                final boolean result = HttpUtil.parseBooleanResult(HttpUtil
                        .requestRouter(mConfigInfo.getWanSetParaUrl(), params));
                Message msgResult = mHandler.obtainMessage(
                        MSG_SENG_RESULT_WHAT, result ? 1 : 0, 0);
                mHandler.sendMessage(msgResult);

                if (!result) {
                    return;
                }

                boolean connected = false;
                String ip = "";
                // while (relash && count > 0) {
                while (relash) {
                    final Map<String, String> connInfo = HttpUtil
                            .parseMapResult(HttpUtil.requestRouter(
                                    mConfigInfo.getWanGetStatusUrl(), null));
                    ip = connInfo.get("wan_ipaddr");
                    connected = "1".equals(connInfo.get("wan_connect_state"))
                            && !"0.0.0.0".equals(ip);
                    Message msgState = mHandler.obtainMessage(
                            MSG_STATE_REFLASH_WHAT, connected ? 1 : 0, 0);
                    mHandler.sendMessage(msgState);

                    if (connected) {
                        Message msgEnd = mHandler.obtainMessage(
                                MSG_CONN_END_WHAT, 1, 0);
                        Bundle bundle = new Bundle();
                        bundle.putString(MSG_GET_WAN_IP, ip);
                        msgEnd.setData(bundle);
                        mHandler.sendMessage(msgEnd);
                        return;
                    }
                    try {
                        Thread.sleep(1000);
                    } catch (InterruptedException e) {
                        e.printStackTrace();
                    }
                }

            }
        }.start();
    }

    public void stopConnWan() {
        relash = false;
        new Thread() {
            @Override
            public void run() {
                List<NameValuePair> params = new ArrayList<NameValuePair>();
                params.add(new BasicNameValuePair("data", mConfigInfo
                        .getWanDisconnPara()));
                final boolean result = HttpUtil.parseBooleanResult(HttpUtil
                        .requestRouter(mConfigInfo.getWanSetParaUrl(), params));
            }
        }.start();
        mHandler.removeMessages(MSG_CONN_TIME_OUT_WHAT);
    }

    public interface WanConnStatListener {
        public void onCommandSendResult(boolean result);

        public void onWanStateReflash(boolean connected);

        public void onWanConnEnd(boolean connected, String ip);

        public void onTimeOut();
    }
}
