package com.softwinner.systemmonitor;

import android.app.ActivityManager;
import android.app.IActivityController;
import android.app.IActivityManager;
import android.content.Intent;
import android.os.RemoteException;
import android.util.Log;

import java.io.FileWriter;
import java.io.IOException;
import java.io.PrintWriter;
import java.text.SimpleDateFormat;
import java.util.Date;

public class SystemMonitor {
    /* package */ static final String TAG = "SystemMonitor";

    private static final int FLAG_ACTIVITY_STARTING = 1 << 5;
    private static final int FLAG_ACTIVITY_RESUMING = 1 << 4;
    private static final int FLAG_APP_CRASHED = 1 << 3;
    private static final int FLAG_APP_EARLY_NOT_RESPONDING = 1 << 2;
    private static final int FLAG_APP_NOT_RESPONDING = 1 << 1;
    private static final int FLAG_SYSTEM_NOT_RESPONDING = 1 << 0;
    private static final int FLAG_ANR = FLAG_APP_EARLY_NOT_RESPONDING | FLAG_APP_NOT_RESPONDING | FLAG_SYSTEM_NOT_RESPONDING;

    private static final String TITLE = "Timestamp                  CPU%\tMEM%\tANR";
    private static final SimpleDateFormat mSdf = new SimpleDateFormat("yyyy-MM-dd HH:mm:ss.SSS");

    private MyActivityController mMyActivityController;
    private SystemTracker mSystemTracker;
    private int mFlags;

    public static void main(String[] args) {
        new SystemMonitor().exec(args);
    }

    public SystemMonitor() {
        IActivityManager am = ActivityManager.getService();
        if (am == null) {
            Log.e(TAG, "no activity manager");
            return;
        }
        try {
            mMyActivityController = new MyActivityController();
            am.setActivityController(mMyActivityController, false);
        } catch (RemoteException e) {
        }
        mSystemTracker = new SystemTracker();
    }

    public void exec(String[] args) {
        int interval = 1000;
        String file = null;
        for (int i = 0; i < args.length; i++) {
            String arg = args[i];
            switch (arg) {
                case "-h":
                    usage();
                    return;
                case "-t":
                    if (++i < args.length) {
                        try {
                            interval = Integer.parseInt(args[i]);
                        } catch (NumberFormatException e) {
                        }
                    } else {
                        System.exit(-1);
                    }
                    break;
                case "-f":
                    if (++i < args.length) {
                        file = args[i];
                    } else {
                        System.exit(-1);
                    }
                    break;
            }
        }
        PrintWriter out;
        if (file != null) {
            try {
                out = new PrintWriter(new FileWriter(file, false));
            } catch (IOException e) {
                out = new PrintWriter(System.out, true);
                out.println(TITLE);
            }
        } else {
            out = new PrintWriter(System.out, true);
            out.println(TITLE);
        }

        while (true) {
            try {
                Thread.sleep(interval);
            } catch (InterruptedException e) {
            }
            mSystemTracker.update();
            out.println(mSdf.format(new Date(System.currentTimeMillis()))
                + "    "
                + mSystemTracker.getCpuUsage()
                + "\t"
                + mSystemTracker.getMemoryUsage()
                + "\t"
                + (mFlags & FLAG_ANR)
            );
            out.flush();
            mFlags = 0;
        }
    }

    private void usage() {
        System.err.println("systemmonitor:");
        System.err.println("    -t interval");
        System.err.println("    -f logfile");
    }

    private class MyActivityController extends IActivityController.Stub {
        @Override
        public boolean activityStarting(Intent intent, String pkg) throws RemoteException {
            Log.d(TAG, "activityStarting: " + intent + ", " + pkg);
            mFlags |= FLAG_ACTIVITY_STARTING;
            return true;
        }

        @Override
        public boolean activityResuming(String pkg) throws RemoteException {
            Log.d(TAG, "activityResuming: " + pkg);
            mFlags |= FLAG_ACTIVITY_RESUMING;
            return true;
        }

        @Override
        public boolean appCrashed(String processName, int pid, String shortMsg, String longMsg,
                long timeMillis, String stackTrace) throws RemoteException {
            Log.d(TAG, "appCrashed: " + processName + ", " + pid + ", " + shortMsg + ", " + longMsg + ", " + timeMillis + ", " + stackTrace);
            mFlags |= FLAG_APP_CRASHED;
            return true;
        }

        @Override
        public int appEarlyNotResponding(String processName, int pid, String annotation)
                throws RemoteException {
            Log.d(TAG, "appEarlyNotResponding: " + processName + ", " + pid + ", " + annotation);
            mFlags |= FLAG_APP_EARLY_NOT_RESPONDING;
            return 0;
        }

        @Override
        public int appNotResponding(String processName, int pid, String processStats)
                throws RemoteException {
            Log.d(TAG, "appNotResponding: " + processName + ", " + pid + ", " + processStats);
            mFlags |= FLAG_APP_NOT_RESPONDING;
            return 0;
        }

        @Override
        public int systemNotResponding(String message)
                throws RemoteException {
            Log.d(TAG, "systemNotResponding: " + message);
            mFlags |= FLAG_SYSTEM_NOT_RESPONDING;
            return 0;
        }
    }
}
