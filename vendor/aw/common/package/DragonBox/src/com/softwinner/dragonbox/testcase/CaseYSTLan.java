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
import com.softwinner.dragonbox.manager.LedManager;
import com.softwinner.dragonbox.manager.YSTLanManager;
import com.softwinner.dragonbox.manager.YSTWanManager;

public class CaseYSTLan extends IBaseCase implements
        YSTLanManager.onPingRespondListener {
    private static final String TAG = "CaseYSTLan";
    YSTLanManager lanManager;

    TextView mMinYSTLan1Status;
    TextView mMaxYSTLan1Status;
    TextView mMinYSTLan2Status;
    TextView mMaxYSTLan2Status;
    TextView mMinYSTLanStatus;
    TextView mMaxYSTLanStatus;

    public CaseYSTLan(Context context) {
        super(context, R.string.case_yst_lan_name, R.layout.case_yst_lan_max,
                R.layout.case_yst_lan_min, TYPE_MODE_AUTO);
        mMinYSTLan1Status = (TextView) mMinView
                .findViewById(R.id.case_yst_lan1_status);
        mMaxYSTLan1Status = (TextView) mMaxView
                .findViewById(R.id.case_yst_lan1_status);
        mMinYSTLan2Status = (TextView) mMinView
                .findViewById(R.id.case_yst_lan2_status);
        mMaxYSTLan2Status = (TextView) mMaxView
                .findViewById(R.id.case_yst_lan2_status);
        mMinYSTLanStatus = (TextView) mMinView
                .findViewById(R.id.case_yst_lan_status);
        mMaxYSTLanStatus = (TextView) mMaxView
                .findViewById(R.id.case_yst_lan_status);
    }

    public CaseYSTLan(Context context, XmlPullParser xmlParser) {
        this(context);
        String lan1IP = xmlParser.getAttributeValue(null, "lan1IP");
        String lan2IP = xmlParser.getAttributeValue(null, "lan2IP");
        int timeOut = 5000;
        try {
            timeOut = Integer.parseInt(xmlParser.getAttributeValue(null, "timeout"));
        } catch (NumberFormatException e) {
            // TODO Auto-generated catch block
            e.printStackTrace();
        }
        lanManager = new YSTLanManager(context, lan1IP, lan2IP, timeOut);

    }

    @Override
    public void onStartCase() {
        lanManager.setOnPingRespondListener(this);
        lanManager.startPingLan();
        mMinYSTLanStatus.setText(R.string.case_yst_lan_status_searching);
        mMaxYSTLanStatus.setText(R.string.case_yst_lan_status_searching);
        this.setDialogPositiveButtonEnable(false);
    }

    @Override
    public void onStopCase() {
        this.setDialogPositiveButtonEnable(getCaseResult());
        lanManager.stopPingLan();
        lanManager.setOnPingRespondListener(null);
        mMinYSTLanStatus.setText(R.string.case_yst_lan_status_search_end);
        mMaxYSTLanStatus.setText(R.string.case_yst_lan_status_search_end);
    }

    @Override
    public void reset() {
        super.reset();
        updateView(new boolean[] { false, false });
        mMinYSTLanStatus.setText(R.string.case_yst_lan_status);
        mMaxYSTLanStatus.setText(R.string.case_yst_lan_status);
    }

    @Override
    public void onPingRespond(boolean[] result) {
        if (result.length == 2) {
            updateView(result);
            setCaseResult(result[0] && result[1]);
            if (result[0] && result[1]) {
                this.stopCase();
            }
        } else {
            setCaseResult(false);
        }
    }

    public void updateView(boolean[] result) {
        mMinYSTLan1Status
                .setText(result[0] ? R.string.case_yst_lan1_status_success_text
                        : R.string.case_yst_lan1_status_fail_text);
        mMaxYSTLan1Status
                .setText(result[0] ? R.string.case_yst_lan1_status_success_text
                        : R.string.case_yst_lan1_status_fail_text);
        mMinYSTLan2Status
                .setText(result[1] ? R.string.case_yst_lan2_status_success_text
                        : R.string.case_yst_lan2_status_fail_text);
        mMaxYSTLan2Status
                .setText(result[1] ? R.string.case_yst_lan2_status_success_text
                        : R.string.case_yst_lan2_status_fail_text);
    }

    @Override
    public void onTimeOut() {
        Log.d(TAG, "onTimeOut");
        setCaseResult(false);
        this.stopCase();
    }

}
