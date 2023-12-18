/*
 * Copyright (C) 2020 The Android Open Source Project
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
package android.device.collectors;

import static com.android.internal.jank.InteractionJankMonitor.PROP_NOTIFY_CUJ_EVENT;

import android.content.BroadcastReceiver;
import android.content.Context;
import android.content.Intent;
import android.content.IntentFilter;
import android.os.Bundle;

import com.android.helpers.MetricUtility;
import com.android.helpers.UiInteractionFrameInfoHelper;
import com.android.internal.annotations.GuardedBy;
import com.android.internal.jank.InteractionJankMonitor;

import org.junit.runner.Description;
import org.junit.runner.Result;

import java.util.HashSet;
import java.util.Set;

/**
 * A listener that captures jank for various system interactions.
 *
 * <p>Do NOT throw exception anywhere in this class. We don't want to halt the test when metrics
 * collection fails.
 */
public class UiInteractionFrameInfoListener extends BaseCollectionListener<StringBuilder> {
    private static final String TAG = UiInteractionFrameInfoListener.class.getSimpleName();
    private static final long POLLING_INTERVAL_MS = 100;
    private static final long POLLING_MAX_TIMES = 100;
    private static final long DELAY_TOGGLE_PANEL_MS = 100;
    private static final String CMD_ENABLE_NOTIFY =
            String.format("setprop %s %d", PROP_NOTIFY_CUJ_EVENT, 1);
    private static final String CMD_DISABLE_NOTIFY =
            String.format("setprop %s %d", PROP_NOTIFY_CUJ_EVENT, 0);
    private static final String CMD_TOGGLE_PANEL = "su shell service call statusbar 3";

    private final MetricsLoggedReceiver mReceiver = new MetricsLoggedReceiver();
    private final Object mLock = new Object();

    @GuardedBy("mLock")
    private final Set<String> mExpectedCujSet = new HashSet<>();

    @GuardedBy("mLock")
    private final Set<String> mLoggedCujSet = new HashSet<>();

    @GuardedBy("mLock")
    private TimestampRecord mCurrentTestTimestamp;

    @GuardedBy("mLock")
    private boolean mMetricsReady;

    public UiInteractionFrameInfoListener() {
        createHelperInstance(new UiInteractionFrameInfoHelper());
    }

    @Override
    public void onTestRunStart(DataRecord runData, Description description) {
        getInstrumentation().getUiAutomation().executeShellCommand(CMD_ENABLE_NOTIFY);
        super.onTestRunStart(runData, description);
    }

    @Override
    public void onTestRunEnd(DataRecord runData, Result result) {
        super.onTestRunEnd(runData, result);
        getInstrumentation().getUiAutomation().executeShellCommand(CMD_DISABLE_NOTIFY);
    }

    @Override
    protected boolean onTestStartAlternative(DataRecord data) {
        synchronized (mLock) {
            mMetricsReady = false;
            mCurrentTestTimestamp = new TimestampRecord();
            mCurrentTestTimestamp.begin(System.nanoTime());
        }
        IntentFilter filter = new IntentFilter();
        filter.addAction(InteractionJankMonitor.ACTION_SESSION_BEGIN);
        filter.addAction(InteractionJankMonitor.ACTION_METRICS_LOGGED);
        filter.addAction(InteractionJankMonitor.ACTION_SESSION_CANCEL);
        getInstrumentation().getContext().registerReceiver(mReceiver, filter);
        mHelper.startCollecting();
        return true;
    }

    @Override
    protected boolean onTestEndAlternative(DataRecord data) {
        try {
            synchronized (mLock) {
                mCurrentTestTimestamp.end(System.nanoTime());
                if (mExpectedCujSet.size() > 0) {
                    for (int i = 0; i < POLLING_MAX_TIMES && !mMetricsReady; i++) {
                        flushSurfaceFlingerCallback();
                        mLock.wait(POLLING_INTERVAL_MS);
                    }
                    if (mMetricsReady) {
                        processMetrics(data);
                    } else {
                        throw new IllegalStateException("metrics not ready: ex="
                                + mExpectedCujSet + ", log=" + mLoggedCujSet);
                    }
                } else {
                    throw new IllegalStateException("No expected CUJ!");
                }
            }
        } catch (InterruptedException e) {
        } finally {
            synchronized (mLock) {
                mMetricsReady = false;
                mExpectedCujSet.clear();
                mLoggedCujSet.clear();
            }
            getInstrumentation().getContext().unregisterReceiver(mReceiver);
            mHelper.stopCollecting();
        }
        return true;
    }

    private void flushSurfaceFlingerCallback() throws InterruptedException {
        try {
            getInstrumentation().getUiAutomation().executeShellCommand(CMD_TOGGLE_PANEL);
            Thread.sleep(DELAY_TOGGLE_PANEL_MS);
        } finally {
            getInstrumentation().getUiAutomation().executeShellCommand(CMD_TOGGLE_PANEL);
        }
    }

    private void reduceMetrics(DataRecord data, String key, String value) {
        if (data == null || key.isEmpty() || value.isEmpty()) return;

        double result = 0;
        String[] tokens = value.split(MetricUtility.METRIC_SEPARATOR);
        for (String token : tokens) {
            if (key.endsWith(UiInteractionFrameInfoHelper.SUFFIX_MAX_FRAME_MS)) {
                result = Double.max(result, Double.parseDouble(token));
            } else {
                result += Double.parseDouble(token);
                result = UiInteractionFrameInfoHelper.makeLogFriendly(Math.floor(result));
            }
        }
        data.addStringMetric(key, Double.toString(result));
    }

    private void processMetrics(DataRecord data) throws InterruptedException {
        final Set<String> foundCujSet = new HashSet<>();
        DataRecord metricsData = new DataRecord();
        while (!(foundCujSet.size() == mLoggedCujSet.size())) {
            super.collectMetrics(metricsData);
            if (!metricsData.hasMetrics()) {
                mLock.wait(POLLING_INTERVAL_MS);
                continue;
            }
            Bundle bundle = metricsData.createBundleFromMetrics();
            for (String key : bundle.keySet()) {
                for (String cujName : mLoggedCujSet) {
                    if (key.startsWith(cujName)) {
                        reduceMetrics(data, key, bundle.getString(key));
                        foundCujSet.add(cujName);
                        break;
                    }
                }
            }
            metricsData.clear();
        }
    }

    private static class TimestampRecord {
        private long begin = Long.MAX_VALUE;
        private long end = Long.MAX_VALUE;

        void begin(long timestamp) {
            begin = timestamp;
        }

        void end(long timestamp) {
            end = timestamp;
        }

        boolean isInRange(long timestamp) {
            return begin <= timestamp && timestamp <= end;
        }
    }

    private class MetricsLoggedReceiver extends BroadcastReceiver {
        @Override
        public void onReceive(Context context, Intent intent) {
            String action = intent.getAction();
            String name = intent.getStringExtra(InteractionJankMonitor.BUNDLE_KEY_CUJ_NAME);
            long timestamp = intent.getLongExtra(InteractionJankMonitor.BUNDLE_KEY_TIMESTAMP, -1L);

            if (InteractionJankMonitor.ACTION_SESSION_BEGIN.equals(action)) {
                handleSessionBegin(name, timestamp);
            } else if (InteractionJankMonitor.ACTION_SESSION_CANCEL.equals(action)) {
                handleSessionCancelled(name);
            } else if (InteractionJankMonitor.ACTION_METRICS_LOGGED.equals(action)) {
                handleMetricsLogged(name);
            }
        }

        private void handleSessionBegin(String name, long timestamp) {
            synchronized (mLock) {
                if (mCurrentTestTimestamp.isInRange(timestamp)) {
                    mExpectedCujSet.add(name);
                }
            }
        }

        private void handleSessionCancelled(String name) {
            synchronized (mLock) {
                boolean valid = mExpectedCujSet.remove(name);
                if (valid && (mLoggedCujSet.size() == mExpectedCujSet.size())) {
                    mMetricsReady = true;
                    mLock.notifyAll();
                }
            }
        }

        private void handleMetricsLogged(String name) {
            synchronized (mLock) {
                if (mExpectedCujSet.contains(name)) {
                    mLoggedCujSet.add(
                            MetricUtility.constructKey(
                                    UiInteractionFrameInfoHelper.KEY_PREFIX_CUJ, name));
                    if (mLoggedCujSet.size() == mExpectedCujSet.size()) {
                        mMetricsReady = true;
                        mLock.notifyAll();
                    }
                }
            }
        }
    }
}
