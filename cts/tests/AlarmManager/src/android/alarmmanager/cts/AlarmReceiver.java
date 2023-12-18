/*
 * Copyright (C) 2021 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

package android.alarmmanager.cts;

import android.content.BroadcastReceiver;
import android.content.Context;
import android.content.Intent;
import android.os.SystemClock;
import android.util.Log;
import android.util.LongArray;

import androidx.annotation.GuardedBy;

import java.util.Arrays;

public class AlarmReceiver extends BroadcastReceiver {
    private static final String TAG = AlarmReceiver.class.getSimpleName();
    public static final String ALARM_ACTION = "android.alarmmanager.cts.ALARM";
    public static final String EXTRA_ALARM_ID = "android.alarmmanager.cts.extra.ALARM_ID";
    public static final String EXTRA_QUOTAED = "android.alarmmanager.cts.extra.QUOTAED";

    static Object sWaitLock = new Object();

    @GuardedBy("sWaitLock")
    private static int sLastAlarmId;
    /** Process global history of all alarms received -- useful in quota calculations */
    private static LongArray sHistory = new LongArray();

    static synchronized long getNthLastAlarmTime(int n) {
        if (n <= 0 || n > sHistory.size()) {
            return 0;
        }
        return sHistory.get(sHistory.size() - n);
    }

    private static synchronized void recordAlarmTime(long timeOfReceipt) {
        if (sHistory.size() == 0 || sHistory.get(sHistory.size() - 1) < timeOfReceipt) {
            sHistory.add(timeOfReceipt);
        }
    }

    static boolean waitForAlarm(int alarmId, long timeOut) throws InterruptedException {
        final long deadline = SystemClock.elapsedRealtime() + timeOut;
        synchronized (sWaitLock) {
            while (sLastAlarmId != alarmId && SystemClock.elapsedRealtime() < deadline) {
                sWaitLock.wait(timeOut);
            }
            return sLastAlarmId == alarmId;
        }
    }

    /**
     * Used to dump debugging information when the test fails.
     */
    static void dumpState() {
        synchronized (sWaitLock) {
            Log.i(TAG, "Last id: " + sLastAlarmId);
        }
        synchronized (AlarmReceiver.class) {
            if (sHistory.size() > 0) {
                Log.i(TAG, "History of quotaed alarms: " + Arrays.toString(sHistory.toArray()));
            }
        }
    }

    @Override
    public void onReceive(Context context, Intent intent) {
        if (ALARM_ACTION.equals(intent.getAction())) {
            final int id = intent.getIntExtra(EXTRA_ALARM_ID, -1);
            final boolean quotaed = intent.getBooleanExtra(EXTRA_QUOTAED, false);
            if (quotaed) {
                recordAlarmTime(SystemClock.elapsedRealtime());
            }
            synchronized (sWaitLock) {
                sLastAlarmId = id;
                sWaitLock.notifyAll();
            }
        }
    }
}
