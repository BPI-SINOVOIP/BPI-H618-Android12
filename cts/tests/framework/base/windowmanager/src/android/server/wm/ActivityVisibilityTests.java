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

import static android.app.WindowConfiguration.ACTIVITY_TYPE_STANDARD;
import static android.app.WindowConfiguration.WINDOWING_MODE_FREEFORM;
import static android.app.WindowConfiguration.WINDOWING_MODE_FULLSCREEN;
import static android.content.Intent.FLAG_ACTIVITY_NEW_TASK;
import static android.content.Intent.FLAG_ACTIVITY_TASK_ON_HOME;
import static android.server.wm.CliIntentExtra.extraString;
import static android.server.wm.UiDeviceUtils.pressBackButton;
import static android.server.wm.UiDeviceUtils.pressHomeButton;
import static android.server.wm.UiDeviceUtils.pressSleepButton;
import static android.server.wm.VirtualDisplayHelper.waitForDefaultDisplayState;
import static android.server.wm.WindowManagerState.STATE_RESUMED;
import static android.server.wm.WindowManagerState.STATE_STOPPED;
import static android.server.wm.app.Components.ALT_LAUNCHING_ACTIVITY;
import static android.server.wm.app.Components.BROADCAST_RECEIVER_ACTIVITY;
import static android.server.wm.app.Components.DOCKED_ACTIVITY;
import static android.server.wm.app.Components.LAUNCHING_ACTIVITY;
import static android.server.wm.app.Components.MOVE_TASK_TO_BACK_ACTIVITY;
import static android.server.wm.app.Components.MoveTaskToBackActivity.EXTRA_FINISH_POINT;
import static android.server.wm.app.Components.MoveTaskToBackActivity.FINISH_POINT_ON_PAUSE;
import static android.server.wm.app.Components.MoveTaskToBackActivity.FINISH_POINT_ON_STOP;
import static android.server.wm.app.Components.NO_HISTORY_ACTIVITY;
import static android.server.wm.app.Components.RESIZEABLE_ACTIVITY;
import static android.server.wm.app.Components.SHOW_WHEN_LOCKED_DIALOG_ACTIVITY;
import static android.server.wm.app.Components.TEST_ACTIVITY;
import static android.server.wm.app.Components.TOP_ACTIVITY;
import static android.server.wm.app.Components.TRANSLUCENT_ACTIVITY;
import static android.server.wm.app.Components.TRANSLUCENT_TEST_ACTIVITY;
import static android.server.wm.app.Components.TRANSLUCENT_TOP_ACTIVITY;
import static android.server.wm.app.Components.TURN_SCREEN_ON_ACTIVITY;
import static android.server.wm.app.Components.TURN_SCREEN_ON_ATTR_ACTIVITY;
import static android.server.wm.app.Components.TURN_SCREEN_ON_ATTR_REMOVE_ATTR_ACTIVITY;
import static android.server.wm.app.Components.TURN_SCREEN_ON_SHOW_ON_LOCK_ACTIVITY;
import static android.server.wm.app.Components.TURN_SCREEN_ON_SINGLE_TASK_ACTIVITY;
import static android.server.wm.app.Components.TURN_SCREEN_ON_WITH_RELAYOUT_ACTIVITY;
import static android.server.wm.app.Components.TopActivity.ACTION_CONVERT_FROM_TRANSLUCENT;
import static android.server.wm.app.Components.TopActivity.ACTION_CONVERT_TO_TRANSLUCENT;
import static android.view.Display.DEFAULT_DISPLAY;

import static org.junit.Assert.assertFalse;
import static org.junit.Assert.assertTrue;
import static org.junit.Assume.assumeTrue;

import android.content.ComponentName;
import android.platform.test.annotations.Presubmit;
import android.server.wm.CommandSession.ActivitySession;
import android.server.wm.CommandSession.ActivitySessionClient;
import android.server.wm.app.Components;

import org.junit.Rule;
import org.junit.Test;

/**
 * Build/Install/Run:
 *     atest CtsWindowManagerDeviceTestCases:ActivityVisibilityTests
 */
@Presubmit
@android.server.wm.annotation.Group2
public class ActivityVisibilityTests extends ActivityManagerTestBase {

    @Rule
    public final DisableScreenDozeRule mDisableScreenDozeRule = new DisableScreenDozeRule();

    /**
     * Asserts that the home activity is visible when a translucent activity is launched in the
     * fullscreen stack over the home activity.
     */
    @Test
    public void testTranslucentActivityOnTopOfHome() {
        if (!hasHomeScreen()) {
            return;
        }

        launchHomeActivity();
        launchActivity(TRANSLUCENT_ACTIVITY, WINDOWING_MODE_FULLSCREEN);

        mWmState.assertFrontStack("Fullscreen stack must be the front stack.",
                WINDOWING_MODE_FULLSCREEN, ACTIVITY_TYPE_STANDARD);
        mWmState.assertVisibility(TRANSLUCENT_ACTIVITY, true);
        mWmState.assertHomeActivityVisible(true);
    }

    @Test
    public void testTranslucentActivityOverMultiWindowActivity() {
        if (!supportsMultiWindow()) {
            // Skipping test: no multi-window support
            return;
        }

        launchActivitiesInSplitScreen(
                getLaunchActivityBuilder().setTargetActivity(DOCKED_ACTIVITY),
                getLaunchActivityBuilder().setTargetActivity(TEST_ACTIVITY));
        launchActivityInSecondarySplit(TRANSLUCENT_ACTIVITY);
        mWmState.computeState(
                new WaitForValidActivityState(TEST_ACTIVITY),
                new WaitForValidActivityState(DOCKED_ACTIVITY),
                new WaitForValidActivityState(TRANSLUCENT_ACTIVITY));
        mWmState.assertVisibility(DOCKED_ACTIVITY, true);
        mWmState.assertVisibility(TEST_ACTIVITY, true);
        mWmState.assertVisibility(TRANSLUCENT_ACTIVITY, true);
    }

    /**
     * Assert that the activity is visible when the intermediate activity finishes and a
     * translucent activity is on the top most.
     */
    @Test
    public void testVisibilityBehindTranslucentActivity_sameTask() {
        launchActivity(TEST_ACTIVITY);
        mWmState.waitForActivityState(TEST_ACTIVITY, STATE_RESUMED);

        launchAndFinishActivityBehindTranslucentActivity(true /* inSameTask */);

        mWmState.computeState(new WaitForValidActivityState(TEST_ACTIVITY));
        mWmState.assertVisibility(TEST_ACTIVITY, true);
    }

    @Test
    public void testVisibilityBehindTranslucentActivity_diffTask() {
        launchActivity(TEST_ACTIVITY);
        mWmState.waitForActivityState(TEST_ACTIVITY, STATE_RESUMED);

        launchAndFinishActivityBehindTranslucentActivity(false /* inSameTask */);

        mWmState.computeState(new WaitForValidActivityState(TEST_ACTIVITY));
        mWmState.assertVisibility(TEST_ACTIVITY, true);
    }

    /**
     * Assert that the home activity is visible when the intermediate activity finishes and a
     * translucent activity is on the top most.
     */
    @Test
    public void testHomeVisibilityBehindTranslucentActivity_sameTask() {
        if (!hasHomeScreen()) {
            return;
        }
        launchHomeActivity();

        launchAndFinishActivityBehindTranslucentActivity(true /* inSameTask */);

        mWmState.waitForHomeActivityVisible();
        mWmState.assertHomeActivityVisible(true);
    }

    @Test
    public void testHomeVisibilityBehindTranslucentActivity_diffTask() {
        if (!hasHomeScreen()) {
            return;
        }
        launchHomeActivity();

        launchAndFinishActivityBehindTranslucentActivity(false /* inSameTask */);

        mWmState.waitForHomeActivityVisible();
        mWmState.assertHomeActivityVisible(true);
    }

    private void launchAndFinishActivityBehindTranslucentActivity(boolean inSameTask) {
        // Launch first activity
        launchActivity(BROADCAST_RECEIVER_ACTIVITY);
        mWmState.waitForActivityState(BROADCAST_RECEIVER_ACTIVITY, STATE_RESUMED);

        // Launch translucent activity
        if (inSameTask) {
            launchActivity(TRANSLUCENT_TEST_ACTIVITY);
        } else {
            launchActivityInNewTask(TRANSLUCENT_TEST_ACTIVITY);
        }
        mWmState.waitForActivityState(TRANSLUCENT_TEST_ACTIVITY, STATE_RESUMED);
        mWmState.assertVisibility(TRANSLUCENT_TEST_ACTIVITY, true);

        // Finish first activity
        mBroadcastActionTrigger.finishBroadcastReceiverActivity();
        mWmState.computeState(BROADCAST_RECEIVER_ACTIVITY);
        mWmState.waitForActivityRemoved(BROADCAST_RECEIVER_ACTIVITY);
        mWmState.computeState(new WaitForValidActivityState(TRANSLUCENT_TEST_ACTIVITY));
    }

    @Test
    public void testTurnScreenOnActivity() {
        assumeTrue(supportsLockScreen());

        final LockScreenSession lockScreenSession = createManagedLockScreenSession();
        final boolean notSupportsInsecureLock = !supportsInsecureLock();
        if (notSupportsInsecureLock) {
            lockScreenSession.setLockCredential();
        }
        final ActivitySessionClient activityClient = createManagedActivityClientSession();
        testTurnScreenOnActivity(lockScreenSession, activityClient,
                true /* useWindowFlags */, true /* showWhenLocked */);
        testTurnScreenOnActivity(lockScreenSession, activityClient,
                false /* useWindowFlags */, true /* showWhenLocked */);
        if (notSupportsInsecureLock) {
            // In the platform without InsecureLock, we just test if the display is on with
            // TurnScreenOnActivity.
            mObjectTracker.close(lockScreenSession);
        }
        testTurnScreenOnActivity(lockScreenSession, activityClient,
                true /* useWindowFlags */, false /* showWhenLocked */);
        testTurnScreenOnActivity(lockScreenSession, activityClient,
                false /* useWindowFlags */, false /* showWhenLocked */);
    }

    @Test
    public void testTurnScreenOnActivity_slowLaunch() {
        assumeTrue(supportsLockScreen());

        final LockScreenSession lockScreenSession = createManagedLockScreenSession();
        final ActivitySessionClient activityClient = createManagedActivityClientSession();
        // The activity will be paused first because the flags turn-screen-on and show-when-locked
        // haven't been applied from relayout. And if it is slow, the ensure-visibility from pause
        // timeout should still notify the client activity to be visible. Then the relayout can
        // send the visible request to apply the flags and turn on screen.
        testTurnScreenOnActivity(lockScreenSession, activityClient, true /* useWindowFlags */,
                true /* showWhenLocked */, 1000 /* sleepMsInOnCreate */);
    }

    private void testTurnScreenOnActivity(LockScreenSession lockScreenSession,
            ActivitySessionClient activitySessionClient, boolean useWindowFlags,
            boolean showWhenLocked) {
        testTurnScreenOnActivity(lockScreenSession, activitySessionClient, useWindowFlags,
                showWhenLocked, 0 /* sleepMsInOnCreate */);
    }

    private void testTurnScreenOnActivity(LockScreenSession lockScreenSession,
            ActivitySessionClient activitySessionClient, boolean useWindowFlags,
            boolean showWhenLocked, int sleepMsInOnCreate) {
        ActivitySession activity = sleepDeviceAndLaunchTurnScreenOnActivity(lockScreenSession,
                activitySessionClient, useWindowFlags, showWhenLocked, sleepMsInOnCreate,
                WINDOWING_MODE_FULLSCREEN);

        mWmState.assertVisibility(TURN_SCREEN_ON_ACTIVITY, true);
        assertTrue("Display turns on by " + (useWindowFlags ? "flags" : "APIs"),
                isDisplayOn(DEFAULT_DISPLAY));

        activity.finish();
    }

    @Test
    public void testFreeformWindowToTurnScreenOn() {
        assumeTrue(supportsLockScreen());
        assumeTrue(supportsFreeform());

        final LockScreenSession lockScreenSession = createManagedLockScreenSession();
        final ActivitySessionClient activityClient = createManagedActivityClientSession();

        testFreeformWindowTurnScreenOnActivity(lockScreenSession, activityClient,
                true/* useWindowFlags */, true/* showWhenLocked */);
        testFreeformWindowTurnScreenOnActivity(lockScreenSession, activityClient,
                true/* useWindowFlags */, false/* showWhenLocked */);
        testFreeformWindowTurnScreenOnActivity(lockScreenSession, activityClient,
                false/* useWindowFlags */, true/* showWhenLocked */);
        testFreeformWindowTurnScreenOnActivity(lockScreenSession, activityClient,
                false/* useWindowFlags */, false/* showWhenLocked */);
    }

    private void testFreeformWindowTurnScreenOnActivity(LockScreenSession lockScreenSession,
            ActivitySessionClient activityClient, boolean useWindowFlags,
            boolean showWhenLocked) {
        ActivitySession activity = sleepDeviceAndLaunchTurnScreenOnActivity(lockScreenSession,
                activityClient, useWindowFlags, showWhenLocked,
                0 /* sleepMsInOnCreate */, WINDOWING_MODE_FREEFORM);
        mWmState.waitForValidState(
                new WaitForValidActivityState.Builder(TURN_SCREEN_ON_ACTIVITY)
                        .setWindowingMode(WINDOWING_MODE_FULLSCREEN)
                        .build());
        assertTrue(mWmState.containsActivityInWindowingMode(
                TURN_SCREEN_ON_ACTIVITY, WINDOWING_MODE_FULLSCREEN));
        mWmState.assertVisibility(TURN_SCREEN_ON_ACTIVITY, true);
        assertTrue("Display should be turned on by flags.", isDisplayOn(DEFAULT_DISPLAY));
        activity.finish();
    }

    private ActivitySession sleepDeviceAndLaunchTurnScreenOnActivity(
            LockScreenSession lockScreenSession, ActivitySessionClient activitySessionClient,
            boolean useWindowFlags, boolean showWhenLocked, int sleepMsInOnCreate,
            int windowingMode) {
        lockScreenSession.sleepDevice();

        return activitySessionClient.startActivity(
                getLaunchActivityBuilder().setUseInstrumentation().setIntentExtra(extra -> {
                    extra.putBoolean(Components.TurnScreenOnActivity.EXTRA_USE_WINDOW_FLAGS,
                            useWindowFlags);
                    extra.putBoolean(Components.TurnScreenOnActivity.EXTRA_SHOW_WHEN_LOCKED,
                            showWhenLocked);
                    extra.putLong(Components.TurnScreenOnActivity.EXTRA_SLEEP_MS_IN_ON_CREATE,
                            sleepMsInOnCreate);
                }).setTargetActivity(TURN_SCREEN_ON_ACTIVITY).setWindowingMode(windowingMode));
    }

    @Test
    public void testFinishActivityInNonFocusedStack() {
        if (!supportsMultiWindow()) {
            // Skipping test: no multi-window support
            return;
        }

        // Launch two activities in docked stack.
        launchActivityInPrimarySplit(LAUNCHING_ACTIVITY);
        getLaunchActivityBuilder()
                .setTargetActivity(BROADCAST_RECEIVER_ACTIVITY)
                .setWaitForLaunched(true)
                .setUseInstrumentation()
                .execute();
        mWmState.assertVisibility(BROADCAST_RECEIVER_ACTIVITY, true);
        // Launch something to second split to make it focused.
        launchActivityInSecondarySplit(TEST_ACTIVITY);
        // Finish activity in non-focused (docked) stack.
        mBroadcastActionTrigger.finishBroadcastReceiverActivity();

        mWmState.computeState(LAUNCHING_ACTIVITY);
        // The testing activities support multiple resume (target SDK >= Q).
        mWmState.waitForActivityState(LAUNCHING_ACTIVITY, STATE_RESUMED);
        mWmState.assertVisibility(LAUNCHING_ACTIVITY, true);
        mWmState.waitAndAssertActivityRemoved(BROADCAST_RECEIVER_ACTIVITY);
    }

    @Test
    public void testLaunchTaskOnHome() {
        if (!hasHomeScreen()) {
            return;
        }

        getLaunchActivityBuilder().setTargetActivity(BROADCAST_RECEIVER_ACTIVITY)
                .setIntentFlags(FLAG_ACTIVITY_NEW_TASK).execute();

        getLaunchActivityBuilder().setTargetActivity(BROADCAST_RECEIVER_ACTIVITY)
                .setIntentFlags(FLAG_ACTIVITY_NEW_TASK | FLAG_ACTIVITY_TASK_ON_HOME).execute();

        mBroadcastActionTrigger.finishBroadcastReceiverActivity();
        mWmState.waitForHomeActivityVisible();
        mWmState.assertHomeActivityVisible(true);
    }

    @Test
    public void testFinishActivityWithMoveTaskToBackAfterPause() {
        performFinishActivityWithMoveTaskToBack(FINISH_POINT_ON_PAUSE);
    }

    @Test
    public void testFinishActivityWithMoveTaskToBackAfterStop() {
        performFinishActivityWithMoveTaskToBack(FINISH_POINT_ON_STOP);
    }

    private void performFinishActivityWithMoveTaskToBack(String finishPoint) {
        // Make sure home activity is visible.
        launchHomeActivity();
        if (hasHomeScreen()) {
            mWmState.assertHomeActivityVisible(true /* visible */);
        }

        // Launch an activity that calls "moveTaskToBack" to finish itself.
        launchActivity(MOVE_TASK_TO_BACK_ACTIVITY, extraString(EXTRA_FINISH_POINT, finishPoint));
        mWmState.assertVisibility(MOVE_TASK_TO_BACK_ACTIVITY, true);

        // Launch a different activity on top.
        launchActivity(BROADCAST_RECEIVER_ACTIVITY, WINDOWING_MODE_FULLSCREEN);
        mWmState.waitForActivityState(BROADCAST_RECEIVER_ACTIVITY, STATE_RESUMED);
        mWmState.waitForActivityState(MOVE_TASK_TO_BACK_ACTIVITY,STATE_STOPPED);
        final boolean shouldBeVisible =
                !mWmState.isBehindOpaqueActivities(MOVE_TASK_TO_BACK_ACTIVITY);
        mWmState.assertVisibility(MOVE_TASK_TO_BACK_ACTIVITY, shouldBeVisible);
        mWmState.assertVisibility(BROADCAST_RECEIVER_ACTIVITY, true);

        // Finish the top-most activity.
        mBroadcastActionTrigger.finishBroadcastReceiverActivity();
        //TODO: BUG: MoveTaskToBackActivity returns to the top of the stack when
        // BroadcastActivity finishes, so homeActivity is not visible afterwards

        // Home must be visible.
        if (hasHomeScreen()) {
            mWmState.waitForHomeActivityVisible();
            mWmState.assertHomeActivityVisible(true /* visible */);
        }
    }

    /**
     * Asserts that launching between reorder to front activities exhibits the correct backstack
     * behavior.
     */
    @Test
    public void testReorderToFrontBackstack() {
        // Start with home on top
        launchHomeActivity();
        if (hasHomeScreen()) {
            mWmState.assertHomeActivityVisible(true /* visible */);
        }

        // Launch the launching activity to the foreground
        launchActivity(LAUNCHING_ACTIVITY);

        // Launch the alternate launching activity from launching activity with reorder to front.
        getLaunchActivityBuilder().setTargetActivity(ALT_LAUNCHING_ACTIVITY)
                .setReorderToFront(true).execute();

        // Launch the launching activity from the alternate launching activity with reorder to
        // front.
        getLaunchActivityBuilder().setTargetActivity(LAUNCHING_ACTIVITY)
                .setLaunchingActivity(ALT_LAUNCHING_ACTIVITY)
                .setReorderToFront(true)
                .execute();

        // Press back
        pressBackButton();

        mWmState.waitForValidState(ALT_LAUNCHING_ACTIVITY);

        // Ensure the alternate launching activity is in focus
        mWmState.assertFocusedActivity("Alt Launching Activity must be focused",
                ALT_LAUNCHING_ACTIVITY);
    }

    /**
     * Asserts that the activity focus and history is preserved moving between the activity and
     * home stack.
     */
    @Test
    public void testReorderToFrontChangingStack() {
        // Start with home on top
        launchHomeActivity();
        if (hasHomeScreen()) {
            mWmState.assertHomeActivityVisible(true /* visible */);
        }

        // Launch the launching activity to the foreground
        launchActivity(LAUNCHING_ACTIVITY);

        // Launch the alternate launching activity from launching activity with reorder to front.
        getLaunchActivityBuilder().setTargetActivity(ALT_LAUNCHING_ACTIVITY)
                .setReorderToFront(true)
                .execute();

        // Return home
        launchHomeActivity();
        if (hasHomeScreen()) {
            mWmState.assertHomeActivityVisible(true /* visible */);
        }
        // Launch the launching activity from the alternate launching activity with reorder to
        // front.

        // Bring launching activity back to the foreground
        launchActivityNoWait(LAUNCHING_ACTIVITY);
        // Wait for the most front activity of the task.
        mWmState.waitForValidState(ALT_LAUNCHING_ACTIVITY);

        // Ensure the alternate launching activity is still in focus.
        mWmState.assertFocusedActivity("Alt Launching Activity must be focused",
                ALT_LAUNCHING_ACTIVITY);

        pressBackButton();

        // Wait for the bottom activity back to the foreground.
        mWmState.waitForValidState(LAUNCHING_ACTIVITY);

        // Ensure launching activity was brought forward.
        mWmState.assertFocusedActivity("Launching Activity must be focused",
                LAUNCHING_ACTIVITY);
    }

    /**
     * Asserts that a nohistory activity is stopped and removed immediately after a resumed activity
     * above becomes visible and does not idle.
     */
    @Test
    public void testNoHistoryActivityFinishedResumedActivityNotIdle() {
        if (!hasHomeScreen()) {
            return;
        }

        // Start with home on top
        launchHomeActivity();

        // Launch no history activity
        launchActivity(NO_HISTORY_ACTIVITY);

        // Launch an activity that won't report idle.
        launchNoIdleActivity();

        pressBackButton();
        mWmState.waitForHomeActivityVisible();
        mWmState.assertHomeActivityVisible(true);
    }

    /**
     * Asserts that a no-history activity is not stopped and removed after a translucent activity
     * above becomes resumed.
     */
    @Test
    public void testNoHistoryActivityNotFinishedBehindTranslucentActivity() {
        // Launch a no-history activity
        launchActivity(NO_HISTORY_ACTIVITY);

        // Launch a translucent activity
        launchActivity(TRANSLUCENT_ACTIVITY);

        // Wait for the activity resumed
        mWmState.waitForActivityState(TRANSLUCENT_ACTIVITY, STATE_RESUMED);
        mWmState.assertVisibility(NO_HISTORY_ACTIVITY, true);

        pressBackButton();

        // Wait for the activity resumed
        mWmState.waitForActivityState(NO_HISTORY_ACTIVITY, STATE_RESUMED);
        mWmState.assertVisibility(NO_HISTORY_ACTIVITY, true);
    }

    /**
     *  If the next activity hasn't reported idle but it has drawn and the transition has done, the
     *  previous activity should be stopped and invisible without waiting for idle timeout.
     */
    @Test
    public void testActivityStoppedWhileNextActivityNotIdle() {
        final ComponentName activityWithSameAffinity = TURN_SCREEN_ON_ATTR_ACTIVITY;
        launchActivity(activityWithSameAffinity);
        launchNoIdleActivity();
        waitAndAssertActivityState(activityWithSameAffinity, STATE_STOPPED,
                "Activity should be stopped before idle timeout");
        mWmState.assertVisibility(activityWithSameAffinity, false);
    }

    private void launchNoIdleActivity() {
        getLaunchActivityBuilder()
                .setUseInstrumentation()
                .setIntentExtra(
                        extra -> extra.putBoolean(Components.TestActivity.EXTRA_NO_IDLE, true))
                .setTargetActivity(TEST_ACTIVITY)
                .setWindowingMode(WINDOWING_MODE_FULLSCREEN)
                .execute();
    }

    @Test
    public void testTurnScreenOnAttrNoLockScreen() {
        assumeTrue(supportsLockScreen());

        final LockScreenSession lockScreenSession = createManagedLockScreenSession();
        lockScreenSession.disableLockScreen().sleepDevice();
        separateTestJournal();
        launchActivity(TURN_SCREEN_ON_ATTR_ACTIVITY, WINDOWING_MODE_FULLSCREEN);
        mWmState.assertVisibility(TURN_SCREEN_ON_ATTR_ACTIVITY, true);
        assertTrue("Display turns on", isDisplayOn(DEFAULT_DISPLAY));
        assertSingleLaunch(TURN_SCREEN_ON_ATTR_ACTIVITY);
    }

    @Test
    public void testTurnScreenOnAttrNoLockScreen_SplitScreen() {
        assumeTrue(supportsLockScreen());
        assumeTrue(supportsMultiWindow());

        launchActivitiesInSplitScreen(
                getLaunchActivityBuilder().setTargetActivity(LAUNCHING_ACTIVITY),
                getLaunchActivityBuilder().setTargetActivity(RESIZEABLE_ACTIVITY));

        final LockScreenSession lockScreenSession = createManagedLockScreenSession();
        lockScreenSession.disableLockScreen().sleepDevice();
        launchActivity(TURN_SCREEN_ON_ATTR_ACTIVITY, WINDOWING_MODE_FULLSCREEN);
        mWmState.assertVisibility(TURN_SCREEN_ON_ATTR_ACTIVITY, true);
        assertTrue("Display turns on", isDisplayOn(DEFAULT_DISPLAY));
    }

    @Test
    public void testTurnScreenOnWithAttr_Freeform() {
        assumeTrue(supportsLockScreen());
        assumeTrue(supportsFreeform());

        final LockScreenSession lockScreenSession = createManagedLockScreenSession();
        lockScreenSession.disableLockScreen().sleepDevice();

        launchActivity(TURN_SCREEN_ON_ATTR_ACTIVITY, WINDOWING_MODE_FREEFORM);
        mWmState.assertVisibility(TURN_SCREEN_ON_ATTR_ACTIVITY, true);
        assertTrue("Display turns on", isDisplayOn(DEFAULT_DISPLAY));
    }

    @Test
    public void testTurnScreenOnAttrWithLockScreen() {
        assumeTrue(supportsSecureLock());

        final LockScreenSession lockScreenSession = createManagedLockScreenSession();
        lockScreenSession.setLockCredential().sleepDevice();
        separateTestJournal();
        launchActivityNoWait(TURN_SCREEN_ON_ATTR_ACTIVITY, WINDOWING_MODE_FULLSCREEN);
        // Wait for the activity stopped because lock screen prevent showing the activity.
        mWmState.waitForActivityState(TURN_SCREEN_ON_ATTR_ACTIVITY, STATE_STOPPED);
        assertFalse("Display keeps off", isDisplayOn(DEFAULT_DISPLAY));
        assertSingleLaunchAndStop(TURN_SCREEN_ON_ATTR_ACTIVITY);
    }

    @Test
    public void testTurnScreenOnShowOnLockAttr() {
        assumeTrue(supportsLockScreen());

        final LockScreenSession lockScreenSession = createManagedLockScreenSession();
        lockScreenSession.sleepDevice();
        mWmState.waitForAllStoppedActivities();
        separateTestJournal();
        launchActivity(TURN_SCREEN_ON_SHOW_ON_LOCK_ACTIVITY, WINDOWING_MODE_FULLSCREEN);
        mWmState.assertVisibility(TURN_SCREEN_ON_SHOW_ON_LOCK_ACTIVITY, true);
        assertTrue("Display turns on", isDisplayOn(DEFAULT_DISPLAY));
        assertSingleLaunch(TURN_SCREEN_ON_SHOW_ON_LOCK_ACTIVITY);
    }

    @Test
    public void testChangeToFullscreenWhenLockWithAttrInFreeform() {
        assumeTrue(supportsLockScreen());
        assumeTrue(supportsFreeform());

        final LockScreenSession lockScreenSession = createManagedLockScreenSession();
        lockScreenSession.sleepDevice();
        mWmState.waitForAllStoppedActivities();

        launchActivityNoWait(TURN_SCREEN_ON_SHOW_ON_LOCK_ACTIVITY, WINDOWING_MODE_FREEFORM);
        mWmState.waitForValidState(
                new WaitForValidActivityState.Builder(TURN_SCREEN_ON_SHOW_ON_LOCK_ACTIVITY)
                        .setWindowingMode(WINDOWING_MODE_FULLSCREEN)
                        .build());
        assertTrue(mWmState.containsActivityInWindowingMode(
                TURN_SCREEN_ON_SHOW_ON_LOCK_ACTIVITY, WINDOWING_MODE_FULLSCREEN));
        mWmState.assertVisibility(TURN_SCREEN_ON_SHOW_ON_LOCK_ACTIVITY, true);
        assertTrue("Display turns on", isDisplayOn(DEFAULT_DISPLAY));
    }

    @Test
    public void testTurnScreenOnAttrRemove() {
        assumeTrue(supportsLockScreen());

        final LockScreenSession lockScreenSession = createManagedLockScreenSession();
        lockScreenSession.sleepDevice();
        mWmState.waitForAllStoppedActivities();
        separateTestJournal();
        launchActivity(TURN_SCREEN_ON_ATTR_REMOVE_ATTR_ACTIVITY);
        assertTrue("Display turns on", isDisplayOn(DEFAULT_DISPLAY));
        assertSingleLaunch(TURN_SCREEN_ON_ATTR_REMOVE_ATTR_ACTIVITY);

        lockScreenSession.sleepDevice();
        mWmState.waitForAllStoppedActivities();
        separateTestJournal();
        launchActivity(TURN_SCREEN_ON_ATTR_REMOVE_ATTR_ACTIVITY);
        mWmState.waitForActivityState(TURN_SCREEN_ON_ATTR_REMOVE_ATTR_ACTIVITY, STATE_STOPPED);
        // Display should keep off, because setTurnScreenOn(false) has been called at
        // {@link TURN_SCREEN_ON_ATTR_REMOVE_ATTR_ACTIVITY}'s onStop.
        assertFalse("Display keeps off", isDisplayOn(DEFAULT_DISPLAY));
        assertSingleStartAndStop(TURN_SCREEN_ON_ATTR_REMOVE_ATTR_ACTIVITY);
    }

    @Test
    public void testTurnScreenOnSingleTask() {
        assumeTrue(supportsLockScreen());

        final LockScreenSession lockScreenSession = createManagedLockScreenSession();
        lockScreenSession.sleepDevice();
        separateTestJournal();
        launchActivity(TURN_SCREEN_ON_SINGLE_TASK_ACTIVITY, WINDOWING_MODE_FULLSCREEN);
        mWmState.assertVisibility(TURN_SCREEN_ON_SINGLE_TASK_ACTIVITY, true);
        assertTrue("Display turns on", isDisplayOn(DEFAULT_DISPLAY));
        assertSingleLaunch(TURN_SCREEN_ON_SINGLE_TASK_ACTIVITY);

        lockScreenSession.sleepDevice();
        // We should make sure test activity stopped to prevent a false alarm stop state
        // included in the lifecycle count.
        waitAndAssertActivityState(TURN_SCREEN_ON_SINGLE_TASK_ACTIVITY, STATE_STOPPED,
                "Activity should be stopped");
        separateTestJournal();
        launchActivity(TURN_SCREEN_ON_SINGLE_TASK_ACTIVITY);
        mWmState.assertVisibility(TURN_SCREEN_ON_SINGLE_TASK_ACTIVITY, true);
        // Wait more for display state change since turning the display ON may take longer
        // and reported after the activity launch.
        waitForDefaultDisplayState(true /* wantOn */);
        assertTrue("Display turns on", isDisplayOn(DEFAULT_DISPLAY));
        assertSingleStart(TURN_SCREEN_ON_SINGLE_TASK_ACTIVITY);
    }

    @Test
    public void testTurnScreenOnActivity_withRelayout() {
        assumeTrue(supportsLockScreen());

        final LockScreenSession lockScreenSession = createManagedLockScreenSession();
        lockScreenSession.sleepDevice();
        launchActivity(TURN_SCREEN_ON_WITH_RELAYOUT_ACTIVITY);
        mWmState.assertVisibility(TURN_SCREEN_ON_WITH_RELAYOUT_ACTIVITY, true);

        lockScreenSession.sleepDevice();
        waitAndAssertActivityState(TURN_SCREEN_ON_WITH_RELAYOUT_ACTIVITY, STATE_STOPPED,
                "Activity should be stopped");
        assertFalse("Display keeps off", isDisplayOn(DEFAULT_DISPLAY));
    }

    @Test
    public void testGoingHomeMultipleTimes() {
        for (int i = 0; i < 10; i++) {
            // Start activity normally
            launchActivityOnDisplay(TEST_ACTIVITY, DEFAULT_DISPLAY);
            waitAndAssertTopResumedActivity(TEST_ACTIVITY, DEFAULT_DISPLAY,
                    "Activity launched on default display must be focused");

            // Start home activity directly
            launchHomeActivity();

            mWmState.assertHomeActivityVisible(true);
            waitAndAssertActivityState(TEST_ACTIVITY, STATE_STOPPED,
                    "Activity should become STOPPED");
            mWmState.assertVisibility(TEST_ACTIVITY, false);
        }
    }

    @Test
    public void testPressingHomeButtonMultipleTimes() {
        for (int i = 0; i < 10; i++) {
            // Start activity normally
            launchActivityOnDisplay(TEST_ACTIVITY, DEFAULT_DISPLAY);
            waitAndAssertTopResumedActivity(TEST_ACTIVITY, DEFAULT_DISPLAY,
                    "Activity launched on default display must be focused");

            // Press home button
            pressHomeButton();

            // Wait and assert home and activity states
            mWmState.waitForHomeActivityVisible();
            mWmState.assertHomeActivityVisible(true);
            waitAndAssertActivityState(TEST_ACTIVITY, STATE_STOPPED,
                    "Activity should become STOPPED");
            mWmState.assertVisibility(TEST_ACTIVITY, false);
        }
    }

    @Test
    public void testPressingHomeButtonMultipleTimesQuick() {
        for (int i = 0; i < 10; i++) {
            // Start activity normally
            launchActivityOnDisplay(TEST_ACTIVITY, DEFAULT_DISPLAY);

            // Press home button
            pressHomeButton();
            mWmState.waitForHomeActivityVisible();
            mWmState.assertHomeActivityVisible(true);
        }
        waitAndAssertActivityState(TEST_ACTIVITY, STATE_STOPPED,
                "Activity should become STOPPED");
        mWmState.assertVisibility(TEST_ACTIVITY, false);
    }

    @Test
    public void testConvertTranslucentOnTranslucentActivity() {
        final ActivitySessionClient activityClient = createManagedActivityClientSession();
        // Start CONVERT_TRANSLUCENT_DIALOG_ACTIVITY on top of LAUNCHING_ACTIVITY
        final ActivitySession activity = activityClient.startActivity(
                getLaunchActivityBuilder().setTargetActivity(TRANSLUCENT_TOP_ACTIVITY));
        verifyActivityVisibilities(TRANSLUCENT_TOP_ACTIVITY, false);
        verifyActivityVisibilities(LAUNCHING_ACTIVITY, false);

        activity.sendCommand(ACTION_CONVERT_FROM_TRANSLUCENT);
        verifyActivityVisibilities(LAUNCHING_ACTIVITY, true);

        activity.sendCommand(ACTION_CONVERT_TO_TRANSLUCENT);
        verifyActivityVisibilities(LAUNCHING_ACTIVITY, false);
    }

    @Test
    public void testConvertTranslucentOnNonTopTranslucentActivity() {
        final ActivitySessionClient activityClient = createManagedActivityClientSession();
        final ActivitySession activity = activityClient.startActivity(
                getLaunchActivityBuilder().setTargetActivity(TRANSLUCENT_TOP_ACTIVITY));
        getLaunchActivityBuilder().setTargetActivity(SHOW_WHEN_LOCKED_DIALOG_ACTIVITY)
                .setUseInstrumentation().execute();
        verifyActivityVisibilities(SHOW_WHEN_LOCKED_DIALOG_ACTIVITY, false);
        verifyActivityVisibilities(TRANSLUCENT_TOP_ACTIVITY, false);
        verifyActivityVisibilities(LAUNCHING_ACTIVITY, false);

        activity.sendCommand(ACTION_CONVERT_FROM_TRANSLUCENT);
        verifyActivityVisibilities(LAUNCHING_ACTIVITY, true);

        activity.sendCommand(ACTION_CONVERT_TO_TRANSLUCENT);
        verifyActivityVisibilities(LAUNCHING_ACTIVITY, false);
    }

    @Test
    public void testConvertTranslucentOnOpaqueActivity() {
        final ActivitySessionClient activityClient = createManagedActivityClientSession();
        final ActivitySession activity = activityClient.startActivity(
                getLaunchActivityBuilder().setTargetActivity(TOP_ACTIVITY));
        verifyActivityVisibilities(TOP_ACTIVITY, false);
        verifyActivityVisibilities(LAUNCHING_ACTIVITY, true);

        activity.sendCommand(ACTION_CONVERT_TO_TRANSLUCENT);
        verifyActivityVisibilities(LAUNCHING_ACTIVITY, false);

        activity.sendCommand(ACTION_CONVERT_FROM_TRANSLUCENT);
        verifyActivityVisibilities(LAUNCHING_ACTIVITY, true);
    }

    @Test
    public void testConvertTranslucentOnNonTopOpaqueActivity() {
        final ActivitySessionClient activityClient = createManagedActivityClientSession();
        final ActivitySession activity = activityClient.startActivity(
                getLaunchActivityBuilder().setTargetActivity(TOP_ACTIVITY));
        getLaunchActivityBuilder().setTargetActivity(SHOW_WHEN_LOCKED_DIALOG_ACTIVITY)
                .setUseInstrumentation().execute();
        verifyActivityVisibilities(SHOW_WHEN_LOCKED_DIALOG_ACTIVITY, false);
        verifyActivityVisibilities(TOP_ACTIVITY, false);
        verifyActivityVisibilities(LAUNCHING_ACTIVITY, true);

        activity.sendCommand(ACTION_CONVERT_TO_TRANSLUCENT);
        verifyActivityVisibilities(LAUNCHING_ACTIVITY, false);

        activity.sendCommand(ACTION_CONVERT_FROM_TRANSLUCENT);
        verifyActivityVisibilities(LAUNCHING_ACTIVITY, true);
    }

    private void verifyActivityVisibilities(ComponentName activityBehind,
            boolean behindFullScreen) {
        if (behindFullScreen) {
            mWmState.waitForActivityState(activityBehind, STATE_STOPPED);
            mWmState.assertVisibility(activityBehind, false);
        } else {
            mWmState.waitForValidState(activityBehind);
            mWmState.assertVisibility(activityBehind, true);
        }
    }
}
