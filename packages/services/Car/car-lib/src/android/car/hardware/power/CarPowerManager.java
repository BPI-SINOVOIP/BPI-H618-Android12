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

package android.car.hardware.power;

import static android.content.pm.PackageManager.PERMISSION_GRANTED;

import android.annotation.CallbackExecutor;
import android.annotation.NonNull;
import android.annotation.Nullable;
import android.annotation.RequiresPermission;
import android.annotation.SystemApi;
import android.car.Car;
import android.car.CarManagerBase;
import android.os.IBinder;
import android.os.RemoteException;
import android.util.ArrayMap;
import android.util.Log;
import android.util.Pair;
import android.util.SparseIntArray;

import com.android.internal.annotations.GuardedBy;

import java.util.ArrayList;
import java.util.concurrent.CancellationException;
import java.util.concurrent.CompletableFuture;
import java.util.concurrent.Executor;

/**
 * API to receive power policy change notifications.
 */
public class CarPowerManager extends CarManagerBase {
    private static final boolean DBG = false;
    private static final String TAG = CarPowerManager.class.getSimpleName();

    private static final int FIRST_POWER_COMPONENT = PowerComponentUtil.FIRST_POWER_COMPONENT;
    private static final int LAST_POWER_COMPONENT = PowerComponentUtil.LAST_POWER_COMPONENT;

    private final Object mLock = new Object();
    private final ICarPower mService;
    @GuardedBy("mLock")
    private final ArrayMap<CarPowerPolicyListener, Pair<Executor, CarPowerPolicyFilter>>
            mPolicyListenerMap = new ArrayMap<>();
    // key: power component, value: number of listeners to have interest in the component
    @GuardedBy("mLock")
    private final SparseIntArray mInterestedComponentMap = new SparseIntArray();
    private final ICarPowerPolicyListener mPolicyChangeBinderCallback =
            new ICarPowerPolicyListener.Stub() {
        @Override
        public void onPolicyChanged(CarPowerPolicy appliedPolicy,
                CarPowerPolicy accumulatedPolicy) {
            notifyPowerPolicyListeners(appliedPolicy, accumulatedPolicy);
        }
    };

    @GuardedBy("mLock")
    private CarPowerStateListener mListener;
    @GuardedBy("mLock")
    private CarPowerStateListenerWithCompletion mListenerWithCompletion;
    @GuardedBy("mLock")
    private CompletableFuture<Void> mFuture;
    @GuardedBy("mLock")
    private ICarPowerStateListener mListenerToService;

    /**
     * Applications set a {@link CarPowerStateListener} for power state event updates.
     *
     * @hide
     */
    @SystemApi
    public interface CarPowerStateListener {
        /**
         * onStateChanged() states. These definitions must match the ones located in the native
         * CarPowerManager:  packages/services/Car/car-lib/native/include/CarPowerManager.h
         */

        /**
         * The current power state is unavailable, unknown, or invalid.
         *
         * @hide
         */
        int INVALID = 0;

        /**
         * Android is up, but vendor is controlling the audio / display.
         *
         * @hide
         */
        int WAIT_FOR_VHAL = 1;

        /**
         * Enter suspend state. CPMS is switching to WAIT_FOR_FINISHED state.
         *
         * @hide
         */
        int SUSPEND_ENTER = 2;

        /**
         * Wake up from suspend.
         *
         * @hide
         */
        int SUSPEND_EXIT = 3;

        /**
         * Enter shutdown state. CPMS is switching to WAIT_FOR_FINISHED state.
         *
         * @hide
         */
        int SHUTDOWN_ENTER = 5;

        /**
         * On state.
         *
         * @hide
         */
        int ON = 6;

        /**
         * State where system is getting ready for shutdown or suspend. Application is expected to
         * cleanup and be ready to suspend.
         *
         * @hide
         */
        int SHUTDOWN_PREPARE = 7;

        /**
         * Shutdown is cancelled, returning to normal state.
         *
         * @hide
         */
        int SHUTDOWN_CANCELLED = 8;

        /**
         * Called when power state changes. This callback is available to
         * any listener, even if it is not running in the system process.
         *
         * @param state New power state of device.
         *
         * @hide
         */
        void onStateChanged(int state);
    }

    /**
     * Applications set a {@link CarPowerStateListenerWithCompletion} for power state
     * event updates where a CompletableFuture is used.
     *
     * @hide
     */
    @SystemApi
    public interface CarPowerStateListenerWithCompletion {
        /**
         * Called when power state changes. This callback is only for listeners
         * that are running in the system process.
         *
         * @param state New power state of device.
         * @param future CompletableFuture used by Car modules to notify CPMS that they
         *               are ready to continue shutting down. CPMS will wait until this
         *               future is completed.
         *
         * @hide
         */
        void onStateChanged(int state, CompletableFuture<Void> future);
    }

    /**
     * Listeners to receive power policy change.
     *
     * <p> Applications interested in power policy change register
     * {@code CarPowerPolicyListener} and will be notified when power policy changes.
     */
    public interface CarPowerPolicyListener {
        /**
         * Called with {@link #CarPowerPolicy} when power policy changes.
         *
         * @param policy The current power policy.
         */
        void onPolicyChanged(@NonNull CarPowerPolicy policy);
    }

    /**
     * Gets an instance of the CarPowerManager.
     *
     * <p>Should not be obtained directly by clients, use {@link Car#getCarManager(String)} instead.
     *
     * @hide
     */
    public CarPowerManager(Car car, IBinder service) {
        super(car);
        mService = ICarPower.Stub.asInterface(service);
    }

    /**
     * Requests power manager to shutdown in lieu of suspend at the next opportunity.
     *
     * @hide
     */
    @RequiresPermission(Car.PERMISSION_CAR_POWER)
    public void requestShutdownOnNextSuspend() {
        try {
            mService.requestShutdownOnNextSuspend();
        } catch (RemoteException e) {
            handleRemoteExceptionFromCarService(e);
        }
    }

    /**
     * Schedules next wake up time in CarPowerManagementService.
     *
     * @hide
     */
    @RequiresPermission(Car.PERMISSION_CAR_POWER)
    public void scheduleNextWakeupTime(int seconds) {
        try {
            mService.scheduleNextWakeupTime(seconds);
        } catch (RemoteException e) {
            handleRemoteExceptionFromCarService(e);
        }
    }

    /**
     * Returns the current power state.
     *
     * @return One of the values defined in {@link CarPowerStateListener}.
     *
     * @hide
     */
    @SystemApi
    @RequiresPermission(Car.PERMISSION_CAR_POWER)
    public int getPowerState() {
        try {
            return mService.getPowerState();
        } catch (RemoteException e) {
            return handleRemoteExceptionFromCarService(e, CarPowerStateListener.INVALID);
        }
    }

    /**
     * Sets a listener to receive power state changes. Only one listener may be set at a
     * time for an instance of CarPowerManager.
     * The listener is assumed to completely handle the 'onStateChanged' before returning.
     *
     * @param listener
     * @throws IllegalStateException
     *
     * @hide
     */
    @SystemApi
    @RequiresPermission(Car.PERMISSION_CAR_POWER)
    public void setListener(@Nullable CarPowerStateListener listener) {
        synchronized (mLock) {
            if (mListener != null || mListenerWithCompletion != null) {
                throw new IllegalStateException("Listener must be cleared first");
            }
            // Update listener
            mListener = listener;
            setServiceForListenerLocked(false);
        }
    }

    /**
     * Sets a listener to receive power state changes. Only one listener may be set at a
     * time for an instance of CarPowerManager.
     * For calls that require completion before continue, we attach a {@link CompletableFuture}
     * which is being used as a signal that caller is finished and ready to proceed.
     * Once future is completed, the {@link finished} method will automatically be called to notify
     * {@link CarPowerManagementService} that the application has handled the
     * {@link #SHUTDOWN_PREPARE} state transition.
     *
     * @param listener
     * @throws IllegalStateException
     *
     * @hide
     */
    @RequiresPermission(Car.PERMISSION_CAR_POWER)
    public void setListenerWithCompletion(@Nullable CarPowerStateListenerWithCompletion listener) {
        synchronized (mLock) {
            if (mListener != null || mListenerWithCompletion != null) {
                throw new IllegalStateException("Listener must be cleared first");
            }
            // Update listener
            mListenerWithCompletion = listener;
            setServiceForListenerLocked(true);
        }
    }

    /**
     * Removes the power state listener.
     *
     * @hide
     */
    @SystemApi
    @RequiresPermission(Car.PERMISSION_CAR_POWER)
    public void clearListener() {
        ICarPowerStateListener listenerToService;
        synchronized (mLock) {
            listenerToService = mListenerToService;
            mListenerToService = null;
            mListener = null;
            mListenerWithCompletion = null;
            cleanupFutureLocked();
        }

        if (listenerToService == null) {
            Log.w(TAG, "unregisterListener: listener was not registered");
            return;
        }

        try {
            mService.unregisterListener(listenerToService);
        } catch (RemoteException e) {
            handleRemoteExceptionFromCarService(e);
        }
    }

    /**
     * Gets the current power policy.
     *
     * <p>The returned power policy has ID of the power policy applied most recently. If no power
     * policy has been applied, the ID is an empty string. Note that enabled components and disabled
     * components might be different from those of the latest power policy applied. This is because
     * the returned power policy contains the current state of all power components determined by
     * applying power policies in an accumulative way.
     *
     * @return The power policy containing the latest state of all power components.
     */
    @RequiresPermission(Car.PERMISSION_READ_CAR_POWER_POLICY)
    @Nullable
    public CarPowerPolicy getCurrentPowerPolicy() {
        try {
            return mService.getCurrentPowerPolicy();
        } catch (RemoteException e) {
            return handleRemoteExceptionFromCarService(e, null);
        }
    }

    /**
     * Applies the given power policy.
     *
     * <p> Power components are turned on or off as specified in the given power policy. Power
     * policies are defined at {@code /vendor/etc/power_policy.xml}. If the given power policy
     * doesn't exist, this method throws {@link java.lang.IllegalArgumentException}.
     *
     * @param policyId ID of power policy.
     * @throws IllegalArgumentException if {@code policyId} is null.
     *
     * @hide
     */
    @SystemApi
    @RequiresPermission(Car.PERMISSION_CONTROL_CAR_POWER_POLICY)
    public void applyPowerPolicy(@NonNull String policyId) {
        checkArgument(policyId != null, "Null policyId");
        try {
            mService.applyPowerPolicy(policyId);
        } catch (RemoteException e) {
            handleRemoteExceptionFromCarService(e);
        }
    }

    /**
     * Sets the current power policy group.
     *
     * Power policy group defines a rule to apply a certain power policy according to the power
     * state transition. For example, a power policy named "default_for_on" is supposed to be
     * applied when the power state becomes ON. This rule is specified in the power policy group.
     * Many power policy groups can be pre-defined, and one of them is set for the current one using
     * {@code setPowerPolicyGroup}.
     *
     * @param policyGroupId ID of power policy group.
     * @throws IllegalArgumentException if {@code policyGroupId} is null.
     *
     * @hide
     */
    @SystemApi
    @RequiresPermission(Car.PERMISSION_CONTROL_CAR_POWER_POLICY)
    public void setPowerPolicyGroup(@NonNull String policyGroupId) {
        checkArgument(policyGroupId != null, "Null policyGroupId");
        try {
            mService.setPowerPolicyGroup(policyGroupId);
        } catch (RemoteException e) {
            handleRemoteExceptionFromCarService(e);
        }
    }

    /**
     * Subscribes to power policy change.
     *
     * <p>If the same listener is added with different filters, the listener is notified based on
     * the last added filter.
     *
     * @param executor Executor where the listener method is called.
     * @param listener Listener to be notified.
     * @param filter Filter specifying power components of interest.
     * @throws IllegalArgumentException if {@code executor}, {@code listener}, or {@code filter} is
     *                                  null.
     */
    @RequiresPermission(Car.PERMISSION_READ_CAR_POWER_POLICY)
    public void addPowerPolicyListener(@NonNull @CallbackExecutor Executor executor,
            @NonNull CarPowerPolicyFilter filter, @NonNull CarPowerPolicyListener listener) {
        assertPermission(Car.PERMISSION_READ_CAR_POWER_POLICY);
        checkArgument(executor != null, "Null executor");
        checkArgument(filter != null, "Null filter");
        checkArgument(listener != null, "Null listener");
        boolean updateCallbackNeeded = false;
        CarPowerPolicyFilter newFilter = null;
        synchronized (mLock) {
            mPolicyListenerMap.remove(listener);
            int[] filterComponents = filter.getComponents().clone();
            Pair<Executor, CarPowerPolicyFilter> pair =
                    new Pair<>(executor, new CarPowerPolicyFilter(filterComponents));
            mPolicyListenerMap.put(listener, pair);
            for (int i = 0; i < filterComponents.length; i++) {
                int key = filterComponents[i];
                int currentCount = mInterestedComponentMap.get(key);
                if (currentCount == 0) {
                    updateCallbackNeeded = true;
                    mInterestedComponentMap.put(key, 1);
                } else {
                    mInterestedComponentMap.put(key, currentCount + 1);
                }
            }
            if (updateCallbackNeeded) {
                newFilter = createFilterFromInterestedComponentsLocked();
            }
        }
        if (updateCallbackNeeded) {
            updatePowerPolicyChangeCallback(newFilter);
        }
    }

    /**
     * Unsubscribes from power policy change.
     *
     * @param listener Listener that will not be notified any more.
     * @throws IllegalArgumentException if {@code listener} is null.
     */
    @RequiresPermission(Car.PERMISSION_READ_CAR_POWER_POLICY)
    public void removePowerPolicyListener(@NonNull CarPowerPolicyListener listener) {
        assertPermission(Car.PERMISSION_READ_CAR_POWER_POLICY);
        checkArgument(listener != null, "Null listener");
        boolean updateCallbackNeeded = false;
        CarPowerPolicyFilter filter = null;
        synchronized (mLock) {
            Pair<Executor, CarPowerPolicyFilter> pair = mPolicyListenerMap.remove(listener);
            if (pair == null) {
                return;
            }
            int[] filterComponents = pair.second.getComponents();
            for (int i = 0; i < filterComponents.length; i++) {
                int key = filterComponents[i];
                int currentCount = mInterestedComponentMap.get(key);
                if (currentCount == 0 || currentCount == 1) {
                    mInterestedComponentMap.delete(key);
                    updateCallbackNeeded = true;
                } else {
                    mInterestedComponentMap.put(key, currentCount - 1);
                }
            }
            if (updateCallbackNeeded) {
                filter = createFilterFromInterestedComponentsLocked();
            }
        }
        if (updateCallbackNeeded) {
            updatePowerPolicyChangeCallback(filter);
        }
    }

    private void setServiceForListenerLocked(boolean useCompletion) {
        if (mListenerToService == null) {
            ICarPowerStateListener listenerToService = new ICarPowerStateListener.Stub() {
                @Override
                public void onStateChanged(int state) throws RemoteException {
                    if (useCompletion) {
                        CarPowerStateListenerWithCompletion listenerWithCompletion;
                        CompletableFuture<Void> future;
                        synchronized (mLock) {
                            // Update CompletableFuture. This will recreate it or just clean it up.
                            updateFutureLocked(state);
                            listenerWithCompletion = mListenerWithCompletion;
                            future = mFuture;
                        }
                        // Notify user that the state has changed and supply a future
                        if (listenerWithCompletion != null) {
                            listenerWithCompletion.onStateChanged(state, future);
                        }
                    } else {
                        CarPowerStateListener listener;
                        synchronized (mLock) {
                            listener = mListener;
                        }
                        // Notify the user without supplying a future
                        if (listener != null) {
                            listener.onStateChanged(state);
                        }
                    }
                }
            };
            try {
                if (useCompletion) {
                    mService.registerListenerWithCompletion(listenerToService);
                } else {
                    mService.registerListener(listenerToService);
                }
                mListenerToService = listenerToService;
            } catch (RemoteException e) {
                handleRemoteExceptionFromCarService(e);
            }
        }
    }

    private void updateFutureLocked(int state) {
        cleanupFutureLocked();
        if (state == CarPowerStateListener.SHUTDOWN_PREPARE) {
            // Create a CompletableFuture and pass it to the listener.
            // When the listener completes the future, tell
            // CarPowerManagementService that this action is finished.
            mFuture = new CompletableFuture<>();
            mFuture.whenComplete((result, exception) -> {
                if (exception != null && !(exception instanceof CancellationException)) {
                    Log.e(TAG, "Exception occurred while waiting for future", exception);
                }
                ICarPowerStateListener listenerToService;
                synchronized (mLock) {
                    listenerToService = mListenerToService;
                }
                try {
                    mService.finished(listenerToService);
                } catch (RemoteException e) {
                    handleRemoteExceptionFromCarService(e);
                }
            });
        }
    }

    private void cleanupFutureLocked() {
        if (mFuture != null) {
            if (!mFuture.isDone()) {
                mFuture.cancel(false);
            }
            mFuture = null;
        }
    }

    private CarPowerPolicyFilter createFilterFromInterestedComponentsLocked() {
        CarPowerPolicyFilter newFilter = null;
        int componentCount = mInterestedComponentMap.size();
        if (componentCount != 0) {
            int[] components = new int[componentCount];
            for (int i = 0; i < componentCount; i++) {
                components[i] = mInterestedComponentMap.keyAt(i);
            }
            newFilter = new CarPowerPolicyFilter(components);
        }
        return newFilter;
    }

    private void updatePowerPolicyChangeCallback(CarPowerPolicyFilter filter) {
        try {
            if (filter == null) {
                mService.removePowerPolicyListener(mPolicyChangeBinderCallback);
            } else {
                mService.addPowerPolicyListener(filter, mPolicyChangeBinderCallback);
            }
        } catch (RemoteException e) {
            handleRemoteExceptionFromCarService(e);
        }
    }

    private void notifyPowerPolicyListeners(CarPowerPolicy appliedPolicy,
            CarPowerPolicy accumulatedPolicy) {
        ArrayList<Pair<CarPowerPolicyListener, Executor>> listeners = new ArrayList<>();
        synchronized (mLock) {
            for (int i = 0; i < mPolicyListenerMap.size(); i++) {
                CarPowerPolicyListener listener = mPolicyListenerMap.keyAt(i);
                Pair<Executor, CarPowerPolicyFilter> pair = mPolicyListenerMap.valueAt(i);
                if (PowerComponentUtil.hasComponents(appliedPolicy, pair.second)) {
                    listeners.add(
                            new Pair<CarPowerPolicyListener, Executor>(listener, pair.first));
                }
            }
        }
        for (int i = 0; i < listeners.size(); i++) {
            Pair<CarPowerPolicyListener, Executor> pair = listeners.get(i);
            pair.second.execute(() -> pair.first.onPolicyChanged(accumulatedPolicy));
        }
    }

    private void assertPermission(String permission) {
        if (getContext().checkCallingOrSelfPermission(permission) != PERMISSION_GRANTED) {
            throw new SecurityException("requires " + permission);
        }
    }

    private void checkArgument(boolean test, String message) {
        if (!test) {
            throw new IllegalArgumentException(message);
        }
    }

    /** @hide */
    @Override
    public void onCarDisconnected() {
        synchronized (mLock) {
            mListener = null;
            mListenerWithCompletion = null;
        }
    }
}
