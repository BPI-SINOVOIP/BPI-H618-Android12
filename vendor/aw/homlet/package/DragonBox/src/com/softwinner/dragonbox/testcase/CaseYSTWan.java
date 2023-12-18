package com.softwinner.dragonbox.testcase;

import org.xmlpull.v1.XmlPullParser;

import android.content.Context;
import android.content.DialogInterface;
import android.content.DialogInterface.OnClickListener;
import android.content.DialogInterface.OnDismissListener;
import android.util.Log;
import android.widget.TextView;

import com.softwinner.dragonbox.R;
import com.softwinner.dragonbox.entity.YSTWanConfigInfo;
import com.softwinner.dragonbox.entity.YSTWifiConnConfigInfo;
import com.softwinner.dragonbox.manager.LedManager;
import com.softwinner.dragonbox.manager.YSTWanManager;
import com.softwinner.dragonbox.manager.YSTWifiManager;

public class CaseYSTWan extends IBaseCase implements
        YSTWanManager.WanConnStatListener {
    private static final String TAG = "CaseYSTWan";
    YSTWanManager wanManager;

    TextView mMinYSTWanStatus;
    TextView mMaxYSTWanStatus;
    TextView mMinYSTWanIP;
    TextView mMaxYSTWanIP;

    public CaseYSTWan(Context context) {
        super(context, R.string.case_yst_wan_name, R.layout.case_yst_wan_max,
                R.layout.case_yst_wan_min, TYPE_MODE_AUTO);
        mMinYSTWanStatus = (TextView) mMinView
                .findViewById(R.id.case_yst_wan_status);
        mMaxYSTWanStatus = (TextView) mMaxView
                .findViewById(R.id.case_yst_wan_status);
        mMinYSTWanIP = (TextView) mMinView.findViewById(R.id.case_yst_wan_ip);
        mMaxYSTWanIP = (TextView) mMaxView.findViewById(R.id.case_yst_wan_ip);
    }

    public CaseYSTWan(Context context, XmlPullParser xmlParser) {
        this(context);
        String routerIP = xmlParser.getAttributeValue(null, "routerIP");
        int timeOut = 5000;
        try {
            timeOut = Integer.parseInt(xmlParser.getAttributeValue(null, "timeout"));
        } catch (NumberFormatException e) {
            // TODO Auto-generated catch block
            e.printStackTrace();
        }
        YSTWanConfigInfo configInfo = new YSTWanConfigInfo(routerIP, timeOut);
        wanManager = new YSTWanManager(context, configInfo);
    }

    @Override
    public void onStartCase() {
        wanManager.setConnStatListener(this);
        wanManager.startConnWan();
        this.setDialogPositiveButtonEnable(false);
    }

    @Override
    public void onStopCase() {
        this.setDialogPositiveButtonEnable(getCaseResult());
        wanManager.stopConnWan();
    }

    @Override
    public void reset() {
        super.reset();
        updateView(false, "");
    }

    @Override
    public void onCommandSendResult(boolean result) {
        if (!result) {
            setCaseResult(result);
            this.startCase();
        }
    }

    @Override
    public void onWanStateReflash(boolean connected) {
        setCaseResult(connected);
        updateView(connected, "");
    }

    @Override
    public void onWanConnEnd(boolean connected, String ip) {
        setCaseResult(connected);
        this.stopCase();
        updateView(connected, ip);
    }

    private void updateView(boolean connected, String ip) {
        mMinYSTWanIP.setText(mContext.getString(R.string.case_yst_wan_ip, ip));
        mMaxYSTWanIP.setText(mContext.getString(R.string.case_yst_wan_ip, ip));
        mMinYSTWanStatus
                .setText(connected ? R.string.case_yst_wan_status_success_text
                        : R.string.case_yst_wan_status_fail_text);
        mMaxYSTWanStatus
                .setText(connected ? R.string.case_yst_wan_status_success_text
                        : R.string.case_yst_wan_status_fail_text);
    }

    @Override
    public void onTimeOut() {
        Log.d(TAG, "onTimeOut");
        updateView(false, "");
        this.stopCase();
    }
}
