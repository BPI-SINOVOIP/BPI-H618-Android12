/*
 * Copyright (C) 2019 The Android Open Source Project
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
package android.jobscheduler.cts;

import static android.app.ActivityManager.getCapabilitiesSummary;
import static android.app.ActivityManager.procStateToString;
import static android.jobscheduler.cts.jobtestapp.TestJobSchedulerReceiver.EXTRA_REQUEST_JOB_UID_STATE;
import static android.jobscheduler.cts.jobtestapp.TestJobService.ACTION_JOB_STARTED;
import static android.jobscheduler.cts.jobtestapp.TestJobService.ACTION_JOB_STOPPED;
import static android.jobscheduler.cts.jobtestapp.TestJobService.INVALID_ADJ;
import static android.jobscheduler.cts.jobtestapp.TestJobService.JOB_CAPABILITIES_KEY;
import static android.jobscheduler.cts.jobtestapp.TestJobService.JOB_OOM_SCORE_ADJ_KEY;
import static android.jobscheduler.cts.jobtestapp.TestJobService.JOB_PARAMS_EXTRA_KEY;
import static android.jobscheduler.cts.jobtestapp.TestJobService.JOB_PROC_STATE_KEY;
import static android.server.wm.WindowManagerState.STATE_RESUMED;

import static org.junit.Assert.assertEquals;

import android.app.ActivityManager;
import android.app.job.JobParameters;
import android.content.BroadcastReceiver;
import android.content.ComponentName;
import android.content.Context;
import android.content.Intent;
import android.content.IntentFilter;
import android.jobscheduler.cts.jobtestapp.TestActivity;
import android.jobscheduler.cts.jobtestapp.TestJobSchedulerReceiver;
import android.os.SystemClock;
import android.os.UserHandle;
import android.server.wm.WindowManagerStateHelper;
import android.util.Log;

import com.android.compatibility.common.util.CallbackAsserter;
import com.android.compatibility.common.util.SystemUtil;

import java.util.Map;

/**
 * Common functions to interact with the test app.
 */
class TestAppInterface {
    private static final String TAG = TestAppInterface.class.getSimpleName();

    static final String TEST_APP_PACKAGE = "android.jobscheduler.cts.jobtestapp";
    private static final String TEST_APP_ACTIVITY = TEST_APP_PACKAGE + ".TestActivity";
    static final String TEST_APP_RECEIVER = TEST_APP_PACKAGE + ".TestJobSchedulerReceiver";

    private final Context mContext;
    private final int mJobId;

    /* accesses must be synchronized on itself */
    private final TestJobState mTestJobState = new TestJobState();

    TestAppInterface(Context ctx, int jobId) {
        mContext = ctx;
        mJobId = jobId;

        final IntentFilter intentFilter = new IntentFilter();
        intentFilter.addAction(ACTION_JOB_STARTED);
        intentFilter.addAction(ACTION_JOB_STOPPED);
        mContext.registerReceiver(mReceiver, intentFilter);
    }

    void cleanup() {
        final Intent cancelJobsIntent = new Intent(TestJobSchedulerReceiver.ACTION_CANCEL_JOBS);
        cancelJobsIntent.setComponent(new ComponentName(TEST_APP_PACKAGE, TEST_APP_RECEIVER));
        cancelJobsIntent.addFlags(Intent.FLAG_ACTIVITY_NEW_TASK);
        mContext.sendBroadcast(cancelJobsIntent);
        closeActivity();
        mContext.unregisterReceiver(mReceiver);
        mTestJobState.reset();
    }

    void scheduleJob(boolean allowWhileIdle, int requiredNetworkType, boolean asExpeditedJob)
            throws Exception {
        scheduleJob(
                Map.of(
                        TestJobSchedulerReceiver.EXTRA_ALLOW_IN_IDLE, allowWhileIdle,
                        TestJobSchedulerReceiver.EXTRA_AS_EXPEDITED, asExpeditedJob
                ),
                Map.of(
                        TestJobSchedulerReceiver.EXTRA_REQUIRED_NETWORK_TYPE, requiredNetworkType
                ));
    }

    void scheduleJob(Map<String, Boolean> booleanExtras, Map<String, Integer> intExtras)
            throws Exception {
        final Intent scheduleJobIntent = new Intent(TestJobSchedulerReceiver.ACTION_SCHEDULE_JOB);
        scheduleJobIntent.addFlags(Intent.FLAG_RECEIVER_FOREGROUND);
        scheduleJobIntent.putExtra(TestJobSchedulerReceiver.EXTRA_JOB_ID_KEY, mJobId);
        booleanExtras.forEach(scheduleJobIntent::putExtra);
        intExtras.forEach(scheduleJobIntent::putExtra);
        scheduleJobIntent.setComponent(new ComponentName(TEST_APP_PACKAGE, TEST_APP_RECEIVER));

        final CallbackAsserter resultBroadcastAsserter = CallbackAsserter.forBroadcast(
                new IntentFilter(TestJobSchedulerReceiver.ACTION_JOB_SCHEDULE_RESULT));
        mContext.sendBroadcast(scheduleJobIntent);
        resultBroadcastAsserter.assertCalled("Didn't get schedule job result broadcast",
                15 /* 15 seconds */);
    }

    /** Asks (not forces) JobScheduler to run the job if constraints are met. */
    void runSatisfiedJob() throws Exception {
        SystemUtil.runShellCommand("cmd jobscheduler run -s"
                + " -u " + UserHandle.myUserId() + " " + TEST_APP_PACKAGE + " " + mJobId);
    }

    void startAndKeepTestActivity() {
        startAndKeepTestActivity(false);
    }

    void startAndKeepTestActivity(boolean waitForResume) {
        final Intent testActivity = new Intent();
        testActivity.addFlags(Intent.FLAG_ACTIVITY_NEW_TASK);
        ComponentName testComponentName = new ComponentName(TEST_APP_PACKAGE, TEST_APP_ACTIVITY);
        testActivity.setComponent(testComponentName);
        mContext.startActivity(testActivity);
        if (waitForResume) {
            new WindowManagerStateHelper().waitForActivityState(testComponentName, STATE_RESUMED);
        }
    }

    void closeActivity() {
        mContext.sendBroadcast(new Intent(TestActivity.ACTION_FINISH_ACTIVITY));
    }

    private final BroadcastReceiver mReceiver = new BroadcastReceiver() {
        @Override
        public void onReceive(Context context, Intent intent) {
            Log.d(TAG, "Received action " + intent.getAction());
            switch (intent.getAction()) {
                case ACTION_JOB_STARTED:
                case ACTION_JOB_STOPPED:
                    final JobParameters params = intent.getParcelableExtra(JOB_PARAMS_EXTRA_KEY);
                    Log.d(TAG, "JobId: " + params.getJobId());
                    synchronized (mTestJobState) {
                        mTestJobState.running = ACTION_JOB_STARTED.equals(intent.getAction());
                        mTestJobState.jobId = params.getJobId();
                        mTestJobState.params = params;
                        if (intent.getBooleanExtra(EXTRA_REQUEST_JOB_UID_STATE, false)) {
                            mTestJobState.procState = intent.getIntExtra(JOB_PROC_STATE_KEY,
                                    ActivityManager.PROCESS_STATE_NONEXISTENT);
                            mTestJobState.capabilities = intent.getIntExtra(JOB_CAPABILITIES_KEY,
                                    ActivityManager.PROCESS_CAPABILITY_NONE);
                            mTestJobState.oomScoreAdj = intent.getIntExtra(JOB_OOM_SCORE_ADJ_KEY,
                                    INVALID_ADJ);
                        }
                    }
                    break;
            }
        }
    };

    boolean awaitJobStart(long maxWait) throws Exception {
        return waitUntilTrue(maxWait, () -> {
            synchronized (mTestJobState) {
                return (mTestJobState.jobId == mJobId) && mTestJobState.running;
            }
        });
    }

    boolean awaitJobStop(long maxWait) throws Exception {
        return waitUntilTrue(maxWait, () -> {
            synchronized (mTestJobState) {
                return (mTestJobState.jobId == mJobId) && !mTestJobState.running;
            }
        });
    }

    void assertJobUidState(int procState, int capabilities, int oomScoreAdj) {
        synchronized (mTestJobState) {
            assertEquals("procState expected=" + procStateToString(procState)
                    + ",actual=" + procStateToString(mTestJobState.procState),
                    procState, mTestJobState.procState);
            assertEquals("capabilities expected=" + getCapabilitiesSummary(capabilities)
                    + ",actual=" + getCapabilitiesSummary(mTestJobState.capabilities),
                    capabilities, mTestJobState.capabilities);
            assertEquals("Unexpected oomScoreAdj", oomScoreAdj, mTestJobState.oomScoreAdj);
        }
    }

    private boolean waitUntilTrue(long maxWait, Condition condition) throws Exception {
        final long deadLine = SystemClock.uptimeMillis() + maxWait;
        do {
            Thread.sleep(500);
        } while (!condition.isTrue() && SystemClock.uptimeMillis() < deadLine);
        return condition.isTrue();
    }

    JobParameters getLastParams() {
        synchronized (mTestJobState) {
            return mTestJobState.params;
        }
    }

    private static final class TestJobState {
        int jobId;
        boolean running;
        int procState;
        int capabilities;
        int oomScoreAdj;
        JobParameters params;

        TestJobState() {
            initState();
        }

        private void reset() {
            initState();
        }

        private void initState() {
            running = false;
            procState = ActivityManager.PROCESS_STATE_NONEXISTENT;
            capabilities = ActivityManager.PROCESS_CAPABILITY_NONE;
            oomScoreAdj = INVALID_ADJ;
        }
    }

    private interface Condition {
        boolean isTrue() throws Exception;
    }
}
