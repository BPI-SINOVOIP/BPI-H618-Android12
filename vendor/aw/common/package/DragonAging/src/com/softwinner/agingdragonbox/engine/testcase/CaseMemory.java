package com.softwinner.agingdragonbox.engine.testcase;

import android.content.SharedPreferences;
import android.os.Handler;
import android.os.Message;
import android.os.storage.StorageManager;
import android.os.SystemProperties;
import android.util.Log;
import android.view.ViewGroup;
import android.widget.ArrayAdapter;
import android.widget.ProgressBar;
import android.widget.TextView;
import android.widget.ScrollView;

import java.io.BufferedReader;
import java.io.BufferedWriter;
import java.io.DataOutputStream;
import java.io.File;
import java.io.FileOutputStream;
import java.io.IOException;
import java.io.InputStream;
import java.io.InputStreamReader;
import java.io.OutputStreamWriter;
import java.text.SimpleDateFormat;
import java.util.ArrayList;
import java.util.Date;

import com.softwinner.agingdragonbox.R;
import com.softwinner.agingdragonbox.ThreadVar;
import com.softwinner.agingdragonbox.engine.BaseCase;
import com.softwinner.agingdragonbox.xml.Node;
import com.softwinner.agingdragonbox.engine.Utils;

public class CaseMemory extends BaseCase {

    private static final String      TAG                   = "DragonAging-CaseMemory";
    public static final String       PASSABLE_MIN_CAP      = "minCap";
    private StorageManager           mStorageManager;
    private ArrayAdapter<String>     mAdapter;
    private int                      mMinCapInMB           = -1;
    // 用来返回结果的，直接加上一个变量
    private long                     mAvailSize            = 0;
    private static final String      CMD_PATH = "/system/bin/memtester";
       // = "/data/data/com.softwinner.agingdragonbox/cache/memtester";
    static final boolean             DEBUG                 = false;
    protected ViewGroup              mStage;
    private boolean                  mIsRunning            = false;
    private boolean                  mResult               = false;
    private ViewGroup                myViewGroup;
    private int                      memSize               = 128;
    private int                      repeat;
    private SharedPreferences        mSharedPreferences;
    private SharedPreferences.Editor mEditor;
    private static final int         HANDLER_UPDATE_OUTPUT = 0;
    private static final int         HANDLER_FINISHED      = 1;
    private static final int         HANDLER_PROGRESS      = 2;
    private ProgressBar              mProgressBar;
    private TextView                 mResultTextView;
    private TextView                 mTitleTextView;//title+aging time
    private TextView                 outputWindow;
    private ScrollView               svMeminfo;   
    private boolean                  threadExitDdr;
    private boolean                  threadExit;
    StringBuilder                    sb                    = new StringBuilder();
    String                           cmd;
    private boolean                  isShow;
    private static final int         MAX_INFO_LINE         = 24;
    private boolean                  mRefresh              = false;
    ArrayList<String> arrLines = new ArrayList<String>(MAX_INFO_LINE);

    private static int               ledTime               = 500;
    Thread                           browseThread          = null;
    ThreadVar                        threadVar            = new ThreadVar();

    private Handler                  myHandler             = new Handler() {
        @Override
        public void handleMessage(Message msg) {
            switch (msg.what) {
                case HANDLER_UPDATE_OUTPUT:
                    refeshOutputWindow();
                    break;
                default:
                    break;
            }
        }
    };

    public void initCMDFile() {

        File cmdFile = new File(CMD_PATH);

        if (!cmdFile.exists()) {
            Log.d(TAG, " create memtester file on cache.");
            // copy memtest from assets of app to cache directory.
            InputStream is = null;
            FileOutputStream fos = null;
            try {
                is = mContext.getAssets().open("memtester");
                fos = new FileOutputStream(cmdFile);

                byte[] buff = new byte[2048];
                int length = 0;
                while ((length = is.read(buff)) != -1) {
                    fos.write(buff, 0, length);
                }
                fos.flush();
            } catch (IOException e) {
                e.printStackTrace();
            } finally {
                if (is != null) {
                    try {
                        is.close();
                    } catch (IOException e) {
                        e.printStackTrace();
                    }
                }

                if (fos != null) {
                    try {
                        fos.close();
                    } catch (IOException e) {
                        e.printStackTrace();
                    }
                }
            }
        }
        // change memtest in cache permission

        Process p = null;
        DataOutputStream dos = null;

        try {
            // p = Runtime.getRuntime().exec("su");
            //
            // dos = new DataOutputStream(p.getOutputStream());
            //
            // dos.writeBytes("chmod 777 " + CMD_PATH);
            // dos.flush();

            Runtime.getRuntime().exec("chmod 6755 " + CMD_PATH);
        } catch (Exception e) {
            e.printStackTrace();
        } finally {
            if (dos != null) {
                try {
                    dos.close();
                } catch (IOException e) {
                    e.printStackTrace();
                }
            }
        }
    }

    private void refeshOutputWindow() {
    	sb.setLength(0);
        if (mRefresh) {
            synchronized (this) {
                for (int i = 0; i < arrLines.size(); i++) {
                    sb.append(arrLines.get(i));
                    Log.w(TAG, "Line" + i + ": " + arrLines.get(i));
                }
                mRefresh = false;
            }
            outputWindow.setText(sb.toString());
        }
        if(mTitleTextView!=null){
            String title = mContext.getResources().getString(R.string.memory_testing);
            int agingtime = SystemProperties.getInt(Utils.PROPERTY_DRAGONAGING_TIME,0);
            mTitleTextView.setText(title+"  "+formatTime(agingtime));
        }
        svMeminfo.fullScroll(ScrollView.FOCUS_DOWN);
        if (isShow) {
            myHandler.sendEmptyMessageDelayed(HANDLER_UPDATE_OUTPUT, 1000);
        }
    }

    private String formatTime(int millionSeconds){
        int msRemained = millionSeconds%1000;//毫秒余数
        int secondsTotal = millionSeconds/1000;//总秒数
        int secondsRemained = secondsTotal%60;//秒余数
        int minutesTotal = secondsTotal/60;//总分钟数
        int minutesRemained = minutesTotal%60;//分钟余数
        int hours = minutesTotal/60;//总小时数
        if(msRemained!=0)
            return hours+"小时"+minutesRemained+"分"+secondsRemained+"秒"+msRemained;
        else if(secondsRemained!=0)
            return hours+"小时"+minutesRemained+"分"+secondsRemained+"秒";
        else return hours+"小时"+minutesRemained+"分";
    }

    class TestRunnable implements Runnable {

        @Override
        public void run() {
            if (memSize <= 0) {
                return;
            }
            initCMDFile();
            myHandler.sendEmptyMessage(HANDLER_UPDATE_OUTPUT);
            BufferedWriter dos = null;
            BufferedReader dis = null;

            Process p = null;

            try {
                p = Runtime.getRuntime().exec("qw");

                cmd = String.format("%s %dm %d\n", CMD_PATH, memSize, repeat);

                dos = new BufferedWriter(new OutputStreamWriter(p.getOutputStream()));
                dis = new BufferedReader(new InputStreamReader(p.getInputStream()));

                Log.d(TAG, "ddr test start........");

                Log.v(TAG, "=========run cmd=========" + cmd);
                dos.write(cmd);
                dos.flush();
                synchronized (this) {
                    arrLines.add("start memtest\n");
                }
                String line = null;
                Log.d(TAG, "ddr test start11........");

                // Record start time of testing to log file.
                SimpleDateFormat sdf = new SimpleDateFormat("yyyy-MM-dd hh:mm:ss");
                String timeStr = sdf.format(new Date(System.currentTimeMillis()));
                String spliteLine = "-----------------------------------------------------\n";
                String testInfo = timeStr + "    " + cmd + "\n";

                while ((line = dis.readLine()) != null) {
                    synchronized (this) {
                        mRefresh = true;// 生产者
                        Log.d(TAG, "ddr test readLine........");
                        if (arrLines.size() > MAX_INFO_LINE) {
                            for (int i = 0; i < arrLines.size() - MAX_INFO_LINE; i++) {
                                arrLines.remove(0);
                            }
                        }
                        if (line.length() > 15 && "Stuck Address".equals(line.substring(2, 15))) {
                            Log.d(TAG, "ddr test Stuck Address........");
                            if (line.endsWith("ok")) {
                                arrLines.add("  Stuck Address        : ok\n");
                            } else {
                                threadVar.threadExitDdr = false;
                                arrLines.add("  Stuck Address        : failed\n");
                                Log.d(TAG, "ddr test ........Stuck Address        : failed");
                            }
                        } else if (line.length() > 14 && "Random Value".equals(line.substring(2, 14))) {
                            Log.d(TAG, "ddr test Random Value........");
                            if (line.endsWith("ok")) {
                                arrLines.add("  Random Value        : ok\n");
                            } else {
                                threadVar.threadExitDdr = false;
                                arrLines.add("  Random Value        : failed\n");
                                Log.d(TAG, "ddr test ........Random Value        : failed");
                            }
                        } else if (line.length() > 14 && "Checkerboard".equals(line.substring(2, 14))) {
                            Log.d(TAG, "ddr test Checkerboard........");
                            if (line.endsWith("ok")) {
                                arrLines.add("  Checkerboard        : ok\n");
                            } else {
                                threadVar.threadExitDdr = false;
                                arrLines.add("  Checkerboard        : failed\n");
                                Log.d(TAG, "ddr test ........Checkerboard        : failed");
                            }
                        } else if (line.length() > 12 && "Solid Bits".equals(line.substring(2, 12))) {
                            Log.d(TAG, "ddr test Solid Bits........");
                            if (line.endsWith("ok")) {
                                arrLines.add("  Solid Bits        : ok\n");
                            } else {
                                threadVar.threadExitDdr = false;
                                arrLines.add("  Solid Bits        : failed\n");
                                Log.d(TAG, "ddr test ........Solid Bits        : failed");
                            }
                        } else if (line.length() > 18 && "Block Sequential".equals(line.substring(2, 18))) {
                            Log.d(TAG, "ddr test Block Sequential........");
                            if (line.endsWith("ok")) {
                                arrLines.add("  Block Sequential        : ok\n");
                            } else {
                                threadVar.threadExitDdr = false;
                                arrLines.add("  Block Sequential        : failed\n");
                                Log.d(TAG, "ddr test ........Solid Bits        : failed");
                            }
                        } else if (line.length() > 12 && "Bit Spread".equals(line.substring(2, 12))) {
                            Log.d(TAG, "ddr test Bit Spread........");
                            if (line.endsWith("ok")) {
                                arrLines.add("  Bit Spread        : ok\n");
                            } else {
                                threadVar.threadExitDdr = false;
                                arrLines.add("  Bit Spread        : failed\n");
                                Log.d(TAG, "ddr test ........Bit Spread        : failed");
                            }
                        } else if (line.length() > 10 && "Bit Flip".equals(line.substring(2, 10))) {
                            Log.d(TAG, "ddr test Bit Flip........");
                            if (line.endsWith("ok")) {
                                arrLines.add("  Bit Flip        : ok\n");
                            } else {
                                threadVar.threadExitDdr = false;
                                arrLines.add("  Bit Flip        : failed\n");
                                Log.d(TAG, "ddr test ........Bit Flip        : failed");
                            }
                        } else if (line.length() > 14 && "Walking Ones".equals(line.substring(2, 14))) {
                            Log.d(TAG, "ddr test Walking Ones........");
                            if (line.endsWith("ok")) {
                                arrLines.add("  Walking Ones        : ok\n");
                            } else {
                                threadVar.threadExitDdr = false;
                                arrLines.add("  Walking Ones        : failed\n");
                                Log.d(TAG, "ddr test ........Walking Ones        : failed");
                            }
                        } else if (line.length() > 16 && "Walking Zeroes".equals(line.substring(2, 16))) {
                            Log.d(TAG, "ddr test Walking Zeroes........");
                            if (line.endsWith("ok")) {
                                arrLines.add("  Walking Zeroes        : ok\n");
                            } else {
                                threadVar.threadExitDdr = false;
                                arrLines.add("  Walking Zeroes        : failed\n");
                                Log.d(TAG, "ddr test ........Walking Zeroes        : failed");
                            }
                        } else if (line.length() > 14 && "8-bit Writes".equals(line.substring(2, 14))) {
                            Log.d(TAG, "ddr test 8-bit Writes........");
                            if (line.endsWith("ok")) {
                                arrLines.add("  8-bit Writes        : ok\n");
                            } else {
                                threadVar.threadExitDdr = false;
                                arrLines.add("  8-bit Writes        : failed\n");
                                Log.d(TAG, "ddr test ........8-bit Writes        : failed");
                            }
                        }else if (line.length() > 15 && "16-bit Writes".equals(line.substring(2, 15))) {
                            Log.d(TAG, "ddr test 16-bit Writes........");
                            if (line.endsWith("ok")) {
                                arrLines.add("  16-bit Writes        : ok\n");
                            } else {
                                threadVar.threadExitDdr = false;
                                arrLines.add("  16-bit Writes        : failed\n");
                                Log.d(TAG, "ddr test ........16-bit Writes        : failed");
                            }
                        } else {
                            arrLines.add(line + "\n");
                        }

                        if (line.startsWith("Done")) {
                            break;
                        }
                    }
                }
                Log.i(TAG, "ddr test end.");
            } catch (IOException e) {
                e.printStackTrace();
            } finally {
                if (dos != null) {
                    try {

                        dos.close();
                    } catch (IOException e) {
                        e.printStackTrace();
                    }
                }

                if (dis != null) {
                    try {
                        dis.close();
                    } catch (IOException e) {
                        e.printStackTrace();
                    }
                }

                if (p != null) {
                    p.destroy();
                }

                // memSize = 0;
                repeat = 1;
                cmd = null;
            }
        }

    };

    @Override
    protected void onInitialize(Node attr) {
        setView(R.layout.memory_test);
        setName(R.string.case_memory_name);
        outputWindow = (TextView) getView().findViewById(R.id.output_window);
        mTitleTextView = (TextView) getView().findViewById(R.id.memory_result_textview);
        svMeminfo = (ScrollView) getView().findViewById(R.id.sv_meminfo);
    }

    @Override
    protected boolean onCaseStarted() {
        Log.i(TAG, "start MemoryTest");
        mIsRunning = true;
        mResult = false;
        isShow = true;
        new Thread(new TestRunnable()).start();
        //startCtrlLedThread();
        return false;
    }

    @Override
    protected void onCaseFinished() {
        // mContext.unregisterReceiver(mStorageReceiver);
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
                    threadExit = threadVar.threadExit;
                    threadExitDdr = threadVar.threadExitDdr;
                    while (threadExitDdr && threadExit) {
                        if (ledTime <= 0) {
                            ledTime = 500;
                        }
                        chanceLedStatus(0);
                        Thread.sleep(ledTime);
                        chanceLedStatus(1);
                        Thread.sleep(ledTime);
                        threadExit = threadVar.threadExit;
                        threadExitDdr = threadVar.threadExitDdr;
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
