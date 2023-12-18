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

import static android.server.wm.StateLogger.log;

import static androidx.test.platform.app.InstrumentationRegistry.getInstrumentation;

import android.app.Instrumentation;
import android.content.Context;
import android.graphics.Rect;
import android.os.SystemClock;
import android.view.InputDevice;
import android.view.KeyEvent;
import android.view.MotionEvent;
import android.view.View;
import android.view.ViewConfiguration;

public class TouchHelper {
    public final Context mContext;
    public final Instrumentation mInstrumentation;
    public final WindowManagerStateHelper mWmState;

    public TouchHelper(Instrumentation instrumentation, WindowManagerStateHelper wmState) {
        mInstrumentation = instrumentation;
        mContext = mInstrumentation.getContext();
        mWmState = wmState;
    }

    /**
     * Insert an input event (ACTION_DOWN -> ACTION_CANCEL) to ensures the display to be focused
     * without triggering potential clicked to impact the test environment.
     * (e.g: Keyguard credential activated unexpectedly.)
     *
     * @param displayId the display ID to gain focused by inject swipe action
     */
    public void touchAndCancelOnDisplayCenterSync(int displayId) {
        WindowManagerState.DisplayContent dc = mWmState.getDisplay(displayId);
        if (dc == null) {
            // never get wm state before?
            mWmState.computeState();
            dc = mWmState.getDisplay(displayId);
        }
        if (dc == null) {
            log("Cannot tap on display: " + displayId);
            return;
        }
        final Rect bounds = dc.getDisplayRect();
        final int x = bounds.left + bounds.width() / 2;
        final int y = bounds.top + bounds.height() / 2;
        final long downTime = SystemClock.uptimeMillis();
        injectMotion(downTime, downTime, MotionEvent.ACTION_DOWN, x, y, displayId, true /* sync */);

        final long eventTime = SystemClock.uptimeMillis();
        final int touchSlop = ViewConfiguration.get(mContext).getScaledTouchSlop();
        final int tapX = x + Math.round(touchSlop / 2.0f);
        final int tapY = y + Math.round(touchSlop / 2.0f);
        injectMotion(downTime, eventTime, MotionEvent.ACTION_CANCEL, tapX, tapY, displayId,
                true /* sync */);
    }

    public void tapOnDisplaySync(int x, int y, int displayId) {
        tapOnDisplay(x, y, displayId, true /* sync*/);
    }

    public void tapOnDisplay(int x, int y, int displayId, boolean sync) {
        tapOnDisplay(x, y, displayId, sync, /* waitAnimations */ true);
    }

    public void tapOnDisplay(int x, int y, int displayId, boolean sync, boolean waitAnimations) {
        final long downTime = SystemClock.uptimeMillis();
        injectMotion(downTime, downTime, MotionEvent.ACTION_DOWN, x, y, displayId, sync,
                waitAnimations);

        final long upTime = SystemClock.uptimeMillis();
        injectMotion(downTime, upTime, MotionEvent.ACTION_UP, x, y, displayId, sync,
                waitAnimations);

        if (waitAnimations) {
            mWmState.waitForWithAmState(state -> state.getFocusedDisplayId() == displayId,
                    "top focused displayId: " + displayId);
        }
        // This is needed after a tap in multi-display to ensure that the display focus has really
        // changed, if needed. The call to syncInputTransaction will wait until focus change has
        // propagated from WMS to native input before returning.
        mInstrumentation.getUiAutomation().syncInputTransactions(waitAnimations);
    }

    public void tapOnCenter(Rect bounds, int displayId) {
        final int tapX = bounds.left + bounds.width() / 2;
        final int tapY = bounds.top + bounds.height() / 2;
        tapOnDisplaySync(tapX, tapY, displayId);
    }

    public void tapOnViewCenter(View view) {
        tapOnViewCenter(view, true /* waitAnimations */);
    }

    public void tapOnViewCenter(View view, boolean waitAnimations) {
        final int[] topleft = new int[2];
        view.getLocationOnScreen(topleft);
        int x = topleft[0] + view.getWidth() / 2;
        int y = topleft[1] + view.getHeight() / 2;
        tapOnDisplay(x, y, view.getDisplay().getDisplayId(), true /* sync */, waitAnimations);
    }

    public void tapOnStackCenter(WindowManagerState.ActivityTask stack) {
        tapOnCenter(stack.getBounds(), stack.mDisplayId);
    }

    public void tapOnDisplayCenter(int displayId) {
        final Rect bounds = mWmState.getDisplay(displayId).getDisplayRect();
        tapOnDisplaySync(bounds.centerX(), bounds.centerY(), displayId);
    }

    public void tapOnDisplayCenterAsync(int displayId) {
        final Rect bounds = mWmState.getDisplay(displayId).getDisplayRect();
        tapOnDisplay(bounds.centerX(), bounds.centerY(), displayId, false /* sync */);
    }

    public static void injectMotion(long downTime, long eventTime, int action,
            int x, int y, int displayId, boolean sync) {
        injectMotion(downTime, eventTime, action, x, y, displayId, sync,
                true /* waitForAnimations */);
    }

    public static void injectMotion(long downTime, long eventTime, int action,
            int x, int y, int displayId, boolean sync, boolean waitAnimations) {
        final MotionEvent event = MotionEvent.obtain(downTime, eventTime, action,
                x, y, 0 /* metaState */);
        event.setSource(InputDevice.SOURCE_TOUCHSCREEN);
        event.setDisplayId(displayId);
        getInstrumentation().getUiAutomation().injectInputEvent(event, sync, waitAnimations);
    }

    public static void injectKey(int keyCode, boolean longPress, boolean sync) {
        final long downTime = SystemClock.uptimeMillis();
        int repeatCount = 0;
        KeyEvent downEvent =
                new KeyEvent(downTime, downTime, KeyEvent.ACTION_DOWN, keyCode, repeatCount);
        getInstrumentation().getUiAutomation().injectInputEvent(downEvent, sync);
        if (longPress) {
            repeatCount += 1;
            KeyEvent repeatEvent = new KeyEvent(downTime, SystemClock.uptimeMillis(),
                    KeyEvent.ACTION_DOWN, keyCode, repeatCount);
            getInstrumentation().getUiAutomation().injectInputEvent(repeatEvent, sync);
        }
        KeyEvent upEvent = new KeyEvent(downTime, SystemClock.uptimeMillis(),
                KeyEvent.ACTION_UP, keyCode, 0 /* repeatCount */);
        getInstrumentation().getUiAutomation().injectInputEvent(upEvent, sync);
    }
}
