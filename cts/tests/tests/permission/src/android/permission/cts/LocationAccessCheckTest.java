/*
 * Copyright (C) 2018 The Android Open Source Project
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

package android.permission.cts;

import static android.Manifest.permission.ACCESS_BACKGROUND_LOCATION;
import static android.Manifest.permission.ACCESS_FINE_LOCATION;
import static android.app.AppOpsManager.OPSTR_FINE_LOCATION;
import static android.app.AppOpsManager.OP_FLAGS_ALL_TRUSTED;
import static android.app.Notification.EXTRA_TITLE;
import static android.content.Context.BIND_AUTO_CREATE;
import static android.content.Context.BIND_NOT_FOREGROUND;
import static android.content.Intent.ACTION_BOOT_COMPLETED;
import static android.content.Intent.FLAG_RECEIVER_FOREGROUND;
import static android.location.Criteria.ACCURACY_FINE;
import static android.os.Process.myUserHandle;
import static android.provider.Settings.Secure.LOCATION_ACCESS_CHECK_DELAY_MILLIS;
import static android.provider.Settings.Secure.LOCATION_ACCESS_CHECK_INTERVAL_MILLIS;

import static com.android.compatibility.common.util.SystemUtil.runShellCommand;
import static com.android.compatibility.common.util.SystemUtil.runWithShellPermissionIdentity;
import static com.android.server.job.nano.JobPackageHistoryProto.START_PERIODIC_JOB;
import static com.android.server.job.nano.JobPackageHistoryProto.STOP_JOB;

import static org.junit.Assert.assertFalse;
import static org.junit.Assert.assertNotEquals;
import static org.junit.Assert.assertNotNull;
import static org.junit.Assert.assertNull;
import static org.junit.Assert.assertTrue;
import static org.junit.Assert.fail;
import static org.junit.Assume.assumeFalse;
import static org.junit.Assume.assumeTrue;

import static java.lang.Math.max;
import static java.util.concurrent.TimeUnit.MILLISECONDS;

import android.app.ActivityManager;
import android.app.AppOpsManager;
import android.app.UiAutomation;
import android.content.ComponentName;
import android.content.ContentResolver;
import android.content.Context;
import android.content.Intent;
import android.content.ServiceConnection;
import android.content.pm.PackageManager;
import android.content.pm.ResolveInfo;
import android.location.Criteria;
import android.location.Location;
import android.location.LocationListener;
import android.location.LocationManager;
import android.os.Bundle;
import android.os.IBinder;
import android.os.Looper;
import android.permission.cts.appthataccesseslocation.IAccessLocationOnCommand;
import android.platform.test.annotations.AppModeFull;
import android.platform.test.annotations.AsbSecurityTest;
import android.platform.test.annotations.SystemUserOnly;
import android.provider.DeviceConfig;
import android.provider.Settings;
import android.service.notification.NotificationListenerService;
import android.service.notification.StatusBarNotification;
import android.util.Log;

import androidx.annotation.NonNull;
import androidx.annotation.Nullable;
import androidx.test.InstrumentationRegistry;
import androidx.test.runner.AndroidJUnit4;

import com.android.compatibility.common.util.DeviceConfigStateHelper;
import com.android.compatibility.common.util.ProtoUtils;
import com.android.compatibility.common.util.mainline.MainlineModule;
import com.android.compatibility.common.util.mainline.ModuleDetector;
import com.android.server.job.nano.JobPackageHistoryProto;
import com.android.server.job.nano.JobSchedulerServiceDumpProto;
import com.android.server.job.nano.JobSchedulerServiceDumpProto.RegisteredJob;

import org.junit.After;
import org.junit.AfterClass;
import org.junit.Before;
import org.junit.BeforeClass;
import org.junit.Test;
import org.junit.runner.RunWith;

import java.util.Arrays;
import java.util.List;
import java.util.concurrent.CountDownLatch;

/**
 * Tests the {@code LocationAccessCheck} in permission controller.
 */
@RunWith(AndroidJUnit4.class)
@AppModeFull(reason = "Cannot set system settings as instant app. Also we never show a location "
        + "access check notification for instant apps.")
public class LocationAccessCheckTest {
    private static final String LOG_TAG = LocationAccessCheckTest.class.getSimpleName();

    private static final String TEST_APP_PKG = "android.permission.cts.appthataccesseslocation";
    private static final String TEST_APP_LABEL = "CtsLocationAccess";
    private static final String TEST_APP_SERVICE = TEST_APP_PKG + ".AccessLocationOnCommand";
    private static final String TEST_APP_LOCATION_BG_ACCESS_APK =
            "/data/local/tmp/cts/permissions/CtsAppThatAccessesLocationOnCommand.apk";
    private static final String TEST_APP_LOCATION_FG_ACCESS_APK =
            "/data/local/tmp/cts/permissions/AppThatDoesNotHaveBgLocationAccess.apk";
    private static final int LOCATION_ACCESS_CHECK_JOB_ID = 0;

    /** Whether to show location access check notifications. */
    private static final String PROPERTY_LOCATION_ACCESS_CHECK_ENABLED =
            "location_access_check_enabled";

    private static final long UNEXPECTED_TIMEOUT_MILLIS = 10000;
    private static final long EXPECTED_TIMEOUT_MILLIS = 15000;
    private static final long LOCATION_ACCESS_TIMEOUT_MILLIS = 15000;

    private static final Context sContext = InstrumentationRegistry.getTargetContext();
    private static final ActivityManager sActivityManager =
            sContext.getSystemService(ActivityManager.class);
    private static final PackageManager sPackageManager = sContext.getPackageManager();
    private static final AppOpsManager sAppOpsManager =
            sContext.getSystemService(AppOpsManager.class);
    private static final UiAutomation sUiAutomation = InstrumentationRegistry.getInstrumentation()
            .getUiAutomation();

    private static final String PERMISSION_CONTROLLER_PKG = sContext.getPackageManager()
            .getPermissionControllerPackageName();

    /**
     * The result of {@link #assumeCanGetFineLocation()}, so we don't have to run it over and over
     * again.
     */
    private static Boolean sCanAccessFineLocation = null;

    private static ServiceConnection sConnection;
    private static IAccessLocationOnCommand sLocationAccessor;

    private DeviceConfigStateHelper mPrivacyDeviceConfig =
            new DeviceConfigStateHelper(DeviceConfig.NAMESPACE_PRIVACY);
    private static DeviceConfigStateHelper sJobSchedulerDeviceConfig =
            new DeviceConfigStateHelper(DeviceConfig.NAMESPACE_JOB_SCHEDULER);

    private static void assumeNotPlayManaged() throws Exception {
        assumeFalse(ModuleDetector.moduleIsPlayManaged(
                sContext.getPackageManager(), MainlineModule.PERMISSION_CONTROLLER));
    }

    /**
     * Connected to {@value #TEST_APP_PKG} and make it access the location in the background
     */
    private void accessLocation() throws Throwable {
        if (sConnection == null || sLocationAccessor == null) {
            bindService();
        }

        long beforeAccess = System.currentTimeMillis();
        // Wait a little to avoid raciness in timing between threads
        Thread.sleep(1000);

        // Try again until binder call goes though. It might not go through if the sLocationAccessor
        // is not bound yet
        eventually(() -> {
            assertNotNull(sLocationAccessor);
            sLocationAccessor.accessLocation();
        }, EXPECTED_TIMEOUT_MILLIS);

        // Wait until the access is recorded
        eventually(() -> {
            List<AppOpsManager.PackageOps> ops = runWithShellPermissionIdentity(
                    () -> sAppOpsManager.getOpsForPackage(
                            sPackageManager.getPackageUid(TEST_APP_PKG, 0), TEST_APP_PKG,
                            OPSTR_FINE_LOCATION));

            // Background access must have happened after "beforeAccess"
            assertTrue(ops.get(0).getOps().get(0).getLastAccessBackgroundTime(OP_FLAGS_ALL_TRUSTED)
                    >= beforeAccess);
        }, EXPECTED_TIMEOUT_MILLIS);
    }

    /**
     * A {@link java.util.concurrent.Callable} that can throw a {@link Throwable}
     */
    private interface ThrowingCallable<T> {
        T call() throws Throwable;
    }

    /**
     * A {@link Runnable} that can throw a {@link Throwable}
     */
    private interface ThrowingRunnable {
        void run() throws Throwable;
    }

    /**
     * Make sure that a {@link ThrowingRunnable} eventually finishes without throwing a {@link
     * Exception}.
     *
     * @param r       The {@link ThrowingRunnable} to run.
     * @param timeout the maximum time to wait
     */
    public static void eventually(@NonNull ThrowingRunnable r, long timeout) throws Throwable {
        eventually(() -> {
            r.run();
            return 0;
        }, timeout);
    }

    /**
     * Make sure that a {@link ThrowingCallable} eventually finishes without throwing a {@link
     * Exception}.
     *
     * @param r       The {@link ThrowingCallable} to run.
     * @param timeout the maximum time to wait
     * @return the return value from the callable
     * @throws NullPointerException If the return value never becomes non-null
     */
    public static <T> T eventually(@NonNull ThrowingCallable<T> r, long timeout) throws Throwable {
        long start = System.currentTimeMillis();

        while (true) {
            try {
                T res = r.call();
                if (res == null) {
                    throw new NullPointerException("No result");
                }

                return res;
            } catch (Throwable e) {
                if (System.currentTimeMillis() - start < timeout) {
                    Log.d(LOG_TAG, "Ignoring exception", e);

                    Thread.sleep(500);
                } else {
                    throw e;
                }
            }
        }
    }

    /**
     * Get the state of the job scheduler
     */
    public static JobSchedulerServiceDumpProto getJobSchedulerDump() throws Exception {
        return ProtoUtils.getProto(sUiAutomation, JobSchedulerServiceDumpProto.class,
                ProtoUtils.DUMPSYS_JOB_SCHEDULER);
    }

    /**
     * Clear all data of a package including permissions and files.
     *
     * @param pkg The name of the package to be cleared
     */
    private static void clearPackageData(@NonNull String pkg) {
        unbindService();
        runShellCommand("pm clear --user -2 " + pkg);
    }

    /**
     * Get the last time the LOCATION_ACCESS_CHECK_JOB_ID job was started/stopped for permission
     * controller.
     *
     * @param event the job event (start/stop)
     *
     * @return the last time the event happened.
     */
    private static long getLastJobTime(int event) throws Exception {
        int permControllerUid = sPackageManager.getPackageUid(PERMISSION_CONTROLLER_PKG, 0);

        long lastTime = -1;

        for (JobPackageHistoryProto.HistoryEvent historyEvent :
                getJobSchedulerDump().history.historyEvent) {
            if (historyEvent.uid == permControllerUid
                    && historyEvent.jobId == LOCATION_ACCESS_CHECK_JOB_ID
                    && historyEvent.event == event) {
                lastTime = max(lastTime,
                        System.currentTimeMillis() - historyEvent.timeSinceEventMs);
            }
        }

        return lastTime;
    }

    /**
     * Force a run of the location check.
     */
    private static void runLocationCheck() throws Throwable {
        // Sleep a little bit to make sure we don't have overlap in timing
        Thread.sleep(1000);

        long beforeJob = System.currentTimeMillis();

        // Sleep a little bit to avoid raciness in time keeping
        Thread.sleep(1000);

        runShellCommand(
                "cmd jobscheduler run -u " + android.os.Process.myUserHandle().getIdentifier()
                        + " -f " + PERMISSION_CONTROLLER_PKG + " 0");

        eventually(() -> {
            long startTime = getLastJobTime(START_PERIODIC_JOB);
            assertTrue(startTime + " !> " + beforeJob, startTime > beforeJob);
        }, EXPECTED_TIMEOUT_MILLIS);

        // We can't simply require startTime <= endTime because the time being reported isn't
        // accurate, and sometimes the end time may come before the start time by around 100 ms.
        eventually(() -> {
            long stopTime = getLastJobTime(STOP_JOB);
            assertTrue(stopTime + " !> " + beforeJob, stopTime > beforeJob);
        }, EXPECTED_TIMEOUT_MILLIS);
    }

    /**
     * Get a notification thrown by the permission controller that is currently visible.
     *
     * @return The notification or {@code null} if there is none
     */
    private @Nullable StatusBarNotification getPermissionControllerNotification() throws Exception {
        NotificationListenerService notificationService = NotificationListener.getInstance();

        for (StatusBarNotification notification : notificationService.getActiveNotifications()) {
            if (notification.getPackageName().equals(PERMISSION_CONTROLLER_PKG)) {
                return notification;
            }
        }

        return null;
    }

    /**
     * Get a location access notification that is currently visible.
     *
     * @param cancelNotification if {@code true} the notification is canceled inside this method
     * @return The notification or {@code null} if there is none
     */
    private StatusBarNotification getNotification(boolean cancelNotification) throws Throwable {
        NotificationListenerService notificationService = NotificationListener.getInstance();

        StatusBarNotification notification = getPermissionControllerNotification();
        if (notification == null) {
            return null;
        }

        if (notification.getNotification().extras.getString(EXTRA_TITLE, "")
                .contains(TEST_APP_LABEL)) {
            if (cancelNotification) {
                notificationService.cancelNotification(notification.getKey());

                // Wait for notification to get canceled
                eventually(() -> assertFalse(
                        Arrays.asList(notificationService.getActiveNotifications()).contains(
                                notification)), UNEXPECTED_TIMEOUT_MILLIS);
            }

            return notification;
        }

        Log.d(LOG_TAG, "Bad notification " + notification);

        return null;
    }

    /**
     * Grant a permission to the {@value #TEST_APP_PKG}.
     *
     * @param permission The permission to grant
     */
    private void grantPermissionToTestApp(@NonNull String permission) {
        sUiAutomation.grantRuntimePermission(TEST_APP_PKG, permission);
    }

    /**
     * Register {@link NotificationListener}.
     */
    @BeforeClass
    public static void allowNotificationAccess() {
        runShellCommand("cmd notification allow_listener " + (new ComponentName(sContext,
                NotificationListener.class).flattenToString()));
    }

    /**
     * Change settings so that permission controller can show location access notifications more
     * often.
     */
    @BeforeClass
    public static void reduceDelays() {
        runWithShellPermissionIdentity(() -> {
            ContentResolver cr = sContext.getContentResolver();

            // New settings will be applied in when permission controller is reset
            Settings.Secure.putLong(cr, LOCATION_ACCESS_CHECK_INTERVAL_MILLIS, 100);
            Settings.Secure.putLong(cr, LOCATION_ACCESS_CHECK_DELAY_MILLIS, 50);

            // Disable job scheduler throttling by allowing 300000 jobs per 30 sec
            sJobSchedulerDeviceConfig.set("qc_max_job_count_per_rate_limiting_window", "3000000");
            sJobSchedulerDeviceConfig.set("qc_rate_limiting_window_ms", "30000");
        });
    }

    @BeforeClass
    public static void installBackgroundAccessApp() throws Exception {
        installBackgroundAccessApp(false);
    }

    private static void installBackgroundAccessApp(boolean isDowngrade) throws Exception {
        String command = "pm install -r -g ";
        if (isDowngrade) {
            command = command + "-d ";
        }
        String output = runShellCommand(command + TEST_APP_LOCATION_BG_ACCESS_APK);
        assertTrue(output.contains("Success"));
        // Wait for user sensitive to be updated, which is checked by LocationAccessCheck.
        Thread.sleep(5000);
    }

    @AfterClass
    public static void uninstallBackgroundAccessApp() {
        unbindService();
        runShellCommand("pm uninstall " + TEST_APP_PKG);
    }

    private static void unbindService() {
        if (sConnection != null) {
            sContext.unbindService(sConnection);
        }
        sConnection = null;
        sLocationAccessor = null;
    }


    private static void installForegroundAccessApp() throws Exception {
        unbindService();
        runShellCommand("pm install -r -g " + TEST_APP_LOCATION_FG_ACCESS_APK);
        // Wait for user sensitive to be updated, which is checked by LocationAccessCheck.
        Thread.sleep(5000);
    }

    private static void uninstallForegroundAccessApp() {
        runShellCommand("pm uninstall " + TEST_APP_LOCATION_FG_ACCESS_APK);
    }

    /**
     * Skip each test for low ram device
     */
    @Before
    public void assumeIsNotLowRamDevice() {
        assumeFalse(sActivityManager.isLowRamDevice());
    }

    @Before
    public void wakeUpAndDismissKeyguard() {
        runShellCommand("input keyevent KEYCODE_WAKEUP");
        runShellCommand("wm dismiss-keyguard");
    }

    @Before
    public void bindService() {
        sConnection = new ServiceConnection() {
            @Override
            public void onServiceConnected(ComponentName name, IBinder service) {
                sLocationAccessor = IAccessLocationOnCommand.Stub.asInterface(service);
            }

            @Override
            public void onServiceDisconnected(ComponentName name) {
                sConnection = null;
                sLocationAccessor = null;
            }
        };

        Intent testAppService = new Intent();
        testAppService.setComponent(new ComponentName(TEST_APP_PKG, TEST_APP_SERVICE));

        sContext.bindService(testAppService, sConnection, BIND_AUTO_CREATE | BIND_NOT_FOREGROUND);
    }

    /**
     * Reset the permission controllers state before each test
     */
    @Before
    public void resetPermissionControllerBeforeEachTest() throws Throwable {
        // Has to be before resetPermissionController to make sure enablement time is the reset time
        // of permission controller
        enableLocationAccessCheck();

        resetPermissionController();

        eventually(() -> assertNull(getNotification(false)), UNEXPECTED_TIMEOUT_MILLIS);

        // Reset job scheduler stats (to allow more jobs to be run)
        runShellCommand(
                "cmd jobscheduler reset-execution-quota -u " + myUserHandle().getIdentifier() + " "
                        + PERMISSION_CONTROLLER_PKG);
    }

    /**
     * Enable location access check
     */
    public void enableLocationAccessCheck() throws Throwable {
        mPrivacyDeviceConfig.set(PROPERTY_LOCATION_ACCESS_CHECK_ENABLED, "true");

        // Run a location access check to update enabled state inside permission controller
        runLocationCheck();
    }

    /**
     * Disable location access check
     */
    private void disableLocationAccessCheck() throws Throwable {
        mPrivacyDeviceConfig.set(PROPERTY_LOCATION_ACCESS_CHECK_ENABLED, "false");

        // Run a location access check to update enabled state inside permission controller
        runLocationCheck();
    }

    /**
     * Make sure fine location can be accessed at all.
     */
    @Before
    public void assumeCanGetFineLocation() {
        if (sCanAccessFineLocation == null) {
            Criteria crit = new Criteria();
            crit.setAccuracy(ACCURACY_FINE);

            CountDownLatch locationCounter = new CountDownLatch(1);
            sContext.getSystemService(LocationManager.class).requestSingleUpdate(crit,
                    new LocationListener() {
                        @Override
                        public void onLocationChanged(Location location) {
                            locationCounter.countDown();
                        }

                        @Override
                        public void onStatusChanged(String provider, int status, Bundle extras) {
                        }

                        @Override
                        public void onProviderEnabled(String provider) {
                        }

                        @Override
                        public void onProviderDisabled(String provider) {
                        }
                    }, Looper.getMainLooper());


            try {
                sCanAccessFineLocation = locationCounter.await(LOCATION_ACCESS_TIMEOUT_MILLIS,
                        MILLISECONDS);
            } catch (InterruptedException ignored) {
            }
        }

        assumeTrue(sCanAccessFineLocation);
    }

    /**
     * Reset the permission controllers state.
     */
    private static void resetPermissionController() throws Throwable {
        clearPackageData(PERMISSION_CONTROLLER_PKG);
        int currentUserId = myUserHandle().getIdentifier();

        // Wait until jobs are cleared
        eventually(() -> {
            JobSchedulerServiceDumpProto dump = getJobSchedulerDump();

            for (RegisteredJob job : dump.registeredJobs) {
                if (job.dump.sourceUserId == currentUserId) {
                    assertNotEquals(job.dump.sourcePackageName, PERMISSION_CONTROLLER_PKG);
                }
            }
        }, UNEXPECTED_TIMEOUT_MILLIS);

        // Setup up permission controller again (simulate a reboot)
        Intent permissionControllerSetupIntent = null;
        for (ResolveInfo ri : sContext.getPackageManager().queryBroadcastReceivers(
                new Intent(ACTION_BOOT_COMPLETED), 0)) {
            String pkg = ri.activityInfo.packageName;

            if (pkg.equals(PERMISSION_CONTROLLER_PKG)) {
                permissionControllerSetupIntent = new Intent()
                        .setClassName(pkg, ri.activityInfo.name)
                        .setFlags(FLAG_RECEIVER_FOREGROUND)
                        .setPackage(PERMISSION_CONTROLLER_PKG);

                sContext.sendBroadcast(permissionControllerSetupIntent);
            }
        }

        // Wait until jobs are set up
        eventually(() -> {
            JobSchedulerServiceDumpProto dump = getJobSchedulerDump();

            for (RegisteredJob job : dump.registeredJobs) {
                if (job.dump.sourceUserId == currentUserId
                        && job.dump.sourcePackageName.equals(PERMISSION_CONTROLLER_PKG)
                        && job.dump.jobInfo.service.className.contains("LocationAccessCheck")) {
                    return;
                }
            }

            fail("Permission controller jobs not found");
        }, UNEXPECTED_TIMEOUT_MILLIS);
    }

    /**
     * Unregister {@link NotificationListener}.
     */
    @AfterClass
    public static void disallowNotificationAccess() {
        runShellCommand("cmd notification disallow_listener " + (new ComponentName(sContext,
                NotificationListener.class)).flattenToString());
    }

    /**
     * Reset settings so that permission controller runs normally.
     */
    @AfterClass
    public static void resetDelays() throws Throwable {
        runWithShellPermissionIdentity(() -> {
            ContentResolver cr = sContext.getContentResolver();

            Settings.Secure.resetToDefaults(cr, LOCATION_ACCESS_CHECK_INTERVAL_MILLIS);
            Settings.Secure.resetToDefaults(cr, LOCATION_ACCESS_CHECK_DELAY_MILLIS);

            sJobSchedulerDeviceConfig.restoreOriginalValues();
        });
    }

    /**
     * Reset location access check
     */
    @After
    public void resetPrivacyConfig() throws Throwable {
        mPrivacyDeviceConfig.restoreOriginalValues();

        // Run a location access check to update enabled state inside permission controller
        runLocationCheck();
    }

    @After
    public void locationUnbind() throws Throwable {
        unbindService();
    }

    @Test
    public void notificationIsShown() throws Throwable {
        accessLocation();
        runLocationCheck();

        eventually(() -> assertNotNull(getNotification(true)), EXPECTED_TIMEOUT_MILLIS);
    }

    @Test
    @AsbSecurityTest(cveBugId = 141028068)
    public void notificationIsShownOnlyOnce() throws Throwable {
        assumeNotPlayManaged();

        accessLocation();
        runLocationCheck();

        eventually(() -> assertNotNull(getNotification(true)), EXPECTED_TIMEOUT_MILLIS);

        accessLocation();
        runLocationCheck();

        assertNull(getNotification(true));
    }

    @SystemUserOnly(reason = "b/172259935")
    @Test
    @AsbSecurityTest(cveBugId = 141028068)
    public void notificationIsShownAgainAfterClear() throws Throwable {
        assumeNotPlayManaged();
        accessLocation();
        runLocationCheck();

        eventually(() -> assertNotNull(getNotification(true)), EXPECTED_TIMEOUT_MILLIS);

        clearPackageData(TEST_APP_PKG);

        // Wait until package is cleared and permission controller has cleared the state
        Thread.sleep(10000);

        // Clearing removed the permissions, hence grant them again
        grantPermissionToTestApp(ACCESS_FINE_LOCATION);
        grantPermissionToTestApp(ACCESS_BACKGROUND_LOCATION);

        accessLocation();
        runLocationCheck();

        eventually(() -> assertNotNull(getNotification(true)), EXPECTED_TIMEOUT_MILLIS);
    }

    @SystemUserOnly(reason = "b/172259935")
    @Test
    public void notificationIsShownAgainAfterUninstallAndReinstall() throws Throwable {
        accessLocation();
        runLocationCheck();

        eventually(() -> assertNotNull(getNotification(true)), EXPECTED_TIMEOUT_MILLIS);

        uninstallBackgroundAccessApp();

        // Wait until package permission controller has cleared the state
        Thread.sleep(2000);

        installBackgroundAccessApp();

        accessLocation();
        runLocationCheck();

        eventually(() -> assertNotNull(getNotification(true)), EXPECTED_TIMEOUT_MILLIS);
    }

    @Test
    @AsbSecurityTest(cveBugId = 141028068)
    public void removeNotificationOnUninstall() throws Throwable {
        assumeNotPlayManaged();

        accessLocation();
        runLocationCheck();

        eventually(() -> assertNotNull(getNotification(false)), EXPECTED_TIMEOUT_MILLIS);

        uninstallBackgroundAccessApp();

        try {
            eventually(() -> assertNull(getNotification(false)), UNEXPECTED_TIMEOUT_MILLIS);
        } finally {
            installBackgroundAccessApp();
            getNotification(true);
        }
    }

    @Test
    public void notificationIsNotShownAfterAppDoesNotRequestLocationAnymore() throws Throwable {
        accessLocation();
        runLocationCheck();

        eventually(() -> assertNotNull(getNotification(true)), EXPECTED_TIMEOUT_MILLIS);

        // Update to app to a version that does not request permission anymore
        installForegroundAccessApp();

        try {
            resetPermissionController();

            runLocationCheck();

            // We don't expect a notification, but try to trigger one anyway
            assertNull(getNotification(false));
        } finally {
            installBackgroundAccessApp(true);
        }
    }

    @Test
    @AsbSecurityTest(cveBugId = 141028068)
    public void noNotificationIfFeatureDisabled() throws Throwable {
        assumeNotPlayManaged();

        disableLocationAccessCheck();

        accessLocation();
        runLocationCheck();

        assertNull(getNotification(false));
    }

    @Test
    @AsbSecurityTest(cveBugId = 141028068)
    public void notificationOnlyForAccessesSinceFeatureWasEnabled() throws Throwable {
        assumeNotPlayManaged();

        disableLocationAccessCheck();

        accessLocation();
        runLocationCheck();

        // No notification expected for accesses before enabling the feature
        assertNull(getNotification(false));

        enableLocationAccessCheck();

        // Trigger update of location enable time. In the real world it enabling happens on the
        // first location check. I.e. accesses before this location check are ignored.
        runLocationCheck();

        // No notification expected for accesses before enabling the feature (even after feature is
        // enabled now)
        assertNull(getNotification(false));

        // Notification expected for access after enabling the feature
        accessLocation();
        runLocationCheck();

        eventually(() -> assertNotNull(getNotification(true)), EXPECTED_TIMEOUT_MILLIS);
    }

    @Test
    @AsbSecurityTest(cveBugId = 141028068)
    public void noNotificationIfBlamerNotSystemOrLocationProvider() throws Throwable {
        assumeNotPlayManaged();

        // Blame the app for access from an untrusted for notification purposes package.
        runWithShellPermissionIdentity(() -> {
            AppOpsManager appOpsManager = sContext.getSystemService(AppOpsManager.class);
            appOpsManager.noteProxyOpNoThrow(OPSTR_FINE_LOCATION, TEST_APP_PKG,
                    sContext.getPackageManager().getPackageUid(TEST_APP_PKG, 0));
        });
        runLocationCheck();

        assertNull(getNotification(false));
    }

    @Test
    @AsbSecurityTest(cveBugId = 141028068)
    public void testOpeningLocationSettingsDoesNotTriggerAccess() throws Throwable {
        assumeNotPlayManaged();

        Intent intent = new Intent(Settings.ACTION_LOCATION_SOURCE_SETTINGS);
        intent.addFlags(Intent.FLAG_ACTIVITY_NEW_TASK);
        sContext.startActivity(intent);

        runLocationCheck();
        assertNull(getNotification(false));
    }
}
