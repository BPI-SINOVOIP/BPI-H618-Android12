/*
 * Copyright (C) 2017 The Android Open Source Project
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

import static android.app.WindowConfiguration.ACTIVITY_TYPE_ASSISTANT;
import static android.app.WindowConfiguration.ACTIVITY_TYPE_STANDARD;
import static android.app.WindowConfiguration.WINDOWING_MODE_FULLSCREEN;
import static android.app.WindowConfiguration.WINDOWING_MODE_PINNED;
import static android.app.WindowConfiguration.WINDOWING_MODE_UNDEFINED;
import static android.server.wm.CliIntentExtra.extraString;
import static android.server.wm.ComponentNameUtils.getActivityName;
import static android.server.wm.UiDeviceUtils.pressHomeButton;
import static android.server.wm.WindowManagerState.STATE_RESUMED;
import static android.server.wm.app.Components.ANIMATION_TEST_ACTIVITY;
import static android.server.wm.app.Components.ASSISTANT_ACTIVITY;
import static android.server.wm.app.Components.ASSISTANT_VOICE_INTERACTION_SERVICE;
import static android.server.wm.app.Components.AssistantActivity.EXTRA_ASSISTANT_DISPLAY_ID;
import static android.server.wm.app.Components.AssistantActivity.EXTRA_ASSISTANT_ENTER_PIP;
import static android.server.wm.app.Components.AssistantActivity.EXTRA_ASSISTANT_FINISH_SELF;
import static android.server.wm.app.Components.AssistantActivity.EXTRA_ASSISTANT_LAUNCH_NEW_TASK;
import static android.server.wm.app.Components.DOCKED_ACTIVITY;
import static android.server.wm.app.Components.LAUNCH_ASSISTANT_ACTIVITY_FROM_SESSION;
import static android.server.wm.app.Components.LAUNCH_ASSISTANT_ACTIVITY_INTO_STACK;
import static android.server.wm.app.Components.LaunchAssistantActivityIntoAssistantStack.EXTRA_ASSISTANT_IS_TRANSLUCENT;
import static android.server.wm.app.Components.PIP_ACTIVITY;
import static android.server.wm.app.Components.PipActivity.EXTRA_ENTER_PIP;
import static android.server.wm.app.Components.TEST_ACTIVITY;
import static android.server.wm.app.Components.TRANSLUCENT_ASSISTANT_ACTIVITY;
import static android.server.wm.app.Components.TestActivity.TEST_ACTIVITY_ACTION_FINISH_SELF;
import static android.view.Display.DEFAULT_DISPLAY;

import static org.hamcrest.Matchers.hasSize;
import static org.junit.Assert.assertEquals;
import static org.junit.Assert.assertThat;
import static org.junit.Assert.assertTrue;
import static org.junit.Assume.assumeTrue;

import android.content.ComponentName;
import android.platform.test.annotations.Presubmit;
import android.provider.Settings;
import android.server.wm.settings.SettingsSession;

import org.junit.Ignore;
import org.junit.Test;

/**
 * Build/Install/Run:
 *     atest CtsWindowManagerDeviceTestCases:AssistantStackTests
 */
@Presubmit
public class AssistantStackTests extends ActivityManagerTestBase {

    private int mAssistantDisplayId = DEFAULT_DISPLAY;
    private int mDefaultWindowingMode;

    public void setUp() throws Exception {
        super.setUp();
        try (final AssistantSession assistantSession = new AssistantSession()) {
            assistantSession.setVoiceInteractionService(ASSISTANT_VOICE_INTERACTION_SERVICE);
            launchActivityNoWait(LAUNCH_ASSISTANT_ACTIVITY_INTO_STACK);
            waitForValidStateWithActivityType(ASSISTANT_ACTIVITY, ACTIVITY_TYPE_ASSISTANT);
            WindowManagerState.ActivityTask assistantStack =
                    mWmState.getStackByActivityType(ACTIVITY_TYPE_ASSISTANT);
            mAssistantDisplayId = assistantStack.mDisplayId;
            mDefaultWindowingMode = getDefaultDisplayWindowingMode();
        }
    }

    @Test
    public void testLaunchingAssistantActivityIntoAssistantStack() throws Exception {
        // Enable the assistant and launch an assistant activity
        try (final AssistantSession assistantSession = new AssistantSession()) {
            assistantSession.setVoiceInteractionService(ASSISTANT_VOICE_INTERACTION_SERVICE);

            launchActivityNoWait(LAUNCH_ASSISTANT_ACTIVITY_FROM_SESSION);
            waitForValidStateWithActivityType(ASSISTANT_ACTIVITY, ACTIVITY_TYPE_ASSISTANT);

            // Ensure that the activity launched in the fullscreen assistant stack
            assertAssistantStackExists();
            // In a multi-window environment the assistant might not be fullscreen
            assumeTrue(mDefaultWindowingMode == WINDOWING_MODE_FULLSCREEN);
            assertTrue("Expected assistant stack to be fullscreen",
                    mWmState.getStackByActivityType(
                            ACTIVITY_TYPE_ASSISTANT).isFullscreen());
        }
    }

    @Test
    public void testAssistantStackZOrder() throws Exception {
        assumeTrue(assistantRunsOnPrimaryDisplay());
        assumeTrue(supportsPip());
        assumeTrue(supportsSplitScreenMultiWindow());

        // Launch a pinned stack task
        launchActivity(PIP_ACTIVITY, extraString(EXTRA_ENTER_PIP, "true"));
        waitForValidStateWithActivityTypeAndWindowingMode(
                PIP_ACTIVITY, ACTIVITY_TYPE_STANDARD, WINDOWING_MODE_PINNED);
        mWmState.assertContainsStack("Must contain pinned stack.",
                WINDOWING_MODE_PINNED, ACTIVITY_TYPE_STANDARD);

        // Dock a task
        launchActivitiesInSplitScreen(
                getLaunchActivityBuilder().setTargetActivity(DOCKED_ACTIVITY),
                getLaunchActivityBuilder().setTargetActivity(TEST_ACTIVITY));

        // Enable the assistant and launch an assistant activity, ensure it is on top
        try (final AssistantSession assistantSession = new AssistantSession()) {
            assistantSession.setVoiceInteractionService(ASSISTANT_VOICE_INTERACTION_SERVICE);

            launchActivityNoWait(LAUNCH_ASSISTANT_ACTIVITY_FROM_SESSION);
            waitForValidStateWithActivityType(ASSISTANT_ACTIVITY, ACTIVITY_TYPE_ASSISTANT);
            assertAssistantStackExists();

            mWmState.assertFrontStack("Pinned stack should be on top.",
                    WINDOWING_MODE_PINNED, ACTIVITY_TYPE_STANDARD);
            mWmState.assertFocusedStack("Assistant stack should be focused.",
                    WINDOWING_MODE_UNDEFINED, ACTIVITY_TYPE_ASSISTANT);
        }
    }

    @Test
    public void testAssistantStackLaunchNewTask() throws Exception {
        assertAssistantStackCanLaunchAndReturnFromNewTask(mDefaultWindowingMode);
    }

    @Test
    @Ignore("b/77272253#comment10")
    public void testAssistantStackLaunchNewTaskWithDockedStack() throws Exception {
        assumeTrue(assistantRunsOnPrimaryDisplay());
        assumeTrue(supportsSplitScreenMultiWindow());

        // Dock a task
        launchActivitiesInSplitScreen(
                getLaunchActivityBuilder().setTargetActivity(DOCKED_ACTIVITY),
                getLaunchActivityBuilder().setTargetActivity(TEST_ACTIVITY));

        //assertAssistantStackCanLaunchAndReturnFromNewTask(WINDOWING_MODE_SPLIT_SCREEN_SECONDARY);
    }

    private void assertAssistantStackCanLaunchAndReturnFromNewTask(int expectedWindowingMode)
            throws Exception {
        // Enable the assistant and launch an assistant activity which will launch a new task
        try (final AssistantSession assistantSession = new AssistantSession()) {
            assistantSession.setVoiceInteractionService(ASSISTANT_VOICE_INTERACTION_SERVICE);

            launchActivityOnDisplayNoWait(LAUNCH_ASSISTANT_ACTIVITY_INTO_STACK, mAssistantDisplayId,
                    extraString(EXTRA_ASSISTANT_LAUNCH_NEW_TASK, getActivityName(TEST_ACTIVITY)),
                    extraString(EXTRA_ASSISTANT_DISPLAY_ID, Integer.toString(mAssistantDisplayId)));
            // Ensure that the fullscreen stack is on top and the test activity is now visible
            waitForValidStateWithActivityTypeAndWindowingMode(
                    TEST_ACTIVITY, ACTIVITY_TYPE_STANDARD, expectedWindowingMode);
        }

        if (isAssistantOnTopOfDream()) {
            // If the assistant is configured to be always-on-top, then the new task should have
            // been started behind it and the assistant stack should still be on top.
            mWmState.assertFocusedActivity(
                    "AssistantActivity should be focused", ASSISTANT_ACTIVITY);
            mWmState.assertFrontStackActivityType(
                    "Assistant stack should be on top.", ACTIVITY_TYPE_ASSISTANT);
            mWmState.assertFocusedStack("Assistant stack should be focused.",
                    WINDOWING_MODE_UNDEFINED, ACTIVITY_TYPE_ASSISTANT);
        } else {
            mWmState.assertFocusedActivity("TestActivity should be resumed", TEST_ACTIVITY);
            mWmState.assertFrontStack("TestActivity stack should be on top.",
                    expectedWindowingMode, ACTIVITY_TYPE_STANDARD);
            mWmState.assertFocusedStack("TestActivity stack should be focused.",
                    expectedWindowingMode, ACTIVITY_TYPE_STANDARD);
        }

        // Now, tell it to finish itself and ensure that the assistant stack is brought back forward
        mBroadcastActionTrigger.doAction(TEST_ACTIVITY_ACTION_FINISH_SELF);
        mWmState.waitForFocusedStack(WINDOWING_MODE_UNDEFINED, ACTIVITY_TYPE_ASSISTANT);
        mWmState.assertFrontStackActivityType(
                "Assistant stack should be on top.", ACTIVITY_TYPE_ASSISTANT);
        mWmState.assertFocusedStack("Assistant stack should be focused.",
                WINDOWING_MODE_UNDEFINED, ACTIVITY_TYPE_ASSISTANT);
    }

    @Test
    public void testAssistantStackFinishToPreviousApp() throws Exception {
        // If the Assistant is configured to be always-on-top, then the assistant activity
        // started in setUp() will not allow any other activities to start. Therefore we should
        // remove it before launching a fullscreen activity.
        if (isAssistantOnTopOfDream()) {
            removeRootTasksWithActivityTypes(ACTIVITY_TYPE_ASSISTANT);
        }

        // Launch an assistant activity on top of an existing fullscreen activity, and ensure that
        // the fullscreen activity is still visible and on top after the assistant activity finishes
        launchActivityOnDisplay(TEST_ACTIVITY, WINDOWING_MODE_FULLSCREEN, mAssistantDisplayId);
        try (final AssistantSession assistantSession = new AssistantSession()) {
            assistantSession.setVoiceInteractionService(ASSISTANT_VOICE_INTERACTION_SERVICE);

            launchActivityNoWait(LAUNCH_ASSISTANT_ACTIVITY_INTO_STACK,
                    extraString(EXTRA_ASSISTANT_FINISH_SELF, "true"));
            mWmState.waitFor((amState) -> !amState.containsActivity(ASSISTANT_ACTIVITY),
                    getActivityName(ASSISTANT_ACTIVITY) + " finished");
        }
        waitForValidStateWithActivityTypeAndWindowingMode(
                TEST_ACTIVITY, ACTIVITY_TYPE_STANDARD, mDefaultWindowingMode);
        waitAndAssertTopResumedActivity(TEST_ACTIVITY, mAssistantDisplayId,
                "TestActivity should be resumed");
        mWmState.assertFocusedActivity("TestActivity should be focused", TEST_ACTIVITY);
        mWmState.assertFrontStack("Fullscreen stack should be on top.",
                mDefaultWindowingMode, ACTIVITY_TYPE_STANDARD);
        mWmState.assertFocusedStack("Fullscreen stack should be focused.",
                mDefaultWindowingMode, ACTIVITY_TYPE_STANDARD);
    }

    @Test
    public void testDisallowEnterPiPFromAssistantStack() throws Exception {
        try (final AssistantSession assistantSession = new AssistantSession()) {
            assistantSession.setVoiceInteractionService(ASSISTANT_VOICE_INTERACTION_SERVICE);

            launchActivityNoWait(LAUNCH_ASSISTANT_ACTIVITY_INTO_STACK,
                    extraString(EXTRA_ASSISTANT_ENTER_PIP, "true"));
        }
        waitForValidStateWithActivityType(ASSISTANT_ACTIVITY, ACTIVITY_TYPE_ASSISTANT);
        mWmState.assertDoesNotContainStack("Must not contain pinned stack.",
                WINDOWING_MODE_PINNED, ACTIVITY_TYPE_STANDARD);
    }

    @Test
    public void testTranslucentAssistantActivityStackVisibility() throws Exception {
        try (final AssistantSession assistantSession = new AssistantSession()) {
            assistantSession.setVoiceInteractionService(ASSISTANT_VOICE_INTERACTION_SERVICE);

            // Go home, launch the assistant and check to see that home is visible
            removeRootTasksInWindowingModes(WINDOWING_MODE_FULLSCREEN);
            pressHomeButton();
            resumeAppSwitches();
            launchActivityNoWait(LAUNCH_ASSISTANT_ACTIVITY_INTO_STACK,
                    extraString(EXTRA_ASSISTANT_IS_TRANSLUCENT, "true"));
            waitForValidStateWithActivityType(
                    TRANSLUCENT_ASSISTANT_ACTIVITY, ACTIVITY_TYPE_ASSISTANT);
            assertAssistantStackExists();
            mWmState.waitForHomeActivityVisible();
            if (hasHomeScreen()) {
                mWmState.assertHomeActivityVisible(true);
            }

            // Launch a fullscreen app and then launch the assistant and check to see that it is
            // also visible
            removeRootTasksWithActivityTypes(ACTIVITY_TYPE_ASSISTANT);
            launchActivityOnDisplay(TEST_ACTIVITY, WINDOWING_MODE_FULLSCREEN, mAssistantDisplayId);
            launchActivityNoWait(LAUNCH_ASSISTANT_ACTIVITY_INTO_STACK,
                    extraString(EXTRA_ASSISTANT_IS_TRANSLUCENT, "true"));
            waitForValidStateWithActivityType(
                    TRANSLUCENT_ASSISTANT_ACTIVITY, ACTIVITY_TYPE_ASSISTANT);
            assertAssistantStackExists();
            mWmState.assertVisibility(TEST_ACTIVITY, true);

            // Go home, launch assistant, launch app into fullscreen with activity present, and go
            // back.Ensure home is visible.
            removeRootTasksWithActivityTypes(ACTIVITY_TYPE_ASSISTANT);
            pressHomeButton();
            resumeAppSwitches();
            launchActivityNoWait(LAUNCH_ASSISTANT_ACTIVITY_INTO_STACK,
                    extraString(EXTRA_ASSISTANT_IS_TRANSLUCENT, "true"),
                    extraString(EXTRA_ASSISTANT_LAUNCH_NEW_TASK, getActivityName(TEST_ACTIVITY)));
            waitForValidStateWithActivityTypeAndWindowingMode(
                    TEST_ACTIVITY, ACTIVITY_TYPE_STANDARD, mDefaultWindowingMode);

            final ComponentName homeActivity = mWmState.getHomeActivityName();
            int windowingMode = mWmState.getFocusedStackWindowingMode();
            // In a multi-window environment the home activity might not be fully covered
            assumeTrue(windowingMode == WINDOWING_MODE_FULLSCREEN);
            mWmState.waitAndAssertVisibilityGone(homeActivity);
            mBroadcastActionTrigger.doAction(TEST_ACTIVITY_ACTION_FINISH_SELF);
            mWmState.waitForFocusedStack(WINDOWING_MODE_UNDEFINED, ACTIVITY_TYPE_ASSISTANT);
            assertAssistantStackExists();
            mWmState.waitForHomeActivityVisible();
            if (hasHomeScreen()) {
                mWmState.assertHomeActivityVisible(true);
            }

            // Launch a fullscreen and docked app and then launch the assistant and check to see
            // that it
            // is also visible
            if (supportsSplitScreenMultiWindow() &&  assistantRunsOnPrimaryDisplay()) {
                removeRootTasksWithActivityTypes(ACTIVITY_TYPE_ASSISTANT);
                launchActivitiesInSplitScreen(
                        getLaunchActivityBuilder().setTargetActivity(DOCKED_ACTIVITY),
                        getLaunchActivityBuilder().setTargetActivity(TEST_ACTIVITY));
                launchActivityNoWait(LAUNCH_ASSISTANT_ACTIVITY_INTO_STACK,
                        extraString(EXTRA_ASSISTANT_IS_TRANSLUCENT, "true"));
                waitForValidStateWithActivityType(
                        TRANSLUCENT_ASSISTANT_ACTIVITY, ACTIVITY_TYPE_ASSISTANT);
                assertAssistantStackExists();
                mWmState.assertVisibility(DOCKED_ACTIVITY, true);
                mWmState.assertVisibility(TEST_ACTIVITY, true);
            }
        }
    }

    @Test
    public void testLaunchIntoSameTask() throws Exception {
        try (final AssistantSession assistantSession = new AssistantSession()) {
            assistantSession.setVoiceInteractionService(ASSISTANT_VOICE_INTERACTION_SERVICE);

            // Launch the assistant
            launchActivityOnDisplayNoWait(LAUNCH_ASSISTANT_ACTIVITY_FROM_SESSION,
                    mAssistantDisplayId);
            waitForValidStateWithActivityType(ASSISTANT_ACTIVITY, ACTIVITY_TYPE_ASSISTANT);
            assertAssistantStackExists();
            mWmState.assertVisibility(ASSISTANT_ACTIVITY, true);
            mWmState.assertFocusedStack("Expected assistant stack focused",
                    WINDOWING_MODE_UNDEFINED, ACTIVITY_TYPE_ASSISTANT);
            final WindowManagerState amState = mWmState;
            assertThat(amState.getStackByActivityType(ACTIVITY_TYPE_ASSISTANT).getTasks(),
                    hasSize(1));
            final int taskId = mWmState.getTaskByActivity(ASSISTANT_ACTIVITY)
                    .mTaskId;

            // Launch a new fullscreen activity
            // Using Animation Test Activity because it is opaque on all devices.
            launchActivityOnDisplay(ANIMATION_TEST_ACTIVITY, WINDOWING_MODE_FULLSCREEN, mAssistantDisplayId);
            // Wait for animation finished.
            mWmState.waitForActivityState(ANIMATION_TEST_ACTIVITY, STATE_RESUMED);

            if (isAssistantOnTopOfDream()) {
                mWmState.assertVisibility(ASSISTANT_ACTIVITY, true);
            } else {
                mWmState.waitAndAssertVisibilityGone(ASSISTANT_ACTIVITY);
            }

            // Launch the assistant again and ensure that it goes into the same task
            launchActivityOnDisplayNoWait(LAUNCH_ASSISTANT_ACTIVITY_FROM_SESSION,
                    mAssistantDisplayId);
            waitForValidStateWithActivityType(ASSISTANT_ACTIVITY, ACTIVITY_TYPE_ASSISTANT);
            assertAssistantStackExists();
            mWmState.assertVisibility(ASSISTANT_ACTIVITY, true);
            mWmState.assertFocusedStack("Expected assistant stack focused",
                    WINDOWING_MODE_UNDEFINED, ACTIVITY_TYPE_ASSISTANT);
            assertThat(amState.getStackByActivityType(ACTIVITY_TYPE_ASSISTANT).getTasks(),
                    hasSize(1));
            assertEquals(taskId,
                    mWmState.getTaskByActivity(ASSISTANT_ACTIVITY).mTaskId);

        }
    }

    @Test
    public void testPinnedStackWithAssistant() throws Exception {
        assumeTrue(supportsPip());
        assumeTrue(supportsSplitScreenMultiWindow());

        try (final AssistantSession assistantSession = new AssistantSession()) {
            assistantSession.setVoiceInteractionService(ASSISTANT_VOICE_INTERACTION_SERVICE);

            // Launch a fullscreen activity and a PIP activity, then launch the assistant, and
            // ensure that the test activity is still visible
            launchActivity(TEST_ACTIVITY, WINDOWING_MODE_FULLSCREEN);
            launchActivity(PIP_ACTIVITY, extraString(EXTRA_ENTER_PIP, "true"));
            launchActivityNoWait(LAUNCH_ASSISTANT_ACTIVITY_INTO_STACK,
                    extraString(EXTRA_ASSISTANT_IS_TRANSLUCENT, String.valueOf(true)));
            waitForValidStateWithActivityType(
                    TRANSLUCENT_ASSISTANT_ACTIVITY, ACTIVITY_TYPE_ASSISTANT);
            assertAssistantStackExists();
            mWmState.assertVisibility(TRANSLUCENT_ASSISTANT_ACTIVITY, true);
            mWmState.assertVisibility(PIP_ACTIVITY, true);
            mWmState.assertVisibility(TEST_ACTIVITY, true);

        }
    }

    private void waitForValidStateWithActivityType(ComponentName activityName, int activityType)
            throws Exception {
        mWmState.waitForValidState(new WaitForValidActivityState.Builder(activityName)
                .setActivityType(activityType)
                .build());
    }

    private void waitForValidStateWithActivityTypeAndWindowingMode(ComponentName activityName,
            int activityType, int windowingMode) throws Exception {
        mWmState.waitForValidState(new WaitForValidActivityState.Builder(activityName)
                .setActivityType(activityType)
                .setWindowingMode(windowingMode)
                .build());
    }

    /**
     * Asserts that the assistant stack exists.
     */
    private void assertAssistantStackExists() throws Exception {
        mWmState.assertContainsStack("Must contain assistant stack.",
                WINDOWING_MODE_UNDEFINED, ACTIVITY_TYPE_ASSISTANT);
    }

    // Any 2D Activity in VR mode is run on a special VR virtual display, so check if the Assistant
    // is going to run on the same display as other tasks.
    protected boolean assistantRunsOnPrimaryDisplay() {
        return mAssistantDisplayId == DEFAULT_DISPLAY;
    }

    /**
     * @return Windowing Mode from the default display
     */
    private int getDefaultDisplayWindowingMode() {
        mWmState.computeState();
        return mWmState.getDisplay(DEFAULT_DISPLAY).getWindowingMode();
    }

    /** Helper class to save, set, and restore
     * {@link Settings.Secure#VOICE_INTERACTION_SERVICE} system preference.
     */
    private static class AssistantSession extends SettingsSession<String> {
        AssistantSession() {
            super(Settings.Secure.getUriFor(Settings.Secure.VOICE_INTERACTION_SERVICE),
                    Settings.Secure::getString, Settings.Secure::putString);
        }

        void setVoiceInteractionService(ComponentName assistantName) throws Exception {
            super.set(getActivityName(assistantName));
        }
    }
}
