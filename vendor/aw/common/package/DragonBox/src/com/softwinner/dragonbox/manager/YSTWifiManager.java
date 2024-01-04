package com.softwinner.dragonbox.manager;

import java.util.ArrayList;
import java.util.List;
import java.util.Map;

import org.apache.http.NameValuePair;
import org.apache.http.message.BasicNameValuePair;

import android.app.Activity;
import android.content.Context;
import android.os.Handler;
import android.os.Message;
import android.util.Log;

import com.softwinner.dragonbox.entity.YSTWifiConnConfigInfo;
import com.softwinner.dragonbox.utils.HttpUtil;

public class YSTWifiManager {
    private static final String TAG = "YSTWifiManager";
    public Context mContext;
    public YSTWifiConnConfigInfo mConfigInfo;
    public WifiConnStatListener mConnStatListener;

    private boolean relash = false;

    public YSTWifiManager(Context context, YSTWifiConnConfigInfo configInfo) {
        mContext = context;
        mConfigInfo = configInfo;
    }

    public void setConnStatListener(WifiConnStatListener connStatListener) {
        mConnStatListener = connStatListener;
    }

    private static final int MSG_SENG_RESULT_WHAT = 1;
    private static final int MSG_STATE_REFLASH_WHAT = 2;
    private static final int MSG_CONN_END_WHAT = 3;
    private static final int MSG_CONN_TIME_OUT_WHAT = 4;

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
                    // nothing to do for here ,cause thread had send
                    break;
                case MSG_CONN_END_WHAT:
                    mConnStatListener.onWifiConnEnd(msg.arg1 == 1);
                    removeMessages(MSG_CONN_TIME_OUT_WHAT);
                    break;
                case MSG_CONN_TIME_OUT_WHAT:
                    mConnStatListener.onTimeOut();
                    break;
            }
        }
    };

    public void startConnWifi() {
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
                        .getWifiDhcpPara()));
                final boolean result = HttpUtil
                        .parseBooleanResult(HttpUtil.requestRouter(
                                mConfigInfo.getWifiSetParaUrl(), params));

                Message msgResult = mHandler.obtainMessage(
                        MSG_SENG_RESULT_WHAT, result ? 1 : 0, 0);
                mHandler.sendMessage(msgResult);
                if (!result) {
                    return;
                }

                boolean connected = false;
                List<NameValuePair> paramStat = new ArrayList<NameValuePair>();
                paramStat.add(new BasicNameValuePair("data", mConfigInfo
                        .getWifiStatPara()));

                // while (relash && count > 0) {
                while (relash) {
                    final Map<String, String> connInfo = HttpUtil
                            .parseMapResult(HttpUtil.requestRouter(
                                    mConfigInfo.getWifiGetParaUrl(), paramStat));
                    connected = "Connected".equals(connInfo
                            .get("wisp_connect_state"))
                            && mConfigInfo.getApName().equals(
                            connInfo.get("wisp_ap_name"));
                    Log.d(TAG,
                            "wisp_connect_state="
                                    + connInfo.get("wisp_connect_state")
                                    + " | " + mConfigInfo.getApName() + " | "
                                    + connInfo.get("wisp_ap_name"));
                    final boolean connectedStat = connected;
                    ((Activity) mContext).runOnUiThread(new Runnable() {
                        @Override
                        public void run() {
                            mConnStatListener.onWifiStateReflash(connInfo,
                                    connectedStat);

                        }
                    });
                    if (connected) {
                        Message msgEnd = mHandler.obtainMessage(
                                MSG_CONN_END_WHAT, 1, 0);
                        mHandler.sendMessage(msgEnd);
                        return;
                    }
                    try {
                        Thread.sleep(1000);
                    } catch (InterruptedException e) {
                        // TODO Auto-generated catch block
                        e.printStackTrace();
                    }
                }
            }
        }.start();

    }

    public void stopConnWifi() {
        relash = false;
        new Thread() {
            @Override
            public void run() {
                List<NameValuePair> params = new ArrayList<NameValuePair>();
                params.add(new BasicNameValuePair("data", mConfigInfo
                        .getWifiDisConnPara()));
                final boolean result = HttpUtil
                        .parseBooleanResult(HttpUtil.requestRouter(
                                mConfigInfo.getWifiSetParaUrl(), params));
            }
        }.start();
    }

    public interface WifiConnStatListener {
        public void onCommandSendResult(boolean result);

        public void onWifiStateReflash(Map<String, String> connInfo,
                                       boolean connected);

        public void onWifiConnEnd(boolean connected);

        public void onTimeOut();

    }
}
