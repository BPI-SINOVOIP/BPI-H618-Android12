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

import com.softwinner.dragonbox.ui.DragonBoxMain;
import com.softwinner.dragonbox.R;
import com.softwinner.dragonbox.entity.TestResultforSQLInfo;
import com.softwinner.dragonbox.manager.LedManager;
import com.softwinner.dragonbox.utils.Utils;

public class CaseCouple extends IBaseCase {
    public static final String TAG = "DragonBox-CaseCouple";
    public static final String COUPLE_RESULT = "persist.sys.dragon_powerdb";
    public static final String COUPLE_NOT_TEST = "not_test";
    public static final String COUPLE_PASS = "pass";
    public static final String COUPLE_FAIL = "fail";
    public static final String DB_ERROR = "err";
    TextView mMinCoupleInfo;
    TextView mMaxCoupleInfo;
    Context mContext;
    String mDB = null;
    String mResult = null;
    private int dbNum = 0;
    public static String coupleResult = "0:0:fail";
    public CaseCouple(Context context) {
      super(context, R.string.case_couple_name, R.layout.case_couple_max,
          R.layout.case_couple_min, TYPE_MODE_AUTO);
      mMinCoupleInfo = (TextView) mMinView.findViewById(R.id.case_couple_min_tv);
      mMaxCoupleInfo = (TextView) mMaxView.findViewById(R.id.case_couple_max_tv);
    }

    public CaseCouple(Context context, XmlPullParser xmlParser) {
      this(context);
      mContext = context;
    }

    @Override
    public void onStartCase() {
      Log.w(TAG,"onStartCase CaseCouple");
      coupleResult = SystemProperties.get(COUPLE_RESULT,COUPLE_NOT_TEST);
      Log.w(TAG,"coupleResult is "+coupleResult);
      stopCase();
      String pass = mContext.getString(R.string.case_couple_pass);
      String fail = mContext.getString(R.string.case_couple_fail);
      String notTest = mContext.getString(R.string.case_couple_not_test);
      if(COUPLE_NOT_TEST.equals(coupleResult)) {//couple not test
        setDialogPositiveButtonEnable(false);
        setCaseResult(false);
        mMinCoupleInfo.setText(mContext.getString(R.string.case_rf_info,notTest,DB_ERROR));
        mMaxCoupleInfo.setText(mContext.getString(R.string.case_rf_info,notTest,DB_ERROR));
      }else {//couple test
        String[] results = coupleResult.split(":");
        if(results!=null && results.length > 1) {
          if(results.length==2){
            mDB = results[0];
            mResult = results[1];
          }else if(results.length==3){
            mDB = results[0] + ":" + results[1];
            mResult = results[2];
          }
          if(COUPLE_PASS.equals(mResult)) {//pass
            setDialogPositiveButtonEnable(true);
            setCaseResult(true);
            mMinCoupleInfo.setText(mContext.getString(R.string.case_couple_info,mResult,mDB));
            mMaxCoupleInfo.setText(mContext.getString(R.string.case_couple_info,mResult,mDB));
          }else if(COUPLE_FAIL.equals(mResult)) {//fail
            setDialogPositiveButtonEnable(false);
            setCaseResult(false);
            mMinCoupleInfo.setText(mContext.getString(R.string.case_couple_info,mResult,mDB));
            mMaxCoupleInfo.setText(mContext.getString(R.string.case_couple_info,mResult,mDB));
          }else {//result format error
            setDialogPositiveButtonEnable(false);
            setCaseResult(false);
            mMinCoupleInfo.setText(mContext.getString(R.string.case_couple_info,"格式错误: "+coupleResult,DB_ERROR));
            mMaxCoupleInfo.setText(mContext.getString(R.string.case_couple_info,"格式错误: "+coupleResult,DB_ERROR));
          }
        } else {//format error
          setDialogPositiveButtonEnable(false);
          setCaseResult(false);
          mMinCoupleInfo.setText(mContext.getString(R.string.case_couple_info,"格式错误: "+coupleResult,DB_ERROR));
          mMaxCoupleInfo.setText(mContext.getString(R.string.case_couple_info,"格式错误: "+coupleResult,DB_ERROR));
        }
      }
      Log.w(TAG,"CaseCouple test over ,test result is "+getCaseResult());
    }

    @Override
    public void onStopCase() {

    }

    @Override
    public void onSetResult() {
      super.onSetResult();
      TestResultforSQLInfo testResultforSQLInfo = DragonBoxMain.getTestResultforSQLInfo();
      testResultforSQLInfo.putOneInfo(testResultforSQLInfo.mLegalKey[11], mDB+"db");//couple
    }

    @Override
    public void reset() {
      super.reset();
      mMinCoupleInfo.setText(R.string.case_rf_info);
      mMaxCoupleInfo.setText(R.string.case_rf_info);
    }

}
