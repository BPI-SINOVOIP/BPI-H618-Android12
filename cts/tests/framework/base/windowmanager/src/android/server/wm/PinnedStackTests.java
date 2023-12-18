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

import static android.app.ActivityManager.LOCK_TASK_MODE_NONE;
import static android.app.WindowConfiguration.ACTIVITY_TYPE_HOME;
import static android.app.WindowConfiguration.ACTIVITY_TYPE_STANDARD;
import static android.app.WindowConfiguration.WINDOWING_MODE_FREEFORM;
import static android.app.WindowConfiguration.WINDOWING_MODE_FULLSCREEN;
import static android.app.WindowConfiguration.WINDOWING_MODE_PINNED;
import static android.app.WindowConfiguration.WINDOWING_MODE_UNDEFINED;
import static android.server.wm.CliIntentExtra.extraBool;
import static android.server.wm.CliIntentExtra.extraString;
import static android.server.wm.ComponentNameUtils.getActivityName;
import static android.server.wm.ComponentNameUtils.getWindowName;
import static android.server.wm.UiDeviceUtils.pressWindowButton;
import static android.server.wm.WindowManagerState.STATE_PAUSED;
import static android.server.wm.WindowManagerState.STATE_RESUMED;
import static android.server.wm.WindowManagerState.STATE_STOPPED;
import static android.server.wm.WindowManagerState.dpToPx;
import static android.server.wm.app.Components.ALWAYS_FOCUSABLE_PIP_ACTIVITY;
import static android.server.wm.app.Components.LAUNCH_ENTER_PIP_ACTIVITY;
import static android.server.wm.app.Components.LAUNCH_INTO_PINNED_STACK_PIP_ACTIVITY;
import static android.server.wm.app.Components.LAUNCH_PIP_ON_PIP_ACTIVITY;
import static android.server.wm.app.Components.NON_RESIZEABLE_ACTIVITY;
import static android.server.wm.app.Components.PIP_ACTIVITY;
import static android.server.wm.app.Components.PIP_ACTIVITY2;
import static android.server.wm.app.Components.PIP_ACTIVITY_WITH_MINIMAL_SIZE;
import static android.server.wm.app.Components.PIP_ACTIVITY_WITH_SAME_AFFINITY;
import static android.server.wm.app.Components.PIP_ACTIVITY_WITH_TINY_MINIMAL_SIZE;
import static android.server.wm.app.Components.PIP_ON_STOP_ACTIVITY;
import static android.server.wm.app.Components.PipActivity.ACTION_ENTER_PIP;
import static android.server.wm.app.Components.PipActivity.ACTION_FINISH;
import static android.server.wm.app.Components.PipActivity.ACTION_MOVE_TO_BACK;
import static android.server.wm.app.Components.PipActivity.ACTION_ON_PIP_REQUESTED;
import static android.server.wm.app.Components.PipActivity.EXTRA_ALLOW_AUTO_PIP;
import static android.server.wm.app.Components.PipActivity.EXTRA_ASSERT_NO_ON_STOP_BEFORE_PIP;
import static android.server.wm.app.Components.PipActivity.EXTRA_ENTER_PIP;
import static android.server.wm.app.Components.PipActivity.EXTRA_ENTER_PIP_ASPECT_RATIO_DENOMINATOR;
import static android.server.wm.app.Components.PipActivity.EXTRA_ENTER_PIP_ASPECT_RATIO_NUMERATOR;
import static android.server.wm.app.Components.PipActivity.EXTRA_ENTER_PIP_ON_PAUSE;
import static android.server.wm.app.Components.PipActivity.EXTRA_ENTER_PIP_ON_PIP_REQUESTED;
import static android.server.wm.app.Components.PipActivity.EXTRA_ENTER_PIP_ON_USER_LEAVE_HINT;
import static android.server.wm.app.Components.PipActivity.EXTRA_FINISH_SELF_ON_RESUME;
import static android.server.wm.app.Components.PipActivity.EXTRA_IS_SEAMLESS_RESIZE_ENABLED;
import static android.server.wm.app.Components.PipActivity.EXTRA_NUMBER_OF_CUSTOM_ACTIONS;
import static android.server.wm.app.Components.PipActivity.EXTRA_ON_PAUSE_DELAY;
import static android.server.wm.app.Components.PipActivity.EXTRA_PIP_ORIENTATION;
import static android.server.wm.app.Components.PipActivity.EXTRA_SET_ASPECT_RATIO_DENOMINATOR;
import static android.server.wm.app.Components.PipActivity.EXTRA_SET_ASPECT_RATIO_NUMERATOR;
import static android.server.wm.app.Components.PipActivity.EXTRA_START_ACTIVITY;
import static android.server.wm.app.Components.PipActivity.EXTRA_TAP_TO_FINISH;
import static android.server.wm.app.Components.PipActivity.PIP_CALLBACK_RESULT_KEY;
import static android.server.wm.app.Components.RESUME_WHILE_PAUSING_ACTIVITY;
import static android.server.wm.app.Components.TEST_ACTIVITY;
import static android.server.wm.app.Components.TEST_ACTIVITY_WITH_SAME_AFFINITY;
import static android.server.wm.app.Components.TRANSLUCENT_TEST_ACTIVITY;
import static android.server.wm.app.Components.TestActivity.EXTRA_CONFIGURATION;
import static android.server.wm.app.Components.TestActivity.EXTRA_FIXED_ORIENTATION;
import static android.server.wm.app.Components.TestActivity.TEST_ACTIVITY_ACTION_FINISH_SELF;
import static android.server.wm.app27.Components.SDK_27_LAUNCH_ENTER_PIP_ACTIVITY;
import static android.server.wm.app27.Components.SDK_27_PIP_ACTIVITY;
import static android.view.Display.DEFAULT_DISPLAY;

import static androidx.test.platform.app.InstrumentationRegistry.getInstrumentation;

import static org.hamcrest.Matchers.greaterThanOrEqualTo;
import static org.hamcrest.Matchers.lessThan;
import static org.hamcrest.Matchers.lessThanOrEqualTo;
import static org.junit.Assert.assertEquals;
import static org.junit.Assert.assertFalse;
import static org.junit.Assert.assertNotEquals;
import static org.junit.Assert.assertNotNull;
import static org.junit.Assert.assertThat;
import static org.junit.Assert.assertTrue;
import static org.junit.Assert.fail;
import static org.junit.Assume.assumeFalse;
import static org.junit.Assume.assumeTrue;

import android.app.ActivityTaskManager;
import android.app.PictureInPictureParams;
import android.app.TaskInfo;
import android.content.ComponentName;
import android.content.Context;
import android.content.pm.ActivityInfo;
import android.content.pm.PackageManager;
import android.content.res.Configuration;
import android.database.ContentObserver;
import android.graphics.Rect;
import android.os.Bundle;
import android.os.Handler;
import android.os.Looper;
import android.os.RemoteCallback;
import android.platform.test.annotations.Presubmit;
import android.platform.test.annotations.AsbSecurityTest;
import android.provider.Settings;
import android.server.wm.CommandSession.ActivityCallback;
import android.server.wm.CommandSession.SizeInfo;
import android.server.wm.TestJournalProvider.TestJournalContainer;
import android.server.wm.WindowManagerState.ActivityTask;
import android.server.wm.settings.SettingsSession;
import android.util.Log;
import android.util.Size;

import com.android.compatibility.common.util.AppOpsUtils;
import com.android.compatibility.common.util.SystemUtil;

import com.google.common.truth.Truth;

import org.junit.Before;
import org.junit.Ignore;
import org.junit.Test;

import java.io.IOException;
import java.util.List;
import java.util.concurrent.CompletableFuture;
import java.util.concurrent.CountDownLatch;
import java.util.concurrent.TimeUnit;

/**
 * Build/Install/Run:
 * atest CtsWindowManagerDeviceTestCases:PinnedStackTests
 */
@Presubmit
@android.server.wm.annotation.Group2
public class PinnedStackTests extends ActivityManagerTestBase {
    private static final String TAG = PinnedStackTests.class.getSimpleName();

    private static final String APP_OPS_OP_ENTER_PICTURE_IN_PICTURE = "PICTURE_IN_PICTURE";
    private static final int APP_OPS_MODE_IGNORED = 1;

    private static final int ROTATION_0 = 0;
    private static final int ROTATION_90 = 1;
    private static final int ROTATION_180 = 2;
    private static final int ROTATION_270 = 3;

    // Corresponds to ActivityInfo.SCREEN_ORIENTATION_LANDSCAPE
    private static final int ORIENTATION_LANDSCAPE = 0;
    // Corresponds to ActivityInfo.SCREEN_ORIENTATION_PORTRAIT
    private static final int ORIENTATION_PORTRAIT = 1;

    private static final float FLOAT_COMPARE_EPSILON = 0.005f;

    // Corresponds to com.android.internal.R.dimen.config_pictureInPictureMinAspectRatio
    private static final int MIN_ASPECT_RATIO_NUMERATOR = 100;
    private static final int MIN_ASPECT_RATIO_DENOMINATOR = 239;
    private static final int BELOW_MIN_ASPECT_RATIO_DENOMINATOR = MIN_ASPECT_RATIO_DENOMINATOR + 1;
    // Corresponds to com.android.internal.R.dimen.config_pictureInPictureMaxAspectRatio
    private static final int MAX_ASPECT_RATIO_NUMERATOR = 239;
    private static final int MAX_ASPECT_RATIO_DENOMINATOR = 100;
    private static final int ABOVE_MAX_ASPECT_RATIO_NUMERATOR = MAX_ASPECT_RATIO_NUMERATOR + 1;
    // Corresponds to com.android.internal.R.dimen.overridable_minimal_size_pip_resizable_task
    private static final int OVERRIDABLE_MINIMAL_SIZE_PIP_RESIZABLE_TASK = 48;

    @Before
    @Override
    public void setUp() throws Exception {
        super.setUp();
        assumeTrue(supportsPip());
    }

    @Test
    public void testMinimumDeviceSize() {
        mWmState.assertDeviceDefaultDisplaySizeForMultiWindow(
                "Devices supporting picture-in-picture must be larger than the default minimum"
                        + " task size");
    }

    @Test
    public void testEnterPictureInPictureMode() {
        pinnedStackTester(getAmStartCmd(PIP_ACTIVITY, extraString(EXTRA_ENTER_PIP, "true")),
                PIP_ACTIVITY, PIP_ACTIVITY, false /* isFocusable */);
    }

    // This test is black-listed in cts-known-failures.xml (b/35314835).
    @Ignore
    @Test
    public void testAlwaysFocusablePipActivity() {
        pinnedStackTester(getAmStartCmd(ALWAYS_FOCUSABLE_PIP_ACTIVITY),
                ALWAYS_FOCUSABLE_PIP_ACTIVITY, ALWAYS_FOCUSABLE_PIP_ACTIVITY,
                true /* isFocusable */);
    }

    // This test is black-listed in cts-known-failures.xml (b/35314835).
    @Ignore
    @Test
    public void testLaunchIntoPinnedStack() {
        pinnedStackTester(getAmStartCmd(LAUNCH_INTO_PINNED_STACK_PIP_ACTIVITY),
                LAUNCH_INTO_PINNED_STACK_PIP_ACTIVITY, ALWAYS_FOCUSABLE_PIP_ACTIVITY,
                true /* isFocusable */);
    }

    @Test
    public void testNonTappablePipActivity() {
        // Launch the tap-to-finish activity at a specific place
        launchActivity(PIP_ACTIVITY, extraString(EXTRA_ENTER_PIP, "true"),
                extraString(EXTRA_TAP_TO_FINISH, "true"));
        // Wait for animation complete since we are tapping on specific bounds
        waitForEnterPipAnimationComplete(PIP_ACTIVITY);
        assertPinnedStackExists();

        // Tap the screen at a known location in the pinned stack bounds, and ensure that it is
        // not passed down to the top task
        tapToFinishPip();
        mWmState.computeState(
                new WaitForValidActivityState(PIP_ACTIVITY));
        mWmState.assertVisibility(PIP_ACTIVITY, true);
    }

    @Test
    public void testPinnedStackInBoundsAfterRotation() {
        assumeTrue("Skipping test: no rotation support", supportsRotation());

        // Launch an activity that is not fixed-orientation so that the display can rotate
        launchActivity(TEST_ACTIVITY);
        // Launch an activity into the pinned stack
        launchActivity(PIP_ACTIVITY, extraString(EXTRA_ENTER_PIP, "true"),
                extraString(EXTRA_TAP_TO_FINISH, "true"));
        // Wait for animation complete since we are comparing bounds
        waitForEnterPipAnimationComplete(PIP_ACTIVITY);

        // Ensure that the PIP stack is fully visible in each orientation
        final RotationSession rotationSession = createManagedRotationSession();
        rotationSession.set(ROTATION_0);
        assertPinnedStackActivityIsInDisplayBounds(PIP_ACTIVITY);
        rotationSession.set(ROTATION_90);
        assertPinnedStackActivityIsInDisplayBounds(PIP_ACTIVITY);
        rotationSession.set(ROTATION_180);
        assertPinnedStackActivityIsInDisplayBounds(PIP_ACTIVITY);
        rotationSession.set(ROTATION_270);
        assertPinnedStackActivityIsInDisplayBounds(PIP_ACTIVITY);
    }

    @Test
    public void testEnterPipToOtherOrientation() {
        // Launch a portrait only app on the fullscreen stack
        launchActivity(TEST_ACTIVITY,
                extraString(EXTRA_FIXED_ORIENTATION, String.valueOf(ORIENTATION_PORTRAIT)));
        // Launch the PiP activity fixed as landscape
        launchActivity(PIP_ACTIVITY,
                extraString(EXTRA_PIP_ORIENTATION, String.valueOf(ORIENTATION_LANDSCAPE)));
        // Enter PiP, and assert that the PiP is within bounds now that the device is back in
        // portrait
        mBroadcastActionTrigger.doAction(ACTION_ENTER_PIP);
        // Wait for animation complete since we are comparing bounds
        waitForEnterPipAnimationComplete(PIP_ACTIVITY);
        assertPinnedStackExists();
        assertPinnedStackActivityIsInDisplayBounds(PIP_ACTIVITY);
    }

    @Test
    public void testEnterPipWithMinimalSize() throws Exception {
        // Launch a PiP activity with minimal size specified
        launchActivity(PIP_ACTIVITY_WITH_MINIMAL_SIZE, extraString(EXTRA_ENTER_PIP, "true"));
        // Wait for animation complete since we are comparing size
        waitForEnterPipAnimationComplete(PIP_ACTIVITY_WITH_MINIMAL_SIZE);
        assertPinnedStackExists();

        // query the minimal size
        final PackageManager pm = getInstrumentation().getTargetContext().getPackageManager();
        final ActivityInfo info = pm.getActivityInfo(
                PIP_ACTIVITY_WITH_MINIMAL_SIZE, 0 /* flags */);
        final Size minSize = new Size(info.windowLayout.minWidth, info.windowLayout.minHeight);

        // compare the bounds with minimal size
        final Rect pipBounds = getPinnedStackBounds();
        assertTrue("Pinned task bounds " + pipBounds + " isn't smaller than minimal " + minSize,
                (pipBounds.width() == minSize.getWidth()
                        && pipBounds.height() >= minSize.getHeight())
                        || (pipBounds.height() == minSize.getHeight()
                        && pipBounds.width() >= minSize.getWidth()));
    }

    @Test
    @AsbSecurityTest(cveBugId = 174302616)
    public void testEnterPipWithTinyMinimalSize() {
        // Launch a PiP activity with minimal size specified and smaller than allowed minimum
        launchActivity(PIP_ACTIVITY_WITH_TINY_MINIMAL_SIZE, extraString(EXTRA_ENTER_PIP, "true"));
        // Wait for animation complete since we are comparing size
        waitForEnterPipAnimationComplete(PIP_ACTIVITY_WITH_TINY_MINIMAL_SIZE);
        assertPinnedStackExists();

        final WindowManagerState.WindowState windowState = getWindowState(
                PIP_ACTIVITY_WITH_TINY_MINIMAL_SIZE);
        final WindowManagerState.DisplayContent display = mWmState.getDisplay(
                windowState.getDisplayId());
        final int overridableMinSize = dpToPx(
                OVERRIDABLE_MINIMAL_SIZE_PIP_RESIZABLE_TASK, display.getDpi());

        // compare the bounds to verify that it's no smaller than allowed minimum on both dimensions
        final Rect pipBounds = getPinnedStackBounds();
        assertTrue("Pinned task bounds " + pipBounds + " isn't smaller than minimal "
                        + overridableMinSize + " on both dimensions",
                pipBounds.width() >= overridableMinSize
                        && pipBounds.height() >= overridableMinSize);
    }

    @Test
    public void testEnterPipAspectRatioMin() {
        testEnterPipAspectRatio(MIN_ASPECT_RATIO_NUMERATOR, MIN_ASPECT_RATIO_DENOMINATOR);
    }

    @Test
    public void testEnterPipAspectRatioMax() {
        testEnterPipAspectRatio(MAX_ASPECT_RATIO_NUMERATOR, MAX_ASPECT_RATIO_DENOMINATOR);
    }

    private void testEnterPipAspectRatio(int num, int denom) {
        // Launch a test activity so that we're not over home
        launchActivity(TEST_ACTIVITY);

        launchActivity(PIP_ACTIVITY,
                extraString(EXTRA_ENTER_PIP, "true"),
                extraString(EXTRA_ENTER_PIP_ASPECT_RATIO_NUMERATOR, Integer.toString(num)),
                extraString(EXTRA_ENTER_PIP_ASPECT_RATIO_DENOMINATOR, Integer.toString(denom)));
        // Wait for animation complete since we are comparing aspect ratio
        waitForEnterPipAnimationComplete(PIP_ACTIVITY);
        assertPinnedStackExists();

        // Assert that we have entered PIP and that the aspect ratio is correct
        Rect pinnedStackBounds = getPinnedStackBounds();
        assertFloatEquals((float) pinnedStackBounds.width() / pinnedStackBounds.height(),
                (float) num / denom);
    }

    @Test
    public void testResizePipAspectRatioMin() {
        testResizePipAspectRatio(MIN_ASPECT_RATIO_NUMERATOR, MIN_ASPECT_RATIO_DENOMINATOR);
    }

    @Test
    public void testResizePipAspectRatioMax() {
        testResizePipAspectRatio(MAX_ASPECT_RATIO_NUMERATOR, MAX_ASPECT_RATIO_DENOMINATOR);
    }

    private void testResizePipAspectRatio(int num, int denom) {
        // Launch a test activity so that we're not over home
        launchActivity(TEST_ACTIVITY);

        launchActivity(PIP_ACTIVITY,
                extraString(EXTRA_ENTER_PIP, "true"),
                extraString(EXTRA_ENTER_PIP_ASPECT_RATIO_NUMERATOR, Integer.toString(num)),
                extraString(EXTRA_ENTER_PIP_ASPECT_RATIO_DENOMINATOR, Integer.toString(denom)));
        // Wait for animation complete since we are comparing aspect ratio
        waitForEnterPipAnimationComplete(PIP_ACTIVITY);
        assertPinnedStackExists();
        waitForValidAspectRatio(num, denom);
        Rect bounds = getPinnedStackBounds();
        assertFloatEquals((float) bounds.width() / bounds.height(), (float) num / denom);
    }

    @Test
    public void testEnterPipExtremeAspectRatioMin() {
        testEnterPipExtremeAspectRatio(MIN_ASPECT_RATIO_NUMERATOR,
                BELOW_MIN_ASPECT_RATIO_DENOMINATOR);
    }

    @Test
    public void testEnterPipExtremeAspectRatioMax() {
        testEnterPipExtremeAspectRatio(ABOVE_MAX_ASPECT_RATIO_NUMERATOR,
                MAX_ASPECT_RATIO_DENOMINATOR);
    }

    private void testEnterPipExtremeAspectRatio(int num, int denom) {
        // Launch a test activity so that we're not over home
        launchActivity(TEST_ACTIVITY);

        // Assert that we could not create a pinned stack with an extreme aspect ratio
        launchActivity(PIP_ACTIVITY,
                extraString(EXTRA_ENTER_PIP, "true"),
                extraString(EXTRA_ENTER_PIP_ASPECT_RATIO_NUMERATOR, Integer.toString(num)),
                extraString(EXTRA_ENTER_PIP_ASPECT_RATIO_DENOMINATOR, Integer.toString(denom)));
        assertPinnedStackDoesNotExist();
    }

    @Test
    public void testSetPipExtremeAspectRatioMin() {
        testSetPipExtremeAspectRatio(MIN_ASPECT_RATIO_NUMERATOR,
                BELOW_MIN_ASPECT_RATIO_DENOMINATOR);
    }

    @Test
    public void testSetPipExtremeAspectRatioMax() {
        testSetPipExtremeAspectRatio(ABOVE_MAX_ASPECT_RATIO_NUMERATOR,
                MAX_ASPECT_RATIO_DENOMINATOR);
    }

    private void testSetPipExtremeAspectRatio(int num, int denom) {
        // Launch a test activity so that we're not over home
        launchActivity(TEST_ACTIVITY);

        // Try to resize the a normal pinned stack to an extreme aspect ratio and ensure that
        // fails (the aspect ratio remains the same)
        launchActivity(PIP_ACTIVITY,
                extraString(EXTRA_ENTER_PIP, "true"),
                extraString(EXTRA_ENTER_PIP_ASPECT_RATIO_NUMERATOR,
                        Integer.toString(MAX_ASPECT_RATIO_NUMERATOR)),
                extraString(EXTRA_ENTER_PIP_ASPECT_RATIO_DENOMINATOR,
                        Integer.toString(MAX_ASPECT_RATIO_DENOMINATOR)),
                extraString(EXTRA_SET_ASPECT_RATIO_NUMERATOR, Integer.toString(num)),
                extraString(EXTRA_SET_ASPECT_RATIO_DENOMINATOR, Integer.toString(denom)));
        // Wait for animation complete since we are comparing aspect ratio
        waitForEnterPipAnimationComplete(PIP_ACTIVITY);
        assertPinnedStackExists();
        Rect pinnedStackBounds = getPinnedStackBounds();
        assertFloatEquals((float) pinnedStackBounds.width() / pinnedStackBounds.height(),
                (float) MAX_ASPECT_RATIO_NUMERATOR / MAX_ASPECT_RATIO_DENOMINATOR);
    }

    @Test
    public void testDisallowPipLaunchFromStoppedActivity() {
        // Launch the bottom pip activity which will launch a new activity on top and attempt to
        // enter pip when it is stopped
        launchActivity(PIP_ON_STOP_ACTIVITY);

        // Wait for the bottom pip activity to be stopped
        mWmState.waitForActivityState(PIP_ON_STOP_ACTIVITY, STATE_STOPPED);

        // Assert that there is no pinned stack (that enterPictureInPicture() failed)
        assertPinnedStackDoesNotExist();
    }

    @Test
    public void testAutoEnterPictureInPicture() {
        // Launch a test activity so that we're not over home
        launchActivity(TEST_ACTIVITY);

        // Launch the PIP activity on pause
        launchActivity(PIP_ACTIVITY, extraString(EXTRA_ENTER_PIP_ON_PAUSE, "true"));
        assertPinnedStackDoesNotExist();

        // Go home and ensure that there is a pinned stack
        launchHomeActivity();
        waitForEnterPip(PIP_ACTIVITY);
        assertPinnedStackExists();
    }

    @Test
    public void testAutoEnterPictureInPictureOnUserLeaveHintWhenPipRequestedNotOverridden()
            {
        // Launch a test activity so that we're not over home
        launchActivity(TEST_ACTIVITY);

        // Launch the PIP activity that enters PIP on user leave hint, not on PIP requested
        launchActivity(PIP_ACTIVITY, extraString(EXTRA_ENTER_PIP_ON_USER_LEAVE_HINT, "true"));
        assertPinnedStackDoesNotExist();

        // Go home and ensure that there is a pinned stack
        separateTestJournal();
        launchHomeActivity();
        waitForEnterPipAnimationComplete(PIP_ACTIVITY);
        assertPinnedStackExists();

        final ActivityLifecycleCounts lifecycleCounts = new ActivityLifecycleCounts(PIP_ACTIVITY);
        // Check the order of the callbacks accounting for a task overlay activity that might show.
        // The PIP request (with a user leave hint) should come before the pip mode change.
        final int firstUserLeaveIndex =
                lifecycleCounts.getFirstIndex(ActivityCallback.ON_USER_LEAVE_HINT);
        final int firstPipRequestedIndex =
                lifecycleCounts.getFirstIndex(ActivityCallback.ON_PICTURE_IN_PICTURE_REQUESTED);
        final int firstPipModeChangedIndex =
                lifecycleCounts.getFirstIndex(ActivityCallback.ON_PICTURE_IN_PICTURE_MODE_CHANGED);
        assertTrue("missing request", firstPipRequestedIndex != -1);
        assertTrue("missing user leave", firstUserLeaveIndex != -1);
        assertTrue("missing pip mode changed", firstPipModeChangedIndex != -1);
        assertTrue("pip requested not before pause",
                firstPipRequestedIndex < firstUserLeaveIndex);
        assertTrue("unexpected user leave hint",
                firstUserLeaveIndex < firstPipModeChangedIndex);
    }

    @Test
    public void testAutoEnterPictureInPictureOnPictureInPictureRequested() {
        // Launch a test activity so that we're not over home
        launchActivity(TEST_ACTIVITY);

        // Launch the PIP activity on pip requested
        launchActivity(PIP_ACTIVITY, extraString(EXTRA_ENTER_PIP_ON_PIP_REQUESTED, "true"));
        assertPinnedStackDoesNotExist();

        // Call onPictureInPictureRequested and verify activity enters pip
        separateTestJournal();
        mBroadcastActionTrigger.doAction(ACTION_ON_PIP_REQUESTED);
        waitForEnterPipAnimationComplete(PIP_ACTIVITY);
        assertPinnedStackExists();

        final ActivityLifecycleCounts lifecycleCounts = new ActivityLifecycleCounts(PIP_ACTIVITY);
        // Check the order of the callbacks accounting for a task overlay activity that might show.
        // The PIP request (without a user leave hint) should come before the pip mode change.
        final int firstUserLeaveIndex =
                lifecycleCounts.getFirstIndex(ActivityCallback.ON_USER_LEAVE_HINT);
        final int firstPipRequestedIndex =
                lifecycleCounts.getFirstIndex(ActivityCallback.ON_PICTURE_IN_PICTURE_REQUESTED);
        final int firstPipModeChangedIndex =
                lifecycleCounts.getFirstIndex(ActivityCallback.ON_PICTURE_IN_PICTURE_MODE_CHANGED);
        assertTrue("missing request", firstPipRequestedIndex != -1);
        assertTrue("missing pip mode changed", firstPipModeChangedIndex != -1);
        assertTrue("pip requested not before pause",
                firstPipRequestedIndex < firstPipModeChangedIndex);
        assertTrue("unexpected user leave hint",
                firstUserLeaveIndex == -1 || firstUserLeaveIndex > firstPipModeChangedIndex);
    }

    @Test
    public void testAutoEnterPictureInPictureLaunchActivity() {
        // Launch a test activity so that we're not over home
        launchActivity(TEST_ACTIVITY);

        // Launch the PIP activity on pause, and have it start another activity on
        // top of itself.  Wait for the new activity to be visible and ensure that the pinned stack
        // was not created in the process
        launchActivity(PIP_ACTIVITY,
                extraString(EXTRA_ENTER_PIP_ON_PAUSE, "true"),
                extraString(EXTRA_START_ACTIVITY, getActivityName(NON_RESIZEABLE_ACTIVITY)));
        mWmState.computeState(
                new WaitForValidActivityState(NON_RESIZEABLE_ACTIVITY));
        assertPinnedStackDoesNotExist();

        // Go home while the pip activity is open and ensure the previous activity is not PIPed
        launchHomeActivity();
        assertPinnedStackDoesNotExist();
    }

    @Test
    public void testAutoEnterPictureInPictureFinish() {
        // Launch a test activity so that we're not over home
        launchActivity(TEST_ACTIVITY);

        // Launch the PIP activity on pause, and set it to finish itself after
        // some period.  Wait for the previous activity to be visible, and ensure that the pinned
        // stack was not created in the process
        launchActivity(PIP_ACTIVITY,
                extraString(EXTRA_ENTER_PIP_ON_PAUSE, "true"),
                extraString(EXTRA_FINISH_SELF_ON_RESUME, "true"));
        assertPinnedStackDoesNotExist();
    }

    @Test
    public void testAutoEnterPictureInPictureAspectRatio() {
        // Launch the PIP activity on pause, and set the aspect ratio
        launchActivity(PIP_ACTIVITY,
                extraString(EXTRA_ENTER_PIP_ON_PAUSE, "true"),
                extraString(EXTRA_SET_ASPECT_RATIO_NUMERATOR,
                        Integer.toString(MAX_ASPECT_RATIO_NUMERATOR)),
                extraString(EXTRA_SET_ASPECT_RATIO_DENOMINATOR,
                        Integer.toString(MAX_ASPECT_RATIO_DENOMINATOR)));

        // Go home while the pip activity is open to trigger auto-PIP
        launchHomeActivity();
        // Wait for animation complete since we are comparing aspect ratio
        waitForEnterPipAnimationComplete(PIP_ACTIVITY);
        assertPinnedStackExists();

        waitForValidAspectRatio(MAX_ASPECT_RATIO_NUMERATOR, MAX_ASPECT_RATIO_DENOMINATOR);
        Rect bounds = getPinnedStackBounds();
        assertFloatEquals((float) bounds.width() / bounds.height(),
                (float) MAX_ASPECT_RATIO_NUMERATOR / MAX_ASPECT_RATIO_DENOMINATOR);
    }

    @Test
    public void testAutoEnterPictureInPictureOverPip() {
        // Launch another PIP activity
        launchActivity(LAUNCH_INTO_PINNED_STACK_PIP_ACTIVITY);
        waitForEnterPip(ALWAYS_FOCUSABLE_PIP_ACTIVITY);
        assertPinnedStackExists();

        // Launch the PIP activity on pause
        launchActivity(PIP_ACTIVITY, extraString(EXTRA_ENTER_PIP_ON_PAUSE, "true"));

        // Go home while the PIP activity is open to try to trigger auto-enter PIP
        launchHomeActivity();
        assertPinnedStackExists();

        // Ensure that auto-enter pip failed and that the resumed activity in the pinned stack is
        // still the first activity
        final ActivityTask pinnedStack = getPinnedStack();
        assertEquals(getActivityName(ALWAYS_FOCUSABLE_PIP_ACTIVITY), pinnedStack.mRealActivity);
    }

    @Test
    public void testDismissPipWhenLaunchNewOne() {
        // Launch another PIP activity
        launchActivity(LAUNCH_INTO_PINNED_STACK_PIP_ACTIVITY);
        waitForEnterPip(ALWAYS_FOCUSABLE_PIP_ACTIVITY);
        assertPinnedStackExists();
        final ActivityTask pinnedStack = getPinnedStack();

        launchActivityInNewTask(LAUNCH_INTO_PINNED_STACK_PIP_ACTIVITY);
        waitForEnterPip(ALWAYS_FOCUSABLE_PIP_ACTIVITY);

        assertEquals(1, mWmState.countStacks(WINDOWING_MODE_PINNED, ACTIVITY_TYPE_STANDARD));
    }

    @Test
    public void testDisallowMultipleTasksInPinnedStack() {
        // Launch a test activity so that we have multiple fullscreen tasks
        launchActivity(TEST_ACTIVITY);

        // Launch first PIP activity
        launchActivity(PIP_ACTIVITY);
        mBroadcastActionTrigger.doAction(ACTION_ENTER_PIP);
        waitForEnterPipAnimationComplete(PIP_ACTIVITY);
        int defaultDisplayWindowingMode = getDefaultDisplayWindowingMode(PIP_ACTIVITY);

        // Launch second PIP activity
        launchActivity(PIP_ACTIVITY2, extraString(EXTRA_ENTER_PIP, "true"));
        waitForEnterPipAnimationComplete(PIP_ACTIVITY2);

        final ActivityTask pinnedStack = getPinnedStack();
        assertEquals(0, pinnedStack.getTasks().size());
        assertTrue(mWmState.containsActivityInWindowingMode(
                PIP_ACTIVITY2, WINDOWING_MODE_PINNED));
        assertTrue(mWmState.containsActivityInWindowingMode(
                PIP_ACTIVITY, defaultDisplayWindowingMode));
    }

    @Test
    public void testPipUnPipOverHome() {
        // Launch a task behind home to assert that the next fullscreen task isn't visible when
        // leaving PiP.
        launchActivity(TEST_ACTIVITY);
        // Go home
        launchHomeActivity();
        // Launch an auto pip activity
        launchActivity(PIP_ACTIVITY, extraString(EXTRA_ENTER_PIP, "true"));
        waitForEnterPip(PIP_ACTIVITY);
        assertPinnedStackExists();

        // Relaunch the activity to fullscreen to trigger the activity to exit and re-enter pip
        launchActivity(PIP_ACTIVITY);
        waitForExitPipToFullscreen(PIP_ACTIVITY);
        mBroadcastActionTrigger.doAction(ACTION_ENTER_PIP);
        waitForEnterPipAnimationComplete(PIP_ACTIVITY);
        mWmState.assertVisibility(TEST_ACTIVITY, false);
        mWmState.assertHomeActivityVisible(true);
    }

    @Test
    public void testPipUnPipOverApp() {
        // Launch a test activity so that we're not over home
        launchActivity(TEST_ACTIVITY);

        // Launch an auto pip activity
        launchActivity(PIP_ACTIVITY, extraString(EXTRA_ENTER_PIP, "true"));
        waitForEnterPip(PIP_ACTIVITY);
        assertPinnedStackExists();

        // Relaunch the activity to fullscreen to trigger the activity to exit and re-enter pip
        launchActivity(PIP_ACTIVITY);
        waitForExitPipToFullscreen(PIP_ACTIVITY);
        mBroadcastActionTrigger.doAction(ACTION_ENTER_PIP);
        waitForEnterPipAnimationComplete(PIP_ACTIVITY);
        mWmState.assertVisibility(TEST_ACTIVITY, true);
    }

    @Test
    public void testRemovePipWithNoFullscreenOrFreeformStack() {
        // Launch a pip activity
        launchActivity(PIP_ACTIVITY);
        int windowingMode = mWmState.getTaskByActivity(PIP_ACTIVITY).getWindowingMode();
        enterPipAndAssertPinnedTaskExists(PIP_ACTIVITY);

        // Remove the stack and ensure that the task is now in the fullscreen/freeform stack (when
        // no fullscreen/freeform stack existed before)
        removeRootTasksInWindowingModes(WINDOWING_MODE_PINNED);
        assertPinnedStackStateOnMoveToBackStack(PIP_ACTIVITY,
                WINDOWING_MODE_UNDEFINED, ACTIVITY_TYPE_HOME, windowingMode);
    }

    @Test
    public void testRemovePipWithVisibleFullscreenOrFreeformStack() {
        // Launch a fullscreen/freeform activity, and a pip activity over that
        launchActivity(TEST_ACTIVITY);
        launchActivity(PIP_ACTIVITY);
        int testAppWindowingMode = mWmState.getTaskByActivity(TEST_ACTIVITY).getWindowingMode();
        int pipWindowingMode = mWmState.getTaskByActivity(PIP_ACTIVITY).getWindowingMode();
        enterPipAndAssertPinnedTaskExists(PIP_ACTIVITY);

        // Remove the stack and ensure that the task is placed in the fullscreen/freeform stack,
        // behind the top fullscreen/freeform activity
        removeRootTasksInWindowingModes(WINDOWING_MODE_PINNED);
        assertPinnedStackStateOnMoveToBackStack(PIP_ACTIVITY,
                testAppWindowingMode, ACTIVITY_TYPE_STANDARD, pipWindowingMode);
    }

    @Test
    public void testRemovePipWithHiddenFullscreenOrFreeformStack() {
        // Launch a fullscreen/freeform activity, return home and while the fullscreen/freeform
        // stack is hidden, launch a pip activity over home
        launchActivity(TEST_ACTIVITY);
        launchHomeActivity();
        launchActivity(PIP_ACTIVITY);
        int windowingMode = mWmState.getTaskByActivity(PIP_ACTIVITY).getWindowingMode();
        enterPipAndAssertPinnedTaskExists(PIP_ACTIVITY);

        // Remove the stack and ensure that the task is placed on top of the hidden
        // fullscreen/freeform stack, but that the home stack is still focused
        removeRootTasksInWindowingModes(WINDOWING_MODE_PINNED);
        assertPinnedStackStateOnMoveToBackStack(PIP_ACTIVITY,
                WINDOWING_MODE_UNDEFINED, ACTIVITY_TYPE_HOME, windowingMode);
    }

    @Test
    public void testMovePipToBackWithNoFullscreenOrFreeformStack() {
        // Start with a clean slate, remove all the stacks but home
        removeRootTasksWithActivityTypes(ALL_ACTIVITY_TYPE_BUT_HOME);

        // Launch a pip activity
        launchActivity(PIP_ACTIVITY);
        int windowingMode = mWmState.getTaskByActivity(PIP_ACTIVITY).getWindowingMode();
        enterPipAndAssertPinnedTaskExists(PIP_ACTIVITY);

        // Remove the stack and ensure that the task is now in the fullscreen/freeform stack (when
        // no fullscreen/freeform stack existed before)
        mBroadcastActionTrigger.doAction(ACTION_MOVE_TO_BACK);
        assertPinnedStackStateOnMoveToBackStack(PIP_ACTIVITY,
                WINDOWING_MODE_UNDEFINED, ACTIVITY_TYPE_HOME, windowingMode);
    }

    @Test
    public void testMovePipToBackWithVisibleFullscreenOrFreeformStack() {
        // Launch a fullscreen/freeform activity, and a pip activity over that
        launchActivity(TEST_ACTIVITY);
        launchActivity(PIP_ACTIVITY);
        int testAppWindowingMode = mWmState.getTaskByActivity(TEST_ACTIVITY).getWindowingMode();
        int pipWindowingMode = mWmState.getTaskByActivity(PIP_ACTIVITY).getWindowingMode();
        enterPipAndAssertPinnedTaskExists(PIP_ACTIVITY);

        // Remove the stack and ensure that the task is placed in the fullscreen/freeform stack,
        // behind the top fullscreen/freeform activity
        mBroadcastActionTrigger.doAction(ACTION_MOVE_TO_BACK);
        assertPinnedStackStateOnMoveToBackStack(PIP_ACTIVITY,
                testAppWindowingMode, ACTIVITY_TYPE_STANDARD, pipWindowingMode);
    }

    @Test
    public void testMovePipToBackWithHiddenFullscreenOrFreeformStack() {
        // Launch a fullscreen/freeform activity, return home and while the fullscreen/freeform
        // stack is hidden, launch a pip activity over home
        launchActivity(TEST_ACTIVITY);
        launchHomeActivity();
        launchActivity(PIP_ACTIVITY);
        int windowingMode = mWmState.getTaskByActivity(PIP_ACTIVITY).getWindowingMode();
        enterPipAndAssertPinnedTaskExists(PIP_ACTIVITY);

        // Remove the stack and ensure that the task is placed on top of the hidden
        // fullscreen/freeform stack, but that the home stack is still focused
        mBroadcastActionTrigger.doAction(ACTION_MOVE_TO_BACK);
        assertPinnedStackStateOnMoveToBackStack(
                PIP_ACTIVITY, WINDOWING_MODE_UNDEFINED, ACTIVITY_TYPE_HOME, windowingMode);
    }

    @Test
    public void testPinnedStackAlwaysOnTop() {
        // Launch activity into pinned stack and assert it's on top.
        launchActivity(PIP_ACTIVITY, extraString(EXTRA_ENTER_PIP, "true"));
        waitForEnterPip(PIP_ACTIVITY);
        assertPinnedStackExists();
        assertPinnedStackIsOnTop();

        // Launch another activity in fullscreen stack and check that pinned stack is still on top.
        launchActivity(TEST_ACTIVITY);
        assertPinnedStackExists();
        assertPinnedStackIsOnTop();

        // Launch home and check that pinned stack is still on top.
        launchHomeActivity();
        assertPinnedStackExists();
        assertPinnedStackIsOnTop();
    }

    @Test
    public void testAppOpsDenyPipOnPause() {
        try (final AppOpsSession appOpsSession = new AppOpsSession(PIP_ACTIVITY)) {
            // Disable enter-pip and try to enter pip
            appOpsSession.setOpToMode(APP_OPS_OP_ENTER_PICTURE_IN_PICTURE, APP_OPS_MODE_IGNORED);

            // Launch the PIP activity on pause
            launchActivity(PIP_ACTIVITY, extraString(EXTRA_ENTER_PIP, "true"));
            assertPinnedStackDoesNotExist();

            // Go home and ensure that there is no pinned stack
            launchHomeActivity();
            assertPinnedStackDoesNotExist();
        }
    }

    @Test
    public void testEnterPipFromTaskWithMultipleActivities() {
        // Try to enter picture-in-picture from an activity that has more than one activity in the
        // task and ensure that it works
        launchActivity(LAUNCH_ENTER_PIP_ACTIVITY);
        waitForEnterPip(PIP_ACTIVITY);

        final ActivityTask task = mWmState.getTaskByActivity(LAUNCH_ENTER_PIP_ACTIVITY);
        assertEquals(1, task.mActivities.size());
        assertPinnedStackExists();
    }

    @Test
    public void testPipFromTaskWithMultipleActivitiesAndExpandPip() {
        // Try to enter picture-in-picture from an activity that has more than one activity in the
        // task and ensure pinned task can go back to its original task when expand to fullscreen
        launchActivity(LAUNCH_ENTER_PIP_ACTIVITY);
        waitForEnterPip(PIP_ACTIVITY);

        mBroadcastActionTrigger.expandPip();
        waitForExitPipToFullscreen(PIP_ACTIVITY);

        final ActivityTask task = mWmState.getTaskByActivity(LAUNCH_ENTER_PIP_ACTIVITY);
        assertEquals(2, task.mActivities.size());
    }

    @Test
    public void testPipFromTaskWithMultipleActivitiesAndDismissPip() {
        // Try to enter picture-in-picture from an activity that has more than one activity in the
        // task and ensure flags on original task get reset after dismissing pip
        launchActivity(LAUNCH_ENTER_PIP_ACTIVITY);
        waitForEnterPip(PIP_ACTIVITY);

        mBroadcastActionTrigger.doAction(ACTION_FINISH);
        waitForPinnedStackRemoved();

        final ActivityTask task = mWmState.getTaskByActivity(LAUNCH_ENTER_PIP_ACTIVITY);
        assertFalse(task.mHasChildPipActivity);
    }

    @Test
    public void testPipFromTaskWithMultipleActivitiesAndFinishOriginalTask() {
        // Try to enter picture-in-picture from an activity that finished itself and ensure
        // pinned task is removed when the original task vanishes
        launchActivity(LAUNCH_ENTER_PIP_ACTIVITY,
                extraString(EXTRA_FINISH_SELF_ON_RESUME, "true"));

        waitForEnterPip(PIP_ACTIVITY);
        waitForPinnedStackRemoved();

        assertPinnedStackDoesNotExist();
    }

    @Test
    public void testPipFromTaskWithMultipleActivitiesAndRemoveOriginalTask() {
        // Try to enter picture-in-picture from an activity that has more than one activity in the
        // task and ensure pinned task is removed when the original task vanishes
        launchActivity(LAUNCH_ENTER_PIP_ACTIVITY);
        waitForEnterPip(PIP_ACTIVITY);

        final int originalTaskId = mWmState.getTaskByActivity(LAUNCH_ENTER_PIP_ACTIVITY).mTaskId;
        removeRootTask(originalTaskId);
        waitForPinnedStackRemoved();

        assertPinnedStackDoesNotExist();
    }

    @Test
    public void testLaunchStoppedActivityWithPiPInSameProcessPreQ() {
        // Try to enter picture-in-picture from an activity that has more than one activity in the
        // task and ensure that it works, for pre-Q app
        launchActivity(SDK_27_LAUNCH_ENTER_PIP_ACTIVITY,
                extraString(EXTRA_ENTER_PIP, "true"));
        waitForEnterPip(SDK_27_PIP_ACTIVITY);
        assertPinnedStackExists();

        // Puts the host activity to stopped state
        launchHomeActivity();
        mWmState.assertHomeActivityVisible(true);
        waitAndAssertActivityState(SDK_27_LAUNCH_ENTER_PIP_ACTIVITY, STATE_STOPPED,
                "Activity should become STOPPED");
        mWmState.assertVisibility(SDK_27_LAUNCH_ENTER_PIP_ACTIVITY, false);

        // Host activity should be visible after re-launch and PiP window still exists
        launchActivity(SDK_27_LAUNCH_ENTER_PIP_ACTIVITY);
        waitAndAssertActivityState(SDK_27_LAUNCH_ENTER_PIP_ACTIVITY, STATE_RESUMED,
                "Activity should become RESUMED");
        mWmState.assertVisibility(SDK_27_LAUNCH_ENTER_PIP_ACTIVITY, true);
        assertPinnedStackExists();
    }

    @Test
    public void testEnterPipWithResumeWhilePausingActivityNoStop() {
        /*
         * Launch the resumeWhilePausing activity and ensure that the PiP activity did not get
         * stopped and actually went into the pinned stack.
         *
         * Note that this is a workaround because to trigger the path that we want to happen in
         * activity manager, we need to add the leaving activity to the stopping state, which only
         * happens when a hidden stack is brought forward. Normally, this happens when you go home,
         * but since we can't launch into the home stack directly, we have a workaround.
         *
         * 1) Launch an activity in a new dynamic stack
         * 2) Start the PiP activity that will enter picture-in-picture when paused in the
         *    fullscreen stack
         * 3) Bring the activity in the dynamic stack forward to trigger PiP
         */
        launchActivity(RESUME_WHILE_PAUSING_ACTIVITY, WINDOWING_MODE_FULLSCREEN);
        // Launch an activity that will enter PiP when it is paused with a delay that is long enough
        // for the next resumeWhilePausing activity to finish resuming, but slow enough to not
        // trigger the current system pause timeout (currently 500ms)
        launchActivity(PIP_ACTIVITY, WINDOWING_MODE_FULLSCREEN,
                extraString(EXTRA_ENTER_PIP_ON_PAUSE, "true"),
                extraString(EXTRA_ON_PAUSE_DELAY, "350"),
                extraString(EXTRA_ASSERT_NO_ON_STOP_BEFORE_PIP, "true"));
        launchActivity(RESUME_WHILE_PAUSING_ACTIVITY);
        assertPinnedStackExists();
    }

    @Test
    public void testDisallowEnterPipActivityLocked() {
        launchActivity(PIP_ACTIVITY, extraString(EXTRA_ENTER_PIP_ON_PAUSE, "true"));
        ActivityTask task = mWmState.getStackByActivity(PIP_ACTIVITY);

        // Lock the task and ensure that we can't enter picture-in-picture both explicitly and
        // when paused
        SystemUtil.runWithShellPermissionIdentity(() -> {
            try {
                mAtm.startSystemLockTaskMode(task.mTaskId);
                waitForOrFail("Task in lock mode", () -> {
                    return mAm.getLockTaskModeState() != LOCK_TASK_MODE_NONE;
                });
                mBroadcastActionTrigger.doAction(ACTION_ENTER_PIP);
                waitForEnterPip(PIP_ACTIVITY);
                assertPinnedStackDoesNotExist();
                launchHomeActivityNoWait();
                mWmState.computeState();
                assertPinnedStackDoesNotExist();
            } finally {
                mAtm.stopSystemLockTaskMode();
            }
        });
    }

    @Test
    public void testConfigurationChangeOrderDuringTransition() {
        // Launch a PiP activity and ensure configuration change only happened once, and that the
        // configuration change happened after the picture-in-picture and multi-window callbacks
        launchActivity(PIP_ACTIVITY, WINDOWING_MODE_FULLSCREEN);
        separateTestJournal();
        int windowingMode = mWmState.getTaskByActivity(PIP_ACTIVITY).getWindowingMode();
        enterPipAndAssertPinnedTaskExists(PIP_ACTIVITY);
        waitForValidPictureInPictureCallbacks(PIP_ACTIVITY);
        assertValidPictureInPictureCallbackOrder(PIP_ACTIVITY, windowingMode);

        // Trigger it to go back to original mode and ensure that only triggered one configuration
        // change as well
        separateTestJournal();
        launchActivity(PIP_ACTIVITY);
        waitForValidPictureInPictureCallbacks(PIP_ACTIVITY);
        assertValidPictureInPictureCallbackOrder(PIP_ACTIVITY, windowingMode);
    }

    /** Helper class to save, set, and restore transition_animation_scale preferences. */
    private static class TransitionAnimationScaleSession extends SettingsSession<Float> {
        TransitionAnimationScaleSession() {
            super(Settings.Global.getUriFor(Settings.Global.TRANSITION_ANIMATION_SCALE),
                    Settings.Global::getFloat,
                    Settings.Global::putFloat);
        }

        @Override
        public void close() {
            // Wait for the restored setting to apply before we continue on with the next test
            final CountDownLatch waitLock = new CountDownLatch(1);
            final Context context = getInstrumentation().getTargetContext();
            context.getContentResolver().registerContentObserver(mUri, false,
                    new ContentObserver(new Handler(Looper.getMainLooper())) {
                        @Override
                        public void onChange(boolean selfChange) {
                            waitLock.countDown();
                        }
                    });
            super.close();
            try {
                if (!waitLock.await(2, TimeUnit.SECONDS)) {
                    Log.i(TAG, "TransitionAnimationScaleSession value not restored");
                }
            } catch (InterruptedException impossible) {}
        }
    }

    @Ignore("b/149946388")
    @Test
    public void testEnterPipInterruptedCallbacks() {
        final TransitionAnimationScaleSession transitionAnimationScaleSession =
                mObjectTracker.manage(new TransitionAnimationScaleSession());
        // Slow down the transition animations for this test
        transitionAnimationScaleSession.set(20f);

        // Launch a PiP activity
        launchActivity(PIP_ACTIVITY, extraString(EXTRA_ENTER_PIP, "true"));
        // Wait until the PiP activity has moved into the pinned stack (happens before the
        // transition has started)
        waitForEnterPip(PIP_ACTIVITY);
        assertPinnedStackExists();

        // Relaunch the PiP activity back into fullscreen
        separateTestJournal();
        launchActivity(PIP_ACTIVITY);
        // Wait until the PiP activity is reparented into the fullscreen stack (happens after
        // the transition has finished)
        waitForExitPipToFullscreen(PIP_ACTIVITY);

        // Ensure that we get the callbacks indicating that PiP/MW mode was cancelled, but no
        // configuration change (since none was sent)
        final ActivityLifecycleCounts lifecycleCounts = new ActivityLifecycleCounts(PIP_ACTIVITY);
        assertEquals("onConfigurationChanged", 0,
                lifecycleCounts.getCount(ActivityCallback.ON_CONFIGURATION_CHANGED));
        assertEquals("onPictureInPictureModeChanged", 1,
                lifecycleCounts.getCount(ActivityCallback.ON_PICTURE_IN_PICTURE_MODE_CHANGED));
        assertEquals("onMultiWindowModeChanged", 1,
                lifecycleCounts.getCount(ActivityCallback.ON_MULTI_WINDOW_MODE_CHANGED));
    }

    @Test
    public void testStopBeforeMultiWindowCallbacksOnDismiss() {
        // Launch a PiP activity
        launchActivity(PIP_ACTIVITY);
        int windowingMode = mWmState.getTaskByActivity(PIP_ACTIVITY).getWindowingMode();

        // Skip the test if it's freeform, since freeform <-> PIP does not trigger any multi-window
        // calbacks.
        assumeFalse(windowingMode == WINDOWING_MODE_FREEFORM);

        mBroadcastActionTrigger.doAction(ACTION_ENTER_PIP);
        // Wait for animation complete so that system has reported pip mode change event to
        // client and the last reported pip mode has updated.
        waitForEnterPipAnimationComplete(PIP_ACTIVITY);
        assertPinnedStackExists();

        // Dismiss it
        separateTestJournal();
        removeRootTasksInWindowingModes(WINDOWING_MODE_PINNED);
        waitForExitPipToFullscreen(PIP_ACTIVITY);
        waitForValidPictureInPictureCallbacks(PIP_ACTIVITY);

        // Confirm that we get stop before the multi-window and picture-in-picture mode change
        // callbacks
        final ActivityLifecycleCounts lifecycles = new ActivityLifecycleCounts(PIP_ACTIVITY);
        assertEquals("onStop", 1, lifecycles.getCount(ActivityCallback.ON_STOP));
        assertEquals("onPictureInPictureModeChanged", 1,
                lifecycles.getCount(ActivityCallback.ON_PICTURE_IN_PICTURE_MODE_CHANGED));
        assertEquals("onMultiWindowModeChanged", 1,
                lifecycles.getCount(ActivityCallback.ON_MULTI_WINDOW_MODE_CHANGED));
        final int lastStopIndex = lifecycles.getLastIndex(ActivityCallback.ON_STOP);
        final int lastPipIndex = lifecycles.getLastIndex(
                ActivityCallback.ON_PICTURE_IN_PICTURE_MODE_CHANGED);
        final int lastMwIndex = lifecycles.getLastIndex(
                ActivityCallback.ON_MULTI_WINDOW_MODE_CHANGED);
        assertThat("onStop should be before onPictureInPictureModeChanged",
                lastStopIndex, lessThan(lastPipIndex));
        assertThat("onPictureInPictureModeChanged should be before onMultiWindowModeChanged",
                lastPipIndex, lessThan(lastMwIndex));
    }

    @Test
    public void testPreventSetAspectRatioWhileExpanding() {
        // Launch the PiP activity
        launchActivity(PIP_ACTIVITY, extraString(EXTRA_ENTER_PIP, "true"));
        waitForEnterPip(PIP_ACTIVITY);

        // Trigger it to go back to fullscreen and try to set the aspect ratio, and ensure that the
        // call to set the aspect ratio did not prevent the PiP from returning to fullscreen
        mBroadcastActionTrigger.expandPipWithAspectRatio("123456789", "100000000");
        waitForExitPipToFullscreen(PIP_ACTIVITY);
        assertPinnedStackDoesNotExist();
    }

    @Test
    public void testSetRequestedOrientationWhilePinned() {
        assumeTrue("Skipping test: no orientation request support", supportsOrientationRequest());
        // Launch the PiP activity fixed as portrait, and enter picture-in-picture
        launchActivity(PIP_ACTIVITY, WINDOWING_MODE_FULLSCREEN,
                extraString(EXTRA_PIP_ORIENTATION, String.valueOf(ORIENTATION_PORTRAIT)),
                extraString(EXTRA_ENTER_PIP, "true"));
        waitForEnterPip(PIP_ACTIVITY);
        assertPinnedStackExists();

        // Request that the orientation is set to landscape
        mBroadcastActionTrigger.requestOrientationForPip(ORIENTATION_LANDSCAPE);

        // Launch the activity back into fullscreen and ensure that it is now in landscape
        launchActivity(PIP_ACTIVITY);
        waitForExitPipToFullscreen(PIP_ACTIVITY);
        assertPinnedStackDoesNotExist();
        mWmState.waitForLastOrientation(ORIENTATION_LANDSCAPE);

        mWmState.computeState(PIP_ACTIVITY);
        final ActivityTask activityTask =
                mWmState.getTaskByActivity(PIP_ACTIVITY);
        if (activityTask.getWindowingMode() == WINDOWING_MODE_FULLSCREEN) {
            assertEquals(ORIENTATION_LANDSCAPE, mWmState.getLastOrientation());
        } else {
            assertEquals(ORIENTATION_LANDSCAPE, activityTask.mOverrideConfiguration.orientation);
        }
    }

    @Test
    public void testWindowButtonEntersPip() {
        assumeTrue(!mWmState.isHomeRecentsComponent());

        // Launch the PiP activity trigger the window button, ensure that we have entered PiP
        launchActivity(PIP_ACTIVITY);
        pressWindowButton();
        waitForEnterPip(PIP_ACTIVITY);
        assertPinnedStackExists();
    }

    @Test
    public void testFinishPipActivityWithTaskOverlay() {
        // Launch PiP activity
        launchActivity(PIP_ACTIVITY, extraString(EXTRA_ENTER_PIP, "true"));
        waitForEnterPip(PIP_ACTIVITY);
        assertPinnedStackExists();
        int taskId = mWmState.getStandardStackByWindowingMode(
                WINDOWING_MODE_PINNED).getTopTask().mTaskId;

        // Ensure that we don't any any other overlays as a result of launching into PIP
        launchHomeActivity();

        // Launch task overlay activity into PiP activity task
        launchPinnedActivityAsTaskOverlay(TRANSLUCENT_TEST_ACTIVITY, taskId);

        // Finish the PiP activity and ensure that there is no pinned stack
        mBroadcastActionTrigger.doAction(ACTION_FINISH);
        waitForPinnedStackRemoved();
        assertPinnedStackDoesNotExist();
    }

    @Test
    public void testNoResumeAfterTaskOverlayFinishes() {
        // Launch PiP activity
        launchActivity(PIP_ACTIVITY, extraString(EXTRA_ENTER_PIP, "true"));
        waitForEnterPip(PIP_ACTIVITY);
        assertPinnedStackExists();
        ActivityTask stack = mWmState.getStandardStackByWindowingMode(WINDOWING_MODE_PINNED);
        int taskId = stack.getTopTask().mTaskId;

        // Launch task overlay activity into PiP activity task
        launchPinnedActivityAsTaskOverlay(TRANSLUCENT_TEST_ACTIVITY, taskId);

        // Finish the task overlay activity and ensure that the PiP activity never got resumed.
        separateTestJournal();
        mBroadcastActionTrigger.doAction(TEST_ACTIVITY_ACTION_FINISH_SELF);
        mWmState.waitFor((amState) ->
                        !amState.containsActivity(TRANSLUCENT_TEST_ACTIVITY),
                "Waiting for test activity to finish...");
        final ActivityLifecycleCounts lifecycleCounts = new ActivityLifecycleCounts(PIP_ACTIVITY);
        assertEquals("onResume", 0, lifecycleCounts.getCount(ActivityCallback.ON_RESUME));
        assertEquals("onPause", 0, lifecycleCounts.getCount(ActivityCallback.ON_PAUSE));
    }

    @Test
    public void testTranslucentActivityOnTopOfPinnedTask() {
        launchActivity(LAUNCH_PIP_ON_PIP_ACTIVITY);
        // NOTE: moving to pinned stack will trigger the pip-on-pip activity to launch the
        // translucent activity.
        enterPipAndAssertPinnedTaskExists(LAUNCH_PIP_ON_PIP_ACTIVITY);
        mWmState.waitForValidState(
                new WaitForValidActivityState.Builder(ALWAYS_FOCUSABLE_PIP_ACTIVITY)
                        .setWindowingMode(WINDOWING_MODE_PINNED)
                        .build());

        assertPinnedStackIsOnTop();
        mWmState.assertVisibility(LAUNCH_PIP_ON_PIP_ACTIVITY, true);
        mWmState.assertVisibility(ALWAYS_FOCUSABLE_PIP_ACTIVITY, true);
    }

    @Test
    public void testLaunchTaskByComponentMatchMultipleTasks() {
        // Launch a fullscreen activity which will launch a PiP activity in a new task with the same
        // affinity
        launchActivity(TEST_ACTIVITY_WITH_SAME_AFFINITY);
        launchActivity(PIP_ACTIVITY_WITH_SAME_AFFINITY);
        assertPinnedStackExists();

        // Launch the root activity again...
        int rootActivityTaskId = mWmState.getTaskByActivity(
                TEST_ACTIVITY_WITH_SAME_AFFINITY).mTaskId;
        launchHomeActivity();
        launchActivity(TEST_ACTIVITY_WITH_SAME_AFFINITY);

        // ...and ensure that the root activity task is found and reused, and that the pinned stack
        // is unaffected
        assertPinnedStackExists();
        mWmState.assertFocusedActivity("Expected root activity focused",
                TEST_ACTIVITY_WITH_SAME_AFFINITY);
        assertEquals(rootActivityTaskId, mWmState.getTaskByActivity(
                TEST_ACTIVITY_WITH_SAME_AFFINITY).mTaskId);
    }

    @Test
    public void testLaunchTaskByAffinityMatchMultipleTasks() {
        // Launch a fullscreen activity which will launch a PiP activity in a new task with the same
        // affinity, and also launch another activity in the same task, while finishing itself. As
        // a result, the task will not have a component matching the same activity as what it was
        // started with
        launchActivityNoWait(TEST_ACTIVITY_WITH_SAME_AFFINITY,
                extraString(EXTRA_START_ACTIVITY, getActivityName(TEST_ACTIVITY)),
                extraString(EXTRA_FINISH_SELF_ON_RESUME, "true"));
        mWmState.waitForValidState(new WaitForValidActivityState.Builder(TEST_ACTIVITY)
                .setWindowingMode(WINDOWING_MODE_FULLSCREEN)
                .setActivityType(ACTIVITY_TYPE_STANDARD)
                .build());
        launchActivityNoWait(PIP_ACTIVITY_WITH_SAME_AFFINITY);
        waitForEnterPip(PIP_ACTIVITY_WITH_SAME_AFFINITY);
        assertPinnedStackExists();

        // Launch the root activity again...
        int rootActivityTaskId = mWmState.getTaskByActivity(
                TEST_ACTIVITY).mTaskId;
        launchHomeActivity();
        launchActivityNoWait(TEST_ACTIVITY_WITH_SAME_AFFINITY);
        mWmState.computeState();

        // ...and ensure that even while matching purely by task affinity, the root activity task is
        // found and reused, and that the pinned stack is unaffected
        assertPinnedStackExists();
        mWmState.assertFocusedActivity("Expected root activity focused", TEST_ACTIVITY);
        assertEquals(rootActivityTaskId, mWmState.getTaskByActivity(
                TEST_ACTIVITY).mTaskId);
    }

    @Test
    public void testLaunchTaskByAffinityMatchSingleTask() {
        // Launch an activity into the pinned stack with a fixed affinity
        launchActivityNoWait(TEST_ACTIVITY_WITH_SAME_AFFINITY,
                extraString(EXTRA_ENTER_PIP, "true"),
                extraString(EXTRA_START_ACTIVITY, getActivityName(PIP_ACTIVITY)),
                extraString(EXTRA_FINISH_SELF_ON_RESUME, "true"));
        waitForEnterPip(PIP_ACTIVITY);
        assertPinnedStackExists();

        // Launch the root activity again, of the matching task and ensure that we expand to
        // fullscreen
        int activityTaskId = mWmState.getTaskByActivity(PIP_ACTIVITY).mTaskId;
        launchHomeActivity();
        launchActivityNoWait(TEST_ACTIVITY_WITH_SAME_AFFINITY);
        waitForExitPipToFullscreen(PIP_ACTIVITY);
        assertPinnedStackDoesNotExist();
        assertEquals(activityTaskId, mWmState.getTaskByActivity(
                PIP_ACTIVITY).mTaskId);
    }

    /** Test that reported display size corresponds to fullscreen after exiting PiP. */
    @Test
    public void testDisplayMetricsPinUnpin() {
        separateTestJournal();
        launchActivity(TEST_ACTIVITY);
        final int defaultWindowingMode = mWmState
                .getTaskByActivity(TEST_ACTIVITY).getWindowingMode();
        final SizeInfo initialSizes = getLastReportedSizesForActivity(TEST_ACTIVITY);
        final Rect initialAppBounds = getAppBounds(TEST_ACTIVITY);
        assertNotNull("Must report display dimensions", initialSizes);
        assertNotNull("Must report app bounds", initialAppBounds);

        separateTestJournal();
        launchActivity(PIP_ACTIVITY, extraString(EXTRA_ENTER_PIP, "true"));
        // Wait for animation complete since we are comparing bounds
        waitForEnterPipAnimationComplete(PIP_ACTIVITY);
        final SizeInfo pinnedSizes = getLastReportedSizesForActivity(PIP_ACTIVITY);
        final Rect pinnedAppBounds = getAppBounds(PIP_ACTIVITY);
        assertNotEquals("Reported display size when pinned must be different from default",
                initialSizes, pinnedSizes);
        final Size initialAppSize = new Size(initialAppBounds.width(), initialAppBounds.height());
        final Size pinnedAppSize = new Size(pinnedAppBounds.width(), pinnedAppBounds.height());
        assertNotEquals("Reported app size when pinned must be different from default",
                initialAppSize, pinnedAppSize);

        separateTestJournal();
        launchActivity(PIP_ACTIVITY, defaultWindowingMode);
        final SizeInfo finalSizes = getLastReportedSizesForActivity(PIP_ACTIVITY);
        final Rect finalAppBounds = getAppBounds(PIP_ACTIVITY);
        final Size finalAppSize = new Size(finalAppBounds.width(), finalAppBounds.height());
        assertEquals("Must report default size after exiting PiP", initialSizes, finalSizes);
        assertEquals("Must report default app size after exiting PiP", initialAppSize,
                finalAppSize);
    }

    @Test
    public void testAutoPipAllowedBypassesExplicitEnterPip() {
        // Launch a test activity so that we're not over home.
        launchActivity(TEST_ACTIVITY);

        // Launch the PIP activity and set its pip params to allow auto-pip.
        launchActivity(PIP_ACTIVITY, extraString(EXTRA_ALLOW_AUTO_PIP, "true"));
        assertPinnedStackDoesNotExist();

        // Go home and ensure that there is a pinned stack.
        launchHomeActivity();
        waitForEnterPip(PIP_ACTIVITY);
        assertPinnedStackExists();
    }

    @Test
    public void testMaxNumberOfActions() {
        final int maxNumberActions = ActivityTaskManager.getMaxNumPictureInPictureActions(mContext);
        assertThat(maxNumberActions, greaterThanOrEqualTo(3));
    }

    @Test
    public void testFillMaxAllowedActions() {
        final int maxNumberActions = ActivityTaskManager.getMaxNumPictureInPictureActions(mContext);
        // Launch the PIP activity with max allowed actions
        launchActivity(PIP_ACTIVITY,
                extraString(EXTRA_NUMBER_OF_CUSTOM_ACTIONS, String.valueOf(maxNumberActions)));
        enterPipAndAssertPinnedTaskExists(PIP_ACTIVITY);

        assertNumberOfActions(PIP_ACTIVITY, maxNumberActions);
    }

    @Test
    public void testRejectExceededActions() {
        final int maxNumberActions = ActivityTaskManager.getMaxNumPictureInPictureActions(mContext);
        // Launch the PIP activity with exceeded amount of actions
        launchActivity(PIP_ACTIVITY,
                extraString(EXTRA_NUMBER_OF_CUSTOM_ACTIONS, String.valueOf(maxNumberActions + 1)));
        enterPipAndAssertPinnedTaskExists(PIP_ACTIVITY);

        assertNumberOfActions(PIP_ACTIVITY, maxNumberActions);
    }

    @Test
    public void testIsSeamlessResizeEnabledDefaultToTrue() {
        // Launch the PIP activity with some random param without setting isSeamlessResizeEnabled
        // so the PictureInPictureParams acquired from TaskInfo is not null
        launchActivity(PIP_ACTIVITY,
                extraString(EXTRA_NUMBER_OF_CUSTOM_ACTIONS, String.valueOf(1)));
        enterPipAndAssertPinnedTaskExists(PIP_ACTIVITY);

        // Assert the default value of isSeamlessResizeEnabled is set to true.
        assertIsSeamlessResizeEnabled(PIP_ACTIVITY, true);
    }

    @Test
    public void testDisableIsSeamlessResizeEnabled() {
        // Launch the PIP activity with overridden isSeamlessResizeEnabled param
        launchActivity(PIP_ACTIVITY, extraBool(EXTRA_IS_SEAMLESS_RESIZE_ENABLED, false));
        enterPipAndAssertPinnedTaskExists(PIP_ACTIVITY);

        // Assert the value of isSeamlessResizeEnabled is overridden.
        assertIsSeamlessResizeEnabled(PIP_ACTIVITY, false);
    }

    @Test
    public void testPictureInPictureStateChangeCallback() throws Exception {
        launchActivity(PIP_ACTIVITY);
        enterPipAndAssertPinnedTaskExists(PIP_ACTIVITY);
        waitForEnterPip(PIP_ACTIVITY);

        final CompletableFuture<Boolean> callbackReturn = new CompletableFuture<>();
        RemoteCallback cb = new RemoteCallback((Bundle result) ->
                callbackReturn.complete(result.getBoolean(PIP_CALLBACK_RESULT_KEY)));
        mBroadcastActionTrigger.sendPipStateUpdate(cb, true);
        Truth.assertThat(callbackReturn.get(5000, TimeUnit.MILLISECONDS)).isEqualTo(true);

        final CompletableFuture<Boolean> callbackReturnNotStashed = new CompletableFuture<>();
        RemoteCallback cbStashed = new RemoteCallback((Bundle result) ->
                callbackReturnNotStashed.complete(result.getBoolean(PIP_CALLBACK_RESULT_KEY)));
        mBroadcastActionTrigger.sendPipStateUpdate(cbStashed, false);
        Truth.assertThat(callbackReturnNotStashed.get(5000, TimeUnit.MILLISECONDS))
                .isEqualTo(false);
    }

    private void assertIsSeamlessResizeEnabled(ComponentName componentName, boolean expected) {
        runWithShellPermission(() -> {
            final ActivityTask task = mWmState.getTaskByActivity(componentName);
            final TaskInfo info = mTaskOrganizer.getTaskInfo(task.getTaskId());
            final PictureInPictureParams params = info.getPictureInPictureParams();

            assertEquals(expected, params.isSeamlessResizeEnabled());
        });
    }

    private void assertNumberOfActions(ComponentName componentName, int numberOfActions) {
        runWithShellPermission(() -> {
            final ActivityTask task = mWmState.getTaskByActivity(componentName);
            final TaskInfo info = mTaskOrganizer.getTaskInfo(task.getTaskId());
            final PictureInPictureParams params = info.getPictureInPictureParams();

            assertNotNull(params);
            assertNotNull(params.getActions());
            assertEquals(params.getActions().size(), numberOfActions);
        });
    }

    private void enterPipAndAssertPinnedTaskExists(ComponentName activityName) {
        mBroadcastActionTrigger.doAction(ACTION_ENTER_PIP);
        waitForEnterPip(activityName);
        assertPinnedStackExists();
    }

    /** Get app bounds in last applied configuration. */
    private Rect getAppBounds(ComponentName activityName) {
        final Configuration config = TestJournalContainer.get(activityName).extras
                .getParcelable(EXTRA_CONFIGURATION);
        if (config != null) {
            return config.windowConfiguration.getAppBounds();
        }
        return null;
    }

    /**
     * Called after the given {@param activityName} has been moved to the back stack, which follows
     * the activity's previous windowing mode. Ensures that the stack matching the
     * {@param windowingMode} and {@param activityType} is focused, and checks PIP activity is now
     * properly stopped and now belongs to a stack of {@param previousWindowingMode}.
     */
    private void assertPinnedStackStateOnMoveToBackStack(ComponentName activityName,
            int windowingMode, int activityType, int previousWindowingMode) {
        mWmState.waitForFocusedStack(windowingMode, activityType);
        mWmState.assertFocusedStack("Wrong focused stack", windowingMode, activityType);
        waitAndAssertActivityState(activityName, STATE_STOPPED,
                "Activity should go to STOPPED");
        assertTrue(mWmState.containsActivityInWindowingMode(
                activityName, previousWindowingMode));
        assertPinnedStackDoesNotExist();
    }

    /**
     * Asserts that the pinned stack bounds is contained in the display bounds.
     */
    private void assertPinnedStackActivityIsInDisplayBounds(ComponentName activityName) {
        final WindowManagerState.WindowState windowState = getWindowState(activityName);
        final WindowManagerState.DisplayContent display = mWmState.getDisplay(
                windowState.getDisplayId());
        final Rect displayRect = display.getDisplayRect();
        final Rect pinnedStackBounds = getPinnedStackBounds();
        assertTrue(displayRect.contains(pinnedStackBounds));
    }

    private int getDefaultDisplayWindowingMode(ComponentName activityName) {
        ActivityTask activityTask = mWmState.getTaskByActivity(activityName);
        return mWmState.getDisplay(activityTask.mDisplayId)
                .getWindowingMode();
    }

    /**
     * Asserts that the pinned stack exists.
     */
    private void assertPinnedStackExists() {
        mWmState.assertContainsStack("Must contain pinned stack.", WINDOWING_MODE_PINNED,
                ACTIVITY_TYPE_STANDARD);
    }

    /**
     * Asserts that the pinned stack does not exist.
     */
    private void assertPinnedStackDoesNotExist() {
        mWmState.assertDoesNotContainStack("Must not contain pinned stack.",
                WINDOWING_MODE_PINNED, ACTIVITY_TYPE_STANDARD);
    }

    /**
     * Asserts that the pinned stack is the front stack.
     */
    private void assertPinnedStackIsOnTop() {
        mWmState.assertFrontStack("Pinned stack must always be on top.",
                WINDOWING_MODE_PINNED, ACTIVITY_TYPE_STANDARD);
    }

    /**
     * Asserts that the activity received exactly one of each of the callbacks when entering and
     * exiting picture-in-picture.
     */
    private void assertValidPictureInPictureCallbackOrder(ComponentName activityName,
            int windowingMode) {
        final ActivityLifecycleCounts lifecycles = new ActivityLifecycleCounts(activityName);
        // There might be one additional config change caused by smallest screen width change when
        // there are cutout areas on the left & right edges of the display.
        assertThat(getActivityName(activityName) +
                        " onConfigurationChanged() shouldn't be triggered more than 2 times",
                lifecycles.getCount(ActivityCallback.ON_CONFIGURATION_CHANGED),
                lessThanOrEqualTo(2));
        assertEquals(getActivityName(activityName) + " onMultiWindowModeChanged",
                windowingMode == WINDOWING_MODE_FULLSCREEN ? 1 : 0,
                lifecycles.getCount(ActivityCallback.ON_MULTI_WINDOW_MODE_CHANGED));
        assertEquals(getActivityName(activityName) + " onPictureInPictureModeChanged()",
                1, lifecycles.getCount(ActivityCallback.ON_PICTURE_IN_PICTURE_MODE_CHANGED));
        final int lastPipIndex = lifecycles
                .getLastIndex(ActivityCallback.ON_PICTURE_IN_PICTURE_MODE_CHANGED);
        final int lastConfigIndex = lifecycles
                .getLastIndex(ActivityCallback.ON_CONFIGURATION_CHANGED);
        // In the case of Freeform, there's no onMultiWindowModeChange callback, so we will only
        // check for that callback for Fullscreen
        if (windowingMode == WINDOWING_MODE_FULLSCREEN) {
            final int lastMwIndex = lifecycles
                    .getLastIndex(ActivityCallback.ON_MULTI_WINDOW_MODE_CHANGED);
            assertThat("onPictureInPictureModeChanged should be before onMultiWindowModeChanged",
                    lastPipIndex, lessThan(lastMwIndex));
            assertThat("onMultiWindowModeChanged should be before onConfigurationChanged",
                    lastMwIndex, lessThan(lastConfigIndex));
        } else {
            assertThat("onPictureInPictureModeChanged should be before onConfigurationChanged",
                    lastPipIndex, lessThan(lastConfigIndex));
        }
    }

    /**
     * Waits until the given activity has entered picture-in-picture mode (allowing for the
     * subsequent animation to start).
     */
    private void waitForEnterPip(ComponentName activityName) {
        mWmState.waitForWithAmState(wmState -> {
            ActivityTask task = wmState.getTaskByActivity(activityName);
            return task != null && task.getWindowingMode() == WINDOWING_MODE_PINNED;
        }, "checking task windowing mode");
    }

    /**
     * Waits until the picture-in-picture animation has finished.
     */
    private void waitForEnterPipAnimationComplete(ComponentName activityName) {
        waitForEnterPip(activityName);
        mWmState.waitForWithAmState(wmState -> {
            ActivityTask task = wmState.getTaskByActivity(activityName);
            if (task == null) {
                return false;
            }
            WindowManagerState.Activity activity = task.getActivity(activityName);
            return activity.getWindowingMode() == WINDOWING_MODE_PINNED
                    && activity.getState().equals(STATE_PAUSED);
        }, "checking activity windowing mode");
    }

    /**
     * Waits until the pinned stack has been removed.
     */
    private void waitForPinnedStackRemoved() {
        mWmState.waitFor((amState) ->
                !amState.containsStack(WINDOWING_MODE_PINNED, ACTIVITY_TYPE_STANDARD),
                "pinned stack to be removed");
    }

    /**
     * Waits until the picture-in-picture animation to fullscreen has finished.
     */
    private void waitForExitPipToFullscreen(ComponentName activityName) {
        mWmState.waitForWithAmState(wmState -> {
            final ActivityTask task = wmState.getTaskByActivity(activityName);
            if (task == null) {
                return false;
            }
            final WindowManagerState.Activity activity = task.getActivity(activityName);
            return activity.getWindowingMode() != WINDOWING_MODE_PINNED;
        }, "checking activity windowing mode");
        mWmState.waitForWithAmState(wmState -> {
            final ActivityTask task = wmState.getTaskByActivity(activityName);
            return task != null && task.getWindowingMode() != WINDOWING_MODE_PINNED;
        }, "checking task windowing mode");
    }

    /**
     * Waits until the expected picture-in-picture callbacks have been made.
     */
    private void waitForValidPictureInPictureCallbacks(ComponentName activityName) {
        mWmState.waitFor((amState) -> {
            final ActivityLifecycleCounts lifecycles = new ActivityLifecycleCounts(activityName);
            return lifecycles.getCount(ActivityCallback.ON_CONFIGURATION_CHANGED) == 1
                    && lifecycles.getCount(ActivityCallback.ON_PICTURE_IN_PICTURE_MODE_CHANGED) == 1
                    && lifecycles.getCount(ActivityCallback.ON_MULTI_WINDOW_MODE_CHANGED) == 1;
        }, "picture-in-picture activity callbacks...");
    }

    private void waitForValidAspectRatio(int num, int denom) {
        // Hacky, but we need to wait for the auto-enter picture-in-picture animation to complete
        // and before we can check the pinned stack bounds
        mWmState.waitForWithAmState((state) -> {
            Rect bounds = state.getStandardStackByWindowingMode(WINDOWING_MODE_PINNED).getBounds();
            return floatEquals((float) bounds.width() / bounds.height(), (float) num / denom);
        }, "valid aspect ratio");
    }

    /**
     * @return the window state for the given {@param activityName}'s window.
     */
    private WindowManagerState.WindowState getWindowState(ComponentName activityName) {
        String windowName = getWindowName(activityName);
        mWmState.computeState(activityName);
        final List<WindowManagerState.WindowState> tempWindowList =
                mWmState.getMatchingVisibleWindowState(windowName);
        return tempWindowList.get(0);
    }

    /**
     * @return the current pinned stack.
     */
    private ActivityTask getPinnedStack() {
        return mWmState.getStandardStackByWindowingMode(WINDOWING_MODE_PINNED);
    }

    /**
     * @return the current pinned stack bounds.
     */
    private Rect getPinnedStackBounds() {
        return getPinnedStack().getBounds();
    }

    /**
     * Compares two floats with a common epsilon.
     */
    private void assertFloatEquals(float actual, float expected) {
        if (!floatEquals(actual, expected)) {
            fail(expected + " not equal to " + actual);
        }
    }

    private boolean floatEquals(float a, float b) {
        return Math.abs(a - b) < FLOAT_COMPARE_EPSILON;
    }

    /**
     * Triggers a tap over the pinned stack bounds to trigger the PIP to close.
     */
    private void tapToFinishPip() {
        Rect pinnedStackBounds = getPinnedStackBounds();
        int tapX = pinnedStackBounds.left + pinnedStackBounds.width() - 100;
        int tapY = pinnedStackBounds.top + pinnedStackBounds.height() - 100;
        tapOnDisplaySync(tapX, tapY, DEFAULT_DISPLAY);
    }

    /**
     * Launches the given {@param activityName} into the {@param taskId} as a task overlay.
     */
    private void launchPinnedActivityAsTaskOverlay(ComponentName activityName, int taskId) {
        executeShellCommand(getAmStartCmd(activityName) + " --task " + taskId + " --task-overlay");

        mWmState.waitForValidState(new WaitForValidActivityState.Builder(activityName)
                .setWindowingMode(WINDOWING_MODE_PINNED)
                .setActivityType(ACTIVITY_TYPE_STANDARD)
                .build());
    }

    private static class AppOpsSession implements AutoCloseable {

        private final String mPackageName;

        AppOpsSession(ComponentName activityName) {
            mPackageName = activityName.getPackageName();
        }

        /**
         * Sets an app-ops op for a given package to a given mode.
         */
        void setOpToMode(String op, int mode) {
            try {
                AppOpsUtils.setOpMode(mPackageName, op, mode);
            } catch (Exception e) {
                e.printStackTrace();
            }
        }

        @Override
        public void close() {
            try {
                AppOpsUtils.reset(mPackageName);
            } catch (IOException e) {
                e.printStackTrace();
            }
        }
    }

    /**
     * TODO: Improve tests check to actually check that apps are not interactive instead of checking
     *       if the stack is focused.
     */
    private void pinnedStackTester(String startActivityCmd, ComponentName startActivity,
            ComponentName topActivityName, boolean isFocusable) {
        executeShellCommand(startActivityCmd);
        mWmState.waitForValidState(startActivity);

        mWmState.waitForValidState(new WaitForValidActivityState.Builder(topActivityName)
                .setWindowingMode(WINDOWING_MODE_PINNED)
                .setActivityType(ACTIVITY_TYPE_STANDARD)
                .build());
        mWmState.computeState();

        if (supportsPip()) {
            final String windowName = getWindowName(topActivityName);
            assertPinnedStackExists();
            mWmState.assertFrontStack("Pinned stack must be the front stack.",
                    WINDOWING_MODE_PINNED, ACTIVITY_TYPE_STANDARD);
            mWmState.assertVisibility(topActivityName, true);

            if (isFocusable) {
                mWmState.assertFocusedStack("Pinned stack must be the focused stack.",
                        WINDOWING_MODE_PINNED, ACTIVITY_TYPE_STANDARD);
                mWmState.assertFocusedActivity(
                        "Pinned activity must be focused activity.", topActivityName);
                mWmState.assertFocusedWindow(
                        "Pinned window must be focused window.", windowName);
                // Not checking for resumed state here because PiP overlay can be launched on top
                // in different task by SystemUI.
            } else {
                // Don't assert that the stack is not focused as a focusable PiP overlay can be
                // launched on top as a task overlay by SystemUI.
                mWmState.assertNotFocusedActivity(
                        "Pinned activity can't be the focused activity.", topActivityName);
                mWmState.assertNotResumedActivity(
                        "Pinned activity can't be the resumed activity.", topActivityName);
                mWmState.assertNotFocusedWindow(
                        "Pinned window can't be focused window.", windowName);
            }
        } else {
            mWmState.assertDoesNotContainStack("Must not contain pinned stack.",
                    WINDOWING_MODE_PINNED, ACTIVITY_TYPE_STANDARD);
        }
    }
}
