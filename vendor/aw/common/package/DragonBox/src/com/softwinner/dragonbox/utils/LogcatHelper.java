package com.softwinner.dragonbox.utils;

import android.content.Context;
import android.os.Environment;
import android.util.Log;

import java.io.BufferedReader;
import java.io.BufferedWriter;
import java.io.File;
import java.io.FileNotFoundException;
import java.io.FileOutputStream;
import java.io.IOException;
import java.io.InputStreamReader;
import java.io.OutputStreamWriter;

import com.softwinner.dragonbox.ui.DragonBoxMain;

/**
 * TODO<log日志统计保存>
 *
 * @author maizirong
 * @data: 2014-4-8
 * @version: V1.0
 */

public final class LogcatHelper {
    private static final String TAG        = "DragonBox-LogcatHelper";
    private static LogcatHelper instance   = null;
    private static String       pathLogcat;
    private LogDumper           mLogDumper = null;
    private int                 mPId;

    /**
     *
     * 初始化目录
     *
     * */
    public void init(Context context) {
        // 优先保存到SD卡中
        if (Environment.getExternalStorageState().equals(Environment.MEDIA_MOUNTED)) {
            pathLogcat = Environment.getExternalStorageDirectory().getAbsolutePath()
                    + File.separator + "ALLWINNERBOX";
        } else {
            // 如果SD卡不存在，就保存到本应用的目录下
            pathLogcat = context.getFilesDir().getAbsolutePath() + File.separator
                    + "ALLWINNERBOX";
        }
        File file = new File(pathLogcat);
        if (!file.exists()) {
            file.mkdirs();
        }

        Log.v(TAG, "==============Begin Logcat==============");
        Log.v(TAG, "========Logcat File========" + file.getPath());

    }

    public static LogcatHelper getInstance(Context context) {
        if (instance == null) {
            instance = new LogcatHelper(context);
        }
        return instance;
    }

    private LogcatHelper(Context context) {
        init(context);
        mPId = android.os.Process.myPid();
    }

    public void start() {
        if (mLogDumper == null) {
            mLogDumper = new LogDumper(String.valueOf(mPId), pathLogcat);
        }
        mLogDumper.start();
    }

    public void stop() {
        if (mLogDumper != null) {
            mLogDumper.stopLogs();
            mLogDumper = null;
        }
    }

    private class LogDumper extends Thread {

        private Process          logcatProc;
        private BufferedReader   mReader  = null;
        private BufferedWriter   mWriter  = null;
        private boolean          mRunning = true;
        String                   cmds     = null;
        private String           mPID;
        private FileOutputStream out      = null;

        LogDumper(String pid, String dir) {
            mPID = pid;
            try {
                out = new FileOutputStream(new File(dir,"DragonBox-log.txt"),true);
            } catch (FileNotFoundException e) {
                // TODO Auto-generated catch block
                e.printStackTrace();
            }

            /**
             *
             * 日志等级：*:v , *:d , *:w , *:e , *:f , *:s
             *
             * 显示当前mPID程序的 E和W等级的日志.
             *
             * */

            // cmds = "logcat *:e *:w | grep \"(" + mPID + ")\"";
            // cmds = "logcat  | grep \"(" + mPID + ")\"";//打印所有日志信息
            // cmds = "logcat -s way";//打印标签过滤信息
            // cmds = "logcat *:e *:i | grep \"(" + mPID + ")\"";
            cmds = "qw";
        }

        public void stopLogs() {
            mRunning = false;
        }

        @Override
        public void run() {
            try {
                logcatProc = Runtime.getRuntime().exec(cmds);
                String tempCmd = "/system/bin/logcat -v time\n";
                mWriter = new BufferedWriter(new OutputStreamWriter(logcatProc.getOutputStream()));
                mReader = new BufferedReader(new InputStreamReader(logcatProc.getInputStream()));
                mWriter.write(tempCmd);
                mWriter.flush();
                String line = null;
                while (mRunning) {
                    line = mReader.readLine();
                    if (!mRunning) {
                        break;
                    }
                    if ((line == null)||(line.length() == 0)) {
                        continue;
                    }
                    if (out != null) {
                        out.write((line + "\n").getBytes());
                        if(line.contains(DragonBoxMain.LOG_END_FLAG)){
                            Log.e(TAG,"-------detect LOG_END_FLAG");
                            out.flush();
                            out.getFD().sync();//强制将数据写入磁盘
                        }
                    }
                }
            } catch (IOException e) {
                Log.e(TAG,"exception");
                e.printStackTrace();
            } finally {
                if (logcatProc != null) {
                    Log.e(TAG,"logcatProc destroy");
                    logcatProc.destroy();
                    logcatProc = null;
                }
                if (mWriter != null) {
                    try {
                        mWriter.close();
                        mWriter = null;
                    } catch (IOException e) {
                        e.printStackTrace();
                    }
                }
                if (mReader != null) {
                    try {
                        mReader.close();
                        mReader = null;
                    } catch (IOException e) {
                        e.printStackTrace();
                    }
                }
                if (out != null) {
                    try {
                        out.close();
                    } catch (IOException e) {
                        e.printStackTrace();
                    }
                    out = null;
                }

            }

        }

    }
}

