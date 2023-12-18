/*
 * Copyright (C) 2015 The Android Open Source Project
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
package com.android.car.dialer.telecom;

import android.bluetooth.BluetoothDevice;
import android.content.Intent;
import android.os.Binder;
import android.os.IBinder;
import android.os.Process;
import android.telecom.Call;
import android.telecom.CallAudioState;
import android.telecom.InCallService;
import android.telecom.PhoneAccountHandle;

import androidx.lifecycle.LiveData;

import com.android.car.dialer.bluetooth.PhoneAccountManager;
import com.android.car.dialer.framework.InCallServiceProxy;
import com.android.car.dialer.log.L;

import java.util.concurrent.CopyOnWriteArrayList;

import javax.inject.Inject;
import javax.inject.Named;

import dagger.hilt.android.AndroidEntryPoint;

/**
 * An implementation of {@link InCallService}. This service is bounded by android telecom and
 * {@link UiCallManager}. For incoming calls it will launch Dialer app.
 */
@AndroidEntryPoint(InCallServiceProxy.class)
public class InCallServiceImpl extends Hilt_InCallServiceImpl {
    private static final String TAG = "CD.InCallService";

    /** An action which indicates a bind is from local component. */
    public static final String ACTION_LOCAL_BIND = "local_bind";

    private CopyOnWriteArrayList<CallAudioStateCallback> mCallAudioStateCallbacks =
            new CopyOnWriteArrayList<>();

    @Inject InCallRouter mInCallRouter;
    @Inject PhoneAccountManager mPhoneAccountManager;
    @Inject @Named("Hfp") LiveData<BluetoothDevice> mCurrentHfpDeviceLiveData;

    /** Listens to active call list changes. Callbacks will be called on main thread. */
    public interface ActiveCallListChangedCallback {

        /**
         * Called when a new call is added.
         *
         * @return if the given call has been handled by this callback.
         */
        boolean onTelecomCallAdded(Call telecomCall);

        /**
         * Called when an existing call is removed.
         *
         * @return if the given call has been handled by this callback.
         */
        boolean onTelecomCallRemoved(Call telecomCall);
    }

    /** Listens to call audio state changes. Callbacks will be called on the main thread. */
    public interface CallAudioStateCallback {
        /**
         * Called when the call audio state has changed.
         */
        void onCallAudioStateChanged(CallAudioState callAudioState);
    }

    @Override
    public void onCreate() {
        super.onCreate();
        mInCallRouter.start();
    }

    @Override
    public void onDestroy() {
        super.onDestroy();
        mInCallRouter.stop();
    }

    @Override
    public void onCallAdded(Call telecomCall) {
        L.d(TAG, "onCallAdded: %s", telecomCall);
        if (telecomCall.getState() == Call.STATE_SELECT_PHONE_ACCOUNT) {
            BluetoothDevice currentHfpDevice = mCurrentHfpDeviceLiveData.getValue();
            PhoneAccountHandle currentPhoneAccountHandle =
                    mPhoneAccountManager.getMatchingPhoneAccount(currentHfpDevice);
            if (currentPhoneAccountHandle != null) {
                telecomCall.phoneAccountSelected(currentPhoneAccountHandle, false);
            } else {
                L.e(TAG, "Not able to get the phone account handle for current hfp device.");
            }
        }
        mInCallRouter.onCallAdded(telecomCall);
    }

    @Override
    public void onCallRemoved(Call telecomCall) {
        L.d(TAG, "onCallRemoved: %s", telecomCall);
        mInCallRouter.onCallRemoved(telecomCall);
    }

    @Override
    public IBinder onBind(Intent intent) {
        L.d(TAG, "onBind: %s", intent);
        return ACTION_LOCAL_BIND.equals(intent.getAction())
                ? new LocalBinder()
                : super.onBind(intent);
    }

    @Override
    public boolean onUnbind(Intent intent) {
        L.d(TAG, "onUnbind, intent: %s", intent);
        if (ACTION_LOCAL_BIND.equals(intent.getAction())) {
            return false;
        }
        return super.onUnbind(intent);
    }

    @Override
    public void onCallAudioStateChanged(CallAudioState audioState) {
        for (CallAudioStateCallback callback : mCallAudioStateCallbacks) {
            callback.onCallAudioStateChanged(audioState);
        }
    }

    @Override
    public void onBringToForeground(boolean showDialpad) {
        L.d(TAG, "onBringToForeground: %s", showDialpad);
        mInCallRouter.routeToFullScreenIncomingCallPage(showDialpad);
    }

    public void addCallAudioStateChangedCallback(CallAudioStateCallback callback) {
        mCallAudioStateCallbacks.add(callback);
    }

    public void removeCallAudioStateChangedCallback(CallAudioStateCallback callback) {
        mCallAudioStateCallbacks.remove(callback);
    }

    public void addActiveCallListChangedCallback(ActiveCallListChangedCallback callback) {
        mInCallRouter.registerActiveCallListChangedCallback(callback);
    }

    public void removeActiveCallListChangedCallback(ActiveCallListChangedCallback callback) {
        mInCallRouter.unregisterActiveCallHandler(callback);
    }

    /**
     * Local binder only available for Car Dialer package.
     */
    public class LocalBinder extends Binder {

        /**
         * Returns a reference to {@link InCallServiceImpl}. Any process other than Dialer
         * process won't be able to get a reference.
         */
        public InCallServiceImpl getService() {
            if (getCallingPid() == Process.myPid()) {
                return InCallServiceImpl.this;
            }
            return null;
        }
    }
}
