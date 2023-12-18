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
 * limitations under the License
 */

package android.server.wm.lifecycle;

import static android.app.WindowConfiguration.WINDOWING_MODE_FULLSCREEN;
import static android.content.Intent.FLAG_ACTIVITY_MULTIPLE_TASK;
import static android.content.Intent.FLAG_ACTIVITY_NEW_TASK;
import static android.server.wm.lifecycle.LifecycleLog.ActivityCallback.ON_ACTIVITY_RESULT;
import static android.server.wm.lifecycle.LifecycleLog.ActivityCallback.ON_CREATE;
import static android.server.wm.lifecycle.LifecycleLog.ActivityCallback.ON_DESTROY;
import static android.server.wm.lifecycle.LifecycleLog.ActivityCallback.ON_MULTI_WINDOW_MODE_CHANGED;
import static android.server.wm.lifecycle.LifecycleLog.ActivityCallback.ON_PAUSE;
import static android.server.wm.lifecycle.LifecycleLog.ActivityCallback.ON_POST_CREATE;
import static android.server.wm.lifecycle.LifecycleLog.ActivityCallback.ON_RESUME;
import static android.server.wm.lifecycle.LifecycleLog.ActivityCallback.ON_START;
import static android.server.wm.lifecycle.LifecycleLog.ActivityCallback.ON_STOP;
import static android.server.wm.lifecycle.LifecycleLog.ActivityCallback.ON_TOP_POSITION_GAINED;
import static android.server.wm.lifecycle.LifecycleLog.ActivityCallback.ON_TOP_POSITION_LOST;
import static android.server.wm.lifecycle.LifecycleLog.ActivityCallback.ON_USER_LEAVE_HINT;
import static android.server.wm.lifecycle.LifecycleVerifier.transition;

import static androidx.test.InstrumentationRegistry.getInstrumentation;

import static org.junit.Assert.assertNotNull;
import static org.junit.Assume.assumeTrue;

import android.app.Activity;
import android.app.ActivityOptions;
import android.app.Instrumentation;
import android.app.WindowConfiguration;
import android.content.Intent;
import android.platform.test.annotations.Presubmit;

import androidx.test.filters.MediumTest;

import org.junit.Before;
import org.junit.Test;

import java.util.Arrays;
import java.util.List;

/**
 * Build/Install/Run:
 *     atest CtsWindowManagerDeviceTestCases:ActivityLifecycleLegacySplitScreenTests
 */
@MediumTest
@Presubmit
@android.server.wm.annotation.Group3
public class ActivityLifecycleLegacySplitScreenTests extends ActivityLifecycleClientTestBase {

    @Before
    public void setUp() throws Exception {
        super.setUp();
        assumeTrue(supportsSplitScreenMultiWindow());
    }

    @Test
    public void testResumedWhenRecreatedFromInNonFocusedTask() throws Exception {
        // Launch an activity that will be moved to split-screen secondary
        final Activity sideActivity = launchActivityAndWait(SideActivity.class);

        // Launch first activity
        final Activity firstActivity = launchActivityAndWait(FirstActivity.class);

        // Launch second activity to stop first
        final Activity secondActivity = launchActivityAndWait(SecondActivity.class);

        // Wait for the first activity to stop, so that this event is not included in the logs.
        waitAndAssertActivityStates(state(firstActivity, ON_STOP));

        // Enter split screen
        moveTaskToPrimarySplitScreenAndVerify(secondActivity, sideActivity);

        // CLear logs so we can capture just the destroy sequence
        getLifecycleLog().clear();

        // Start an activity in separate task (will be placed in secondary stack)
        mTaskOrganizer.setLaunchRoot(mTaskOrganizer.getSecondarySplitTaskId());
        new Launcher(ThirdActivity.class)
                .setFlags(FLAG_ACTIVITY_MULTIPLE_TASK | FLAG_ACTIVITY_NEW_TASK)
                .launch();

        // Finish top activity
        secondActivity.finish();

        waitAndAssertActivityStates(state(secondActivity, ON_DESTROY),
                state(firstActivity, ON_RESUME));

        // Verify that the first activity was recreated to resume as it was created before
        // windowing mode was switched
        LifecycleVerifier.assertRecreateAndResumeSequence(FirstActivity.class, getLifecycleLog());

        // Verify that the lifecycle state did not change for activity in non-focused stack
        LifecycleVerifier.assertLaunchSequence(ThirdActivity.class, getLifecycleLog());
    }

    @Test
    public void testOccludingOnSplitSecondaryTask() throws Exception {
        // Launch an activity that will be moved to split-screen secondary
        final Activity sideActivity = launchActivityAndWait(SideActivity.class);

        // Launch first activity
        final Activity firstActivity = launchActivityAndWait(FirstActivity.class);

        // Enter split screen
        moveTaskToPrimarySplitScreenAndVerify(firstActivity, sideActivity);

        // Launch third activity on top of second
        getLifecycleLog().clear();
        mTaskOrganizer.setLaunchRoot(mTaskOrganizer.getSecondarySplitTaskId());
        new Launcher(ThirdActivity.class)
                .setFlags(FLAG_ACTIVITY_NEW_TASK | FLAG_ACTIVITY_MULTIPLE_TASK)
                .launch();
        waitAndAssertActivityStates(state(sideActivity, ON_STOP));
    }

    @Test
    public void testTranslucentOnSplitSecondaryTask() throws Exception {
        // Launch an activity that will be moved to split-screen secondary
        final Activity sideActivity = launchActivityAndWait(ThirdActivity.class);

        ActivityOptions options = ActivityOptions.makeBasic();
        options.setLaunchWindowingMode(WindowConfiguration.WINDOWING_MODE_FULLSCREEN);

        // Launch an activity in a new task
        final Activity firstActivity = new Launcher(FirstActivity.class)
                .setFlags(FLAG_ACTIVITY_NEW_TASK | FLAG_ACTIVITY_MULTIPLE_TASK)
                .setOptions(options)
                .launch();

        // Enter split screen
        moveTaskToPrimarySplitScreenAndVerify(firstActivity, sideActivity);

        // Launch translucent activity on top of second
        getLifecycleLog().clear();

        mTaskOrganizer.setLaunchRoot(mTaskOrganizer.getSecondarySplitTaskId());
        new Launcher(TranslucentActivity.class)
                .setFlags(FLAG_ACTIVITY_NEW_TASK | FLAG_ACTIVITY_MULTIPLE_TASK)
                .launch();
        waitAndAssertActivityStates(state(sideActivity, ON_PAUSE));
    }

    @Test
    public void testResultInNonFocusedTask() throws Exception {
        // Launch an activity that will be moved to split-screen secondary
        final Activity sideActivity = launchActivityAndWait(SideActivity.class);

        // Launch first activity in another task
        final Activity callbackTrackingActivity = launchActivityAndWait(
                CallbackTrackingActivity.class);

        // Launch second activity
        // Create an ActivityMonitor that catch ChildActivity and return mock ActivityResult:
        Instrumentation.ActivityMonitor activityMonitor = getInstrumentation()
                .addMonitor(SecondActivity.class.getName(), null /* activityResult */,
                        false /* block */);

        callbackTrackingActivity.startActivityForResult(
                new Intent(callbackTrackingActivity, SecondActivity.class), 1 /* requestCode */);

        // Wait for the ActivityMonitor to be hit
        final Activity secondActivity = getInstrumentation()
                .waitForMonitorWithTimeout(activityMonitor, 5 * 1000);

        // Wait for second activity to resume
        assertNotNull("Second activity should be started", secondActivity);
        waitAndAssertActivityStates(state(secondActivity, ON_RESUME));

        // Verify if the first activity stopped (since it is not currently visible)
        waitAndAssertActivityStates(state(callbackTrackingActivity, ON_STOP));

        // Enter split screen
        moveTaskToPrimarySplitScreenAndVerify(secondActivity, sideActivity);

        // Finish top activity and verify that activity below became focused.
        getLifecycleLog().clear();
        secondActivity.setResult(Activity.RESULT_OK);
        secondActivity.finish();

        // Check that activity was restarted and result was delivered
        waitAndAssertActivityStates(state(callbackTrackingActivity, ON_RESUME));
        LifecycleVerifier.assertSequence(CallbackTrackingActivity.class, getLifecycleLog(),
                Arrays.asList(ON_DESTROY, ON_CREATE, ON_START, ON_POST_CREATE,
                        ON_ACTIVITY_RESULT, ON_RESUME), "restart");
    }

    @Test
    public void testResumedWhenRestartedFromInNonFocusedTask() throws Exception {
        // Launch first activity
        final Activity sideActivity = launchActivityAndWait(SideActivity.class);

        // Start an activity in separate task
        final Activity firstActivity = launchActivityAndWait(FirstActivity.class);

        // Enter split screen
        moveTaskToPrimarySplitScreenAndVerify(firstActivity, sideActivity);

        // Start an activity in separate task (will be placed in secondary stack)
        mTaskOrganizer.setLaunchRoot(mTaskOrganizer.getSecondarySplitTaskId());
        final Activity newTaskActivity = new Launcher(ThirdActivity.class)
                .setFlags(FLAG_ACTIVITY_NEW_TASK | FLAG_ACTIVITY_MULTIPLE_TASK)
                .launch();

        // Launch second activity, first become stopped
        getLifecycleLog().clear();
        final Activity secondActivity = launchActivityAndWait(SecondActivity.class);

        // Wait for second activity to resume and first to stop
        waitAndAssertActivityStates(state(newTaskActivity, ON_STOP));

        // Finish top activity
        getLifecycleLog().clear();
        secondActivity.finish();

        waitAndAssertActivityStates(state(newTaskActivity, ON_RESUME));
        waitAndAssertActivityStates(state(secondActivity, ON_DESTROY));

        // Verify that the first activity was restarted to resumed state as it was brought back
        // after windowing mode was switched
        LifecycleVerifier.assertRestartAndResumeSequence(ThirdActivity.class, getLifecycleLog());
        LifecycleVerifier.assertResumeToDestroySequence(SecondActivity.class, getLifecycleLog());
    }

    @Test
    public void testResumedTranslucentWhenRestartedFromInNonFocusedTask() throws Exception {
        // Launch an activity that will be moved to split-screen secondary
        final Activity sideActivity = launchActivityAndWait(SideActivity.class);

        // Launch first activity
        final Activity firstActivity = launchActivityAndWait(FirstActivity.class);

        // Launch a translucent activity, first become paused
        final Activity translucentActivity = launchActivityAndWait(TranslucentActivity.class);

        // Wait for first activity to pause
        waitAndAssertActivityStates(state(firstActivity, ON_PAUSE));

        // Enter split screen
        mWmState.computeState(firstActivity.getComponentName());
        moveActivitiesToSplitScreen(firstActivity.getComponentName(),
                sideActivity.getComponentName());

        // Finish top activity
        getLifecycleLog().clear();
        translucentActivity.finish();

        waitAndAssertActivityStates(state(firstActivity, ON_RESUME));
        waitAndAssertActivityStates(state(translucentActivity, ON_DESTROY));

        // Verify that the first activity was resumed
        LifecycleVerifier.assertSequence(FirstActivity.class, getLifecycleLog(),
                Arrays.asList(ON_RESUME), "resume");
        LifecycleVerifier.assertResumeToDestroySequence(TranslucentActivity.class,
                getLifecycleLog());
    }

    @Test
    public void testLifecycleOnMoveToFromSplitScreenRelaunch() throws Exception {
        // Launch an activity that will be moved to split-screen secondary
        final Activity sideActivity = launchActivityAndWait(SecondActivity.class);

        // Launch a singleTop activity
        final Activity firstActivity = new Launcher(CallbackTrackingActivity.class)
                .setFlags(FLAG_ACTIVITY_NEW_TASK | FLAG_ACTIVITY_MULTIPLE_TASK)
                .launch();

        // Wait for the activity to resume
        LifecycleVerifier.assertLaunchSequence(CallbackTrackingActivity.class, getLifecycleLog());

        // Enter split screen
        getLifecycleLog().clear();
        moveTaskToPrimarySplitScreenAndVerify(firstActivity, sideActivity);

        // Wait for the activity to relaunch and receive multi-window mode change
        final List<LifecycleLog.ActivityCallback> expectedEnterSequence =
                Arrays.asList(ON_TOP_POSITION_LOST, ON_PAUSE, ON_STOP, ON_DESTROY,
                        ON_CREATE, ON_START, ON_POST_CREATE, ON_RESUME, ON_TOP_POSITION_GAINED,
                        ON_TOP_POSITION_LOST, ON_PAUSE);
        waitForActivityTransitions(CallbackTrackingActivity.class, expectedEnterSequence);
        LifecycleVerifier.assertOrder(getLifecycleLog(), CallbackTrackingActivity.class,
                Arrays.asList(ON_TOP_POSITION_LOST, ON_PAUSE, ON_STOP, ON_DESTROY, ON_CREATE,
                        ON_RESUME), "moveToSplitScreen");

        // Exit split-screen
        getLifecycleLog().clear();
        dismissSplitScreen(true /* primaryOnTop */);

        // Wait for the activity to relaunch and receive multi-window mode change
        final List<LifecycleLog.ActivityCallback> expectedExitSequence =
                Arrays.asList(ON_STOP, ON_DESTROY, ON_CREATE, ON_START,
                        ON_POST_CREATE, ON_RESUME, ON_PAUSE, ON_RESUME, ON_TOP_POSITION_GAINED);
        waitForActivityTransitions(CallbackTrackingActivity.class, expectedExitSequence);
        LifecycleVerifier.assertOrder(getLifecycleLog(), CallbackTrackingActivity.class,
                Arrays.asList(ON_DESTROY, ON_CREATE, ON_RESUME, ON_TOP_POSITION_GAINED),
                "moveFromSplitScreen");
    }

    @Test
    public void testLifecycleOnMoveToFromSplitScreenNoRelaunch() throws Exception {

        // Launch activities and enter split screen. Launched an activity on
        // split-screen secondary stack to ensure the TOP_POSITION_LOST is send
        // prior to MULTI_WINDOW_MODE_CHANGED.
        launchActivitiesInSplitScreen(
                getLaunchActivityBuilder().
                        setTargetActivity(getComponentName(ConfigChangeHandlingActivity.class)),
                getLaunchActivityBuilder().
                        setTargetActivity(getComponentName(SecondActivity.class)));

        final int displayWindowingMode = getDisplayWindowingModeByActivity(
                getComponentName(ConfigChangeHandlingActivity.class));
        if (displayWindowingMode == WINDOWING_MODE_FULLSCREEN) {
            // Wait for the activity to receive the change.
            waitForActivityTransitions(ConfigChangeHandlingActivity.class,
                    Arrays.asList(ON_TOP_POSITION_LOST, ON_MULTI_WINDOW_MODE_CHANGED));
            LifecycleVerifier.assertOrder(getLifecycleLog(), ConfigChangeHandlingActivity.class,
                    Arrays.asList(ON_MULTI_WINDOW_MODE_CHANGED, ON_TOP_POSITION_LOST),
                    "moveToSplitScreen");
        } else {
            // For non-fullscreen display mode, there won't be a multi-window callback.
            waitForActivityTransitions(ConfigChangeHandlingActivity.class,
                    Arrays.asList(ON_TOP_POSITION_LOST));
            LifecycleVerifier.assertTransitionObserved(getLifecycleLog(),
                    transition(ConfigChangeHandlingActivity.class, ON_TOP_POSITION_LOST),
                    "moveToSplitScreen");
        }

        // Exit split-screen
        getLifecycleLog().clear();
        dismissSplitScreen(true /* primaryOnTop */);

        // Wait for the activity to receive the change
        final List<LifecycleLog.ActivityCallback> expectedSequence =
                Arrays.asList(ON_TOP_POSITION_GAINED, ON_MULTI_WINDOW_MODE_CHANGED);
        waitForActivityTransitions(ConfigChangeHandlingActivity.class, expectedSequence);

        if (displayWindowingMode == WINDOWING_MODE_FULLSCREEN) {
                LifecycleVerifier.assertTransitionObserved(getLifecycleLog(),
                        transition(ConfigChangeHandlingActivity.class,
                        ON_MULTI_WINDOW_MODE_CHANGED), "exitSplitScreen");
        }
        LifecycleVerifier.assertTransitionObserved(getLifecycleLog(),
                transition(ConfigChangeHandlingActivity.class, ON_TOP_POSITION_GAINED),
                "exitSplitScreen");
    }

    @Test
    public void testOnUserLeaveHint() throws Exception {
        launchActivitiesInSplitScreen(
                getLaunchActivityBuilder()
                        .setTargetActivity(getComponentName(ConfigChangeHandlingActivity.class)),
                getLaunchActivityBuilder()
                        .setIntentExtra(
                                extra -> extra.putBoolean(EXTRA_ACTIVITY_ON_USER_LEAVE_HINT, true))
                        .setTargetActivity(getComponentName(FirstActivity.class)));

        getLifecycleLog().clear();
        launchActivityAndWait(SecondActivity.class);

        waitForIdle();

        LifecycleVerifier.assertOrder(getLifecycleLog(), FirstActivity.class,
                Arrays.asList(ON_USER_LEAVE_HINT, ON_PAUSE, ON_STOP),
                "moveFromSplitScreen");
    }
}
