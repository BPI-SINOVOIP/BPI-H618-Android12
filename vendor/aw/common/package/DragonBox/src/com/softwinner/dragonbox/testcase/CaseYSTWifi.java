package com.softwinner.dragonbox.testcase;

import java.util.Map;

import org.xmlpull.v1.XmlPullParser;

import android.content.Context;
import android.util.Log;
import android.widget.TextView;

import com.softwinner.dragonbox.R;
import com.softwinner.dragonbox.entity.YSTWifiConnConfigInfo;
import com.softwinner.dragonbox.manager.YSTWifiManager;

public class CaseYSTWifi extends IBaseCase implements YSTWifiManager.WifiConnStatListener{
    private static final String TAG = "CaseYSTWifi";
    YSTWifiManager wifiManager;

    TextView mMinYSTWifiStatus;
    TextView mMaxYSTWifiStatus;
    TextView mMinYSTWifiConnName;
    TextView mMaxYSTWifiConnName;
    TextView mMinYSTWifiConnStrength;
    TextView mMaxYSTWifiConnStrength;
    TextView mMinYSTWifiResult;
    TextView mMaxYSTWifiResult;
    private Map<String, String>  mConnInfo;

    public CaseYSTWifi(Context context) {
        super(context, R.string.case_yst_wifi_name, R.layout.case_yst_wifi_max,
                R.layout.case_yst_wifi_min, TYPE_MODE_AUTO);
        mMinYSTWifiStatus = (TextView) mMinView.findViewById(R.id.case_yst_wifi_status);
        mMaxYSTWifiStatus = (TextView) mMaxView.findViewById(R.id.case_yst_wifi_status);
        mMinYSTWifiConnName = (TextView) mMinView.findViewById(R.id.case_yst_wifi_conn_name);
        mMaxYSTWifiConnName = (TextView) mMaxView.findViewById(R.id.case_yst_wifi_conn_name);
        mMinYSTWifiConnStrength = (TextView) mMinView.findViewById(R.id.case_yst_wifi_strength);
        mMaxYSTWifiConnStrength = (TextView) mMaxView.findViewById(R.id.case_yst_wifi_strength);
        mMinYSTWifiResult = (TextView) mMinView.findViewById(R.id.case_yst_wifi_result);
        mMaxYSTWifiResult = (TextView) mMaxView.findViewById(R.id.case_yst_wifi_result);
    }

    public CaseYSTWifi(Context context, XmlPullParser xmlParser) {
        this(context);
        String routerIP = xmlParser.getAttributeValue(null, "routerIP");
        String apName = xmlParser.getAttributeValue(null, "apName");
        String apPSW = xmlParser.getAttributeValue(null, "apPSW");
        String apSecurity = xmlParser.getAttributeValue(null, "apSecurity");
        int timeOut = 5000;
        try {
            timeOut = Integer.parseInt(xmlParser.getAttributeValue(null, "timeout"));
        } catch (NumberFormatException e) {
            // TODO Auto-generated catch block
            e.printStackTrace();
        }
        YSTWifiConnConfigInfo configInfo = new YSTWifiConnConfigInfo(routerIP, apName, apPSW, apSecurity, timeOut);
        wifiManager = new YSTWifiManager(context, configInfo);
    }

    @Override
    public void onStartCase() {
        wifiManager.setConnStatListener(this);
        wifiManager.startConnWifi();
        this.setDialogPositiveButtonEnable(false);
    }

    @Override
    public void onStopCase() {
        wifiManager.stopConnWifi();
        updateView(mConnInfo, getCaseResult());
        this.setDialogPositiveButtonEnable(getCaseResult());
    }

    @Override
    public void reset() {
        super.reset();
        updateView(null, false);
        mConnInfo = null;
    }

    @Override
    public void onCommandSendResult(boolean result) {
        if (!result) {
            setCaseResult(result);
        }
    }

    @Override
    public void onWifiStateReflash(Map<String, String>  connInfo, boolean connected) {
        mConnInfo = connInfo;
        setCaseResult(connected);
        updateView(mConnInfo, connected);
    }

    @Override
    public void onWifiConnEnd(boolean connected) {
        setCaseResult(connected);
        this.stopCase();
    }


    @Override
    public void onTimeOut() {
        Log.d(TAG, "onTimeOut");
        this.stopCase();
    }

    private void updateView(Map<String, String>  connInfo, boolean result){
        mMinYSTWifiStatus.setText(mContext.getString(R.string.case_yst_wifi_conn_status, connInfo == null? "" : connInfo.get("wisp_connect_state")));
        mMaxYSTWifiStatus.setText(mContext.getString(R.string.case_yst_wifi_conn_status, connInfo == null? "" : connInfo.get("wisp_connect_state")));
        mMinYSTWifiConnName.setText(mContext.getString(R.string.case_yst_wifi_conn_name, connInfo == null? "" : connInfo.get("wisp_ap_name")));
        mMaxYSTWifiConnName.setText(mContext.getString(R.string.case_yst_wifi_conn_name, connInfo == null? "" : connInfo.get("wisp_ap_name")));
        mMinYSTWifiConnStrength.setText(mContext.getString(R.string.case_yst_wifi_conn_streagth, connInfo == null? "" : connInfo.get("wisp_ap_signal_strength")));
        mMaxYSTWifiConnStrength.setText(mContext.getString(R.string.case_yst_wifi_conn_streagth, connInfo == null? "" : connInfo.get("wisp_ap_signal_strength")));
        mMinYSTWifiResult.setText(result? R.string.case_yst_wifi_status_success_text : R.string.case_yst_wifi_status_fail_text);
        mMaxYSTWifiResult.setText(result? R.string.case_yst_wifi_status_success_text : R.string.case_yst_wifi_status_fail_text);
    }


}
