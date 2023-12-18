/*
 * Copyright (C) 2008 The Android Open Source Project
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
package android.app.cts;

import static android.content.ComponentCallbacks2.TRIM_MEMORY_BACKGROUND;
import static android.content.ComponentCallbacks2.TRIM_MEMORY_COMPLETE;
import static android.content.ComponentCallbacks2.TRIM_MEMORY_RUNNING_CRITICAL;
import static android.content.ComponentCallbacks2.TRIM_MEMORY_RUNNING_LOW;
import static android.content.ComponentCallbacks2.TRIM_MEMORY_RUNNING_MODERATE;
import static android.content.ComponentCallbacks2.TRIM_MEMORY_UI_HIDDEN;

import static org.junit.Assert.assertArrayEquals;

import android.app.Activity;
import android.app.ActivityManager;
import android.app.ActivityManager.RecentTaskInfo;
import android.app.ActivityManager.RunningAppProcessInfo;
import android.app.ActivityManager.RunningServiceInfo;
import android.app.ActivityManager.RunningTaskInfo;
import android.app.ActivityOptions;
import android.app.HomeVisibilityListener;
import android.app.Instrumentation;
import android.app.Instrumentation.ActivityMonitor;
import android.app.Instrumentation.ActivityResult;
import android.app.PendingIntent;
import android.app.cts.android.app.cts.tools.WatchUidRunner;
import android.app.stubs.ActivityManagerRecentOneActivity;
import android.app.stubs.ActivityManagerRecentTwoActivity;
import android.app.stubs.CommandReceiver;
import android.app.stubs.LocalForegroundService;
import android.app.stubs.MockApplicationActivity;
import android.app.stubs.MockService;
import android.app.stubs.ScreenOnActivity;
import android.app.stubs.TrimMemService;
import android.content.BroadcastReceiver;
import android.content.Context;
import android.content.Intent;
import android.content.IntentFilter;
import android.content.ServiceConnection;
import android.content.pm.ApplicationInfo;
import android.content.pm.ConfigurationInfo;
import android.content.pm.PackageManager;
import android.content.res.Resources;
import android.os.Binder;
import android.os.Bundle;
import android.os.Handler;
import android.os.HandlerThread;
import android.os.IBinder;
import android.os.Message;
import android.os.Messenger;
import android.os.Parcel;
import android.os.RemoteException;
import android.os.SystemClock;
import android.platform.test.annotations.RestrictedBuildTest;
import android.provider.DeviceConfig;
import android.provider.Settings;
import android.server.wm.settings.SettingsSession;
import android.support.test.uiautomator.UiDevice;
import android.test.InstrumentationTestCase;
import android.util.ArrayMap;
import android.util.ArraySet;
import android.util.Log;
import android.util.Pair;

import androidx.test.filters.LargeTest;

import com.android.compatibility.common.util.AmMonitor;
import com.android.compatibility.common.util.ShellIdentityUtils;
import com.android.compatibility.common.util.SystemUtil;

import java.io.IOException;
import java.util.ArrayList;
import java.util.HashSet;
import java.util.List;
import java.util.Set;
import java.util.concurrent.CountDownLatch;
import java.util.concurrent.LinkedBlockingQueue;
import java.util.concurrent.TimeUnit;
import java.util.function.Predicate;
import java.util.function.Supplier;

public class ActivityManagerTest extends InstrumentationTestCase {
    private static final String TAG = ActivityManagerTest.class.getSimpleName();
    private static final String STUB_PACKAGE_NAME = "android.app.stubs";
    private static final int WAITFOR_MSEC = 5000;
    private static final String SERVICE_NAME = "android.app.stubs.MockService";
    private static final int WAIT_TIME = 2000;
    // A secondary test activity from another APK.
    static final String SIMPLE_PACKAGE_NAME = "com.android.cts.launcherapps.simpleapp";
    static final String SIMPLE_ACTIVITY = ".SimpleActivity";
    static final String SIMPLE_ACTIVITY_IMMEDIATE_EXIT = ".SimpleActivityImmediateExit";
    static final String SIMPLE_ACTIVITY_CHAIN_EXIT = ".SimpleActivityChainExit";
    static final String SIMPLE_RECEIVER = ".SimpleReceiver";
    static final String SIMPLE_REMOTE_RECEIVER = ".SimpleRemoteReceiver";
    // The action sent back by the SIMPLE_APP after a restart.
    private static final String ACTIVITY_LAUNCHED_ACTION =
            "com.android.cts.launchertests.LauncherAppsTests.LAUNCHED_ACTION";
    // The action sent back by the SIMPLE_APP_IMMEDIATE_EXIT when it terminates.
    private static final String ACTIVITY_EXIT_ACTION =
            "com.android.cts.launchertests.LauncherAppsTests.EXIT_ACTION";
    // The action sent back by the SIMPLE_APP_CHAIN_EXIT when the task chain ends. 
    private static final String ACTIVITY_CHAIN_EXIT_ACTION =
            "com.android.cts.launchertests.LauncherAppsTests.CHAIN_EXIT_ACTION";
    // The action sent to identify the time track info.
    private static final String ACTIVITY_TIME_TRACK_INFO = "com.android.cts.TIME_TRACK_INFO";

    private static final String PACKAGE_NAME_APP1 = "com.android.app1";
    private static final String PACKAGE_NAME_APP2 = "com.android.app2";
    private static final String PACKAGE_NAME_APP3 = "com.android.app3";

    private static final String CANT_SAVE_STATE_1_PACKAGE_NAME = "com.android.test.cantsavestate1";
    private static final String ACTION_FINISH = "com.android.test.action.FINISH";

    private static final String MCC_TO_UPDATE = "987";
    private static final String MNC_TO_UPDATE = "654";
    private static final String SHELL_COMMAND_GET_CONFIG = "am get-config";
    private static final String SHELL_COMMAND_RESULT_CONFIG_NAME_MCC = "mcc";
    private static final String SHELL_COMMAND_RESULT_CONFIG_NAME_MNC = "mnc";

    // Return states of the ActivityReceiverFilter.
    public static final int RESULT_PASS = 1;
    public static final int RESULT_FAIL = 2;
    public static final int RESULT_TIMEOUT = 3;

    private Context mTargetContext;
    private ActivityManager mActivityManager;
    private PackageManager mPackageManager;
    private Intent mIntent;
    private List<Activity> mStartedActivityList;
    private int mErrorProcessID;
    private Instrumentation mInstrumentation;

    @Override
    protected void setUp() throws Exception {
        super.setUp();
        mInstrumentation = getInstrumentation();
        mTargetContext = mInstrumentation.getTargetContext();
        mActivityManager = (ActivityManager) mInstrumentation.getContext()
                .getSystemService(Context.ACTIVITY_SERVICE);
        mPackageManager = mInstrumentation.getContext().getPackageManager();
        mStartedActivityList = new ArrayList<Activity>();
        mErrorProcessID = -1;
        startSubActivity(ScreenOnActivity.class);
    }

    @Override
    protected void tearDown() throws Exception {
        super.tearDown();
        if (mIntent != null) {
            mInstrumentation.getContext().stopService(mIntent);
        }
        for (int i = 0; i < mStartedActivityList.size(); i++) {
            mStartedActivityList.get(i).finish();
        }
        if (mErrorProcessID != -1) {
            android.os.Process.killProcess(mErrorProcessID);
        }
    }

    public void testGetRecentTasks() throws Exception {
        int maxNum = 0;
        int flags = 0;

        List<RecentTaskInfo> recentTaskList;
        // Test parameter: maxNum is set to 0
        recentTaskList = mActivityManager.getRecentTasks(maxNum, flags);
        assertNotNull(recentTaskList);
        assertTrue(recentTaskList.size() == 0);
        // Test parameter: maxNum is set to 50
        maxNum = 50;
        recentTaskList = mActivityManager.getRecentTasks(maxNum, flags);
        assertNotNull(recentTaskList);
        // start recent1_activity.
        startSubActivity(ActivityManagerRecentOneActivity.class);
        Thread.sleep(WAIT_TIME);
        // start recent2_activity
        startSubActivity(ActivityManagerRecentTwoActivity.class);
        Thread.sleep(WAIT_TIME);
        /*
         * assert both recent1_activity and recent2_activity exist in the recent
         * tasks list. Moreover,the index of the recent2_activity is smaller
         * than the index of recent1_activity
         */
        recentTaskList = mActivityManager.getRecentTasks(maxNum, flags);
        int indexRecentOne = -1;
        int indexRecentTwo = -1;
        int i = 0;
        for (RecentTaskInfo rti : recentTaskList) {
            if (rti.baseIntent.getComponent().getClassName().equals(
                    ActivityManagerRecentOneActivity.class.getName())) {
                indexRecentOne = i;
            } else if (rti.baseIntent.getComponent().getClassName().equals(
                    ActivityManagerRecentTwoActivity.class.getName())) {
                indexRecentTwo = i;
            }
            i++;
        }
        assertTrue(indexRecentOne != -1 && indexRecentTwo != -1);
        assertTrue(indexRecentTwo < indexRecentOne);

        try {
            mActivityManager.getRecentTasks(-1, 0);
            fail("Should throw IllegalArgumentException");
        } catch (IllegalArgumentException e) {
            // expected exception
        }
    }

    public void testGetRecentTasksLimitedToCurrentAPK() throws Exception {
        int maxNum = 0;
        int flags = 0;

        // Check the number of tasks at this time.
        List<RecentTaskInfo>  recentTaskList;
        recentTaskList = mActivityManager.getRecentTasks(maxNum, flags);
        int numberOfEntriesFirstRun = recentTaskList.size();

        // Start another activity from another APK.
        Intent intent = new Intent(Intent.ACTION_MAIN);
        intent.setClassName(SIMPLE_PACKAGE_NAME, SIMPLE_PACKAGE_NAME + SIMPLE_ACTIVITY);
        intent.addFlags(Intent.FLAG_ACTIVITY_NEW_TASK);
        ActivityReceiverFilter receiver = new ActivityReceiverFilter(ACTIVITY_LAUNCHED_ACTION);
        mTargetContext.startActivity(intent);

        // Make sure the activity has really started.
        assertEquals(RESULT_PASS, receiver.waitForActivity());
        receiver.close();

        // There shouldn't be any more tasks in this list at this time.
        recentTaskList = mActivityManager.getRecentTasks(maxNum, flags);
        int numberOfEntriesSecondRun = recentTaskList.size();
        assertTrue(numberOfEntriesSecondRun == numberOfEntriesFirstRun);

        // Tell the activity to finalize.
        intent.addFlags(Intent.FLAG_ACTIVITY_SINGLE_TOP);
        intent.putExtra("finish", true);
        mTargetContext.startActivity(intent);
    }

    // The receiver filter needs to be instantiated with the command to filter for before calling
    // startActivity.
    private class ActivityReceiverFilter extends BroadcastReceiver {
        // The activity we want to filter for.
        private String mActivityToFilter;
        private int result = RESULT_TIMEOUT;
        public long mTimeUsed = 0;
        private static final int TIMEOUT_IN_MS = 5000;

        // Create the filter with the intent to look for.
        public ActivityReceiverFilter(String activityToFilter) {
            mActivityToFilter = activityToFilter;
            IntentFilter filter = new IntentFilter();
            filter.addAction(mActivityToFilter);
            mInstrumentation.getTargetContext().registerReceiver(this, filter);
        }

        // Turn off the filter.
        public void close() {
            mInstrumentation.getTargetContext().unregisterReceiver(this);
        }

        @Override
        public void onReceive(Context context, Intent intent) {
            if (intent.getAction().equals(mActivityToFilter)) {
                synchronized(this) {
                   result = RESULT_PASS;
                   if (mActivityToFilter.equals(ACTIVITY_TIME_TRACK_INFO)) {
                       mTimeUsed = intent.getExtras().getLong(
                               ActivityOptions.EXTRA_USAGE_TIME_REPORT);
                   }
                   notifyAll();
                }
            }
        }

        public int waitForActivity() {
            synchronized(this) {
                try {
                    wait(TIMEOUT_IN_MS);
                } catch (InterruptedException e) {
                }
            }
            return result;
        }
    }

    private final <T extends Activity> void startSubActivity(Class<T> activityClass) {
        final Instrumentation.ActivityResult result = new ActivityResult(0, new Intent());
        final ActivityMonitor monitor = new ActivityMonitor(activityClass.getName(), result, false);
        mInstrumentation.addMonitor(monitor);
        launchActivity(STUB_PACKAGE_NAME, activityClass, null);
        mStartedActivityList.add(monitor.waitForActivity());
    }

    public void testGetRunningTasks() {
        // Test illegal parameter
        List<RunningTaskInfo> runningTaskList;
        runningTaskList = mActivityManager.getRunningTasks(-1);
        assertTrue(runningTaskList.size() == 0);

        runningTaskList = mActivityManager.getRunningTasks(0);
        assertTrue(runningTaskList.size() == 0);

        runningTaskList = mActivityManager.getRunningTasks(20);
        int taskSize = runningTaskList.size();
        assertTrue(taskSize >= 0 && taskSize <= 20);

        // start recent1_activity.
        startSubActivity(ActivityManagerRecentOneActivity.class);
        // start recent2_activity
        startSubActivity(ActivityManagerRecentTwoActivity.class);

        /*
         * assert both recent1_activity and recent2_activity exist in the
         * running tasks list. Moreover,the index of the recent2_activity is
         * smaller than the index of recent1_activity
         */
        runningTaskList = mActivityManager.getRunningTasks(20);
        int indexRecentOne = -1;
        int indexRecentTwo = -1;
        int i = 0;
        for (RunningTaskInfo rti : runningTaskList) {
            if (rti.baseActivity.getClassName().equals(
                    ActivityManagerRecentOneActivity.class.getName())) {
                indexRecentOne = i;
            } else if (rti.baseActivity.getClassName().equals(
                    ActivityManagerRecentTwoActivity.class.getName())) {
                indexRecentTwo = i;
            }
            i++;
        }
        assertTrue(indexRecentOne != -1 && indexRecentTwo != -1);
        assertTrue(indexRecentTwo < indexRecentOne);
    }

    public void testGetRunningServices() throws Exception {
        // Test illegal parameter
        List<RunningServiceInfo> runningServiceInfo;
        runningServiceInfo = mActivityManager.getRunningServices(-1);
        assertTrue(runningServiceInfo.isEmpty());

        runningServiceInfo = mActivityManager.getRunningServices(0);
        assertTrue(runningServiceInfo.isEmpty());

        runningServiceInfo = mActivityManager.getRunningServices(5);
        assertTrue(runningServiceInfo.size() <= 5);

        Intent intent = new Intent();
        intent.setClass(mInstrumentation.getTargetContext(), MockService.class);
        intent.putExtra(MockService.EXTRA_NO_STOP, true);
        mInstrumentation.getTargetContext().startService(intent);
        MockService.waitForStart(WAIT_TIME);

        runningServiceInfo = mActivityManager.getRunningServices(Integer.MAX_VALUE);
        boolean foundService = false;
        for (RunningServiceInfo rs : runningServiceInfo) {
            if (rs.service.getClassName().equals(SERVICE_NAME)) {
                foundService = true;
                break;
            }
        }
        assertTrue(foundService);
        MockService.prepareDestroy();
        mTargetContext.stopService(intent);
        boolean destroyed = MockService.waitForDestroy(WAIT_TIME);
        assertTrue(destroyed);
    }

    private void executeAndLogShellCommand(String cmd) throws IOException {
        final UiDevice uiDevice = UiDevice.getInstance(mInstrumentation);
        final String output = uiDevice.executeShellCommand(cmd);
        Log.d(TAG, "executed[" + cmd + "]; output[" + output.trim() + "]");
    }

    private String executeShellCommand(String cmd) throws IOException {
        final UiDevice uiDevice = UiDevice.getInstance(mInstrumentation);
        return uiDevice.executeShellCommand(cmd).trim();
    }

    private void setForcedAppStandby(String packageName, boolean enabled) throws IOException {
        final StringBuilder cmdBuilder = new StringBuilder("appops set ")
                .append(packageName)
                .append(" RUN_ANY_IN_BACKGROUND ")
                .append(enabled ? "ignore" : "allow");
        executeAndLogShellCommand(cmdBuilder.toString());
    }

    public void testIsBackgroundRestricted() throws IOException {
        // This instrumentation runs in the target package's uid.
        final Context targetContext = mInstrumentation.getTargetContext();
        final String targetPackage = targetContext.getPackageName();
        final ActivityManager am = targetContext.getSystemService(ActivityManager.class);
        setForcedAppStandby(targetPackage, true);
        assertTrue(am.isBackgroundRestricted());
        setForcedAppStandby(targetPackage, false);
        assertFalse(am.isBackgroundRestricted());
    }

    public void testGetMemoryInfo() {
        ActivityManager.MemoryInfo outInfo = new ActivityManager.MemoryInfo();
        mActivityManager.getMemoryInfo(outInfo);
        assertTrue(outInfo.lowMemory == (outInfo.availMem <= outInfo.threshold));
    }

    public void testGetRunningAppProcesses() throws Exception {
        List<RunningAppProcessInfo> list = mActivityManager.getRunningAppProcesses();
        assertNotNull(list);
        final String SYSTEM_PROCESS = "system";
        boolean hasSystemProcess = false;
        // The package name is also the default name for the application process
        final String TEST_PROCESS = STUB_PACKAGE_NAME;
        boolean hasTestProcess = false;
        for (RunningAppProcessInfo ra : list) {
            if (ra.processName.equals(SYSTEM_PROCESS)) {
                hasSystemProcess = true;

                // Make sure the importance is a sane value.
                assertTrue(ra.importance >= RunningAppProcessInfo.IMPORTANCE_FOREGROUND);
                assertTrue(ra.importance < RunningAppProcessInfo.IMPORTANCE_GONE);
            } else if (ra.processName.equals(TEST_PROCESS)) {
                hasTestProcess = true;
            }
        }

        // For security reasons the system process is not exposed.
        assertFalse(hasSystemProcess);
        assertTrue(hasTestProcess);

        for (RunningAppProcessInfo ra : list) {
            if (ra.processName.equals("android.app.stubs:remote")) {
                fail("should be no process named android.app.stubs:remote");
            }
        }
        // start a new process
        // XXX would be a lot cleaner to bind instead of start.
        mIntent = new Intent("android.app.REMOTESERVICE");
        mIntent.setPackage("android.app.stubs");
        mInstrumentation.getTargetContext().startService(mIntent);
        Thread.sleep(WAITFOR_MSEC);

        List<RunningAppProcessInfo> listNew = mActivityManager.getRunningAppProcesses();
        mInstrumentation.getTargetContext().stopService(mIntent);

        for (RunningAppProcessInfo ra : listNew) {
            if (ra.processName.equals("android.app.stubs:remote")) {
                return;
            }
        }
        fail("android.app.stubs:remote process should be available");
    }

    public void testGetMyMemoryState() {
        final RunningAppProcessInfo ra = new RunningAppProcessInfo();
        ActivityManager.getMyMemoryState(ra);

        assertEquals(android.os.Process.myUid(), ra.uid);

        // When an instrumentation test is running, the importance is high.
        assertEquals(RunningAppProcessInfo.IMPORTANCE_FOREGROUND, ra.importance);
    }

    public void testGetProcessInErrorState() throws Exception {
        List<ActivityManager.ProcessErrorStateInfo> errList = null;
        errList = mActivityManager.getProcessesInErrorState();
    }

    public void testGetDeviceConfigurationInfo() {
        ConfigurationInfo conInf = mActivityManager.getDeviceConfigurationInfo();
        assertNotNull(conInf);
    }

    public void testUpdateMccMncConfiguration() throws Exception {
        if (!mPackageManager.hasSystemFeature(PackageManager.FEATURE_TELEPHONY)) {
            Log.i(TAG, "testUpdateMccMncConfiguration skipped: no telephony available");
            return;
        }

        // Store the original mcc mnc to set back
        String[] mccMncConfigOriginal = new String[2];
        // Store other configs to check they won't be affected
        Set<String> otherConfigsOriginal = new HashSet<>();
        getMccMncConfigsAndOthers(mccMncConfigOriginal, otherConfigsOriginal);

        String[] mccMncConfigToUpdate = new String[] {MCC_TO_UPDATE, MNC_TO_UPDATE};
        boolean success = ShellIdentityUtils.invokeMethodWithShellPermissions(mActivityManager,
                (am) -> am.updateMccMncConfiguration(mccMncConfigToUpdate[0],
                        mccMncConfigToUpdate[1]));

        if (success) {
            String[] mccMncConfigUpdated = new String[2];
            Set<String> otherConfigsUpdated = new HashSet<>();
            getMccMncConfigsAndOthers(mccMncConfigUpdated, otherConfigsUpdated);
            // Check the mcc mnc are updated as expected
            assertArrayEquals(mccMncConfigToUpdate, mccMncConfigUpdated);
            // Check other configs are not changed
            assertEquals(otherConfigsOriginal, otherConfigsUpdated);
        }

        // Set mcc mnc configs back in the end of the test
        ShellIdentityUtils.invokeMethodWithShellPermissions(mActivityManager,
                (am) -> am.updateMccMncConfiguration(mccMncConfigOriginal[0],
                        mccMncConfigOriginal[1]));
    }

    private void getMccMncConfigsAndOthers(String[] mccMncConfigs, Set<String> otherConfigs)
            throws Exception {
        String[] configs = SystemUtil.runShellCommand(
                mInstrumentation, SHELL_COMMAND_GET_CONFIG).split(" |\\-");
        for (String config : configs) {
            if (config.startsWith(SHELL_COMMAND_RESULT_CONFIG_NAME_MCC)) {
                mccMncConfigs[0] = config.substring(
                        SHELL_COMMAND_RESULT_CONFIG_NAME_MCC.length());
            } else if (config.startsWith(SHELL_COMMAND_RESULT_CONFIG_NAME_MNC)) {
                mccMncConfigs[1] = config.substring(
                        SHELL_COMMAND_RESULT_CONFIG_NAME_MNC.length());
            } else {
                otherConfigs.add(config);
            }
        }
    }

    /**
     * Simple test for {@link ActivityManager#isUserAMonkey()} - verifies its false.
     *
     * TODO: test positive case
     */
    public void testIsUserAMonkey() {
        assertFalse(ActivityManager.isUserAMonkey());
    }

    /**
     * Verify that {@link ActivityManager#isRunningInTestHarness()} is false.
     */
    @RestrictedBuildTest
    public void testIsRunningInTestHarness() {
        assertFalse("isRunningInTestHarness must be false in production builds",
                ActivityManager.isRunningInTestHarness());
    }

    /**
     * Go back to the home screen since running applications can interfere with application
     * lifetime tests.
     */
    private void launchHome() throws Exception {
        if (!noHomeScreen()) {
            Intent intent = new Intent(Intent.ACTION_MAIN);
            intent.addCategory(Intent.CATEGORY_HOME);
            intent.setFlags(Intent.FLAG_ACTIVITY_NEW_TASK);
            mTargetContext.startActivity(intent);
            Thread.sleep(WAIT_TIME);
        }
    }

   /**
    * Gets the value of com.android.internal.R.bool.config_noHomeScreen.
    * @return true if no home screen is supported, false otherwise.
    */
   private boolean noHomeScreen() {
       try {
           return getInstrumentation().getContext().getResources().getBoolean(
                   Resources.getSystem().getIdentifier("config_noHomeScreen", "bool", "android"));
       } catch (Resources.NotFoundException e) {
           // Assume there's a home screen.
           return false;
       }
    }

    /**
     * Verify that the TimeTrackingAPI works properly when starting and ending an activity.
     */
    public void testTimeTrackingAPI_SimpleStartExit() throws Exception {
        launchHome();
        // Prepare to start an activity from another APK.
        Intent intent = new Intent(Intent.ACTION_MAIN);
        intent.setClassName(SIMPLE_PACKAGE_NAME,
                SIMPLE_PACKAGE_NAME + SIMPLE_ACTIVITY_IMMEDIATE_EXIT);
        intent.addFlags(Intent.FLAG_ACTIVITY_NEW_TASK);

        // Prepare the time receiver action.
        Context context = mInstrumentation.getTargetContext();
        ActivityOptions options = ActivityOptions.makeBasic();
        Intent receiveIntent = new Intent(ACTIVITY_TIME_TRACK_INFO);
        options.requestUsageTimeReport(PendingIntent.getBroadcast(context, 0, receiveIntent,
                PendingIntent.FLAG_CANCEL_CURRENT | PendingIntent.FLAG_MUTABLE));

        // The application finished tracker.
        ActivityReceiverFilter appEndReceiver = new ActivityReceiverFilter(ACTIVITY_EXIT_ACTION);

        // The filter for the time event.
        ActivityReceiverFilter timeReceiver = new ActivityReceiverFilter(ACTIVITY_TIME_TRACK_INFO);

        // Run the activity.
        mTargetContext.startActivity(intent, options.toBundle());

        // Wait until it finishes and end the reciever then.
        assertEquals(RESULT_PASS, appEndReceiver.waitForActivity());
        appEndReceiver.close();

        if (!noHomeScreen()) {
            // At this time the timerReceiver should not fire, even though the activity has shut
            // down, because we are back to the home screen. Going to the home screen does not
            // qualify as the user leaving the activity's flow. The time tracking is considered
            // complete only when the user switches to another activity that is not part of the
            // tracked flow.
            assertEquals(RESULT_TIMEOUT, timeReceiver.waitForActivity());
            assertTrue(timeReceiver.mTimeUsed == 0);
        } else {
            // With platforms that have no home screen, focus is returned to something else that is
            // considered a completion of the tracked activity flow, and hence time tracking is
            // triggered.
            assertEquals(RESULT_PASS, timeReceiver.waitForActivity());
        }

        // Issuing now another activity will trigger the timing information release.
        final Intent dummyIntent = new Intent(context, MockApplicationActivity.class);
        dummyIntent.addFlags(Intent.FLAG_ACTIVITY_NEW_TASK);
        final Activity activity = mInstrumentation.startActivitySync(dummyIntent);

        // Wait until it finishes and end the reciever then.
        assertEquals(RESULT_PASS, timeReceiver.waitForActivity());
        timeReceiver.close();
        assertTrue(timeReceiver.mTimeUsed != 0);
    }

    public void testHomeVisibilityListener() throws Exception {
        LinkedBlockingQueue<Boolean> currentHomeScreenVisibility = new LinkedBlockingQueue<>(2);
        HomeVisibilityListener homeVisibilityListener = new HomeVisibilityListener() {
            @Override
            public void onHomeVisibilityChanged(boolean isHomeActivityVisible) {
                currentHomeScreenVisibility.offer(isHomeActivityVisible);
            }
        };
        launchHome();
        ShellIdentityUtils.invokeMethodWithShellPermissionsNoReturn(mActivityManager,
                (am) -> am.addHomeVisibilityListener(Runnable::run, homeVisibilityListener));

        try {
            // Make sure we got the first notification that the home screen is visible.
            assertTrue(currentHomeScreenVisibility.poll(WAIT_TIME, TimeUnit.MILLISECONDS));
            // Launch a basic activity to obscure the home screen.
            Intent intent = new Intent(Intent.ACTION_MAIN);
            intent.setClassName(SIMPLE_PACKAGE_NAME, SIMPLE_PACKAGE_NAME + SIMPLE_ACTIVITY);
            intent.addFlags(Intent.FLAG_ACTIVITY_NEW_TASK);
            mTargetContext.startActivity(intent);

            // Make sure the observer reports the home screen as no longer visible
            assertFalse(currentHomeScreenVisibility.poll(WAIT_TIME, TimeUnit.MILLISECONDS));
        } finally {
            ShellIdentityUtils.invokeMethodWithShellPermissionsNoReturn(mActivityManager,
                    (am) -> am.removeHomeVisibilityListener(homeVisibilityListener));
        }
    }

    /**
     * Verify that the TimeTrackingAPI works properly when switching away from the monitored task.
     */
    public void testTimeTrackingAPI_SwitchAwayTriggers() throws Exception {
        launchHome();

        // Prepare to start an activity from another APK.
        Intent intent = new Intent(Intent.ACTION_MAIN);
        intent.setClassName(SIMPLE_PACKAGE_NAME, SIMPLE_PACKAGE_NAME + SIMPLE_ACTIVITY);
        intent.addFlags(Intent.FLAG_ACTIVITY_NEW_TASK);

        // Prepare the time receiver action.
        Context context = mInstrumentation.getTargetContext();
        ActivityOptions options = ActivityOptions.makeBasic();
        Intent receiveIntent = new Intent(ACTIVITY_TIME_TRACK_INFO);
        options.requestUsageTimeReport(PendingIntent.getBroadcast(context, 0, receiveIntent,
                    PendingIntent.FLAG_CANCEL_CURRENT | PendingIntent.FLAG_MUTABLE));

        // The application started tracker.
        ActivityReceiverFilter appStartedReceiver = new ActivityReceiverFilter(
                ACTIVITY_LAUNCHED_ACTION);

        // The filter for the time event.
        ActivityReceiverFilter timeReceiver = new ActivityReceiverFilter(ACTIVITY_TIME_TRACK_INFO);

        // Run the activity.
        mTargetContext.startActivity(intent, options.toBundle());

        // Wait until it finishes and end the reciever then.
        assertEquals(RESULT_PASS, appStartedReceiver.waitForActivity());
        appStartedReceiver.close();

        // At this time the timerReceiver should not fire since our app is running.
        assertEquals(RESULT_TIMEOUT, timeReceiver.waitForActivity());
        assertTrue(timeReceiver.mTimeUsed == 0);

        // Starting now another activity will put ours into the back hence releasing the timing.
        final Intent dummyIntent = new Intent(context, MockApplicationActivity.class);
        dummyIntent.addFlags(Intent.FLAG_ACTIVITY_NEW_TASK);
        final Activity activity = mInstrumentation.startActivitySync(dummyIntent);

        // Wait until it finishes and end the reciever then.
        assertEquals(RESULT_PASS, timeReceiver.waitForActivity());
        timeReceiver.close();
        assertTrue(timeReceiver.mTimeUsed != 0);
    }

    /**
     * Verify that the TimeTrackingAPI works properly when handling an activity chain gets started
     * and ended.
     */
    public void testTimeTrackingAPI_ChainedActivityExit() throws Exception {
        launchHome();
        // Prepare to start an activity from another APK.
        Intent intent = new Intent(Intent.ACTION_MAIN);
        intent.setClassName(SIMPLE_PACKAGE_NAME,
                SIMPLE_PACKAGE_NAME + SIMPLE_ACTIVITY_CHAIN_EXIT);
        intent.addFlags(Intent.FLAG_ACTIVITY_NEW_TASK);

        // Prepare the time receiver action.
        Context context = mInstrumentation.getTargetContext();
        ActivityOptions options = ActivityOptions.makeBasic();
        Intent receiveIntent = new Intent(ACTIVITY_TIME_TRACK_INFO);
        options.requestUsageTimeReport(PendingIntent.getBroadcast(context, 0, receiveIntent,
                    PendingIntent.FLAG_CANCEL_CURRENT | PendingIntent.FLAG_MUTABLE));

        // The application finished tracker.
        ActivityReceiverFilter appEndReceiver = new ActivityReceiverFilter(
                ACTIVITY_CHAIN_EXIT_ACTION);

        // The filter for the time event.
        ActivityReceiverFilter timeReceiver = new ActivityReceiverFilter(ACTIVITY_TIME_TRACK_INFO);

        // Run the activity.
        mTargetContext.startActivity(intent, options.toBundle());

        // Wait until it finishes and end the reciever then.
        assertEquals(RESULT_PASS, appEndReceiver.waitForActivity());
        appEndReceiver.close();

        if (!noHomeScreen()) {
            // At this time the timerReceiver should not fire, even though the activity has shut
            // down, because we are back to the home screen. Going to the home screen does not
            // qualify as the user leaving the activity's flow. The time tracking is considered
            // complete only when the user switches to another activity that is not part of the
            // tracked flow.
            assertEquals(RESULT_TIMEOUT, timeReceiver.waitForActivity());
            assertTrue(timeReceiver.mTimeUsed == 0);
        } else {
            // With platforms that have no home screen, focus is returned to something else that is
            // considered a completion of the tracked activity flow, and hence time tracking is
            // triggered.
            assertEquals(RESULT_PASS, timeReceiver.waitForActivity());
        }

        // Issue another activity so that the timing information gets released.
        final Intent dummyIntent = new Intent(context, MockApplicationActivity.class);
        dummyIntent.addFlags(Intent.FLAG_ACTIVITY_NEW_TASK);
        final Activity activity = mInstrumentation.startActivitySync(dummyIntent);

        // Wait until it finishes and end the reciever then.
        assertEquals(RESULT_PASS, timeReceiver.waitForActivity());
        timeReceiver.close();
        assertTrue(timeReceiver.mTimeUsed != 0);
    }

    /**
     * Verify that after force-stopping a package which has a foreground task contains multiple
     * activities, the process of the package should not be alive (restarted).
     */
    public void testForceStopPackageWontRestartProcess() throws Exception {
        // Ensure that there are no remaining component records of the test app package.
        SystemUtil.runWithShellPermissionIdentity(
                () -> mActivityManager.forceStopPackage(SIMPLE_PACKAGE_NAME));
        ActivityReceiverFilter appStartedReceiver = new ActivityReceiverFilter(
                ACTIVITY_LAUNCHED_ACTION);
        // Start an activity of another APK.
        Intent intent = new Intent();
        intent.setClassName(SIMPLE_PACKAGE_NAME, SIMPLE_PACKAGE_NAME + SIMPLE_ACTIVITY);
        intent.addFlags(Intent.FLAG_ACTIVITY_NEW_TASK);
        mTargetContext.startActivity(intent);
        assertEquals(RESULT_PASS, appStartedReceiver.waitForActivity());

        // Start a new activity in the same task. Here adds an action to make a different to intent
        // filter comparison so another same activity will be created.
        intent.setAction(Intent.ACTION_MAIN);
        mTargetContext.startActivity(intent);
        assertEquals(RESULT_PASS, appStartedReceiver.waitForActivity());
        appStartedReceiver.close();

        // Wait for the first activity to stop so its ActivityRecord.haveState will be true. The
        // condition is required to keep the activity record when its process is died.
        Thread.sleep(WAIT_TIME);

        // The package name is also the default name for the activity process.
        final String testProcess = SIMPLE_PACKAGE_NAME;
        Predicate<RunningAppProcessInfo> processNamePredicate =
                runningApp -> testProcess.equals(runningApp.processName);

        List<RunningAppProcessInfo> runningApps = SystemUtil.callWithShellPermissionIdentity(
                () -> mActivityManager.getRunningAppProcesses());
        assertTrue("Process " + testProcess + " should be found in running process list",
                runningApps.stream().anyMatch(processNamePredicate));

        runningApps = SystemUtil.callWithShellPermissionIdentity(() -> {
            mActivityManager.forceStopPackage(SIMPLE_PACKAGE_NAME);
            // Wait awhile (process starting may be asynchronous) to verify if the process is
            // started again unexpectedly.
            Thread.sleep(WAIT_TIME);
            return mActivityManager.getRunningAppProcesses();
        });

        assertFalse("Process " + testProcess + " should not be alive after force-stop",
                runningApps.stream().anyMatch(processNamePredicate));
    }

    /**
     * This test is to verify that devices are patched with the fix in b/119327603 for b/115384617.
     */
    public void testIsAppForegroundRemoved() throws ClassNotFoundException {
       try {
           Class.forName("android.app.IActivityManager").getDeclaredMethod(
                   "isAppForeground", int.class);
           fail("IActivityManager.isAppForeground() API should not be available.");
       } catch (NoSuchMethodException e) {
           // Patched devices should throw this exception since isAppForeground is removed.
       }
    }

    /**
     * This test verifies the self-induced ANR by ActivityManager.appNotResponding().
     */
    public void testAppNotResponding() throws Exception {
        // Setup the ANR monitor
        AmMonitor monitor = new AmMonitor(mInstrumentation,
                new String[]{AmMonitor.WAIT_FOR_CRASHED});

        // Now tell it goto ANR
        CommandReceiver.sendCommand(mTargetContext, CommandReceiver.COMMAND_SELF_INDUCED_ANR,
                PACKAGE_NAME_APP1, PACKAGE_NAME_APP1, 0, null);

        try {

            // Verify we got the ANR
            assertTrue(monitor.waitFor(AmMonitor.WAIT_FOR_EARLY_ANR, WAITFOR_MSEC));

            // Just kill the test app
            monitor.sendCommand(AmMonitor.CMD_KILL);
        } finally {
            // clean up
            monitor.finish();
            SystemUtil.runWithShellPermissionIdentity(() -> {
                mActivityManager.forceStopPackage(PACKAGE_NAME_APP1);
            });
        }
    }

    /*
     * Verifies the {@link android.app.ActivityManager#killProcessesWhenImperceptible}.
     */
    public void testKillingPidsOnImperceptible() throws Exception {
        // Start remote service process
        final String remoteProcessName = STUB_PACKAGE_NAME + ":remote";
        Intent remoteIntent = new Intent("android.app.REMOTESERVICE");
        remoteIntent.setPackage(STUB_PACKAGE_NAME);
        mTargetContext.startService(remoteIntent);
        Thread.sleep(WAITFOR_MSEC);

        RunningAppProcessInfo remote = getRunningAppProcessInfo(remoteProcessName);
        assertNotNull(remote);

        ActivityReceiverFilter appStartedReceiver = new ActivityReceiverFilter(
                ACTIVITY_LAUNCHED_ACTION);
        boolean disabled = "0".equals(executeShellCommand("cmd deviceidle enabled light"));
        try {
            if (disabled) {
                executeAndLogShellCommand("cmd deviceidle enable light");
            }
            final Intent intent = new Intent(Intent.ACTION_MAIN);
            intent.setClassName(SIMPLE_PACKAGE_NAME, SIMPLE_PACKAGE_NAME + SIMPLE_ACTIVITY);
            intent.addFlags(Intent.FLAG_ACTIVITY_NEW_TASK);
            mTargetContext.startActivity(intent);
            assertEquals(RESULT_PASS, appStartedReceiver.waitForActivity());

            RunningAppProcessInfo proc = getRunningAppProcessInfo(SIMPLE_PACKAGE_NAME);
            assertNotNull(proc);

            final String reason = "cts";

            try {
                mActivityManager.killProcessesWhenImperceptible(new int[]{proc.pid}, reason);
                fail("Shouldn't have the permission");
            } catch (SecurityException e) {
                // expected
            }

            final long defaultWaitForKillTimeout = 5_000;

            // Keep the device awake
            toggleScreenOn(true);

            // Kill the remote process
            SystemUtil.runWithShellPermissionIdentity(() -> {
                mActivityManager.killProcessesWhenImperceptible(new int[]{remote.pid}, reason);
            });

            // Kill the activity process
            SystemUtil.runWithShellPermissionIdentity(() -> {
                mActivityManager.killProcessesWhenImperceptible(new int[]{proc.pid}, reason);
            });

            // The processes should be still alive because device isn't in idle
            assertFalse(waitUntilTrue(defaultWaitForKillTimeout, () -> isProcessGone(
                    remote.pid, remoteProcessName)));
            assertFalse(waitUntilTrue(defaultWaitForKillTimeout, () -> isProcessGone(
                    proc.pid, SIMPLE_PACKAGE_NAME)));

            // force device idle
            toggleScreenOn(false);
            triggerIdle(true);

            // Now the remote process should have been killed.
            assertTrue(waitUntilTrue(defaultWaitForKillTimeout, () -> isProcessGone(
                    remote.pid, remoteProcessName)));

            // The activity process should be still alive because it's is on the top (perceptible)
            assertFalse(waitUntilTrue(defaultWaitForKillTimeout, () -> isProcessGone(
                    proc.pid, SIMPLE_PACKAGE_NAME)));

            triggerIdle(false);
            // Toogle screen ON
            toggleScreenOn(true);

            // Now launch home
            executeAndLogShellCommand("input keyevent KEYCODE_HOME");

            // force device idle again
            toggleScreenOn(false);
            triggerIdle(true);

            // Now the activity process should be gone.
            assertTrue(waitUntilTrue(defaultWaitForKillTimeout, () -> isProcessGone(
                    proc.pid, SIMPLE_PACKAGE_NAME)));

        } finally {
            // Clean up code
            triggerIdle(false);
            toggleScreenOn(true);
            appStartedReceiver.close();
            mTargetContext.stopService(remoteIntent);

            if (disabled) {
                executeAndLogShellCommand("cmd deviceidle disable light");
            }
            SystemUtil.runWithShellPermissionIdentity(() -> {
                mActivityManager.forceStopPackage(SIMPLE_PACKAGE_NAME);
            });
        }
    }

    /**
     * Verifies the system will kill app's child processes if they are using excessive cpu
     */
    @LargeTest
    public void testKillingAppChildProcess() throws Exception {
        final long powerCheckInterval = 5 * 1000;
        final long processGoneTimeout = powerCheckInterval * 4;
        final int waitForSec = 5 * 1000;
        final String activityManagerConstants = "activity_manager_constants";

        final SettingsSession<String> amSettings = new SettingsSession<>(
                Settings.Global.getUriFor(activityManagerConstants),
                Settings.Global::getString, Settings.Global::putString);

        final ApplicationInfo ai = mTargetContext.getPackageManager()
                .getApplicationInfo(PACKAGE_NAME_APP1, 0);
        final WatchUidRunner watcher = new WatchUidRunner(mInstrumentation, ai.uid, waitForSec);

        try {
            // Shorten the power check intervals
            amSettings.set("power_check_interval=" + powerCheckInterval);

            // Make sure we could start activity from background
            SystemUtil.runShellCommand(mInstrumentation,
                    "cmd deviceidle whitelist +" + PACKAGE_NAME_APP1);

            // Keep the device awake
            toggleScreenOn(true);

            // Start an activity
            CommandReceiver.sendCommand(mTargetContext, CommandReceiver.COMMAND_START_ACTIVITY,
                    PACKAGE_NAME_APP1, PACKAGE_NAME_APP1, 0, null);

            watcher.waitFor(WatchUidRunner.CMD_PROCSTATE, WatchUidRunner.STATE_TOP, null);

            // Spawn a light weight child process
            CountDownLatch startLatch = startChildProcessInPackage(PACKAGE_NAME_APP1,
                    new String[] {"/system/bin/sh", "-c",  "sleep 1000"});

            // Wait for the start of the child process
            assertTrue("Failed to spawn child process",
                    startLatch.await(waitForSec, TimeUnit.MILLISECONDS));

            // Stop the activity
            CommandReceiver.sendCommand(mTargetContext, CommandReceiver.COMMAND_STOP_ACTIVITY,
                    PACKAGE_NAME_APP1, PACKAGE_NAME_APP1, 0, null);

            watcher.waitFor(WatchUidRunner.CMD_CACHED, null);

            // Wait for the system to kill that light weight child (it won't happen actually)
            CountDownLatch stopLatch = initWaitingForChildProcessGone(
                    PACKAGE_NAME_APP1, processGoneTimeout);

            assertFalse("App's light weight child process shouldn't be gone",
                    stopLatch.await(processGoneTimeout, TimeUnit.MILLISECONDS));

            // Now kill the light weight child
            stopLatch = stopChildProcess(PACKAGE_NAME_APP1, waitForSec);

            assertTrue("Failed to kill app's light weight child process",
                    stopLatch.await(waitForSec, TimeUnit.MILLISECONDS));

            // Start an activity again
            CommandReceiver.sendCommand(mTargetContext, CommandReceiver.COMMAND_START_ACTIVITY,
                    PACKAGE_NAME_APP1, PACKAGE_NAME_APP1, 0, null);

            watcher.waitFor(WatchUidRunner.CMD_PROCSTATE, WatchUidRunner.STATE_TOP, null);

            // Spawn the cpu intensive child process
            startLatch = startChildProcessInPackage(PACKAGE_NAME_APP1,
                    new String[] {"/system/bin/sh", "-c",  "while true; do :; done"});

            // Wait for the start of the child process
            assertTrue("Failed to spawn child process",
                    startLatch.await(waitForSec, TimeUnit.MILLISECONDS));

            // Stop the activity
            CommandReceiver.sendCommand(mTargetContext, CommandReceiver.COMMAND_STOP_ACTIVITY,
                    PACKAGE_NAME_APP1, PACKAGE_NAME_APP1, 0, null);

            watcher.waitFor(WatchUidRunner.CMD_CACHED, null);

            // Wait for the system to kill that heavy child due to excessive cpu usage,
            // as well as the parent process.
            watcher.waitFor(WatchUidRunner.CMD_GONE, processGoneTimeout);

        } finally {
            amSettings.close();

            SystemUtil.runShellCommand(mInstrumentation,
                    "cmd deviceidle whitelist -" + PACKAGE_NAME_APP1);

            SystemUtil.runWithShellPermissionIdentity(() -> {
                // force stop test package, where the whole test process group will be killed.
                mActivityManager.forceStopPackage(PACKAGE_NAME_APP1);
            });

            watcher.finish();
        }
    }


    /**
     * Verifies the system will trim app's child processes if there are too many
     */
    @LargeTest
    public void testTrimAppChildProcess() throws Exception {
        final long powerCheckInterval = 5 * 1000;
        final long processGoneTimeout = powerCheckInterval * 4;
        final int waitForSec = 5 * 1000;
        final int maxPhantomProcessesNum = 2;
        final String namespaceActivityManager = "activity_manager";
        final String activityManagerConstants = "activity_manager_constants";
        final String maxPhantomProcesses = "max_phantom_processes";

        final SettingsSession<String> amSettings = new SettingsSession<>(
                Settings.Global.getUriFor(activityManagerConstants),
                Settings.Global::getString, Settings.Global::putString);
        final Bundle currentMax = new Bundle();
        final String keyCurrent = "current";

        ApplicationInfo ai = mTargetContext.getPackageManager()
                .getApplicationInfo(PACKAGE_NAME_APP1, 0);
        final WatchUidRunner watcher1 = new WatchUidRunner(mInstrumentation, ai.uid, waitForSec);
        ai = mTargetContext.getPackageManager().getApplicationInfo(PACKAGE_NAME_APP2, 0);
        final WatchUidRunner watcher2 = new WatchUidRunner(mInstrumentation, ai.uid, waitForSec);
        ai = mTargetContext.getPackageManager().getApplicationInfo(PACKAGE_NAME_APP3, 0);
        final WatchUidRunner watcher3 = new WatchUidRunner(mInstrumentation, ai.uid, waitForSec);

        try {
            // Shorten the power check intervals
            amSettings.set("power_check_interval=" + powerCheckInterval);

            // Reduce the maximum phantom processes allowance
            SystemUtil.runWithShellPermissionIdentity(() -> {
                int current = DeviceConfig.getInt(namespaceActivityManager,
                        maxPhantomProcesses, -1);
                currentMax.putInt(keyCurrent, current);
                DeviceConfig.setProperty(namespaceActivityManager,
                        maxPhantomProcesses,
                        Integer.toString(maxPhantomProcessesNum), false);
            });

            // Make sure we could start activity from background
            SystemUtil.runShellCommand(mInstrumentation,
                    "cmd deviceidle whitelist +" + PACKAGE_NAME_APP1);
            SystemUtil.runShellCommand(mInstrumentation,
                    "cmd deviceidle whitelist +" + PACKAGE_NAME_APP2);
            SystemUtil.runShellCommand(mInstrumentation,
                    "cmd deviceidle whitelist +" + PACKAGE_NAME_APP3);

            // Keep the device awake
            toggleScreenOn(true);

            // Start an activity
            CommandReceiver.sendCommand(mTargetContext, CommandReceiver.COMMAND_START_ACTIVITY,
                    PACKAGE_NAME_APP1, PACKAGE_NAME_APP1, 0, null);

            watcher1.waitFor(WatchUidRunner.CMD_PROCSTATE, WatchUidRunner.STATE_TOP, null);

            // Spawn a light weight child process
            CountDownLatch startLatch = startChildProcessInPackage(PACKAGE_NAME_APP1,
                    new String[] {"/system/bin/sh", "-c",  "sleep 1000"});

            // Wait for the start of the child process
            assertTrue("Failed to spawn child process",
                    startLatch.await(waitForSec, TimeUnit.MILLISECONDS));

            // Start an activity in another package
            CommandReceiver.sendCommand(mTargetContext, CommandReceiver.COMMAND_START_ACTIVITY,
                    PACKAGE_NAME_APP2, PACKAGE_NAME_APP2, 0, null);

            watcher2.waitFor(WatchUidRunner.CMD_PROCSTATE, WatchUidRunner.STATE_TOP, null);

            // Spawn a light weight child process
            startLatch = startChildProcessInPackage(PACKAGE_NAME_APP2,
                    new String[] {"/system/bin/sh", "-c",  "sleep 1000"});

            // Wait for the start of the child process
            assertTrue("Failed to spawn child process",
                    startLatch.await(waitForSec, TimeUnit.MILLISECONDS));

            // Finish the 1st activity
            CommandReceiver.sendCommand(mTargetContext, CommandReceiver.COMMAND_STOP_ACTIVITY,
                    PACKAGE_NAME_APP1, PACKAGE_NAME_APP1, 0, null);

            watcher1.waitFor(WatchUidRunner.CMD_CACHED, null);

            // Wait for the system to kill that light weight child (it won't happen actually)
            CountDownLatch stopLatch = initWaitingForChildProcessGone(
                    PACKAGE_NAME_APP1, processGoneTimeout);

            assertFalse("App's light weight child process shouldn't be gone",
                    stopLatch.await(processGoneTimeout, TimeUnit.MILLISECONDS));

            // Sleep a while
            SystemClock.sleep(powerCheckInterval);

            // Now start an activity in the 3rd party
            CommandReceiver.sendCommand(mTargetContext, CommandReceiver.COMMAND_START_ACTIVITY,
                    PACKAGE_NAME_APP3, PACKAGE_NAME_APP3, 0, null);

            watcher3.waitFor(WatchUidRunner.CMD_PROCSTATE, WatchUidRunner.STATE_TOP, null);

            // Spawn a light weight child process
            startLatch = startChildProcessInPackage(PACKAGE_NAME_APP3,
                    new String[] {"/system/bin/sh", "-c",  "sleep 1000"});

            // Wait for the start of the child process
            assertTrue("Failed to spawn child process",
                    startLatch.await(waitForSec, TimeUnit.MILLISECONDS));

            // Now the 1st child process should have been gone.
            stopLatch = initWaitingForChildProcessGone(
                    PACKAGE_NAME_APP1, processGoneTimeout);

            assertTrue("1st App's child process should have been gone",
                    stopLatch.await(processGoneTimeout, TimeUnit.MILLISECONDS));

        } finally {
            amSettings.close();

            SystemUtil.runWithShellPermissionIdentity(() -> {
                final int current = currentMax.getInt(keyCurrent);
                if (current < 0) {
                    // Hm, DeviceConfig doesn't have an API to delete a property,
                    // let's set it empty so the code will use the built-in default value.
                    DeviceConfig.setProperty(namespaceActivityManager,
                            maxPhantomProcesses, "", false);
                } else {
                    DeviceConfig.setProperty(namespaceActivityManager,
                            maxPhantomProcesses, Integer.toString(current), false);
                }
            });

            SystemUtil.runShellCommand(mInstrumentation,
                    "cmd deviceidle whitelist -" + PACKAGE_NAME_APP1);
            SystemUtil.runShellCommand(mInstrumentation,
                    "cmd deviceidle whitelist -" + PACKAGE_NAME_APP2);
            SystemUtil.runShellCommand(mInstrumentation,
                    "cmd deviceidle whitelist -" + PACKAGE_NAME_APP3);

            SystemUtil.runWithShellPermissionIdentity(() -> {
                // force stop test package, where the whole test process group will be killed.
                mActivityManager.forceStopPackage(PACKAGE_NAME_APP1);
                mActivityManager.forceStopPackage(PACKAGE_NAME_APP2);
                mActivityManager.forceStopPackage(PACKAGE_NAME_APP3);
            });

            watcher1.finish();
            watcher2.finish();
            watcher3.finish();
        }
    }

    private CountDownLatch startChildProcessInPackage(String pkgName, String[] cmdline) {
        final CountDownLatch startLatch = new CountDownLatch(1);

        final IBinder binder = new Binder() {
            @Override
            protected boolean onTransact(int code, Parcel data, Parcel reply, int flags)
                    throws RemoteException {
                switch (code) {
                    case CommandReceiver.RESULT_CHILD_PROCESS_STARTED:
                        startLatch.countDown();
                        return true;
                    default:
                        return false;
                }
            }
        };
        final Bundle extras = new Bundle();
        extras.putBinder(CommandReceiver.EXTRA_CALLBACK, binder);
        extras.putStringArray(CommandReceiver.EXTRA_CHILD_CMDLINE, cmdline);

        CommandReceiver.sendCommand(mTargetContext, CommandReceiver.COMMAND_START_CHILD_PROCESS,
                pkgName, pkgName, 0, extras);

        return startLatch;
    }

    final CountDownLatch stopChildProcess(String pkgName, long timeout) {
        final CountDownLatch stopLatch = new CountDownLatch(1);

        final IBinder binder = new Binder() {
            @Override
            protected boolean onTransact(int code, Parcel data, Parcel reply, int flags)
                    throws RemoteException {
                switch (code) {
                    case CommandReceiver.RESULT_CHILD_PROCESS_STOPPED:
                        stopLatch.countDown();
                        return true;
                    default:
                        return false;
                }
            }
        };
        final Bundle extras = new Bundle();
        extras.putBinder(CommandReceiver.EXTRA_CALLBACK, binder);
        extras.putLong(CommandReceiver.EXTRA_TIMEOUT, timeout);

        CommandReceiver.sendCommand(mTargetContext,
                CommandReceiver.COMMAND_STOP_CHILD_PROCESS, pkgName, pkgName, 0, extras);

        return stopLatch;
    }

    final CountDownLatch initWaitingForChildProcessGone(String pkgName, long timeout) {
        final CountDownLatch stopLatch = new CountDownLatch(1);

        final IBinder binder = new Binder() {
            @Override
            protected boolean onTransact(int code, Parcel data, Parcel reply, int flags)
                    throws RemoteException {
                switch (code) {
                    case CommandReceiver.RESULT_CHILD_PROCESS_GONE:
                        stopLatch.countDown();
                        return true;
                    default:
                        return false;
                }
            }
        };
        final Bundle extras = new Bundle();
        extras.putBinder(CommandReceiver.EXTRA_CALLBACK, binder);
        extras.putLong(CommandReceiver.EXTRA_TIMEOUT, timeout);

        CommandReceiver.sendCommand(mTargetContext,
                CommandReceiver.COMMAND_WAIT_FOR_CHILD_PROCESS_GONE, pkgName, pkgName, 0, extras);

        return stopLatch;
    }

    public void testTrimMemActivityFg() throws Exception {
        final int waitForSec = 5 * 1000;
        final ApplicationInfo ai1 = mTargetContext.getPackageManager()
                .getApplicationInfo(PACKAGE_NAME_APP1, 0);
        final WatchUidRunner watcher1 = new WatchUidRunner(mInstrumentation, ai1.uid, waitForSec);

        final ApplicationInfo ai2 = mTargetContext.getPackageManager()
                .getApplicationInfo(PACKAGE_NAME_APP2, 0);
        final WatchUidRunner watcher2 = new WatchUidRunner(mInstrumentation, ai2.uid, waitForSec);

        final ApplicationInfo ai3 = mTargetContext.getPackageManager()
                .getApplicationInfo(CANT_SAVE_STATE_1_PACKAGE_NAME, 0);
        final WatchUidRunner watcher3 = new WatchUidRunner(mInstrumentation, ai3.uid, waitForSec);

        final CountDownLatch[] latchHolder = new CountDownLatch[1];
        final int[] levelHolder = new int[1];
        final Bundle extras = initWaitingForTrimLevel(latchHolder, levelHolder);
        try {
            // Make sure we could start activity from background
            SystemUtil.runShellCommand(mInstrumentation,
                    "cmd deviceidle whitelist +" + PACKAGE_NAME_APP1);

            // Override the memory pressure level, force it staying at normal.
            SystemUtil.runShellCommand(mInstrumentation, "am memory-factor set NORMAL");

            // Keep the device awake
            toggleScreenOn(true);

            latchHolder[0] = new CountDownLatch(1);

            // Start an activity
            CommandReceiver.sendCommand(mTargetContext, CommandReceiver.COMMAND_START_ACTIVITY,
                    PACKAGE_NAME_APP1, PACKAGE_NAME_APP1, 0, extras);

            watcher1.waitFor(WatchUidRunner.CMD_PROCSTATE, WatchUidRunner.STATE_TOP, null);

            // Force the memory pressure to moderate
            SystemUtil.runShellCommand(mInstrumentation, "am memory-factor set MODERATE");
            assertTrue("Failed to wait for the trim memory event",
                    latchHolder[0].await(waitForSec, TimeUnit.MILLISECONDS));
            assertEquals(TRIM_MEMORY_RUNNING_MODERATE, levelHolder[0]);

            latchHolder[0] = new CountDownLatch(1);
            // Force the memory pressure to low
            SystemUtil.runShellCommand(mInstrumentation, "am memory-factor set LOW");
            assertTrue("Failed to wait for the trim memory event",
                    latchHolder[0].await(waitForSec, TimeUnit.MILLISECONDS));
            assertEquals(TRIM_MEMORY_RUNNING_LOW, levelHolder[0]);

            latchHolder[0] = new CountDownLatch(1);
            // Force the memory pressure to critical
            SystemUtil.runShellCommand(mInstrumentation, "am memory-factor set CRITICAL");
            assertTrue("Failed to wait for the trim memory event",
                    latchHolder[0].await(waitForSec, TimeUnit.MILLISECONDS));
            assertEquals(TRIM_MEMORY_RUNNING_CRITICAL, levelHolder[0]);

            // Reset the memory pressure override
            SystemUtil.runShellCommand(mInstrumentation, "am memory-factor reset");

            latchHolder[0] = new CountDownLatch(1);
            // Start another activity in package2
            CommandReceiver.sendCommand(mTargetContext, CommandReceiver.COMMAND_START_ACTIVITY,
                    PACKAGE_NAME_APP1, PACKAGE_NAME_APP2, 0, null);
            watcher2.waitFor(WatchUidRunner.CMD_PROCSTATE, WatchUidRunner.STATE_TOP, null);
            assertTrue("Failed to wait for the trim memory event",
                    latchHolder[0].await(waitForSec, TimeUnit.MILLISECONDS));
            assertEquals(TRIM_MEMORY_UI_HIDDEN, levelHolder[0]);

            // Start the heavy weight activity
            final Intent intent = new Intent();
            final CountDownLatch[] heavyLatchHolder = new CountDownLatch[1];
            final int[] heavyLevelHolder = new int[1];

            intent.setPackage(CANT_SAVE_STATE_1_PACKAGE_NAME);
            intent.setAction(Intent.ACTION_MAIN);
            intent.addCategory(Intent.CATEGORY_LAUNCHER);
            intent.addFlags(Intent.FLAG_ACTIVITY_NEW_TASK);
            intent.putExtras(initWaitingForTrimLevel(heavyLatchHolder, heavyLevelHolder));

            mTargetContext.startActivity(intent);
            watcher3.waitFor(WatchUidRunner.CMD_PROCSTATE, WatchUidRunner.STATE_TOP, null);

            heavyLatchHolder[0] = new CountDownLatch(1);
            // Force the memory pressure to moderate
            SystemUtil.runShellCommand(mInstrumentation, "am memory-factor set MODERATE");
            assertTrue("Failed to wait for the trim memory event",
                    heavyLatchHolder[0].await(waitForSec, TimeUnit.MILLISECONDS));
            assertEquals(TRIM_MEMORY_RUNNING_MODERATE, heavyLevelHolder[0]);

            // Now go home
            final Intent homeIntent = new Intent();
            homeIntent.setAction(Intent.ACTION_MAIN);
            homeIntent.addCategory(Intent.CATEGORY_HOME);
            homeIntent.addFlags(Intent.FLAG_ACTIVITY_NEW_TASK);

            heavyLatchHolder[0] = new CountDownLatch(1);
            mTargetContext.startActivity(homeIntent);
            assertTrue("Failed to wait for the trim memory event",
                    heavyLatchHolder[0].await(waitForSec, TimeUnit.MILLISECONDS));
            assertEquals(TRIM_MEMORY_BACKGROUND, heavyLevelHolder[0]);

            // All done, clean up.
            CommandReceiver.sendCommand(mTargetContext, CommandReceiver.COMMAND_STOP_ACTIVITY,
                    PACKAGE_NAME_APP1, PACKAGE_NAME_APP2, 0, null);
            CommandReceiver.sendCommand(mTargetContext, CommandReceiver.COMMAND_STOP_ACTIVITY,
                    PACKAGE_NAME_APP1, PACKAGE_NAME_APP1, 0, null);

            final Intent finishIntent = new Intent();
            finishIntent.setPackage(CANT_SAVE_STATE_1_PACKAGE_NAME);
            finishIntent.setAction(ACTION_FINISH);
            finishIntent.addFlags(Intent.FLAG_ACTIVITY_NEW_TASK);
            finishIntent.addFlags(Intent.FLAG_ACTIVITY_SINGLE_TOP);
            mTargetContext.startActivity(finishIntent);

            watcher1.waitFor(WatchUidRunner.CMD_CACHED, null);
            watcher2.waitFor(WatchUidRunner.CMD_CACHED, null);
            watcher3.waitFor(WatchUidRunner.CMD_CACHED, null);
        } finally {
            SystemUtil.runShellCommand(mInstrumentation,
                    "cmd deviceidle whitelist -" + PACKAGE_NAME_APP1);

            SystemUtil.runShellCommand(mInstrumentation, "am memory-factor reset");

            SystemUtil.runWithShellPermissionIdentity(() -> {
                mActivityManager.forceStopPackage(PACKAGE_NAME_APP1);
                mActivityManager.forceStopPackage(PACKAGE_NAME_APP2);
                mActivityManager.forceStopPackage(CANT_SAVE_STATE_1_PACKAGE_NAME);
            });

            watcher1.finish();
            watcher2.finish();
            watcher3.finish();
        }
    }

    public void testTrimMemActivityBg() throws Exception {
        final int minLru = 8;
        final int waitForSec = 30 * 1000;
        final String prefix = "trimmem_";
        final CountDownLatch[] latchHolder = new CountDownLatch[1];
        final String pkgName = PACKAGE_NAME_APP1;
        final ArrayMap<String, Pair<int[], ServiceConnection>> procName2Level = new ArrayMap<>();
        int startSeq = 0;

        try {
            // Kill all background processes
            SystemUtil.runShellCommand(mInstrumentation, "am kill-all");

            // Override the memory pressure level, force it staying at normal.
            SystemUtil.runShellCommand(mInstrumentation, "am memory-factor set NORMAL");

            List<String> lru;
            // Start a new isolated service once a time, and then check the lru list
            do {
                final String instanceName = prefix + startSeq++;
                final int[] levelHolder = new int[1];

                // Spawn the new isolated service
                final ServiceConnection conn = TrimMemService.bindToTrimMemService(
                        pkgName, instanceName, latchHolder, levelHolder, mTargetContext);

                // Get the list of all cached apps
                lru = getCachedAppsLru();
                assertTrue(lru.size() > 0);

                for (int i = lru.size() - 1; i >= 0; i--) {
                    String p = lru.get(i);
                    if (p.indexOf(instanceName) != -1) {
                        // This is the new one we just created
                        procName2Level.put(p, new Pair<>(levelHolder, conn));
                        break;
                    }
                }
                if (lru.size() < minLru) {
                    continue;
                }
                if (lru.get(0).indexOf(pkgName) != -1) {
                    // Okay now the very least recent used cached process is one of ours
                    break;
                } else {
                    // Hm, someone dropped below us in the between, let's kill it
                    ArraySet<String> others = new ArraySet<>();
                    for (int i = 0, size = lru.size(); i < size; i++) {
                        final String name = lru.get(i);
                        if (name.indexOf(pkgName) != -1) {
                            break;
                        }
                        others.add(name);
                    }
                    SystemUtil.runWithShellPermissionIdentity(() -> {
                        final List<ActivityManager.RunningAppProcessInfo> procs = mActivityManager
                                .getRunningAppProcesses();
                        for (ActivityManager.RunningAppProcessInfo info: procs) {
                            if (info.importance
                                    == ActivityManager.RunningAppProcessInfo.IMPORTANCE_CACHED) {
                                if (others.contains(info.processName)) {
                                    mActivityManager.killBackgroundProcesses(info.pkgList[0]);
                                }
                            }
                        }
                    });
                }
            } while (true);

            // Remove all other processes
            for (int i = lru.size() - 1; i >= 0; i--) {
                if (lru.get(i).indexOf(pkgName) == -1) {
                    lru.remove(i);
                }
            }

            latchHolder[0] = new CountDownLatch(lru.size());
            // Force the memory pressure to moderate
            SystemUtil.runShellCommand(mInstrumentation, "am memory-factor set MODERATE");
            assertTrue("Failed to wait for the trim memory event",
                    latchHolder[0].await(waitForSec, TimeUnit.MILLISECONDS));

            // Verify the trim levels among the LRU
            int level = TRIM_MEMORY_COMPLETE;
            assertEquals(level, procName2Level.get(lru.get(0)).first[0]);
            for (int i = 1, size = lru.size(); i < size; i++) {
                int curLevel = procName2Level.get(lru.get(i)).first[0];
                assertTrue(level >= curLevel);
                level = curLevel;
            }

            // Cleanup: Unbind from them
            for (int i = procName2Level.size() - 1; i >= 0; i--) {
                mTargetContext.unbindService(procName2Level.valueAt(i).second);
            }
        } finally {
            SystemUtil.runShellCommand(mInstrumentation, "am memory-factor reset");

            SystemUtil.runWithShellPermissionIdentity(() -> {
                mActivityManager.forceStopPackage(PACKAGE_NAME_APP1);
            });
        }
    }

    public void testServiceDoneLRUPosition() throws Exception {
        ApplicationInfo ai = mTargetContext.getPackageManager()
                .getApplicationInfo(PACKAGE_NAME_APP1, 0);
        final WatchUidRunner watcher1 = new WatchUidRunner(mInstrumentation, ai.uid, WAITFOR_MSEC);
        ai = mTargetContext.getPackageManager().getApplicationInfo(PACKAGE_NAME_APP2, 0);
        final WatchUidRunner watcher2 = new WatchUidRunner(mInstrumentation, ai.uid, WAITFOR_MSEC);
        ai = mTargetContext.getPackageManager().getApplicationInfo(PACKAGE_NAME_APP3, 0);
        final WatchUidRunner watcher3 = new WatchUidRunner(mInstrumentation, ai.uid, WAITFOR_MSEC);
        final HandlerThread handlerThread = new HandlerThread("worker");
        final Messenger[] controllerHolder = new Messenger[1];
        final CountDownLatch[] countDownLatchHolder = new CountDownLatch[1];
        handlerThread.start();
        final Messenger messenger = new Messenger(new Handler(handlerThread.getLooper(), msg -> {
            final Bundle bundle = (Bundle) msg.obj;
            final IBinder binder = bundle.getBinder(CommandReceiver.EXTRA_MESSENGER);
            if (binder != null) {
                controllerHolder[0] = new Messenger(binder);
                countDownLatchHolder[0].countDown();
            }
            return true;
        }));

        try {
            // Make sure we could start activity from background
            SystemUtil.runShellCommand(mInstrumentation,
                    "cmd deviceidle whitelist +" + PACKAGE_NAME_APP1);
            SystemUtil.runShellCommand(mInstrumentation,
                    "cmd deviceidle whitelist +" + PACKAGE_NAME_APP2);
            SystemUtil.runShellCommand(mInstrumentation,
                    "cmd deviceidle whitelist +" + PACKAGE_NAME_APP3);

            // Keep the device awake
            toggleScreenOn(true);

            // Start a FGS in app1
            final Bundle extras = new Bundle();
            countDownLatchHolder[0] = new CountDownLatch(1);
            extras.putBinder(CommandReceiver.EXTRA_MESSENGER, messenger.getBinder());
            CommandReceiver.sendCommand(mTargetContext,
                    CommandReceiver.COMMAND_START_FOREGROUND_SERVICE,
                    PACKAGE_NAME_APP1, PACKAGE_NAME_APP1, 0, extras);

            watcher1.waitFor(WatchUidRunner.CMD_PROCSTATE, WatchUidRunner.STATE_FG_SERVICE, null);

            assertTrue("Failed to get the controller interface",
                    countDownLatchHolder[0].await(WAITFOR_MSEC, TimeUnit.MILLISECONDS));

            // Start an activity in another package
            CommandReceiver.sendCommand(mTargetContext, CommandReceiver.COMMAND_START_ACTIVITY,
                    PACKAGE_NAME_APP2, PACKAGE_NAME_APP2, 0, null);

            watcher2.waitFor(WatchUidRunner.CMD_PROCSTATE, WatchUidRunner.STATE_TOP, null);

            // Start another activity in another package
            CommandReceiver.sendCommand(mTargetContext, CommandReceiver.COMMAND_START_ACTIVITY,
                    PACKAGE_NAME_APP3, PACKAGE_NAME_APP3, 0, null);

            watcher3.waitFor(WatchUidRunner.CMD_PROCSTATE, WatchUidRunner.STATE_TOP, null);

            // Stop both of these activities
            CommandReceiver.sendCommand(mTargetContext, CommandReceiver.COMMAND_STOP_ACTIVITY,
                    PACKAGE_NAME_APP2, PACKAGE_NAME_APP2, 0, null);
            watcher2.waitFor(WatchUidRunner.CMD_PROCSTATE, WatchUidRunner.STATE_CACHED_EMPTY, null);
            CommandReceiver.sendCommand(mTargetContext, CommandReceiver.COMMAND_STOP_ACTIVITY,
                    PACKAGE_NAME_APP3, PACKAGE_NAME_APP3, 0, null);
            watcher3.waitFor(WatchUidRunner.CMD_PROCSTATE, WatchUidRunner.STATE_CACHED_EMPTY, null);

            // Launch home so we'd have cleared these the above test activities from recents.
            launchHome();

            // Now stop the foreground service, we'd have to do via the controller interface
            final Message msg = Message.obtain();
            try {
                msg.what = LocalForegroundService.COMMAND_STOP_SELF;
                controllerHolder[0].send(msg);
            } catch (RemoteException e) {
                fail("Unable to stop test package");
            }
            msg.recycle();
            watcher1.waitFor(WatchUidRunner.CMD_PROCSTATE, WatchUidRunner.STATE_CACHED_EMPTY, null);

            final List<String> lru = getCachedAppsLru();

            assertTrue("Failed to get cached app list", lru.size() > 0);
            final int app1LruPos = lru.indexOf(PACKAGE_NAME_APP1);
            final int app2LruPos = lru.indexOf(PACKAGE_NAME_APP2);
            final int app3LruPos = lru.indexOf(PACKAGE_NAME_APP3);
            if (app1LruPos != -1) {
                assertTrue(PACKAGE_NAME_APP1 + " should be newer than " + PACKAGE_NAME_APP2,
                        app1LruPos > app2LruPos);
                assertTrue(PACKAGE_NAME_APP1 + " should be newer than " + PACKAGE_NAME_APP3,
                        app1LruPos > app3LruPos);
            } else {
                assertEquals(PACKAGE_NAME_APP2 + " should have gone", -1, app2LruPos);
                assertEquals(PACKAGE_NAME_APP3 + " should have gone", -1, app3LruPos);
            }
        } finally {
            handlerThread.quitSafely();

            SystemUtil.runShellCommand(mInstrumentation,
                    "cmd deviceidle whitelist -" + PACKAGE_NAME_APP1);
            SystemUtil.runShellCommand(mInstrumentation,
                    "cmd deviceidle whitelist -" + PACKAGE_NAME_APP2);
            SystemUtil.runShellCommand(mInstrumentation,
                    "cmd deviceidle whitelist -" + PACKAGE_NAME_APP3);

            SystemUtil.runWithShellPermissionIdentity(() -> {
                // force stop test package, where the whole test process group will be killed.
                mActivityManager.forceStopPackage(PACKAGE_NAME_APP1);
                mActivityManager.forceStopPackage(PACKAGE_NAME_APP2);
                mActivityManager.forceStopPackage(PACKAGE_NAME_APP3);
            });

            watcher1.finish();
            watcher2.finish();
            watcher3.finish();
        }
    }

    private List<String> getCachedAppsLru() throws Exception {
        final List<String> lru = new ArrayList<>();
        final String output = SystemUtil.runShellCommand(mInstrumentation, "dumpsys activity lru");
        final String[] lines = output.split("\n");
        for (String line: lines) {
            if (line == null || line.indexOf(" cch") == -1) {
                continue;
            }
            final int slash = line.lastIndexOf('/');
            if (slash == -1) {
                continue;
            }
            line = line.substring(0, slash);
            final int space = line.lastIndexOf(' ');
            if (space == -1) {
                continue;
            }
            line = line.substring(space + 1);
            final int colon = line.indexOf(':');
            if (colon == -1) {
                continue;
            }
            lru.add(0, line.substring(colon + 1));
        }
        return lru;
    }

    private Bundle initWaitingForTrimLevel(
            final CountDownLatch[] latchHolder, final int[] levelHolder) {
        final IBinder binder = new Binder() {
            @Override
            protected boolean onTransact(int code, Parcel data, Parcel reply, int flags)
                    throws RemoteException {
                switch (code) {
                    case IBinder.FIRST_CALL_TRANSACTION:
                        levelHolder[0] = data.readInt();
                        latchHolder[0].countDown();
                        return true;
                    default:
                        return false;
                }
            }
        };
        final Bundle extras = new Bundle();
        extras.putBinder(CommandReceiver.EXTRA_CALLBACK, binder);
        return extras;
    }

    private RunningAppProcessInfo getRunningAppProcessInfo(String processName) {
        try {
            return SystemUtil.callWithShellPermissionIdentity(()-> {
                return mActivityManager.getRunningAppProcesses().stream().filter(
                        (ra) -> processName.equals(ra.processName)).findFirst().orElse(null);
            });
        } catch (Exception e) {
        }
        return null;
    }

    private boolean isProcessGone(int pid, String processName) {
        RunningAppProcessInfo info = getRunningAppProcessInfo(processName);
        return info == null || info.pid != pid;
    }

    // Copied from DeviceStatesTest
    /**
     * Make sure the screen state.
     */
    private void toggleScreenOn(final boolean screenon) throws Exception {
        if (screenon) {
            executeAndLogShellCommand("input keyevent KEYCODE_WAKEUP");
            executeAndLogShellCommand("wm dismiss-keyguard");
        } else {
            executeAndLogShellCommand("input keyevent KEYCODE_SLEEP");
        }
        // Since the screen on/off intent is ordered, they will not be sent right now.
        SystemClock.sleep(2_000);
    }

    /**
     * Simulated for idle, and then perform idle maintenance now.
     */
    private void triggerIdle(boolean idle) throws Exception {
        if (idle) {
            executeAndLogShellCommand("cmd deviceidle force-idle light");
        } else {
            executeAndLogShellCommand("cmd deviceidle unforce");
        }
        // Wait a moment to let that happen before proceeding.
        SystemClock.sleep(2_000);
    }

    /**
     * Return true if the given supplier says it's true
     */
    private boolean waitUntilTrue(long maxWait, Supplier<Boolean> supplier) throws Exception {
        final long deadLine = SystemClock.uptimeMillis() + maxWait;
        boolean result = false;
        do {
            Thread.sleep(500);
        } while (!(result = supplier.get()) && SystemClock.uptimeMillis() < deadLine);
        return result;
    }
}
