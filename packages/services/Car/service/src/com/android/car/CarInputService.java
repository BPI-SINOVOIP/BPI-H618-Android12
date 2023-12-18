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
 * limitations under the License.
 */
package com.android.car;

import static android.car.CarOccupantZoneManager.DisplayTypeEnum;
import static android.hardware.input.InputManager.INJECT_INPUT_EVENT_MODE_ASYNC;
import static android.service.voice.VoiceInteractionSession.SHOW_SOURCE_PUSH_TO_TALK;

import static com.android.car.internal.ExcludeFromCodeCoverageGeneratedReport.DUMP_INFO;

import android.annotation.Nullable;
import android.annotation.UserIdInt;
import android.app.ActivityManager;
import android.bluetooth.BluetoothAdapter;
import android.bluetooth.BluetoothDevice;
import android.bluetooth.BluetoothHeadsetClient;
import android.bluetooth.BluetoothProfile;
import android.car.CarOccupantZoneManager;
import android.car.CarProjectionManager;
import android.car.input.CarInputManager;
import android.car.input.CustomInputEvent;
import android.car.input.ICarInput;
import android.car.input.ICarInputCallback;
import android.car.input.RotaryEvent;
import android.car.user.CarUserManager;
import android.content.ContentResolver;
import android.content.Context;
import android.content.Intent;
import android.content.pm.PackageManager;
import android.content.res.Resources;
import android.hardware.input.InputManager;
import android.net.Uri;
import android.os.Binder;
import android.os.Bundle;
import android.os.Handler;
import android.os.Looper;
import android.os.UserHandle;
import android.provider.CallLog.Calls;
import android.provider.Settings;
import android.telecom.TelecomManager;
import android.text.TextUtils;
import android.util.IndentingPrintWriter;
import android.view.InputDevice;
import android.view.KeyEvent;
import android.view.ViewConfiguration;

import com.android.car.hal.InputHalService;
import com.android.car.internal.ExcludeFromCodeCoverageGeneratedReport;
import com.android.car.internal.common.UserHelperLite;
import com.android.car.user.CarUserService;
import com.android.internal.annotations.GuardedBy;
import com.android.internal.annotations.VisibleForTesting;
import com.android.internal.app.AssistUtils;
import com.android.internal.app.IVoiceInteractionSessionShowCallback;
import com.android.internal.os.BackgroundThread;
import com.android.server.utils.Slogf;

import java.util.ArrayList;
import java.util.BitSet;
import java.util.Collections;
import java.util.List;
import java.util.function.BooleanSupplier;
import java.util.function.IntSupplier;
import java.util.function.Supplier;

/**
 * CarInputService monitors and handles input event through vehicle HAL.
 */
public class CarInputService extends ICarInput.Stub
        implements CarServiceBase, InputHalService.InputListener {

    private static final String TAG = CarLog.TAG_INPUT;

    /** An interface to receive {@link KeyEvent}s as they occur. */
    public interface KeyEventListener {
        /** Called when a key event occurs. */
        void onKeyEvent(KeyEvent event);
    }

    private final class KeyPressTimer {
        private final Runnable mLongPressRunnable;
        private final Runnable mCallback = this::onTimerExpired;
        private final IntSupplier mLongPressDelaySupplier;

        @GuardedBy("CarInputService.this.mLock")
        private final Handler mHandler;
        @GuardedBy("CarInputService.this.mLock")
        private boolean mDown;
        @GuardedBy("CarInputService.this.mLock")
        private boolean mLongPress = false;

        KeyPressTimer(
                Handler handler, IntSupplier longPressDelaySupplier, Runnable longPressRunnable) {
            mHandler = handler;
            mLongPressRunnable = longPressRunnable;
            mLongPressDelaySupplier = longPressDelaySupplier;
        }

        /** Marks that a key was pressed, and starts the long-press timer. */
        void keyDown() {
            synchronized (mLock) {
                mDown = true;
                mLongPress = false;
                mHandler.removeCallbacks(mCallback);
                mHandler.postDelayed(mCallback, mLongPressDelaySupplier.getAsInt());
            }
        }

        /**
         * Marks that a key was released, and stops the long-press timer.
         *
         * Returns true if the press was a long-press.
         */
        boolean keyUp() {
            synchronized (mLock) {
                mHandler.removeCallbacks(mCallback);
                mDown = false;
                return mLongPress;
            }
        }

        private void onTimerExpired() {
            synchronized (mLock) {
                // If the timer expires after key-up, don't retroactively make the press long.
                if (!mDown) {
                    return;
                }
                mLongPress = true;
            }
            mLongPressRunnable.run();
        }
    }

    private final IVoiceInteractionSessionShowCallback mShowCallback =
            new IVoiceInteractionSessionShowCallback.Stub() {
                @Override
                public void onFailed() {
                    Slogf.w(TAG, "Failed to show VoiceInteractionSession");
                }

                @Override
                public void onShown() {
                    Slogf.d(TAG, "IVoiceInteractionSessionShowCallback onShown()");
                }
            };

    @VisibleForTesting
    static final String EXTRA_CAR_PUSH_TO_TALK =
            "com.android.car.input.EXTRA_CAR_PUSH_TO_TALK";

    private final Context mContext;
    private final InputHalService mInputHalService;
    private final CarUserService mUserService;
    private final CarOccupantZoneService mCarOccupantZoneService;
    private final TelecomManager mTelecomManager;
    private final AssistUtils mAssistUtils;

    // The default handler for main-display input events. By default, injects the events into
    // the input queue via InputManager, but can be overridden for testing.
    private final KeyEventListener mMainDisplayHandler;
    // The supplier for the last-called number. By default, gets the number from the call log.
    // May be overridden for testing.
    private final Supplier<String> mLastCalledNumberSupplier;
    // The supplier for the system long-press delay, in milliseconds. By default, gets the value
    // from Settings.Secure for the current user, falling back to the system-wide default
    // long-press delay defined in ViewConfiguration. May be overridden for testing.
    private final IntSupplier mLongPressDelaySupplier;
    // ComponentName of the RotaryService.
    private final String mRotaryServiceComponentName;

    private final BooleanSupplier mShouldCallButtonEndOngoingCallSupplier;

    private final Object mLock = new Object();

    @GuardedBy("mLock")
    private CarProjectionManager.ProjectionKeyEventHandler mProjectionKeyEventHandler;

    @GuardedBy("mLock")
    private final BitSet mProjectionKeyEventsSubscribed = new BitSet();

    private final KeyPressTimer mVoiceKeyTimer;
    private final KeyPressTimer mCallKeyTimer;

    @GuardedBy("mLock")
    private KeyEventListener mInstrumentClusterKeyListener;

    private final InputCaptureClientController mCaptureController;

    private final BluetoothAdapter mBluetoothAdapter;

    // BluetoothHeadsetClient set through mBluetoothProfileServiceListener, and used by
    // launchBluetoothVoiceRecognition().
    @GuardedBy("mLock")
    private BluetoothHeadsetClient mBluetoothHeadsetClient;

    private final BluetoothProfile.ServiceListener mBluetoothProfileServiceListener =
            new BluetoothProfile.ServiceListener() {
        @Override
        public void onServiceConnected(int profile, BluetoothProfile proxy) {
            if (profile == BluetoothProfile.HEADSET_CLIENT) {
                Slogf.d(TAG, "Bluetooth proxy connected for HEADSET_CLIENT profile");
                synchronized (mLock) {
                    mBluetoothHeadsetClient = (BluetoothHeadsetClient) proxy;
                }
            }
        }

        @Override
        public void onServiceDisconnected(int profile) {
            if (profile == BluetoothProfile.HEADSET_CLIENT) {
                Slogf.d(TAG, "Bluetooth proxy disconnected for HEADSET_CLIENT profile");
                synchronized (mLock) {
                    mBluetoothHeadsetClient = null;
                }
            }
        }
    };

    private final CarUserManager.UserLifecycleListener mUserLifecycleListener = event -> {
        Slogf.d(TAG, "CarInputService.onEvent(%s)", event);
        if (CarUserManager.USER_LIFECYCLE_EVENT_TYPE_SWITCHING == event.getEventType()) {
            updateRotaryServiceSettings(event.getUserId());
        }
    };

    private static int getViewLongPressDelay(ContentResolver cr) {
        return Settings.Secure.getIntForUser(
                cr,
                Settings.Secure.LONG_PRESS_TIMEOUT,
                ViewConfiguration.getLongPressTimeout(),
                UserHandle.USER_CURRENT);
    }

    public CarInputService(Context context, InputHalService inputHalService,
            CarUserService userService, CarOccupantZoneService occupantZoneService) {
        this(context, inputHalService, userService, occupantZoneService,
                new Handler(Looper.getMainLooper()),
                context.getSystemService(TelecomManager.class), new AssistUtils(context),
                event ->
                        context.getSystemService(InputManager.class)
                                .injectInputEvent(event, INJECT_INPUT_EVENT_MODE_ASYNC),
                () -> Calls.getLastOutgoingCall(context),
                () -> getViewLongPressDelay(context.getContentResolver()),
                () -> context.getResources().getBoolean(R.bool.config_callButtonEndsOngoingCall),
                new InputCaptureClientController(context),
                BluetoothAdapter.getDefaultAdapter());
    }

    @VisibleForTesting
    CarInputService(Context context, InputHalService inputHalService, CarUserService userService,
            CarOccupantZoneService occupantZoneService, Handler handler,
            TelecomManager telecomManager, AssistUtils assistUtils,
            KeyEventListener mainDisplayHandler,
            Supplier<String> lastCalledNumberSupplier, IntSupplier longPressDelaySupplier,
            BooleanSupplier shouldCallButtonEndOngoingCallSupplier,
            InputCaptureClientController captureController, BluetoothAdapter bluetoothAdapter) {
        mContext = context;
        mCaptureController = captureController;
        mInputHalService = inputHalService;
        mUserService = userService;
        mCarOccupantZoneService = occupantZoneService;
        mTelecomManager = telecomManager;
        mAssistUtils = assistUtils;
        mMainDisplayHandler = mainDisplayHandler;
        mLastCalledNumberSupplier = lastCalledNumberSupplier;
        mLongPressDelaySupplier = longPressDelaySupplier;

        mVoiceKeyTimer =
                new KeyPressTimer(
                        handler, longPressDelaySupplier, this::handleVoiceAssistLongPress);
        mCallKeyTimer =
                new KeyPressTimer(handler, longPressDelaySupplier, this::handleCallLongPress);

        mRotaryServiceComponentName = mContext.getString(R.string.rotaryService);
        mShouldCallButtonEndOngoingCallSupplier = shouldCallButtonEndOngoingCallSupplier;
        mBluetoothAdapter = bluetoothAdapter;
    }

    /**
     * Set projection key event listener. If null, unregister listener.
     */
    public void setProjectionKeyEventHandler(
            @Nullable CarProjectionManager.ProjectionKeyEventHandler listener,
            @Nullable BitSet events) {
        synchronized (mLock) {
            mProjectionKeyEventHandler = listener;
            mProjectionKeyEventsSubscribed.clear();
            if (events != null) {
                mProjectionKeyEventsSubscribed.or(events);
            }
        }
    }

    /**
     * Sets the instrument cluster key event listener.
     */
    public void setInstrumentClusterKeyListener(KeyEventListener listener) {
        synchronized (mLock) {
            mInstrumentClusterKeyListener = listener;
        }
    }

    @Override
    public void init() {
        if (!mInputHalService.isKeyInputSupported()) {
            Slogf.w(TAG, "Hal does not support key input.");
            return;
        }
        Slogf.d(TAG, "Hal supports key input.");
        mInputHalService.setInputListener(this);
        if (mBluetoothAdapter != null) {
            BackgroundThread.getHandler().post(() -> {
                mBluetoothAdapter.getProfileProxy(mContext,
                        mBluetoothProfileServiceListener, BluetoothProfile.HEADSET_CLIENT);
            });
        }
        if (!TextUtils.isEmpty(mRotaryServiceComponentName)) {
            mUserService.addUserLifecycleListener(mUserLifecycleListener);
        }
    }

    @Override
    public void release() {
        synchronized (mLock) {
            mProjectionKeyEventHandler = null;
            mProjectionKeyEventsSubscribed.clear();
            mInstrumentClusterKeyListener = null;
            if (mBluetoothHeadsetClient != null) {
                mBluetoothAdapter.closeProfileProxy(
                        BluetoothProfile.HEADSET_CLIENT, mBluetoothHeadsetClient);
                mBluetoothHeadsetClient = null;
            }
        }
        if (!TextUtils.isEmpty(mRotaryServiceComponentName)) {
            mUserService.removeUserLifecycleListener(mUserLifecycleListener);
        }
    }

    @Override
    public void onKeyEvent(KeyEvent event, @DisplayTypeEnum int targetDisplayType) {
        // Special case key code that have special "long press" handling for automotive
        switch (event.getKeyCode()) {
            case KeyEvent.KEYCODE_VOICE_ASSIST:
                handleVoiceAssistKey(event);
                return;
            case KeyEvent.KEYCODE_CALL:
                handleCallKey(event);
                return;
            default:
                break;
        }

        assignDisplayId(event, targetDisplayType);

        // Allow specifically targeted keys to be routed to the cluster
        if (targetDisplayType == CarOccupantZoneManager.DISPLAY_TYPE_INSTRUMENT_CLUSTER
                && handleInstrumentClusterKey(event)) {
            return;
        }
        if (mCaptureController.onKeyEvent(targetDisplayType, event)) {
            return;
        }
        mMainDisplayHandler.onKeyEvent(event);
    }

    private void assignDisplayId(KeyEvent event, @DisplayTypeEnum int targetDisplayType) {
        // Setting display id for driver user id (currently MAIN and CLUSTER display types are
        // linked to driver user only)
        int newDisplayId = mCarOccupantZoneService.getDisplayIdForDriver(targetDisplayType);

        // Display id is overridden even if already set.
        event.setDisplayId(newDisplayId);
    }

    @Override
    public void onRotaryEvent(RotaryEvent event, @DisplayTypeEnum int targetDisplay) {
        if (!mCaptureController.onRotaryEvent(targetDisplay, event)) {
            List<KeyEvent> keyEvents = rotaryEventToKeyEvents(event);
            for (KeyEvent keyEvent : keyEvents) {
                onKeyEvent(keyEvent, targetDisplay);
            }
        }
    }

    @Override
    public void onCustomInputEvent(CustomInputEvent event) {
        if (!mCaptureController.onCustomInputEvent(event)) {
            Slogf.w(TAG, "Failed to propagate (%s)", event);
            return;
        }
        Slogf.d(TAG, "Succeed injecting (%s)", event);
    }

    private static List<KeyEvent> rotaryEventToKeyEvents(RotaryEvent event) {
        int numClicks = event.getNumberOfClicks();
        int numEvents = numClicks * 2; // up / down per each click
        boolean clockwise = event.isClockwise();
        int keyCode;
        switch (event.getInputType()) {
            case CarInputManager.INPUT_TYPE_ROTARY_NAVIGATION:
                keyCode = clockwise
                        ? KeyEvent.KEYCODE_NAVIGATE_NEXT
                        : KeyEvent.KEYCODE_NAVIGATE_PREVIOUS;
                break;
            case CarInputManager.INPUT_TYPE_ROTARY_VOLUME:
                keyCode = clockwise
                        ? KeyEvent.KEYCODE_VOLUME_UP
                        : KeyEvent.KEYCODE_VOLUME_DOWN;
                break;
            default:
                Slogf.e(TAG, "Unknown rotary input type: %d", event.getInputType());
                return Collections.EMPTY_LIST;
        }
        ArrayList<KeyEvent> keyEvents = new ArrayList<>(numEvents);
        for (int i = 0; i < numClicks; i++) {
            long uptime = event.getUptimeMillisForClick(i);
            KeyEvent downEvent = createKeyEvent(/* down= */ true, uptime, uptime, keyCode);
            KeyEvent upEvent = createKeyEvent(/* down= */ false, uptime, uptime, keyCode);
            keyEvents.add(downEvent);
            keyEvents.add(upEvent);
        }
        return keyEvents;
    }

    private static KeyEvent createKeyEvent(boolean down, long downTime, long eventTime,
            int keyCode) {
        return new KeyEvent(
                downTime,
                eventTime,
                /* action= */ down ? KeyEvent.ACTION_DOWN : KeyEvent.ACTION_UP,
                keyCode,
                /* repeat= */ 0,
                /* metaState= */ 0,
                /* deviceId= */ 0,
                /* scancode= */ 0,
                /* flags= */ 0,
                InputDevice.SOURCE_CLASS_BUTTON);
    }

    @Override
    public int requestInputEventCapture(ICarInputCallback callback,
            @DisplayTypeEnum int targetDisplayType,
            int[] inputTypes, int requestFlags) {
        return mCaptureController.requestInputEventCapture(callback, targetDisplayType, inputTypes,
                requestFlags);
    }

    @Override
    public void releaseInputEventCapture(ICarInputCallback callback,
            @DisplayTypeEnum int targetDisplayType) {
        mCaptureController.releaseInputEventCapture(callback, targetDisplayType);
    }

    /**
     * Injects the {@link KeyEvent} passed as parameter against Car Input API.
     * <p>
     * The event's display id will be overridden accordingly to the display type (it will be
     * retrieved from {@link CarOccupantZoneService}).
     *
     * @param event             the event to inject
     * @param targetDisplayType the display type associated with the event
     * @throws SecurityException when caller doesn't have INJECT_EVENTS permission granted
     */
    @Override
    public void injectKeyEvent(KeyEvent event, @DisplayTypeEnum int targetDisplayType) {
        // Permission check
        if (PackageManager.PERMISSION_GRANTED != mContext.checkCallingOrSelfPermission(
                android.Manifest.permission.INJECT_EVENTS)) {
            throw new SecurityException("Injecting KeyEvent requires INJECT_EVENTS permission");
        }

        long token = Binder.clearCallingIdentity();
        try {
            // Redirect event to onKeyEvent
            onKeyEvent(event, targetDisplayType);
        } finally {
            Binder.restoreCallingIdentity(token);
        }
    }

    private void handleVoiceAssistKey(KeyEvent event) {
        int action = event.getAction();
        if (action == KeyEvent.ACTION_DOWN && event.getRepeatCount() == 0) {
            mVoiceKeyTimer.keyDown();
            dispatchProjectionKeyEvent(CarProjectionManager.KEY_EVENT_VOICE_SEARCH_KEY_DOWN);
        } else if (action == KeyEvent.ACTION_UP) {
            if (mVoiceKeyTimer.keyUp()) {
                // Long press already handled by handleVoiceAssistLongPress(), nothing more to do.
                // Hand it off to projection, if it's interested, otherwise we're done.
                dispatchProjectionKeyEvent(
                        CarProjectionManager.KEY_EVENT_VOICE_SEARCH_LONG_PRESS_KEY_UP);
                return;
            }

            if (dispatchProjectionKeyEvent(
                    CarProjectionManager.KEY_EVENT_VOICE_SEARCH_SHORT_PRESS_KEY_UP)) {
                return;
            }

            launchDefaultVoiceAssistantHandler();
        }
    }

    private void handleVoiceAssistLongPress() {
        // If projection wants this event, let it take it.
        if (dispatchProjectionKeyEvent(
                CarProjectionManager.KEY_EVENT_VOICE_SEARCH_LONG_PRESS_KEY_DOWN)) {
            return;
        }
        // Otherwise, try to launch voice recognition on a BT device.
        if (launchBluetoothVoiceRecognition()) {
            return;
        }
        // Finally, fallback to the default voice assist handling.
        launchDefaultVoiceAssistantHandler();
    }

    private void handleCallKey(KeyEvent event) {
        int action = event.getAction();
        if (action == KeyEvent.ACTION_DOWN && event.getRepeatCount() == 0) {
            mCallKeyTimer.keyDown();
            dispatchProjectionKeyEvent(CarProjectionManager.KEY_EVENT_CALL_KEY_DOWN);
        } else if (action == KeyEvent.ACTION_UP) {
            if (mCallKeyTimer.keyUp()) {
                // Long press already handled by handleCallLongPress(), nothing more to do.
                // Hand it off to projection, if it's interested, otherwise we're done.
                dispatchProjectionKeyEvent(CarProjectionManager.KEY_EVENT_CALL_LONG_PRESS_KEY_UP);
                return;
            }

            if (acceptCallIfRinging()) {
                // Ringing call answered, nothing more to do.
                return;
            }

            if (mShouldCallButtonEndOngoingCallSupplier.getAsBoolean() && endCall()) {
                // On-going call ended, nothing more to do.
                return;
            }

            if (dispatchProjectionKeyEvent(
                    CarProjectionManager.KEY_EVENT_CALL_SHORT_PRESS_KEY_UP)) {
                return;
            }

            launchDialerHandler();
        }
    }

    private void handleCallLongPress() {
        // Long-press answers call if ringing, same as short-press.
        if (acceptCallIfRinging()) {
            return;
        }

        if (mShouldCallButtonEndOngoingCallSupplier.getAsBoolean() && endCall()) {
            return;
        }

        if (dispatchProjectionKeyEvent(CarProjectionManager.KEY_EVENT_CALL_LONG_PRESS_KEY_DOWN)) {
            return;
        }

        dialLastCallHandler();
    }

    private boolean dispatchProjectionKeyEvent(@CarProjectionManager.KeyEventNum int event) {
        CarProjectionManager.ProjectionKeyEventHandler projectionKeyEventHandler;
        synchronized (mLock) {
            projectionKeyEventHandler = mProjectionKeyEventHandler;
            if (projectionKeyEventHandler == null || !mProjectionKeyEventsSubscribed.get(event)) {
                // No event handler, or event handler doesn't want this event - we're done.
                return false;
            }
        }

        projectionKeyEventHandler.onKeyEvent(event);
        return true;
    }

    private void launchDialerHandler() {
        Slogf.i(TAG, "call key, launch dialer intent");
        Intent dialerIntent = new Intent(Intent.ACTION_DIAL);
        mContext.startActivityAsUser(dialerIntent, null, UserHandle.CURRENT_OR_SELF);
    }

    private void dialLastCallHandler() {
        Slogf.i(TAG, "call key, dialing last call");

        String lastNumber = mLastCalledNumberSupplier.get();
        if (!TextUtils.isEmpty(lastNumber)) {
            Intent callLastNumberIntent = new Intent(Intent.ACTION_CALL)
                    .setData(Uri.fromParts("tel", lastNumber, null))
                    .setFlags(Intent.FLAG_ACTIVITY_NEW_TASK);
            mContext.startActivityAsUser(callLastNumberIntent, null, UserHandle.CURRENT_OR_SELF);
        }
    }

    private boolean acceptCallIfRinging() {
        if (mTelecomManager != null && mTelecomManager.isRinging()) {
            Slogf.i(TAG, "call key while ringing. Answer the call!");
            mTelecomManager.acceptRingingCall();
            return true;
        }
        return false;
    }

    private boolean endCall() {
        if (mTelecomManager != null && mTelecomManager.isInCall()) {
            Slogf.i(TAG, "End the call!");
            mTelecomManager.endCall();
            return true;
        }
        return false;
    }

    private boolean isBluetoothVoiceRecognitionEnabled() {
        Resources res = mContext.getResources();
        return res.getBoolean(R.bool.enableLongPressBluetoothVoiceRecognition);
    }

    private boolean launchBluetoothVoiceRecognition() {
        synchronized (mLock) {
            if (mBluetoothHeadsetClient == null || !isBluetoothVoiceRecognitionEnabled()) {
                return false;
            }
            // getConnectedDevices() does not make any guarantees about the order of the returned
            // list. As of 2019-02-26, this code is only triggered through a long-press of the
            // voice recognition key, so handling of multiple connected devices that support voice
            // recognition is not expected to be a primary use case.
            List<BluetoothDevice> devices = mBluetoothHeadsetClient.getConnectedDevices();
            if (devices != null) {
                for (BluetoothDevice device : devices) {
                    Bundle bundle = mBluetoothHeadsetClient.getCurrentAgFeatures(device);
                    if (bundle == null || !bundle.getBoolean(
                            BluetoothHeadsetClient.EXTRA_AG_FEATURE_VOICE_RECOGNITION)) {
                        continue;
                    }
                    if (mBluetoothHeadsetClient.startVoiceRecognition(device)) {
                        Slogf.d(TAG, "started voice recognition on BT device at (%s)",
                                device.getAddress());
                        return true;
                    }
                }
            }
        }
        return false;
    }

    private void launchDefaultVoiceAssistantHandler() {
        Slogf.i(TAG, "voice key, invoke AssistUtils");

        if (mAssistUtils.getAssistComponentForUser(ActivityManager.getCurrentUser()) == null) {
            Slogf.w(TAG, "Unable to retrieve assist component for current user");
            return;
        }

        final Bundle args = new Bundle();
        args.putBoolean(EXTRA_CAR_PUSH_TO_TALK, true);

        mAssistUtils.showSessionForActiveService(args,
                SHOW_SOURCE_PUSH_TO_TALK, mShowCallback, null /*activityToken*/);
    }

    /**
     * @return false if the KeyEvent isn't consumed because there is no
     * InstrumentClusterKeyListener.
     */
    private boolean handleInstrumentClusterKey(KeyEvent event) {
        KeyEventListener listener = null;
        synchronized (mLock) {
            listener = mInstrumentClusterKeyListener;
        }
        if (listener == null) {
            return false;
        }
        listener.onKeyEvent(event);
        return true;
    }

    @Override
    @ExcludeFromCodeCoverageGeneratedReport(reason = DUMP_INFO)
    public void dump(IndentingPrintWriter writer) {
        writer.println("*Input Service*");
        writer.println("Long-press delay: " + mLongPressDelaySupplier.getAsInt() + "ms");
        writer.println("Call button ends ongoing call: "
                + mShouldCallButtonEndOngoingCallSupplier.getAsBoolean());
        mCaptureController.dump(writer);
    }

    private void updateRotaryServiceSettings(@UserIdInt int userId) {
        if (UserHelperLite.isHeadlessSystemUser(userId)) {
            return;
        }
        ContentResolver contentResolver = mContext.getContentResolver();
        Settings.Secure.putStringForUser(contentResolver,
                Settings.Secure.ENABLED_ACCESSIBILITY_SERVICES,
                mRotaryServiceComponentName,
                userId);
        Settings.Secure.putStringForUser(contentResolver,
                Settings.Secure.ACCESSIBILITY_ENABLED,
                "1",
                userId);
    }
}
