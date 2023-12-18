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

package android.view.cts;

import static org.junit.Assert.assertEquals;
import static org.junit.Assert.assertNotEquals;
import static org.junit.Assert.assertNotNull;

import android.app.Instrumentation;
import android.view.InputDevice;
import android.view.KeyEvent;

import androidx.annotation.NonNull;
import androidx.test.ext.junit.runners.AndroidJUnit4;
import androidx.test.filters.MediumTest;
import androidx.test.platform.app.InstrumentationRegistry;
import androidx.test.rule.ActivityTestRule;

import com.android.compatibility.common.util.PollingCheck;
import com.android.cts.input.InputJsonParser;
import com.android.cts.input.UinputDevice;

import org.json.JSONArray;
import org.json.JSONException;
import org.json.JSONObject;
import org.junit.After;
import org.junit.Before;
import org.junit.Rule;
import org.junit.Test;
import org.junit.runner.RunWith;

import java.util.Arrays;
import java.util.List;
import java.util.Map;

/**
 * CTS test case for generic.kl key layout mapping.
 * This test utilize uinput command line tool to create a test device, and configure the virtual
 * device to have all keys need to be tested. The JSON format input for device configuration
 * and EV_KEY injection will be created directly from this test for uinput command.
 * Keep res/raw/Generic.kl in sync with framework/base/data/keyboards/Generic.kl, this file
 * will be loaded and parsed in this test, looping through all key labels and the corresponding
 * EV_KEY code, injecting the KEY_UP and KEY_DOWN event to uinput, then verify the KeyEvent
 * delivered to test application view. Except meta control keys and special keys not delivered
 * to apps, all key codes in generic.kl will be verified.
 *
 */
@MediumTest
@RunWith(AndroidJUnit4.class)
public class InputDeviceKeyLayoutMapTest {
    private static final String TAG = "InputDeviceKeyLayoutMapTest";
    private static final String LABEL_PREFIX = "KEYCODE_";
    private static final int DEVICE_ID = 1;
    private static final int EV_SYN = 0;
    private static final int EV_KEY = 1;
    private static final int EV_KEY_DOWN = 1;
    private static final int EV_KEY_UP = 0;
    private static final int UI_SET_EVBIT = 100;
    private static final int UI_SET_KEYBIT = 101;
    private static final int GOOGLE_VENDOR_ID = 0x18d1;
    private static final int GOOGLE_VIRTUAL_KEYBOARD_ID = 0x001f;
    private static final int POLL_EVENT_TIMEOUT_SECONDS = 1;
    private static final int RETRY_COUNT = 10;

    private Map<String, Integer> mKeyLayout;
    private Instrumentation mInstrumentation;
    private UinputDevice mUinputDevice;
    private int mMetaState;
    private InputJsonParser mParser;

    private static native Map<String, Integer> nativeLoadKeyLayout(String genericKeyLayout);

    static {
        System.loadLibrary("ctsview_jni");
    }

    @Rule
    public ActivityTestRule<InputDeviceKeyLayoutMapTestActivity> mActivityRule =
            new ActivityTestRule<>(InputDeviceKeyLayoutMapTestActivity.class);

    @Before
    public void setup() {
        mInstrumentation = InstrumentationRegistry.getInstrumentation();
        PollingCheck.waitFor(mActivityRule.getActivity()::hasWindowFocus);
        mParser = new InputJsonParser(mInstrumentation.getTargetContext());
        mKeyLayout = nativeLoadKeyLayout(mParser.readRegisterCommand(R.raw.Generic));
        mUinputDevice = new UinputDevice(mInstrumentation, DEVICE_ID, GOOGLE_VENDOR_ID,
                GOOGLE_VIRTUAL_KEYBOARD_ID, InputDevice.SOURCE_KEYBOARD,
                createDeviceRegisterCommand());

        mMetaState = KeyEvent.META_NUM_LOCK_ON;
    }

    @After
    public void tearDown() {
        if (mUinputDevice != null) {
            mUinputDevice.close();
        }
    }

    /**
     * Get a KeyEvent from event queue or timeout.
     * The test activity instance may change in the middle, calling getKeyEvent with the old
     * activity instance will get timed out when test activity instance changed. Rather than
     * doing a long wait for timeout with same activity instance, break the polling into a number
     * of retries and each time of retry call the ActivityTestRule.getActivity for current activity
     * instance to avoid the test failure because of polling the old activity instance get timed
     * out consequently failed the test.
     *
     * @param retryCount The times to retry get KeyEvent from test activity.
     *
     * @return KeyEvent delivered to test activity, null if timeout.
     */
    private KeyEvent getKeyEvent(int retryCount) {
        for (int i = 0; i < retryCount; i++) {
            KeyEvent event = mActivityRule.getActivity().getKeyEvent(POLL_EVENT_TIMEOUT_SECONDS);
            if (event != null) {
                return event;
            }
        }
        return null;
    }

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
        if (expectedKeyEvent.getKeyCode() == KeyEvent.KEYCODE_UNKNOWN) {
            return;
        }

        KeyEvent receivedKeyEvent = getKeyEvent(RETRY_COUNT);
        String log = "Expected " + expectedKeyEvent + " Received " + receivedKeyEvent;
        assertNotNull(log, receivedKeyEvent);
        assertEquals(log, expectedKeyEvent.getAction(), receivedKeyEvent.getAction());
        assertEquals(log, expectedKeyEvent.getSource(), receivedKeyEvent.getSource());
        assertEquals(log, expectedKeyEvent.getKeyCode(), receivedKeyEvent.getKeyCode());
        assertEquals(log, expectedKeyEvent.getMetaState(), receivedKeyEvent.getMetaState());
    }

    /**
     * Create the uinput device registration command, in JSON format of uinput commandline tool.
     * Refer to {@link framework/base/cmds/uinput/README.md}
     */
    private String createDeviceRegisterCommand() {
        JSONObject json = new JSONObject();
        JSONArray arrayConfigs =  new JSONArray();
        try {
            json.put("id", DEVICE_ID);
            json.put("type", "uinput");
            json.put("command", "register");
            json.put("name", "Virtual All Buttons Device (Test)");
            json.put("vid", GOOGLE_VENDOR_ID);
            json.put("pid", GOOGLE_VIRTUAL_KEYBOARD_ID);
            json.put("bus", "bluetooth");

            JSONObject jsonSetEvBit = new JSONObject();
            JSONArray arraySetEvBit =  new JSONArray();
            arraySetEvBit.put(EV_KEY);
            jsonSetEvBit.put("type", UI_SET_EVBIT);
            jsonSetEvBit.put("data", arraySetEvBit);
            arrayConfigs.put(jsonSetEvBit);

            // Configure device have all keys from key layout map.
            JSONArray arraySetKeyBit = new JSONArray();
            for (Map.Entry<String, Integer> entry : mKeyLayout.entrySet()) {
                arraySetKeyBit.put(entry.getValue());
            }
            JSONObject jsonSetKeyBit = new JSONObject();
            jsonSetKeyBit.put("type", UI_SET_KEYBIT);
            jsonSetKeyBit.put("data", arraySetKeyBit);
            arrayConfigs.put(jsonSetKeyBit);
            json.put("configuration", arrayConfigs);
        } catch (JSONException e) {
            throw new RuntimeException(
                    "Could not create JSON object");
        }

        return json.toString();
    }

    /**
     * Update expected meta state for incoming key event.
     * @param action KeyEvent.ACTION_DOWN or KeyEvent.ACTION_UP
     * @param label Key label from key layout mapping definition
     * @return updated meta state
     */

    private int updateMetaState(int action, String label) {

        int metaState = 0;
        int metaStateToggle = 0;
        if (label.equals("CTRL_LEFT")) {
            metaState = KeyEvent.META_CTRL_ON | KeyEvent.META_CTRL_LEFT_ON;
        }
        if (label.equals("CTRL_RIGHT")) {
            metaState = KeyEvent.META_CTRL_ON | KeyEvent.META_CTRL_RIGHT_ON;
        }
        if (label.equals("SHIFT_LEFT")) {
            metaState = KeyEvent.META_SHIFT_ON | KeyEvent.META_SHIFT_LEFT_ON;
        }
        if (label.equals("SHIFT_RIGHT")) {
            metaState = KeyEvent.META_SHIFT_ON | KeyEvent.META_SHIFT_RIGHT_ON;
        }
        if (label.equals("ALT_LEFT")) {
            metaState = KeyEvent.META_ALT_ON | KeyEvent.META_ALT_LEFT_ON;
        }
        if (label.equals("ALT_RIGHT")) {
            metaState = KeyEvent.META_ALT_ON | KeyEvent.META_ALT_RIGHT_ON;
        }
        if (label.equals("CAPS_LOCK")) {
            metaStateToggle =  KeyEvent.META_CAPS_LOCK_ON;
        }
        if (label.equals("NUM_LOCK")) {
            metaStateToggle =  KeyEvent.META_NUM_LOCK_ON;
        }
        if (label.equals("SCROLL_LOCK")) {
            metaStateToggle =  KeyEvent.META_SCROLL_LOCK_ON;
        }

        if (action == KeyEvent.ACTION_DOWN) {
            mMetaState |= metaState;
        } else if (action == KeyEvent.ACTION_UP) {
            mMetaState &= ~metaState;
        }

        if (action == KeyEvent.ACTION_UP) {
            if ((mMetaState & metaStateToggle) == 0) {
                mMetaState |= metaStateToggle;
            } else {
                mMetaState &= ~metaStateToggle;
            }
        }
        return mMetaState;
    }

    /**
     * Generate a key event from the key label and action.
     * @param action KeyEvent.ACTION_DOWN or KeyEvent.ACTION_UP
     * @param label Key label from key layout mapping definition
     * @return KeyEvent expected to receive
     */
    private KeyEvent generateKeyEvent(int action, String label) {
        int source = InputDevice.SOURCE_KEYBOARD | InputDevice.SOURCE_GAMEPAD
                | InputDevice.SOURCE_DPAD;
        int keyCode = KeyEvent.keyCodeFromString(LABEL_PREFIX + label);
        int metaState = updateMetaState(action, label);
        // We will only check select fields of the KeyEvent. Times are not checked.
        KeyEvent event = new KeyEvent(/* downTime */ 0, /* eventTime */ 0, action, keyCode,
                /* repeat */ 0, metaState, /* deviceId */ 0, /* scanCode */ 0,
                /* flags */ 0, source);

        return event;
    }

    /**
     * Simulate pressing a key.
     * @param evKeyCode The key scan code
     */
    private void pressKey(int evKeyCode) {
        int[] evCodesDown = new int[] {
                EV_KEY, evKeyCode, EV_KEY_DOWN,
                EV_SYN, 0, 0};
        mUinputDevice.injectEvents(Arrays.toString(evCodesDown));

        int[] evCodesUp = new int[] {
                EV_KEY, evKeyCode, EV_KEY_UP,
                EV_SYN, 0, 0 };
        mUinputDevice.injectEvents(Arrays.toString(evCodesUp));
    }

    /**
     * Check the initial global meta key state.
     * @param label Key label from key layout mapping definition
     * @param metaState The meta state that the meta key changes
     */
    private void checkMetaKeyState(String label, int metaState) {
        int eveKeyCode = mKeyLayout.get(label);
        pressKey(eveKeyCode);
        // Get 2 key events for up and down.
        KeyEvent keyDownEvent = getKeyEvent(RETRY_COUNT);
        assertNotNull("Didn't get KeyDown event " + label, keyDownEvent);
        KeyEvent keyUpEvent = getKeyEvent(RETRY_COUNT);
        assertNotNull("Didn't get KeyUp event " + label, keyUpEvent);

        if (keyUpEvent.getKeyCode() == KeyEvent.keyCodeFromString(label)
                && keyUpEvent.getAction() == KeyEvent.ACTION_UP) {
            mMetaState &= ~metaState;
            mMetaState |= (keyUpEvent.getMetaState() & metaState);
        }
    }

    /**
     * Initialize NUM_LOCK, CAPS_LOCK, SCROLL_LOCK state as they are global meta state
     */
    private void initializeMetaKeysState() {
        // Detect NUM_LOCK key state before test.
        checkMetaKeyState("NUM_LOCK", KeyEvent.META_NUM_LOCK_ON);
        // Detect CAPS_LOCK key state before test.
        checkMetaKeyState("CAPS_LOCK", KeyEvent.META_CAPS_LOCK_ON);
        // Detect CAPS_LOCK key state before test.
        checkMetaKeyState("SCROLL_LOCK", KeyEvent.META_SCROLL_LOCK_ON);
    }

    @Test
    public void testLayoutKeyEvents() {
        final List<String> excludedKeys = Arrays.asList(
                // Meta control keys.
                "CAPS_LOCK", "NUM_LOCK", "SCROLL_LOCK", "META_LEFT", "META_RIGHT", "FUNCTION",
                // KeyEvents not delivered to apps.
                "APP_SWITCH", "SYSRQ", "ASSIST", "VOICE_ASSIST",
                "HOME", "POWER", "SLEEP", "WAKEUP",
                "BRIGHTNESS_UP", "BRIGHTNESS_DOWN");

        initializeMetaKeysState();

        for (Map.Entry<String, Integer> entry : mKeyLayout.entrySet()) {
            String label = LABEL_PREFIX + entry.getKey();
            int evKeyCode = entry.getValue();

            if (excludedKeys.contains(label)) {
                continue;
            }

            assertNotEquals(KeyEvent.keyCodeFromString(label), KeyEvent.KEYCODE_UNKNOWN);
            // Press the key
            pressKey(evKeyCode);
            // Generate expected key down event and verify
            KeyEvent expectedDownEvent = generateKeyEvent(KeyEvent.ACTION_DOWN,  label);
            assertReceivedKeyEvent(expectedDownEvent);
            // Generate expected key up event and verify
            KeyEvent expectedUpEvent = generateKeyEvent(KeyEvent.ACTION_UP,  label);
            assertReceivedKeyEvent(expectedUpEvent);
        }
    }

}
