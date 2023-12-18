/*
 * Copyright 2015 The Android Open Source Project
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

package android.hardware.input.cts.tests;

import static org.junit.Assert.assertEquals;
import static org.junit.Assert.assertNotEquals;
import static org.junit.Assert.assertTrue;
import static org.junit.Assert.fail;

import android.app.Instrumentation;
import android.hardware.input.cts.InputCallback;
import android.hardware.input.cts.InputCtsActivity;
import android.util.Log;
import android.view.InputDevice;
import android.view.InputEvent;
import android.view.KeyEvent;
import android.view.MotionEvent;
import android.view.View;

import androidx.annotation.NonNull;
import androidx.test.platform.app.InstrumentationRegistry;
import androidx.test.rule.ActivityTestRule;

import com.android.compatibility.common.util.PollingCheck;
import com.android.cts.input.InputJsonParser;

import org.junit.After;
import org.junit.Before;
import org.junit.Rule;

import java.util.ArrayList;
import java.util.List;
import java.util.concurrent.BlockingQueue;
import java.util.concurrent.CountDownLatch;
import java.util.concurrent.LinkedBlockingQueue;
import java.util.concurrent.TimeUnit;

public abstract class InputTestCase {
    private static final String TAG = "InputTestCase";
    private static final float TOLERANCE = 0.005f;

    private final BlockingQueue<InputEvent> mEvents;

    private InputListener mInputListener;
    private View mDecorView;

    protected Instrumentation mInstrumentation;
    protected InputJsonParser mParser;
    // Stores the name of the currently running test
    protected String mCurrentTestCase;
    private int mRegisterResourceId; // raw resource that contains json for registering a hid device
    protected int mVid;
    protected int mPid;

    // State used for motion events
    private int mLastButtonState;

    InputTestCase(int registerResourceId) {
        mEvents = new LinkedBlockingQueue<>();
        mInputListener = new InputListener();
        mRegisterResourceId = registerResourceId;
    }

    @Rule
    public ActivityTestRule<InputCtsActivity> mActivityRule =
        new ActivityTestRule<>(InputCtsActivity.class);

    @Before
    public void setUp() throws Exception {
        mInstrumentation = InstrumentationRegistry.getInstrumentation();
        mActivityRule.getActivity().clearUnhandleKeyCode();
        mDecorView = mActivityRule.getActivity().getWindow().getDecorView();
        mParser = new InputJsonParser(mInstrumentation.getTargetContext());
        mVid = mParser.readVendorId(mRegisterResourceId);
        mPid = mParser.readProductId(mRegisterResourceId);
        int deviceId = mParser.readDeviceId(mRegisterResourceId);
        String registerCommand = mParser.readRegisterCommand(mRegisterResourceId);
        setUpDevice(deviceId, mParser.readVendorId(mRegisterResourceId),
                mParser.readProductId(mRegisterResourceId),
                mParser.readSources(mRegisterResourceId), registerCommand);
        mEvents.clear();
    }

    @After
    public void tearDown() throws Exception {
        tearDownDevice();
    }

    // To be implemented by device specific test case.
    protected abstract void setUpDevice(int id, int vendorId, int productId, int sources,
            String registerCommand);

    protected abstract void tearDownDevice();

    protected abstract void testInputDeviceEvents(int resourceId);

    /**
     * Asserts that the application received a {@link android.view.KeyEvent} with the given
     * metadata.
     *
     * If other KeyEvents are received by the application prior to the expected KeyEvent, or no
     * KeyEvents are received within a reasonable amount of time, then this will throw an
     * {@link AssertionError}.
     *
     * Only action, source, keyCode and metaState are being compared.
     */
    private void assertReceivedKeyEvent(@NonNull KeyEvent expectedKeyEvent) {
        KeyEvent receivedKeyEvent = waitForKey();
        if (receivedKeyEvent == null) {
            failWithMessage("Did not receive " + expectedKeyEvent);
        }
        assertEquals(mCurrentTestCase + " (action)",
                expectedKeyEvent.getAction(), receivedKeyEvent.getAction());
        assertSource(mCurrentTestCase, expectedKeyEvent, receivedKeyEvent);
        assertEquals(mCurrentTestCase + " (keycode)",
                expectedKeyEvent.getKeyCode(), receivedKeyEvent.getKeyCode());
        assertMetaState(mCurrentTestCase, expectedKeyEvent.getMetaState(),
                receivedKeyEvent.getMetaState());
    }

    private void assertReceivedMotionEvent(@NonNull MotionEvent expectedEvent) {
        MotionEvent event = waitForMotion();
        /*
         If the test fails here, one thing to try is to forcefully add a delay after the device
         added callback has been received, but before any hid data has been written to the device.
         We already wait for all of the proper callbacks here and in other places of the stack, but
         it appears that the device sometimes is still not ready to receive hid data. If any data
         gets written to the device in that state, it will disappear,
         and no events will be generated.
          */

        if (event == null) {
            failWithMessage("Did not receive " + expectedEvent);
        }
        if (event.getHistorySize() > 0) {
            failWithMessage("expected each MotionEvent to only have a single entry");
        }
        assertEquals(mCurrentTestCase + " (action)",
                expectedEvent.getAction(), event.getAction());
        assertSource(mCurrentTestCase, expectedEvent, event);
        assertEquals(mCurrentTestCase + " (button state)",
                expectedEvent.getButtonState(), event.getButtonState());
        if (event.getActionMasked() == MotionEvent.ACTION_BUTTON_PRESS
                || event.getActionMasked() == MotionEvent.ACTION_BUTTON_RELEASE) {
            // Only checking getActionButton() for ACTION_BUTTON_PRESS or ACTION_BUTTON_RELEASE
            // because for actions other than ACTION_BUTTON_PRESS and ACTION_BUTTON_RELEASE the
            // returned value of getActionButton() is undefined.
            assertEquals(mCurrentTestCase + " (action button)",
                    mLastButtonState ^ event.getButtonState(), event.getActionButton());
            mLastButtonState = event.getButtonState();
        }
        assertAxis(mCurrentTestCase, expectedEvent, event);
    }

    /**
     * Asserts motion event axis values. Separate this into a different method to allow individual
     * test case to specify it.
     *
     * @param expectedSource expected source flag specified in JSON files.
     * @param actualSource actual source flag received in the test app.
     */
    void assertAxis(String testCase, MotionEvent expectedEvent, MotionEvent actualEvent) {
        for (int i = 0; i < actualEvent.getPointerCount(); i++) {
            for (int axis = MotionEvent.AXIS_X; axis <= MotionEvent.AXIS_GENERIC_16; axis++) {
                assertEquals(testCase + " pointer " + i
                        + " (" + MotionEvent.axisToString(axis) + ")",
                        expectedEvent.getAxisValue(axis, i), actualEvent.getAxisValue(axis, i),
                        TOLERANCE);
            }
        }
    }

    /**
     * Asserts source flags. Separate this into a different method to allow individual test case to
     * specify it.
     * The input source check verifies if actual source is equal or a subset of the expected source.
     * With Linux kernel 4.18 or later the input hid driver could register multiple evdev devices
     * when the HID descriptor has HID usages for different applications. Android frameworks will
     * create multiple KeyboardInputMappers for each of the evdev device, and each
     * KeyboardInputMapper will generate key events with source of the evdev device it belongs to.
     * As long as the source of these key events is a subset of expected source, we consider it as
     * a valid source.
     *
     * @param expected expected event with source flag specified in JSON files.
     * @param actual actual event with source flag received in the test app.
     */
    private void assertSource(String testCase, InputEvent expected, InputEvent actual) {
        assertNotEquals(testCase + " (source)", InputDevice.SOURCE_CLASS_NONE, actual.getSource());
        assertTrue(testCase + " (source)", expected.isFromSource(actual.getSource()));
    }

    /**
     * Asserts meta states. Separate this into a different method to allow individual test case to
     * specify it.
     *
     * @param expectedMetaState expected meta state specified in JSON files.
     * @param actualMetaState actual meta state received in the test app.
     */
    void assertMetaState(String testCase, int expectedMetaState, int actualMetaState) {
        assertEquals(testCase + " (meta state)", expectedMetaState, actualMetaState);
    }

    /**
     * Assert that no more events have been received by the application.
     *
     * If any more events have been received by the application, this will cause failure.
     */
    private void assertNoMoreEvents() {
        mInstrumentation.waitForIdleSync();
        InputEvent event = mEvents.poll();
        if (event == null) {
            return;
        }
        failWithMessage("extraneous events generated: " + event);
    }

    protected void verifyEvents(List<InputEvent> events) {
        mActivityRule.getActivity().setInputCallback(mInputListener);
        // Make sure we received the expected input events
        if (events.size() == 0) {
            // If no event is expected we need to wait for event until timeout and fail on
            // any unexpected event received caused by the HID report injection.
            InputEvent event = waitForEvent();
            if (event != null) {
                fail(mCurrentTestCase + " : Received unexpected event " + event);
            }
            return;
        }
        for (int i = 0; i < events.size(); i++) {
            final InputEvent event = events.get(i);
            try {
                if (event instanceof MotionEvent) {
                    assertReceivedMotionEvent((MotionEvent) event);
                    continue;
                }
                if (event instanceof KeyEvent) {
                    assertReceivedKeyEvent((KeyEvent) event);
                    continue;
                }
            } catch (AssertionError error) {
                throw new AssertionError("Assertion on entry " + i + " failed.", error);
            }
            fail("Entry " + i + " is neither a KeyEvent nor a MotionEvent: " + event);
        }
        assertNoMoreEvents();
    }

    protected void testInputEvents(int resourceId) {
        testInputDeviceEvents(resourceId);
        assertNoMoreEvents();
    }

    private InputEvent waitForEvent() {
        try {
            return mEvents.poll(1, TimeUnit.SECONDS);
        } catch (InterruptedException e) {
            failWithMessage("unexpectedly interrupted while waiting for InputEvent");
            return null;
        }
    }

    // Ignore Motion event received during the 5 seconds timeout period. Return on the first Key
    // event received.
    private KeyEvent waitForKey() {
        for (int i = 0; i < 5; i++) {
            InputEvent event = waitForEvent();
            if (event instanceof KeyEvent) {
                return (KeyEvent) event;
            }
        }
        return null;
    }

    // Ignore Key event received during the 5 seconds timeout period. Return on the first Motion
    // event received.
    private MotionEvent waitForMotion() {
        for (int i = 0; i < 5; i++) {
            InputEvent event = waitForEvent();
            if (event instanceof MotionEvent) {
                return (MotionEvent) event;
            }
        }
        return null;
    }

    /**
     * Since MotionEvents are batched together based on overall system timings (i.e. vsync), we
     * can't rely on them always showing up batched in the same way. In order to make sure our
     * test results are consistent, we instead split up the batches so they end up in a
     * consistent and reproducible stream.
     *
     * Note, however, that this ignores the problem of resampling, as we still don't know how to
     * distinguish resampled events from real events. Only the latter will be consistent and
     * reproducible.
     *
     * @param event The (potentially) batched MotionEvent
     * @return List of MotionEvents, with each event guaranteed to have zero history size, and
     * should otherwise be equivalent to the original batch MotionEvent.
     */
    private static List<MotionEvent> splitBatchedMotionEvent(MotionEvent event) {
        List<MotionEvent> events = new ArrayList<>();
        final int historySize = event.getHistorySize();
        final int pointerCount = event.getPointerCount();
        MotionEvent.PointerProperties[] properties =
                new MotionEvent.PointerProperties[pointerCount];
        MotionEvent.PointerCoords[] currentCoords = new MotionEvent.PointerCoords[pointerCount];
        for (int p = 0; p < pointerCount; p++) {
            properties[p] = new MotionEvent.PointerProperties();
            event.getPointerProperties(p, properties[p]);
            currentCoords[p] = new MotionEvent.PointerCoords();
            event.getPointerCoords(p, currentCoords[p]);
        }
        for (int h = 0; h < historySize; h++) {
            long eventTime = event.getHistoricalEventTime(h);
            MotionEvent.PointerCoords[] coords = new MotionEvent.PointerCoords[pointerCount];

            for (int p = 0; p < pointerCount; p++) {
                coords[p] = new MotionEvent.PointerCoords();
                event.getHistoricalPointerCoords(p, h, coords[p]);
            }
            MotionEvent singleEvent =
                    MotionEvent.obtain(event.getDownTime(), eventTime, event.getAction(),
                            pointerCount, properties, coords,
                            event.getMetaState(), event.getButtonState(),
                            event.getXPrecision(), event.getYPrecision(),
                            event.getDeviceId(), event.getEdgeFlags(),
                            event.getSource(), event.getFlags());
            singleEvent.setActionButton(event.getActionButton());
            events.add(singleEvent);
        }

        MotionEvent singleEvent =
                MotionEvent.obtain(event.getDownTime(), event.getEventTime(), event.getAction(),
                        pointerCount, properties, currentCoords,
                        event.getMetaState(), event.getButtonState(),
                        event.getXPrecision(), event.getYPrecision(),
                        event.getDeviceId(), event.getEdgeFlags(),
                        event.getSource(), event.getFlags());
        singleEvent.setActionButton(event.getActionButton());
        events.add(singleEvent);
        return events;
    }

    /**
     * Append the name of the currently executing test case to the fail message.
     * Dump out the events queue to help debug.
     */
    private void failWithMessage(String message) {
        if (mEvents.isEmpty()) {
            Log.i(TAG, "The events queue is empty");
        } else {
            Log.e(TAG, "There are additional events received by the test activity:");
            for (InputEvent event : mEvents) {
                Log.i(TAG, event.toString());
            }
        }
        fail(mCurrentTestCase + ": " + message);
    }

    private class InputListener implements InputCallback {
        @Override
        public void onKeyEvent(KeyEvent ev) {
            try {
                mEvents.put(new KeyEvent(ev));
            } catch (InterruptedException ex) {
                failWithMessage("interrupted while adding a KeyEvent to the queue");
            }
        }

        @Override
        public void onMotionEvent(MotionEvent ev) {
            try {
                for (MotionEvent event : splitBatchedMotionEvent(ev)) {
                    mEvents.put(event);
                }
            } catch (InterruptedException ex) {
                failWithMessage("interrupted while adding a MotionEvent to the queue");
            }
        }
    }

    protected void requestFocusSync() {
        mActivityRule.getActivity().runOnUiThread(() -> {
            mDecorView.setFocusable(true);
            mDecorView.setFocusableInTouchMode(true);
            mDecorView.requestFocus();
        });
        PollingCheck.waitFor(mDecorView::hasFocus);
    }

    protected class PointerCaptureSession implements AutoCloseable {
        protected PointerCaptureSession() {
            requestFocusSync();
            ensurePointerCaptureState(true);
        }

        @Override
        public void close() {
            ensurePointerCaptureState(false);
        }

        private void ensurePointerCaptureState(boolean enable) {
            final CountDownLatch latch = new CountDownLatch(1);
            mActivityRule.getActivity().setPointerCaptureCallback(hasCapture -> {
                if (enable == hasCapture) {
                    latch.countDown();
                }
            });
            mActivityRule.getActivity().runOnUiThread(enable ? mDecorView::requestPointerCapture
                    : mDecorView::releasePointerCapture);
            try {
                if (!latch.await(60, TimeUnit.SECONDS)) {
                    throw new IllegalStateException(
                            "Did not receive callback after "
                                    + (enable ? "enabling" : "disabling")
                                    + " Pointer Capture.");
                }
            } catch (InterruptedException e) {
                throw new IllegalStateException(
                        "Interrupted while waiting for Pointer Capture state.");
            } finally {
                mActivityRule.getActivity().setPointerCaptureCallback(null);
            }
            assertEquals("The view's Pointer Capture state did not match.", enable,
                    mDecorView.hasPointerCapture());
        }
    }
}
