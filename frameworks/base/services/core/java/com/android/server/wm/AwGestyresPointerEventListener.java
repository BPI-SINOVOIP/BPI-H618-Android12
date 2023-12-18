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
 * limitations under the License.
 */

package com.android.server.wm;

import android.app.ActivityManager;
import android.content.Context;
import android.content.ComponentName;
import android.content.Intent;
import android.content.pm.ActivityInfo;
import android.hardware.input.InputManager;
import android.os.Handler;
import android.os.SystemClock;
import android.os.UserHandle;
import android.provider.Settings;
import android.util.Log;
import android.util.Slog;
import android.view.DisplayInfo;
import android.view.GestureDetector;
import android.view.KeyEvent;
import android.view.KeyCharacterMap;
import android.view.InputDevice;
import android.view.MotionEvent;
import android.view.WindowManagerPolicyConstants.PointerEventListener;

/**
 * Listens for system-wide input gestures, firing callbacks when detected.
 * @hide
 */
class AwGesturesPointerEventListener implements PointerEventListener {
    private static final String TAG = "AwGestures";
    private static final boolean DEBUG = Log.isLoggable(TAG, Log.DEBUG);
    private static final long SWIPE_TIMEOUT_MS = 500;
    private static final int MAX_TRACKED_POINTERS = 5;  // max per input system
    private static final int UNTRACKED_POINTER = -1;

    private static final int SWIPE_NONE = 0;
    private static final int SWIPE_FROM_TOP = 1;
    private static final int SWIPE_FROM_BOTTOM = 2;
    private static final int SWIPE_FROM_RIGHT = 3;
    private static final int SWIPE_FROM_LEFT = 4;
    private static final String GESTURE_SCREENSHOT_ENABLE = "gesture_screenshot_enable";
    private static final String GESTURE_SCREENRECORD_ENABLE = "gesture_screenrecord_enable";
    private static final String RECORDSERVICE = "com.android.systemui.screenrecord.RecordingService";
    private static final String RECORDDIALOGPACKAGE = "com.android.systemui";
    private static final String RECORDDIALOGCLASS = "com.android.systemui.screenrecord.ScreenRecordDialog";
    private static final String ACTION_STOP = "com.android.systemui.screenrecord.STOP";

    private final Context mContext;
    private final Handler mHandler;
    private int mDisplayCutoutTouchableRegionSize;
    private int mSwipeStartThreshold = 100;
    private int mSwipeDistanceThreshold = 50;
    private final int[] mDownPointerId = new int[MAX_TRACKED_POINTERS];
    private final float[] mDownX = new float[MAX_TRACKED_POINTERS];
    private final float[] mDownY = new float[MAX_TRACKED_POINTERS];
    private final long[] mDownTime = new long[MAX_TRACKED_POINTERS];

    private GestureDetector mGestureDetector;

    int screenHeight;
    int screenWidth;
    private int mDownPointers;
    private boolean mSwipeFireable;
    private boolean mDebugFireable;
    private boolean mMouseHoveringAtEdge;
    private long mLastFlingTime;

    AwGesturesPointerEventListener(Context context, Handler handler) {
        mContext = context;
        mHandler = handler;
    }

    void onDisplayInfoChanged(DisplayInfo info) {
        screenWidth = info.logicalWidth;
        screenHeight = info.logicalHeight;
    }

    public void systemReady() {
         // GestureDetector records statistics about gesture classification events to inform gesture
         // usage trends. SystemGesturesPointerEventListener creates a lot of noise in these
         // statistics because it passes every touch event though a GestureDetector. By creating an
         // anonymous subclass of GestureDetector, these statistics will be recorded with a unique
         // source name that can be filtered.

         // GestureDetector would get a ViewConfiguration instance by context, that may also
         // create a new WindowManagerImpl for the new display, and lock WindowManagerGlobal
         // temporarily in the constructor that would make a deadlock.
         mHandler.post(() -> mGestureDetector =
                new GestureDetector(mContext, new FlingGestureDetector(), mHandler) {});
    }



    private void sendDownAndUpKeyEvents(int keyCode) {
        final long downTime = SystemClock.uptimeMillis();
        sendKeyEventIdentityCleared(keyCode, KeyEvent.ACTION_DOWN, downTime, downTime);
        sendKeyEventIdentityCleared(keyCode, KeyEvent.ACTION_UP, downTime, SystemClock.uptimeMillis());
    }

    private void sendKeyEventIdentityCleared(int keyCode, int action, long downTime, long time) {
        KeyEvent event = KeyEvent.obtain(downTime, time, action, keyCode, 0, 0,
                KeyCharacterMap.VIRTUAL_KEYBOARD, 0, KeyEvent.FLAG_FROM_SYSTEM,
                InputDevice.SOURCE_KEYBOARD, null);
        InputManager.getInstance()
            .injectInputEvent(event, InputManager.INJECT_INPUT_EVENT_MODE_ASYNC);
        event.recycle();
    }

    private void runScreenShot() {
        if (Settings.System.getIntForUser(mContext.getContentResolver(),
                    GESTURE_SCREENSHOT_ENABLE, 0, UserHandle.USER_CURRENT) == 1) {
            Slog.d(TAG, "runScreenShot");
            sendDownAndUpKeyEvents(KeyEvent.KEYCODE_SYSRQ);
        }
    }

    private boolean isRecording() {
        ActivityManager am = (ActivityManager) mContext.getSystemService(Context.ACTIVITY_SERVICE);
        for (ActivityManager.RunningServiceInfo info : am.getRunningServices(Integer.MAX_VALUE)) {
            if (RECORDSERVICE.equals(info.service.getClassName())) {
                return true;
            }
        }
        return false;
    }

    private void startScreenRecord() {
        Slog.d(TAG, "startScreenRecord from gesture");
        Intent intent = new Intent().setComponent(new ComponentName(RECORDDIALOGPACKAGE, RECORDDIALOGCLASS));
        intent.addFlags(Intent.FLAG_ACTIVITY_NEW_TASK);
        mContext.startActivity(intent);
    }

    private void stopScreenRecord() {
        Slog.d(TAG, "stopScreenRecord from gesture");
        Intent intent = new Intent().setComponent(new ComponentName(RECORDDIALOGPACKAGE, RECORDSERVICE));
        intent.setAction(ACTION_STOP);
        mContext.startService(intent);
    }

    private void runScreenRecord() {
        if (Settings.System.getIntForUser(mContext.getContentResolver(),
                    GESTURE_SCREENRECORD_ENABLE, 0, UserHandle.USER_CURRENT) == 1) {
            if (isRecording()) {
                stopScreenRecord();
            } else {
                startScreenRecord();
            }
        }
    }

    @Override
    public void onPointerEvent(MotionEvent event) {
        if (mGestureDetector != null && event.isTouchEvent()) {
            mGestureDetector.onTouchEvent(event);
        }
        switch (event.getActionMasked()) {
            case MotionEvent.ACTION_DOWN:
                mSwipeFireable = true;
                mDebugFireable = true;
                mDownPointers = 0;
                captureDown(event, 0);
                if (mMouseHoveringAtEdge) {
                    mMouseHoveringAtEdge = false;
                }
                break;
            case MotionEvent.ACTION_POINTER_DOWN:
                captureDown(event, event.getActionIndex());
                if (mDebugFireable) {
                    mDebugFireable = event.getPointerCount() < 5;
                    if (!mDebugFireable) {
                        if (DEBUG) Slog.d(TAG, "Firing debug");
                    }
                }
                break;
            case MotionEvent.ACTION_MOVE:
                if (mSwipeFireable) {
                    if (event.getPointerCount() == 2) {
                        final int swipe = detectSwipe(event);
                        mSwipeFireable = swipe == SWIPE_NONE;
                        if (swipe == SWIPE_FROM_RIGHT) {
                            if (DEBUG) Slog.d(TAG, "Firing onSwipeFromRight");
                            runScreenShot();
                        } else if (swipe == SWIPE_FROM_LEFT) {
                            if (DEBUG) Slog.d(TAG, "Firing onSwipeFromLeft");
                            runScreenRecord();
                        }
                    }
                }
                break;
            case MotionEvent.ACTION_HOVER_MOVE:
                break;
            case MotionEvent.ACTION_UP:
            case MotionEvent.ACTION_CANCEL:
                mSwipeFireable = false;
                mDebugFireable = false;
                break;
            default:
                if (DEBUG) Slog.d(TAG, "Ignoring " + event);
        }
    }

    private void captureDown(MotionEvent event, int pointerIndex) {
        final int pointerId = event.getPointerId(pointerIndex);
        final int i = findIndex(pointerId);
        if (DEBUG) Slog.d(TAG, "pointer " + pointerId
                + " down pointerIndex=" + pointerIndex + " trackingIndex=" + i);
        if (i != UNTRACKED_POINTER) {
            mDownX[i] = event.getX(pointerIndex);
            mDownY[i] = event.getY(pointerIndex);
            mDownTime[i] = event.getEventTime();
            if (DEBUG) Slog.d(TAG, "pointer " + pointerId
                    + " down x=" + mDownX[i] + " y=" + mDownY[i]);
        }
    }

    private int findIndex(int pointerId) {
        for (int i = 0; i < mDownPointers; i++) {
            if (mDownPointerId[i] == pointerId) {
                return i;
            }
        }
        if (mDownPointers == MAX_TRACKED_POINTERS || pointerId == MotionEvent.INVALID_POINTER_ID) {
            return UNTRACKED_POINTER;
        }
        mDownPointerId[mDownPointers++] = pointerId;
        return mDownPointers - 1;
    }

    private int detectSwipe(MotionEvent move) {
        final int historySize = move.getHistorySize();
        final int pointerCount = move.getPointerCount();
        for (int p = 0; p < pointerCount; p++) {
            final int pointerId = move.getPointerId(p);
            final int i = findIndex(pointerId);
            if (i != UNTRACKED_POINTER) {
                for (int h = 0; h < historySize; h++) {
                    final long time = move.getHistoricalEventTime(h);
                    final float x = move.getHistoricalX(p, h);
                    final float y = move.getHistoricalY(p,  h);
                    final int swipe = detectSwipe(i, time, x, y);
                    if (swipe != SWIPE_NONE) {
                        return swipe;
                    }
                }
                final int swipe = detectSwipe(i, move.getEventTime(), move.getX(p), move.getY(p));
                if (swipe != SWIPE_NONE) {
                    return swipe;
                }
            }
        }
        return SWIPE_NONE;
    }

    private int detectSwipe(int i, long time, float x, float y) {
        final float fromX = mDownX[i];
        final float fromY = mDownY[i];
        final long elapsed = time - mDownTime[i];
        if (DEBUG) Slog.d(TAG, "pointer " + mDownPointerId[i]
                + " moved (" + fromX + "->" + x + "," + fromY + "->" + y + ") in " + elapsed);
        if (fromY <= mSwipeStartThreshold
                && y > fromY + mSwipeDistanceThreshold
                && elapsed < SWIPE_TIMEOUT_MS) {
            return SWIPE_FROM_TOP;
        }
        if (fromY >= screenHeight - mSwipeStartThreshold
                && y < fromY - mSwipeDistanceThreshold
                && elapsed < SWIPE_TIMEOUT_MS) {
            return SWIPE_FROM_BOTTOM;
        }
        if (fromX >= screenWidth - mSwipeStartThreshold
                && x < fromX - mSwipeDistanceThreshold
                && elapsed < SWIPE_TIMEOUT_MS) {
            return SWIPE_FROM_RIGHT;
        }
        if (fromX <= mSwipeStartThreshold
                && x > fromX + mSwipeDistanceThreshold
                && elapsed < SWIPE_TIMEOUT_MS) {
            return SWIPE_FROM_LEFT;
        }
        return SWIPE_NONE;
    }

    private final class FlingGestureDetector extends GestureDetector.SimpleOnGestureListener {

        FlingGestureDetector() {
        }

        @Override
        public boolean onSingleTapUp(MotionEvent e) {
            return true;
        }
        @Override
        public boolean onFling(MotionEvent down, MotionEvent up,
                float velocityX, float velocityY) {
            return true;
        }
    }

}
