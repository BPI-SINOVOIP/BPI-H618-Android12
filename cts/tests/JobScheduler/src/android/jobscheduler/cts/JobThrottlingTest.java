/*
 * Copyright (C) 2017 The Android Open Source Project
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
 * limitations under the License
 */

package android.jobscheduler.cts;

import static android.app.job.JobInfo.NETWORK_TYPE_ANY;
import static android.app.job.JobInfo.NETWORK_TYPE_NONE;
import static android.jobscheduler.cts.ConnectivityConstraintTest.ensureSavedWifiNetwork;
import static android.jobscheduler.cts.ConnectivityConstraintTest.isWiFiConnected;
import static android.jobscheduler.cts.ConnectivityConstraintTest.setWifiState;
import static android.jobscheduler.cts.TestAppInterface.TEST_APP_PACKAGE;
import static android.os.PowerManager.ACTION_DEVICE_IDLE_MODE_CHANGED;
import static android.os.PowerManager.ACTION_LIGHT_DEVICE_IDLE_MODE_CHANGED;

import static com.android.compatibility.common.util.TestUtils.waitUntil;

import static junit.framework.Assert.fail;

import static org.junit.Assert.assertEquals;
import static org.junit.Assert.assertFalse;
import static org.junit.Assert.assertTrue;
import static org.junit.Assume.assumeFalse;
import static org.junit.Assume.assumeTrue;

import android.Manifest;
import android.app.AppOpsManager;
import android.app.job.JobInfo;
import android.app.job.JobParameters;
import android.content.BroadcastReceiver;
import android.content.Context;
import android.content.Intent;
import android.content.IntentFilter;
import android.content.pm.PackageManager;
import android.jobscheduler.cts.jobtestapp.TestJobSchedulerReceiver;
import android.net.ConnectivityManager;
import android.net.wifi.WifiManager;
import android.os.PowerManager;
import android.os.SystemClock;
import android.os.Temperature;
import android.os.UserHandle;
import android.platform.test.annotations.RequiresDevice;
import android.provider.DeviceConfig;
import android.provider.Settings;
import android.support.test.uiautomator.UiDevice;
import android.util.Log;

import androidx.test.InstrumentationRegistry;
import androidx.test.filters.LargeTest;
import androidx.test.runner.AndroidJUnit4;

import com.android.compatibility.common.util.AppOpsUtils;
import com.android.compatibility.common.util.AppStandbyUtils;
import com.android.compatibility.common.util.BatteryUtils;
import com.android.compatibility.common.util.CallbackAsserter;
import com.android.compatibility.common.util.DeviceConfigStateHelper;
import com.android.compatibility.common.util.SystemUtil;
import com.android.compatibility.common.util.ThermalUtils;

import junit.framework.AssertionFailedError;

import org.junit.After;
import org.junit.Before;
import org.junit.Test;
import org.junit.runner.RunWith;

import java.io.IOException;
import java.util.concurrent.atomic.AtomicReference;
import java.util.regex.Matcher;
import java.util.regex.Pattern;

/**
 * Tests related to job throttling -- device idle, app standby and battery saver.
 */
@RunWith(AndroidJUnit4.class)
@LargeTest
public class JobThrottlingTest {
    private static final String TAG = JobThrottlingTest.class.getSimpleName();
    private static final long BACKGROUND_JOBS_EXPECTED_DELAY = 3_000;
    private static final long POLL_INTERVAL = 500;
    private static final long DEFAULT_WAIT_TIMEOUT = 2000;
    private static final long SHELL_TIMEOUT = 3_000;
    // TODO: mark Settings.System.SCREEN_OFF_TIMEOUT as @TestApi
    private static final String SCREEN_OFF_TIMEOUT = "screen_off_timeout";

    enum Bucket {
        ACTIVE,
        WORKING_SET,
        FREQUENT,
        RARE,
        RESTRICTED,
        NEVER
    }

    private Context mContext;
    private UiDevice mUiDevice;
    private PowerManager mPowerManager;
    private int mTestJobId;
    private int mTestPackageUid;
    private boolean mDeviceInDoze;
    private boolean mDeviceIdleEnabled;
    private boolean mAppStandbyEnabled;
    private WifiManager mWifiManager;
    private ConnectivityManager mCm;
    /** Whether the device running these tests supports WiFi. */
    private boolean mHasWifi;
    /** Track whether WiFi was enabled in case we turn it off. */
    private boolean mInitialWiFiState;
    private boolean mInitialAirplaneModeState;
    private String mInitialDisplayTimeout;
    private String mInitialRestrictedBucketEnabled;
    private boolean mAutomotiveDevice;
    private boolean mLeanbackOnly;

    private TestAppInterface mTestAppInterface;
    private DeviceConfigStateHelper mDeviceConfigStateHelper;

    private final BroadcastReceiver mReceiver = new BroadcastReceiver() {
        @Override
        public void onReceive(Context context, Intent intent) {
            Log.d(TAG, "Received action " + intent.getAction());
            switch (intent.getAction()) {
                case ACTION_DEVICE_IDLE_MODE_CHANGED:
                case ACTION_LIGHT_DEVICE_IDLE_MODE_CHANGED:
                    synchronized (JobThrottlingTest.this) {
                        mDeviceInDoze = mPowerManager.isDeviceIdleMode();
                        Log.d(TAG, "mDeviceInDoze: " + mDeviceInDoze);
                    }
                    break;
            }
        }
    };

    private static boolean isDeviceIdleEnabled(UiDevice uiDevice) throws Exception {
        final String output = uiDevice.executeShellCommand("cmd deviceidle enabled deep").trim();
        return Integer.parseInt(output) != 0;
    }

    @Before
    public void setUp() throws Exception {
        mContext = InstrumentationRegistry.getTargetContext();
        mUiDevice = UiDevice.getInstance(InstrumentationRegistry.getInstrumentation());
        mPowerManager = mContext.getSystemService(PowerManager.class);
        mDeviceInDoze = mPowerManager.isDeviceIdleMode();
        mTestPackageUid = mContext.getPackageManager().getPackageUid(TEST_APP_PACKAGE, 0);
        mTestJobId = (int) (SystemClock.uptimeMillis() / 1000);
        mTestAppInterface = new TestAppInterface(mContext, mTestJobId);
        final IntentFilter intentFilter = new IntentFilter();
        intentFilter.addAction(ACTION_DEVICE_IDLE_MODE_CHANGED);
        intentFilter.addAction(ACTION_LIGHT_DEVICE_IDLE_MODE_CHANGED);
        mContext.registerReceiver(mReceiver, intentFilter);
        assertFalse("Test package already in temp whitelist", isTestAppTempWhitelisted());
        makeTestPackageIdle();
        mDeviceIdleEnabled = isDeviceIdleEnabled(mUiDevice);
        mAppStandbyEnabled = AppStandbyUtils.isAppStandbyEnabled();
        if (mAppStandbyEnabled) {
            setTestPackageStandbyBucket(Bucket.ACTIVE);
        } else {
            Log.w(TAG, "App standby not enabled on test device");
        }
        mWifiManager = mContext.getSystemService(WifiManager.class);
        mCm = mContext.getSystemService(ConnectivityManager.class);
        mHasWifi = mContext.getPackageManager().hasSystemFeature(PackageManager.FEATURE_WIFI);
        mInitialWiFiState = mWifiManager.isWifiEnabled();
        mInitialAirplaneModeState = isAirplaneModeOn();
        mInitialRestrictedBucketEnabled = Settings.Global.getString(mContext.getContentResolver(),
                Settings.Global.ENABLE_RESTRICTED_BUCKET);
        // Make sure test jobs can run regardless of bucket.
        mDeviceConfigStateHelper =
                new DeviceConfigStateHelper(DeviceConfig.NAMESPACE_JOB_SCHEDULER);
        mDeviceConfigStateHelper.set(
                new DeviceConfig.Properties.Builder(DeviceConfig.NAMESPACE_JOB_SCHEDULER)
                        .setInt("min_ready_non_active_jobs_count", 0).build());
        // Make sure the screen doesn't turn off when the test turns it on.
        mInitialDisplayTimeout =
                Settings.System.getString(mContext.getContentResolver(), SCREEN_OFF_TIMEOUT);
        Settings.System.putString(mContext.getContentResolver(), SCREEN_OFF_TIMEOUT, "300000");

        // In automotive device, always-on screen and endless battery charging are assumed.
        mAutomotiveDevice =
                mContext.getPackageManager().hasSystemFeature(PackageManager.FEATURE_AUTOMOTIVE);
        // In leanback devices, it is assumed that there is no battery.
        mLeanbackOnly =
                mContext.getPackageManager().hasSystemFeature(PackageManager.FEATURE_LEANBACK_ONLY);
        if (mAutomotiveDevice || mLeanbackOnly) {
            setScreenState(true);
            // TODO(b/159176758): make sure that initial power supply is on.
            BatteryUtils.runDumpsysBatterySetPluggedIn(true);
        }

        // Kill as many things in the background as possible so we avoid LMK interfering with the
        // test.
        mUiDevice.executeShellCommand("am kill-all");
    }

    @Test
    public void testAllowWhileIdleJobInTempwhitelist() throws Exception {
        assumeTrue("device idle not enabled", mDeviceIdleEnabled);

        toggleDozeState(true);
        Thread.sleep(DEFAULT_WAIT_TIMEOUT);
        sendScheduleJobBroadcast(true);
        assertFalse("Job started without being tempwhitelisted",
                mTestAppInterface.awaitJobStart(5_000));
        tempWhitelistTestApp(5_000);
        assertTrue("Job with allow_while_idle flag did not start when the app was tempwhitelisted",
                mTestAppInterface.awaitJobStart(5_000));
    }

    @Test
    public void testForegroundJobsStartImmediately() throws Exception {
        assumeTrue("device idle not enabled", mDeviceIdleEnabled);

        sendScheduleJobBroadcast(false);
        runJob();
        assertTrue("Job did not start after scheduling",
                mTestAppInterface.awaitJobStart(DEFAULT_WAIT_TIMEOUT));
        toggleDozeState(true);
        assertTrue("Job did not stop on entering doze",
                mTestAppInterface.awaitJobStop(DEFAULT_WAIT_TIMEOUT));
        Thread.sleep(TestJobSchedulerReceiver.JOB_INITIAL_BACKOFF);
        // The adb command will force idle even with the screen on, so we need to turn Doze off
        // explicitly.
        toggleDozeState(false);
        // Turn the screen on to ensure the test app ends up in TOP.
        setScreenState(true);
        mTestAppInterface.startAndKeepTestActivity();
        assertTrue("Job for foreground app did not start immediately when device exited doze",
                mTestAppInterface.awaitJobStart(DEFAULT_WAIT_TIMEOUT));
    }

    @Test
    public void testBackgroundJobsDelayed() throws Exception {
        assumeTrue("device idle not enabled", mDeviceIdleEnabled);

        sendScheduleJobBroadcast(false);
        runJob();
        assertTrue("Job did not start after scheduling",
                mTestAppInterface.awaitJobStart(DEFAULT_WAIT_TIMEOUT));
        toggleDozeState(true);
        assertTrue("Job did not stop on entering doze",
                mTestAppInterface.awaitJobStop(DEFAULT_WAIT_TIMEOUT));
        Thread.sleep(TestJobSchedulerReceiver.JOB_INITIAL_BACKOFF);
        toggleDozeState(false);
        assertFalse("Job for background app started immediately when device exited doze",
                mTestAppInterface.awaitJobStart(DEFAULT_WAIT_TIMEOUT));
        Thread.sleep(BACKGROUND_JOBS_EXPECTED_DELAY - DEFAULT_WAIT_TIMEOUT);
        assertTrue("Job for background app did not start after the expected delay of "
                        + BACKGROUND_JOBS_EXPECTED_DELAY + "ms",
                mTestAppInterface.awaitJobStart(DEFAULT_WAIT_TIMEOUT));
    }

    @Test
    public void testJobStoppedWhenRestricted() throws Exception {
        sendScheduleJobBroadcast(false);
        runJob();
        assertTrue("Job did not start after scheduling",
                mTestAppInterface.awaitJobStart(DEFAULT_WAIT_TIMEOUT));
        setTestPackageRestricted(true);
        assertTrue("Job did not stop after test app was restricted",
                mTestAppInterface.awaitJobStop(DEFAULT_WAIT_TIMEOUT));
        assertEquals(JobParameters.STOP_REASON_BACKGROUND_RESTRICTION,
                mTestAppInterface.getLastParams().getStopReason());
    }

    @Test
    public void testRestrictedJobStartedWhenUnrestricted() throws Exception {
        setTestPackageRestricted(true);
        sendScheduleJobBroadcast(false);
        assertFalse("Job started for restricted app",
                mTestAppInterface.awaitJobStart(DEFAULT_WAIT_TIMEOUT));
        setTestPackageRestricted(false);
        assertTrue("Job did not start when app was unrestricted",
                mTestAppInterface.awaitJobStart(DEFAULT_WAIT_TIMEOUT));
    }

    @Test
    public void testRestrictedJobAllowedWhenUidActive() throws Exception {
        setTestPackageRestricted(true);
        sendScheduleJobBroadcast(false);
        assertFalse("Job started for restricted app",
                mTestAppInterface.awaitJobStart(DEFAULT_WAIT_TIMEOUT));
        // Turn the screen on to ensure the app gets into the TOP state.
        setScreenState(true);
        mTestAppInterface.startAndKeepTestActivity(true);
        assertTrue("Job did not start when app had an activity",
                mTestAppInterface.awaitJobStart(DEFAULT_WAIT_TIMEOUT));

        mTestAppInterface.closeActivity();
        // Don't put full minute as the timeout to give some leeway with test timing/processing.
        assertFalse("Job stopped within grace period after activity closed",
                mTestAppInterface.awaitJobStop(55_000L));
        assertTrue("Job did not stop after grace period ended",
                mTestAppInterface.awaitJobStop(15_000L));
        assertEquals(JobParameters.STOP_REASON_BACKGROUND_RESTRICTION,
                mTestAppInterface.getLastParams().getStopReason());
    }

    @Test
    public void testEJStoppedWhenRestricted() throws Exception {
        mTestAppInterface.scheduleJob(false, JobInfo.NETWORK_TYPE_NONE, true);
        runJob();
        assertTrue("Job did not start after scheduling",
                mTestAppInterface.awaitJobStart(DEFAULT_WAIT_TIMEOUT));
        setTestPackageRestricted(true);
        assertTrue("Job did not stop after test app was restricted",
                mTestAppInterface.awaitJobStop(DEFAULT_WAIT_TIMEOUT));
        assertEquals(JobParameters.STOP_REASON_BACKGROUND_RESTRICTION,
                mTestAppInterface.getLastParams().getStopReason());
    }

    @Test
    public void testRestrictedEJStartedWhenUnrestricted() throws Exception {
        setTestPackageRestricted(true);
        mTestAppInterface.scheduleJob(false, JobInfo.NETWORK_TYPE_NONE, true);
        assertFalse("Job started for restricted app",
                mTestAppInterface.awaitJobStart(DEFAULT_WAIT_TIMEOUT));
        setTestPackageRestricted(false);
        assertTrue("Job did not start when app was unrestricted",
                mTestAppInterface.awaitJobStart(DEFAULT_WAIT_TIMEOUT));
    }

    @Test
    public void testRestrictedEJAllowedWhenUidActive() throws Exception {
        setTestPackageRestricted(true);
        mTestAppInterface.scheduleJob(false, JobInfo.NETWORK_TYPE_NONE, true);
        assertFalse("Job started for restricted app",
                mTestAppInterface.awaitJobStart(DEFAULT_WAIT_TIMEOUT));
        // Turn the screen on to ensure the app gets into the TOP state.
        setScreenState(true);
        mTestAppInterface.startAndKeepTestActivity(true);
        assertTrue("Job did not start when app had an activity",
                mTestAppInterface.awaitJobStart(DEFAULT_WAIT_TIMEOUT));

        mTestAppInterface.closeActivity();
        // Don't put full minute as the timeout to give some leeway with test timing/processing.
        assertFalse("Job stopped within grace period after activity closed",
                mTestAppInterface.awaitJobStop(55_000L));
        assertTrue("Job did not stop after grace period ended",
                mTestAppInterface.awaitJobStop(15_000L));
        assertEquals(JobParameters.STOP_REASON_BACKGROUND_RESTRICTION,
                mTestAppInterface.getLastParams().getStopReason());
    }

    @RequiresDevice // Emulators don't always have access to wifi/network
    @Test
    public void testBackgroundConnectivityJobsThrottled() throws Exception {
        if (!mHasWifi) {
            Log.d(TAG, "Skipping test that requires the device be WiFi enabled.");
            return;
        }
        ensureSavedWifiNetwork(mWifiManager);
        setAirplaneMode(false);
        setWifiState(true, mCm, mWifiManager);
        assumeTrue("device idle not enabled", mDeviceIdleEnabled);
        mTestAppInterface.scheduleJob(false, NETWORK_TYPE_ANY, false);
        runJob();
        assertTrue("Job did not start after scheduling",
                mTestAppInterface.awaitJobStart(DEFAULT_WAIT_TIMEOUT));
        ThermalUtils.overrideThermalStatus(Temperature.THROTTLING_CRITICAL);
        assertTrue("Job did not stop on thermal throttling",
                mTestAppInterface.awaitJobStop(DEFAULT_WAIT_TIMEOUT));
        Thread.sleep(TestJobSchedulerReceiver.JOB_INITIAL_BACKOFF);
        ThermalUtils.overrideThermalNotThrottling();
        runJob();
        assertTrue("Job did not start back from throttling",
                mTestAppInterface.awaitJobStart(DEFAULT_WAIT_TIMEOUT));
    }

    /** Tests that apps in the RESTRICTED bucket still get their one parole session per day. */
    @Test
    public void testJobsInRestrictedBucket_ParoleSession() throws Exception {
        assumeTrue("app standby not enabled", mAppStandbyEnabled);
        assumeFalse("not testable in automotive device", mAutomotiveDevice);
        assumeFalse("not testable in leanback device", mLeanbackOnly);

        setRestrictedBucketEnabled(true);

        // Disable coalescing
        mDeviceConfigStateHelper.set("qc_timing_session_coalescing_duration_ms", "0");

        setScreenState(true);

        BatteryUtils.runDumpsysBatteryUnplug();
        setTestPackageStandbyBucket(Bucket.RESTRICTED);
        Thread.sleep(DEFAULT_WAIT_TIMEOUT);
        sendScheduleJobBroadcast(false);
        runJob();
        assertTrue("Parole job didn't start in RESTRICTED bucket",
                mTestAppInterface.awaitJobStart(3_000));

        sendScheduleJobBroadcast(false);
        assertFalse("New job started in RESTRICTED bucket", mTestAppInterface.awaitJobStart(3_000));
    }

    /**
     * Tests that apps in the RESTRICTED bucket have their parole sessions properly counted even
     * when charging (but not idle).
     */
    @Test
    public void testJobsInRestrictedBucket_CorrectParoleWhileCharging() throws Exception {
        assumeTrue("app standby not enabled", mAppStandbyEnabled);
        assumeFalse("not testable in automotive device", mAutomotiveDevice);
        assumeFalse("not testable in leanback device", mLeanbackOnly);

        setRestrictedBucketEnabled(true);

        // Disable coalescing
        mDeviceConfigStateHelper.set("qc_timing_session_coalescing_duration_ms", "0");
        mDeviceConfigStateHelper.set("qc_max_session_count_restricted", "1");

        setScreenState(true);
        BatteryUtils.runDumpsysBatterySetPluggedIn(true);
        BatteryUtils.runDumpsysBatterySetLevel(100);

        setTestPackageStandbyBucket(Bucket.RESTRICTED);
        Thread.sleep(DEFAULT_WAIT_TIMEOUT);
        sendScheduleJobBroadcast(false);
        runJob();
        assertTrue("Parole job didn't start in RESTRICTED bucket",
                mTestAppInterface.awaitJobStart(3_000));

        sendScheduleJobBroadcast(false);
        assertFalse("New job started in RESTRICTED bucket after parole used",
                mTestAppInterface.awaitJobStart(3_000));
    }

    /**
     * Tests that apps in the RESTRICTED bucket that have used their one parole session per day
     * don't get to run again until the device is charging + idle.
     */
    @Test
    public void testJobsInRestrictedBucket_DeferredUntilFreeResources() throws Exception {
        assumeTrue("app standby not enabled", mAppStandbyEnabled);
        assumeFalse("not testable in automotive device", mAutomotiveDevice);
        assumeFalse("not testable in leanback device", mLeanbackOnly);

        setRestrictedBucketEnabled(true);

        // Disable coalescing
        mDeviceConfigStateHelper.set("qc_timing_session_coalescing_duration_ms", "0");

        setScreenState(true);

        BatteryUtils.runDumpsysBatteryUnplug();
        setTestPackageStandbyBucket(Bucket.RESTRICTED);
        Thread.sleep(DEFAULT_WAIT_TIMEOUT);
        sendScheduleJobBroadcast(false);
        runJob();
        assertTrue("Parole job didn't start in RESTRICTED bucket",
                mTestAppInterface.awaitJobStart(3_000));

        sendScheduleJobBroadcast(false);
        assertFalse("New job started in RESTRICTED bucket after parole used",
                mTestAppInterface.awaitJobStart(3_000));

        BatteryUtils.runDumpsysBatterySetPluggedIn(true);
        BatteryUtils.runDumpsysBatterySetLevel(100);
        assertFalse("New job started in RESTRICTED bucket after parole when charging but not idle",
                mTestAppInterface.awaitJobStart(3_000));

        setScreenState(false);
        triggerJobIdle();
        assertTrue("Job didn't start in RESTRICTED bucket when charging + idle",
                mTestAppInterface.awaitJobStart(3_000));

        // Make sure job can be stopped and started again when charging + idle
        sendScheduleJobBroadcast(false);
        assertTrue("Job didn't restart in RESTRICTED bucket when charging + idle",
                mTestAppInterface.awaitJobStart(3_000));
    }

    @Test
    public void testJobsInRestrictedBucket_NoRequiredNetwork() throws Exception {
        assumeTrue("app standby not enabled", mAppStandbyEnabled);
        assumeFalse("not testable in automotive device", mAutomotiveDevice);
        assumeFalse("not testable in leanback device", mLeanbackOnly);

        setRestrictedBucketEnabled(true);

        // Disable coalescing and the parole session
        mDeviceConfigStateHelper.set("qc_timing_session_coalescing_duration_ms", "0");
        mDeviceConfigStateHelper.set("qc_max_session_count_restricted", "0");

        setAirplaneMode(true);
        setScreenState(true);

        BatteryUtils.runDumpsysBatteryUnplug();
        setTestPackageStandbyBucket(Bucket.RESTRICTED);
        Thread.sleep(DEFAULT_WAIT_TIMEOUT);
        mTestAppInterface.scheduleJob(false, NETWORK_TYPE_NONE, false);
        assertFalse("New job started in RESTRICTED bucket", mTestAppInterface.awaitJobStart(3_000));

        // Slowly add back required bucket constraints.

        // Battery charging and high.
        BatteryUtils.runDumpsysBatterySetPluggedIn(true);
        assertFalse("New job started in RESTRICTED bucket", mTestAppInterface.awaitJobStart(3_000));
        BatteryUtils.runDumpsysBatterySetLevel(100);
        assertFalse("New job started in RESTRICTED bucket", mTestAppInterface.awaitJobStart(3_000));

        // Device is idle.
        setScreenState(false);
        assertFalse("New job started in RESTRICTED bucket", mTestAppInterface.awaitJobStart(3_000));
        triggerJobIdle();
        assertTrue("New job didn't start in RESTRICTED bucket",
                mTestAppInterface.awaitJobStart(3_000));
    }

    @RequiresDevice // Emulators don't always have access to wifi/network
    @Test
    public void testJobsInRestrictedBucket_WithRequiredNetwork() throws Exception {
        assumeTrue("app standby not enabled", mAppStandbyEnabled);
        assumeFalse("not testable in automotive device", mAutomotiveDevice);
        assumeFalse("not testable in leanback device", mLeanbackOnly);

        assumeTrue(mHasWifi);
        ensureSavedWifiNetwork(mWifiManager);

        setRestrictedBucketEnabled(true);

        // Disable coalescing and the parole session
        mDeviceConfigStateHelper.set("qc_timing_session_coalescing_duration_ms", "0");
        mDeviceConfigStateHelper.set("qc_max_session_count_restricted", "0");

        setAirplaneMode(true);
        setScreenState(true);

        BatteryUtils.runDumpsysBatteryUnplug();
        setTestPackageStandbyBucket(Bucket.RESTRICTED);
        Thread.sleep(DEFAULT_WAIT_TIMEOUT);
        mTestAppInterface.scheduleJob(false, NETWORK_TYPE_ANY, false);
        runJob();
        assertFalse("New job started in RESTRICTED bucket", mTestAppInterface.awaitJobStart(3_000));

        // Slowly add back required bucket constraints.

        // Battery charging and high.
        BatteryUtils.runDumpsysBatterySetPluggedIn(true);
        runJob();
        assertFalse("New job started in RESTRICTED bucket", mTestAppInterface.awaitJobStart(3_000));
        BatteryUtils.runDumpsysBatterySetLevel(100);
        runJob();
        assertFalse("New job started in RESTRICTED bucket", mTestAppInterface.awaitJobStart(3_000));

        // Device is idle.
        setScreenState(false);
        runJob();
        assertFalse("New job started in RESTRICTED bucket", mTestAppInterface.awaitJobStart(3_000));
        triggerJobIdle();
        runJob();
        assertFalse("New job started in RESTRICTED bucket", mTestAppInterface.awaitJobStart(3_000));

        // Add network
        setAirplaneMode(false);
        setWifiState(true, mCm, mWifiManager);
        setWifiMeteredState(false);
        runJob();
        assertTrue("New job didn't start in RESTRICTED bucket",
                mTestAppInterface.awaitJobStart(5_000));
    }

    @Test
    public void testJobsInNeverApp() throws Exception {
        assumeTrue("app standby not enabled", mAppStandbyEnabled);
        assumeFalse("not testable in automotive device", mAutomotiveDevice);
        assumeFalse("not testable in leanback device", mLeanbackOnly);

        BatteryUtils.runDumpsysBatteryUnplug();
        setTestPackageStandbyBucket(Bucket.NEVER);
        Thread.sleep(DEFAULT_WAIT_TIMEOUT);
        sendScheduleJobBroadcast(false);
        assertFalse("New job started in NEVER bucket", mTestAppInterface.awaitJobStart(3_000));
    }

    @Test
    public void testUidActiveBypassesStandby() throws Exception {
        assumeFalse("not testable in automotive device", mAutomotiveDevice);
        assumeFalse("not testable in leanback device", mLeanbackOnly);

        BatteryUtils.runDumpsysBatteryUnplug();
        setTestPackageStandbyBucket(Bucket.NEVER);
        tempWhitelistTestApp(6_000);
        Thread.sleep(DEFAULT_WAIT_TIMEOUT);
        sendScheduleJobBroadcast(false);
        assertTrue("New job in uid-active app failed to start in NEVER standby",
                mTestAppInterface.awaitJobStart(4_000));
    }

    @Test
    public void testBatterySaverOff() throws Exception {
        assumeFalse("not testable in automotive device", mAutomotiveDevice);
        assumeFalse("not testable in leanback device", mLeanbackOnly);

        BatteryUtils.assumeBatterySaverFeature();

        BatteryUtils.runDumpsysBatteryUnplug();
        BatteryUtils.enableBatterySaver(false);
        sendScheduleJobBroadcast(false);
        assertTrue("New job failed to start with battery saver OFF",
                mTestAppInterface.awaitJobStart(3_000));
    }

    @Test
    public void testBatterySaverOn() throws Exception {
        assumeFalse("not testable in automotive device", mAutomotiveDevice);
        assumeFalse("not testable in leanback device", mLeanbackOnly);

        BatteryUtils.assumeBatterySaverFeature();

        BatteryUtils.runDumpsysBatteryUnplug();
        BatteryUtils.enableBatterySaver(true);
        sendScheduleJobBroadcast(false);
        assertFalse("New job started with battery saver ON",
                mTestAppInterface.awaitJobStart(3_000));
    }

    @Test
    public void testUidActiveBypassesBatterySaverOn() throws Exception {
        assumeFalse("not testable in automotive device", mAutomotiveDevice);
        assumeFalse("not testable in leanback device", mLeanbackOnly);

        BatteryUtils.assumeBatterySaverFeature();

        BatteryUtils.runDumpsysBatteryUnplug();
        BatteryUtils.enableBatterySaver(true);
        tempWhitelistTestApp(6_000);
        sendScheduleJobBroadcast(false);
        assertTrue("New job in uid-active app failed to start with battery saver ON",
                mTestAppInterface.awaitJobStart(3_000));
    }

    @Test
    public void testBatterySaverOnThenUidActive() throws Exception {
        assumeFalse("not testable in automotive device", mAutomotiveDevice);
        assumeFalse("not testable in leanback device", mLeanbackOnly);

        BatteryUtils.assumeBatterySaverFeature();

        // Enable battery saver, and schedule a job. It shouldn't run.
        BatteryUtils.runDumpsysBatteryUnplug();
        BatteryUtils.enableBatterySaver(true);
        sendScheduleJobBroadcast(false);
        assertFalse("New job started with battery saver ON",
                mTestAppInterface.awaitJobStart(3_000));

        // Then make the UID active. Now the job should run.
        tempWhitelistTestApp(120_000);
        assertTrue("New job in uid-active app failed to start with battery saver OFF",
                mTestAppInterface.awaitJobStart(120_000));
    }

    @Test
    public void testExpeditedJobBypassesBatterySaverOn() throws Exception {
        assumeFalse("not testable in automotive device", mAutomotiveDevice);
        assumeFalse("not testable in leanback device", mLeanbackOnly);

        BatteryUtils.assumeBatterySaverFeature();

        BatteryUtils.runDumpsysBatteryUnplug();
        BatteryUtils.enableBatterySaver(true);
        mTestAppInterface.scheduleJob(false, JobInfo.NETWORK_TYPE_NONE, true);
        assertTrue("New expedited job failed to start with battery saver ON",
                mTestAppInterface.awaitJobStart(DEFAULT_WAIT_TIMEOUT));
    }

    @Test
    public void testExpeditedJobBypassesBatterySaver_toggling() throws Exception {
        assumeFalse("not testable in automotive device", mAutomotiveDevice);
        assumeFalse("not testable in leanback device", mLeanbackOnly);

        BatteryUtils.assumeBatterySaverFeature();

        BatteryUtils.runDumpsysBatteryUnplug();
        BatteryUtils.enableBatterySaver(false);
        mTestAppInterface.scheduleJob(false, JobInfo.NETWORK_TYPE_NONE, true);
        assertTrue("New expedited job failed to start with battery saver ON",
                mTestAppInterface.awaitJobStart(DEFAULT_WAIT_TIMEOUT));
        BatteryUtils.enableBatterySaver(true);
        assertFalse("Job stopped when battery saver turned on",
                mTestAppInterface.awaitJobStop(DEFAULT_WAIT_TIMEOUT));
    }

    @Test
    public void testExpeditedJobBypassesDeviceIdle() throws Exception {
        assumeTrue("device idle not enabled", mDeviceIdleEnabled);

        toggleDozeState(true);
        mTestAppInterface.scheduleJob(false, JobInfo.NETWORK_TYPE_NONE, true);
        runJob();
        assertTrue("Job did not start after scheduling",
                mTestAppInterface.awaitJobStart(DEFAULT_WAIT_TIMEOUT));
    }

    @Test
    public void testExpeditedJobBypassesDeviceIdle_toggling() throws Exception {
        assumeTrue("device idle not enabled", mDeviceIdleEnabled);

        toggleDozeState(false);
        mTestAppInterface.scheduleJob(false, JobInfo.NETWORK_TYPE_NONE, true);
        runJob();
        assertTrue("Job did not start after scheduling",
                mTestAppInterface.awaitJobStart(DEFAULT_WAIT_TIMEOUT));
        toggleDozeState(true);
        assertFalse("Job stopped when device enabled turned on",
                mTestAppInterface.awaitJobStop(DEFAULT_WAIT_TIMEOUT));
    }

    @Test
    public void testExpeditedJobDeferredAfterTimeoutInDoze() throws Exception {
        assumeTrue("device idle not enabled", mDeviceIdleEnabled);
        // Intentionally set a value below 1 minute to ensure the range checks work.
        mDeviceConfigStateHelper.set("runtime_min_ej_guarantee_ms", Long.toString(30_000L));

        toggleDozeState(true);
        mTestAppInterface.scheduleJob(false, JobInfo.NETWORK_TYPE_NONE, true);
        runJob();
        assertTrue("Job did not start after scheduling",
                mTestAppInterface.awaitJobStart(DEFAULT_WAIT_TIMEOUT));
        // Don't put full minute as the timeout to give some leeway with test timing/processing.
        assertFalse("Job stopped before min runtime limit",
                mTestAppInterface.awaitJobStop(55_000L));
        assertTrue("Job did not stop after timeout", mTestAppInterface.awaitJobStop(15_000L));
        assertEquals(JobParameters.STOP_REASON_DEVICE_STATE,
                mTestAppInterface.getLastParams().getStopReason());
        // Should be rescheduled.
        assertJobNotReady();
        assertJobWaiting();
        Thread.sleep(TestJobSchedulerReceiver.JOB_INITIAL_BACKOFF);
        runJob();
        assertFalse("Job started after timing out in Doze",
                mTestAppInterface.awaitJobStart(DEFAULT_WAIT_TIMEOUT));

        // Should start when Doze is turned off.
        toggleDozeState(false);
        assertTrue("Job did not start after Doze turned off",
                mTestAppInterface.awaitJobStart(DEFAULT_WAIT_TIMEOUT));
    }

    @Test
    public void testExpeditedJobDeferredAfterTimeoutInBatterySaver() throws Exception {
        BatteryUtils.assumeBatterySaverFeature();

        // Intentionally set a value below 1 minute to ensure the range checks work.
        mDeviceConfigStateHelper.set("runtime_min_ej_guarantee_ms", Long.toString(47_000L));

        BatteryUtils.runDumpsysBatteryUnplug();
        BatteryUtils.enableBatterySaver(true);
        mTestAppInterface.scheduleJob(false, JobInfo.NETWORK_TYPE_NONE, true);
        runJob();
        assertTrue("Job did not start after scheduling",
                mTestAppInterface.awaitJobStart(DEFAULT_WAIT_TIMEOUT));
        // Don't put full minute as the timeout to give some leeway with test timing/processing.
        assertFalse("Job stopped before min runtime limit",
                mTestAppInterface.awaitJobStop(55_000L));
        assertTrue("Job did not stop after timeout", mTestAppInterface.awaitJobStop(15_000L));
        assertEquals(JobParameters.STOP_REASON_DEVICE_STATE,
                mTestAppInterface.getLastParams().getStopReason());
        // Should be rescheduled.
        assertJobNotReady();
        assertJobWaiting();
        Thread.sleep(TestJobSchedulerReceiver.JOB_INITIAL_BACKOFF);
        runJob();
        assertFalse("Job started after timing out in battery saver",
                mTestAppInterface.awaitJobStart(DEFAULT_WAIT_TIMEOUT));

        // Should start when battery saver is turned off.
        BatteryUtils.enableBatterySaver(false);
        assertTrue("Job did not start after battery saver turned off",
                mTestAppInterface.awaitJobStart(DEFAULT_WAIT_TIMEOUT));
    }

    @Test
    public void testExpeditedJobDeferredAfterTimeout_DozeAndBatterySaver() throws Exception {
        BatteryUtils.assumeBatterySaverFeature();
        assumeTrue("device idle not enabled", mDeviceIdleEnabled);
        mDeviceConfigStateHelper.set("runtime_min_ej_guarantee_ms", Long.toString(60_000L));

        BatteryUtils.runDumpsysBatteryUnplug();
        toggleDozeState(true);
        mTestAppInterface.scheduleJob(false, JobInfo.NETWORK_TYPE_NONE, true);
        runJob();
        assertTrue("Job did not start after scheduling",
                mTestAppInterface.awaitJobStart(DEFAULT_WAIT_TIMEOUT));
        // Don't put full minute as the timeout to give some leeway with test timing/processing.
        assertFalse("Job stopped before min runtime limit",
                mTestAppInterface.awaitJobStop(55_000L));
        assertTrue("Job did not stop after timeout", mTestAppInterface.awaitJobStop(15_000L));
        assertEquals(JobParameters.STOP_REASON_DEVICE_STATE,
                mTestAppInterface.getLastParams().getStopReason());
        // Should be rescheduled.
        assertJobNotReady();
        assertJobWaiting();
        // Battery saver kicks in before Doze ends. Job shouldn't start while BS is on.
        BatteryUtils.enableBatterySaver(true);
        toggleDozeState(false);
        Thread.sleep(TestJobSchedulerReceiver.JOB_INITIAL_BACKOFF);
        runJob();
        assertFalse("Job started while power restrictions active after timing out",
                mTestAppInterface.awaitJobStart(DEFAULT_WAIT_TIMEOUT));

        // Should start when battery saver is turned off.
        BatteryUtils.enableBatterySaver(false);
        assertTrue("Job did not start after power restrictions turned off",
                mTestAppInterface.awaitJobStart(DEFAULT_WAIT_TIMEOUT));
    }

    @Test
    public void testLongExpeditedJobStoppedByDoze() throws Exception {
        assumeTrue("device idle not enabled", mDeviceIdleEnabled);
        // Intentionally set a value below 1 minute to ensure the range checks work.
        mDeviceConfigStateHelper.set("runtime_min_ej_guarantee_ms", Long.toString(59_000L));

        toggleDozeState(false);
        mTestAppInterface.scheduleJob(false, JobInfo.NETWORK_TYPE_NONE, true);
        runJob();
        assertTrue("Job did not start after scheduling",
                mTestAppInterface.awaitJobStart(DEFAULT_WAIT_TIMEOUT));
        // Should get to run past min runtime.
        assertFalse("Job stopped after min runtime", mTestAppInterface.awaitJobStop(90_000L));

        // Should stop when Doze is turned on.
        toggleDozeState(true);
        assertTrue("Job did not stop after Doze turned on",
                mTestAppInterface.awaitJobStop(DEFAULT_WAIT_TIMEOUT));
        assertEquals(JobParameters.STOP_REASON_DEVICE_STATE,
                mTestAppInterface.getLastParams().getStopReason());
    }

    @Test
    public void testLongExpeditedJobStoppedByBatterySaver() throws Exception {
        BatteryUtils.assumeBatterySaverFeature();

        // Intentionally set a value below 1 minute to ensure the range checks work.
        mDeviceConfigStateHelper.set("runtime_min_ej_guarantee_ms", Long.toString(0L));

        BatteryUtils.runDumpsysBatteryUnplug();
        BatteryUtils.enableBatterySaver(false);
        mTestAppInterface.scheduleJob(false, JobInfo.NETWORK_TYPE_NONE, true);
        runJob();
        assertTrue("Job did not start after scheduling",
                mTestAppInterface.awaitJobStart(DEFAULT_WAIT_TIMEOUT));
        // Should get to run past min runtime.
        assertFalse("Job stopped after runtime", mTestAppInterface.awaitJobStop(90_000L));

        // Should stop when battery saver is turned on.
        BatteryUtils.enableBatterySaver(true);
        assertTrue("Job did not stop after battery saver turned on",
                mTestAppInterface.awaitJobStop(DEFAULT_WAIT_TIMEOUT));
        assertEquals(JobParameters.STOP_REASON_DEVICE_STATE,
                mTestAppInterface.getLastParams().getStopReason());
    }

    @Test
    public void testRestrictingStopReason_RestrictedBucket() throws Exception {
        assumeTrue("app standby not enabled", mAppStandbyEnabled);
        assumeFalse("not testable in automotive device", mAutomotiveDevice);
        assumeFalse("not testable in leanback device", mLeanbackOnly);

        assumeTrue(mHasWifi);
        ensureSavedWifiNetwork(mWifiManager);

        setRestrictedBucketEnabled(true);
        setTestPackageStandbyBucket(Bucket.RESTRICTED);

        // Disable coalescing and the parole session
        mDeviceConfigStateHelper.set("qc_timing_session_coalescing_duration_ms", "0");
        mDeviceConfigStateHelper.set("qc_max_session_count_restricted", "0");

        // Satisfy all additional constraints.
        setAirplaneMode(false);
        setWifiState(true, mCm, mWifiManager);
        setWifiMeteredState(false);
        BatteryUtils.runDumpsysBatterySetPluggedIn(true);
        BatteryUtils.runDumpsysBatterySetLevel(100);
        setScreenState(false);
        triggerJobIdle();

        // Toggle individual constraints

        // Connectivity
        mTestAppInterface.scheduleJob(false, NETWORK_TYPE_ANY, false);
        runJob();
        assertTrue("New job didn't start in RESTRICTED bucket",
                mTestAppInterface.awaitJobStart(DEFAULT_WAIT_TIMEOUT));
        setAirplaneMode(true);
        assertTrue("New job didn't stop when connectivity dropped",
                mTestAppInterface.awaitJobStop(DEFAULT_WAIT_TIMEOUT));
        assertEquals(JobParameters.STOP_REASON_CONSTRAINT_CONNECTIVITY,
                mTestAppInterface.getLastParams().getStopReason());
        setAirplaneMode(false);
        setWifiState(true, mCm, mWifiManager);

        // Idle
        mTestAppInterface.scheduleJob(false, NETWORK_TYPE_ANY, false);
        runJob();
        assertTrue("New job didn't start in RESTRICTED bucket",
                mTestAppInterface.awaitJobStart(DEFAULT_WAIT_TIMEOUT));
        setScreenState(true);
        assertTrue("New job didn't stop when device no longer idle",
                mTestAppInterface.awaitJobStop(DEFAULT_WAIT_TIMEOUT));
        assertEquals(JobParameters.STOP_REASON_APP_STANDBY,
                mTestAppInterface.getLastParams().getStopReason());
        setScreenState(false);
        triggerJobIdle();

        // Charging
        mTestAppInterface.scheduleJob(false, NETWORK_TYPE_ANY, false);
        runJob();
        assertTrue("New job didn't start in RESTRICTED bucket",
                mTestAppInterface.awaitJobStart(DEFAULT_WAIT_TIMEOUT));
        BatteryUtils.runDumpsysBatteryUnplug();
        assertTrue("New job didn't stop when device no longer charging",
                mTestAppInterface.awaitJobStop(DEFAULT_WAIT_TIMEOUT));
        assertEquals(JobParameters.STOP_REASON_APP_STANDBY,
                mTestAppInterface.getLastParams().getStopReason());
        BatteryUtils.runDumpsysBatterySetPluggedIn(true);
        BatteryUtils.runDumpsysBatterySetLevel(100);

        // Battery not low
        setScreenState(false);
        triggerJobIdle();
        mTestAppInterface.scheduleJob(false, NETWORK_TYPE_ANY, false);
        runJob();
        assertTrue("New job didn't start in RESTRICTED bucket",
                mTestAppInterface.awaitJobStart(DEFAULT_WAIT_TIMEOUT));
        BatteryUtils.runDumpsysBatterySetLevel(1);
        assertTrue("New job didn't stop when battery too low",
                mTestAppInterface.awaitJobStop(DEFAULT_WAIT_TIMEOUT));
        assertEquals(JobParameters.STOP_REASON_APP_STANDBY,
                mTestAppInterface.getLastParams().getStopReason());
    }

    @Test
    public void testRestrictingStopReason_Quota() throws Exception {
        // Reduce allowed time for testing.
        mDeviceConfigStateHelper.set("qc_allowed_time_per_period_ms", "60000");
        BatteryUtils.runDumpsysBatteryUnplug();
        setTestPackageStandbyBucket(Bucket.RARE);

        sendScheduleJobBroadcast(false);
        runJob();
        assertTrue("New job didn't start",
                mTestAppInterface.awaitJobStart(DEFAULT_WAIT_TIMEOUT));

        Thread.sleep(60000);

        assertTrue("New job didn't stop after using up quota",
                mTestAppInterface.awaitJobStop(DEFAULT_WAIT_TIMEOUT));
        assertEquals(JobParameters.STOP_REASON_QUOTA,
                mTestAppInterface.getLastParams().getStopReason());
    }

    @Test
    public void testRestrictingStopReason_BatterySaver() throws Exception {
        BatteryUtils.assumeBatterySaverFeature();

        BatteryUtils.runDumpsysBatteryUnplug();
        BatteryUtils.enableBatterySaver(false);
        sendScheduleJobBroadcast(false);
        runJob();
        assertTrue("Job did not start after scheduling",
                mTestAppInterface.awaitJobStart(DEFAULT_WAIT_TIMEOUT));

        BatteryUtils.enableBatterySaver(true);
        assertTrue("Job did not stop on entering battery saver",
                mTestAppInterface.awaitJobStop(DEFAULT_WAIT_TIMEOUT));
        assertEquals(JobParameters.STOP_REASON_DEVICE_STATE,
                mTestAppInterface.getLastParams().getStopReason());
    }

    @Test
    public void testRestrictingStopReason_Doze() throws Exception {
        assumeTrue("device idle not enabled", mDeviceIdleEnabled);

        toggleDozeState(false);
        mTestAppInterface.scheduleJob(false, NETWORK_TYPE_NONE, false);
        runJob();
        assertTrue("Job did not start after scheduling",
                mTestAppInterface.awaitJobStart(DEFAULT_WAIT_TIMEOUT));

        toggleDozeState(true);
        assertTrue("Job did not stop on entering doze",
                mTestAppInterface.awaitJobStop(DEFAULT_WAIT_TIMEOUT));
        assertEquals(JobParameters.STOP_REASON_DEVICE_STATE,
                mTestAppInterface.getLastParams().getStopReason());
    }

    @After
    public void tearDown() throws Exception {
        AppOpsUtils.reset(TEST_APP_PACKAGE);
        // Lock thermal service to not throttling
        ThermalUtils.overrideThermalNotThrottling();
        if (mDeviceIdleEnabled) {
            toggleDozeState(false);
        }
        mTestAppInterface.cleanup();
        BatteryUtils.runDumpsysBatteryReset();
        BatteryUtils.enableBatterySaver(false);
        removeTestAppFromTempWhitelist();

        // Ensure that we leave WiFi in its previous state.
        if (mHasWifi && mWifiManager.isWifiEnabled() != mInitialWiFiState) {
            try {
                setWifiState(mInitialWiFiState, mCm, mWifiManager);
            } catch (AssertionFailedError e) {
                // Don't fail the test just because wifi state wasn't set in tearDown.
                Log.e(TAG, "Failed to return wifi state to " + mInitialWiFiState, e);
            }
        }
        mDeviceConfigStateHelper.restoreOriginalValues();
        Settings.Global.putString(mContext.getContentResolver(),
                Settings.Global.ENABLE_RESTRICTED_BUCKET, mInitialRestrictedBucketEnabled);
        if (isAirplaneModeOn() != mInitialAirplaneModeState) {
            setAirplaneMode(mInitialAirplaneModeState);
        }
        mUiDevice.executeShellCommand(
                "cmd jobscheduler reset-execution-quota -u " + UserHandle.myUserId()
                        + " " + TEST_APP_PACKAGE);

        Settings.System.putString(
                mContext.getContentResolver(), SCREEN_OFF_TIMEOUT, mInitialDisplayTimeout);
    }

    private void setTestPackageRestricted(boolean restricted) throws Exception {
        AppOpsUtils.setOpMode(TEST_APP_PACKAGE, "RUN_ANY_IN_BACKGROUND",
                restricted ? AppOpsManager.MODE_IGNORED : AppOpsManager.MODE_ALLOWED);
    }

    private void setRestrictedBucketEnabled(boolean enabled) {
        Settings.Global.putString(mContext.getContentResolver(),
                Settings.Global.ENABLE_RESTRICTED_BUCKET, enabled ? "1" : "0");
    }

    private boolean isTestAppTempWhitelisted() throws Exception {
        final String output = mUiDevice.executeShellCommand("cmd deviceidle tempwhitelist").trim();
        for (String line : output.split("\n")) {
            if (line.contains("UID=" + mTestPackageUid)) {
                return true;
            }
        }
        return false;
    }

    private void sendScheduleJobBroadcast(boolean allowWhileIdle) throws Exception {
        mTestAppInterface.scheduleJob(allowWhileIdle, NETWORK_TYPE_NONE, false);
    }

    private void toggleDozeState(final boolean idle) throws Exception {
        mUiDevice.executeShellCommand("cmd deviceidle " + (idle ? "force-idle" : "unforce"));
        if (!idle) {
            // Make sure the device doesn't stay idle, even after unforcing.
            mUiDevice.executeShellCommand("cmd deviceidle motion");
        }
        assertTrue("Could not change device idle state to " + idle,
                waitUntilTrue(SHELL_TIMEOUT, () -> {
                    synchronized (JobThrottlingTest.this) {
                        return mDeviceInDoze == idle;
                    }
                }));
    }

    private void tempWhitelistTestApp(long duration) throws Exception {
        mUiDevice.executeShellCommand("cmd deviceidle tempwhitelist -d " + duration
                + " " + TEST_APP_PACKAGE);
    }

    private void makeTestPackageIdle() throws Exception {
        mUiDevice.executeShellCommand("am make-uid-idle --user current " + TEST_APP_PACKAGE);
    }

    void setTestPackageStandbyBucket(Bucket bucket) throws Exception {
        setTestPackageStandbyBucket(mUiDevice, bucket);
    }

    static void setTestPackageStandbyBucket(UiDevice uiDevice, Bucket bucket) throws Exception {
        final String bucketName;
        switch (bucket) {
            case ACTIVE:
                bucketName = "active";
                break;
            case WORKING_SET:
                bucketName = "working";
                break;
            case FREQUENT:
                bucketName = "frequent";
                break;
            case RARE:
                bucketName = "rare";
                break;
            case RESTRICTED:
                bucketName = "restricted";
                break;
            case NEVER:
                bucketName = "never";
                break;
            default:
                throw new IllegalArgumentException("Requested unknown bucket " + bucket);
        }
        uiDevice.executeShellCommand("am set-standby-bucket " + TEST_APP_PACKAGE
                + " " + bucketName);
    }

    private boolean removeTestAppFromTempWhitelist() throws Exception {
        mUiDevice.executeShellCommand("cmd deviceidle tempwhitelist -r " + TEST_APP_PACKAGE);
        return waitUntilTrue(SHELL_TIMEOUT, () -> !isTestAppTempWhitelisted());
    }

    /**
     * Set the screen state.
     */
    private void setScreenState(boolean on) throws Exception {
        if (on) {
            mUiDevice.executeShellCommand("input keyevent KEYCODE_WAKEUP");
            mUiDevice.executeShellCommand("wm dismiss-keyguard");
        } else {
            mUiDevice.executeShellCommand("input keyevent KEYCODE_SLEEP");
        }
        // Wait a little bit to make sure the screen state has changed.
        Thread.sleep(2_000);
    }

    /**
     * Trigger job idle (not device idle);
     */
    private void triggerJobIdle() throws Exception {
        mUiDevice.executeShellCommand("cmd activity idle-maintenance");
        // Wait a moment to let that happen before proceeding.
        Thread.sleep(2_000);
    }

    /** Asks (not forces) JobScheduler to run the job if constraints are met. */
    private void runJob() throws Exception {
        // Since connectivity is a functional constraint, calling the "run" command without force
        // will only get the job to run if the constraint is satisfied.
        mUiDevice.executeShellCommand("cmd jobscheduler run -s"
                + " -u " + UserHandle.myUserId() + " " + TEST_APP_PACKAGE + " " + mTestJobId);
    }

    private boolean isAirplaneModeOn() throws IOException {
        final String output =
                mUiDevice.executeShellCommand("cmd connectivity airplane-mode").trim();
        return "enabled".equals(output);
    }

    private void setAirplaneMode(boolean on) throws Exception {
        final CallbackAsserter airplaneModeBroadcastAsserter = CallbackAsserter.forBroadcast(
                new IntentFilter(Intent.ACTION_AIRPLANE_MODE_CHANGED));
        mUiDevice.executeShellCommand(
                "cmd connectivity airplane-mode " + (on ? "enable" : "disable"));
        airplaneModeBroadcastAsserter.assertCalled("Didn't get airplane mode changed broadcast",
                15 /* 15 seconds */);
        if (!on && mHasWifi) {
            // Force wifi to connect ASAP.
            mUiDevice.executeShellCommand("svc wifi enable");
            //noinspection deprecation
            SystemUtil.runWithShellPermissionIdentity(mWifiManager::reconnect,
                    android.Manifest.permission.NETWORK_SETTINGS);
        }
        waitUntil("Networks didn't change to " + (!on ? "on" : "off"), 60 /* seconds */,
                () -> {
                    if (on) {
                        return mCm.getActiveNetwork() == null
                                && (!mHasWifi || !isWiFiConnected(mCm, mWifiManager));
                    } else {
                        return mCm.getActiveNetwork() != null;
                    }
                });
        // Wait some time for the network changes to propagate. Can't use
        // waitUntil(isAirplaneModeOn() == on) because the response quickly gives the new
        // airplane mode status even though the network changes haven't propagated all the way to
        // JobScheduler.
        Thread.sleep(5000);
    }

    private static String unquoteSSID(String ssid) {
        // SSID is returned surrounded by quotes if it can be decoded as UTF-8.
        // Otherwise it's guaranteed not to start with a quote.
        if (ssid.charAt(0) == '"') {
            return ssid.substring(1, ssid.length() - 1);
        } else {
            return ssid;
        }
    }

    private String getWifiSSID() {
        final AtomicReference<String> ssid = new AtomicReference<>();
        SystemUtil.runWithShellPermissionIdentity(() -> {
            ssid.set(mWifiManager.getConnectionInfo().getSSID());
        }, Manifest.permission.ACCESS_FINE_LOCATION);
        return unquoteSSID(ssid.get());
    }

    // Returns "true", "false" or "none"
    private String getWifiMeteredStatus(String ssid) {
        // Interestingly giving the SSID as an argument to list wifi-networks
        // only works iff the network in question has the "false" policy.
        // Also unfortunately runShellCommand does not pass the command to the interpreter
        // so it's not possible to | grep the ssid.
        final String command = "cmd netpolicy list wifi-networks";
        final String policyString = SystemUtil.runShellCommand(command);

        final Matcher m = Pattern.compile("^" + ssid + ";(true|false|none)$",
                Pattern.MULTILINE | Pattern.UNIX_LINES).matcher(policyString);
        if (!m.find()) {
            fail("Unexpected format from cmd netpolicy (when looking for " + ssid + "): "
                    + policyString);
        }
        return m.group(1);
    }

    private void setWifiMeteredState(boolean metered) throws Exception {
        if (metered) {
            // Make sure unmetered cellular networks don't interfere.
            setAirplaneMode(true);
            setWifiState(true, mCm, mWifiManager);
        }
        final String ssid = getWifiSSID();
        setWifiMeteredState(ssid, metered ? "true" : "false");
    }

    // metered should be "true", "false" or "none"
    private void setWifiMeteredState(String ssid, String metered) {
        if (metered.equals(getWifiMeteredStatus(ssid))) {
            return;
        }
        SystemUtil.runShellCommand("cmd netpolicy set metered-network " + ssid + " " + metered);
        assertEquals(getWifiMeteredStatus(ssid), metered);
    }

    private String getJobState() throws Exception {
        return mUiDevice.executeShellCommand("cmd jobscheduler get-job-state --user cur "
                + TEST_APP_PACKAGE + " " + mTestJobId).trim();
    }

    private void assertJobWaiting() throws Exception {
        String state = getJobState();
        assertTrue("Job unexpectedly not waiting, in state: " + state, state.contains("waiting"));
    }

    private void assertJobNotReady() throws Exception {
        String state = getJobState();
        assertFalse("Job unexpectedly ready, in state: " + state, state.contains("ready"));
    }

    private void assertJobReady() throws Exception {
        String state = getJobState();
        assertTrue("Job unexpectedly not ready, in state: " + state, state.contains("ready"));
    }

    private boolean waitUntilTrue(long maxWait, Condition condition) throws Exception {
        final long deadLine = SystemClock.uptimeMillis() + maxWait;
        do {
            Thread.sleep(POLL_INTERVAL);
        } while (!condition.isTrue() && SystemClock.uptimeMillis() < deadLine);
        return condition.isTrue();
    }

    private interface Condition {
        boolean isTrue() throws Exception;
    }
}
