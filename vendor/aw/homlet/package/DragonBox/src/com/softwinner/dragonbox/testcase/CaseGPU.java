package com.softwinner.dragonbox.testcase;

import java.text.NumberFormat;

import org.xmlpull.v1.XmlPullParser;

import android.content.Context;
import android.os.Handler;
import android.os.Message;
import android.util.Log;
import android.view.ViewGroup;
import android.widget.LinearLayout;
import android.widget.TextView;

import com.softwinner.dragonbox.R;
import com.softwinner.dragonbox.view.ThreeDimensionalView;
import com.softwinner.SystemMix;

public class CaseGPU extends IBaseCase {
    private static final String TAG = "DragonBox-CaseGpu";
    private static final String GPU_UTILIZATION_PATH = "/sys/kernel/debug/mali/utilization_gp_pp";
    private static final int WHAT_GET_GPU_UTILIZATION = 0;
    private static final int GET_GPU_UTILIZATION_DELAY = 1000;//ms
    private static final int GET_GPU_UTILIZATION_TIMES = 4;
    int mGetGPUUtilizationTimes = 0;
    double mAverageUtilization = 0;
    double mSumUtilization = 0;
    double mUtilizationThreshold = 0.05;
    String mstrUtilizationThreshold;//配置文件中的利用率, 如3%.
    TextView mMinGPUStatus;
    LinearLayout mLLGPU;
    ThreeDimensionalView mThreeDimensionalView;
    Context mContext;
    NumberFormat mNumberFormat = NumberFormat.getPercentInstance();

    Handler mHandler = new Handler() {
        public void handleMessage(android.os.Message msg) {
            switch (msg.what) {
                case WHAT_GET_GPU_UTILIZATION:
                    String strUtilization = SystemMix.readFile(GPU_UTILIZATION_PATH);
                    Log.w(TAG,"strUtilization is "+strUtilization);
                    if(strUtilization!=null){
                        try {
                            Number nu = mNumberFormat.parse(strUtilization);
                            double utilization = nu.doubleValue();
                            Log.w(TAG,String.format("utilization is %.2f",utilization));
                            mSumUtilization +=utilization;
                        } catch (Exception e) {
                            e.printStackTrace();
                        }
                    }
                    mGetGPUUtilizationTimes++;
                    if(mGetGPUUtilizationTimes<GET_GPU_UTILIZATION_TIMES) {
                        sendEmptyMessageDelayed(WHAT_GET_GPU_UTILIZATION, GET_GPU_UTILIZATION_DELAY);
                    }else {
                        mAverageUtilization = mSumUtilization/GET_GPU_UTILIZATION_TIMES;
                        Log.w(TAG,"mAverageUtilization is " + mAverageUtilization);
                        mNumberFormat.setMaximumFractionDigits(2);
                        mMinGPUStatus.setText(mContext.getString(R.string.case_GPU_status_text, mNumberFormat.format(mAverageUtilization), mstrUtilizationThreshold));
                        if(mAverageUtilization > mUtilizationThreshold) {
                            setCaseResult(true);
                        }else {
                            setCaseResult(false);
                        }
                        mMaxViewDialog.dismiss();
                    }
                    break;

                default:
                    break;
            }
        };
    };

    public CaseGPU(Context context) {
        super(context, R.string.case_GPU_name, R.layout.case_gpu_max,
                R.layout.case_gpu_min, TYPE_MODE_MANUAL);
        mContext = context;
        mMinGPUStatus = (TextView) mMinView.findViewById(R.id.case_GPU_min_status);
        mLLGPU = (LinearLayout) mMaxView.findViewById(R.id.ll_gpu);
    }

    public CaseGPU(Context context, XmlPullParser xmlParser) {
        this(context);
        mstrUtilizationThreshold = xmlParser.getAttributeValue(null, "utilization");
        if(mstrUtilizationThreshold==null) {
            return;
        }
        try {
            Number nu = mNumberFormat.parse(mstrUtilizationThreshold);
            mUtilizationThreshold = nu.doubleValue();
            Log.w(TAG,"utilizationThreshold string is "+mstrUtilizationThreshold+" value is "+mUtilizationThreshold);
        } catch (Exception e) {
            e.printStackTrace();
        }
    }

    @Override
    public void onStartCase() {
        Log.w(TAG,"onStartCase");
        mThreeDimensionalView = new ThreeDimensionalView(mContext);
        mLLGPU.addView(mThreeDimensionalView, new ViewGroup.LayoutParams(ViewGroup.LayoutParams.MATCH_PARENT, ViewGroup.LayoutParams.MATCH_PARENT));
        mThreeDimensionalView.setZOrderOnTop(true);
        setDialogPositiveButtonEnable(false);
        mHandler.sendEmptyMessageDelayed(WHAT_GET_GPU_UTILIZATION, GET_GPU_UTILIZATION_DELAY);
    }

    @Override
    public void onStopCase() {
        Log.w(TAG,"onStopCase, case result is "+getCaseResult());
        mGetGPUUtilizationTimes = 0;
        mAverageUtilization = 0;
        mSumUtilization = 0;
        if(mHandler.hasMessages(WHAT_GET_GPU_UTILIZATION)) {
            Log.w(TAG,"mHandler remove message: WHAT_GET_GPU_UTILIZATION");
            mHandler.removeMessages(WHAT_GET_GPU_UTILIZATION);
        }

    }

    @Override
    public void reset() {
        super.reset();
        mGetGPUUtilizationTimes = 0;
        mAverageUtilization = 0;
        mSumUtilization = 0;
    }

}
