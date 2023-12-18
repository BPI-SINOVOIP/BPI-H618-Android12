package com.softwinner.dragonbox.testcase;

import org.xmlpull.v1.XmlPullParser;

import android.content.Context;
import android.util.Log;
import android.widget.TextView;
import android.os.SystemProperties;

import com.softwinner.dragonbox.ui.DragonBoxMain;
import com.softwinner.dragonbox.R;
import com.softwinner.dragonbox.entity.TestResultforSQLInfo;
import com.softwinner.dragonbox.utils.Utils;


public class CaseAging extends IBaseCase {
    public static final String TAG = "DragonBox-CaseAging";
    int mAgingTimeThreshold =0;
    String mAgingTime;//配置文件中的时间, 单位分钟
    String mStrAgingTime = "";//当前老化时间, 格式:xx小时xx分钟
    TextView mMinAgingStatus;
    TextView mMaxAgingStatus;
    Context mContext;
    public static String aging_time="0";

    public CaseAging(Context context) {
        super(context, R.string.case_aging_name, R.layout.case_aging_max,
                R.layout.case_aging_min, TYPE_MODE_AUTO);
        mMinAgingStatus = (TextView) mMinView.findViewById(R.id.case_aging_min_info);
        mMaxAgingStatus = (TextView) mMaxView.findViewById(R.id.case_aging_max_info);
        mContext = context;
    }

    public CaseAging(Context context, XmlPullParser xmlParser) {
        this(context);
        mAgingTime = xmlParser.getAttributeValue(null, "agingTime");
        mAgingTimeThreshold = Integer.parseInt(mAgingTime)*60*1000;//单位ms
    }

    @Override
    public void onStartCase() {
        Log.w(TAG,"onStartCase");
        int agingTime = SystemProperties.getInt(DragonBoxMain.PROPERTY_DRAGONAGING_TIME,0);
        Log.w(TAG,"agingTime is "+agingTime+"mAgingTimeThreshold is "+mAgingTimeThreshold);
        mStrAgingTime = Utils.formatTime(agingTime);
        aging_time = mStrAgingTime;
        mMinAgingStatus.setText(mContext.getString(R.string.case_aging_info, mStrAgingTime, mAgingTime));
        mMaxAgingStatus.setText(mContext.getString(R.string.case_aging_info, mStrAgingTime, mAgingTime));
        stopCase();
        setCaseResult(agingTime>=mAgingTimeThreshold);
        setDialogPositiveButtonEnable(agingTime>=mAgingTimeThreshold);
    }

    @Override
    public void onStopCase() {
        Log.w(TAG,"onStopCase, case result is "+getCaseResult());
    }

    @Override
    public void onSetResult() {
        super.onSetResult();
        TestResultforSQLInfo testResultforSQLInfo = DragonBoxMain.getTestResultforSQLInfo();
        testResultforSQLInfo.putOneInfo(testResultforSQLInfo.mLegalKey[12], mStrAgingTime);//aging
    }

    @Override
    public void reset() {
        super.reset();
    }

}
