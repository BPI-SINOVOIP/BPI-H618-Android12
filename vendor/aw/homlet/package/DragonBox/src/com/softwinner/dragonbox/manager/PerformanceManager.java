package com.softwinner.dragonbox.manager;

import android.content.Context;
import android.os.Handler;

import com.softwinner.dragonbox.platform.CommonPlatformUtil;
import com.softwinner.dragonbox.utils.SystemUtil;

public class PerformanceManager {
    public Context mContext;
    private boolean mRunning = false;

    private int mCpuFreqMax = -1;
    private int mCpuFreqMin = -1;
    private int mCpuFreqCur;

    private int[] mCpuFreqStep;

    private Handler mHandler = new Handler();
    private OnCpuFreqChangeListener mCpuFreqChangeListener;

    public PerformanceManager(Context context) {
        mContext = context;
        try {
            mCpuFreqMax = Integer.parseInt(SystemUtil
                    .getCpuFreq(SystemUtil.TYPE_CPU_MAX));
            mCpuFreqMin = Integer.parseInt(SystemUtil
                    .getCpuFreq(SystemUtil.TYPE_CPU_MIN));
        } catch (Exception e) {
            e.printStackTrace();
        }
        mCpuFreqStep = CommonPlatformUtil.getCpuFreqStep();
    }

    public void setOnCpuFrqChangeListener(
            OnCpuFreqChangeListener cpuFreqChangeListener) {
        mCpuFreqChangeListener = cpuFreqChangeListener;
    }

    public void startChangeCPUFreq() {
        if (mCpuFreqMax == -1 || mCpuFreqMin == -1) {
            if (mCpuFreqChangeListener != null) {
                mCpuFreqChangeListener.onCpuFreqChangeERR();
            }
            return;
        }
        new Thread(new Runnable() {

            @Override
            public void run() {
                mRunning = true;
                if (mCpuFreqChangeListener != null) {
                    mHandler.post(new Runnable() {
                        @Override
                        public void run() {
                            mCpuFreqChangeListener.onCpuFreqChangeStart();
                        }
                    });
                }
                int cpuFreqIndex = 0;
                int runLoopTimes = 2;
                while (mRunning) {

                    final int cpufreqCur = mCpuFreqStep[cpuFreqIndex];
                    SystemUtil.setCpuFreq(cpufreqCur);
                    if (mCpuFreqChangeListener != null) {
                        mHandler.post(new Runnable() {
                            @Override
                            public void run() {
                                mCpuFreqChangeListener
                                        .onCpuFreqChanging(mCpuFreqMax, mCpuFreqMin, cpufreqCur);
                            }
                        });
                    }
                    try {
                        Thread.sleep(40);
                    } catch (InterruptedException e) {
                        // TODO Auto-generated catch block
                        e.printStackTrace();
                    }

                    cpuFreqIndex++;
                    if (cpuFreqIndex >= mCpuFreqStep.length) {
                        cpuFreqIndex = 0;
                        runLoopTimes--;
                    }

                    if (runLoopTimes == 0) {
                        mRunning = false;
                    }
                }
                final int times = runLoopTimes;
                if (mCpuFreqChangeListener != null) {
                    mHandler.post(new Runnable() {
                        @Override
                        public void run() {
                            mCpuFreqChangeListener.onCpuFreqChangeEnd(times == 0);
                        }
                    });
                }
            }
        }).start();
    }

    public void stopChangeCPUFreq() {
        mRunning = false;
        SystemUtil.resetCPU(mCpuFreqMin, mCpuFreqMax);
    }

    public int getMaxCpuFreq() {
        return mCpuFreqMax;
    }

    public int getMinCpuFreq() {
        return mCpuFreqMin;
    }

    public interface OnCpuFreqChangeListener {
        void onCpuFreqChangeERR();

        void onCpuFreqChangeStart();

        void onCpuFreqChanging(int cpuMax, int cpuMin, int cpuFreq);

        void onCpuFreqChangeEnd(boolean result);
    }

}
