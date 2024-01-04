package com.softwinner.dragonbox.testcase;

import java.io.File;

import org.xmlpull.v1.XmlPullParser;

import android.content.Context;
import android.content.DialogInterface;
import android.content.DialogInterface.OnClickListener;
import android.content.DialogInterface.OnDismissListener;
import android.widget.TextView;
import android.util.Log;
import android.os.SystemProperties;

import com.softwinner.dragonbox.R;
import com.softwinner.dragonbox.manager.LedManager;
import com.softwinner.dragonbox.utils.Utils;
import com.softwinner.dragonbox.ui.DragonBoxMain;
import com.softwinner.dragonbox.entity.TestResultforSQLInfo;

public class CaseRF extends IBaseCase {
    public static final String TAG = "DragonBox-CaseRF";
    public static final String RF_RESULT = "persist.sys.dragon_rf";
    public static final String RF_NOT_TEST = "not_test";
    public static final String RF_PASS = "pass";
    public static final String RF_FAIL = "fail";
    public static String RF_FILE_NAME = "null";
    TextView mMinRFInfo;
    TextView mMaxRFInfo;
    Context mContext;

    public CaseRF(Context context) {
        super(context, R.string.case_rf_name, R.layout.case_rf_max,
            R.layout.case_rf_min, TYPE_MODE_AUTO);
        mMinRFInfo = (TextView) mMinView.findViewById(R.id.case_rf_min_tv);
        mMaxRFInfo = (TextView) mMaxView.findViewById(R.id.case_rf_max_tv);
    }

    public CaseRF(Context context, XmlPullParser xmlParser) {
        this(context);
        mContext = context;
    }

    @Override
    public void onStartCase() {
        Log.w(TAG,"onStartCase CaseRf");
        String rfResult = SystemProperties.get(RF_RESULT,RF_NOT_TEST);
        RF_FILE_NAME = SystemProperties.get("persist.sys.dragon_rf_log_path","null");
        Log.w(TAG,"rfResult is "+rfResult);
        stopCase();
        String pass = mContext.getString(R.string.case_rf_pass);
        String fail = mContext.getString(R.string.case_rf_fail);
        String notTest = mContext.getString(R.string.case_rf_not_test);
        if(RF_PASS.equals(rfResult)) {
            setDialogPositiveButtonEnable(true);
            setCaseResult(true);
            mMinRFInfo.setText(mContext.getString(R.string.case_rf_info,pass));
            mMaxRFInfo.setText(mContext.getString(R.string.case_rf_info,pass));
        } else if(RF_FAIL.equals(rfResult)) {
            setDialogPositiveButtonEnable(false);
            setCaseResult(false);
            mMinRFInfo.setText(mContext.getString(R.string.case_rf_info,fail));
            mMaxRFInfo.setText(mContext.getString(R.string.case_rf_info,fail));
        } else if(RF_NOT_TEST.equals(rfResult)) {
            setDialogPositiveButtonEnable(false);
            setCaseResult(false);
            mMinRFInfo.setText(mContext.getString(R.string.case_rf_info,notTest));
            mMaxRFInfo.setText(mContext.getString(R.string.case_rf_info,notTest));
        } else {
            setDialogPositiveButtonEnable(false);
            setCaseResult(false);
            mMinRFInfo.setText(mContext.getString(R.string.case_rf_info,"格式错误: "+rfResult));
            mMaxRFInfo.setText(mContext.getString(R.string.case_rf_info,"格式错误: "+rfResult));
        }
        Log.w(TAG,"CaseRf test over ,test result is "+getCaseResult());
    }

    @Override
    public void onStopCase() {

    }

    @Override
    public void onSetResult() {
        super.onSetResult();
        TestResultforSQLInfo testResultforSQLInfo = DragonBoxMain.getTestResultforSQLInfo();
        testResultforSQLInfo.putOneInfo(testResultforSQLInfo.mLegalKey[25], getCaseResult()+"");
        testResultforSQLInfo.putOneInfo(testResultforSQLInfo.mLegalKey[24], RF_FILE_NAME);
    }

    @Override
    public void reset() {
        super.reset();
        mMinRFInfo.setText(R.string.case_rf_info);
        mMaxRFInfo.setText(R.string.case_rf_info);
    }

}
