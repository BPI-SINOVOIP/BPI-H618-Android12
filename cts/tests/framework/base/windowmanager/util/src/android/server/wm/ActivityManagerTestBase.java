/*
 * Copyright (C) 2016 The Android Open Source Project
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

package android.server.wm;

import static android.app.AppOpsManager.MODE_ALLOWED;
import static android.app.AppOpsManager.OPSTR_SYSTEM_ALERT_WINDOW;
import static android.app.Instrumentation.ActivityMonitor;
import static android.app.WindowConfiguration.ACTIVITY_TYPE_ASSISTANT;
import static android.app.WindowConfiguration.ACTIVITY_TYPE_RECENTS;
import static android.app.WindowConfiguration.ACTIVITY_TYPE_STANDARD;
import static android.app.WindowConfiguration.ACTIVITY_TYPE_UNDEFINED;
import static android.app.WindowConfiguration.WINDOWING_MODE_UNDEFINED;
import static android.content.Intent.ACTION_MAIN;
import static android.content.Intent.CATEGORY_HOME;
import static android.content.Intent.FLAG_ACTIVITY_MULTIPLE_TASK;
import static android.content.Intent.FLAG_ACTIVITY_NEW_TASK;
import static android.content.pm.PackageManager.COMPONENT_ENABLED_STATE_DISABLED;
import static android.content.pm.PackageManager.COMPONENT_ENABLED_STATE_ENABLED;
import static android.content.pm.PackageManager.DONT_KILL_APP;
import static android.content.pm.PackageManager.FEATURE_ACTIVITIES_ON_SECONDARY_DISPLAYS;
import static android.content.pm.PackageManager.FEATURE_AUTOMOTIVE;
import static android.content.pm.PackageManager.FEATURE_EMBEDDED;
import static android.content.pm.PackageManager.FEATURE_FREEFORM_WINDOW_MANAGEMENT;
import static android.content.pm.PackageManager.FEATURE_INPUT_METHODS;
import static android.content.pm.PackageManager.FEATURE_LEANBACK;
import static android.content.pm.PackageManager.FEATURE_PICTURE_IN_PICTURE;
import static android.content.pm.PackageManager.FEATURE_SCREEN_LANDSCAPE;
import static android.content.pm.PackageManager.FEATURE_SCREEN_PORTRAIT;
import static android.content.pm.PackageManager.FEATURE_SECURE_LOCK_SCREEN;
import static android.content.pm.PackageManager.FEATURE_TELEVISION;
import static android.content.pm.PackageManager.FEATURE_VR_MODE_HIGH_PERFORMANCE;
import static android.content.pm.PackageManager.FEATURE_WATCH;
import static android.content.pm.PackageManager.MATCH_DEFAULT_ONLY;
import static android.server.wm.ActivityLauncher.KEY_ACTIVITY_TYPE;
import static android.server.wm.ActivityLauncher.KEY_DISPLAY_ID;
import static android.server.wm.ActivityLauncher.KEY_INTENT_EXTRAS;
import static android.server.wm.ActivityLauncher.KEY_INTENT_FLAGS;
import static android.server.wm.ActivityLauncher.KEY_LAUNCH_ACTIVITY;
import static android.server.wm.ActivityLauncher.KEY_LAUNCH_TASK_BEHIND;
import static android.server.wm.ActivityLauncher.KEY_LAUNCH_TO_SIDE;
import static android.server.wm.ActivityLauncher.KEY_MULTIPLE_INSTANCES;
import static android.server.wm.ActivityLauncher.KEY_MULTIPLE_TASK;
import static android.server.wm.ActivityLauncher.KEY_NEW_TASK;
import static android.server.wm.ActivityLauncher.KEY_RANDOM_DATA;
import static android.server.wm.ActivityLauncher.KEY_REORDER_TO_FRONT;
import static android.server.wm.ActivityLauncher.KEY_SUPPRESS_EXCEPTIONS;
import static android.server.wm.ActivityLauncher.KEY_TARGET_COMPONENT;
import static android.server.wm.ActivityLauncher.KEY_USE_APPLICATION_CONTEXT;
import static android.server.wm.ActivityLauncher.KEY_WINDOWING_MODE;
import static android.server.wm.ActivityLauncher.launchActivityFromExtras;
import static android.server.wm.CommandSession.KEY_FORWARD;
import static android.server.wm.ComponentNameUtils.getActivityName;
import static android.server.wm.ComponentNameUtils.getLogTag;
import static android.server.wm.StateLogger.log;
import static android.server.wm.StateLogger.logE;
import static android.server.wm.UiDeviceUtils.pressBackButton;
import static android.server.wm.UiDeviceUtils.pressEnterButton;
import static android.server.wm.UiDeviceUtils.pressHomeButton;
import static android.server.wm.UiDeviceUtils.pressSleepButton;
import static android.server.wm.UiDeviceUtils.pressUnlockButton;
import static android.server.wm.UiDeviceUtils.pressWakeupButton;
import static android.server.wm.UiDeviceUtils.waitForDeviceIdle;
import static android.server.wm.WindowManagerState.STATE_RESUMED;
import static android.server.wm.app.Components.BROADCAST_RECEIVER_ACTIVITY;
import static android.server.wm.app.Components.BroadcastReceiverActivity.ACTION_TRIGGER_BROADCAST;
import static android.server.wm.app.Components.BroadcastReceiverActivity.EXTRA_BROADCAST_ORIENTATION;
import static android.server.wm.app.Components.BroadcastReceiverActivity.EXTRA_CUTOUT_EXISTS;
import static android.server.wm.app.Components.BroadcastReceiverActivity.EXTRA_DISMISS_KEYGUARD;
import static android.server.wm.app.Components.BroadcastReceiverActivity.EXTRA_DISMISS_KEYGUARD_METHOD;
import static android.server.wm.app.Components.BroadcastReceiverActivity.EXTRA_FINISH_BROADCAST;
import static android.server.wm.app.Components.BroadcastReceiverActivity.EXTRA_MOVE_BROADCAST_TO_BACK;
import static android.server.wm.app.Components.LAUNCHING_ACTIVITY;
import static android.server.wm.app.Components.LaunchingActivity.KEY_FINISH_BEFORE_LAUNCH;
import static android.server.wm.app.Components.PipActivity.ACTION_EXPAND_PIP;
import static android.server.wm.app.Components.PipActivity.ACTION_SET_REQUESTED_ORIENTATION;
import static android.server.wm.app.Components.PipActivity.ACTION_UPDATE_PIP_STATE;
import static android.server.wm.app.Components.PipActivity.EXTRA_PIP_ORIENTATION;
import static android.server.wm.app.Components.PipActivity.EXTRA_SET_ASPECT_RATIO_WITH_DELAY_DENOMINATOR;
import static android.server.wm.app.Components.PipActivity.EXTRA_SET_ASPECT_RATIO_WITH_DELAY_NUMERATOR;
import static android.server.wm.app.Components.PipActivity.EXTRA_SET_PIP_CALLBACK;
import static android.server.wm.app.Components.PipActivity.EXTRA_SET_PIP_STASHED;
import static android.server.wm.app.Components.TEST_ACTIVITY;
import static android.server.wm.second.Components.SECOND_ACTIVITY;
import static android.server.wm.third.Components.THIRD_ACTIVITY;
import static android.view.Display.DEFAULT_DISPLAY;
import static android.view.Display.INVALID_DISPLAY;
import static android.view.Surface.ROTATION_0;
import static android.view.WindowManager.LayoutParams.TYPE_BASE_APPLICATION;

import static androidx.test.platform.app.InstrumentationRegistry.getInstrumentation;

import static org.junit.Assert.assertEquals;
import static org.junit.Assert.assertNotNull;
import static org.junit.Assert.assertTrue;
import static org.junit.Assert.fail;
import static org.junit.Assume.assumeTrue;

import static java.lang.Integer.toHexString;

import android.accessibilityservice.AccessibilityService;
import android.app.Activity;
import android.app.ActivityManager;
import android.app.ActivityOptions;
import android.app.ActivityTaskManager;
import android.app.Instrumentation;
import android.app.KeyguardManager;
import android.app.WindowConfiguration;
import android.content.ComponentName;
import android.content.ContentResolver;
import android.content.Context;
import android.content.Intent;
import android.content.pm.PackageManager;
import android.content.pm.ResolveInfo;
import android.content.res.Resources;
import android.database.ContentObserver;
import android.graphics.Bitmap;
import android.graphics.Rect;
import android.hardware.display.AmbientDisplayConfiguration;
import android.hardware.display.DisplayManager;
import android.os.Bundle;
import android.os.Handler;
import android.os.HandlerThread;
import android.os.PowerManager;
import android.os.RemoteCallback;
import android.os.SystemClock;
import android.os.SystemProperties;
import android.provider.Settings;
import android.server.wm.CommandSession.ActivityCallback;
import android.server.wm.CommandSession.ActivitySession;
import android.server.wm.CommandSession.ActivitySessionClient;
import android.server.wm.CommandSession.ConfigInfo;
import android.server.wm.CommandSession.LaunchInjector;
import android.server.wm.CommandSession.LaunchProxy;
import android.server.wm.CommandSession.SizeInfo;
import android.server.wm.TestJournalProvider.TestJournalContainer;
import android.server.wm.WindowManagerState.WindowState;
import android.server.wm.settings.SettingsSession;
import android.util.DisplayMetrics;
import android.util.EventLog;
import android.util.EventLog.Event;
import android.view.Display;
import android.view.View;
import android.view.WindowManager;

import androidx.annotation.NonNull;
import androidx.annotation.Nullable;
import androidx.test.ext.junit.rules.ActivityScenarioRule;

import com.android.compatibility.common.util.AppOpsUtils;
import com.android.compatibility.common.util.SystemUtil;

import org.junit.Before;
import org.junit.Rule;
import org.junit.rules.ErrorCollector;
import org.junit.rules.RuleChain;
import org.junit.rules.TestRule;
import org.junit.runner.Description;
import org.junit.runners.model.Statement;

import java.io.IOException;
import java.util.ArrayList;
import java.util.Arrays;
import java.util.Collections;
import java.util.HashMap;
import java.util.Iterator;
import java.util.List;
import java.util.Map;
import java.util.Objects;
import java.util.UUID;
import java.util.concurrent.atomic.AtomicBoolean;
import java.util.function.BooleanSupplier;
import java.util.function.Consumer;
import java.util.regex.Matcher;
import java.util.regex.Pattern;

public abstract class ActivityManagerTestBase {
    private static final boolean PRETEND_DEVICE_SUPPORTS_PIP = false;
    private static final boolean PRETEND_DEVICE_SUPPORTS_FREEFORM = false;
    private static final String LOG_SEPARATOR = "LOG_SEPARATOR";
    // Use one of the test tags as a separator
    private static final int EVENT_LOG_SEPARATOR_TAG = 42;

    protected static final int[] ALL_ACTIVITY_TYPE_BUT_HOME = {
            ACTIVITY_TYPE_STANDARD, ACTIVITY_TYPE_ASSISTANT, ACTIVITY_TYPE_RECENTS,
            ACTIVITY_TYPE_UNDEFINED
    };

    private static final String TEST_PACKAGE = TEST_ACTIVITY.getPackageName();
    private static final String SECOND_TEST_PACKAGE = SECOND_ACTIVITY.getPackageName();
    private static final String THIRD_TEST_PACKAGE = THIRD_ACTIVITY.getPackageName();
    private static final List<String> TEST_PACKAGES;

    static {
        final List<String> testPackages = new ArrayList<>();
        testPackages.add(TEST_PACKAGE);
        testPackages.add(SECOND_TEST_PACKAGE);
        testPackages.add(THIRD_TEST_PACKAGE);
        testPackages.add("android.server.wm.cts");
        testPackages.add("android.server.wm.jetpack");
        TEST_PACKAGES = Collections.unmodifiableList(testPackages);
    }

    protected static final String AM_START_HOME_ACTIVITY_COMMAND =
            "am start -a android.intent.action.MAIN -c android.intent.category.HOME";

    protected static final String MSG_NO_MOCK_IME =
            "MockIme cannot be used for devices that do not support installable IMEs";

    private static final String AM_BROADCAST_CLOSE_SYSTEM_DIALOGS =
            "am broadcast -a android.intent.action.CLOSE_SYSTEM_DIALOGS";

    protected static final String LOCK_CREDENTIAL = "1234";

    private static final int UI_MODE_TYPE_MASK = 0x0f;
    private static final int UI_MODE_TYPE_VR_HEADSET = 0x07;

    private static Boolean sHasHomeScreen = null;
    private static Boolean sSupportsSystemDecorsOnSecondaryDisplays = null;
    private static Boolean sSupportsInsecureLockScreen = null;
    private static Boolean sIsAssistantOnTop = null;
    private static boolean sIllegalTaskStateFound;

    protected static final int INVALID_DEVICE_ROTATION = -1;

    protected final Instrumentation mInstrumentation = getInstrumentation();
    protected final Context mContext = getInstrumentation().getContext();
    protected final ActivityManager mAm = mContext.getSystemService(ActivityManager.class);
    protected final ActivityTaskManager mAtm = mContext.getSystemService(ActivityTaskManager.class);
    protected final DisplayManager mDm = mContext.getSystemService(DisplayManager.class);
    protected final WindowManager mWm = mContext.getSystemService(WindowManager.class);
    protected final KeyguardManager mKm = mContext.getSystemService(KeyguardManager.class);

    /** The tracker to manage objects (especially {@link AutoCloseable}) in a test method. */
    protected final ObjectTracker mObjectTracker = new ObjectTracker();

    /** The last rule to handle all errors. */
    private final ErrorCollector mPostAssertionRule = new PostAssertionRule();

    /** The necessary procedures of set up and tear down. */
    @Rule
    public final TestRule mBaseRule = RuleChain.outerRule(mPostAssertionRule)
            .around(new WrapperRule(null /* before */, this::tearDownBase));

    /**
     * @return the am command to start the given activity with the following extra key/value pairs.
     * {@param extras} a list of {@link CliIntentExtra} representing a generic intent extra
     */
    // TODO: Make this more generic, for instance accepting flags or extras of other types.
    protected static String getAmStartCmd(final ComponentName activityName,
            final CliIntentExtra... extras) {
        return getAmStartCmdInternal(getActivityName(activityName), extras);
    }

    private static String getAmStartCmdInternal(final String activityName,
            final CliIntentExtra... extras) {
        return appendKeyValuePairs(
                new StringBuilder("am start -n ").append(activityName),
                extras);
    }

    private static String appendKeyValuePairs(
            final StringBuilder cmd, final CliIntentExtra... extras) {
        for (int i = 0; i < extras.length; i++) {
            extras[i].appendTo(cmd);
        }
        return cmd.toString();
    }

    protected static String getAmStartCmd(final ComponentName activityName, final int displayId,
            final CliIntentExtra... extras) {
        return getAmStartCmdInternal(getActivityName(activityName), displayId, extras);
    }

    private static String getAmStartCmdInternal(final String activityName, final int displayId,
            final CliIntentExtra... extras) {
        return appendKeyValuePairs(
                new StringBuilder("am start -n ")
                        .append(activityName)
                        .append(" -f 0x")
                        .append(toHexString(FLAG_ACTIVITY_NEW_TASK | FLAG_ACTIVITY_MULTIPLE_TASK))
                        .append(" --display ")
                        .append(displayId),
                extras);
    }

    protected static String getAmStartCmdInNewTask(final ComponentName activityName) {
        return "am start -n " + getActivityName(activityName) + " -f 0x18000000";
    }

    protected static String getAmStartCmdWithData(final ComponentName activityName, String data) {
        return "am start -n " + getActivityName(activityName) + " -d " + data;
    }

    protected static String getAmStartCmdOverHome(final ComponentName activityName) {
        return "am start --activity-task-on-home -n " + getActivityName(activityName);
    }

    protected WindowManagerStateHelper mWmState = new WindowManagerStateHelper();
    protected TouchHelper mTouchHelper = new TouchHelper(mInstrumentation, mWmState);
    // Initialized in setUp to execute with proper permission, such as MANAGE_ACTIVITY_TASKS
    public TestTaskOrganizer mTaskOrganizer;

    public WindowManagerStateHelper getWmState() {
        return mWmState;
    }

    protected BroadcastActionTrigger mBroadcastActionTrigger = new BroadcastActionTrigger();

    /** Runs a runnable with shell permissions. These can be nested. */
    protected void runWithShellPermission(Runnable runnable) {
        NestedShellPermission.run(runnable);
    }
    /**
     * Returns true if the activity is shown before timeout.
     */
    protected boolean waitForActivityFocused(int timeoutMs, ComponentName componentName) {
        long endTime = System.currentTimeMillis() + timeoutMs;
        while (endTime > System.currentTimeMillis()) {
            mWmState.computeState();
            if (mWmState.hasActivityState(componentName, STATE_RESUMED)) {
                SystemClock.sleep(200);
                mWmState.computeState();
                break;
            }
            SystemClock.sleep(200);
            mWmState.computeState();
        }
        return getActivityName(componentName).equals(mWmState.getFocusedActivity());
    }

    /**
     * Helper class to process test actions by broadcast.
     */
    protected class BroadcastActionTrigger {

        private Intent createIntentWithAction(String broadcastAction) {
            return new Intent(broadcastAction)
                    .setFlags(Intent.FLAG_RECEIVER_FOREGROUND);
        }

        void doAction(String broadcastAction) {
            mContext.sendBroadcast(createIntentWithAction(broadcastAction));
        }

        void finishBroadcastReceiverActivity() {
            mContext.sendBroadcast(createIntentWithAction(ACTION_TRIGGER_BROADCAST)
                    .putExtra(EXTRA_FINISH_BROADCAST, true));
        }

        void launchActivityNewTask(String launchComponent) {
            mContext.sendBroadcast(createIntentWithAction(ACTION_TRIGGER_BROADCAST)
                    .putExtra(KEY_LAUNCH_ACTIVITY, true)
                    .putExtra(KEY_NEW_TASK, true)
                    .putExtra(KEY_TARGET_COMPONENT, launchComponent));
        }

        void moveTopTaskToBack() {
            mContext.sendBroadcast(createIntentWithAction(ACTION_TRIGGER_BROADCAST)
                    .putExtra(EXTRA_MOVE_BROADCAST_TO_BACK, true));
        }

        void requestOrientation(int orientation) {
            mContext.sendBroadcast(createIntentWithAction(ACTION_TRIGGER_BROADCAST)
                    .putExtra(EXTRA_BROADCAST_ORIENTATION, orientation));
        }

        void dismissKeyguardByFlag() {
            mContext.sendBroadcast(createIntentWithAction(ACTION_TRIGGER_BROADCAST)
                    .putExtra(EXTRA_DISMISS_KEYGUARD, true));
        }

        void dismissKeyguardByMethod() {
            mContext.sendBroadcast(createIntentWithAction(ACTION_TRIGGER_BROADCAST)
                    .putExtra(EXTRA_DISMISS_KEYGUARD_METHOD, true));
        }

        void expandPip() {
            mContext.sendBroadcast(createIntentWithAction(ACTION_EXPAND_PIP));
        }

        void expandPipWithAspectRatio(String extraNum, String extraDenom) {
            mContext.sendBroadcast(createIntentWithAction(ACTION_EXPAND_PIP)
                    .putExtra(EXTRA_SET_ASPECT_RATIO_WITH_DELAY_NUMERATOR, extraNum)
                    .putExtra(EXTRA_SET_ASPECT_RATIO_WITH_DELAY_DENOMINATOR, extraDenom));
        }

        void sendPipStateUpdate(RemoteCallback callback, boolean stashed) {
            mContext.sendBroadcast(createIntentWithAction(ACTION_UPDATE_PIP_STATE)
                    .putExtra(EXTRA_SET_PIP_CALLBACK, callback)
                    .putExtra(EXTRA_SET_PIP_STASHED, stashed));
        }

        void requestOrientationForPip(int orientation) {
            mContext.sendBroadcast(createIntentWithAction(ACTION_SET_REQUESTED_ORIENTATION)
                    .putExtra(EXTRA_PIP_ORIENTATION, String.valueOf(orientation)));
        }
    }

    /**
     * Helper class to launch / close test activity by instrumentation way.
     */
    protected class TestActivitySession<T extends Activity> implements AutoCloseable {
        private T mTestActivity;
        boolean mFinishAfterClose;
        private static final int ACTIVITY_LAUNCH_TIMEOUT = 10000;
        private static final int WAIT_SLICE = 50;

        /**
         * Launches an {@link Activity} on a target display synchronously.
         * @param activityClass The {@link Activity} class to be launched
         * @param displayId ID of the target display
         */
        void launchTestActivityOnDisplaySync(Class<T> activityClass, int displayId) {
            launchTestActivityOnDisplaySync(activityClass, displayId, WINDOWING_MODE_UNDEFINED);
        }

        /**
         * Launches an {@link Activity} on a target display synchronously.
         *
         * @param activityClass The {@link Activity} class to be launched
         * @param displayId ID of the target display
         * @param windowingMode Windowing mode at launch
         */
        void launchTestActivityOnDisplaySync(
                Class<T> activityClass, int displayId, int windowingMode) {
            final Intent intent = new Intent(mContext, activityClass)
                    .addFlags(FLAG_ACTIVITY_NEW_TASK);
            final String className = intent.getComponent().getClassName();
            launchTestActivityOnDisplaySync(className, intent, displayId, windowingMode);
        }

        /**
         * Launches an {@link Activity} synchronously on a target display. The class name needs to
         * be provided either implicitly through the {@link Intent} or explicitly as a parameter
         *
         * @param className Optional class name of expected activity
         * @param intent Intent to launch an activity
         * @param displayId ID for the target display
         */
        void launchTestActivityOnDisplaySync(@Nullable String className, Intent intent,
                int displayId) {
            launchTestActivityOnDisplaySync(className, intent, displayId, WINDOWING_MODE_UNDEFINED);
        }

        /**
         * Launches an {@link Activity} synchronously on a target display. The class name needs to
         * be provided either implicitly through the {@link Intent} or explicitly as a parameter
         *
         * @param className Optional class name of expected activity
         * @param intent Intent to launch an activity
         * @param displayId ID for the target display
         * @param windowingMode Windowing mode at launch
         */
        void launchTestActivityOnDisplaySync(
                @Nullable String className, Intent intent, int displayId, int windowingMode) {
            runWithShellPermission(
                    () -> {
                        mTestActivity =
                                launchActivityOnDisplay(
                                        className, intent, displayId, windowingMode);
                        // Check activity is launched and resumed.
                        final ComponentName testActivityName = mTestActivity.getComponentName();
                        waitAndAssertTopResumedActivity(
                                testActivityName, displayId, "Activity must be resumed");
                    });
        }

        /**
         * Launches an {@link Activity} on a target display asynchronously.
         * @param activityClass The {@link Activity} class to be launched
         * @param displayId ID of the target display
         */
        void launchTestActivityOnDisplay(Class<T> activityClass, int displayId) {
            final Intent intent = new Intent(mContext, activityClass)
                    .addFlags(FLAG_ACTIVITY_NEW_TASK);
            final String className = intent.getComponent().getClassName();
            runWithShellPermission(
                    () -> {
                        mTestActivity =
                                launchActivityOnDisplay(
                                        className, intent, displayId, WINDOWING_MODE_UNDEFINED);
                        assertNotNull(mTestActivity);
                    });
        }

        /**
         * Launches an {@link Activity} on a target display. In order to return the correct activity
         * the class name or an explicit {@link Intent} must be provided.
         *
         * @param className Optional class name of expected activity
         * @param intent {@link Intent} to launch an activity
         * @param displayId ID for the target display
         * @param windowingMode Windowing mode at launch
         * @return The {@link Activity} that was launched
         */
        private T launchActivityOnDisplay(
                @Nullable String className, Intent intent, int displayId, int windowingMode) {
            final String localClassName = className != null ? className :
              (intent.getComponent() != null ? intent.getComponent().getClassName() : null);
            if (localClassName == null || localClassName.isEmpty()) {
                fail("Must provide either a class name or an intent with a component");
            }
            final ActivityOptions launchOptions = ActivityOptions.makeBasic();
            launchOptions.setLaunchDisplayId(displayId);
            launchOptions.setLaunchWindowingMode(windowingMode);
            final Bundle bundle = launchOptions.toBundle();
            final ActivityMonitor monitor = mInstrumentation.addMonitor(localClassName, null,
                    false);
            mContext.startActivity(intent.addFlags(FLAG_ACTIVITY_NEW_TASK), bundle);
            // Wait for activity launch with timeout.
            mTestActivity = (T) mInstrumentation.waitForMonitorWithTimeout(monitor,
                    ACTIVITY_LAUNCH_TIMEOUT);
            assertNotNull(mTestActivity);
            return mTestActivity;
        }

        void finishCurrentActivityNoWait() {
            if (mTestActivity != null) {
                mTestActivity.finishAndRemoveTask();
                mTestActivity = null;
            }
        }

        void runOnMainSyncAndWait(Runnable runnable) {
            mInstrumentation.runOnMainSync(runnable);
            mInstrumentation.waitForIdleSync();
        }

        void runOnMainAndAssertWithTimeout(@NonNull BooleanSupplier condition, long timeoutMs,
                String message) {
            final AtomicBoolean result = new AtomicBoolean();
            final long expiredTime = System.currentTimeMillis() + timeoutMs;
            while (!result.get()) {
                if (System.currentTimeMillis() >= expiredTime) {
                    fail(message);
                }
                runOnMainSyncAndWait(() -> {
                    if (condition.getAsBoolean()) {
                        result.set(true);
                    }
                });
                SystemClock.sleep(WAIT_SLICE);
            }
        }

        T getActivity() {
            return mTestActivity;
        }

        @Override
        public void close() {
            if (mTestActivity != null && mFinishAfterClose) {
                mTestActivity.finishAndRemoveTask();
            }
        }
    }

    @Before
    public void setUp() throws Exception {
        if (isKeyguardLocked() || !Objects.requireNonNull(
                mContext.getSystemService(PowerManager.class)).isInteractive()) {
            pressWakeupButton();
            pressUnlockButton();
        }
        launchHomeActivityNoWait();
        removeRootTasksWithActivityTypes(ALL_ACTIVITY_TYPE_BUT_HOME);

        runWithShellPermission(() -> {
            // TaskOrganizer ctor requires MANAGE_ACTIVITY_TASKS permission
            mTaskOrganizer = new TestTaskOrganizer(mContext);
            // Clear launch params for all test packages to make sure each test is run in a clean
            // state.
            mAtm.clearLaunchParamsForPackages(TEST_PACKAGES);
        });
    }

    /** It always executes after {@link org.junit.After}. */
    private void tearDownBase() {
        mObjectTracker.tearDown(mPostAssertionRule::addError);

        if (mTaskOrganizer != null) {
            mTaskOrganizer.unregisterOrganizerIfNeeded();
        }
        // Synchronous execution of removeRootTasksWithActivityTypes() ensures that all
        // activities but home are cleaned up from the root task at the end of each test. Am force
        // stop shell commands might be asynchronous and could interrupt the task cleanup
        // process if executed first.
        removeRootTasksWithActivityTypes(ALL_ACTIVITY_TYPE_BUT_HOME);
        stopTestPackage(TEST_PACKAGE);
        stopTestPackage(SECOND_TEST_PACKAGE);
        stopTestPackage(THIRD_TEST_PACKAGE);
        launchHomeActivityNoWait();
    }

    /**
     * After home key is pressed ({@link #pressHomeButton} is called), the later launch may be
     * deferred if the calling uid doesn't have android.permission.STOP_APP_SWITCHES. This method
     * will resume the temporary stopped state, so the launch won't be affected.
     */
    protected void resumeAppSwitches() {
        SystemUtil.runWithShellPermissionIdentity(ActivityManager::resumeAppSwitches);
    }

    protected void startActivityOnDisplay(int displayId, ComponentName component) {
        final ActivityOptions options = ActivityOptions.makeBasic();
        options.setLaunchDisplayId(displayId);

        mContext.startActivity(new Intent().addFlags(Intent.FLAG_ACTIVITY_NEW_TASK)
                .setComponent(component), options.toBundle());
    }

    protected boolean noHomeScreen() {
        try {
            return mContext.getResources().getBoolean(
                    Resources.getSystem().getIdentifier("config_noHomeScreen", "bool",
                            "android"));
        } catch (Resources.NotFoundException e) {
            // Assume there's a home screen.
            return false;
        }
    }

    private boolean getSupportsSystemDecorsOnSecondaryDisplays() {
        try {
            return mContext.getResources().getBoolean(
                    Resources.getSystem().getIdentifier(
                            "config_supportsSystemDecorsOnSecondaryDisplays", "bool", "android"));
        } catch (Resources.NotFoundException e) {
            // Assume this device support system decorations.
            return true;
        }
    }

    protected ComponentName getDefaultSecondaryHomeComponent() {
        assumeTrue(supportsMultiDisplay());
        int resId = Resources.getSystem().getIdentifier(
                "config_secondaryHomePackage", "string", "android");
        final Intent intent = new Intent(Intent.ACTION_MAIN);
        intent.addCategory(Intent.CATEGORY_SECONDARY_HOME);
        intent.setPackage(mContext.getResources().getString(resId));
        final ResolveInfo resolveInfo =
                mContext.getPackageManager().resolveActivity(intent, MATCH_DEFAULT_ONLY);
        assertNotNull("Should have default secondary home activity", resolveInfo);

        return new ComponentName(resolveInfo.activityInfo.packageName,
                resolveInfo.activityInfo.name);
    }

    /**
     * Insert an input event (ACTION_DOWN -> ACTION_CANCEL) to ensures the display to be focused
     * without triggering potential clicked to impact the test environment.
     * (e.g: Keyguard credential activated unexpectedly.)
     *
     * @param displayId the display ID to gain focused by inject swipe action
     */
    protected void touchAndCancelOnDisplayCenterSync(int displayId) {
        mTouchHelper.touchAndCancelOnDisplayCenterSync(displayId);
    }

    protected void tapOnDisplaySync(int x, int y, int displayId) {
        mTouchHelper.tapOnDisplaySync(x, y, displayId);
    }

    private void tapOnDisplay(int x, int y, int displayId, boolean sync) {
        mTouchHelper.tapOnDisplay(x, y, displayId, sync);
    }

    protected void tapOnCenter(Rect bounds, int displayId) {
        mTouchHelper.tapOnCenter(bounds, displayId);
    }

    protected void tapOnViewCenter(View view) {
        mTouchHelper.tapOnViewCenter(view);
    }

    protected void tapOnStackCenter(WindowManagerState.ActivityTask stack) {
        mTouchHelper.tapOnStackCenter(stack);
    }

    protected void tapOnDisplayCenter(int displayId) {
        mTouchHelper.tapOnDisplayCenter(displayId);
    }

    protected void tapOnDisplayCenterAsync(int displayId) {
        mTouchHelper.tapOnDisplayCenterAsync(displayId);
    }

    public static void injectKey(int keyCode, boolean longPress, boolean sync) {
        TouchHelper.injectKey(keyCode, longPress, sync);
    }

    protected void removeRootTasksWithActivityTypes(int... activityTypes) {
        runWithShellPermission(() -> mAtm.removeRootTasksWithActivityTypes(activityTypes));
        waitForIdle();
    }

    protected void removeRootTasksInWindowingModes(int... windowingModes) {
        runWithShellPermission(() -> mAtm.removeRootTasksInWindowingModes(windowingModes));
        waitForIdle();
    }

    protected void removeRootTask(int taskId) {
        runWithShellPermission(() -> mAtm.removeTask(taskId));
        waitForIdle();
    }

    public static String executeShellCommand(String command) {
        log("Shell command: " + command);
        try {
            return SystemUtil.runShellCommand(getInstrumentation(), command);
        } catch (IOException e) {
            //bubble it up
            logE("Error running shell command: " + command);
            throw new RuntimeException(e);
        }
    }

    protected Bitmap takeScreenshot() {
        return mInstrumentation.getUiAutomation().takeScreenshot();
    }

    protected void launchActivity(final ComponentName activityName,
            final CliIntentExtra... extras) {
        launchActivityNoWait(activityName, extras);
        mWmState.waitForValidState(activityName);
    }

    protected void launchActivityNoWait(final ComponentName activityName,
            final CliIntentExtra... extras) {
        executeShellCommand(getAmStartCmd(activityName, extras));
    }

    protected void launchActivityInNewTask(final ComponentName activityName) {
        executeShellCommand(getAmStartCmdInNewTask(activityName));
        mWmState.waitForValidState(activityName);
    }

    protected void launchActivityWithData(final ComponentName activityName, String data) {
        executeShellCommand(getAmStartCmdWithData(activityName, data));
        mWmState.waitForValidState(activityName);
    }

    protected static void waitForIdle() {
        getInstrumentation().waitForIdleSync();
    }

    static void waitForOrFail(String message, BooleanSupplier condition) {
        Condition.waitFor(new Condition<>(message, condition)
                .setRetryIntervalMs(500)
                .setRetryLimit(20)
                .setOnFailure(unusedResult -> fail("FAILED because unsatisfied: " + message)));
    }

    /** Returns the stack that contains the provided task. */
    protected WindowManagerState.ActivityTask getStackForTaskId(int taskId) {
        mWmState.computeState();
        final List<WindowManagerState.ActivityTask> stacks = mWmState.getRootTasks();
        for (WindowManagerState.ActivityTask stack : stacks) {
            if (stack.getTask(taskId) != null) {
                return stack;
            }
        }
        return null;
    }

    protected WindowManagerState.ActivityTask getRootTask(int taskId) {
        mWmState.computeState();
        final List<WindowManagerState.ActivityTask> rootTasks = mWmState.getRootTasks();
        for (WindowManagerState.ActivityTask rootTask : rootTasks) {
            if (rootTask.getTaskId() == taskId) {
                return rootTask;
            }
        }
        return null;
    }

    protected int getDisplayWindowingModeByActivity(ComponentName activity) {
        return mWmState.getDisplay(mWmState.getDisplayByActivity(activity)).getWindowingMode();
    }

    /**
     * Launches the home activity directly. If there is no specific reason to simulate a home key
     * (which will trigger stop-app-switches), it is the recommended method to go home.
     */
    protected static void launchHomeActivityNoWait() {
        // dismiss all system dialogs before launch home.
        executeShellCommand(AM_BROADCAST_CLOSE_SYSTEM_DIALOGS);
        executeShellCommand(AM_START_HOME_ACTIVITY_COMMAND);
    }

    /** Launches the home activity directly with waiting for it to be visible. */
    protected void launchHomeActivity() {
        launchHomeActivityNoWait();
        mWmState.waitForHomeActivityVisible();
    }

    protected void launchActivityNoWait(ComponentName activityName, int windowingMode,
            final CliIntentExtra... extras) {
        executeShellCommand(getAmStartCmd(activityName, extras)
                + " --windowingMode " + windowingMode);
    }

    protected void launchActivity(ComponentName activityName, int windowingMode,
            final CliIntentExtra... keyValuePairs) {
        launchActivityNoWait(activityName, windowingMode, keyValuePairs);
        mWmState.waitForValidState(new WaitForValidActivityState.Builder(activityName)
                .setWindowingMode(windowingMode)
                .build());
    }

    protected void launchActivityOnDisplay(ComponentName activityName, int windowingMode,
            int displayId, final CliIntentExtra... extras) {
        executeShellCommand(getAmStartCmd(activityName, displayId, extras)
                + " --windowingMode " + windowingMode);
        mWmState.waitForValidState(new WaitForValidActivityState.Builder(activityName)
                .setWindowingMode(windowingMode)
                .build());
    }

    protected void launchActivityOnDisplay(ComponentName activityName, int displayId,
            CliIntentExtra... extras) {
        launchActivityOnDisplayNoWait(activityName, displayId, extras);
        mWmState.waitForValidState(activityName);
    }

    protected void launchActivityOnDisplayNoWait(ComponentName activityName, int displayId,
            CliIntentExtra... extras) {
        executeShellCommand(getAmStartCmd(activityName, displayId, extras));
    }

    protected void launchActivityInPrimarySplit(ComponentName activityName) {
        runWithShellPermission(() -> {
            launchActivity(activityName);
            final int taskId = mWmState.getTaskByActivity(activityName).mTaskId;
            mTaskOrganizer.putTaskInSplitPrimary(taskId);
            mWmState.waitForValidState(activityName);
        });
    }

    protected void launchActivityInSecondarySplit(ComponentName activityName) {
        runWithShellPermission(() -> {
            launchActivity(activityName);
            final int taskId = mWmState.getTaskByActivity(activityName).mTaskId;
            mTaskOrganizer.putTaskInSplitSecondary(taskId);
            mWmState.waitForValidState(activityName);
        });
    }

    protected void putActivityInPrimarySplit(ComponentName activityName) {
        final int taskId = mWmState.getTaskByActivity(activityName).mTaskId;
        mTaskOrganizer.putTaskInSplitPrimary(taskId);
        mWmState.waitForValidState(activityName);
    }

    protected void putActivityInSecondarySplit(ComponentName activityName) {
        final int taskId = mWmState.getTaskByActivity(activityName).mTaskId;
        mTaskOrganizer.putTaskInSplitSecondary(taskId);
        mWmState.waitForValidState(activityName);
    }

    /**
     * Launches {@param primaryActivity} into split-screen primary windowing mode
     * and {@param secondaryActivity} to the side in split-screen secondary windowing mode.
     */
    protected void launchActivitiesInSplitScreen(LaunchActivityBuilder primaryActivity,
            LaunchActivityBuilder secondaryActivity) {
        // Launch split-screen primary.
        primaryActivity
                .setUseInstrumentation()
                .setWaitForLaunched(true)
                .execute();

        final int primaryTaskId = mWmState.getTaskByActivity(
                primaryActivity.mTargetActivity).mTaskId;
        mTaskOrganizer.putTaskInSplitPrimary(primaryTaskId);

        // Launch split-screen secondary
        secondaryActivity
                .setUseInstrumentation()
                .setWaitForLaunched(true)
                .setNewTask(true)
                .setMultipleTask(true)
                .execute();

        final int secondaryTaskId = mWmState.getTaskByActivity(
                secondaryActivity.mTargetActivity).mTaskId;
        mTaskOrganizer.putTaskInSplitSecondary(secondaryTaskId);
        mWmState.computeState(primaryActivity.getTargetActivity(),
                secondaryActivity.getTargetActivity());
        log("launchActivitiesInSplitScreen(), primaryTaskId=" + primaryTaskId +
                ", secondaryTaskId=" + secondaryTaskId);
    }

    /**
     * Move the task of {@param primaryActivity} into split-screen primary and the task of
     * {@param secondaryActivity} to the side in split-screen secondary.
     */
    protected void moveActivitiesToSplitScreen(ComponentName primaryActivity,
            ComponentName secondaryActivity) {
        final int primaryTaskId = mWmState.getTaskByActivity(primaryActivity).mTaskId;
        mTaskOrganizer.putTaskInSplitPrimary(primaryTaskId);

        final int secondaryTaskId = mWmState.getTaskByActivity(secondaryActivity).mTaskId;
        mTaskOrganizer.putTaskInSplitSecondary(secondaryTaskId);

        mWmState.computeState(primaryActivity, secondaryActivity);
        log("moveActivitiesToSplitScreen(), primaryTaskId=" + primaryTaskId +
                ", secondaryTaskId=" + secondaryTaskId);
    }

    protected void dismissSplitScreen(boolean primaryOnTop) {
        if (mTaskOrganizer != null) {
            mTaskOrganizer.dismissSplitScreen(primaryOnTop);
        }
    }

    /**
     * Move activity to root task or on top of the given root task when the root task is also a leaf
     * task.
     */
    protected void moveActivityToRootTaskOrOnTop(ComponentName activityName, int rootTaskId) {
        mWmState.computeState(activityName);
        WindowManagerState.ActivityTask rootTask = getRootTask(rootTaskId);
        if (rootTask.getActivities().size() != 0) {
            // If the root task is a 1-level task, start the activity on top of given task.
            getLaunchActivityBuilder()
                    .setDisplayId(rootTask.mDisplayId)
                    .setWindowingMode(rootTask.getWindowingMode())
                    .setActivityType(rootTask.getActivityType())
                    .setTargetActivity(activityName)
                    .allowMultipleInstances(false)
                    .setUseInstrumentation()
                    .execute();
        } else {
            final int taskId = mWmState.getTaskByActivity(activityName).mTaskId;
            runWithShellPermission(() -> mAtm.moveTaskToRootTask(taskId, rootTaskId, true));
        }
        mWmState.waitForValidState(new WaitForValidActivityState.Builder(activityName)
                .setStackId(rootTaskId)
                .build());
    }

    protected void resizeActivityTask(
            ComponentName activityName, int left, int top, int right, int bottom) {
        mWmState.computeState(activityName);
        final int taskId = mWmState.getTaskByActivity(activityName).mTaskId;
        runWithShellPermission(() -> mAtm.resizeTask(taskId, new Rect(left, top, right, bottom)));
    }

    protected boolean supportsVrMode() {
        return hasDeviceFeature(FEATURE_VR_MODE_HIGH_PERFORMANCE);
    }

    protected boolean supportsPip() {
        return hasDeviceFeature(FEATURE_PICTURE_IN_PICTURE)
                || PRETEND_DEVICE_SUPPORTS_PIP;
    }

    protected boolean supportsFreeform() {
        return hasDeviceFeature(FEATURE_FREEFORM_WINDOW_MANAGEMENT)
                || PRETEND_DEVICE_SUPPORTS_FREEFORM;
    }

    /** Whether or not the device supports lock screen. */
    protected boolean supportsLockScreen() {
        return supportsInsecureLock() || supportsSecureLock();
    }

    /** Whether or not the device supports pin/pattern/password lock. */
    protected boolean supportsSecureLock() {
        return hasDeviceFeature(FEATURE_SECURE_LOCK_SCREEN);
    }

    /** Whether or not the device supports "swipe" lock. */
    protected boolean supportsInsecureLock() {
        return !hasDeviceFeature(FEATURE_LEANBACK)
                && !hasDeviceFeature(FEATURE_WATCH)
                && !hasDeviceFeature(FEATURE_EMBEDDED)
                && !hasDeviceFeature(FEATURE_AUTOMOTIVE)
                && getSupportsInsecureLockScreen();
    }

    protected boolean supportsBlur() {
        return SystemProperties.get("ro.surface_flinger.supports_background_blur", "default")
                .equals("1");
    }

    protected boolean isWatch() {
        return hasDeviceFeature(FEATURE_WATCH);
    }

    protected boolean isCar() {
        return hasDeviceFeature(FEATURE_AUTOMOTIVE);
    }

    protected boolean isLeanBack() {
        return hasDeviceFeature(FEATURE_TELEVISION);
    }

    protected boolean isTablet() {
        // Larger than approx 7" tablets
        return mContext.getResources().getConfiguration().smallestScreenWidthDp >= 600;
    }

    protected boolean isOperatorTierDevice() {
        return hasDeviceFeature("com.google.android.tv.operator_tier");
    }

    protected void waitAndAssertActivityState(ComponentName activityName,
            String state, String message) {
        mWmState.waitForActivityState(activityName, state);

        assertTrue(message, mWmState.hasActivityState(activityName, state));
    }

    protected boolean isKeyguardLocked() {
        return mKm != null && mKm.isKeyguardLocked();
    }

    protected void waitAndAssertActivityStateOnDisplay(ComponentName activityName, String state,
            int displayId, String message) {
        waitAndAssertActivityState(activityName, state, message);
        assertEquals(message, mWmState.getDisplayByActivity(activityName),
                displayId);
    }

    public void waitAndAssertTopResumedActivity(ComponentName activityName, int displayId,
            String message) {
        final String activityClassName = getActivityName(activityName);
        mWmState.waitForWithAmState(state -> activityClassName.equals(state.getFocusedActivity()),
                "activity to be on top");
        waitAndAssertResumedActivity(activityName, "Activity must be resumed");
        mWmState.assertFocusedActivity(message, activityName);

        final int frontRootTaskId = mWmState.getFrontRootTaskId(displayId);
        WindowManagerState.ActivityTask frontRootTaskOnDisplay =
                mWmState.getRootTask(frontRootTaskId);
        assertEquals(
                "Resumed activity of front root task of the target display must match. " + message,
                activityClassName,
                frontRootTaskOnDisplay.isLeafTask() ? frontRootTaskOnDisplay.mResumedActivity
                        : frontRootTaskOnDisplay.getTopTask().mResumedActivity);
        mWmState.assertFocusedStack("Top activity's rootTask must also be on top", frontRootTaskId);
    }

    /**
     * Waits and asserts that the activity represented by the given activity name is resumed and
     * visible, but is not necessarily the top activity.
     *
     * @param activityName the activity name
     * @param message the error message
     */
    public void waitAndAssertResumedActivity(ComponentName activityName, String message) {
        mWmState.waitForValidState(activityName);
        mWmState.waitForActivityState(activityName, STATE_RESUMED);
        mWmState.assertValidity();
        assertTrue(message, mWmState.hasActivityState(activityName, STATE_RESUMED));
        mWmState.assertVisibility(activityName, true /* visible */);
    }

    // TODO: Switch to using a feature flag, when available.
    protected static boolean isUiModeLockedToVrHeadset() {
        final String output = runCommandAndPrintOutput("dumpsys uimode");

        Integer curUiMode = null;
        Boolean uiModeLocked = null;
        for (String line : output.split("\\n")) {
            line = line.trim();
            Matcher matcher = sCurrentUiModePattern.matcher(line);
            if (matcher.find()) {
                curUiMode = Integer.parseInt(matcher.group(1), 16);
            }
            matcher = sUiModeLockedPattern.matcher(line);
            if (matcher.find()) {
                uiModeLocked = matcher.group(1).equals("true");
            }
        }

        boolean uiModeLockedToVrHeadset = (curUiMode != null) && (uiModeLocked != null)
                && ((curUiMode & UI_MODE_TYPE_MASK) == UI_MODE_TYPE_VR_HEADSET) && uiModeLocked;

        if (uiModeLockedToVrHeadset) {
            log("UI mode is locked to VR headset");
        }

        return uiModeLockedToVrHeadset;
    }

    protected boolean supportsMultiWindow() {
        Display defaultDisplay = mDm.getDisplay(DEFAULT_DISPLAY);
        return ActivityTaskManager.supportsSplitScreenMultiWindow(
                mContext.createDisplayContext(defaultDisplay));
    }

    /** Returns true if the default display supports split screen multi-window. */
    protected boolean supportsSplitScreenMultiWindow() {
        Display defaultDisplay = mDm.getDisplay(DEFAULT_DISPLAY);
        return supportsSplitScreenMultiWindow(mContext.createDisplayContext(defaultDisplay));
    }

    /**
     * Returns true if the display associated with the supplied {@code context} supports split
     * screen multi-window.
     */
    protected boolean supportsSplitScreenMultiWindow(Context context) {
        return ActivityTaskManager.supportsSplitScreenMultiWindow(context);
    }

    protected boolean hasHomeScreen() {
        if (sHasHomeScreen == null) {
            sHasHomeScreen = !noHomeScreen();
        }
        return sHasHomeScreen;
    }

    protected boolean supportsSystemDecorsOnSecondaryDisplays() {
        if (sSupportsSystemDecorsOnSecondaryDisplays == null) {
            sSupportsSystemDecorsOnSecondaryDisplays = getSupportsSystemDecorsOnSecondaryDisplays();
        }
        return sSupportsSystemDecorsOnSecondaryDisplays;
    }

    protected boolean getSupportsInsecureLockScreen() {
        if (sSupportsInsecureLockScreen == null) {
            try {
                sSupportsInsecureLockScreen = mContext.getResources().getBoolean(
                        Resources.getSystem().getIdentifier(
                                "config_supportsInsecureLockScreen", "bool", "android"));
            } catch (Resources.NotFoundException e) {
                sSupportsInsecureLockScreen = true;
            }
        }
        return sSupportsInsecureLockScreen;
    }

    protected boolean isAssistantOnTopOfDream() {
        if (sIsAssistantOnTop == null) {
            sIsAssistantOnTop = mContext.getResources().getBoolean(
                    android.R.bool.config_assistantOnTopOfDream);
        }
        return sIsAssistantOnTop;
    }

    /**
     * Rotation support is indicated by explicitly having both landscape and portrait
     * features or not listing either at all.
     */
    protected boolean supportsRotation() {
        final boolean supportsLandscape = hasDeviceFeature(FEATURE_SCREEN_LANDSCAPE);
        final boolean supportsPortrait = hasDeviceFeature(FEATURE_SCREEN_PORTRAIT);
        return (supportsLandscape && supportsPortrait)
                || (!supportsLandscape && !supportsPortrait);
    }

    /**
     * The device should support orientation request from apps if it supports rotation and the
     * display is not close to square.
     */
    protected boolean supportsOrientationRequest() {
        return supportsRotation() && !isCloseToSquareDisplay();
    }

    /** Checks whether the display dimension is close to square. */
    protected boolean isCloseToSquareDisplay() {
        final Resources resources = mContext.getResources();
        final float closeToSquareMaxAspectRatio;
        try {
            closeToSquareMaxAspectRatio = resources.getFloat(resources.getIdentifier(
                    "config_closeToSquareDisplayMaxAspectRatio", "dimen", "android"));
        } catch (Resources.NotFoundException e) {
            // Assume device is not close to square.
            return false;
        }
        final DisplayMetrics displayMetrics = new DisplayMetrics();
        mDm.getDisplay(DEFAULT_DISPLAY).getRealMetrics(displayMetrics);
        final int w = displayMetrics.widthPixels;
        final int h = displayMetrics.heightPixels;
        final float aspectRatio = Math.max(w, h) / (float) Math.min(w, h);
        return aspectRatio <= closeToSquareMaxAspectRatio;
    }

    protected boolean hasDeviceFeature(final String requiredFeature) {
        return mContext.getPackageManager()
                .hasSystemFeature(requiredFeature);
    }

    protected static boolean isDisplayPortrait() {
        final DisplayManager displayManager = getInstrumentation()
                .getContext().getSystemService(DisplayManager.class);
        final Display display = displayManager.getDisplay(DEFAULT_DISPLAY);
        final DisplayMetrics displayMetrics = new DisplayMetrics();
        display.getRealMetrics(displayMetrics);
        return displayMetrics.widthPixels < displayMetrics.heightPixels;
    }

    protected static boolean isDisplayOn(int displayId) {
        final DisplayManager displayManager = getInstrumentation()
                .getContext().getSystemService(DisplayManager.class);
        final Display display = displayManager.getDisplay(displayId);
        return display != null && display.getState() == Display.STATE_ON;
    }

    protected static boolean perDisplayFocusEnabled() {
        return getInstrumentation().getTargetContext().getResources()
                .getBoolean(android.R.bool.config_perDisplayFocusEnabled);
    }

    protected static void removeLockCredential() {
        runCommandAndPrintOutput("locksettings clear --old " + LOCK_CREDENTIAL);
    }

    protected static boolean remoteInsetsControllerControlsSystemBars() {
        return getInstrumentation().getTargetContext().getResources()
                .getBoolean(android.R.bool.config_remoteInsetsControllerControlsSystemBars);
    }

    /** @see ObjectTracker#manage(AutoCloseable) */
    protected HomeActivitySession createManagedHomeActivitySession(ComponentName homeActivity) {
        return mObjectTracker.manage(new HomeActivitySession(homeActivity));
    }

    /** @see ObjectTracker#manage(AutoCloseable) */
    protected ActivitySessionClient createManagedActivityClientSession() {
        return mObjectTracker.manage(new ActivitySessionClient(mContext));
    }

    /** @see ObjectTracker#manage(AutoCloseable) */
    protected LockScreenSession createManagedLockScreenSession() {
        return mObjectTracker.manage(new LockScreenSession());
    }

    /** @see ObjectTracker#manage(AutoCloseable) */
    protected RotationSession createManagedRotationSession() {
        return mObjectTracker.manage(new RotationSession());
    }

    /** @see ObjectTracker#manage(AutoCloseable) */
    protected AodSession createManagedAodSession() {
        return mObjectTracker.manage(new AodSession());
    }

    /** @see ObjectTracker#manage(AutoCloseable) */
    protected DevEnableNonResizableMultiWindowSession
    createManagedDevEnableNonResizableMultiWindowSession() {
        return mObjectTracker.manage(new DevEnableNonResizableMultiWindowSession());
    }

    /** @see ObjectTracker#manage(AutoCloseable) */
    protected <T extends Activity> TestActivitySession<T> createManagedTestActivitySession() {
        return new TestActivitySession<T>();
    }

    /** @see ObjectTracker#manage(AutoCloseable) */
    protected SystemAlertWindowAppOpSession createAllowSystemAlertWindowAppOpSession() {
        return mObjectTracker.manage(
                new SystemAlertWindowAppOpSession(mContext.getOpPackageName(), MODE_ALLOWED));
    }

    /** @see ObjectTracker#manage(AutoCloseable) */
    protected FontScaleSession createManagedFontScaleSession() {
        return mObjectTracker.manage(new FontScaleSession());
    }

    /**
     * Test @Rule class that disables screen doze settings before each test method running and
     * restoring to initial values after test method finished.
     */
    protected static class DisableScreenDozeRule implements TestRule {

        /** Copied from android.provider.Settings.Secure since these keys are hiden. */
        private static final String[] DOZE_SETTINGS = {
                "doze_enabled",
                "doze_always_on",
                "doze_pulse_on_pick_up",
                "doze_pulse_on_long_press",
                "doze_pulse_on_double_tap",
                "doze_wake_screen_gesture",
                "doze_wake_display_gesture",
                "doze_tap_gesture"
        };

        private String get(String key) {
            return executeShellCommand("settings get secure " + key).trim();
        }

        private void put(String key, String value) {
            executeShellCommand("settings put secure " + key + " " + value);
        }

        @Override
        public Statement apply(final Statement base, final Description description) {
            return new Statement() {
                @Override
                public void evaluate() throws Throwable {
                    final Map<String, String> initialValues = new HashMap<>();
                    Arrays.stream(DOZE_SETTINGS).forEach(k -> initialValues.put(k, get(k)));
                    try {
                        Arrays.stream(DOZE_SETTINGS).forEach(k -> put(k, "0"));
                        base.evaluate();
                    } finally {
                        Arrays.stream(DOZE_SETTINGS).forEach(k -> put(k, initialValues.get(k)));
                    }
                }
            };
        }
    }

    ComponentName getDefaultHomeComponent() {
        final Intent intent = new Intent(ACTION_MAIN);
        intent.addCategory(CATEGORY_HOME);
        intent.addFlags(FLAG_ACTIVITY_NEW_TASK);
        final ResolveInfo resolveInfo =
                mContext.getPackageManager().resolveActivity(intent, MATCH_DEFAULT_ONLY);
        if (resolveInfo == null) {
            throw new AssertionError("Home activity not found");
        }
        return new ComponentName(resolveInfo.activityInfo.packageName,
                resolveInfo.activityInfo.name);
    }

    /**
     * HomeActivitySession is used to replace the default home component, so that you can use
     * your preferred home for testing within the session. The original default home will be
     * restored automatically afterward.
     */
    protected class HomeActivitySession implements AutoCloseable {
        private PackageManager mPackageManager;
        private ComponentName mOrigHome;
        private ComponentName mSessionHome;

        HomeActivitySession(ComponentName sessionHome) {
            mSessionHome = sessionHome;
            mPackageManager = mContext.getPackageManager();
            mOrigHome = getDefaultHomeComponent();

            runWithShellPermission(
                    () -> mPackageManager.setComponentEnabledSetting(mSessionHome,
                            COMPONENT_ENABLED_STATE_ENABLED, DONT_KILL_APP));
            setDefaultHome(mSessionHome);
        }

        @Override
        public void close() {
            runWithShellPermission(
                    () -> mPackageManager.setComponentEnabledSetting(mSessionHome,
                            COMPONENT_ENABLED_STATE_DISABLED, DONT_KILL_APP));
            if (mOrigHome != null) {
                setDefaultHome(mOrigHome);
            }
        }

        private void setDefaultHome(ComponentName componentName) {
            executeShellCommand("cmd package set-home-activity --user "
                    + android.os.Process.myUserHandle().getIdentifier() + " "
                    + componentName.flattenToString());
        }
    }

    public class LockScreenSession implements AutoCloseable {
        private static final boolean DEBUG = false;

        private final boolean mIsLockDisabled;
        private boolean mLockCredentialSet;
        private boolean mRemoveActivitiesOnClose;
        private AmbientDisplayConfiguration mAmbientDisplayConfiguration;

        public static final int FLAG_REMOVE_ACTIVITIES_ON_CLOSE = 1;

        public LockScreenSession() {
            this(0 /* flags */);
        }

        public LockScreenSession(int flags) {
            mIsLockDisabled = isLockDisabled();
            // Enable lock screen (swipe) by default.
            setLockDisabled(false);
            if ((flags & FLAG_REMOVE_ACTIVITIES_ON_CLOSE) != 0) {
                mRemoveActivitiesOnClose = true;
            }
            mAmbientDisplayConfiguration = new AmbientDisplayConfiguration(mContext);
        }

        public LockScreenSession setLockCredential() {
            mLockCredentialSet = true;
            runCommandAndPrintOutput("locksettings set-pin " + LOCK_CREDENTIAL);
            return this;
        }

        public LockScreenSession enterAndConfirmLockCredential() {
            // Ensure focus will switch to default display. Meanwhile we cannot tap on center area,
            // which may tap on input credential area.
            touchAndCancelOnDisplayCenterSync(DEFAULT_DISPLAY);

            waitForDeviceIdle(3000);
            SystemUtil.runWithShellPermissionIdentity(() ->
                    mInstrumentation.sendStringSync(LOCK_CREDENTIAL));
            pressEnterButton();
            return this;
        }

        LockScreenSession disableLockScreen() {
            setLockDisabled(true);
            return this;
        }

        public LockScreenSession sleepDevice() {
            pressSleepButton();
            // Not all device variants lock when we go to sleep, so we need to explicitly lock the
            // device. Note that pressSleepButton() above is redundant because the action also
            // puts the device to sleep, but kept around for clarity.
            if (isWatch()) {
                mInstrumentation.getUiAutomation().performGlobalAction(
                        AccessibilityService.GLOBAL_ACTION_LOCK_SCREEN);
            }
            if (mAmbientDisplayConfiguration.alwaysOnEnabled(
                    android.os.Process.myUserHandle().getIdentifier())) {
                mWmState.waitForAodShowing();
            } else {
                Condition.waitFor("display to turn off", () -> !isDisplayOn(DEFAULT_DISPLAY));
            }
            if(!isLockDisabled()) {
                mWmState.waitFor(state -> state.getKeyguardControllerState().keyguardShowing,
                        "Keyguard showing");
            }
            return this;
        }

        LockScreenSession wakeUpDevice() {
            pressWakeupButton();
            return this;
        }

        LockScreenSession unlockDevice() {
            // Make sure the unlock button event is send to the default display.
            touchAndCancelOnDisplayCenterSync(DEFAULT_DISPLAY);

            pressUnlockButton();
            return this;
        }

        public LockScreenSession gotoKeyguard(ComponentName... showWhenLockedActivities) {
            if (DEBUG && isLockDisabled()) {
                logE("LockScreenSession.gotoKeyguard() is called without lock enabled.");
            }
            sleepDevice();
            wakeUpDevice();
            if (showWhenLockedActivities.length == 0) {
                mWmState.waitForKeyguardShowingAndNotOccluded();
            } else {
                mWmState.waitForValidState(showWhenLockedActivities);
            }
            return this;
        }

        @Override
        public void close() {
            if (mRemoveActivitiesOnClose) {
                removeRootTasksWithActivityTypes(ALL_ACTIVITY_TYPE_BUT_HOME);
            }

            setLockDisabled(mIsLockDisabled);
            final boolean wasCredentialSet = mLockCredentialSet;
            boolean wasDeviceLocked = false;
            if (mLockCredentialSet) {
                wasDeviceLocked = mKm != null && mKm.isDeviceLocked();
                removeLockCredential();
                mLockCredentialSet = false;
            }

            // Dismiss active keyguard after credential is cleared, so keyguard doesn't ask for
            // the stale credential.
            // TODO (b/112015010) If keyguard is occluded, credential cannot be removed as expected.
            // LockScreenSession#close is always called before stopping all test activities,
            // which could cause the keyguard to stay occluded after wakeup.
            // If Keyguard is occluded, pressing the back key can hide the ShowWhenLocked activity.
            pressBackButton();

            // If the credential wasn't set, the steps for restoring can be simpler.
            if (!wasCredentialSet) {
                mWmState.computeState();
                if (WindowManagerStateHelper.isKeyguardShowingAndNotOccluded(mWmState)) {
                    // Keyguard is showing and not occluded so only need to unlock.
                    unlockDevice();
                    return;
                }

                final ComponentName home = mWmState.getHomeActivityName();
                if (home != null && mWmState.hasActivityState(home, STATE_RESUMED)) {
                    // Home is resumed so nothing to do (e.g. after finishing show-when-locked app).
                    return;
                }
            }

            // If device is unlocked, there might have ShowWhenLocked activity runs on,
            // use home key to clear all activity at foreground.
            pressHomeButton();
            if (wasDeviceLocked) {
                // The removal of credential needs an extra cycle to take effect.
                sleepDevice();
                wakeUpDevice();
            }
            if (isKeyguardLocked()) {
                unlockDevice();
            }
        }

        /**
         * Returns whether the lock screen is disabled.
         *
         * @return true if the lock screen is disabled, false otherwise.
         */
        private boolean isLockDisabled() {
            final String isLockDisabled = runCommandAndPrintOutput(
                    "locksettings get-disabled").trim();
            return !"null".equals(isLockDisabled) && Boolean.parseBoolean(isLockDisabled);
        }

        /**
         * Disable the lock screen.
         *
         * @param lockDisabled true if should disable, false otherwise.
         */
        protected void setLockDisabled(boolean lockDisabled) {
            runCommandAndPrintOutput("locksettings set-disabled " + lockDisabled);
        }
    }

    /** Helper class to set and restore appop mode "android:system_alert_window". */
    protected static class SystemAlertWindowAppOpSession implements AutoCloseable {
        private final String mPackageName;
        private final int mPreviousOpMode;

        SystemAlertWindowAppOpSession(String packageName, int mode) {
            mPackageName = packageName;
            try {
                mPreviousOpMode = AppOpsUtils.getOpMode(mPackageName, OPSTR_SYSTEM_ALERT_WINDOW);
            } catch (IOException e) {
                throw new RuntimeException(e);
            }
            setOpMode(mode);
        }

        @Override
        public void close() {
            setOpMode(mPreviousOpMode);
        }

        void setOpMode(int mode) {
            try {
                AppOpsUtils.setOpMode(mPackageName, OPSTR_SYSTEM_ALERT_WINDOW, mode);
            } catch (IOException e) {
                throw new RuntimeException(e);
            }
        }
    }

    protected class AodSession extends SettingsSession<Integer> {
        private AmbientDisplayConfiguration mConfig;

        AodSession() {
            super(Settings.Secure.getUriFor(Settings.Secure.DOZE_ALWAYS_ON),
                    Settings.Secure::getInt,
                    Settings.Secure::putInt);
            mConfig = new AmbientDisplayConfiguration(mContext);
        }

        boolean isAodAvailable() {
            return mConfig.alwaysOnAvailable();
        }

        void setAodEnabled(boolean enabled) {
            set(enabled ? 1 : 0);
        }
    }

    protected class DevEnableNonResizableMultiWindowSession extends SettingsSession<Integer> {
        DevEnableNonResizableMultiWindowSession() {
            super(Settings.Global.getUriFor(
                    Settings.Global.DEVELOPMENT_ENABLE_NON_RESIZABLE_MULTI_WINDOW),
                    (cr, name) -> Settings.Global.getInt(cr, name, 0 /* def */),
                    Settings.Global::putInt);
        }
    }

    /** Helper class to save, set & wait, and restore rotation related preferences. */
    protected class RotationSession extends SettingsSession<Integer> {
        private final String FIXED_TO_USER_ROTATION_COMMAND =
                "cmd window fixed-to-user-rotation ";
        private final SettingsSession<Integer> mAccelerometerRotation;
        private final HandlerThread mThread;
        private final Handler mRunnableHandler;
        private final SettingsObserver mRotationObserver;
        private int mPreviousDegree;
        private String mPreviousFixedToUserRotationMode;

        public RotationSession() {
            // Save user_rotation and accelerometer_rotation preferences.
            super(Settings.System.getUriFor(Settings.System.USER_ROTATION),
                    Settings.System::getInt, Settings.System::putInt);
            mAccelerometerRotation = new SettingsSession<>(
                    Settings.System.getUriFor(Settings.System.ACCELEROMETER_ROTATION),
                    Settings.System::getInt, Settings.System::putInt);

            mThread = new HandlerThread("Observer_Thread");
            mThread.start();
            mRunnableHandler = new Handler(mThread.getLooper());
            mRotationObserver = new SettingsObserver(mRunnableHandler);

            // Disable fixed to user rotation
            mPreviousFixedToUserRotationMode = executeShellCommand(FIXED_TO_USER_ROTATION_COMMAND);
            executeShellCommand(FIXED_TO_USER_ROTATION_COMMAND + "disabled");

            mPreviousDegree = get();
            // Disable accelerometer_rotation.
            mAccelerometerRotation.set(0);
        }

        @Override
        public void set(@NonNull Integer value) {
            set(value, true /* waitDeviceRotation */);
        }

        /**
         * Sets the rotation preference.
         *
         * @param value The rotation between {@link android.view.Surface#ROTATION_0} ~
         *              {@link android.view.Surface#ROTATION_270}
         * @param waitDeviceRotation If {@code true}, it will wait until the display has applied the
         *                           rotation. Otherwise it only waits for the settings value has
         *                           been changed.
         */
        public void set(@NonNull Integer value, boolean waitDeviceRotation) {
            // When the rotation is locked and the SystemUI receives the rotation becoming 0deg, it
            // will call freezeRotation to WMS, which will cause USER_ROTATION be set to zero again.
            // In order to prevent our test target from being overwritten by SystemUI during
            // rotation test, wait for the USER_ROTATION changed then continue testing.
            final boolean waitSystemUI = value == ROTATION_0 && mPreviousDegree != ROTATION_0;
            final boolean observeRotationSettings = waitSystemUI || !waitDeviceRotation;
            if (observeRotationSettings) {
                mRotationObserver.observe();
            }
            super.set(value);
            mPreviousDegree = value;

            if (waitSystemUI) {
                Condition.waitFor(new Condition<>("rotation notified",
                        // There will receive USER_ROTATION changed twice because when the device
                        // rotates to 0deg, RotationContextButton will also set ROTATION_0 again.
                        () -> mRotationObserver.count == 2).setRetryIntervalMs(500));
            }

            if (waitDeviceRotation) {
                // Wait for the display to apply the rotation.
                mWmState.waitForRotation(value);
            } else {
                // Wait for the settings have been changed.
                Condition.waitFor(new Condition<>("rotation setting changed",
                        () -> mRotationObserver.count > 0).setRetryIntervalMs(100));
            }

            if (observeRotationSettings) {
                mRotationObserver.stopObserver();
            }
        }

        @Override
        public void close() {
            // Restore fixed to user rotation to default
            executeShellCommand(FIXED_TO_USER_ROTATION_COMMAND + mPreviousFixedToUserRotationMode);
            mThread.quitSafely();
            super.close();
            // Restore accelerometer_rotation preference.
            mAccelerometerRotation.close();
        }

        private class SettingsObserver extends ContentObserver {
            int count;

            SettingsObserver(Handler handler) { super(handler); }

            void observe() {
                count = 0;
                final ContentResolver resolver = mContext.getContentResolver();
                resolver.registerContentObserver(Settings.System.getUriFor(
                        Settings.System.USER_ROTATION), false, this);
            }

            void stopObserver() {
                count = 0;
                final ContentResolver resolver = mContext.getContentResolver();
                resolver.unregisterContentObserver(this);
            }

            @Override
            public void onChange(boolean selfChange) {
                count++;
            }
        }
    }

    /** Helper class to save, set, and restore font_scale preferences. */
    protected static class FontScaleSession extends SettingsSession<Float> {
        FontScaleSession() {
            super(Settings.System.getUriFor(Settings.System.FONT_SCALE),
                    Settings.System::getFloat,
                    Settings.System::putFloat);
        }

        @Override
        public Float get() {
            Float value = super.get();
            return value == null ? 1f : value;
        }
    }

    /**
     * Returns whether the test device respects settings of locked user rotation mode.
     *
     * The method sets the locked user rotation settings to the rotation that rotates the display by
     * 180 degrees and checks if the actual display rotation changes after that.
     *
     * This is a necessary assumption check before leveraging user rotation mode to force display
     * rotation, because there is no requirement that an Android device that supports both
     * orientations needs to support user rotation mode.
     *
     * @param session   the rotation session used to set user rotation
     * @param displayId the display ID to check rotation against
     * @return {@code true} if test device respects settings of locked user rotation mode;
     * {@code false} if not.
     */
    protected boolean supportsLockedUserRotation(RotationSession session, int displayId) {
        final int origRotation = getDeviceRotation(displayId);
        // Use the same orientation as target rotation to avoid affect of app-requested orientation.
        final int targetRotation = (origRotation + 2) % 4;
        session.set(targetRotation);
        final boolean result = (getDeviceRotation(displayId) == targetRotation);
        session.set(origRotation);
        return result;
    }

    protected int getDeviceRotation(int displayId) {
        final String displays = runCommandAndPrintOutput("dumpsys display displays").trim();
        Pattern pattern = Pattern.compile(
                "(mDisplayId=" + displayId + ")([\\s\\S]*?)(mOverrideDisplayInfo)(.*)"
                        + "(rotation)(\\s+)(\\d+)");
        Matcher matcher = pattern.matcher(displays);
        if (matcher.find()) {
            final String match = matcher.group(7);
            return Integer.parseInt(match);
        }

        return INVALID_DEVICE_ROTATION;
    }

    /**
     * Creates a {#link ActivitySessionClient} instance with instrumentation context. It is used
     * when the caller doen't need try-with-resource.
     */
    public static ActivitySessionClient createActivitySessionClient() {
        return new ActivitySessionClient(getInstrumentation().getContext());
    }

    /** Empties the test journal so the following events won't be mixed-up with previous records. */
    protected void separateTestJournal() {
        TestJournalContainer.start();
    }

    protected static String runCommandAndPrintOutput(String command) {
        final String output = executeShellCommand(command);
        log(output);
        return output;
    }

    protected static class LogSeparator {
        private final String mUniqueString;

        private LogSeparator() {
            mUniqueString = UUID.randomUUID().toString();
        }

        @Override
        public String toString() {
            return mUniqueString;
        }
    }

    /**
     * Inserts a log separator so we can always find the starting point from where to evaluate
     * following logs.
     *
     * @return Unique log separator.
     */
    protected LogSeparator separateLogs() {
        final LogSeparator logSeparator = new LogSeparator();
        executeShellCommand("log -t " + LOG_SEPARATOR + " " + logSeparator);
        EventLog.writeEvent(EVENT_LOG_SEPARATOR_TAG, logSeparator.mUniqueString);
        return logSeparator;
    }

    protected static String[] getDeviceLogsForComponents(
            LogSeparator logSeparator, String... logTags) {
        String filters = LOG_SEPARATOR + ":I ";
        for (String component : logTags) {
            filters += component + ":I ";
        }
        final String[] result = executeShellCommand("logcat -v brief -d " + filters + " *:S")
                .split("\\n");
        if (logSeparator == null) {
            return result;
        }

        // Make sure that we only check logs after the separator.
        int i = 0;
        boolean lookingForSeparator = true;
        while (i < result.length && lookingForSeparator) {
            if (result[i].contains(logSeparator.toString())) {
                lookingForSeparator = false;
            }
            i++;
        }
        final String[] filteredResult = new String[result.length - i];
        for (int curPos = 0; i < result.length; curPos++, i++) {
            filteredResult[curPos] = result[i];
        }
        return filteredResult;
    }

    protected static List<Event> getEventLogsForComponents(LogSeparator logSeparator, int... tags) {
        List<Event> events = new ArrayList<>();

        int[] searchTags = Arrays.copyOf(tags, tags.length + 1);
        searchTags[searchTags.length - 1] = EVENT_LOG_SEPARATOR_TAG;

        try {
            EventLog.readEvents(searchTags, events);
        } catch (IOException e) {
            fail("Could not read from event log." + e);
        }

        for (Iterator<Event> itr = events.iterator(); itr.hasNext(); ) {
            Event event = itr.next();
            itr.remove();
            if (event.getTag() == EVENT_LOG_SEPARATOR_TAG &&
                    logSeparator.mUniqueString.equals(event.getData())) {
                break;
            }
        }
        return events;
    }

    protected boolean supportsMultiDisplay() {
        return mContext.getPackageManager().hasSystemFeature(
                FEATURE_ACTIVITIES_ON_SECONDARY_DISPLAYS);
    }

    protected boolean supportsInstallableIme() {
        return mContext.getPackageManager().hasSystemFeature(FEATURE_INPUT_METHODS);
    }

    static class CountSpec<T> {
        static final int DONT_CARE = Integer.MIN_VALUE;
        static final int EQUALS = 1;
        static final int GREATER_THAN = 2;
        static final int LESS_THAN = 3;

        final T mEvent;
        final int mRule;
        final int mCount;
        final String mMessage;

        CountSpec(T event, int rule, int count, String message) {
            mEvent = event;
            mRule = count == DONT_CARE ? DONT_CARE : rule;
            mCount = count;
            if (message != null) {
                mMessage = message;
            } else {
                switch (rule) {
                    case EQUALS:
                        mMessage = event + " must equal to " + count;
                        break;
                    case GREATER_THAN:
                        mMessage = event + " must be greater than " + count;
                        break;
                    case LESS_THAN:
                        mMessage = event + " must be less than " + count;
                        break;
                    default:
                        mMessage = "Don't care";
                }
            }
        }

        /** @return {@code true} if the given value is satisfied the condition. */
        boolean validate(int value) {
            switch (mRule) {
                case DONT_CARE:
                    return true;
                case EQUALS:
                    return value == mCount;
                case GREATER_THAN:
                    return value > mCount;
                case LESS_THAN:
                    return value < mCount;
                default:
            }
            throw new RuntimeException("Unknown CountSpec rule");
        }
    }

    static <T> CountSpec<T> countSpec(T event, int rule, int count, String message) {
        return new CountSpec<>(event, rule, count, message);
    }

    static <T> CountSpec<T> countSpec(T event, int rule, int count) {
        return new CountSpec<>(event, rule, count, null /* message */);
    }

    static void assertLifecycleCounts(ComponentName activityName, String message,
            int createCount, int startCount, int resumeCount, int pauseCount, int stopCount,
            int destroyCount, int configChangeCount) {
        new ActivityLifecycleCounts(activityName).assertCountWithRetry(
                message,
                countSpec(ActivityCallback.ON_CREATE, CountSpec.EQUALS, createCount),
                countSpec(ActivityCallback.ON_START, CountSpec.EQUALS, startCount),
                countSpec(ActivityCallback.ON_RESUME, CountSpec.EQUALS, resumeCount),
                countSpec(ActivityCallback.ON_PAUSE, CountSpec.EQUALS, pauseCount),
                countSpec(ActivityCallback.ON_STOP, CountSpec.EQUALS, stopCount),
                countSpec(ActivityCallback.ON_DESTROY, CountSpec.EQUALS, destroyCount),
                countSpec(ActivityCallback.ON_CONFIGURATION_CHANGED, CountSpec.EQUALS,
                        configChangeCount));
    }

    static void assertLifecycleCounts(ComponentName activityName,
            int createCount, int startCount, int resumeCount, int pauseCount, int stopCount,
            int destroyCount, int configChangeCount) {
        assertLifecycleCounts(activityName, "Assert lifecycle of " + getLogTag(activityName),
                createCount, startCount, resumeCount, pauseCount, stopCount,
                destroyCount, configChangeCount);
    }

    static void assertSingleLaunch(ComponentName activityName) {
        assertLifecycleCounts(activityName,
                "activity create, start, and resume",
                1 /* createCount */, 1 /* startCount */, 1 /* resumeCount */,
                0 /* pauseCount */, 0 /* stopCount */, 0 /* destroyCount */,
                CountSpec.DONT_CARE /* configChangeCount */);
    }

    static void assertSingleLaunchAndStop(ComponentName activityName) {
        assertLifecycleCounts(activityName,
                "activity create, start, resume, pause, and stop",
                1 /* createCount */, 1 /* startCount */, 1 /* resumeCount */,
                1 /* pauseCount */, 1 /* stopCount */, 0 /* destroyCount */,
                CountSpec.DONT_CARE /* configChangeCount */);
    }

    static void assertSingleStartAndStop(ComponentName activityName) {
        assertLifecycleCounts(activityName,
                "activity start, resume, pause, and stop",
                0 /* createCount */, 1 /* startCount */, 1 /* resumeCount */,
                1 /* pauseCount */, 1 /* stopCount */, 0 /* destroyCount */,
                CountSpec.DONT_CARE /* configChangeCount */);
    }

    static void assertSingleStart(ComponentName activityName) {
        assertLifecycleCounts(activityName,
                "activity start and resume",
                0 /* createCount */, 1 /* startCount */, 1 /* resumeCount */,
                0 /* pauseCount */, 0 /* stopCount */, 0 /* destroyCount */,
                CountSpec.DONT_CARE /* configChangeCount */);
    }

    /** Assert the activity is either relaunched or received configuration changed. */
    static void assertActivityLifecycle(ComponentName activityName, boolean relaunched) {
        Condition.<String>waitForResult(
                activityName + (relaunched ? " relaunched" : " config changed"),
                condition -> condition
                .setResultSupplier(() -> checkActivityIsRelaunchedOrConfigurationChanged(
                        getActivityName(activityName),
                        TestJournalContainer.get(activityName).callbacks, relaunched))
                .setResultValidator(failedReasons -> failedReasons == null)
                .setOnFailure(failedReasons -> fail(failedReasons)));
    }

    /** Assert the activity is either relaunched or received configuration changed. */
    static List<ActivityCallback> assertActivityLifecycle(ActivitySession activitySession,
            boolean relaunched) {
        final String name = activitySession.getName().flattenToShortString();
        final List<ActivityCallback> callbackHistory = activitySession.takeCallbackHistory();
        String failedReason = checkActivityIsRelaunchedOrConfigurationChanged(
                name, callbackHistory, relaunched);
        if (failedReason != null) {
            fail(failedReason);
        }
        return callbackHistory;
    }

    private static String checkActivityIsRelaunchedOrConfigurationChanged(String name,
            List<ActivityCallback> callbackHistory, boolean relaunched) {
        final ActivityLifecycleCounts lifecycles = new ActivityLifecycleCounts(callbackHistory);
        if (relaunched) {
            return lifecycles.validateCount(
                    countSpec(ActivityCallback.ON_DESTROY, CountSpec.GREATER_THAN, 0,
                            name + " must have been destroyed."),
                    countSpec(ActivityCallback.ON_CREATE, CountSpec.GREATER_THAN, 0,
                            name + " must have been (re)created."));
        }
        return lifecycles.validateCount(
                countSpec(ActivityCallback.ON_DESTROY, CountSpec.LESS_THAN, 1,
                        name + " must *NOT* have been destroyed."),
                countSpec(ActivityCallback.ON_CREATE, CountSpec.LESS_THAN, 1,
                        name + " must *NOT* have been (re)created."),
                countSpec(ActivityCallback.ON_CONFIGURATION_CHANGED, CountSpec.GREATER_THAN, 0,
                                name + " must have received configuration changed."));
    }

    static void assertRelaunchOrConfigChanged(ComponentName activityName, int numRelaunch,
            int numConfigChange) {
        new ActivityLifecycleCounts(activityName).assertCountWithRetry("relaunch or config changed",
                countSpec(ActivityCallback.ON_DESTROY, CountSpec.EQUALS, numRelaunch),
                countSpec(ActivityCallback.ON_CREATE, CountSpec.EQUALS, numRelaunch),
                countSpec(ActivityCallback.ON_CONFIGURATION_CHANGED, CountSpec.EQUALS,
                        numConfigChange));
    }

    static void assertActivityDestroyed(ComponentName activityName) {
        new ActivityLifecycleCounts(activityName).assertCountWithRetry("activity destroyed",
                countSpec(ActivityCallback.ON_DESTROY, CountSpec.EQUALS, 1),
                countSpec(ActivityCallback.ON_CREATE, CountSpec.EQUALS, 0),
                countSpec(ActivityCallback.ON_CONFIGURATION_CHANGED, CountSpec.EQUALS, 0));
    }

    static void assertSecurityExceptionFromActivityLauncher() {
        waitForOrFail("SecurityException from " + ActivityLauncher.TAG,
                ActivityLauncher::hasCaughtSecurityException);
    }

    private static final Pattern sCurrentUiModePattern = Pattern.compile("mCurUiMode=0x(\\d+)");
    private static final Pattern sUiModeLockedPattern =
            Pattern.compile("mUiModeLocked=(true|false)");

    @NonNull
    SizeInfo getLastReportedSizesForActivity(ComponentName activityName) {
        return Condition.waitForResult("sizes of " + activityName + " to be reported",
                condition -> condition.setResultSupplier(() -> {
                    final ConfigInfo info = TestJournalContainer.get(activityName).lastConfigInfo;
                    return info != null ? info.sizeInfo : null;
                }).setResultValidator(Objects::nonNull).setOnFailure(unusedResult ->
                        fail("No config reported from " + activityName)));
    }

    /** Check if a device has display cutout. */
    boolean hasDisplayCutout() {
        // Launch an activity to report cutout state
        separateTestJournal();
        launchActivity(BROADCAST_RECEIVER_ACTIVITY);

        // Read the logs to check if cutout is present
        final Boolean displayCutoutPresent = getCutoutStateForActivity(BROADCAST_RECEIVER_ACTIVITY);
        assertNotNull("The activity should report cutout state", displayCutoutPresent);

        // Finish activity
        mBroadcastActionTrigger.finishBroadcastReceiverActivity();
        mWmState.waitForWithAmState(
                (state) -> !state.containsActivity(BROADCAST_RECEIVER_ACTIVITY),
                "activity to be removed");

        return displayCutoutPresent;
    }

    /**
     * Wait for activity to report cutout state in logs and return it. Will return {@code null}
     * after timeout.
     */
    @Nullable
    private Boolean getCutoutStateForActivity(ComponentName activityName) {
        return Condition.waitForResult("cutout state to be reported", condition -> condition
                .setResultSupplier(() -> {
                    final Bundle extras = TestJournalContainer.get(activityName).extras;
                    return extras.containsKey(EXTRA_CUTOUT_EXISTS)
                            ? extras.getBoolean(EXTRA_CUTOUT_EXISTS)
                            : null;
                }).setResultValidator(cutoutExists -> cutoutExists != null));
    }

    /** Waits for at least one onMultiWindowModeChanged event. */
    ActivityLifecycleCounts waitForOnMultiWindowModeChanged(ComponentName activityName) {
        final ActivityLifecycleCounts counts = new ActivityLifecycleCounts(activityName);
        Condition.waitFor(counts.countWithRetry("waitForOnMultiWindowModeChanged", countSpec(
                ActivityCallback.ON_MULTI_WINDOW_MODE_CHANGED, CountSpec.GREATER_THAN, 0)));
        return counts;
    }

    WindowState getPackageWindowState(String packageName) {
        final WindowManagerState.WindowState window =
                mWmState.getWindowByPackageName(packageName, TYPE_BASE_APPLICATION);
        assertNotNull(window);
        return window;
    }

    static class ActivityLifecycleCounts {
        private final int[] mCounts = new int[ActivityCallback.SIZE];
        private final int[] mFirstIndexes = new int[ActivityCallback.SIZE];
        private final int[] mLastIndexes = new int[ActivityCallback.SIZE];
        private ComponentName mActivityName;

        ActivityLifecycleCounts(ComponentName componentName) {
            mActivityName = componentName;
            updateCount(TestJournalContainer.get(componentName).callbacks);
        }

        ActivityLifecycleCounts(List<ActivityCallback> callbacks) {
            updateCount(callbacks);
        }

        private void updateCount(List<ActivityCallback> callbacks) {
            // The callback list could be from the reference of TestJournal. If we are counting for
            // retrying, there may be new data added to the list from other threads.
            TestJournalContainer.withThreadSafeAccess(() -> {
                Arrays.fill(mFirstIndexes, -1);
                for (int i = 0; i < callbacks.size(); i++) {
                    final ActivityCallback callback = callbacks.get(i);
                    final int ordinal = callback.ordinal();
                    mCounts[ordinal]++;
                    mLastIndexes[ordinal] = i;
                    if (mFirstIndexes[ordinal] == -1) {
                        mFirstIndexes[ordinal] = i;
                    }
                }
            });
        }

        int getCount(ActivityCallback callback) {
            return mCounts[callback.ordinal()];
        }

        int getFirstIndex(ActivityCallback callback) {
            return mFirstIndexes[callback.ordinal()];
        }

        int getLastIndex(ActivityCallback callback) {
            return mLastIndexes[callback.ordinal()];
        }

        @SafeVarargs
        final Condition<String> countWithRetry(String message,
                CountSpec<ActivityCallback>... countSpecs) {
            if (mActivityName == null) {
                throw new IllegalStateException(
                        "It is meaningless to retry without specified activity");
            }
            return new Condition<String>(message)
                    .setOnRetry(() -> {
                        Arrays.fill(mCounts, 0);
                        Arrays.fill(mLastIndexes, 0);
                        updateCount(TestJournalContainer.get(mActivityName).callbacks);
                    })
                    .setResultSupplier(() -> validateCount(countSpecs))
                    .setResultValidator(failedReasons -> failedReasons == null);
        }

        @SafeVarargs
        final void assertCountWithRetry(String message, CountSpec<ActivityCallback>... countSpecs) {
            if (mActivityName == null) {
                throw new IllegalStateException(
                        "It is meaningless to retry without specified activity");
            }
            Condition.<String>waitForResult(countWithRetry(message, countSpecs)
                    .setOnFailure(failedReasons -> fail(message + ": " + failedReasons)));
        }

        @SafeVarargs
        final String validateCount(CountSpec<ActivityCallback>... countSpecs) {
            ArrayList<String> failedReasons = null;
            for (CountSpec<ActivityCallback> spec : countSpecs) {
                final int realCount = mCounts[spec.mEvent.ordinal()];
                if (!spec.validate(realCount)) {
                    if (failedReasons == null) {
                        failedReasons = new ArrayList<>();
                    }
                    failedReasons.add(spec.mMessage + " (got " + realCount + ")");
                }
            }
            return failedReasons == null ? null : String.join("\n", failedReasons);
        }
    }

    protected void stopTestPackage(final String packageName) {
        runWithShellPermission(() -> mAm.forceStopPackage(packageName));
    }

    protected LaunchActivityBuilder getLaunchActivityBuilder() {
        return new LaunchActivityBuilder(mWmState);
    }

    public static <T extends Activity>
    ActivityScenarioRule<T> createFullscreenActivityScenarioRule(Class<T> clazz) {
        final ActivityOptions options = ActivityOptions.makeBasic();
        options.setLaunchWindowingMode(WindowConfiguration.WINDOWING_MODE_FULLSCREEN);
        return new ActivityScenarioRule<>(clazz, options.toBundle());
    }

    protected static class LaunchActivityBuilder implements LaunchProxy {
        private final WindowManagerStateHelper mAmWmState;

        // The activity to be launched
        private ComponentName mTargetActivity = TEST_ACTIVITY;
        private boolean mUseApplicationContext;
        private boolean mToSide;
        private boolean mRandomData;
        private boolean mNewTask;
        private boolean mMultipleTask;
        private boolean mAllowMultipleInstances = true;
        private boolean mLaunchTaskBehind;
        private boolean mFinishBeforeLaunch;
        private int mDisplayId = INVALID_DISPLAY;
        private int mWindowingMode = -1;
        private int mActivityType = ACTIVITY_TYPE_UNDEFINED;
        // A proxy activity that launches other activities including mTargetActivityName
        private ComponentName mLaunchingActivity = LAUNCHING_ACTIVITY;
        private boolean mReorderToFront;
        private boolean mWaitForLaunched;
        private boolean mSuppressExceptions;
        private boolean mWithShellPermission;
        // Use of the following variables indicates that a broadcast receiver should be used instead
        // of a launching activity;
        private ComponentName mBroadcastReceiver;
        private String mBroadcastReceiverAction;
        private int mIntentFlags;
        private Bundle mExtras;
        private LaunchInjector mLaunchInjector;
        private ActivitySessionClient mActivitySessionClient;

        private enum LauncherType {
            INSTRUMENTATION, LAUNCHING_ACTIVITY, BROADCAST_RECEIVER
        }

        private LauncherType mLauncherType = LauncherType.LAUNCHING_ACTIVITY;

        public LaunchActivityBuilder(WindowManagerStateHelper amWmState) {
            mAmWmState = amWmState;
            mWaitForLaunched = true;
            mWithShellPermission = true;
        }

        public LaunchActivityBuilder setToSide(boolean toSide) {
            mToSide = toSide;
            return this;
        }

        public LaunchActivityBuilder setRandomData(boolean randomData) {
            mRandomData = randomData;
            return this;
        }

        public LaunchActivityBuilder setNewTask(boolean newTask) {
            mNewTask = newTask;
            return this;
        }

        public LaunchActivityBuilder setMultipleTask(boolean multipleTask) {
            mMultipleTask = multipleTask;
            return this;
        }

        public LaunchActivityBuilder allowMultipleInstances(boolean allowMultipleInstances) {
            mAllowMultipleInstances = allowMultipleInstances;
            return this;
        }

        public LaunchActivityBuilder setLaunchTaskBehind(boolean launchTaskBehind) {
            mLaunchTaskBehind = launchTaskBehind;
            return this;
        }

        public LaunchActivityBuilder setReorderToFront(boolean reorderToFront) {
            mReorderToFront = reorderToFront;
            return this;
        }

        public LaunchActivityBuilder setUseApplicationContext(boolean useApplicationContext) {
            mUseApplicationContext = useApplicationContext;
            return this;
        }

        public LaunchActivityBuilder setFinishBeforeLaunch(boolean finishBeforeLaunch) {
            mFinishBeforeLaunch = finishBeforeLaunch;
            return this;
        }

        public ComponentName getTargetActivity() {
            return mTargetActivity;
        }

        public boolean isTargetActivityTranslucent() {
            return mAmWmState.isActivityTranslucent(mTargetActivity);
        }

        public LaunchActivityBuilder setTargetActivity(ComponentName targetActivity) {
            mTargetActivity = targetActivity;
            return this;
        }

        public LaunchActivityBuilder setDisplayId(int id) {
            mDisplayId = id;
            return this;
        }

        public LaunchActivityBuilder setWindowingMode(int windowingMode) {
            mWindowingMode = windowingMode;
            return this;
        }

        public LaunchActivityBuilder setActivityType(int type) {
            mActivityType = type;
            return this;
        }

        public LaunchActivityBuilder setLaunchingActivity(ComponentName launchingActivity) {
            mLaunchingActivity = launchingActivity;
            mLauncherType = LauncherType.LAUNCHING_ACTIVITY;
            return this;
        }

        public LaunchActivityBuilder setWaitForLaunched(boolean shouldWait) {
            mWaitForLaunched = shouldWait;
            return this;
        }

        /** Use broadcast receiver as a launchpad for activities. */
        public LaunchActivityBuilder setUseBroadcastReceiver(final ComponentName broadcastReceiver,
                final String broadcastAction) {
            mBroadcastReceiver = broadcastReceiver;
            mBroadcastReceiverAction = broadcastAction;
            mLauncherType = LauncherType.BROADCAST_RECEIVER;
            return this;
        }

        /** Use {@link android.app.Instrumentation} as a launchpad for activities. */
        public LaunchActivityBuilder setUseInstrumentation() {
            mLauncherType = LauncherType.INSTRUMENTATION;
            // Calling startActivity() from outside of an Activity context requires the
            // FLAG_ACTIVITY_NEW_TASK flag.
            setNewTask(true);
            return this;
        }

        public LaunchActivityBuilder setSuppressExceptions(boolean suppress) {
            mSuppressExceptions = suppress;
            return this;
        }

        public LaunchActivityBuilder setWithShellPermission(boolean withShellPermission) {
            mWithShellPermission = withShellPermission;
            return this;
        }

        public LaunchActivityBuilder setActivitySessionClient(ActivitySessionClient sessionClient) {
            mActivitySessionClient = sessionClient;
            return this;
        }

        @Override
        public boolean shouldWaitForLaunched() {
            return mWaitForLaunched;
        }

        public LaunchActivityBuilder setIntentFlags(int flags) {
            mIntentFlags = flags;
            return this;
        }

        public LaunchActivityBuilder setIntentExtra(Consumer<Bundle> extrasConsumer) {
            if (extrasConsumer != null) {
                mExtras = new Bundle();
                extrasConsumer.accept(mExtras);
            }
            return this;
        }

        @Override
        public Bundle getExtras() {
            return mExtras;
        }

        @Override
        public void setLaunchInjector(LaunchInjector injector) {
            mLaunchInjector = injector;
        }

        @Override
        public void execute() {
            if (mActivitySessionClient != null) {
                final ActivitySessionClient client = mActivitySessionClient;
                // Clear the session client so its startActivity can call the real execute().
                mActivitySessionClient = null;
                client.startActivity(this);
                return;
            }
            switch (mLauncherType) {
                case INSTRUMENTATION:
                    if (mWithShellPermission) {
                        NestedShellPermission.run(this::launchUsingInstrumentation);
                    } else {
                        launchUsingInstrumentation();
                    }
                    break;
                case LAUNCHING_ACTIVITY:
                case BROADCAST_RECEIVER:
                    launchUsingShellCommand();
            }

            if (mWaitForLaunched) {
                mAmWmState.waitForValidState(mTargetActivity);
            }
        }

        /** Launch an activity using instrumentation. */
        private void launchUsingInstrumentation() {
            final Bundle b = new Bundle();
            b.putBoolean(KEY_LAUNCH_ACTIVITY, true);
            b.putBoolean(KEY_LAUNCH_TO_SIDE, mToSide);
            b.putBoolean(KEY_RANDOM_DATA, mRandomData);
            b.putBoolean(KEY_NEW_TASK, mNewTask);
            b.putBoolean(KEY_MULTIPLE_TASK, mMultipleTask);
            b.putBoolean(KEY_MULTIPLE_INSTANCES, mAllowMultipleInstances);
            b.putBoolean(KEY_LAUNCH_TASK_BEHIND, mLaunchTaskBehind);
            b.putBoolean(KEY_REORDER_TO_FRONT, mReorderToFront);
            b.putInt(KEY_DISPLAY_ID, mDisplayId);
            b.putInt(KEY_WINDOWING_MODE, mWindowingMode);
            b.putInt(KEY_ACTIVITY_TYPE, mActivityType);
            b.putBoolean(KEY_USE_APPLICATION_CONTEXT, mUseApplicationContext);
            b.putString(KEY_TARGET_COMPONENT, getActivityName(mTargetActivity));
            b.putBoolean(KEY_SUPPRESS_EXCEPTIONS, mSuppressExceptions);
            b.putInt(KEY_INTENT_FLAGS, mIntentFlags);
            b.putBundle(KEY_INTENT_EXTRAS, getExtras());
            final Context context = getInstrumentation().getContext();
            launchActivityFromExtras(context, b, mLaunchInjector);
        }

        /** Build and execute a shell command to launch an activity. */
        private void launchUsingShellCommand() {
            StringBuilder commandBuilder = new StringBuilder();
            if (mBroadcastReceiver != null && mBroadcastReceiverAction != null) {
                // Use broadcast receiver to launch the target.
                commandBuilder.append("am broadcast -a ").append(mBroadcastReceiverAction)
                        .append(" -p ").append(mBroadcastReceiver.getPackageName())
                        // Include stopped packages
                        .append(" -f 0x00000020");
            } else {
                // If new task flag isn't set the windowing mode of launcher activity will be the
                // windowing mode of the target activity, so we need to launch launcher activity in
                // it.
                String amStartCmd =
                        (mWindowingMode == -1 || mNewTask)
                                ? getAmStartCmd(mLaunchingActivity)
                                : getAmStartCmd(mLaunchingActivity, mWindowingMode);
                // Use launching activity to launch the target.
                commandBuilder.append(amStartCmd)
                        .append(" -f 0x20000020");
            }

            // Add a flag to ensure we actually mean to launch an activity.
            commandBuilder.append(" --ez " + KEY_LAUNCH_ACTIVITY + " true");

            if (mToSide) {
                commandBuilder.append(" --ez " + KEY_LAUNCH_TO_SIDE + " true");
            }
            if (mRandomData) {
                commandBuilder.append(" --ez " + KEY_RANDOM_DATA + " true");
            }
            if (mNewTask) {
                commandBuilder.append(" --ez " + KEY_NEW_TASK + " true");
            }
            if (mMultipleTask) {
                commandBuilder.append(" --ez " + KEY_MULTIPLE_TASK + " true");
            }
            if (mAllowMultipleInstances) {
                commandBuilder.append(" --ez " + KEY_MULTIPLE_INSTANCES + " true");
            }
            if (mReorderToFront) {
                commandBuilder.append(" --ez " + KEY_REORDER_TO_FRONT + " true");
            }
            if (mFinishBeforeLaunch) {
                commandBuilder.append(" --ez " + KEY_FINISH_BEFORE_LAUNCH + " true");
            }
            if (mDisplayId != INVALID_DISPLAY) {
                commandBuilder.append(" --ei " + KEY_DISPLAY_ID + " ").append(mDisplayId);
            }
            if (mWindowingMode != -1) {
                commandBuilder.append(" --ei " + KEY_WINDOWING_MODE + " ").append(mWindowingMode);
            }
            if (mActivityType != ACTIVITY_TYPE_UNDEFINED) {
                commandBuilder.append(" --ei " + KEY_ACTIVITY_TYPE + " ").append(mActivityType);
            }

            if (mUseApplicationContext) {
                commandBuilder.append(" --ez " + KEY_USE_APPLICATION_CONTEXT + " true");
            }

            if (mTargetActivity != null) {
                // {@link ActivityLauncher} parses this extra string by
                // {@link ComponentName#unflattenFromString(String)}.
                commandBuilder.append(" --es " + KEY_TARGET_COMPONENT + " ")
                        .append(getActivityName(mTargetActivity));
            }

            if (mSuppressExceptions) {
                commandBuilder.append(" --ez " + KEY_SUPPRESS_EXCEPTIONS + " true");
            }

            if (mIntentFlags != 0) {
                commandBuilder.append(" --ei " + KEY_INTENT_FLAGS + " ").append(mIntentFlags);
            }

            if (mLaunchInjector != null) {
                commandBuilder.append(" --ez " + KEY_FORWARD + " true");
                mLaunchInjector.setupShellCommand(commandBuilder);
            }
            executeShellCommand(commandBuilder.toString());
        }
    }

    /**
     * The actions which wraps a test method. It is used to set necessary rules that cannot be
     * overridden by subclasses. It executes in the outer scope of {@link Before} and {@link After}.
     */
    protected class WrapperRule implements TestRule {
        private final Runnable mBefore;
        private final Runnable mAfter;

        protected WrapperRule(Runnable before, Runnable after) {
            mBefore = before;
            mAfter = after;
        }

        @Override
        public Statement apply(final Statement base, final Description description) {
            return new Statement() {
                @Override
                public void evaluate()  {
                    if (mBefore != null) {
                        mBefore.run();
                    }
                    try {
                        base.evaluate();
                    } catch (Throwable e) {
                        mPostAssertionRule.addError(e);
                    } finally {
                        if (mAfter != null) {
                            mAfter.run();
                        }
                    }
                }
            };
        }
    }

    /**
     * The post assertion to ensure all test methods don't violate the generic rule. It is also used
     * to collect multiple errors.
     */
    private class PostAssertionRule extends ErrorCollector {
        private Throwable mLastError;

        @Override
        protected void verify() throws Throwable {
            if (mLastError != null) {
                // Try to recover the bad state of device to avoid subsequent test failures.
                if (isKeyguardLocked()) {
                    mLastError.addSuppressed(new IllegalStateException("Keyguard is locked"));
                    // To clear the credential immediately, the screen need to be turned on.
                    pressWakeupButton();
                    removeLockCredential();
                    // Off/on to refresh the keyguard state.
                    pressSleepButton();
                    pressWakeupButton();
                    pressUnlockButton();
                }
                final String overlayDisplaySettings = Settings.Global.getString(
                        mContext.getContentResolver(), Settings.Global.OVERLAY_DISPLAY_DEVICES);
                if (overlayDisplaySettings != null && overlayDisplaySettings.length() > 0) {
                    mLastError.addSuppressed(new IllegalStateException(
                            "Overlay display is found: " + overlayDisplaySettings));
                    // Remove the overlay display because it may obscure the screen and causes the
                    // next tests to fail.
                    SettingsSession.delete(Settings.Global.getUriFor(
                            Settings.Global.OVERLAY_DISPLAY_DEVICES));
                }
            }
            if (!sIllegalTaskStateFound) {
                // Skip if a illegal task state was already found in previous test, or all tests
                // afterward could also fail and fire unnecessary false alarms.
                try {
                    mWmState.assertIllegalTaskState();
                } catch (Throwable t) {
                    sIllegalTaskStateFound = true;
                    addError(t);
                }
            }
            super.verify();
        }

        @Override
        public void addError(Throwable error) {
            super.addError(error);
            logE("addError: " + error);
            mLastError = error;
        }
    }

    /** Activity that can handle all config changes. */
    public static class ConfigChangeHandlingActivity extends CommandSession.BasicTestActivity {
    }
}
