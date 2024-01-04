package com.softwinner.dragonbox.testcase;

import java.io.BufferedReader;
import java.io.File;
import java.io.FileInputStream;
import java.io.FileReader;
import java.io.IOException;
import java.io.InputStreamReader;

import org.xmlpull.v1.XmlPullParser;

import android.content.Context;
import android.util.Log;
import android.widget.TextView;

import com.softwinner.dragonbox.ui.DragonBoxMain;
import com.softwinner.dragonbox.R;
import com.softwinner.dragonbox.entity.TestResultforSQLInfo;

public class CaseCPU extends IBaseCase {
    TextView mMinCPUStatus;
    TextView mMaxCPUStatus;
    Context mContext;
    int mTempThreshold;
    int mAverageTemp;//三次温度平均
    private static final String TEMPERATURE_NODE = "/sys/devices/virtual/thermal/thermal_zone0/temp";
    private static final String TAG = "DragonBox-CaseCPU";
    public static String cpu="null";

    public CaseCPU(Context context) {
        super(context, R.string.case_cpu_name, R.layout.case_cpu_max,
                R.layout.case_cpu_min, TYPE_MODE_AUTO);
        mMinCPUStatus = (TextView) mMinView.findViewById(R.id.case_cpu_min_status);
        mMaxCPUStatus = (TextView) mMaxView.findViewById(R.id.case_cpu_max_info);
        mContext = context;
    }

    public CaseCPU(Context context, XmlPullParser xmlParser) {
        this(context);
        String tempThreshold = xmlParser.getAttributeValue(null,"tempThreshold");
        mTempThreshold = Integer.parseInt(tempThreshold);
    }

    @Override
    public void onStartCase() {
        Log.w(TAG,"onStartCase");

        int temp1 = getTemperature();
        try {
            Thread.sleep(100);
        } catch (InterruptedException e) {
            // TODO 自动生成的 catch 块
            e.printStackTrace();
        }
        int temp2 = getTemperature();
        try {
            Thread.sleep(100);
        } catch (InterruptedException e) {
            // TODO 自动生成的 catch 块
            e.printStackTrace();
        }
        int temp3 = getTemperature();

        mAverageTemp = (temp1+temp2+temp3)/3;
        Log.w(TAG,"averageTemperature is "+mAverageTemp);
        mMinCPUStatus.setText(mContext.getString(R.string.case_cpu_info, mAverageTemp+"", mTempThreshold+""));
        mMaxCPUStatus.setText(mContext.getString(R.string.case_cpu_info, mAverageTemp+"", mTempThreshold+""));
        if(mAverageTemp>mTempThreshold) {
            stopCase();
            setCaseResult(false);
            setDialogPositiveButtonEnable(false);
        }else {
            stopCase();
            setCaseResult(true);
        }
        cpu = mAverageTemp + "";
    }
    int getTemperature() {
        BufferedReader br = null;
        String temp = "0";
        try {
            br = new BufferedReader(new InputStreamReader(new FileInputStream(TEMPERATURE_NODE)));
            br.mark(100);
            temp = br.readLine();
            Log.w(TAG,"temperature is "+temp);
        } catch (Exception e) {
            e.printStackTrace();
        }finally {
            if (br!=null) {
                try {
                    br.close();
                    br=null;
                } catch (IOException e) {
                    e.printStackTrace();
                }
            }
        }
        return Integer.parseInt(temp);
    }

    @Override
    public void onStopCase() {
        Log.w(TAG,"onStopCase, case result is "+getCaseResult());

    }

    @Override
    public void onSetResult() {
        super.onSetResult();
        TestResultforSQLInfo testResultforSQLInfo = DragonBoxMain.getTestResultforSQLInfo();
        testResultforSQLInfo.putOneInfo(testResultforSQLInfo.mLegalKey[13], mAverageTemp+"°");//cpu
    }

    @Override
    public void reset() {
        super.reset();
    }

}
