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

import static android.graphics.PixelFormat.TRANSLUCENT;
import static android.view.KeyEvent.ACTION_DOWN;
import static android.view.KeyEvent.KEYCODE_BACK;
import static android.view.View.SYSTEM_UI_FLAG_FULLSCREEN;
import static android.view.View.SYSTEM_UI_FLAG_HIDE_NAVIGATION;
import static android.view.View.SYSTEM_UI_FLAG_IMMERSIVE;
import static android.view.View.SYSTEM_UI_FLAG_LOW_PROFILE;
import static android.view.WindowInsets.Type.ime;
import static android.view.WindowInsets.Type.navigationBars;
import static android.view.WindowInsets.Type.statusBars;
import static android.view.WindowInsets.Type.systemBars;
import static android.view.WindowInsets.Type.systemGestures;
import static android.view.WindowInsetsController.BEHAVIOR_DEFAULT;
import static android.view.WindowInsetsController.BEHAVIOR_SHOW_TRANSIENT_BARS_BY_SWIPE;
import static android.view.WindowManager.LayoutParams.FLAG_ALT_FOCUSABLE_IM;
import static android.view.WindowManager.LayoutParams.FLAG_FULLSCREEN;
import static android.view.WindowManager.LayoutParams.FLAG_NOT_FOCUSABLE;
import static android.view.WindowManager.LayoutParams.LAYOUT_IN_DISPLAY_CUTOUT_MODE_ALWAYS;
import static android.view.WindowManager.LayoutParams.SOFT_INPUT_STATE_HIDDEN;
import static android.view.WindowManager.LayoutParams.TYPE_APPLICATION;

import static androidx.test.platform.app.InstrumentationRegistry.getInstrumentation;

import static com.android.cts.mockime.ImeEventStreamTestUtils.editorMatcher;
import static com.android.cts.mockime.ImeEventStreamTestUtils.expectEvent;

import static org.hamcrest.Matchers.is;
import static org.hamcrest.Matchers.notNullValue;
import static org.hamcrest.Matchers.nullValue;
import static org.junit.Assert.assertEquals;
import static org.junit.Assert.assertFalse;
import static org.junit.Assert.assertTrue;
import static org.junit.Assume.assumeFalse;
import static org.junit.Assume.assumeThat;
import static org.junit.Assume.assumeTrue;

import android.app.Activity;
import android.app.AlertDialog;
import android.app.Instrumentation;
import android.content.Context;
import android.content.pm.PackageManager;
import android.os.Bundle;
import android.os.SystemClock;
import android.platform.test.annotations.Presubmit;
import android.view.MotionEvent;
import android.view.View;
import android.view.Window;
import android.view.WindowInsets;
import android.view.WindowInsetsAnimation;
import android.view.WindowInsetsController;
import android.view.WindowManager;
import android.widget.EditText;
import android.widget.LinearLayout;
import android.widget.TextView;

import androidx.annotation.Nullable;

import com.android.compatibility.common.util.PollingCheck;
import com.android.compatibility.common.util.SystemUtil;
import com.android.cts.mockime.ImeEventStream;
import com.android.cts.mockime.ImeSettings;
import com.android.cts.mockime.MockImeSession;

import org.junit.Rule;
import org.junit.Test;
import org.junit.rules.ErrorCollector;

import java.util.ArrayList;
import java.util.List;
import java.util.concurrent.CountDownLatch;
import java.util.concurrent.TimeUnit;

/**
 * Test whether WindowInsetsController controls window insets as expected.
 *
 * Build/Install/Run:
 *     atest CtsWindowManagerDeviceTestCases:WindowInsetsControllerTests
 */
@Presubmit
public class WindowInsetsControllerTests extends WindowManagerTestBase {

    private final static long TIMEOUT = 1000; // milliseconds
    private final static long TIMEOUT_UPDATING_INPUT_WINDOW = 500; // milliseconds
    private final static long TIME_SLICE = 50; // milliseconds
    private final static AnimationCallback ANIMATION_CALLBACK = new AnimationCallback();

    private static final String AM_BROADCAST_CLOSE_SYSTEM_DIALOGS =
            "am broadcast -a android.intent.action.CLOSE_SYSTEM_DIALOGS";

    @Rule
    public final ErrorCollector mErrorCollector = new ErrorCollector();

    @Test
    public void testHide() {
        final TestActivity activity = startActivity(TestActivity.class);
        final View rootView = activity.getWindow().getDecorView();

        testHideInternal(rootView, statusBars());
        testHideInternal(rootView, navigationBars());
    }

    private void testHideInternal(View rootView, int types) {
        if (rootView.getRootWindowInsets().isVisible(types)) {
            getInstrumentation().runOnMainSync(() -> {
                rootView.getWindowInsetsController().hide(types);
            });
            PollingCheck.waitFor(TIMEOUT, () -> !rootView.getRootWindowInsets().isVisible(types));
        }
    }

    @Test
    public void testShow() {
        final TestActivity activity = startActivity(TestActivity.class);
        final View rootView = activity.getWindow().getDecorView();

        testShowInternal(rootView, statusBars());
        testShowInternal(rootView, navigationBars());
    }

    private void testShowInternal(View rootView, int types) {
        if (rootView.getRootWindowInsets().isVisible(types)) {
            getInstrumentation().runOnMainSync(() -> {
                rootView.getWindowInsetsController().hide(types);
            });
            PollingCheck.waitFor(TIMEOUT, () -> !rootView.getRootWindowInsets().isVisible(types));
            getInstrumentation().runOnMainSync(() -> {
                rootView.getWindowInsetsController().show(types);
            });
            PollingCheck.waitFor(TIMEOUT, () -> rootView.getRootWindowInsets().isVisible(types));
        }
    }

    private void testTopAppHidesStatusBarInternal(Activity activity, View rootView,
            Runnable hidingStatusBar) {
        if (rootView.getRootWindowInsets().isVisible(statusBars())) {

            // The top-fullscreen-app window hides status bar.
            getInstrumentation().runOnMainSync(hidingStatusBar);
            PollingCheck.waitFor(TIMEOUT,
                    () -> !rootView.getRootWindowInsets().isVisible(statusBars()));

            // Add a non-fullscreen window on top of the fullscreen window.
            // The new focused window doesn't hide status bar.
            getInstrumentation().runOnMainSync(
                    () -> activity.getWindowManager().addView(
                            new View(activity),
                            new WindowManager.LayoutParams(1 /* w */, 1 /* h */, TYPE_APPLICATION,
                                    0 /* flags */, TRANSLUCENT)));

            // Check if status bar stays invisible.
            for (long time = TIMEOUT; time >= 0; time -= TIME_SLICE) {
                assertFalse(rootView.getRootWindowInsets().isVisible(statusBars()));
                SystemClock.sleep(TIME_SLICE);
            }
        }
    }

    @Test
    public void testTopAppHidesStatusBarByMethod() {
        final TestActivity activity = startActivity(TestActivity.class);
        final View rootView = activity.getWindow().getDecorView();

        testTopAppHidesStatusBarInternal(activity, rootView,
                () -> rootView.getWindowInsetsController().hide(statusBars()));
    }

    @Test
    public void testTopAppHidesStatusBarByWindowFlag() {
        final TestActivity activity = startActivity(TestActivity.class);
        final View rootView = activity.getWindow().getDecorView();

        testTopAppHidesStatusBarInternal(activity, rootView,
                () -> activity.getWindow().addFlags(FLAG_FULLSCREEN));
    }

    @Test
    public void testTopAppHidesStatusBarBySystemUiFlag() {
        final TestActivity activity = startActivity(TestActivity.class);
        final View rootView = activity.getWindow().getDecorView();

        testTopAppHidesStatusBarInternal(activity, rootView,
                () -> rootView.setSystemUiVisibility(SYSTEM_UI_FLAG_FULLSCREEN));
    }

    @Test
    public void testImeShowAndHide() throws Exception {
        final Instrumentation instrumentation = getInstrumentation();
        assumeThat(MockImeSession.getUnavailabilityReason(instrumentation.getContext()),
                nullValue());
        final MockImeSession imeSession = MockImeHelper.createManagedMockImeSession(this);
        final ImeEventStream stream = imeSession.openEventStream();
        final TestActivity activity = startActivity(TestActivity.class);
        expectEvent(stream, editorMatcher("onStartInput", activity.mEditTextMarker), TIMEOUT);

        final View rootView = activity.getWindow().getDecorView();
        getInstrumentation().runOnMainSync(() -> rootView.getWindowInsetsController().show(ime()));
        PollingCheck.waitFor(TIMEOUT, () -> rootView.getRootWindowInsets().isVisible(ime()));
        getInstrumentation().runOnMainSync(() -> rootView.getWindowInsetsController().hide(ime()));
        PollingCheck.waitFor(TIMEOUT, () -> !rootView.getRootWindowInsets().isVisible(ime()));
    }

    @Test
    public void testImeForceShowingNavigationBar() throws Exception {
        final Instrumentation instrumentation = getInstrumentation();
        assumeThat(MockImeSession.getUnavailabilityReason(instrumentation.getContext()),
                nullValue());
        final MockImeSession imeSession = MockImeHelper.createManagedMockImeSession(this);
        final ImeEventStream stream = imeSession.openEventStream();
        final TestActivity activity = startActivity(TestActivity.class);
        expectEvent(stream, editorMatcher("onStartInput", activity.mEditTextMarker), TIMEOUT);

        final View rootView = activity.getWindow().getDecorView();
        assumeTrue(rootView.getRootWindowInsets().isVisible(navigationBars()));
        getInstrumentation().runOnMainSync(
                () -> rootView.getWindowInsetsController().hide(navigationBars()));
        PollingCheck.waitFor(TIMEOUT,
                () -> !rootView.getRootWindowInsets().isVisible(navigationBars()));
        getInstrumentation().runOnMainSync(() -> rootView.getWindowInsetsController().show(ime()));
        PollingCheck.waitFor(TIMEOUT,
                () -> rootView.getRootWindowInsets().isVisible(ime() | navigationBars()));
        getInstrumentation().runOnMainSync(() -> rootView.getWindowInsetsController().hide(ime()));
        PollingCheck.waitFor(TIMEOUT,
                () -> !rootView.getRootWindowInsets().isVisible(ime())
                        && !rootView.getRootWindowInsets().isVisible(navigationBars()));
    }

    @Test
    public void testSetSystemBarsBehavior_default() throws InterruptedException {
        final TestActivity activity = startActivity(TestActivity.class);
        final View rootView = activity.getWindow().getDecorView();

        // Assume we have the bars and they can be visible.
        final int types = statusBars();
        assumeTrue(rootView.getRootWindowInsets().isVisible(types));

        rootView.getWindowInsetsController().setSystemBarsBehavior(BEHAVIOR_DEFAULT);

        hideInsets(rootView, types);

        // Tapping on display cannot show bars.
        tapOnDisplay(rootView.getWidth() / 2f, rootView.getHeight() / 2f);
        PollingCheck.waitFor(TIMEOUT, () -> !rootView.getRootWindowInsets().isVisible(types));

        // Wait for status bar invisible from InputDispatcher. Otherwise, the following
        // dragFromTopToCenter might expand notification shade.
        SystemClock.sleep(TIMEOUT_UPDATING_INPUT_WINDOW);

        // Swiping from top of display can show bars.
        dragFromTopToCenter(rootView);
        PollingCheck.waitFor(TIMEOUT, () -> rootView.getRootWindowInsets().isVisible(types));
    }

    @Test
    public void testSetSystemBarsBehavior_showTransientBarsBySwipe() throws InterruptedException {
        final TestActivity activity = startActivity(TestActivity.class);
        final View rootView = activity.getWindow().getDecorView();

        // Assume we have the bars and they can be visible.
        final int types = statusBars();
        assumeTrue(rootView.getRootWindowInsets().isVisible(types));

        rootView.getWindowInsetsController().setSystemBarsBehavior(
                BEHAVIOR_SHOW_TRANSIENT_BARS_BY_SWIPE);

        hideInsets(rootView, types);

        // Tapping on display cannot show bars.
        tapOnDisplay(rootView.getWidth() / 2f, rootView.getHeight() / 2f);
        PollingCheck.waitFor(TIMEOUT, () -> !rootView.getRootWindowInsets().isVisible(types));

        // Wait for status bar invisible from InputDispatcher. Otherwise, the following
        // dragFromTopToCenter might expand notification shade.
        SystemClock.sleep(TIMEOUT_UPDATING_INPUT_WINDOW);

        // Swiping from top of display can show transient bars, but apps cannot detect that.
        dragFromTopToCenter(rootView);
        // Make sure status bar stays invisible.
        for (long time = TIMEOUT; time >= 0; time -= TIME_SLICE) {
            assertFalse(rootView.getRootWindowInsets().isVisible(types));
            SystemClock.sleep(TIME_SLICE);
        }
    }

    @Test
    public void testSetSystemBarsBehavior_systemGesture_default() throws InterruptedException {
        final TestActivity activity = startActivity(TestActivity.class);
        final View rootView = activity.getWindow().getDecorView();

        // Assume the current navigation mode has the back gesture.
        assumeTrue(rootView.getRootWindowInsets().getInsets(systemGestures()).left > 0);
        assumeTrue(canTriggerBackGesture(rootView));

        rootView.getWindowInsetsController().setSystemBarsBehavior(BEHAVIOR_DEFAULT);
        hideInsets(rootView, systemBars());

        // Test if the back gesture can be triggered while system bars are hidden with the behavior.
        assertTrue(canTriggerBackGesture(rootView));
    }

    @Test
    public void testSetSystemBarsBehavior_systemGesture_showTransientBarsBySwipe()
            throws InterruptedException {
        final TestActivity activity = startActivity(TestActivity.class);
        final View rootView = activity.getWindow().getDecorView();

        // Assume the current navigation mode has the back gesture.
        assumeTrue(rootView.getRootWindowInsets().getInsets(systemGestures()).left > 0);
        assumeTrue(canTriggerBackGesture(rootView));

        rootView.getWindowInsetsController().setSystemBarsBehavior(
                BEHAVIOR_SHOW_TRANSIENT_BARS_BY_SWIPE);
        hideInsets(rootView, systemBars());

        // Test if the back gesture can be triggered while system bars are hidden with the behavior.
        assertFalse(canTriggerBackGesture(rootView));
    }

    private boolean canTriggerBackGesture(View rootView) throws InterruptedException {
        final boolean[] hasBack = { false };
        final CountDownLatch latch = new CountDownLatch(1);
        rootView.findFocus().setOnKeyListener((v, keyCode, event) -> {
            if (keyCode == KEYCODE_BACK && event.getAction() == ACTION_DOWN) {
                hasBack[0] = true;
                latch.countDown();
                return true;
            }
            return false;
        });
        dragFromLeftToCenter(rootView);
        latch.await(1, TimeUnit.SECONDS);
        return hasBack[0];
    }

    @Test
    public void testSystemUiVisibilityCallbackCausedByInsets() {
        final TestActivity activity = startActivity(TestActivity.class);
        final View controlTarget = activity.getWindow().getDecorView();
        final int[] targetSysUiVis = new int[1];
        final View nonControlTarget = new View(mTargetContext);
        final int[] nonTargetSysUiVis = new int[1];
        final WindowManager.LayoutParams nonTargetAttrs =
                new WindowManager.LayoutParams(TYPE_APPLICATION);
        nonTargetAttrs.flags = FLAG_NOT_FOCUSABLE;
        getInstrumentation().runOnMainSync(() -> {
            controlTarget.setOnSystemUiVisibilityChangeListener(
                    visibility -> targetSysUiVis[0] = visibility);
            nonControlTarget.setOnSystemUiVisibilityChangeListener(
                    visibility -> nonTargetSysUiVis[0] = visibility);
            activity.getWindowManager().addView(nonControlTarget, nonTargetAttrs);
        });
        waitForIdle();
        testSysUiVisCallbackCausedByInsets(statusBars(), SYSTEM_UI_FLAG_FULLSCREEN,
                controlTarget, targetSysUiVis, nonTargetSysUiVis);
        testSysUiVisCallbackCausedByInsets(navigationBars(), SYSTEM_UI_FLAG_HIDE_NAVIGATION,
                controlTarget, targetSysUiVis, nonTargetSysUiVis);
    }

    private void testSysUiVisCallbackCausedByInsets(int insetsType, int sysUiFlag, View target,
            int[] targetSysUiVis, int[] nonTargetSysUiVis) {
        if (target.getRootWindowInsets().isVisible(insetsType)) {

            // Controlled by methods
            getInstrumentation().runOnMainSync(
                    () -> target.getWindowInsetsController().hide(insetsType));
            PollingCheck.waitFor(TIMEOUT, () ->
                    targetSysUiVis[0] == sysUiFlag && targetSysUiVis[0] == nonTargetSysUiVis[0]);
            getInstrumentation().runOnMainSync(
                    () -> target.getWindowInsetsController().show(insetsType));
            PollingCheck.waitFor(TIMEOUT, () ->
                    targetSysUiVis[0] == 0 && targetSysUiVis[0] == nonTargetSysUiVis[0]);

            // Controlled by legacy flags
            getInstrumentation().runOnMainSync(
                    () -> target.setSystemUiVisibility(sysUiFlag));
            PollingCheck.waitFor(TIMEOUT, () ->
                    targetSysUiVis[0] == sysUiFlag && targetSysUiVis[0] == nonTargetSysUiVis[0]);
            getInstrumentation().runOnMainSync(
                    () -> target.setSystemUiVisibility(0));
            PollingCheck.waitFor(TIMEOUT, () ->
                    targetSysUiVis[0] == 0 && targetSysUiVis[0] == nonTargetSysUiVis[0]);
        }
    }

    @Test
    public void testSystemUiVisibilityCallbackCausedByAppearance() {
        final TestActivity activity = startActivity(TestActivity.class);
        final View controlTarget = activity.getWindow().getDecorView();
        final int[] targetSysUiVis = new int[1];
        getInstrumentation().runOnMainSync(() -> {
            controlTarget.setOnSystemUiVisibilityChangeListener(
                    visibility -> targetSysUiVis[0] = visibility);
        });
        waitForIdle();
        final int sysUiFlag = SYSTEM_UI_FLAG_LOW_PROFILE;
        getInstrumentation().runOnMainSync(() -> controlTarget.setSystemUiVisibility(sysUiFlag));
        PollingCheck.waitFor(TIMEOUT, () -> targetSysUiVis[0] == sysUiFlag);
        getInstrumentation().runOnMainSync(() -> controlTarget.setSystemUiVisibility(0));
        PollingCheck.waitFor(TIMEOUT, () -> targetSysUiVis[0] == 0);
    }

    @Test
    public void testSetSystemUiVisibilityAfterCleared_showBarsBySwipe() throws Exception {
        final TestActivity activity = startActivity(TestActivity.class);
        final View rootView = activity.getWindow().getDecorView();

        // Assume we have the bars and they can be visible.
        final int types = statusBars();
        assumeTrue(rootView.getRootWindowInsets().isVisible(types));

        final int targetFlags = SYSTEM_UI_FLAG_IMMERSIVE | SYSTEM_UI_FLAG_FULLSCREEN;

        // Use flags to hide status bar.
        ANIMATION_CALLBACK.reset();
        getInstrumentation().runOnMainSync(() -> {
            rootView.setWindowInsetsAnimationCallback(ANIMATION_CALLBACK);
            rootView.setSystemUiVisibility(targetFlags);
        });
        ANIMATION_CALLBACK.waitForFinishing();
        PollingCheck.waitFor(TIMEOUT, () -> !rootView.getRootWindowInsets().isVisible(types));

        // Wait for status bar invisible from InputDispatcher. Otherwise, the following
        // dragFromTopToCenter might expand notification shade.
        SystemClock.sleep(TIMEOUT_UPDATING_INPUT_WINDOW);

        // Swiping from top of display can show bars.
        ANIMATION_CALLBACK.reset();
        dragFromTopToCenter(rootView);
        ANIMATION_CALLBACK.waitForFinishing();
        PollingCheck.waitFor(TIMEOUT, () -> rootView.getRootWindowInsets().isVisible(types)
            && rootView.getSystemUiVisibility() != targetFlags);

        // Use flags to hide status bar again.
        ANIMATION_CALLBACK.reset();
        getInstrumentation().runOnMainSync(() -> {
            rootView.setWindowInsetsAnimationCallback(ANIMATION_CALLBACK);
            rootView.setSystemUiVisibility(targetFlags);
        });
        ANIMATION_CALLBACK.waitForFinishing();
        PollingCheck.waitFor(TIMEOUT, () -> !rootView.getRootWindowInsets().isVisible(types));

        // Wait for status bar invisible from InputDispatcher. Otherwise, the following
        // dragFromTopToCenter might expand notification shade.
        SystemClock.sleep(TIMEOUT_UPDATING_INPUT_WINDOW);

        // Swiping from top of display can show bars.
        ANIMATION_CALLBACK.reset();
        dragFromTopToCenter(rootView);
        ANIMATION_CALLBACK.waitForFinishing();
        PollingCheck.waitFor(TIMEOUT, () -> rootView.getRootWindowInsets().isVisible(types));

        // The swipe action brings down the notification shade which causes subsequent tests to
        // fail.
        if (isAutomotive(mContext)) {
            // Bring system to a known state before requesting to close system dialogs.
            launchHomeActivity();
            broadcastCloseSystemDialogs();
        }
    }

    @Test
    public void testSetSystemUiVisibilityAfterCleared_showBarsByApp() throws Exception {
        final TestActivity activity = startActivity(TestActivity.class);
        final View rootView = activity.getWindow().getDecorView();

        // Assume we have the bars and they can be visible.
        final int types = statusBars();
        assumeTrue(rootView.getRootWindowInsets().isVisible(types));

        // Use the flag to hide status bar.
        ANIMATION_CALLBACK.reset();
        getInstrumentation().runOnMainSync(() -> {
            rootView.setWindowInsetsAnimationCallback(ANIMATION_CALLBACK);
            rootView.setSystemUiVisibility(SYSTEM_UI_FLAG_FULLSCREEN);
        });
        ANIMATION_CALLBACK.waitForFinishing();
        PollingCheck.waitFor(TIMEOUT, () -> !rootView.getRootWindowInsets().isVisible(types));

        // Clearing the flag can show status bar.
        getInstrumentation().runOnMainSync(() -> {
            rootView.setSystemUiVisibility(0);
        });
        PollingCheck.waitFor(TIMEOUT, () -> rootView.getRootWindowInsets().isVisible(types));

        // Use the flag to hide status bar again.
        ANIMATION_CALLBACK.reset();
        getInstrumentation().runOnMainSync(() -> {
            rootView.setWindowInsetsAnimationCallback(ANIMATION_CALLBACK);
            rootView.setSystemUiVisibility(SYSTEM_UI_FLAG_FULLSCREEN);
        });
        ANIMATION_CALLBACK.waitForFinishing();
        PollingCheck.waitFor(TIMEOUT, () -> !rootView.getRootWindowInsets().isVisible(types));

        // Clearing the flag can show status bar.
        getInstrumentation().runOnMainSync(() -> {
            rootView.setSystemUiVisibility(0);
        });
        PollingCheck.waitFor(TIMEOUT, () -> rootView.getRootWindowInsets().isVisible(types));
    }

    @Test
    public void testHideOnCreate() throws Exception {
        final TestHideOnCreateActivity activity = startActivity(TestHideOnCreateActivity.class);
        final View rootView = activity.getWindow().getDecorView();
        ANIMATION_CALLBACK.waitForFinishing();
        PollingCheck.waitFor(TIMEOUT,
                () -> !rootView.getRootWindowInsets().isVisible(statusBars())
                        && !rootView.getRootWindowInsets().isVisible(navigationBars()));
    }

    @Test
    public void testShowImeOnCreate() throws Exception {
        final Instrumentation instrumentation = getInstrumentation();
        assumeThat(MockImeSession.getUnavailabilityReason(instrumentation.getContext()),
                nullValue());
        MockImeHelper.createManagedMockImeSession(this);
        final TestShowOnCreateActivity activity = startActivity(TestShowOnCreateActivity.class);
        final View rootView = activity.getWindow().getDecorView();
        ANIMATION_CALLBACK.waitForFinishing();
        PollingCheck.waitFor(TIMEOUT, () -> rootView.getRootWindowInsets().isVisible(ime()));
    }

    @Test
    public void testShowImeOnCreate_doesntCauseImeToReappearWhenDialogIsShown() throws Exception {
        final Instrumentation instrumentation = getInstrumentation();
        assumeThat(MockImeSession.getUnavailabilityReason(instrumentation.getContext()),
                nullValue());
        try (MockImeSession imeSession = MockImeSession.create(instrumentation.getContext(),
                instrumentation.getUiAutomation(), new ImeSettings.Builder())) {
            final TestShowOnCreateActivity activity = startActivity(TestShowOnCreateActivity.class);
            final View rootView = activity.getWindow().getDecorView();
            PollingCheck.waitFor(TIMEOUT,
                    () -> rootView.getRootWindowInsets().isVisible(ime()));
            ANIMATION_CALLBACK.waitForFinishing();
            ANIMATION_CALLBACK.reset();
            getInstrumentation().runOnMainSync(() ->  {
                rootView.getWindowInsetsController().hide(ime());
            });
            PollingCheck.waitFor(TIMEOUT,
                    () -> !rootView.getRootWindowInsets().isVisible(ime()));
            ANIMATION_CALLBACK.waitForFinishing();
            getInstrumentation().runOnMainSync(() ->  {
                activity.showAltImDialog();
            });

            for (long time = TIMEOUT; time >= 0; time -= TIME_SLICE) {
                assertFalse("IME visible when it shouldn't be",
                        rootView.getRootWindowInsets().isVisible(ime()));
                SystemClock.sleep(TIME_SLICE);
            }
        }
    }

    @Test
    public void testInsetsDispatch() throws Exception {
        // Start an activity which hides system bars.
        final TestHideOnCreateActivity activity = startActivity(TestHideOnCreateActivity.class);
        final View rootView = activity.getWindow().getDecorView();
        ANIMATION_CALLBACK.waitForFinishing();
        PollingCheck.waitFor(TIMEOUT,
                () -> !rootView.getRootWindowInsets().isVisible(statusBars())
                        && !rootView.getRootWindowInsets().isVisible(navigationBars()));

        // Add a dialog which hides system bars before the dialog is added to the system while the
        // system bar was hidden previously, and collect the window insets that the dialog receives.
        final ArrayList<WindowInsets> windowInsetsList = new ArrayList<>();
        getInstrumentation().runOnMainSync(() -> {
            final AlertDialog dialog = new AlertDialog.Builder(activity).create();
            final Window dialogWindow = dialog.getWindow();
            dialogWindow.getDecorView().setOnApplyWindowInsetsListener((view, insets) -> {
                windowInsetsList.add(insets);
                return view.onApplyWindowInsets(insets);
            });
            dialogWindow.getInsetsController().hide(statusBars() | navigationBars());
            dialog.show();
        });
        getInstrumentation().waitForIdleSync();

        // The dialog must never receive any of visible insets of system bars.
        for (WindowInsets windowInsets : windowInsetsList) {
            assertFalse(windowInsets.isVisible(statusBars()));
            assertFalse(windowInsets.isVisible(navigationBars()));
        }
    }

    @Test
    public void testWindowInsetsController_availableAfterAddView() throws Exception {
        final TestHideOnCreateActivity activity = startActivity(TestHideOnCreateActivity.class);
        final View rootView = activity.getWindow().getDecorView();
        ANIMATION_CALLBACK.waitForFinishing();
        PollingCheck.waitFor(TIMEOUT,
                () -> !rootView.getRootWindowInsets().isVisible(statusBars())
                        && !rootView.getRootWindowInsets().isVisible(navigationBars()));

        final View childWindow = new View(activity);
        getInstrumentation().runOnMainSync(() -> {
            activity.getWindowManager().addView(childWindow,
                    new WindowManager.LayoutParams(TYPE_APPLICATION));
            mErrorCollector.checkThat(childWindow.getWindowInsetsController(), is(notNullValue()));
        });
        getInstrumentation().waitForIdleSync();
        getInstrumentation().runOnMainSync(() -> {
            activity.getWindowManager().removeView(childWindow);
        });

    }

    @Test
    public void testDispatchApplyWindowInsetsCount_systemBars() throws InterruptedException {
        final TestActivity activity = startActivity(TestActivity.class);
        final View rootView = activity.getWindow().getDecorView();
        getInstrumentation().waitForIdleSync();

        // Assume we have at least one visible system bar.
        assumeTrue(rootView.getRootWindowInsets().isVisible(statusBars())
                || rootView.getRootWindowInsets().isVisible(navigationBars()));

        getInstrumentation().runOnMainSync(() -> {
            // This makes the window frame stable while changing the system bar visibility.
            final WindowManager.LayoutParams attrs = activity.getWindow().getAttributes();
            attrs.layoutInDisplayCutoutMode = LAYOUT_IN_DISPLAY_CUTOUT_MODE_ALWAYS;
            activity.getWindow().setAttributes(attrs);
        });
        getInstrumentation().waitForIdleSync();

        final int[] dispatchApplyWindowInsetsCount = {0};
        rootView.setOnApplyWindowInsetsListener((v, insets) -> {
            dispatchApplyWindowInsetsCount[0]++;
            return v.onApplyWindowInsets(insets);
        });

        // One hide-system-bar call...
        ANIMATION_CALLBACK.reset();
        getInstrumentation().runOnMainSync(() -> {
            rootView.setWindowInsetsAnimationCallback(ANIMATION_CALLBACK);
            rootView.getWindowInsetsController().hide(systemBars());
        });
        ANIMATION_CALLBACK.waitForFinishing();

        // ... should only trigger one dispatchApplyWindowInsets
        assertEquals(1, dispatchApplyWindowInsetsCount[0]);

        // One show-system-bar call...
        dispatchApplyWindowInsetsCount[0] = 0;
        ANIMATION_CALLBACK.reset();
        getInstrumentation().runOnMainSync(() -> {
            rootView.setWindowInsetsAnimationCallback(ANIMATION_CALLBACK);
            rootView.getWindowInsetsController().show(systemBars());
        });
        ANIMATION_CALLBACK.waitForFinishing();

        // ... should only trigger one dispatchApplyWindowInsets
        assertEquals(1, dispatchApplyWindowInsetsCount[0]);
    }

    @Test
    public void testDispatchApplyWindowInsetsCount_ime() throws Exception {
        assumeFalse("Automotive is to skip this test until showing and hiding certain insets "
                + "simultaneously in a single request is supported", isAutomotive(mContext));
        assumeThat(MockImeSession.getUnavailabilityReason(getInstrumentation().getContext()),
                nullValue());

        MockImeHelper.createManagedMockImeSession(this);
        final TestActivity activity = startActivity(TestActivity.class);
        final View rootView = activity.getWindow().getDecorView();
        getInstrumentation().waitForIdleSync();

        final int[] dispatchApplyWindowInsetsCount = {0};
        rootView.setOnApplyWindowInsetsListener((v, insets) -> {
            dispatchApplyWindowInsetsCount[0]++;
            return v.onApplyWindowInsets(insets);
        });

        // One show-ime call...
        ANIMATION_CALLBACK.reset();
        getInstrumentation().runOnMainSync(() -> {
            rootView.setWindowInsetsAnimationCallback(ANIMATION_CALLBACK);
            rootView.getWindowInsetsController().show(ime());
        });
        ANIMATION_CALLBACK.waitForFinishing();

        // ... should only trigger one dispatchApplyWindowInsets
        assertEquals(1, dispatchApplyWindowInsetsCount[0]);

        // One hide-ime call...
        dispatchApplyWindowInsetsCount[0] = 0;
        ANIMATION_CALLBACK.reset();
        getInstrumentation().runOnMainSync(() -> {
            rootView.setWindowInsetsAnimationCallback(ANIMATION_CALLBACK);
            rootView.getWindowInsetsController().hide(ime());
        });
        ANIMATION_CALLBACK.waitForFinishing();

        // ... should only trigger one dispatchApplyWindowInsets
        assertEquals(1, dispatchApplyWindowInsetsCount[0]);
    }

    private static void broadcastCloseSystemDialogs() {
        executeShellCommand(AM_BROADCAST_CLOSE_SYSTEM_DIALOGS);
    }

    private static boolean isAutomotive(Context context) {
        return context.getPackageManager().hasSystemFeature(PackageManager.FEATURE_AUTOMOTIVE);
    }

    private static void hideInsets(View view, int types) throws InterruptedException {
        ANIMATION_CALLBACK.reset();
        getInstrumentation().runOnMainSync(() -> {
            view.setWindowInsetsAnimationCallback(ANIMATION_CALLBACK);
            view.getWindowInsetsController().hide(types);
        });
        ANIMATION_CALLBACK.waitForFinishing();
        PollingCheck.waitFor(TIMEOUT, () -> !view.getRootWindowInsets().isVisible(types));
    }

    private void tapOnDisplay(float x, float y) {
        dragOnDisplay(x, y, x, y);
    }

    private void dragFromTopToCenter(View view) {
        dragOnDisplay(view.getWidth() / 2f, 0 /* downY */,
                view.getWidth() / 2f, view.getHeight() / 2f);
    }

    private void dragFromLeftToCenter(View view) {
        dragOnDisplay(0 /* downX */, view.getHeight() / 2f,
                view.getWidth() / 2f, view.getHeight() / 2f);
    }

    private void dragOnDisplay(float downX, float downY, float upX, float upY) {
        final long downTime = SystemClock.elapsedRealtime();

        // down event
        MotionEvent event = MotionEvent.obtain(downTime, downTime, MotionEvent.ACTION_DOWN,
                downX, downY, 0 /* metaState */);
        sendPointerSync(event);
        event.recycle();

        // move event
        event = MotionEvent.obtain(downTime, downTime + 1, MotionEvent.ACTION_MOVE,
                (downX + upX) / 2f, (downY + upY) / 2f, 0 /* metaState */);
        sendPointerSync(event);
        event.recycle();

        // up event
        event = MotionEvent.obtain(downTime, downTime + 2, MotionEvent.ACTION_UP,
                upX, upY, 0 /* metaState */);
        sendPointerSync(event);
        event.recycle();
    }

    private void sendPointerSync(MotionEvent event) {
        SystemUtil.runWithShellPermissionIdentity(
                () -> getInstrumentation().sendPointerSync(event));
    }

    private static class AnimationCallback extends WindowInsetsAnimation.Callback {

        private static final long ANIMATION_TIMEOUT = 5000; // milliseconds

        private boolean mFinished = false;

        AnimationCallback() {
            super(DISPATCH_MODE_CONTINUE_ON_SUBTREE);
        }

        @Override
        public WindowInsets onProgress(WindowInsets insets,
                List<WindowInsetsAnimation> runningAnimations) {
            return insets;
        }

        @Override
        public void onEnd(WindowInsetsAnimation animation) {
            synchronized (this) {
                mFinished = true;
                notify();
            }
        }

        void waitForFinishing() throws InterruptedException {
            synchronized (this) {
                if (!mFinished) {
                    wait(ANIMATION_TIMEOUT);
                }
            }
        }

        void reset() {
            synchronized (this) {
                mFinished = false;
            }
        }
    }

    private static View setViews(Activity activity, @Nullable String privateImeOptions) {
        LinearLayout layout = new LinearLayout(activity);
        View text = new TextView(activity);
        EditText editor = new EditText(activity);
        editor.setPrivateImeOptions(privateImeOptions);
        layout.addView(text);
        layout.addView(editor);
        activity.setContentView(layout);
        editor.requestFocus();
        return layout;
    }

    public static class TestActivity extends FocusableActivity {
        final String mEditTextMarker =
                getClass().getName() + "/" + SystemClock.elapsedRealtimeNanos();

        @Override
        protected void onCreate(Bundle savedInstanceState) {
            super.onCreate(savedInstanceState);
            setViews(this, mEditTextMarker);
            getWindow().setSoftInputMode(SOFT_INPUT_STATE_HIDDEN);
        }
    }

    public static class TestHideOnCreateActivity extends FocusableActivity {

        @Override
        protected void onCreate(Bundle savedInstanceState) {
            super.onCreate(savedInstanceState);
            View layout = setViews(this, null /* privateImeOptions */);
            ANIMATION_CALLBACK.reset();
            getWindow().getDecorView().setWindowInsetsAnimationCallback(ANIMATION_CALLBACK);
            getWindow().getInsetsController().hide(statusBars());
            layout.getWindowInsetsController().hide(navigationBars());
        }
    }

    public static class TestShowOnCreateActivity extends FocusableActivity {
        @Override
        protected void onCreate(Bundle savedInstanceState) {
            super.onCreate(savedInstanceState);
            setViews(this, null /* privateImeOptions */);
            ANIMATION_CALLBACK.reset();
            getWindow().getDecorView().setWindowInsetsAnimationCallback(ANIMATION_CALLBACK);
            getWindow().getInsetsController().show(ime());
        }

        void showAltImDialog() {
            AlertDialog dialog = new AlertDialog.Builder(this)
                    .setTitle("TestDialog")
                    .create();
            dialog.getWindow().addFlags(FLAG_ALT_FOCUSABLE_IM);
            dialog.show();
        }
    }
}
