package com.softwinner.agingdragonbox.engine.testcase;

import android.view.ViewGroup;

import java.util.Timer;
import java.util.TimerTask;

import com.softwinner.agingdragonbox.R;
import com.softwinner.agingdragonbox.ThreadVar;
import com.softwinner.agingdragonbox.engine.BaseCase;
import com.softwinner.agingdragonbox.xml.Node;

public class CaseGPUStress extends BaseCase {
    private EarthAndMoonView testView;
    private ViewGroup        iewGroup;
    private TimerTask        mTimerTask;
    private Timer            mTimer         = new Timer();
    private boolean          threadExit     = true;
    private boolean          threadExitDdr = true;
    private static int       ledTime        = 500;
    Thread                   browseThread   = null;
    ThreadVar                threadVar     = new ThreadVar();

    @Override
    protected void onInitialize(Node attr) {
        setView(R.layout.case_threedimensional);
        setName(R.string.case_memory_name);
        testView = new EarthAndMoonView(mContext);
        iewGroup = (ViewGroup) getView().findViewById(R.id.myViewGroup);
    }

    @Override
    protected boolean onCaseStarted() {
        iewGroup.addView(testView, new ViewGroup.LayoutParams(ViewGroup.LayoutParams.MATCH_PARENT,
                ViewGroup.LayoutParams.MATCH_PARENT));
        return false;
    }

    @Override
    protected void onCaseFinished() {

    }

    @Override
    protected void onRelease() {

    }

    private void chanceLedStatus(int status) {
        if (status > 0) {
            com.softwinner.Gpio.setNormalLedOn(false);
            com.softwinner.Gpio.setStandbyLedOn(true);
        } else {
            com.softwinner.Gpio.setNormalLedOn(true);
            com.softwinner.Gpio.setStandbyLedOn(false);
        }
    }

    private void startCtrlLedThread() {
        browseThread = new Thread() {
            public void run() {
                try {
                    threadExit = ThreadVar.threadExit;
                    threadExitDdr = ThreadVar.threadExitDdr;
                    while (threadExitDdr && threadExit) {
                        if (ledTime <= 0) {
                            ledTime = 500;
                        }
                        chanceLedStatus(0);
                        Thread.sleep(ledTime);
                        chanceLedStatus(1);
                        Thread.sleep(ledTime);
                        threadExit = ThreadVar.threadExit;
                        threadExitDdr = ThreadVar.threadExitDdr;
                    }
                    chanceLedStatus(0);
                } catch (Exception e) {
                    e.printStackTrace();
                }

            }
        };
        browseThread.start();
    }

}
