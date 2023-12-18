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
 * limitations under the License
 */

package android.server.wm;

import static android.app.WindowConfiguration.WINDOWING_MODE_FULLSCREEN;
import static android.server.wm.WindowManagerState.STATE_STOPPED;
import static android.server.wm.app.Components.TEST_ACTIVITY;
import static android.server.wm.app.Components.TEST_DREAM_SERVICE;
import static android.server.wm.app.Components.TEST_STUBBORN_DREAM_SERVICE;
import static android.server.wm.ComponentNameUtils.getWindowName;

import static org.junit.Assert.assertFalse;
import static org.junit.Assert.assertTrue;
import static android.view.Display.DEFAULT_DISPLAY;

import static org.junit.Assume.assumeTrue;

import android.app.DreamManager;
import android.content.ComponentName;
import android.platform.test.annotations.Presubmit;
import android.provider.Settings;
import android.server.wm.app.Components;
import android.view.Surface;
import android.content.res.Resources;

import androidx.test.filters.FlakyTest;

import com.android.compatibility.common.util.SystemUtil;

import org.junit.After;
import org.junit.Before;
import org.junit.Test;

@Presubmit
@FlakyTest(detail = "Promote once confirmed non-flaky")
public class DreamManagerServiceTests extends ActivityManagerTestBase {

    // Timeout after which the dream should have finished willingly
    private static final long ACTIVITY_STOP_TIMEOUT = 3000;

    // Timeout after which the dream should have been forcefully stopped
    private static final long ACTIVITY_FORCE_STOP_TIMEOUT = 6500;

    private ComponentName mDreamActivityName;

    private boolean mDefaultDreamServiceEnabled = true;

    private static final ComponentName getDreamActivityName(ComponentName dream) {
        return new ComponentName(dream.getPackageName(),
                                 "android.service.dreams.DreamActivity");
    }

    @Before
    public void setup() {
        assumeTrue("Skipping test: no dream support", supportsDream());

        mDefaultDreamServiceEnabled =
                Settings.Secure.getInt(mContext.getContentResolver(),
                                "screensaver_enabled", 1) != 0;
        if (!mDefaultDreamServiceEnabled) {
            SystemUtil.runWithShellPermissionIdentity(() -> {
                Settings.Secure.putInt(mContext.getContentResolver(), "screensaver_enabled", 1);
            });
        }
    }

    @After
    public void reset()  {
        if (!mDefaultDreamServiceEnabled) {
            SystemUtil.runWithShellPermissionIdentity(() -> {
                Settings.Secure.putInt(mContext.getContentResolver(), "screensaver_enabled", 0);
            });
        }
    }

    private void startDream(ComponentName name) {
        DreamManager dreamer = mContext.getSystemService(DreamManager.class);
        SystemUtil.runWithShellPermissionIdentity(() -> {
            dreamer.startDream(name);
        });
    }

    private void stopDream() {
        DreamManager dreamer = mContext.getSystemService(DreamManager.class);
        SystemUtil.runWithShellPermissionIdentity(() -> {
            dreamer.stopDream();
        });
    }

    private void setActiveDream(ComponentName dream) {
        DreamManager dreamer = mContext.getSystemService(DreamManager.class);
        SystemUtil.runWithShellPermissionIdentity(() -> {
            dreamer.setActiveDream(dream);
        });
        mDreamActivityName = getDreamActivityName(dream);
    }

    private boolean getIsDreaming() {
        DreamManager dreamer = mContext.getSystemService(DreamManager.class);
        return SystemUtil.runWithShellPermissionIdentity(() -> {
            return dreamer.isDreaming();
        });
    }

    private boolean supportsDream() {
        return mContext.getResources().getBoolean(
                Resources.getSystem().getIdentifier("config_dreamsSupported", "bool", "android"));
    }

    private void assertDreamActivityGone() {
        mWmState.computeState();
        assertTrue(!mWmState.containsWindow(getWindowName(mDreamActivityName))
                   && !mWmState.containsActivity(mDreamActivityName));
    }

    private void startFullscreenTestActivity() {
        launchActivity(TEST_ACTIVITY, WINDOWING_MODE_FULLSCREEN);
        waitAndAssertTopResumedActivity(TEST_ACTIVITY, DEFAULT_DISPLAY,
                "Test activity should be the top resumed activity");
        mWmState.assertVisibility(TEST_ACTIVITY, true);
    }

    @Test
    public void testStartAndStopDream() throws Exception {
        startFullscreenTestActivity();
        setActiveDream(TEST_DREAM_SERVICE);

        startDream(TEST_DREAM_SERVICE);
        waitAndAssertTopResumedActivity(mDreamActivityName, DEFAULT_DISPLAY,
                "Dream activity should be the top resumed activity");
        mWmState.waitForValidState(mWmState.getHomeActivityName());
        mWmState.assertVisibility(mWmState.getHomeActivityName(), false);
        mWmState.waitForValidState(TEST_ACTIVITY);
        mWmState.assertVisibility(TEST_ACTIVITY, false);

        assertTrue(getIsDreaming());

        stopDream();
        mWmState.waitAndAssertActivityRemoved(mDreamActivityName);

        waitAndAssertTopResumedActivity(TEST_ACTIVITY, DEFAULT_DISPLAY,
                "Previous top activity should show when dream is stopped");
    }

    @Test
    public void testDreamServiceStopsTimely() throws Exception {
        setActiveDream(TEST_DREAM_SERVICE);

        startDream(TEST_DREAM_SERVICE);
        waitAndAssertTopResumedActivity(mDreamActivityName, DEFAULT_DISPLAY,
                "Dream activity should be the top resumed activity");
        mWmState.waitForValidState(mWmState.getHomeActivityName());
        mWmState.assertVisibility(mWmState.getHomeActivityName(), false);
        assertTrue(getIsDreaming());

        stopDream();

        Thread.sleep(ACTIVITY_STOP_TIMEOUT);

        assertDreamActivityGone();
        assertFalse(getIsDreaming());
    }

    @Test
    public void testForceStopStubbornDream() throws Exception {
        startFullscreenTestActivity();
        setActiveDream(TEST_STUBBORN_DREAM_SERVICE);

        startDream(TEST_STUBBORN_DREAM_SERVICE);
        waitAndAssertTopResumedActivity(mDreamActivityName, DEFAULT_DISPLAY,
                "Dream activity should be the top resumed activity");
        mWmState.waitForValidState(mWmState.getHomeActivityName());
        mWmState.assertVisibility(mWmState.getHomeActivityName(), false);
        mWmState.waitForValidState(TEST_ACTIVITY);
        mWmState.assertVisibility(TEST_ACTIVITY, false);

        stopDream();

        Thread.sleep(ACTIVITY_FORCE_STOP_TIMEOUT);

        assertDreamActivityGone();
        assertFalse(getIsDreaming());
        waitAndAssertTopResumedActivity(TEST_ACTIVITY, DEFAULT_DISPLAY,
                "Previous top activity should show when dream is stopped");
    }

    @Test
    public void testDreamNotFinishAfterRotation() {
        assumeTrue("Skipping test: no rotation support", supportsRotation());

        final RotationSession rotationSession = createManagedRotationSession();
        rotationSession.set(Surface.ROTATION_0);
        setActiveDream(TEST_DREAM_SERVICE);
        startDream(TEST_DREAM_SERVICE);
        rotationSession.set(Surface.ROTATION_90);

        waitAndAssertTopResumedActivity(mDreamActivityName, DEFAULT_DISPLAY,
                "Dream activity should be the top resumed activity");
    }

    @Test
    public void testStartActivityDoesNotWakeAndIsNotResumed() {
        try (DreamingState state = new DreamingState(TEST_DREAM_SERVICE)) {
            launchActivity(Components.TEST_ACTIVITY);
            mWmState.waitForActivityState(Components.TEST_ACTIVITY, STATE_STOPPED);
            assertTrue(getIsDreaming());
        }
    }

    @Test
    public void testStartTurnScreenOnActivityDoesWake() {
        try (DreamingState state = new DreamingState(TEST_DREAM_SERVICE)) {
            launchActivity(Components.TURN_SCREEN_ON_ACTIVITY);

            state.waitForDreamGone();
            waitAndAssertTopResumedActivity(Components.TURN_SCREEN_ON_ACTIVITY,
                    DEFAULT_DISPLAY, "TurnScreenOnActivity should resume through dream");
        }
    }

    @Test
    public void testStartTurnScreenOnAttrActivityDoesWake() {
        try (DreamingState state = new DreamingState(TEST_DREAM_SERVICE)) {
            launchActivity(Components.TURN_SCREEN_ON_ATTR_ACTIVITY);

            state.waitForDreamGone();
            waitAndAssertTopResumedActivity(Components.TURN_SCREEN_ON_ATTR_ACTIVITY,
                    DEFAULT_DISPLAY, "TurnScreenOnAttrActivity should resume through dream");
        }
    }

    @Test
    public void testStartActivityOnKeyguardLocked() {
        assumeTrue(supportsLockScreen());

        final LockScreenSession lockScreenSession = createManagedLockScreenSession();
        lockScreenSession.setLockCredential();
        try (DreamingState state = new DreamingState(TEST_DREAM_SERVICE)) {
            launchActivityNoWait(Components.TEST_ACTIVITY);
            waitAndAssertActivityState(Components.TEST_ACTIVITY, STATE_STOPPED,
                "Activity must be started and stopped");
            assertTrue(getIsDreaming());

            launchActivity(Components.TURN_SCREEN_ON_SHOW_ON_LOCK_ACTIVITY);
            state.waitForDreamGone();
            waitAndAssertTopResumedActivity(Components.TURN_SCREEN_ON_SHOW_ON_LOCK_ACTIVITY,
                    DEFAULT_DISPLAY, "TurnScreenOnShowOnLockActivity should resume through dream");
            assertFalse(getIsDreaming());
        }
    }

    private class DreamingState implements AutoCloseable {
        public DreamingState(ComponentName dream) {
            setActiveDream(dream);
            startDream(dream);
            waitAndAssertDreaming();
        }

        @Override
        public void close() {
            stopDream();
        }

        public void waitAndAssertDreaming() {
            waitAndAssertTopResumedActivity(mDreamActivityName, DEFAULT_DISPLAY,
                    "Dream activity should be the top resumed activity");
            mWmState.waitForValidState(mWmState.getHomeActivityName());
            mWmState.assertVisibility(mWmState.getHomeActivityName(), false);
            assertTrue(getIsDreaming());
        }

        public void waitForDreamGone() {
            mWmState.waitForDreamGone();
            assertFalse(getIsDreaming());
        }
    }
}
