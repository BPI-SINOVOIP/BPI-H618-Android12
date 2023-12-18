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

import static android.server.wm.StateLogger.log;
import static android.server.wm.lifecycle.LifecycleLog.ActivityCallback.ON_CREATE;
import static android.server.wm.lifecycle.LifecycleLog.ActivityCallback.ON_DESTROY;
import static android.server.wm.lifecycle.LifecycleLog.ActivityCallback.ON_MULTI_WINDOW_MODE_CHANGED;
import static android.server.wm.lifecycle.LifecycleLog.ActivityCallback.ON_PAUSE;
import static android.server.wm.lifecycle.LifecycleLog.ActivityCallback.ON_POST_CREATE;
import static android.server.wm.lifecycle.LifecycleLog.ActivityCallback.ON_RESTART;
import static android.server.wm.lifecycle.LifecycleLog.ActivityCallback.ON_RESUME;
import static android.server.wm.lifecycle.LifecycleLog.ActivityCallback.ON_START;
import static android.server.wm.lifecycle.LifecycleLog.ActivityCallback.ON_STOP;
import static android.server.wm.lifecycle.LifecycleLog.ActivityCallback.ON_TOP_POSITION_GAINED;
import static android.server.wm.lifecycle.LifecycleLog.ActivityCallback.ON_TOP_POSITION_LOST;

import static org.junit.Assert.assertEquals;
import static org.junit.Assert.assertFalse;
import static org.junit.Assert.assertTrue;
import static org.junit.Assert.fail;

import android.app.Activity;
import android.content.Context;
import android.content.pm.PackageManager;
import android.server.wm.lifecycle.ActivityLifecycleClientTestBase.CallbackTrackingActivity;
import android.server.wm.lifecycle.ActivityLifecycleClientTestBase.ConfigChangeHandlingActivity;
import android.server.wm.lifecycle.LifecycleLog.ActivityCallback;
import android.util.Pair;

import java.util.ArrayList;
import java.util.Arrays;
import java.util.List;

/** Util class that verifies correct activity state transition sequences. */
class LifecycleVerifier {

    private static final Class CALLBACK_TRACKING_CLASS = CallbackTrackingActivity.class;
    private static final Class CONFIG_CHANGE_HANDLING_CLASS = ConfigChangeHandlingActivity.class;

    static void assertLaunchSequence(Class<? extends Activity> activityClass,
            LifecycleLog lifecycleLog, LifecycleLog.ActivityCallback... expectedSubsequentEvents) {
        final List<LifecycleLog.ActivityCallback> observedTransitions =
                lifecycleLog.getActivityLog(activityClass);
        log("Observed sequence: " + observedTransitions);
        final String errorMessage = errorDuringTransition(activityClass, "launch");

        final List<LifecycleLog.ActivityCallback> launchSequence = getLaunchSequence(activityClass);
        final List<LifecycleLog.ActivityCallback> expectedTransitions;
        expectedTransitions = new ArrayList<>(launchSequence.size() +
                expectedSubsequentEvents.length);
        expectedTransitions.addAll(launchSequence);
        expectedTransitions.addAll(Arrays.asList(expectedSubsequentEvents));
        assertEquals(errorMessage, expectedTransitions, observedTransitions);
    }

    public static List<LifecycleLog.ActivityCallback> getLaunchSequence(
            Class<? extends Activity> activityClass) {
        return CALLBACK_TRACKING_CLASS.isAssignableFrom(activityClass)
                ? Arrays.asList(ON_CREATE, ON_START, ON_POST_CREATE, ON_RESUME,
                ON_TOP_POSITION_GAINED)
                : Arrays.asList(ON_CREATE, ON_START, ON_RESUME);
    }

    static List<ActivityCallback> getLaunchAndDestroySequence(
            Class<? extends Activity> activityClass) {
        final List<ActivityCallback> expectedTransitions = new ArrayList<>();
        expectedTransitions.addAll(getLaunchSequence(activityClass));
        expectedTransitions.addAll(getResumeToDestroySequence(activityClass));
        return expectedTransitions;
    }

    static void assertLaunchSequence(Class<? extends Activity> launchingActivity,
            Class<? extends Activity> existingActivity, LifecycleLog lifecycleLog,
            boolean launchingIsTranslucent) {
        final boolean includingCallbacks;
        if (CALLBACK_TRACKING_CLASS.isAssignableFrom(launchingActivity)
                && CALLBACK_TRACKING_CLASS.isAssignableFrom(existingActivity)) {
            includingCallbacks = true;
        } else if (!CALLBACK_TRACKING_CLASS.isAssignableFrom(launchingActivity)
                && !CALLBACK_TRACKING_CLASS.isAssignableFrom(existingActivity)) {
            includingCallbacks = false;
        } else {
            throw new IllegalArgumentException("Mixed types of callback tracking not supported. "
                    + "Both activities must support or not support callback tracking "
                    + "simultaneously");
        }


        final List<Pair<String, ActivityCallback>> observedTransitions = lifecycleLog.getLog();
        log("Observed sequence: " + observedTransitions);
        final String errorMessage = errorDuringTransition(launchingActivity, "launch");

        final List<Pair<String, ActivityCallback>> expectedTransitions = new ArrayList<>();
        // First top position will be lost
        if (includingCallbacks) {
            expectedTransitions.add(transition(existingActivity, ON_TOP_POSITION_LOST));
        }
        // Next the existing activity is paused and the next one is launched
        expectedTransitions.add(transition(existingActivity, ON_PAUSE));
        expectedTransitions.add(transition(launchingActivity, ON_CREATE));
        expectedTransitions.add(transition(launchingActivity, ON_START));
        if (includingCallbacks) {
            expectedTransitions.add(transition(launchingActivity, ON_POST_CREATE));
        }
        expectedTransitions.add(transition(launchingActivity, ON_RESUME));
        if (includingCallbacks) {
            expectedTransitions.add(transition(launchingActivity, ON_TOP_POSITION_GAINED));
        }
        if (!launchingIsTranslucent) {
            expectedTransitions.add(transition(existingActivity, ON_STOP));
        }

        assertEquals(errorMessage, expectedTransitions, observedTransitions);
    }

    static void assertLaunchAndStopSequence(Class<? extends Activity> activityClass,
            LifecycleLog lifecycleLog) {
        assertLaunchAndStopSequence(activityClass, lifecycleLog,
                false /* onTop */);
    }

    static void assertLaunchAndStopSequence(Class<? extends Activity> activityClass,
            LifecycleLog lifecycleLog, boolean onTop) {
        final List<ActivityCallback> observedTransitions =
                lifecycleLog.getActivityLog(activityClass);
        log("Observed sequence: " + observedTransitions);
        final String errorMessage = errorDuringTransition(activityClass, "launch and stop");

        final boolean includeCallbacks = CALLBACK_TRACKING_CLASS.isAssignableFrom(activityClass);

        final List<ActivityCallback> expectedTransitions = new ArrayList<>(
                Arrays.asList(ON_CREATE, ON_START));
        if (includeCallbacks) {
            expectedTransitions.add(ON_POST_CREATE);
        }
        expectedTransitions.add(ON_RESUME);
        if (includeCallbacks && onTop) {
            expectedTransitions.addAll(Arrays.asList(ON_TOP_POSITION_GAINED, ON_TOP_POSITION_LOST));
        }
        expectedTransitions.addAll(Arrays.asList(ON_PAUSE, ON_STOP));
        assertEquals(errorMessage, expectedTransitions, observedTransitions);
    }

    static void assertLaunchAndPauseSequence(Class<? extends Activity> activityClass,
            LifecycleLog lifecycleLog) {
        final List<ActivityCallback> observedTransitions =
                lifecycleLog.getActivityLog(activityClass);
        log("Observed sequence: " + observedTransitions);
        final String errorMessage = errorDuringTransition(activityClass, "launch and pause");

        final List<ActivityCallback> expectedTransitions =
                Arrays.asList(ON_CREATE, ON_START, ON_RESUME, ON_PAUSE);
        assertEquals(errorMessage, expectedTransitions, observedTransitions);
    }

    static void assertRestartSequence(Class<? extends Activity> activityClass,
            LifecycleLog lifecycleLog) {
        final List<LifecycleLog.ActivityCallback> observedTransitions =
                lifecycleLog.getActivityLog(activityClass);
        log("Observed sequence: " + observedTransitions);
        final String errorMessage = errorDuringTransition(activityClass, "restart");

        final List<LifecycleLog.ActivityCallback> expectedTransitions =
                Arrays.asList(ON_RESTART, ON_START);
        assertEquals(errorMessage, expectedTransitions, observedTransitions);
    }

    static void assertRestartAndResumeSequence(Class<? extends Activity> activityClass,
            LifecycleLog lifecycleLog) {
        final List<LifecycleLog.ActivityCallback> observedTransitions =
                lifecycleLog.getActivityLog(activityClass);
        log("Observed sequence: " + observedTransitions);
        final String errorMessage = errorDuringTransition(activityClass, "restart and pause");

        final List<LifecycleLog.ActivityCallback> expectedTransitions =
                Arrays.asList(ON_RESTART, ON_START, ON_RESUME);
        assertEquals(errorMessage, expectedTransitions, observedTransitions);
    }

    /**
     * TODO(b/192274045): In Automotive, we tolerate superfluous lifecycle events between the first
     * lifecycle events and the last one until any discrepancy between ActivityManager and Keyguard
     * state is resolved.
     */
    static void assertRestartAndResumeSubSequence(Class<? extends Activity> activityClass,
            LifecycleLog lifecycleLog) {
        final List<LifecycleLog.ActivityCallback> observedTransitions =
                lifecycleLog.getActivityLog(activityClass);
        log("Observed sequence: " + observedTransitions);

        final List<Pair<String, ActivityCallback>> expectedTransitions =
                Arrays.asList(transition(activityClass, ON_RESTART),
                        transition(activityClass, ON_START), transition(activityClass, ON_RESUME));

        assertOrder(lifecycleLog, expectedTransitions, "restart and resume");
    }

    static void assertRecreateAndResumeSequence(Class<? extends Activity> activityClass,
            LifecycleLog lifecycleLog) {
        final List<LifecycleLog.ActivityCallback> observedTransitions =
                lifecycleLog.getActivityLog(activityClass);
        log("Observed sequence: " + observedTransitions);
        final String errorMessage = errorDuringTransition(activityClass, "recreateA  and pause");

        final List<LifecycleLog.ActivityCallback> expectedTransitions =
                Arrays.asList(ON_DESTROY, ON_CREATE, ON_START, ON_RESUME);
        assertEquals(errorMessage, expectedTransitions, observedTransitions);
    }

    static void assertLaunchAndDestroySequence(Class<? extends Activity> activityClass,
            LifecycleLog lifecycleLog) {
        final List<LifecycleLog.ActivityCallback> observedTransitions =
                lifecycleLog.getActivityLog(activityClass);
        log("Observed sequence: " + observedTransitions);
        final String errorMessage = errorDuringTransition(activityClass, "launch and destroy");

        final List<LifecycleLog.ActivityCallback> expectedTransitions = Arrays.asList(
                ON_CREATE, ON_START, ON_RESUME, ON_PAUSE, ON_STOP, ON_DESTROY);
        assertEquals(errorMessage, expectedTransitions, observedTransitions);
    }

    static void assertResumeToDestroySequence(Class<? extends Activity> activityClass,
            LifecycleLog lifecycleLog) {
        final List<LifecycleLog.ActivityCallback> observedTransitions =
                lifecycleLog.getActivityLog(activityClass);
        log("Observed sequence: " + observedTransitions);
        final String errorMessage = errorDuringTransition(activityClass, "launch and destroy");

        final List<LifecycleLog.ActivityCallback> expectedTransitions =
                getResumeToDestroySequence(activityClass);
        assertEquals(errorMessage, expectedTransitions, observedTransitions);
    }

    static List<LifecycleLog.ActivityCallback> getResumeToDestroySequence(
            Class<? extends Activity> activityClass) {
        return CALLBACK_TRACKING_CLASS.isAssignableFrom(activityClass)
                ? Arrays.asList(ON_TOP_POSITION_LOST, ON_PAUSE, ON_STOP, ON_DESTROY)
                : Arrays.asList(ON_PAUSE, ON_STOP, ON_DESTROY);
    }

    static void assertResumeToStopSequence(Class<? extends Activity> activityClass,
            LifecycleLog lifecycleLog) {
        final List<LifecycleLog.ActivityCallback> observedTransitions =
                lifecycleLog.getActivityLog(activityClass);
        log("Observed sequence: " + observedTransitions);
        final String errorMessage = errorDuringTransition(activityClass, "resumed to stopped");
        final boolean includeCallbacks = CALLBACK_TRACKING_CLASS.isAssignableFrom(activityClass);

        final List<LifecycleLog.ActivityCallback> expectedTransitions = new ArrayList<>();
        if (includeCallbacks) {
            expectedTransitions.add(ON_TOP_POSITION_LOST);
        }
        expectedTransitions.add(ON_PAUSE);
        expectedTransitions.add(ON_STOP);

        assertEquals(errorMessage, expectedTransitions, observedTransitions);
    }

    static void assertStopToResumeSequence(Class<? extends Activity> activityClass,
            LifecycleLog lifecycleLog) {
        final List<LifecycleLog.ActivityCallback> observedTransitions =
                lifecycleLog.getActivityLog(activityClass);
        log("Observed sequence: " + observedTransitions);
        final String errorMessage = errorDuringTransition(activityClass, "stopped to resumed");
        final boolean includeCallbacks = CALLBACK_TRACKING_CLASS.isAssignableFrom(activityClass);

        final List<LifecycleLog.ActivityCallback> expectedTransitions = new ArrayList<>(
                Arrays.asList(ON_RESTART, ON_START, ON_RESUME));
        if (includeCallbacks) {
            expectedTransitions.add(ON_TOP_POSITION_GAINED);
        }

        assertEquals(errorMessage, expectedTransitions, observedTransitions);
    }

    /**
     * TODO(b/192274045): In Automotive, we tolerate superfluous lifecycle events between the first
     * lifecycle events and the last one until any discrepancy between ActivityManager and Keyguard
     * state is resolved.
     */
    static void assertStopToResumeSubSequence(Class<? extends Activity> activityClass,
            LifecycleLog lifecycleLog) {
        final List<LifecycleLog.ActivityCallback> observedTransitions =
                lifecycleLog.getActivityLog(activityClass);
        log("Observed sequence: " + observedTransitions);
        final boolean includeCallbacks = CALLBACK_TRACKING_CLASS.isAssignableFrom(activityClass);

        final List<Pair<String, ActivityCallback>> expectedTransitions = new ArrayList<>(
                Arrays.asList(transition(activityClass, ON_RESTART),
                        transition(activityClass, ON_START), transition(activityClass, ON_RESUME)));
        if (includeCallbacks) {
            expectedTransitions.add(transition(activityClass, ON_TOP_POSITION_GAINED));
        }

        assertOrder(lifecycleLog, expectedTransitions, "stop and resume");
    }

    static void assertRelaunchSequence(Class<? extends Activity> activityClass,
            LifecycleLog lifecycleLog, LifecycleLog.ActivityCallback startState) {
        final List<LifecycleLog.ActivityCallback> expectedTransitions = getRelaunchSequence(startState);
        assertSequence(activityClass, lifecycleLog, expectedTransitions, "relaunch");
    }

    static List<LifecycleLog.ActivityCallback> getRelaunchSequence(
            LifecycleLog.ActivityCallback startState) {
        final List<LifecycleLog.ActivityCallback> expectedTransitions;
        if (startState == ON_PAUSE) {
            expectedTransitions = Arrays.asList(
                    ON_STOP, ON_DESTROY, ON_CREATE, ON_START, ON_RESUME, ON_PAUSE);
        } else if (startState == ON_STOP) {
            expectedTransitions = Arrays.asList(
                    ON_DESTROY, ON_CREATE, ON_START, ON_RESUME, ON_PAUSE, ON_STOP);
        } else if (startState == ON_RESUME) {
            expectedTransitions = Arrays.asList(
                    ON_PAUSE, ON_STOP, ON_DESTROY, ON_CREATE, ON_START, ON_RESUME);
        } else if (startState == ON_TOP_POSITION_GAINED) {
            // Looks like we're tracking the callbacks here
            expectedTransitions = Arrays.asList(
                    ON_TOP_POSITION_LOST, ON_PAUSE, ON_STOP, ON_DESTROY, ON_CREATE,
                    ON_START, ON_POST_CREATE, ON_RESUME, ON_TOP_POSITION_GAINED);
        } else {
            throw new IllegalArgumentException("Start state not supported: " + startState);
        }
        return expectedTransitions;
    }

    static List<LifecycleLog.ActivityCallback> getSplitScreenTransitionSequence(
            Class<? extends Activity> activityClass) {
        // Minimized-dock is not a policy requirement and but SysUI-specific concept, so we here
        // don't expect a trailing ON_PAUSE.
        return CALLBACK_TRACKING_CLASS.isAssignableFrom(activityClass)
                ? CONFIG_CHANGE_HANDLING_CLASS.isAssignableFrom(activityClass)
                ? Arrays.asList(ON_MULTI_WINDOW_MODE_CHANGED, ON_TOP_POSITION_LOST)
                : Arrays.asList(ON_TOP_POSITION_LOST, ON_PAUSE, ON_STOP, ON_DESTROY,
                        ON_CREATE, ON_START, ON_POST_CREATE, ON_RESUME,
                        ON_TOP_POSITION_GAINED, ON_TOP_POSITION_LOST)
                : Arrays.asList(ON_PAUSE, ON_STOP, ON_DESTROY, ON_CREATE, ON_START, ON_RESUME);
    }

    // TODO(b/149338177): Remove this workaround once test passes with TestTaskOrganizer not to
    // depend on minimized dock feature which is not policy requirement, but SysUI-specific.
    /**
     * Returns the result of appending "leave from minimized dock" transitions to given transitions
     * to "consume" these activity callbacks.
     */
    static List<ActivityCallback> appendMinimizedDockTransitionTrail(
            List<ActivityCallback> transitions) {
        final List<LifecycleLog.ActivityCallback> newTransitions =
                new ArrayList<LifecycleLog.ActivityCallback>(transitions);
        newTransitions.addAll(Arrays.asList(ON_PAUSE, ON_RESUME));

        return newTransitions;
    }

    static void assertSequence(Class<? extends Activity> activityClass, LifecycleLog lifecycleLog,
            List<ActivityCallback> expectedTransitions, String transition) {
        final List<ActivityCallback> observedTransitions =
                lifecycleLog.getActivityLog(activityClass);
        log("Observed sequence: " + observedTransitions);
        final String errorMessage = errorDuringTransition(activityClass, transition);

        assertEquals(errorMessage, expectedTransitions, observedTransitions);
    }

    /**
     * Assert that the observed transitions of a particular activity happened in expected order.
     * There may be more observed transitions than in the expected array, only their order matters.
     *
     * Use this method when there is no need to verify the entire sequence, only that some
     * transitions happened after another.
     */
    static void assertOrder(LifecycleLog lifecycleLog, Class<? extends Activity> activityClass,
            List<ActivityCallback> expectedTransitionsOrder, String transition) {
        List<Pair<String, ActivityCallback>> expectedTransitions = new ArrayList<>();
        for (ActivityCallback callback : expectedTransitionsOrder) {
            expectedTransitions.add(transition(activityClass, callback));
        }
        assertOrder(lifecycleLog, expectedTransitions, transition);
    }

    /**
     * Assert that the observed transitions happened in expected order. There may be more observed
     * transitions than in the expected array, only their order matters.
     *
     * Use this method when there is no need to verify the entire sequence, only that some
     * transitions happened after another.
     */
    static void assertOrder(LifecycleLog lifecycleLog,
            List<Pair<String, ActivityCallback>> expectedTransitionsOrder,
            String transition) {
        final List<Pair<String, ActivityCallback>> observedTransitions = lifecycleLog.getLog();
        int nextObservedPosition = 0;
        for (Pair<String, ActivityCallback> expectedTransition
                : expectedTransitionsOrder) {
            while (nextObservedPosition < observedTransitions.size()
                    && !observedTransitions.get(nextObservedPosition).equals(expectedTransition))
            {
                nextObservedPosition++;
            }
            if (nextObservedPosition == observedTransitions.size()) {
                fail("Transition wasn't observed in the expected position: " + expectedTransition
                        + " during transition: " + transition);
            }
        }
    }

    /**
     * Assert that a transition was observer, no particular order.
     */
    static void assertTransitionObserved(LifecycleLog lifecycleLog,
            Pair<String, ActivityCallback> expectedTransition, String transition) {
        assertTrue("Transition " + expectedTransition + " must be observed during " + transition,
                lifecycleLog.getLog().contains(expectedTransition));
    }

    /**
     * Assert that a transition was not observer, no particular order.
     */
    static void assertTransitionNotObserved(LifecycleLog lifecycleLog,
            Pair<String, ActivityCallback> expectedTransition, String transition) {
        assertFalse("Transition " + expectedTransition + " must not be observed during "
                        + transition, lifecycleLog.getLog().contains(expectedTransition));
    }

    static void assertEmptySequence(Class<? extends Activity> activityClass,
            LifecycleLog lifecycleLog, String transition) {
        assertSequence(activityClass, lifecycleLog, new ArrayList<>(), transition);
    }

    /** Assert that a lifecycle sequence matches one of the possible variants. */
    static void assertSequenceMatchesOneOf(Class<? extends Activity> activityClass,
            LifecycleLog lifecycleLog, List<List<ActivityCallback>> expectedTransitions,
            String transition) {
        final List<ActivityCallback> observedTransitions =
                lifecycleLog.getActivityLog(activityClass);
        log("Observed sequence: " + observedTransitions);
        final String errorMessage = errorDuringTransition(activityClass, transition);

        boolean oneOfExpectedSequencesObserved = false;
        for (List<ActivityCallback> transitionVariant : expectedTransitions) {
            if (transitionVariant.equals(observedTransitions)) {
                oneOfExpectedSequencesObserved = true;
                break;
            }
        }
        assertTrue(errorMessage + "\nObserved transitions: " + observedTransitions
                        + "\nExpected one of: " + expectedTransitions,
                oneOfExpectedSequencesObserved);
    }

    /** Assert the entire sequence for all involved activities. */
    static void assertEntireSequence(
            List<Pair<String, ActivityCallback>> expectedTransitions,
            LifecycleLog lifecycleLog, String message) {
        final List<Pair<String, ActivityCallback>> observedTransitions = lifecycleLog.getLog();
        assertEquals(message, expectedTransitions, observedTransitions);
    }

    static Pair<String, ActivityCallback> transition(Class<? extends Activity> activityClass,
            ActivityCallback state) {
        return new Pair<>(activityClass.getCanonicalName(), state);
    }

    private static String errorDuringTransition(Class<? extends Activity> activityClass,
            String transition) {
        return "Failed verification during moving activity: " + activityClass.getCanonicalName()
                + " through transition: " + transition;
    }
}
