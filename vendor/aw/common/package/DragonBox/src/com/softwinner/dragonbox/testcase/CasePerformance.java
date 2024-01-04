package com.softwinner.dragonbox.testcase;

import org.xmlpull.v1.XmlPullParser;

import android.content.Context;
import android.os.Handler;
import android.os.Message;
import android.widget.ProgressBar;
import android.widget.TextView;

import com.softwinner.dragonbox.R;
import com.softwinner.dragonbox.manager.PerformanceManager;
import com.softwinner.dragonbox.manager.PerformanceManager.OnCpuFreqChangeListener;

public class CasePerformance extends IBaseCase implements OnCpuFreqChangeListener{
    PerformanceManager performanceManager;

    TextView mMaxCpuFreqMaxTV;
    ProgressBar mMaxCpuFreqProgressBar;
    TextView mMaxCpuFreqMinTV;
    TextView mMaxCpuFreqCur;
    TextView mMaxCpuFreqStatus;

    TextView mMinMinCpuFreq;
    TextView mMinMaxCpuFreq;
    TextView mMinCpuStatus;

    Handler mHandler = new Handler() {
        public void handleMessage(Message msg) {

        };
    };

    public CasePerformance(Context context) {
        super(context, R.string.case_performance_name,
                R.layout.case_performance_max, R.layout.case_performance_min,
                TYPE_MODE_AUTO);
        mMaxCpuFreqMaxTV = (TextView) mMaxView.findViewById(R.id.case_performance_max);
        mMaxCpuFreqProgressBar = (ProgressBar) mMaxView.findViewById(R.id.case_performance_progress_bar);
        mMaxCpuFreqMinTV = (TextView) mMaxView.findViewById(R.id.case_performance_min);
        mMaxCpuFreqCur = (TextView) mMaxView.findViewById(R.id.case_performance_cur_cpu_freq);
        mMaxCpuFreqStatus = (TextView) mMaxView.findViewById(R.id.case_performance_cur_cpu_status);

        mMinMinCpuFreq = (TextView) mMinView.findViewById(R.id.case_performance_min_cpu_freq);
        mMinMaxCpuFreq = (TextView) mMinView.findViewById(R.id.case_performance_max_cpu_freq);
        mMinCpuStatus = (TextView) mMinView.findViewById(R.id.case_performance_cpu_status);

        performanceManager = new PerformanceManager(mContext);
        performanceManager.setOnCpuFrqChangeListener(this);
        String maxFreq = performanceManager.getMaxCpuFreq() / 1000 + "MHZ";
        String minFreq = performanceManager.getMinCpuFreq() / 1000 + "MHZ";
        mMaxCpuFreqMinTV.setText(minFreq);
        mMaxCpuFreqMaxTV.setText(maxFreq);
        mMinMinCpuFreq.setText(mContext.getString(R.string.case_performance_min_cpu_freq, minFreq));
        mMinMaxCpuFreq.setText(mContext.getString(R.string.case_performance_max_cpu_freq, maxFreq));
    }

    public CasePerformance(Context context, XmlPullParser xmlParser) {
        this(context);
    }

    @Override
    public void onStartCase() {
        performanceManager.startChangeCPUFreq();
    }

    @Override
    public void onStopCase() {
        performanceManager.stopChangeCPUFreq();
    }

    @Override
    public void onCpuFreqChangeERR() {
        mMaxCpuFreqStatus.setText(R.string.case_performance_cur_cpu_status_fail_text);
        mMinCpuStatus.setText(R.string.case_performance_cur_cpu_status_fail_text);
    }

    @Override
    public void onCpuFreqChangeStart() {
        mMaxCpuFreqStatus.setText(R.string.case_performance_cur_cpu_status_start);
        mMinCpuStatus.setText(R.string.case_performance_cur_cpu_status_start);
    }

    @Override
    public void onCpuFreqChanging(int cpuMax, int cpuMin, int cpuFreq) {
        // TODO Auto-generated method stub
        mMaxCpuFreqProgressBar.setMax(cpuMax - cpuMin);
        mMaxCpuFreqProgressBar.setProgress(cpuFreq - cpuMin);
        mMaxCpuFreqCur.setText(cpuFreq / 1000 + "MHZ");
        mMaxCpuFreqStatus.setText(R.string.case_performance_cur_cpu_status_running);
        mMinCpuStatus.setText(R.string.case_performance_cur_cpu_status_running);
        //mMinCpuFreqCur.setText(cpuFreq / 1000 + "MHZ");

    }

    @Override
    public void onCpuFreqChangeEnd(boolean result) {
        int resId = result ? R.string.case_performance_cur_cpu_status_success_text : R.string.case_performance_cur_cpu_status_fail_text;
        mMaxCpuFreqStatus.setText(resId);
        mMinCpuStatus.setText(resId);
        setCaseResult(result);
        stopCase();
    }

    @Override
    public void reset() {
        super.reset();
        mMinCpuStatus.setText(R.string.case_performance_cur_cpu_status);
    }
}
