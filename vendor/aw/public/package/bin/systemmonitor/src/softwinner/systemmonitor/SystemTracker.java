package com.softwinner.systemmonitor;

import android.os.Process;
import android.os.SystemClock;
import android.system.ErrnoException;
import android.system.Os;
import android.system.OsConstants;

import com.android.internal.util.MemInfoReader;

import java.io.File;
import java.io.FileDescriptor;
import java.io.PrintWriter;
import java.io.StringWriter;
import java.text.SimpleDateFormat;
import java.util.ArrayList;
import java.util.Collections;
import java.util.Comparator;
import java.util.Date;
import java.util.List;

public class SystemTracker {
    private static final int[] SYSTEM_CPU_FORMAT = new int[] {
        Process.PROC_SPACE_TERM|Process.PROC_COMBINE,
        Process.PROC_SPACE_TERM|Process.PROC_OUT_LONG,                  // 1: user time
        Process.PROC_SPACE_TERM|Process.PROC_OUT_LONG,                  // 2: nice time
        Process.PROC_SPACE_TERM|Process.PROC_OUT_LONG,                  // 3: sys time
        Process.PROC_SPACE_TERM|Process.PROC_OUT_LONG,                  // 4: idle time
        Process.PROC_SPACE_TERM|Process.PROC_OUT_LONG,                  // 5: iowait time
        Process.PROC_SPACE_TERM|Process.PROC_OUT_LONG,                  // 6: irq time
        Process.PROC_SPACE_TERM|Process.PROC_OUT_LONG                   // 7: softirq time
    };

    private final long[] mSystemCpuData = new long[7];

    // How long a CPU jiffy is in milliseconds.
    private final long mJiffyMillis;

    private long mBaseUserTime;
    private long mBaseSystemTime;
    private long mBaseIoWaitTime;
    private long mBaseIrqTime;
    private long mBaseSoftIrqTime;
    private long mBaseIdleTime;
    private int mRelUserTime;
    private int mRelSystemTime;
    private int mRelIoWaitTime;
    private int mRelIrqTime;
    private int mRelSoftIrqTime;
    private int mRelIdleTime;

    private int mMemoryTotal;
    private int mMemoryFree;

    public SystemTracker() {
        long jiffyHz = Os.sysconf(OsConstants._SC_CLK_TCK);
        mJiffyMillis = 1000/jiffyHz;
        update();
    }

    public void update() {

        final long nowUptime = SystemClock.uptimeMillis();

        final long[] sysCpu = mSystemCpuData;
        if (Process.readProcFile("/proc/stat", SYSTEM_CPU_FORMAT,
                null, sysCpu, null)) {
            // Total user time is user + nice time.
            final long usertime = (sysCpu[0]+sysCpu[1]) * mJiffyMillis;
            // Total system time is simply system time.
            final long systemtime = sysCpu[2] * mJiffyMillis;
            // Total idle time is simply idle time.
            final long idletime = sysCpu[3] * mJiffyMillis;
            // Total irq time is iowait + irq + softirq time.
            final long iowaittime = sysCpu[4] * mJiffyMillis;
            final long irqtime = sysCpu[5] * mJiffyMillis;
            final long softirqtime = sysCpu[6] * mJiffyMillis;

            // This code is trying to avoid issues with idle time going backwards,
            // but currently it gets into situations where it triggers most of the time. :(
            if (true || (usertime >= mBaseUserTime && systemtime >= mBaseSystemTime
                    && iowaittime >= mBaseIoWaitTime && irqtime >= mBaseIrqTime
                    && softirqtime >= mBaseSoftIrqTime && idletime >= mBaseIdleTime)) {
                mRelUserTime = (int)(usertime - mBaseUserTime);
                mRelSystemTime = (int)(systemtime - mBaseSystemTime);
                mRelIoWaitTime = (int)(iowaittime - mBaseIoWaitTime);
                mRelIrqTime = (int)(irqtime - mBaseIrqTime);
                mRelSoftIrqTime = (int)(softirqtime - mBaseSoftIrqTime);
                mRelIdleTime = (int)(idletime - mBaseIdleTime);

                mBaseUserTime = usertime;
                mBaseSystemTime = systemtime;
                mBaseIoWaitTime = iowaittime;
                mBaseIrqTime = irqtime;
                mBaseSoftIrqTime = softirqtime;
                mBaseIdleTime = idletime;

            } else {
                //Log.w(TAG, "/proc/stats has gone backwards; skipping CPU update");
                return;
            }
        }

        MemInfoReader mMemInfoReader = new MemInfoReader();
        mMemInfoReader.readMemInfo();
        mMemoryTotal = (int)(mMemInfoReader.getTotalSizeKb() / 1024);
        mMemoryFree = (int)((mMemInfoReader.getFreeSizeKb() + mMemInfoReader.getCachedSizeKb()) / 1024);

        //Log.i(TAG, "*** TIME TO COLLECT STATS: "
        //        + (SystemClock.uptimeMillis()-nowUptime));

    }

    private String getRatio(long numerator, long denominator) {
        long thousands = (numerator*1000)/denominator;
        long hundreds = thousands/10;
        if (hundreds < 10) {
            long remainder = thousands - (hundreds*10);
            if (remainder != 0) {
                return hundreds + "." + remainder;
            }
        }
        return Long.toString(hundreds);
    }

    public String getCpuUsage() {
        final int useTime = mRelUserTime + mRelSystemTime + mRelIoWaitTime
                + mRelIrqTime + mRelSoftIrqTime;
        final int totalTime = useTime + mRelIdleTime;
        return getRatio(useTime, totalTime);
    }

    public String getMemoryUsage() {
        return getRatio(mMemoryTotal - mMemoryFree, mMemoryTotal);
    }

    public void dump(FileDescriptor fd, PrintWriter pw, String[] args) {
        update();

        final SimpleDateFormat sdf = new SimpleDateFormat("yyyy-MM-dd HH:mm:ss.SSS");
        pw.print(sdf.format(new Date(System.currentTimeMillis())));
        pw.print(" ");
        pw.print(getCpuUsage());
        pw.print(" ");
        pw.print(getMemoryUsage());
        pw.print("\n");
    }
}
