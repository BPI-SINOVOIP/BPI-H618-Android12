package com.softwinner.dragonbox.testcase;

import org.xmlpull.v1.XmlPullParser;

import android.content.Context;
import android.content.DialogInterface;
import android.content.DialogInterface.OnClickListener;
import android.content.DialogInterface.OnDismissListener;
import android.widget.TextView;

import com.softwinner.dragonbox.R;
import com.softwinner.dragonbox.manager.LedManager;
import com.softwinner.dragonbox.ui.DragonBoxMain;
import com.softwinner.dragonbox.entity.TestResultforSQLInfo;

public class CaseLed extends IBaseCase {
    LedManager ledManager;

    TextView mMinLEDStatus;

    public CaseLed(Context context) {
        super(context, R.string.case_led_name, R.layout.case_led_max,
                R.layout.case_led_min, TYPE_MODE_MANUAL);
        ledManager = new LedManager();
        mMinLEDStatus = (TextView) mMinView.findViewById(R.id.case_led_status);
    }

    public CaseLed(Context context, XmlPullParser xmlParser) {
        this(context);
    }

    @Override
    public void onStartCase() {
        ledManager.startFlashLed();
    }

    @Override
    public void onStopCase() {
        ledManager.stopFlashLed();
        mMinLEDStatus
                .setText(getCaseResult() ? R.string.case_led_status_success_text
                        : R.string.case_led_status_fail_text);
    }

    @Override
    public void onSetResult() {
        super.onSetResult();
        TestResultforSQLInfo testResultforSQLInfo = DragonBoxMain.getTestResultforSQLInfo();
        testResultforSQLInfo.putOneInfo(testResultforSQLInfo.mLegalKey[23], getCaseResult()+"");
    }


    @Override
    public void reset() {
        super.reset();
        mMinLEDStatus.setText(R.string.case_led_status_text);

    }

}
