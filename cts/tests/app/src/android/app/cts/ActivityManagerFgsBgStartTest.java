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

package android.app.cts;

import static android.app.ActivityManager.PROCESS_CAPABILITY_ALL;
import static android.app.ActivityManager.PROCESS_CAPABILITY_FOREGROUND_CAMERA;
import static android.app.ActivityManager.PROCESS_CAPABILITY_FOREGROUND_MICROPHONE;
import static android.app.ActivityManager.PROCESS_CAPABILITY_NETWORK;
import static android.app.ActivityManager.PROCESS_CAPABILITY_NONE;
import static android.app.stubs.LocalForegroundService.ACTION_START_FGS_RESULT;
import static android.app.stubs.LocalForegroundServiceLocation.ACTION_START_FGSL_RESULT;
import static android.os.PowerExemptionManager.REASON_PUSH_MESSAGING;
import static android.os.PowerExemptionManager.REASON_PUSH_MESSAGING_OVER_QUOTA;
import static android.os.PowerExemptionManager.REASON_UNKNOWN;
import static android.os.PowerExemptionManager.TEMPORARY_ALLOW_LIST_TYPE_FOREGROUND_SERVICE_ALLOWED;
import static android.os.PowerExemptionManager.TEMPORARY_ALLOW_LIST_TYPE_FOREGROUND_SERVICE_NOT_ALLOWED;
import static android.os.PowerExemptionManager.TEMPORARY_ALLOW_LIST_TYPE_NONE;

import static com.android.compatibility.common.util.SystemUtil.runWithShellPermissionIdentity;

import static junit.framework.Assert.assertNotNull;
import static junit.framework.Assert.assertTrue;
import static junit.framework.Assert.fail;

import android.accessibilityservice.AccessibilityService;
import android.app.ActivityManager;
import android.app.BroadcastOptions;
import android.app.ForegroundServiceStartNotAllowedException;
import android.app.Instrumentation;
import android.app.cts.android.app.cts.tools.WaitForBroadcast;
import android.app.cts.android.app.cts.tools.WatchUidRunner;
import android.app.stubs.CommandReceiver;
import android.app.stubs.LocalForegroundService;
import android.app.stubs.LocalForegroundServiceLocation;
import android.content.ComponentName;
import android.content.Context;
import android.content.Intent;
import android.content.ServiceConnection;
import android.content.pm.ApplicationInfo;
import android.content.pm.ServiceInfo;
import android.os.Bundle;
import android.os.IBinder;
import android.os.PowerExemptionManager;
import android.os.SystemClock;
import android.permission.cts.PermissionUtils;
import android.platform.test.annotations.AsbSecurityTest;
import android.provider.DeviceConfig;
import android.provider.Settings;

import androidx.test.InstrumentationRegistry;
import androidx.test.ext.junit.runners.AndroidJUnit4;
import androidx.test.filters.LargeTest;

import com.android.compatibility.common.util.SystemUtil;

import org.junit.After;
import org.junit.Before;
import org.junit.Ignore;
import org.junit.Test;
import org.junit.runner.RunWith;

@RunWith(AndroidJUnit4.class)
public class ActivityManagerFgsBgStartTest {
    private static final String TAG = ActivityManagerFgsBgStartTest.class.getName();

    private static final String STUB_PACKAGE_NAME = "android.app.stubs";
    private static final String PACKAGE_NAME_APP1 = "com.android.app1";
    private static final String PACKAGE_NAME_APP2 = "com.android.app2";
    private static final String PACKAGE_NAME_APP3 = "com.android.app3";

    private static final String KEY_DEFAULT_FGS_STARTS_RESTRICTION_ENABLED =
            "default_fgs_starts_restriction_enabled";

    private static final String KEY_FGS_START_FOREGROUND_TIMEOUT =
            "fgs_start_foreground_timeout";

    private static final String KEY_PUSH_MESSAGING_OVER_QUOTA_BEHAVIOR =
            "push_messaging_over_quota_behavior";

    private static final int DEFAULT_FGS_START_FOREGROUND_TIMEOUT_MS = 10 * 1000;

    public static final Integer LOCAL_SERVICE_PROCESS_CAPABILITY = new Integer(
            PROCESS_CAPABILITY_FOREGROUND_CAMERA
            | PROCESS_CAPABILITY_FOREGROUND_MICROPHONE
            | PROCESS_CAPABILITY_NETWORK);

    private static final int WAITFOR_MSEC = 10000;

    private static final int TEMP_ALLOWLIST_DURATION_MS = 2000;

    private static final String[] PACKAGE_NAMES = {
            PACKAGE_NAME_APP1, PACKAGE_NAME_APP2, PACKAGE_NAME_APP3
    };

    private Context mContext;
    private Instrumentation mInstrumentation;
    private Context mTargetContext;

    private int mOrigDeviceDemoMode = 0;

    @Before
    public void setUp() throws Exception {
        mInstrumentation = InstrumentationRegistry.getInstrumentation();
        mContext = mInstrumentation.getContext();
        mTargetContext = mInstrumentation.getTargetContext();
        for (int i = 0; i < PACKAGE_NAMES.length; ++i) {
            CtsAppTestUtils.makeUidIdle(mInstrumentation, PACKAGE_NAMES[i]);
            // The manifest file gives test app SYSTEM_ALERT_WINDOW permissions, which also exempt
            // the app from BG-FGS-launch restriction. Remove SYSTEM_ALERT_WINDOW permission to test
            // other BG-FGS-launch exemptions.
            allowBgActivityStart(PACKAGE_NAMES[i], false);
        }
        CtsAppTestUtils.turnScreenOn(mInstrumentation, mContext);
        cleanupResiduals();
        enableFgsRestriction(true, true, null);
    }

    @After
    public void tearDown() throws Exception {
        for (int i = 0; i < PACKAGE_NAMES.length; ++i) {
            CtsAppTestUtils.makeUidIdle(mInstrumentation, PACKAGE_NAMES[i]);
            allowBgActivityStart(PACKAGE_NAMES[i], true);
        }
        cleanupResiduals();
        enableFgsRestriction(true, true, null);
        for (String packageName: PACKAGE_NAMES) {
            resetFgsRestriction(packageName);
        }
    }

    private void cleanupResiduals() {
        // Stop all the packages to avoid residual impact
        final ActivityManager am = mContext.getSystemService(ActivityManager.class);
        for (int i = 0; i < PACKAGE_NAMES.length; i++) {
            final String pkgName = PACKAGE_NAMES[i];
            SystemUtil.runWithShellPermissionIdentity(() -> {
                am.forceStopPackage(pkgName);
            });
        }
        // Make sure we are in Home screen
        mInstrumentation.getUiAutomation().performGlobalAction(
                AccessibilityService.GLOBAL_ACTION_HOME);
    }

    /**
     * APP1 is in BG state, it can start FGSL, but it won't get location capability.
     * APP1 is in TOP state, it gets location capability.
     * @throws Exception
     */
    @Test
    public void testFgsLocationStartFromBG() throws Exception {
        ApplicationInfo app1Info = mContext.getPackageManager().getApplicationInfo(
                PACKAGE_NAME_APP1, 0);
        WatchUidRunner uid1Watcher = new WatchUidRunner(mInstrumentation, app1Info.uid,
                WAITFOR_MSEC);

        try {
            WaitForBroadcast waiter = new WaitForBroadcast(mInstrumentation.getTargetContext());
            waiter.prepare(ACTION_START_FGSL_RESULT);
            // APP1 is in BG state, Start FGSL in APP1, it won't get location capability.
            Bundle bundle = new Bundle();
            bundle.putInt(LocalForegroundServiceLocation.EXTRA_FOREGROUND_SERVICE_TYPE,
                    ServiceInfo.FOREGROUND_SERVICE_TYPE_LOCATION);
            // start FGSL.
            enableFgsRestriction(false, true, null);
            CommandReceiver.sendCommand(mContext,
                    CommandReceiver.COMMAND_START_FOREGROUND_SERVICE_LOCATION,
                    PACKAGE_NAME_APP1, PACKAGE_NAME_APP1, 0, bundle);
            // APP1 is in FGS state, but won't get location capability.
            uid1Watcher.waitFor(WatchUidRunner.CMD_PROCSTATE,
                    WatchUidRunner.STATE_FG_SERVICE,
                    new Integer(PROCESS_CAPABILITY_NETWORK));
            waiter.doWait(WAITFOR_MSEC);
            // stop FGSL
            CommandReceiver.sendCommand(mContext,
                    CommandReceiver.COMMAND_STOP_FOREGROUND_SERVICE_LOCATION,
                    PACKAGE_NAME_APP1, PACKAGE_NAME_APP1, 0, null);
            uid1Watcher.waitFor(WatchUidRunner.CMD_PROCSTATE,
                    WatchUidRunner.STATE_CACHED_EMPTY,
                    new Integer(PROCESS_CAPABILITY_NONE));

            waiter = new WaitForBroadcast(mInstrumentation.getTargetContext());
            waiter.prepare(ACTION_START_FGS_RESULT);
            // APP1 is in FGS state,
            CommandReceiver.sendCommand(mContext,
                    CommandReceiver.COMMAND_START_FOREGROUND_SERVICE,
                    PACKAGE_NAME_APP1, PACKAGE_NAME_APP1, 0, bundle);
            // start FGSL in app1, it won't get location capability.
            CommandReceiver.sendCommand(mContext,
                    CommandReceiver.COMMAND_START_FOREGROUND_SERVICE_LOCATION,
                    PACKAGE_NAME_APP1, PACKAGE_NAME_APP1, 0, bundle);
            // APP1 is in STATE_FG_SERVICE, but won't get location capability.
            uid1Watcher.waitFor(WatchUidRunner.CMD_PROCSTATE,
                    WatchUidRunner.STATE_FG_SERVICE,
                    new Integer(PROCESS_CAPABILITY_NETWORK));
            waiter.doWait(WAITFOR_MSEC);
            // stop FGS.
            CommandReceiver.sendCommand(mContext,
                    CommandReceiver.COMMAND_STOP_FOREGROUND_SERVICE,
                    PACKAGE_NAME_APP1, PACKAGE_NAME_APP1, 0, null);
            // stop FGSL.
            CommandReceiver.sendCommand(mContext,
                    CommandReceiver.COMMAND_STOP_FOREGROUND_SERVICE_LOCATION,
                    PACKAGE_NAME_APP1, PACKAGE_NAME_APP1, 0, null);

            // Put APP1 in TOP state, now it gets location capability (because the TOP process
            // gets all while-in-use permission (not from FGSL).
            allowBgActivityStart(PACKAGE_NAME_APP1, true);
            CommandReceiver.sendCommand(mContext,
                    CommandReceiver.COMMAND_START_ACTIVITY,
                    PACKAGE_NAME_APP1, PACKAGE_NAME_APP1, 0, null);
            uid1Watcher.waitFor(WatchUidRunner.CMD_PROCSTATE,
                    WatchUidRunner.STATE_TOP,
                    new Integer(PROCESS_CAPABILITY_ALL));

            waiter = new WaitForBroadcast(mInstrumentation.getTargetContext());
            waiter.prepare(ACTION_START_FGSL_RESULT);
            // APP1 is in TOP state, start the FGSL in APP1, it will get location capability.
            CommandReceiver.sendCommand(mContext,
                    CommandReceiver.COMMAND_START_FOREGROUND_SERVICE_LOCATION,
                    PACKAGE_NAME_APP1, PACKAGE_NAME_APP1, 0, bundle);
            // Stop the activity.
            CommandReceiver.sendCommand(mContext,
                    CommandReceiver.COMMAND_STOP_ACTIVITY,
                    PACKAGE_NAME_APP1, PACKAGE_NAME_APP1, 0, null);
            // The FGSL still has location capability because it is started from TOP.
            uid1Watcher.waitFor(WatchUidRunner.CMD_PROCSTATE,
                    WatchUidRunner.STATE_FG_SERVICE,
                    new Integer(PROCESS_CAPABILITY_ALL));
            waiter.doWait(WAITFOR_MSEC);
            // Stop FGSL.
            CommandReceiver.sendCommand(mContext,
                    CommandReceiver.COMMAND_STOP_FOREGROUND_SERVICE_LOCATION,
                    PACKAGE_NAME_APP1, PACKAGE_NAME_APP1, 0, null);
            uid1Watcher.waitFor(WatchUidRunner.CMD_PROCSTATE,
                    WatchUidRunner.STATE_CACHED_EMPTY,
                    new Integer(PROCESS_CAPABILITY_NONE));
        } finally {
            uid1Watcher.finish();
        }
    }

    /**
     * APP1 is in BG state, it can start FGSL in APP2, but the FGS won't get location
     * capability.
     * APP1 is in TOP state, it can start FGSL in APP2, FGSL gets location capability.
     * @throws Exception
     */
    @Test
    public void testFgsLocationStartFromBGTwoProcesses() throws Exception {
        ApplicationInfo app1Info = mContext.getPackageManager().getApplicationInfo(
                PACKAGE_NAME_APP1, 0);
        ApplicationInfo app2Info = mContext.getPackageManager().getApplicationInfo(
                PACKAGE_NAME_APP2, 0);
        WatchUidRunner uid1Watcher = new WatchUidRunner(mInstrumentation, app1Info.uid,
                WAITFOR_MSEC);
        WatchUidRunner uid2Watcher = new WatchUidRunner(mInstrumentation, app2Info.uid,
                WAITFOR_MSEC);

        try {
            // APP1 is in BG state, start FGSL in APP2.
            Bundle bundle = new Bundle();
            bundle.putInt(LocalForegroundServiceLocation.EXTRA_FOREGROUND_SERVICE_TYPE,
                    ServiceInfo.FOREGROUND_SERVICE_TYPE_LOCATION);
            WaitForBroadcast waiter = new WaitForBroadcast(mInstrumentation.getTargetContext());
            waiter.prepare(ACTION_START_FGSL_RESULT);
            enableFgsRestriction(false, true, null);
            CommandReceiver.sendCommand(mContext,
                    CommandReceiver.COMMAND_START_FOREGROUND_SERVICE_LOCATION,
                    PACKAGE_NAME_APP1, PACKAGE_NAME_APP2, 0, bundle);
            // APP2 won't have location capability because APP1 is not in TOP state.
            uid2Watcher.waitFor(WatchUidRunner.CMD_PROCSTATE,
                    WatchUidRunner.STATE_FG_SERVICE,
                    new Integer(PROCESS_CAPABILITY_NETWORK));
            waiter.doWait(WAITFOR_MSEC);

            CommandReceiver.sendCommand(mContext,
                    CommandReceiver.COMMAND_STOP_FOREGROUND_SERVICE_LOCATION,
                    PACKAGE_NAME_APP1, PACKAGE_NAME_APP2, 0, null);
            uid2Watcher.waitFor(WatchUidRunner.CMD_PROCSTATE,
                    WatchUidRunner.STATE_CACHED_EMPTY,
                    new Integer(PROCESS_CAPABILITY_NONE));

            // Put APP1 in TOP state
            allowBgActivityStart(PACKAGE_NAME_APP1, true);
            CommandReceiver.sendCommand(mContext,
                    CommandReceiver.COMMAND_START_ACTIVITY,
                    PACKAGE_NAME_APP1, PACKAGE_NAME_APP1, 0, null);
            uid1Watcher.waitFor(WatchUidRunner.CMD_PROCSTATE,
                    WatchUidRunner.STATE_TOP,
                    new Integer(PROCESS_CAPABILITY_ALL));

            waiter = new WaitForBroadcast(mInstrumentation.getTargetContext());
            waiter.prepare(ACTION_START_FGSL_RESULT);
            // From APP1, start FGSL in APP2.
            CommandReceiver.sendCommand(mContext,
                    CommandReceiver.COMMAND_START_FOREGROUND_SERVICE_LOCATION,
                    PACKAGE_NAME_APP1, PACKAGE_NAME_APP2, 0, bundle);
            // Now APP2 gets location capability.
            uid2Watcher.waitFor(WatchUidRunner.CMD_PROCSTATE,
                    WatchUidRunner.STATE_FG_SERVICE,
                    new Integer(PROCESS_CAPABILITY_ALL));
            waiter.doWait(WAITFOR_MSEC);

            CommandReceiver.sendCommand(mContext,
                    CommandReceiver.COMMAND_STOP_FOREGROUND_SERVICE_LOCATION,
                    PACKAGE_NAME_APP2, PACKAGE_NAME_APP2, 0, null);

            uid2Watcher.waitFor(WatchUidRunner.CMD_PROCSTATE,
                    WatchUidRunner.STATE_CACHED_EMPTY,
                    new Integer(PROCESS_CAPABILITY_NONE));

            CommandReceiver.sendCommand(mContext,
                    CommandReceiver.COMMAND_STOP_ACTIVITY,
                    PACKAGE_NAME_APP1, PACKAGE_NAME_APP1, 0, null);

            uid1Watcher.waitFor(WatchUidRunner.CMD_PROCSTATE,
                    WatchUidRunner.STATE_CACHED_EMPTY,
                    new Integer(PROCESS_CAPABILITY_NONE));
        } finally {
            uid1Watcher.finish();
            uid2Watcher.finish();
        }
    }

    /**
     * APP1 is in BG state, by a PendingIntent, it can start FGSL in APP2,
     * but the FGS won't get location capability.
     * APP1 is in TOP state, by a PendingIntent, it can start FGSL in APP2,
     * FGSL gets location capability.
     * @throws Exception
     */
    @Test
    public void testFgsLocationPendingIntent() throws Exception {
        ApplicationInfo app1Info = mContext.getPackageManager().getApplicationInfo(
                PACKAGE_NAME_APP1, 0);
        ApplicationInfo app2Info = mContext.getPackageManager().getApplicationInfo(
                PACKAGE_NAME_APP2, 0);
        WatchUidRunner uid1Watcher = new WatchUidRunner(mInstrumentation, app1Info.uid,
                WAITFOR_MSEC);
        WatchUidRunner uid2Watcher = new WatchUidRunner(mInstrumentation, app2Info.uid,
                WAITFOR_MSEC);

        try {
            WaitForBroadcast waiter = new WaitForBroadcast(mInstrumentation.getTargetContext());
            waiter.prepare(ACTION_START_FGSL_RESULT);
            // APP1 is in BG state, start FGSL in APP2.
            enableFgsRestriction(false, true, null);
            CommandReceiver.sendCommand(mContext,
                    CommandReceiver.COMMAND_CREATE_FGSL_PENDING_INTENT,
                    PACKAGE_NAME_APP1, PACKAGE_NAME_APP2, 0, null);
            CommandReceiver.sendCommand(mContext,
                    CommandReceiver.COMMAND_SEND_FGSL_PENDING_INTENT,
                    PACKAGE_NAME_APP1, PACKAGE_NAME_APP2, 0, null);
            // APP2 won't have location capability.
            uid2Watcher.waitFor(WatchUidRunner.CMD_PROCSTATE,
                    WatchUidRunner.STATE_FG_SERVICE,
                    new Integer(PROCESS_CAPABILITY_NETWORK));
            waiter.doWait(WAITFOR_MSEC);
            // Stop FGSL in APP2.
            CommandReceiver.sendCommand(mContext,
                    CommandReceiver.COMMAND_STOP_FOREGROUND_SERVICE_LOCATION,
                    PACKAGE_NAME_APP1, PACKAGE_NAME_APP2, 0, null);
            uid2Watcher.waitFor(WatchUidRunner.CMD_PROCSTATE,
                    WatchUidRunner.STATE_CACHED_EMPTY,
                    new Integer(PROCESS_CAPABILITY_NONE));

            waiter = new WaitForBroadcast(mInstrumentation.getTargetContext());
            waiter.prepare(ACTION_START_FGS_RESULT);
            // Put APP1 in FGS state, start FGSL in APP2.
            CommandReceiver.sendCommand(mContext,
                    CommandReceiver.COMMAND_START_FOREGROUND_SERVICE,
                    PACKAGE_NAME_APP1, PACKAGE_NAME_APP1, 0, null);
            uid1Watcher.waitFor(WatchUidRunner.CMD_PROCSTATE,
                    WatchUidRunner.STATE_FG_SERVICE,
                    new Integer(PROCESS_CAPABILITY_NETWORK));
            waiter.doWait(WAITFOR_MSEC);
            CommandReceiver.sendCommand(mContext,
                    CommandReceiver.COMMAND_CREATE_FGSL_PENDING_INTENT,
                    PACKAGE_NAME_APP1, PACKAGE_NAME_APP2, 0, null);

            waiter = new WaitForBroadcast(mInstrumentation.getTargetContext());
            waiter.prepare(ACTION_START_FGSL_RESULT);
            CommandReceiver.sendCommand(mContext,
                    CommandReceiver.COMMAND_SEND_FGSL_PENDING_INTENT,
                    PACKAGE_NAME_APP1, PACKAGE_NAME_APP2, 0, null);
            // APP2 won't have location capability.
            uid2Watcher.waitFor(WatchUidRunner.CMD_PROCSTATE,
                    WatchUidRunner.STATE_FG_SERVICE,
                    new Integer(PROCESS_CAPABILITY_NETWORK));
            waiter.doWait(WAITFOR_MSEC);
            // stop FGSL in APP2.
            CommandReceiver.sendCommand(mContext,
                    CommandReceiver.COMMAND_STOP_FOREGROUND_SERVICE_LOCATION,
                    PACKAGE_NAME_APP1, PACKAGE_NAME_APP2, 0, null);
            uid2Watcher.waitFor(WatchUidRunner.CMD_PROCSTATE,
                    WatchUidRunner.STATE_CACHED_EMPTY,
                    new Integer(PROCESS_CAPABILITY_NONE));

            // put APP1 in TOP state, start FGSL in APP2.
            allowBgActivityStart(PACKAGE_NAME_APP1, true);
            CommandReceiver.sendCommand(mContext,
                    CommandReceiver.COMMAND_START_ACTIVITY,
                    PACKAGE_NAME_APP1, PACKAGE_NAME_APP1, 0, null);
            uid1Watcher.waitFor(WatchUidRunner.CMD_PROCSTATE,
                    WatchUidRunner.STATE_TOP,
                    new Integer(PROCESS_CAPABILITY_ALL));
            CommandReceiver.sendCommand(mContext,
                    CommandReceiver.COMMAND_CREATE_FGSL_PENDING_INTENT,
                    PACKAGE_NAME_APP1, PACKAGE_NAME_APP2, 0, null);

            waiter = new WaitForBroadcast(mInstrumentation.getTargetContext());
            waiter.prepare(ACTION_START_FGSL_RESULT);
            CommandReceiver.sendCommand(mContext,
                    CommandReceiver.COMMAND_SEND_FGSL_PENDING_INTENT,
                    PACKAGE_NAME_APP1, PACKAGE_NAME_APP2, 0, null);
            // APP2 now have location capability (because APP1 is TOP)
            uid2Watcher.waitFor(WatchUidRunner.CMD_PROCSTATE,
                    WatchUidRunner.STATE_FG_SERVICE,
                    new Integer(PROCESS_CAPABILITY_ALL));
            waiter.doWait(WAITFOR_MSEC);

            // stop FGSL in APP2.
            CommandReceiver.sendCommand(mContext,
                    CommandReceiver.COMMAND_STOP_FOREGROUND_SERVICE_LOCATION,
                    PACKAGE_NAME_APP1, PACKAGE_NAME_APP2, 0, null);
            uid2Watcher.waitFor(WatchUidRunner.CMD_PROCSTATE,
                    WatchUidRunner.STATE_CACHED_EMPTY,
                    new Integer(PROCESS_CAPABILITY_NONE));

            // stop FGS in APP1,
            CommandReceiver.sendCommand(mContext,
                    CommandReceiver.COMMAND_STOP_FOREGROUND_SERVICE,
                    PACKAGE_NAME_APP1, PACKAGE_NAME_APP1, 0, null);
            // stop TOP activity in APP1.
            CommandReceiver.sendCommand(mContext,
                    CommandReceiver.COMMAND_STOP_ACTIVITY,
                    PACKAGE_NAME_APP1, PACKAGE_NAME_APP1, 0, null);
            uid1Watcher.waitFor(WatchUidRunner.CMD_PROCSTATE,
                    WatchUidRunner.STATE_CACHED_EMPTY,
                    new Integer(PROCESS_CAPABILITY_NONE));
        } finally {
            uid1Watcher.finish();
            uid2Watcher.finish();
        }
    }

    /**
     * Test a FGS start by bind from BG does not get get while-in-use capability.
     * @throws Exception
     */
    @Test
    @AsbSecurityTest(cveBugId = 173516292)
    public void testFgsLocationStartFromBGWithBind() throws Exception {
        ApplicationInfo app1Info = mContext.getPackageManager().getApplicationInfo(
                PACKAGE_NAME_APP1, 0);
        WatchUidRunner uid1Watcher = new WatchUidRunner(mInstrumentation, app1Info.uid,
                WAITFOR_MSEC);

        try {
            WaitForBroadcast waiter = new WaitForBroadcast(mInstrumentation.getTargetContext());
            waiter.prepare(ACTION_START_FGSL_RESULT);
            // APP1 is in BG state, bind FGSL in APP1 first.
            enableFgsRestriction(false, true, null);
            CommandReceiver.sendCommand(mContext, CommandReceiver.COMMAND_BIND_FOREGROUND_SERVICE,
                    PACKAGE_NAME_APP1, PACKAGE_NAME_APP1, 0, null);
            Bundle bundle = new Bundle();
            bundle.putInt(LocalForegroundServiceLocation.EXTRA_FOREGROUND_SERVICE_TYPE,
                    ServiceInfo.FOREGROUND_SERVICE_TYPE_LOCATION);
            // Then start FGSL in APP1, it won't get location capability.
            CommandReceiver.sendCommand(mContext,
                    CommandReceiver.COMMAND_START_FOREGROUND_SERVICE_LOCATION,
                    PACKAGE_NAME_APP1, PACKAGE_NAME_APP1, 0, bundle);

            // APP1 is in FGS state, but won't get location capability.
            uid1Watcher.waitFor(WatchUidRunner.CMD_PROCSTATE,
                    WatchUidRunner.STATE_FG_SERVICE,
                    new Integer(PROCESS_CAPABILITY_NETWORK));
            waiter.doWait(WAITFOR_MSEC);

            // unbind service.
            CommandReceiver.sendCommand(mContext, CommandReceiver.COMMAND_UNBIND_SERVICE,
                    PACKAGE_NAME_APP1, PACKAGE_NAME_APP1, 0, null);
            // stop FGSL
            CommandReceiver.sendCommand(mContext,
                    CommandReceiver.COMMAND_STOP_FOREGROUND_SERVICE_LOCATION,
                    PACKAGE_NAME_APP1, PACKAGE_NAME_APP1, 0, null);
            uid1Watcher.waitFor(WatchUidRunner.CMD_PROCSTATE,
                    WatchUidRunner.STATE_CACHED_EMPTY,
                    new Integer(PROCESS_CAPABILITY_NONE));
        } finally {
            uid1Watcher.finish();
        }
    }

    @Test
    public void testUpdateUidProcState() throws Exception {
        ApplicationInfo app1Info = mContext.getPackageManager().getApplicationInfo(
                PACKAGE_NAME_APP1, 0);
        WatchUidRunner uid1Watcher = new WatchUidRunner(mInstrumentation, app1Info.uid,
                WAITFOR_MSEC);
        ApplicationInfo app2Info = mContext.getPackageManager().getApplicationInfo(
                PACKAGE_NAME_APP2, 0);
        WatchUidRunner uid2Watcher = new WatchUidRunner(mInstrumentation, app2Info.uid,
                WAITFOR_MSEC);
        ApplicationInfo app3Info = mContext.getPackageManager().getApplicationInfo(
                PACKAGE_NAME_APP3, 0);
        WatchUidRunner uid3Watcher = new WatchUidRunner(mInstrumentation, app3Info.uid,
                WAITFOR_MSEC);

        try {
            WaitForBroadcast waiter = new WaitForBroadcast(mInstrumentation.getTargetContext());
            waiter.prepare(ACTION_START_FGS_RESULT);

            enableFgsRestriction(false, true, null);

            // START FGS in APP2.
            CommandReceiver.sendCommand(mContext,
                    CommandReceiver.COMMAND_START_FOREGROUND_SERVICE,
                    PACKAGE_NAME_APP2, PACKAGE_NAME_APP2, 0, null);
            // APP2 proc state is 4.
            uid2Watcher.waitFor(WatchUidRunner.CMD_PROCSTATE,
                    WatchUidRunner.STATE_FG_SERVICE);
            waiter.doWait(WAITFOR_MSEC);

            // APP2 binds to APP1.
            CommandReceiver.sendCommand(mContext, CommandReceiver.COMMAND_BIND_SERVICE,
                    PACKAGE_NAME_APP2, PACKAGE_NAME_APP1, Context.BIND_INCLUDE_CAPABILITIES, null);
            // APP1 gets proc state 4.
            uid1Watcher.waitFor(WatchUidRunner.CMD_PROCSTATE,
                    WatchUidRunner.STATE_FG_SERVICE);

            // Start activity in APP3, this put APP3 in TOP state.
            allowBgActivityStart(PACKAGE_NAME_APP3, true);
            CommandReceiver.sendCommand(mContext,
                    CommandReceiver.COMMAND_START_ACTIVITY,
                    PACKAGE_NAME_APP3, PACKAGE_NAME_APP3, 0, null);
            // APP3 gets proc state 2.
            uid3Watcher.waitFor(WatchUidRunner.CMD_PROCSTATE, WatchUidRunner.STATE_TOP);

            // APP3 repeatedly bind/unbind with APP2, observer APP1 proc state change.
            // Observe updateUidProcState() call latency.
            for (int i = 0; i < 10; ++i) {
                // APP3 bind to APP2
                CommandReceiver.sendCommand(mContext,
                        CommandReceiver.COMMAND_BIND_SERVICE,
                        PACKAGE_NAME_APP3, PACKAGE_NAME_APP2, Context.BIND_INCLUDE_CAPABILITIES,
                        null);
                uid1Watcher.waitFor(WatchUidRunner.CMD_PROCSTATE, WatchUidRunner.STATE_BOUND_TOP);

                CommandReceiver.sendCommand(mContext, CommandReceiver.COMMAND_UNBIND_SERVICE,
                        PACKAGE_NAME_APP3, PACKAGE_NAME_APP2, 0, null);
                uid1Watcher.waitFor(WatchUidRunner.CMD_PROCSTATE, WatchUidRunner.STATE_FG_SERVICE);
            }

            // unbind service.
            CommandReceiver.sendCommand(mContext, CommandReceiver.COMMAND_STOP_ACTIVITY,
                    PACKAGE_NAME_APP3, PACKAGE_NAME_APP3, 0, null);
            CommandReceiver.sendCommand(mContext, CommandReceiver.COMMAND_UNBIND_SERVICE,
                    PACKAGE_NAME_APP3, PACKAGE_NAME_APP2, 0, null);
            CommandReceiver.sendCommand(mContext, CommandReceiver.COMMAND_UNBIND_SERVICE,
                    PACKAGE_NAME_APP2, PACKAGE_NAME_APP1, 0, null);
            CommandReceiver.sendCommand(mContext,
                    CommandReceiver.COMMAND_STOP_FOREGROUND_SERVICE,
                    PACKAGE_NAME_APP2, PACKAGE_NAME_APP2, 0, null);

        } finally {
            uid1Watcher.finish();
            uid2Watcher.finish();
            uid3Watcher.finish();
            allowBgActivityStart(PACKAGE_NAME_APP3, false);
        }
    }

    /**
     * Test FGS background startForeground() restriction, use DeviceConfig to turn on restriction.
     * @throws Exception
     */
    @Test
    public void testFgsStartFromBG1() throws Exception {
        testFgsStartFromBG(true);
    }

    /**
     * Test FGS background startForeground() restriction, use AppCompat CHANGE ID to turn on
     * restriction.
     * @throws Exception
     */
    @Test
    public void testFgsStartFromBG2() throws Exception {
        testFgsStartFromBG(false);
    }

    private void testFgsStartFromBG(boolean useDeviceConfig) throws Exception {
        ApplicationInfo app1Info = mContext.getPackageManager().getApplicationInfo(
                PACKAGE_NAME_APP1, 0);
        WatchUidRunner uid1Watcher = new WatchUidRunner(mInstrumentation, app1Info.uid,
                WAITFOR_MSEC);
        try {
            WaitForBroadcast waiter = new WaitForBroadcast(mInstrumentation.getTargetContext());
            waiter.prepare(ACTION_START_FGS_RESULT);
            // disable the FGS background startForeground() restriction.
            enableFgsRestriction(false, true, null);
            enableFgsRestriction(false, useDeviceConfig, PACKAGE_NAME_APP1);
            // APP1 is in BG state, Start FGS in APP1.
            CommandReceiver.sendCommand(mContext,
                    CommandReceiver.COMMAND_START_FOREGROUND_SERVICE,
                    PACKAGE_NAME_APP1, PACKAGE_NAME_APP1, 0, null);
            // APP1 is in STATE_FG_SERVICE.
            uid1Watcher.waitFor(WatchUidRunner.CMD_PROCSTATE, WatchUidRunner.STATE_FG_SERVICE);
            waiter.doWait(WAITFOR_MSEC);
            // stop FGS.
            CommandReceiver.sendCommand(mContext,
                    CommandReceiver.COMMAND_STOP_FOREGROUND_SERVICE,
                    PACKAGE_NAME_APP1, PACKAGE_NAME_APP1, 0, null);
            uid1Watcher.waitFor(WatchUidRunner.CMD_PROCSTATE, WatchUidRunner.STATE_CACHED_EMPTY);

            // Enable the FGS background startForeground() restriction.
            allowBgActivityStart(PACKAGE_NAME_APP1, false);
            enableFgsRestriction(true, true, null);
            enableFgsRestriction(true, useDeviceConfig, PACKAGE_NAME_APP1);
            // Start FGS in BG state.
            waiter = new WaitForBroadcast(mInstrumentation.getTargetContext());
            waiter.prepare(ACTION_START_FGS_RESULT);
            CommandReceiver.sendCommand(mContext,
                    CommandReceiver.COMMAND_START_FOREGROUND_SERVICE,
                    PACKAGE_NAME_APP1, PACKAGE_NAME_APP1, 0, null);
            // APP1 does not enter FGS state
            try {
                waiter.doWait(WAITFOR_MSEC);
                fail("Service should not enter foreground service state");
            } catch (Exception e) {
            }

            // Put APP1 in TOP state.
            allowBgActivityStart(PACKAGE_NAME_APP1, true);
            CommandReceiver.sendCommand(mContext,
                    CommandReceiver.COMMAND_START_ACTIVITY,
                    PACKAGE_NAME_APP1, PACKAGE_NAME_APP1, 0, null);
            uid1Watcher.waitFor(WatchUidRunner.CMD_PROCSTATE, WatchUidRunner.STATE_TOP);
            allowBgActivityStart(PACKAGE_NAME_APP1, false);

            waiter = new WaitForBroadcast(mInstrumentation.getTargetContext());
            waiter.prepare(ACTION_START_FGS_RESULT);
            // Now it can start FGS.
            CommandReceiver.sendCommand(mContext,
                    CommandReceiver.COMMAND_START_FOREGROUND_SERVICE,
                    PACKAGE_NAME_APP1, PACKAGE_NAME_APP1, 0, null);
            // Stop activity.
            CommandReceiver.sendCommand(mContext,
                    CommandReceiver.COMMAND_STOP_ACTIVITY,
                    PACKAGE_NAME_APP1, PACKAGE_NAME_APP1, 0, null);
            // FGS is still running.
            uid1Watcher.waitFor(WatchUidRunner.CMD_PROCSTATE, WatchUidRunner.STATE_FG_SERVICE);
            waiter.doWait(WAITFOR_MSEC);
            // Stop the FGS.
            CommandReceiver.sendCommand(mContext,
                    CommandReceiver.COMMAND_STOP_FOREGROUND_SERVICE,
                    PACKAGE_NAME_APP1, PACKAGE_NAME_APP1, 0, null);
            uid1Watcher.waitFor(WatchUidRunner.CMD_PROCSTATE, WatchUidRunner.STATE_CACHED_EMPTY);
        } finally {
            uid1Watcher.finish();
        }
    }

    /**
     * Test a FGS can start from a process that is at BOUND_TOP state.
     * @throws Exception
     */
    @Test
    public void testFgsStartFromBoundTopState() throws Exception {
        ApplicationInfo app1Info = mContext.getPackageManager().getApplicationInfo(
                PACKAGE_NAME_APP1, 0);
        ApplicationInfo app2Info = mContext.getPackageManager().getApplicationInfo(
                PACKAGE_NAME_APP2, 0);
        ApplicationInfo app3Info = mContext.getPackageManager().getApplicationInfo(
                PACKAGE_NAME_APP3, 0);
        WatchUidRunner uid1Watcher = new WatchUidRunner(mInstrumentation, app1Info.uid,
                WAITFOR_MSEC);
        WatchUidRunner uid2Watcher = new WatchUidRunner(mInstrumentation, app2Info.uid,
                WAITFOR_MSEC);
        WatchUidRunner uid3Watcher = new WatchUidRunner(mInstrumentation, app3Info.uid,
                WAITFOR_MSEC);
        try {
            // Enable the FGS background startForeground() restriction.
            enableFgsRestriction(true, true, null);

            // Put APP1 in TOP state.
            allowBgActivityStart(PACKAGE_NAME_APP1, true);
            CommandReceiver.sendCommand(mContext,
                    CommandReceiver.COMMAND_START_ACTIVITY,
                    PACKAGE_NAME_APP1, PACKAGE_NAME_APP1, 0, null);
            uid1Watcher.waitFor(WatchUidRunner.CMD_PROCSTATE, WatchUidRunner.STATE_TOP);

            // APP1 bound to service in APP2, APP2 get BOUND_TOP state.
            CommandReceiver.sendCommand(mContext,
                    CommandReceiver.COMMAND_BIND_SERVICE,
                    PACKAGE_NAME_APP1, PACKAGE_NAME_APP2, 0, null);
            uid2Watcher.waitFor(WatchUidRunner.CMD_PROCSTATE, WatchUidRunner.STATE_BOUND_TOP);

            WaitForBroadcast waiter = new WaitForBroadcast(mInstrumentation.getTargetContext());
            waiter.prepare(ACTION_START_FGS_RESULT);
            // APP2 can start FGS in APP3.
            CommandReceiver.sendCommand(mContext,
                    CommandReceiver.COMMAND_START_FOREGROUND_SERVICE,
                    PACKAGE_NAME_APP2, PACKAGE_NAME_APP3, 0, null);
            uid3Watcher.waitFor(WatchUidRunner.CMD_PROCSTATE, WatchUidRunner.STATE_FG_SERVICE);
            waiter.doWait(WAITFOR_MSEC);

            // Stop activity.
            CommandReceiver.sendCommand(mContext,
                    CommandReceiver.COMMAND_STOP_ACTIVITY,
                    PACKAGE_NAME_APP1, PACKAGE_NAME_APP1, 0, null);
            uid1Watcher.waitFor(WatchUidRunner.CMD_PROCSTATE, WatchUidRunner.STATE_CACHED_EMPTY);
            // unbind service.
            CommandReceiver.sendCommand(mContext,
                    CommandReceiver.COMMAND_UNBIND_SERVICE,
                    PACKAGE_NAME_APP1, PACKAGE_NAME_APP2, 0, null);
            uid2Watcher.waitFor(WatchUidRunner.CMD_PROCSTATE, WatchUidRunner.STATE_CACHED_EMPTY);
            // Stop the FGS.
            CommandReceiver.sendCommand(mContext,
                    CommandReceiver.COMMAND_STOP_FOREGROUND_SERVICE,
                    PACKAGE_NAME_APP2, PACKAGE_NAME_APP3, 0, null);
            uid3Watcher.waitFor(WatchUidRunner.CMD_PROCSTATE, WatchUidRunner.STATE_CACHED_EMPTY);
        } finally {
            uid1Watcher.finish();
            uid2Watcher.finish();
            uid3Watcher.finish();
        }
    }

    /**
     * Test a FGS can start from a process that is at FOREGROUND_SERVICE state.
     * @throws Exception
     */
    @Test
    public void testFgsStartFromFgsState() throws Exception {
        ApplicationInfo app1Info = mContext.getPackageManager().getApplicationInfo(
                PACKAGE_NAME_APP1, 0);
        ApplicationInfo app2Info = mContext.getPackageManager().getApplicationInfo(
                PACKAGE_NAME_APP2, 0);
        ApplicationInfo app3Info = mContext.getPackageManager().getApplicationInfo(
                PACKAGE_NAME_APP3, 0);
        WatchUidRunner uid1Watcher = new WatchUidRunner(mInstrumentation, app1Info.uid,
                WAITFOR_MSEC);
        WatchUidRunner uid2Watcher = new WatchUidRunner(mInstrumentation, app2Info.uid,
                WAITFOR_MSEC);
        WatchUidRunner uid3Watcher = new WatchUidRunner(mInstrumentation, app3Info.uid,
                WAITFOR_MSEC);
        try {
            // Enable the FGS background startForeground() restriction.
            enableFgsRestriction(true, true, null);

            // Put APP1 in TOP state.
            allowBgActivityStart(PACKAGE_NAME_APP1, true);
            CommandReceiver.sendCommand(mContext,
                    CommandReceiver.COMMAND_START_ACTIVITY,
                    PACKAGE_NAME_APP1, PACKAGE_NAME_APP1, 0, null);
            uid1Watcher.waitFor(WatchUidRunner.CMD_PROCSTATE, WatchUidRunner.STATE_TOP);

            WaitForBroadcast waiter = new WaitForBroadcast(mInstrumentation.getTargetContext());
            waiter.prepare(ACTION_START_FGS_RESULT);
            // APP1 can start FGS in APP2, APP2 gets FOREGROUND_SERVICE state.
            CommandReceiver.sendCommand(mContext,
                    CommandReceiver.COMMAND_START_FOREGROUND_SERVICE,
                    PACKAGE_NAME_APP1, PACKAGE_NAME_APP2, 0, null);
            uid2Watcher.waitFor(WatchUidRunner.CMD_PROCSTATE, WatchUidRunner.STATE_FG_SERVICE);
            waiter.doWait(WAITFOR_MSEC);

            waiter = new WaitForBroadcast(mInstrumentation.getTargetContext());
            waiter.prepare(ACTION_START_FGS_RESULT);
            // APP2 can start FGS in APP3.
            CommandReceiver.sendCommand(mContext,
                    CommandReceiver.COMMAND_START_FOREGROUND_SERVICE,
                    PACKAGE_NAME_APP2, PACKAGE_NAME_APP3, 0, null);
            uid3Watcher.waitFor(WatchUidRunner.CMD_PROCSTATE, WatchUidRunner.STATE_FG_SERVICE);
            waiter.doWait(WAITFOR_MSEC);

            // Stop activity in APP1.
            CommandReceiver.sendCommand(mContext,
                    CommandReceiver.COMMAND_STOP_ACTIVITY,
                    PACKAGE_NAME_APP1, PACKAGE_NAME_APP1, 0, null);
            uid1Watcher.waitFor(WatchUidRunner.CMD_PROCSTATE, WatchUidRunner.STATE_CACHED_EMPTY);
            // Stop FGS in APP2.
            CommandReceiver.sendCommand(mContext,
                    CommandReceiver.COMMAND_STOP_FOREGROUND_SERVICE,
                    PACKAGE_NAME_APP1, PACKAGE_NAME_APP2, 0, null);
            uid2Watcher.waitFor(WatchUidRunner.CMD_PROCSTATE, WatchUidRunner.STATE_CACHED_EMPTY);
            // Stop FGS in APP3.
            CommandReceiver.sendCommand(mContext,
                    CommandReceiver.COMMAND_STOP_FOREGROUND_SERVICE,
                    PACKAGE_NAME_APP3, PACKAGE_NAME_APP3, 0, null);
            uid3Watcher.waitFor(WatchUidRunner.CMD_PROCSTATE, WatchUidRunner.STATE_CACHED_EMPTY);
        } finally {
            uid1Watcher.finish();
            uid2Watcher.finish();
            uid3Watcher.finish();
        }
    }

    /**
     * When the service is started by bindService() command, test when BG-FGS-launch
     * restriction is disabled, FGS can start from background.
     * @throws Exception
     */
    @Test
    public void testFgsStartFromBGWithBind() throws Exception {
        ApplicationInfo app1Info = mContext.getPackageManager().getApplicationInfo(
                PACKAGE_NAME_APP1, 0);
        WatchUidRunner uid1Watcher = new WatchUidRunner(mInstrumentation, app1Info.uid,
                WAITFOR_MSEC);

        try {
            WaitForBroadcast waiter = new WaitForBroadcast(mInstrumentation.getTargetContext());
            waiter.prepare(ACTION_START_FGSL_RESULT);
            // APP1 is in BG state, bind FGSL in APP1 first.
            CommandReceiver.sendCommand(mContext, CommandReceiver.COMMAND_BIND_FOREGROUND_SERVICE,
                    PACKAGE_NAME_APP1, PACKAGE_NAME_APP1, 0, null);
            // Then start FGSL in APP1
            enableFgsRestriction(false, true, null);
            CommandReceiver.sendCommand(mContext,
                    CommandReceiver.COMMAND_START_FOREGROUND_SERVICE_LOCATION,
                    PACKAGE_NAME_APP1, PACKAGE_NAME_APP1, 0, null);
            // APP1 is in FGS state
            uid1Watcher.waitFor(WatchUidRunner.CMD_PROCSTATE, WatchUidRunner.STATE_FG_SERVICE);
            waiter.doWait(WAITFOR_MSEC);

            // stop FGS
            CommandReceiver.sendCommand(mContext,
                    CommandReceiver.COMMAND_STOP_FOREGROUND_SERVICE_LOCATION,
                    PACKAGE_NAME_APP1, PACKAGE_NAME_APP1, 0, null);
            // unbind service.
            CommandReceiver.sendCommand(mContext, CommandReceiver.COMMAND_UNBIND_SERVICE,
                    PACKAGE_NAME_APP1, PACKAGE_NAME_APP1, 0, null);
            uid1Watcher.waitFor(WatchUidRunner.CMD_PROCSTATE, WatchUidRunner.STATE_CACHED_EMPTY);
        } finally {
            uid1Watcher.finish();
        }
    }

    /**
     * When the service is started by bindService() command, test when BG-FGS-launch
     * restriction is enabled, FGS can NOT start from background.
     * @throws Exception
     */
    @Test
    public void testFgsStartFromBGWithBindWithRestriction() throws Exception {
        ApplicationInfo app1Info = mContext.getPackageManager().getApplicationInfo(
                PACKAGE_NAME_APP1, 0);
        WatchUidRunner uid1Watcher = new WatchUidRunner(mInstrumentation, app1Info.uid,
                WAITFOR_MSEC);

        try {
            enableFgsRestriction(true, true, null);
            // APP1 is in BG state, bind FGSL in APP1 first.
            CommandReceiver.sendCommand(mContext, CommandReceiver.COMMAND_BIND_FOREGROUND_SERVICE,
                    PACKAGE_NAME_APP1, PACKAGE_NAME_APP1, 0, null);
            // Then start FGS in APP1
            WaitForBroadcast waiter = new WaitForBroadcast(mInstrumentation.getTargetContext());
            waiter.prepare(ACTION_START_FGS_RESULT);
            CommandReceiver.sendCommand(mContext,
                    CommandReceiver.COMMAND_START_FOREGROUND_SERVICE,
                    PACKAGE_NAME_APP1, PACKAGE_NAME_APP1, 0, null);
            // APP1 does not enter FGS state
            try {
                waiter.doWait(WAITFOR_MSEC);
                fail("Service should not enter foreground service state");
            } catch (Exception e) {
            }

            // stop FGS
            CommandReceiver.sendCommand(mContext,
                    CommandReceiver.COMMAND_STOP_FOREGROUND_SERVICE,
                    PACKAGE_NAME_APP1, PACKAGE_NAME_APP1, 0, null);
            // unbind service.
            CommandReceiver.sendCommand(mContext, CommandReceiver.COMMAND_UNBIND_SERVICE,
                    PACKAGE_NAME_APP1, PACKAGE_NAME_APP1, 0, null);
            uid1Watcher.waitFor(WatchUidRunner.CMD_PROCSTATE, WatchUidRunner.STATE_CACHED_EMPTY);
        } finally {
            uid1Watcher.finish();
        }
    }

    /**
     * Test BIND_ALLOW_BACKGROUND_ACTIVITY_STARTS flag.
     * Shell has START_ACTIVITIES_FROM_BACKGROUND permission, it can use this bind flag to
     * pass BG-Activity-launch ability to APP2, then APP2 can start APP2 FGS from background.
     */
    @Test
    public void testFgsBindingFlagActivity() throws Exception {
        testFgsBindingFlag(Context.BIND_ALLOW_BACKGROUND_ACTIVITY_STARTS);
    }

    /**
     * Test BIND_ALLOW_FOREGROUND_SERVICE_STARTS_FROM_BACKGROUND flag.
     * Shell has START_FOREGROUND_SERVICES_FROM_BACKGROUND permission, it can use this bind flag to
     * pass BG-FGS-launch ability to APP2, then APP2 can start APP3 FGS from background.
     */
    @Test
    public void testFgsBindingFlagFGS() throws Exception {
        testFgsBindingFlag(Context.BIND_ALLOW_FOREGROUND_SERVICE_STARTS_FROM_BACKGROUND);
    }

    /**
     * Test no binding flag.
     * Shell has START_FOREGROUND_SERVICES_FROM_BACKGROUND permission, without any bind flag,
     * the BG-FGS-launch ability can be passed to APP2 by service binding, then APP2 can start
     * APP3 FGS from background.
     */
    @Test
    public void testFgsBindingFlagNone() throws Exception {
        testFgsBindingFlag(0);
    }

    private void testFgsBindingFlag(int bindingFlag) throws Exception {
        ApplicationInfo app1Info = mContext.getPackageManager().getApplicationInfo(
                PACKAGE_NAME_APP1, 0);
        ApplicationInfo app2Info = mContext.getPackageManager().getApplicationInfo(
                PACKAGE_NAME_APP2, 0);
        ApplicationInfo app3Info = mContext.getPackageManager().getApplicationInfo(
                PACKAGE_NAME_APP3, 0);
        WatchUidRunner uid1Watcher = new WatchUidRunner(mInstrumentation, app1Info.uid,
                WAITFOR_MSEC);
        WatchUidRunner uid2Watcher = new WatchUidRunner(mInstrumentation, app2Info.uid,
                WAITFOR_MSEC);
        WatchUidRunner uid3Watcher = new WatchUidRunner(mInstrumentation, app3Info.uid,
                WAITFOR_MSEC);
        try {
            // Enable the FGS background startForeground() restriction.
            enableFgsRestriction(true, true, null);

            // testapp is in background.
            // testapp binds to service in APP2, APP2 still in background state.
            final Intent intent = new Intent().setClassName(
                    PACKAGE_NAME_APP2, "android.app.stubs.LocalService");

            /*
            final ServiceConnection connection = new ServiceConnection() {
                @Override
                public void onServiceConnected(ComponentName name, IBinder service) {
                }
                @Override
                public void onServiceDisconnected(ComponentName name) {
                }
            };
            runWithShellPermissionIdentity(() -> {
                mTargetContext.bindService(intent, connection,
                        Context.BIND_AUTO_CREATE | Context.BIND_WAIVE_PRIORITY);
            });

            // APP2 can not start FGS in APP3.
            WaitForBroadcast waiter = new WaitForBroadcast(mInstrumentation.getTargetContext());
            waiter.prepare(ACTION_START_FGS_RESULT);
            CommandReceiver.sendCommand(mContext,
                    CommandReceiver.COMMAND_START_FOREGROUND_SERVICE,
                    PACKAGE_NAME_APP2, PACKAGE_NAME_APP3, 0, null);
            try {
                waiter.doWait(WAITFOR_MSEC);
                fail("Service should not enter foreground service state");
            } catch (Exception e) {
            }

            // testapp unbind service in APP2.
            runWithShellPermissionIdentity(() -> mTargetContext.unbindService(connection));
            uid2Watcher.waitFor(WatchUidRunner.CMD_PROCSTATE, WatchUidRunner.STATE_CACHED_EMPTY);
            */

            // testapp is in background.
            // testapp binds to service in APP2 using the binding flag.
            // APP2 still in background state.
            final ServiceConnection connection2 = new ServiceConnection() {
                @Override
                public void onServiceConnected(ComponentName name, IBinder service) {
                }
                @Override
                public void onServiceDisconnected(ComponentName name) {
                }
            };
            runWithShellPermissionIdentity(() -> mTargetContext.bindService(intent, connection2,
                    Context.BIND_AUTO_CREATE | Context.BIND_WAIVE_PRIORITY
                            | bindingFlag));

            WaitForBroadcast waiter = new WaitForBroadcast(mInstrumentation.getTargetContext());
            waiter.prepare(ACTION_START_FGS_RESULT);
            // Because the binding flag,
            // APP2 can start FGS from background.
            CommandReceiver.sendCommand(mContext,
                    CommandReceiver.COMMAND_START_FOREGROUND_SERVICE,
                    PACKAGE_NAME_APP2, PACKAGE_NAME_APP3, 0, null);
            uid3Watcher.waitFor(WatchUidRunner.CMD_PROCSTATE, WatchUidRunner.STATE_FG_SERVICE);
            waiter.doWait(WAITFOR_MSEC);

            // testapp unbind service in APP2.
            runWithShellPermissionIdentity(() -> mTargetContext.unbindService(connection2));
            uid2Watcher.waitFor(WatchUidRunner.CMD_PROCSTATE, WatchUidRunner.STATE_CACHED_EMPTY);
            // Stop the FGS in APP3.
            CommandReceiver.sendCommand(mContext,
                    CommandReceiver.COMMAND_STOP_FOREGROUND_SERVICE,
                    PACKAGE_NAME_APP3, PACKAGE_NAME_APP3, 0, null);
            uid3Watcher.waitFor(WatchUidRunner.CMD_PROCSTATE, WatchUidRunner.STATE_CACHED_EMPTY);
        } finally {
            uid1Watcher.finish();
            uid2Watcher.finish();
            uid3Watcher.finish();
        }
    }

    /**
     * Test a FGS can start from BG if the app has SYSTEM_ALERT_WINDOW permission.
     */
    @Test
    public void testFgsStartSystemAlertWindow() throws Exception {
        ApplicationInfo app1Info = mContext.getPackageManager().getApplicationInfo(
                PACKAGE_NAME_APP1, 0);
        WatchUidRunner uid1Watcher = new WatchUidRunner(mInstrumentation, app1Info.uid,
                WAITFOR_MSEC);
        try {
            // Enable the FGS background startForeground() restriction.
            enableFgsRestriction(true, true, null);
            // Start FGS in BG state.
            WaitForBroadcast waiter = new WaitForBroadcast(mInstrumentation.getTargetContext());
            waiter.prepare(ACTION_START_FGS_RESULT);
            CommandReceiver.sendCommand(mContext,
                    CommandReceiver.COMMAND_START_FOREGROUND_SERVICE,
                    PACKAGE_NAME_APP1, PACKAGE_NAME_APP1, 0, null);
            // APP1 does not enter FGS state
            try {
                waiter.doWait(WAITFOR_MSEC);
                fail("Service should not enter foreground service state");
            } catch (Exception e) {
            }

            PermissionUtils.grantPermission(
                    PACKAGE_NAME_APP1, android.Manifest.permission.SYSTEM_ALERT_WINDOW);
            waiter = new WaitForBroadcast(mInstrumentation.getTargetContext());
            waiter.prepare(ACTION_START_FGS_RESULT);
            // Now it can start FGS.
            CommandReceiver.sendCommand(mContext,
                    CommandReceiver.COMMAND_START_FOREGROUND_SERVICE,
                    PACKAGE_NAME_APP1, PACKAGE_NAME_APP1, 0, null);
            uid1Watcher.waitFor(WatchUidRunner.CMD_PROCSTATE, WatchUidRunner.STATE_FG_SERVICE);
            waiter.doWait(WAITFOR_MSEC);
            // Stop the FGS.
            CommandReceiver.sendCommand(mContext,
                    CommandReceiver.COMMAND_STOP_FOREGROUND_SERVICE,
                    PACKAGE_NAME_APP1, PACKAGE_NAME_APP1, 0, null);
            uid1Watcher.waitFor(WatchUidRunner.CMD_PROCSTATE, WatchUidRunner.STATE_CACHED_EMPTY);
        } finally {
            uid1Watcher.finish();
        }
    }

    /**
     * Test a FGS can start from BG if the device is in retail demo mode.
     */
    @Test
    // Change Settings.Global.DEVICE_DEMO_MODE on device may trigger other listener and put
    // the device in undesired state, for example, the battery charge level is set to 35%
    // permanently, ignore this test for now.
    @Ignore
    public void testFgsStartRetailDemoMode() throws Exception {
        ApplicationInfo app1Info = mContext.getPackageManager().getApplicationInfo(
                PACKAGE_NAME_APP1, 0);
        WatchUidRunner uid1Watcher = new WatchUidRunner(mInstrumentation, app1Info.uid,
                WAITFOR_MSEC);
        runWithShellPermissionIdentity(()-> {
            mOrigDeviceDemoMode = Settings.Global.getInt(mContext.getContentResolver(),
                    Settings.Global.DEVICE_DEMO_MODE, 0); });

        try {
            // Enable the FGS background startForeground() restriction.
            enableFgsRestriction(true, true, null);
            // Start FGS in BG state.
            WaitForBroadcast waiter = new WaitForBroadcast(mInstrumentation.getTargetContext());
            waiter.prepare(ACTION_START_FGS_RESULT);
            CommandReceiver.sendCommand(mContext,
                    CommandReceiver.COMMAND_START_FOREGROUND_SERVICE,
                    PACKAGE_NAME_APP1, PACKAGE_NAME_APP1, 0, null);
            // APP1 does not enter FGS state
            try {
                waiter.doWait(WAITFOR_MSEC);
                fail("Service should not enter foreground service state");
            } catch (Exception e) {
            }

            runWithShellPermissionIdentity(()-> {
                Settings.Global.putInt(mContext.getContentResolver(),
                        Settings.Global.DEVICE_DEMO_MODE, 1); });
            waiter = new WaitForBroadcast(mInstrumentation.getTargetContext());
            waiter.prepare(ACTION_START_FGS_RESULT);
            // Now it can start FGS.
            CommandReceiver.sendCommand(mContext,
                    CommandReceiver.COMMAND_START_FOREGROUND_SERVICE,
                    PACKAGE_NAME_APP1, PACKAGE_NAME_APP1, 0, null);
            uid1Watcher.waitFor(WatchUidRunner.CMD_PROCSTATE, WatchUidRunner.STATE_FG_SERVICE);
            waiter.doWait(WAITFOR_MSEC);
            // Stop the FGS.
            CommandReceiver.sendCommand(mContext,
                    CommandReceiver.COMMAND_STOP_FOREGROUND_SERVICE,
                    PACKAGE_NAME_APP1, PACKAGE_NAME_APP1, 0, null);
            uid1Watcher.waitFor(WatchUidRunner.CMD_PROCSTATE, WatchUidRunner.STATE_CACHED_EMPTY);
        } finally {
            uid1Watcher.finish();
            runWithShellPermissionIdentity(()-> {
                Settings.Global.putInt(mContext.getContentResolver(),
                        Settings.Global.DEVICE_DEMO_MODE, mOrigDeviceDemoMode); });
        }
    }

    // At Context.startForegroundService() or Service.startForeground() calls, if the FGS is
    // restricted by background restriction and the app's targetSdkVersion is at least S, the
    // framework throws a ForegroundServiceStartNotAllowedException with error message.
    @Test
    @Ignore("The instrumentation is allowed to star FGS, it does not throw the exception")
    public void testFgsStartFromBGException() throws Exception {
        ForegroundServiceStartNotAllowedException expectedException = null;
        final Intent intent = new Intent().setClassName(
                PACKAGE_NAME_APP1, "android.app.stubs.LocalForegroundService");
        try {
            allowBgActivityStart("android.app.stubs", false);
            enableFgsRestriction(true, true, null);
            mContext.startForegroundService(intent);
        } catch (ForegroundServiceStartNotAllowedException e) {
            expectedException = e;
        } finally {
            mContext.stopService(intent);
            allowBgActivityStart("android.app.stubs", true);
        }
        String expectedMessage = "mAllowStartForeground false";
        assertNotNull(expectedException);
        assertTrue(expectedException.getMessage().contains(expectedMessage));
    }

    /**
     * Test a FGS can start from BG if the app is in the DeviceIdleController's AllowList.
     */
    @Test
    public void testFgsStartAllowList() throws Exception {
        ApplicationInfo app1Info = mContext.getPackageManager().getApplicationInfo(
                PACKAGE_NAME_APP1, 0);
        WatchUidRunner uid1Watcher = new WatchUidRunner(mInstrumentation, app1Info.uid,
                WAITFOR_MSEC);
        try {
            // Enable the FGS background startForeground() restriction.
            enableFgsRestriction(true, true, null);
            // Start FGS in BG state.
            WaitForBroadcast waiter = new WaitForBroadcast(mInstrumentation.getTargetContext());
            waiter.prepare(ACTION_START_FGS_RESULT);
            CommandReceiver.sendCommand(mContext,
                    CommandReceiver.COMMAND_START_FOREGROUND_SERVICE,
                    PACKAGE_NAME_APP1, PACKAGE_NAME_APP1, 0, null);
            // APP1 does not enter FGS state
            try {
                waiter.doWait(WAITFOR_MSEC);
                fail("Service should not enter foreground service state");
            } catch (Exception e) {
            }

            // Add package to AllowList.
            CtsAppTestUtils.executeShellCmd(mInstrumentation,
                    "dumpsys deviceidle whitelist +" + PACKAGE_NAME_APP1);
            waiter = new WaitForBroadcast(mInstrumentation.getTargetContext());
            waiter.prepare(ACTION_START_FGS_RESULT);
            // Now it can start FGS.
            CommandReceiver.sendCommand(mContext,
                    CommandReceiver.COMMAND_START_FOREGROUND_SERVICE,
                    PACKAGE_NAME_APP1, PACKAGE_NAME_APP1, 0, null);
            uid1Watcher.waitFor(WatchUidRunner.CMD_PROCSTATE, WatchUidRunner.STATE_FG_SERVICE);
            waiter.doWait(WAITFOR_MSEC);
            // Stop the FGS.
            CommandReceiver.sendCommand(mContext,
                    CommandReceiver.COMMAND_STOP_FOREGROUND_SERVICE,
                    PACKAGE_NAME_APP1, PACKAGE_NAME_APP1, 0, null);
            uid1Watcher.waitFor(WatchUidRunner.CMD_PROCSTATE, WatchUidRunner.STATE_CACHED_EMPTY);
        } finally {
            uid1Watcher.finish();
            // Remove package from AllowList.
            CtsAppTestUtils.executeShellCmd(mInstrumentation,
                    "dumpsys deviceidle whitelist -" + PACKAGE_NAME_APP1);
        }
    }

    /**
     * Test temp allowlist types in BroadcastOptions.
     */
    @Test
    public void testTempAllowListType() throws Exception {
        testTempAllowListTypeInternal(TEMPORARY_ALLOW_LIST_TYPE_FOREGROUND_SERVICE_NOT_ALLOWED);
        testTempAllowListTypeInternal(TEMPORARY_ALLOW_LIST_TYPE_FOREGROUND_SERVICE_ALLOWED);
    }

    private void testTempAllowListTypeInternal(int type) throws Exception {
        ApplicationInfo app1Info = mContext.getPackageManager().getApplicationInfo(
                PACKAGE_NAME_APP1, 0);
        ApplicationInfo app2Info = mContext.getPackageManager().getApplicationInfo(
                PACKAGE_NAME_APP2, 0);
        WatchUidRunner uid1Watcher = new WatchUidRunner(mInstrumentation, app1Info.uid,
                WAITFOR_MSEC);
        WatchUidRunner uid2Watcher = new WatchUidRunner(mInstrumentation, app2Info.uid,
                WAITFOR_MSEC);
        try {
            // Enable the FGS background startForeground() restriction.
            enableFgsRestriction(true, true, null);
            // Start FGS in BG state.
            WaitForBroadcast waiter = new WaitForBroadcast(mInstrumentation.getTargetContext());
            waiter.prepare(ACTION_START_FGS_RESULT);
            CommandReceiver.sendCommand(mContext,
                    CommandReceiver.COMMAND_START_FOREGROUND_SERVICE,
                    PACKAGE_NAME_APP1, PACKAGE_NAME_APP2, 0, null);
            // APP1 does not enter FGS state
            try {
                waiter.doWait(WAITFOR_MSEC);
                fail("Service should not enter foreground service state");
            } catch (Exception e) {
            }

            // Now it can start FGS.
            waiter = new WaitForBroadcast(mInstrumentation.getTargetContext());
            waiter.prepare(ACTION_START_FGS_RESULT);
            runWithShellPermissionIdentity(()-> {
                final BroadcastOptions options = BroadcastOptions.makeBasic();
                // setTemporaryAppAllowlist API requires
                // START_FOREGROUND_SERVICES_FROM_BACKGROUND permission.
                options.setTemporaryAppAllowlist(TEMP_ALLOWLIST_DURATION_MS, type, REASON_UNKNOWN,
                        "");
                // Must use Shell to issue this command because Shell has
                // START_FOREGROUND_SERVICES_FROM_BACKGROUND permission.
                CommandReceiver.sendCommandWithBroadcastOptions(mContext,
                        CommandReceiver.COMMAND_START_FOREGROUND_SERVICE,
                        PACKAGE_NAME_APP1, PACKAGE_NAME_APP2, 0, null,
                        options.toBundle());
            });
            if (type == TEMPORARY_ALLOW_LIST_TYPE_FOREGROUND_SERVICE_ALLOWED) {
                uid2Watcher.waitFor(WatchUidRunner.CMD_PROCSTATE, WatchUidRunner.STATE_FG_SERVICE);
                waiter.doWait(WAITFOR_MSEC);
                // Stop the FGS.
                CommandReceiver.sendCommand(mContext,
                        CommandReceiver.COMMAND_STOP_FOREGROUND_SERVICE,
                        PACKAGE_NAME_APP1, PACKAGE_NAME_APP2, 0, null);
                uid2Watcher.waitFor(WatchUidRunner.CMD_PROCSTATE,
                        WatchUidRunner.STATE_CACHED_EMPTY);
            } else if (type == TEMPORARY_ALLOW_LIST_TYPE_FOREGROUND_SERVICE_NOT_ALLOWED) {
                // APP1 does not enter FGS state
                try {
                    waiter.doWait(WAITFOR_MSEC);
                    fail("Service should not enter foreground service state");
                } catch (Exception e) {
                }
            }
        } finally {
            uid1Watcher.finish();
            uid2Watcher.finish();
            // Sleep 10 seconds to let the temp allowlist expire so it won't affect next test case.
            SystemClock.sleep(TEMP_ALLOWLIST_DURATION_MS);
        }

    }

    /**
     * Test a FGS can start from BG if the process had a visible activity recently.
     */
    @LargeTest
    @Test
    public void testVisibleActivityGracePeriod() throws Exception {
        ApplicationInfo app2Info = mContext.getPackageManager().getApplicationInfo(
                PACKAGE_NAME_APP2, 0);
        WatchUidRunner uid2Watcher = new WatchUidRunner(mInstrumentation, app2Info.uid,
                WAITFOR_MSEC);
        final String namespaceActivityManager = "activity_manager";
        final String keyFgToBgFgsGraceDuration = "fg_to_bg_fgs_grace_duration";
        final long[] curFgToBgFgsGraceDuration = {-1};
        try {
            // Enable the FGS background startForeground() restriction.
            enableFgsRestriction(true, true, null);
            // Allow bg actvity start from APP1.
            allowBgActivityStart(PACKAGE_NAME_APP1, true);

            SystemUtil.runWithShellPermissionIdentity(() -> {
                curFgToBgFgsGraceDuration[0] = DeviceConfig.getInt(
                        namespaceActivityManager,
                        keyFgToBgFgsGraceDuration, -1);
                DeviceConfig.setProperty(namespaceActivityManager,
                        keyFgToBgFgsGraceDuration,
                        Long.toString(WAITFOR_MSEC), false);
            });

            testVisibleActivityGracePeriodInternal(uid2Watcher, "KEYCODE_HOME");
            testVisibleActivityGracePeriodInternal(uid2Watcher, "KEYCODE_BACK");
        } finally {
            uid2Watcher.finish();
            // Remove package from AllowList.
            allowBgActivityStart(PACKAGE_NAME_APP1, false);
            if (curFgToBgFgsGraceDuration[0] >= 0) {
                SystemUtil.runWithShellPermissionIdentity(() -> {
                    DeviceConfig.setProperty(namespaceActivityManager,
                            keyFgToBgFgsGraceDuration,
                            Long.toString(curFgToBgFgsGraceDuration[0]), false);
                });
            } else {
                CtsAppTestUtils.executeShellCmd(mInstrumentation,
                        "device_config delete " + namespaceActivityManager
                        + " " + keyFgToBgFgsGraceDuration);
            }
        }
    }

    private void testVisibleActivityGracePeriodInternal(WatchUidRunner uidWatcher, String keyCode)
            throws Exception {
        testVisibleActivityGracePeriodInternal(uidWatcher, keyCode, null,
                () -> uidWatcher.waitFor(WatchUidRunner.CMD_PROCSTATE,
                                         WatchUidRunner.STATE_FG_SERVICE), true);

        testVisibleActivityGracePeriodInternal(uidWatcher, keyCode,
                () -> SystemClock.sleep(WAITFOR_MSEC + 2000), // Wait for the grace period to expire
                () -> {
                    try {
                        uidWatcher.waitFor(WatchUidRunner.CMD_PROCSTATE,
                                WatchUidRunner.STATE_FG_SERVICE);
                        fail("Service should not enter foreground service state");
                    } catch (Exception e) {
                        // Expected.
                    }
                }, false);
    }

    private void testVisibleActivityGracePeriodInternal(WatchUidRunner uidWatcher,
            String keyCode, Runnable prep, Runnable verifier, boolean stopFgs) throws Exception {
        // Put APP2 in TOP state.
        CommandReceiver.sendCommand(mContext,
                CommandReceiver.COMMAND_START_ACTIVITY,
                PACKAGE_NAME_APP1, PACKAGE_NAME_APP2, 0, null);
        uidWatcher.waitFor(WatchUidRunner.CMD_PROCSTATE, WatchUidRunner.STATE_TOP);

        // Take a nap to wait for the UI to settle down.
        SystemClock.sleep(2000);

        // Now inject key event.
        CtsAppTestUtils.executeShellCmd(mInstrumentation, "input keyevent " + keyCode);

        // It should go to the cached state.
        uidWatcher.waitFor(WatchUidRunner.CMD_CACHED, null);

        if (prep != null) {
            prep.run();
        }

        // Start FGS from APP2.
        CommandReceiver.sendCommand(mContext,
                CommandReceiver.COMMAND_START_FOREGROUND_SERVICE,
                PACKAGE_NAME_APP2, PACKAGE_NAME_APP2, 0, null);

        if (verifier != null) {
            verifier.run();
        }

        if (stopFgs) {
            // Stop the FGS.
            CommandReceiver.sendCommand(mContext,
                    CommandReceiver.COMMAND_STOP_FOREGROUND_SERVICE,
                    PACKAGE_NAME_APP2, PACKAGE_NAME_APP2, 0, null);
            uidWatcher.waitFor(WatchUidRunner.CMD_CACHED, null);
        }
    }

    /**
     * After background service is started, after 10 seconds timeout, the startForeground() can
     * succeed or not depends on the service's app proc state.
     * Test starService() -> startForeground()
     */
    @Test
    public void testStartForegroundTimeout() throws Exception {
        ApplicationInfo app1Info = mContext.getPackageManager().getApplicationInfo(
                PACKAGE_NAME_APP1, 0);
        WatchUidRunner uid1Watcher = new WatchUidRunner(mInstrumentation, app1Info.uid,
                WAITFOR_MSEC);
        try {
            // Enable the FGS background startForeground() restriction.
            enableFgsRestriction(true, true, null);
            setFgsStartForegroundTimeout(DEFAULT_FGS_START_FOREGROUND_TIMEOUT_MS);

            // Put app to a TOP proc state.
            allowBgActivityStart(PACKAGE_NAME_APP1, true);
            CommandReceiver.sendCommand(mContext, CommandReceiver.COMMAND_START_ACTIVITY,
                    PACKAGE_NAME_APP1, PACKAGE_NAME_APP1, 0, null);
            uid1Watcher.waitFor(WatchUidRunner.CMD_PROCSTATE, WatchUidRunner.STATE_TOP);
            allowBgActivityStart(PACKAGE_NAME_APP1, false);

            // start background service.
            Bundle extras = LocalForegroundService.newCommand(
                    LocalForegroundService.COMMAND_START_NO_FOREGROUND);
            CommandReceiver.sendCommand(mContext, CommandReceiver.COMMAND_START_SERVICE,
                    PACKAGE_NAME_APP1, PACKAGE_NAME_APP1, 0, extras);

            // stop the activity.
            CommandReceiver.sendCommand(mContext, CommandReceiver.COMMAND_STOP_ACTIVITY,
                    PACKAGE_NAME_APP1, PACKAGE_NAME_APP1, 0, null);
            uid1Watcher.waitFor(WatchUidRunner.CMD_PROCSTATE, WatchUidRunner.STATE_SERVICE);

            // Sleep after the timeout DEFAULT_FGS_START_FOREGROUND_TIMEOUT_MS
            SystemClock.sleep(DEFAULT_FGS_START_FOREGROUND_TIMEOUT_MS + 1000);

            extras = LocalForegroundService.newCommand(
                    LocalForegroundService.COMMAND_START_FOREGROUND);
            CommandReceiver.sendCommand(mContext, CommandReceiver.COMMAND_START_SERVICE,
                    PACKAGE_NAME_APP1, PACKAGE_NAME_APP1, 0, extras);
            // APP1 does not enter FGS state
            // startForeground() is called after 10 seconds FgsStartForegroundTimeout.
            try {
                uid1Watcher.waitFor(WatchUidRunner.CMD_PROCSTATE, WatchUidRunner.STATE_FG_SERVICE);
                fail("Service should not enter foreground service state");
            } catch (Exception e) {
            }

            // Put app to a TOP proc state.
            allowBgActivityStart(PACKAGE_NAME_APP1, true);
            CommandReceiver.sendCommand(mContext, CommandReceiver.COMMAND_START_ACTIVITY,
                    PACKAGE_NAME_APP1, PACKAGE_NAME_APP1, 0, null);
            uid1Watcher.waitFor(WatchUidRunner.CMD_PROCSTATE,
                    WatchUidRunner.STATE_TOP, new Integer(PROCESS_CAPABILITY_ALL));
            allowBgActivityStart(PACKAGE_NAME_APP1, false);

            // Call startForeground().
            WaitForBroadcast waiter = new WaitForBroadcast(mInstrumentation.getTargetContext());
            waiter.prepare(ACTION_START_FGS_RESULT);
            extras = LocalForegroundService.newCommand(
                    LocalForegroundService.COMMAND_START_FOREGROUND);
            CommandReceiver.sendCommand(mContext, CommandReceiver.COMMAND_START_SERVICE,
                    PACKAGE_NAME_APP1, PACKAGE_NAME_APP1, 0, extras);
            CommandReceiver.sendCommand(mContext, CommandReceiver.COMMAND_STOP_ACTIVITY,
                    PACKAGE_NAME_APP1, PACKAGE_NAME_APP1, 0, null);

            uid1Watcher.waitFor(WatchUidRunner.CMD_PROCSTATE, WatchUidRunner.STATE_FG_SERVICE);
            waiter.doWait(WAITFOR_MSEC);

            // Stop the FGS.
            CommandReceiver.sendCommand(mContext, CommandReceiver.COMMAND_STOP_SERVICE,
                    PACKAGE_NAME_APP1, PACKAGE_NAME_APP1, 0, null);
            uid1Watcher.waitFor(WatchUidRunner.CMD_PROCSTATE, WatchUidRunner.STATE_CACHED_EMPTY,
                    new Integer(PROCESS_CAPABILITY_NONE));
        } finally {
            uid1Watcher.finish();
            setFgsStartForegroundTimeout(DEFAULT_FGS_START_FOREGROUND_TIMEOUT_MS);
        }
    }

    /**
     * After startForeground() and stopForeground(), the second startForeground() can succeed or not
     * depends on the service's app proc state.
     * Test startForegroundService() -> startForeground() -> stopForeground() -> startForeground().
     */
    @Test
    public void testSecondStartForeground() throws Exception {
        ApplicationInfo app1Info = mContext.getPackageManager().getApplicationInfo(
                PACKAGE_NAME_APP1, 0);
        WatchUidRunner uid1Watcher = new WatchUidRunner(mInstrumentation, app1Info.uid,
                WAITFOR_MSEC);
        try {
            // Enable the FGS background startForeground() restriction.
            enableFgsRestriction(true, true, null);
            WaitForBroadcast waiter = new WaitForBroadcast(mInstrumentation.getTargetContext());
            waiter.prepare(ACTION_START_FGS_RESULT);
            // bypass bg-service-start restriction.
            CtsAppTestUtils.executeShellCmd(mInstrumentation,
                    "dumpsys deviceidle whitelist +" + PACKAGE_NAME_APP1);
            // start foreground service.
            CommandReceiver.sendCommand(mContext, CommandReceiver.COMMAND_START_FOREGROUND_SERVICE,
                    PACKAGE_NAME_APP1, PACKAGE_NAME_APP1, 0, null);
            uid1Watcher.waitFor(WatchUidRunner.CMD_PROCSTATE, WatchUidRunner.STATE_FG_SERVICE);
            waiter.doWait(WAITFOR_MSEC);
            CtsAppTestUtils.executeShellCmd(mInstrumentation,
                    "dumpsys deviceidle whitelist -" + PACKAGE_NAME_APP1);

            // stopForeground()
            Bundle extras = LocalForegroundService.newCommand(
                    LocalForegroundService.COMMAND_STOP_FOREGROUND_REMOVE_NOTIFICATION);
            CommandReceiver.sendCommand(mContext, CommandReceiver.COMMAND_START_SERVICE,
                    PACKAGE_NAME_APP1, PACKAGE_NAME_APP1, 0, extras);
            uid1Watcher.waitFor(WatchUidRunner.CMD_PROCSTATE, WatchUidRunner.STATE_SERVICE,
                    new Integer(PROCESS_CAPABILITY_NONE));

            // startForeground() again.
            extras = LocalForegroundService.newCommand(
                    LocalForegroundService.COMMAND_START_FOREGROUND);
            CommandReceiver.sendCommand(mContext, CommandReceiver.COMMAND_START_SERVICE,
                    PACKAGE_NAME_APP1, PACKAGE_NAME_APP1, 0, extras);
            try {
                uid1Watcher.waitFor(WatchUidRunner.CMD_PROCSTATE, WatchUidRunner.STATE_FG_SERVICE);
                fail("Service should not enter foreground service state");
            } catch (Exception e) {
            }

            // Put app to a TOP proc state.
            allowBgActivityStart(PACKAGE_NAME_APP1, true);
            CommandReceiver.sendCommand(mContext, CommandReceiver.COMMAND_START_ACTIVITY,
                    PACKAGE_NAME_APP1, PACKAGE_NAME_APP1, 0, null);
            uid1Watcher.waitFor(WatchUidRunner.CMD_PROCSTATE, WatchUidRunner.STATE_TOP,
                    new Integer(PROCESS_CAPABILITY_ALL));
            allowBgActivityStart(PACKAGE_NAME_APP1, false);

            // Call startForeground() second time.
            waiter = new WaitForBroadcast(mInstrumentation.getTargetContext());
            waiter.prepare(ACTION_START_FGS_RESULT);
            CommandReceiver.sendCommand(mContext, CommandReceiver.COMMAND_START_FOREGROUND_SERVICE,
                    PACKAGE_NAME_APP1, PACKAGE_NAME_APP1, 0, null);
            CommandReceiver.sendCommand(mContext, CommandReceiver.COMMAND_STOP_ACTIVITY,
                    PACKAGE_NAME_APP1, PACKAGE_NAME_APP1, 0, null);

            uid1Watcher.waitFor(WatchUidRunner.CMD_PROCSTATE, WatchUidRunner.STATE_FG_SERVICE,
                    LOCAL_SERVICE_PROCESS_CAPABILITY);
            waiter.doWait(WAITFOR_MSEC);

            // Stop the FGS.
            CommandReceiver.sendCommand(mContext, CommandReceiver.COMMAND_STOP_FOREGROUND_SERVICE,
                    PACKAGE_NAME_APP1, PACKAGE_NAME_APP1, 0, null);
            uid1Watcher.waitFor(WatchUidRunner.CMD_PROCSTATE, WatchUidRunner.STATE_CACHED_EMPTY,
                    new Integer(PROCESS_CAPABILITY_NONE));
        } finally {
            uid1Watcher.finish();
        }
    }

    /**
     * Test OP_ACTIVATE_VPN and OP_ACTIVATE_PLATFORM_VPN are exempted from BG-FGS-launch
     * restriction.
     * @throws Exception
     */
    @Test
    public void testFgsStartVpn() throws Exception {
        testFgsStartVpnInternal("ACTIVATE_VPN");
        testFgsStartVpnInternal("ACTIVATE_PLATFORM_VPN");
    }

    private void testFgsStartVpnInternal(String vpnAppOp) throws Exception {
        ApplicationInfo app1Info = mContext.getPackageManager().getApplicationInfo(
                PACKAGE_NAME_APP1, 0);
        WatchUidRunner uid1Watcher = new WatchUidRunner(mInstrumentation, app1Info.uid,
                WAITFOR_MSEC);
        try {
            // Enable the FGS background startForeground() restriction.
            enableFgsRestriction(true, true, null);
            // Start FGS in BG state.
            WaitForBroadcast waiter = new WaitForBroadcast(mInstrumentation.getTargetContext());
            waiter.prepare(ACTION_START_FGS_RESULT);
            CommandReceiver.sendCommand(mContext,
                    CommandReceiver.COMMAND_START_FOREGROUND_SERVICE,
                    PACKAGE_NAME_APP1, PACKAGE_NAME_APP1, 0, null);
            // APP1 does not enter FGS state
            try {
                waiter.doWait(WAITFOR_MSEC);
                fail("Service should not enter foreground service state");
            } catch (Exception e) {
            }

            setAppOp(PACKAGE_NAME_APP1, vpnAppOp, true);

            waiter = new WaitForBroadcast(mInstrumentation.getTargetContext());
            waiter.prepare(ACTION_START_FGS_RESULT);
            // Now it can start FGS.
            CommandReceiver.sendCommand(mContext,
                    CommandReceiver.COMMAND_START_FOREGROUND_SERVICE,
                    PACKAGE_NAME_APP1, PACKAGE_NAME_APP1, 0, null);
            uid1Watcher.waitFor(WatchUidRunner.CMD_PROCSTATE, WatchUidRunner.STATE_FG_SERVICE);
            waiter.doWait(WAITFOR_MSEC);
            // Stop the FGS.
            CommandReceiver.sendCommand(mContext,
                    CommandReceiver.COMMAND_STOP_FOREGROUND_SERVICE,
                    PACKAGE_NAME_APP1, PACKAGE_NAME_APP1, 0, null);
            uid1Watcher.waitFor(WatchUidRunner.CMD_PROCSTATE, WatchUidRunner.STATE_CACHED_EMPTY);
        } finally {
            uid1Watcher.finish();
            CtsAppTestUtils.executeShellCmd(mInstrumentation,
                    "appops reset " + PACKAGE_NAME_APP1);
        }
    }

    /**
     * The default behavior for temp allowlist reasonCode REASON_PUSH_MESSAGING_OVER_QUOTA
     * is TEMPORARY_ALLOW_LIST_TYPE_FOREGROUND_SERVICE_NOT_ALLOWED (not allowed to start FGS). But
     * the behavior can be changed by device config command. There are three possible values:
     * {@link TEMPORARY_ALLOW_LIST_TYPE_NONE} (-1):
     *      not temp allowlisted.
     * {@link TEMPORARY_ALLOW_LIST_TYPE_FOREGROUND_SERVICE_ALLOWED} (0):
     *      temp allowlisted and allow FGS.
     * {@link TEMPORARY_ALLOW_LIST_TYPE_FOREGROUND_SERVICE_NOT_ALLOWED} (1):
     *      temp allowlisted, not allow FGS.
     * @throws Exception
     */
    @Test
    public void testPushMessagingOverQuota() throws Exception {
        ApplicationInfo app1Info = mContext.getPackageManager().getApplicationInfo(
                PACKAGE_NAME_APP1, 0);
        WatchUidRunner uid1Watcher = new WatchUidRunner(mInstrumentation, app1Info.uid,
                WAITFOR_MSEC);
        try {
            // Enable the FGS background startForeground() restriction.
            enableFgsRestriction(true, true, null);
            // Default behavior is TEMPORARY_ALLOW_LIST_TYPE_FOREGROUND_SERVICE_NOT_ALLOWED.
            setPushMessagingOverQuotaBehavior(
                    TEMPORARY_ALLOW_LIST_TYPE_FOREGROUND_SERVICE_NOT_ALLOWED);
            // Start FGS in BG state.
            WaitForBroadcast waiter = new WaitForBroadcast(mInstrumentation.getTargetContext());
            waiter.prepare(ACTION_START_FGS_RESULT);
            CommandReceiver.sendCommand(mContext,
                    CommandReceiver.COMMAND_START_FOREGROUND_SERVICE,
                    PACKAGE_NAME_APP1, PACKAGE_NAME_APP1, 0, null);
            // APP1 does not enter FGS state
            try {
                waiter.doWait(WAITFOR_MSEC);
                fail("Service should not enter foreground service state");
            } catch (Exception e) {
            }

            setPushMessagingOverQuotaBehavior(TEMPORARY_ALLOW_LIST_TYPE_NONE);
            waiter = new WaitForBroadcast(mInstrumentation.getTargetContext());
            waiter.prepare(ACTION_START_FGS_RESULT);
            runWithShellPermissionIdentity(() -> {
                mContext.getSystemService(PowerExemptionManager.class).addToTemporaryAllowList(
                        PACKAGE_NAME_APP1, PowerExemptionManager.REASON_PUSH_MESSAGING_OVER_QUOTA,
                        "", TEMP_ALLOWLIST_DURATION_MS);
            });
            CommandReceiver.sendCommand(mContext,
                    CommandReceiver.COMMAND_START_FOREGROUND_SERVICE,
                    PACKAGE_NAME_APP1, PACKAGE_NAME_APP1, 0, null);
            // APP1 does not enter FGS state
            try {
                waiter.doWait(WAITFOR_MSEC);
                fail("Service should not enter foreground service state");
            } catch (Exception e) {
            }

            // Change behavior to TEMPORARY_ALLOW_LIST_TYPE_FOREGROUND_SERVICE_ALLOWED.
            setPushMessagingOverQuotaBehavior(
                    TEMPORARY_ALLOW_LIST_TYPE_FOREGROUND_SERVICE_ALLOWED);
            runWithShellPermissionIdentity(() -> {
                mContext.getSystemService(PowerExemptionManager.class).addToTemporaryAllowList(
                        PACKAGE_NAME_APP1, PowerExemptionManager.REASON_PUSH_MESSAGING_OVER_QUOTA,
                        "", TEMP_ALLOWLIST_DURATION_MS);
            });
            waiter = new WaitForBroadcast(mInstrumentation.getTargetContext());
            waiter.prepare(ACTION_START_FGS_RESULT);
            // Now it can start FGS.
            CommandReceiver.sendCommand(mContext,
                    CommandReceiver.COMMAND_START_FOREGROUND_SERVICE,
                    PACKAGE_NAME_APP1, PACKAGE_NAME_APP1, 0, null);
            uid1Watcher.waitFor(WatchUidRunner.CMD_PROCSTATE, WatchUidRunner.STATE_FG_SERVICE);
            waiter.doWait(WAITFOR_MSEC);
            // Stop the FGS.
            CommandReceiver.sendCommand(mContext,
                    CommandReceiver.COMMAND_STOP_FOREGROUND_SERVICE,
                    PACKAGE_NAME_APP1, PACKAGE_NAME_APP1, 0, null);
            uid1Watcher.waitFor(WatchUidRunner.CMD_PROCSTATE, WatchUidRunner.STATE_CACHED_EMPTY);
        } finally {
            uid1Watcher.finish();
            // Change back to default behavior.
            setPushMessagingOverQuotaBehavior(
                    TEMPORARY_ALLOW_LIST_TYPE_FOREGROUND_SERVICE_NOT_ALLOWED);
            // allow temp allowlist to expire.
            SystemClock.sleep(TEMP_ALLOWLIST_DURATION_MS);
        }
    }

    /**
     * Test temp allowlist reasonCode in BroadcastOptions.
     * When REASON_PUSH_MESSAGING_OVER_QUOTA, DeviceIdleController changes temp allowlist type to
     * TEMPORARY_ALLOWLIST_TYPE_FOREGROUND_SERVICE_NOT_ALLOWED so FGS start is not allowed.
     * When REASON_DENIED (-1), DeviceIdleController changes temp allowlist type to
     * TEMPORARY_ALLOWLIST_TYPE_NONE, the temp allowlist itself is not allowed.
     * All other reason codes, DeviceIdleController does not change temp allowlist type.
     */
    @Test
    public void testTempAllowListReasonCode() throws Exception {
        // FGS start is temp allowed.
        testTempAllowListReasonCodeInternal(REASON_PUSH_MESSAGING);
        // FGS start is not allowed.
        testTempAllowListReasonCodeInternal(REASON_PUSH_MESSAGING_OVER_QUOTA);
        // Temp allowlist itself is not allowed. REASON_DENIED is not exposed in
        // PowerExemptionManager, just use its value "-1" here.
        testTempAllowListReasonCodeInternal(-1);
    }

    private void testTempAllowListReasonCodeInternal(int reasonCode) throws Exception {
        ApplicationInfo app1Info = mContext.getPackageManager().getApplicationInfo(
                PACKAGE_NAME_APP1, 0);
        ApplicationInfo app2Info = mContext.getPackageManager().getApplicationInfo(
                PACKAGE_NAME_APP2, 0);
        WatchUidRunner uid1Watcher = new WatchUidRunner(mInstrumentation, app1Info.uid,
                WAITFOR_MSEC);
        WatchUidRunner uid2Watcher = new WatchUidRunner(mInstrumentation, app2Info.uid,
                WAITFOR_MSEC);
        try {
            // Enable the FGS background startForeground() restriction.
            enableFgsRestriction(true, true, null);
            // Now it can start FGS.
            WaitForBroadcast waiter = new WaitForBroadcast(mInstrumentation.getTargetContext());
            waiter.prepare(ACTION_START_FGS_RESULT);
            runWithShellPermissionIdentity(()-> {
                final BroadcastOptions options = BroadcastOptions.makeBasic();
                // setTemporaryAppAllowlist API requires
                // START_FOREGROUND_SERVICES_FROM_BACKGROUND permission.
                options.setTemporaryAppAllowlist(TEMP_ALLOWLIST_DURATION_MS,
                        TEMPORARY_ALLOW_LIST_TYPE_FOREGROUND_SERVICE_ALLOWED, reasonCode,
                        "");
                // Must use Shell to issue this command because Shell has
                // START_FOREGROUND_SERVICES_FROM_BACKGROUND permission.
                CommandReceiver.sendCommandWithBroadcastOptions(mContext,
                        CommandReceiver.COMMAND_START_FOREGROUND_SERVICE,
                        PACKAGE_NAME_APP1, PACKAGE_NAME_APP2, 0, null,
                        options.toBundle());
            });
            if (reasonCode == REASON_PUSH_MESSAGING) {
                uid2Watcher.waitFor(WatchUidRunner.CMD_PROCSTATE, WatchUidRunner.STATE_FG_SERVICE);
                waiter.doWait(WAITFOR_MSEC);
                // Stop the FGS.
                CommandReceiver.sendCommand(mContext,
                        CommandReceiver.COMMAND_STOP_FOREGROUND_SERVICE,
                        PACKAGE_NAME_APP1, PACKAGE_NAME_APP2, 0, null);
                uid2Watcher.waitFor(WatchUidRunner.CMD_PROCSTATE,
                        WatchUidRunner.STATE_CACHED_EMPTY);
            } else if (reasonCode == REASON_PUSH_MESSAGING_OVER_QUOTA) {
                // APP1 does not enter FGS state
                try {
                    waiter.doWait(WAITFOR_MSEC);
                    fail("Service should not enter foreground service state");
                } catch (Exception e) {
                }
            }
        } finally {
            uid1Watcher.finish();
            uid2Watcher.finish();
            // Sleep to let the temp allowlist expire so it won't affect next test case.
            SystemClock.sleep(TEMP_ALLOWLIST_DURATION_MS);
        }
    }

    /**
     * Test default_input_method is exempted from BG-FGS-start restriction.
     * @throws Exception
     */
    @Test
    public void testFgsStartInputMethod() throws Exception {
        ApplicationInfo app1Info = mContext.getPackageManager().getApplicationInfo(
                PACKAGE_NAME_APP1, 0);
        WatchUidRunner uid1Watcher = new WatchUidRunner(mInstrumentation, app1Info.uid,
                WAITFOR_MSEC);
        final String defaultInputMethod = CtsAppTestUtils.executeShellCmd(mInstrumentation,
                "settings get --user current secure default_input_method");
        try {
            // Enable the FGS background startForeground() restriction.
            enableFgsRestriction(true, true, null);
            // Start FGS in BG state.
            WaitForBroadcast waiter = new WaitForBroadcast(mInstrumentation.getTargetContext());
            waiter.prepare(ACTION_START_FGS_RESULT);
            CommandReceiver.sendCommand(mContext,
                    CommandReceiver.COMMAND_START_FOREGROUND_SERVICE,
                    PACKAGE_NAME_APP1, PACKAGE_NAME_APP1, 0, null);
            // APP1 does not enter FGS state
            try {
                waiter.doWait(WAITFOR_MSEC);
                fail("Service should not enter foreground service state");
            } catch (Exception e) {
            }

            // Change default_input_method to PACKAGE_NAME_APP1.
            final ComponentName cn = new ComponentName(PACKAGE_NAME_APP1, "xxx");
            CtsAppTestUtils.executeShellCmd(mInstrumentation,
                    "settings put --user current secure default_input_method "
                            + cn.flattenToShortString());

            waiter = new WaitForBroadcast(mInstrumentation.getTargetContext());
            waiter.prepare(ACTION_START_FGS_RESULT);
            // Now it can start FGS.
            CommandReceiver.sendCommand(mContext,
                    CommandReceiver.COMMAND_START_FOREGROUND_SERVICE,
                    PACKAGE_NAME_APP1, PACKAGE_NAME_APP1, 0, null);
            uid1Watcher.waitFor(WatchUidRunner.CMD_PROCSTATE, WatchUidRunner.STATE_FG_SERVICE);
            waiter.doWait(WAITFOR_MSEC);
            // Stop the FGS.
            CommandReceiver.sendCommand(mContext,
                    CommandReceiver.COMMAND_STOP_FOREGROUND_SERVICE,
                    PACKAGE_NAME_APP1, PACKAGE_NAME_APP1, 0, null);
            uid1Watcher.waitFor(WatchUidRunner.CMD_PROCSTATE, WatchUidRunner.STATE_CACHED_EMPTY);
        } finally {
            uid1Watcher.finish();
            CtsAppTestUtils.executeShellCmd(mInstrumentation,
                    "settings put --user current secure default_input_method "
                            + defaultInputMethod);
        }
    }

    /**
     * Turn on the FGS BG-launch restriction. DeviceConfig can turn on restriction on the whole
     * device (across all apps). AppCompat can turn on restriction on a single app package.
     * @param enable true to turn on restriction, false to turn off.
     * @param useDeviceConfig true to use DeviceConfig, false to use AppCompat CHANGE ID.
     * @param packageName the packageName if using AppCompat CHANGE ID.
     * @throws Exception
     */
    private void enableFgsRestriction(boolean enable, boolean useDeviceConfig, String packageName)
            throws Exception {
        if (useDeviceConfig) {
            runWithShellPermissionIdentity(() -> {
                        DeviceConfig.setProperty("activity_manager",
                                KEY_DEFAULT_FGS_STARTS_RESTRICTION_ENABLED,
                                Boolean.toString(enable), false);
                    }
            );
        } else {
            CtsAppTestUtils.executeShellCmd(mInstrumentation,
                    "am compat " + (enable ? "enable" : "disable")
                            + " FGS_BG_START_RESTRICTION_CHANGE_ID " + packageName);
        }
    }

    /**
     * Clean up the FGS BG-launch restriction.
     * @param packageName the packageName that will have its changeid override reset.
     * @throws Exception
     */
    private void resetFgsRestriction(String packageName)
            throws Exception {
        CtsAppTestUtils.executeShellCmd(mInstrumentation,
                "am compat reset FGS_BG_START_RESTRICTION_CHANGE_ID " + packageName);
    }

    /**
     * SYSTEM_ALERT_WINDOW permission will allow both BG-activity start and BG-FGS start.
     * Some cases we want to grant this permission to allow activity start to bring the app up to
     * TOP state.
     * Some cases we want to revoke this permission to test other BG-FGS-launch exemptions.
     * @param packageName
     * @param allow
     * @throws Exception
     */
    private void allowBgActivityStart(String packageName, boolean allow) throws Exception {
        if (allow) {
            PermissionUtils.grantPermission(
                    packageName, android.Manifest.permission.SYSTEM_ALERT_WINDOW);
        } else {
            PermissionUtils.revokePermission(
                    packageName, android.Manifest.permission.SYSTEM_ALERT_WINDOW);
        }
    }

    private void setFgsStartForegroundTimeout(int timeoutMs) throws Exception {
        runWithShellPermissionIdentity(() -> {
                    DeviceConfig.setProperty("activity_manager",
                            KEY_FGS_START_FOREGROUND_TIMEOUT,
                            Integer.toString(timeoutMs), false);
                }
        );
    }

    private void setAppOp(String packageName, String opStr, boolean allow) throws Exception {
        CtsAppTestUtils.executeShellCmd(mInstrumentation,
                "appops set " + packageName + " " + opStr + " "
                        + (allow ? "allow" : "deny"));
    }

    private void setPushMessagingOverQuotaBehavior(
            /* @PowerExemptionManager.TempAllowListType */ int type) throws Exception {
        runWithShellPermissionIdentity(() -> {
                    DeviceConfig.setProperty("activity_manager",
                            KEY_PUSH_MESSAGING_OVER_QUOTA_BEHAVIOR,
                            Integer.toString(type), false);
                }
        );
    }
}
