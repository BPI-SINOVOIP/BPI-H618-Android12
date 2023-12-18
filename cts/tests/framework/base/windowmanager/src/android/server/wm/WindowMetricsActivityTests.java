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

package android.server.wm;

import static android.app.WindowConfiguration.WINDOWING_MODE_FREEFORM;
import static android.app.WindowConfiguration.WINDOWING_MODE_FULLSCREEN;
import static android.app.WindowConfiguration.WINDOWING_MODE_MULTI_WINDOW;
import static android.app.WindowConfiguration.WINDOWING_MODE_PINNED;
import static android.server.wm.WindowManagerState.STATE_PAUSED;
import static android.server.wm.WindowMetricsTestHelper.assertBoundsMatchDisplay;
import static android.server.wm.WindowMetricsTestHelper.getBoundsExcludingNavigationBarAndCutout;
import static android.server.wm.WindowMetricsTestHelper.maxWindowBoundsSandboxed;
import static android.view.WindowManager.LayoutParams.LAYOUT_IN_DISPLAY_CUTOUT_MODE_ALWAYS;

import static org.junit.Assert.assertEquals;
import static org.junit.Assert.assertTrue;
import static org.junit.Assert.fail;
import static org.junit.Assume.assumeTrue;

import android.app.Activity;
import android.app.PictureInPictureParams;
import android.content.ComponentName;
import android.content.Context;
import android.graphics.Point;
import android.graphics.Rect;
import android.os.Bundle;
import android.platform.test.annotations.Presubmit;
import android.server.wm.WindowMetricsTestHelper.OnLayoutChangeListener;
import android.view.Display;
import android.view.WindowManager;
import android.view.WindowMetrics;

import androidx.test.filters.FlakyTest;

import org.junit.Test;

/**
 * Tests that verify the behavior of {@link WindowMetrics} APIs on {@link Activity activities}.
 *
 * Build/Install/Run:
 *     atest CtsWindowManagerDeviceTestCases:WindowMetricsActivityTests
 */
@Presubmit
public class WindowMetricsActivityTests extends WindowManagerTestBase {
    private static final Rect WINDOW_BOUNDS = new Rect(100, 100, 400, 400);
    private static final Rect RESIZED_WINDOW_BOUNDS = new Rect(100, 100, 900, 900);
    private static final int MOVE_OFFSET = 100;

    @Test
    public void testMetricsMatchesLayoutOnActivityOnCreate() {
        final MetricsActivity activity = startActivityInWindowingMode(MetricsActivity.class,
                WINDOWING_MODE_FULLSCREEN);
        final OnLayoutChangeListener listener = activity.mListener;

        listener.waitForLayout();

        WindowMetricsTestHelper.assertMetricsMatchesLayout(activity.mOnCreateCurrentMetrics,
                activity.mOnCreateMaximumMetrics, listener.getLayoutBounds(),
                listener.getLayoutInsets());
    }

    @Test
    public void testMetricsMatchesDisplayAreaOnActivity() {
        final MetricsActivity activity = startActivityInWindowingMode(MetricsActivity.class,
                WINDOWING_MODE_FULLSCREEN);

        assertMetricsValidity(activity);
    }

    @Test
    @FlakyTest(bugId = 188207199)
    public void testMetricsMatchesActivityBoundsOnNonresizableActivity() {
        assumeTrue("Skipping test: no rotation support", supportsRotation());

        final MinAspectRatioActivity activity = startActivityInWindowingMode(
                MinAspectRatioActivity.class, WINDOWING_MODE_FULLSCREEN);
        mWmState.computeState(activity.getComponentName());

        assertMetricsValidityForNonresizableActivity(activity);
    }

    @Test
    public void testMetricsMatchesLayoutOnPipActivity() {
        assumeTrue(supportsPip());

        final MetricsActivity activity = startActivityInWindowingMode(MetricsActivity.class,
                WINDOWING_MODE_FULLSCREEN);

        assertMetricsMatchesLayout(activity);

        activity.enterPictureInPictureMode(new PictureInPictureParams.Builder().build());
        waitForEnterPipAnimationComplete(activity.getComponentName());

        assertMetricsMatchesLayout(activity);
    }

    @Test
    public void testMetricsMatchesDisplayAreaOnPipActivity() {
        assumeTrue(supportsPip());

        final MetricsActivity activity = startActivityInWindowingMode(MetricsActivity.class,
                WINDOWING_MODE_FULLSCREEN);

        assertMetricsValidity(activity);

        activity.enterPictureInPictureMode(new PictureInPictureParams.Builder().build());
        waitForEnterPipAnimationComplete(activity.getComponentName());

        assertMetricsValidity(activity);
    }

    /**
     * Waits until the picture-in-picture animation has finished.
     */
    private void waitForEnterPipAnimationComplete(ComponentName activityName) {
        waitForEnterPip(activityName);
        mWmState.waitForWithAmState(wmState -> {
            WindowManagerState.ActivityTask task = wmState.getTaskByActivity(activityName);
            if (task == null) {
                return false;
            }
            WindowManagerState.Activity activity = task.getActivity(activityName);
            return activity.getWindowingMode() == WINDOWING_MODE_PINNED
                    && activity.getState().equals(STATE_PAUSED);
        }, "checking activity windowing mode");
    }

    /**
     * Waits until the given activity has entered picture-in-picture mode (allowing for the
     * subsequent animation to start).
     */
    private void waitForEnterPip(ComponentName activityName) {
        mWmState.waitForWithAmState(wmState -> {
            WindowManagerState.ActivityTask task = wmState.getTaskByActivity(activityName);
            return task != null && task.getWindowingMode() == WINDOWING_MODE_PINNED;
        }, "checking task windowing mode");
    }

    @Test
    public void testMetricsMatchesLayoutOnSplitActivity() {
        assumeTrue(supportsSplitScreenMultiWindow());

        final MetricsActivity activity = startActivityInWindowingMode(MetricsActivity.class,
                WINDOWING_MODE_FULLSCREEN);

        assertMetricsMatchesLayout(activity);

        mWmState.computeState(activity.getComponentName());
        putActivityInPrimarySplit(activity.getComponentName());

        mWmState.computeState(activity.getComponentName());
        assertTrue(mWmState.getActivity(activity.getComponentName()).getWindowingMode()
                == WINDOWING_MODE_MULTI_WINDOW);

        assertMetricsMatchesLayout(activity);
    }

    @Test
    public void testMetricsMatchesDisplayAreaOnSplitActivity() {
        assumeTrue(supportsSplitScreenMultiWindow());

        final MetricsActivity activity = startActivityInWindowingMode(MetricsActivity.class,
                WINDOWING_MODE_FULLSCREEN);

        assertMetricsValidity(activity);

        mWmState.computeState(activity.getComponentName());
        putActivityInPrimarySplit(activity.getComponentName());

        mWmState.computeState(activity.getComponentName());
        assertTrue(mWmState.getActivity(activity.getComponentName()).getWindowingMode()
                == WINDOWING_MODE_MULTI_WINDOW);

        assertMetricsValidity(activity);
    }

    @Test
    public void testMetricsMatchesLayoutOnFreeformActivity() {
        assumeTrue(supportsFreeform());

        final MetricsActivity activity = startActivityInWindowingMode(MetricsActivity.class,
                WINDOWING_MODE_FULLSCREEN);

        assertMetricsMatchesLayout(activity);

        launchActivity(new ComponentName(mTargetContext, MetricsActivity.class),
                WINDOWING_MODE_FREEFORM);

        // Resize the freeform activity.
        resizeActivityTask(activity.getComponentName(), WINDOW_BOUNDS.left, WINDOW_BOUNDS.top,
                WINDOW_BOUNDS.right, WINDOW_BOUNDS.bottom);
        mWmState.computeState(activity.getComponentName());

        assertMetricsMatchesLayout(activity);

        // Resize again.
        resizeActivityTask(activity.getComponentName(), RESIZED_WINDOW_BOUNDS.left,
                RESIZED_WINDOW_BOUNDS.top, RESIZED_WINDOW_BOUNDS.right,
                RESIZED_WINDOW_BOUNDS.bottom);
        mWmState.computeState(activity.getComponentName());

        assertMetricsMatchesLayout(activity);

        // Move the activity.
        resizeActivityTask(activity.getComponentName(), MOVE_OFFSET + RESIZED_WINDOW_BOUNDS.left,
                MOVE_OFFSET + RESIZED_WINDOW_BOUNDS.top, MOVE_OFFSET + RESIZED_WINDOW_BOUNDS.right,
                MOVE_OFFSET + RESIZED_WINDOW_BOUNDS.bottom);
        mWmState.computeState(activity.getComponentName());

        assertMetricsMatchesLayout(activity);
    }

    @Test
    public void testMetricsMatchesDisplayAreaOnFreeformActivity() {
        assumeTrue(supportsFreeform());

        final MetricsActivity activity = startActivityInWindowingMode(MetricsActivity.class,
                WINDOWING_MODE_FULLSCREEN);

        assertMetricsValidity(activity);

        launchActivity(new ComponentName(mTargetContext, MetricsActivity.class),
                WINDOWING_MODE_FREEFORM);

        // Resize the freeform activity.
        resizeActivityTask(activity.getComponentName(), WINDOW_BOUNDS.left, WINDOW_BOUNDS.top,
                WINDOW_BOUNDS.right, WINDOW_BOUNDS.bottom);

        assertMetricsValidity(activity);

        // Resize again.
        resizeActivityTask(activity.getComponentName(), RESIZED_WINDOW_BOUNDS.left,
                RESIZED_WINDOW_BOUNDS.top, RESIZED_WINDOW_BOUNDS.right,
                RESIZED_WINDOW_BOUNDS.bottom);

        assertMetricsValidity(activity);

        // Move the activity.
        resizeActivityTask(activity.getComponentName(), MOVE_OFFSET + RESIZED_WINDOW_BOUNDS.left,
                MOVE_OFFSET + RESIZED_WINDOW_BOUNDS.top, MOVE_OFFSET + RESIZED_WINDOW_BOUNDS.right,
                MOVE_OFFSET + RESIZED_WINDOW_BOUNDS.bottom);

        assertMetricsValidity(activity);
    }

    private static void assertMetricsMatchesLayout(MetricsActivity activity) {
        final OnLayoutChangeListener listener = activity.mListener;
        listener.waitForLayout();

        final WindowMetrics currentMetrics = activity.getWindowManager().getCurrentWindowMetrics();
        final WindowMetrics maxMetrics = activity.getWindowManager().getMaximumWindowMetrics();

        Condition.waitFor(new Condition<>("WindowMetrics must match layout metrics",
                () -> currentMetrics.getBounds().equals(listener.getLayoutBounds()))
                .setRetryIntervalMs(500).setRetryLimit(10)
                .setOnFailure(unused -> fail("WindowMetrics must match layout metrics. Layout"
                        + "bounds is" + listener.getLayoutBounds() + ", while current window"
                        + "metrics is " + currentMetrics.getBounds())));

        final boolean isFreeForm = activity.getResources().getConfiguration().windowConfiguration
                .getWindowingMode() == WINDOWING_MODE_FREEFORM;
        WindowMetricsTestHelper.assertMetricsMatchesLayout(currentMetrics, maxMetrics,
                listener.getLayoutBounds(), listener.getLayoutInsets(), isFreeForm);
    }

    /**
     * Verifies two scenarios for an {@link Activity}. If the activity is freeform, then the bounds
     * should not include insets for navigation bar and cutout area.
     * <ul>
     *     <li>{@link WindowManager#getCurrentWindowMetrics()} matches
     *     {@link Display#getSize(Point)}</li>
     *     <li>{@link WindowManager#getMaximumWindowMetrics()} and {@link Display#getSize(Point)}
     *     either matches DisplayArea bounds which the {@link Activity} is attached to, or matches
     *     {@link WindowManager#getCurrentWindowMetrics()} if sandboxing is applied.</li>
     * </ul>
     */
    private void assertMetricsValidity(Activity activity) {
        mWmState.computeState(activity.getComponentName());
        WindowMetricsTestHelper.assertMetricsValidity(activity,
                getTaskDisplayAreaBounds(activity.getComponentName()));
    }

    /**
     * Verifies two scenarios for a non-resizable {@link Activity}. Similar to
     * {@link #assertMetricsValidity(Activity)}, verifies the values of window metrics against
     * Display size. {@link WindowManager#getMaximumWindowMetrics()} must match activity bounds
     * if the activity is sandboxed.
     *
     * App bounds calculation of a nonresizable activity depends on the orientation of the display
     * and the app, and the display and activity bounds. Directly compare maximum WindowMetrics
     * against the value used for sandboxing, since other ways of accessing activity bounds may
     * have different insets applied.
     *
     * @param activity the activity under test
     */
    private void assertMetricsValidityForNonresizableActivity(Activity activity) {
        ComponentName activityName = activity.getComponentName();
        mWmState.computeState(activityName);
        WindowManagerState.Activity activityContainer = mWmState.getActivity(activityName);
        final boolean boundsShouldIncludeInsets =
                activity.getResources().getConfiguration().windowConfiguration
                        .getWindowingMode() == WINDOWING_MODE_FREEFORM
                        || activityContainer.inSizeCompatMode();
        final WindowManager windowManager = activity.getWindowManager();
        final WindowMetrics currentMetrics = windowManager.getCurrentWindowMetrics();
        final Rect currentBounds = boundsShouldIncludeInsets ? currentMetrics.getBounds()
                : getBoundsExcludingNavigationBarAndCutout(currentMetrics);
        final Rect maxBounds = windowManager.getMaximumWindowMetrics().getBounds();
        final Display display = activity.getDisplay();

        assertBoundsMatchDisplay(maxBounds, currentBounds, display);

        // Max window bounds should match either DisplayArea bounds, or activity bounds if it is
        // sandboxed.
        final Rect displayAreaBounds = getTaskDisplayAreaBounds(activityName);
        final Rect activityBounds =
                activityContainer.mFullConfiguration.windowConfiguration.getBounds();
        if (maxWindowBoundsSandboxed(displayAreaBounds, maxBounds)) {
            // Max window bounds are sandboxed, so max window bounds should match activity bounds.
            assertEquals("Maximum window metrics of a non-resizable activity matches "
                    + "activity bounds, when sandboxed", activityBounds, maxBounds);
        } else {
            // Max window bounds are not sandboxed, so max window bounds should match display area
            // bounds.
            assertEquals("Display area bounds must match max window size", displayAreaBounds,
                    maxBounds);
        }
    }

    private Rect getTaskDisplayAreaBounds(ComponentName activityName) {
        WindowManagerState.DisplayArea tda = mWmState.getTaskDisplayArea(activityName);
        return tda.mFullConfiguration.windowConfiguration.getBounds();
    }

    public static class MetricsActivity extends FocusableActivity {
        private WindowMetrics mOnCreateMaximumMetrics;
        private WindowMetrics mOnCreateCurrentMetrics;
        private final OnLayoutChangeListener mListener = new OnLayoutChangeListener();

        @Override
        protected void onCreate(Bundle savedInstanceState) {
            super.onCreate(savedInstanceState);
            mOnCreateCurrentMetrics = getWindowManager().getCurrentWindowMetrics();
            mOnCreateMaximumMetrics = getWindowManager().getMaximumWindowMetrics();
            getWindow().getDecorView().addOnLayoutChangeListener(mListener);

            // Always extend the cutout areas because layout doesn't get the waterfall cutout.
            final WindowManager.LayoutParams attrs = getWindow().getAttributes();
            attrs.layoutInDisplayCutoutMode = LAYOUT_IN_DISPLAY_CUTOUT_MODE_ALWAYS;
            getWindow().setAttributes(attrs);
        }

        @Override
        protected void onDestroy() {
            super.onDestroy();
            getWindow().getDecorView().removeOnLayoutChangeListener(mListener);
        }
    }

    public static class MinAspectRatioActivity extends MetricsActivity {
    }
}
