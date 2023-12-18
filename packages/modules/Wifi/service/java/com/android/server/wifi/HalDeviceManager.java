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
 * limitations under the License.
 */

package com.android.server.wifi;

import android.annotation.IntDef;
import android.annotation.NonNull;
import android.annotation.Nullable;
import android.hardware.wifi.V1_0.IWifi;
import android.hardware.wifi.V1_0.IWifiApIface;
import android.hardware.wifi.V1_0.IWifiChip;
import android.hardware.wifi.V1_0.IWifiChipEventCallback;
import android.hardware.wifi.V1_0.IWifiEventCallback;
import android.hardware.wifi.V1_0.IWifiIface;
import android.hardware.wifi.V1_0.IWifiNanIface;
import android.hardware.wifi.V1_0.IWifiP2pIface;
import android.hardware.wifi.V1_0.IWifiRttController;
import android.hardware.wifi.V1_0.IWifiStaIface;
import android.hardware.wifi.V1_0.IfaceType;
import android.hardware.wifi.V1_0.WifiDebugRingBufferStatus;
import android.hardware.wifi.V1_0.WifiStatus;
import android.hardware.wifi.V1_0.WifiStatusCode;
import android.hidl.manager.V1_0.IServiceNotification;
import android.hidl.manager.V1_2.IServiceManager;
import android.os.Handler;
import android.os.IHwBinder.DeathRecipient;
import android.os.RemoteException;
import android.os.WorkSource;
import android.util.Log;
import android.util.Pair;
import android.util.SparseArray;
import android.util.SparseIntArray;

import com.android.internal.annotations.VisibleForTesting;
import com.android.modules.utils.build.SdkLevel;
import com.android.server.wifi.util.GeneralUtil.Mutable;
import com.android.server.wifi.util.WorkSourceHelper;

import java.io.FileDescriptor;
import java.io.PrintWriter;
import java.lang.annotation.Retention;
import java.lang.annotation.RetentionPolicy;
import java.util.ArrayList;
import java.util.Arrays;
import java.util.HashMap;
import java.util.HashSet;
import java.util.List;
import java.util.Map;
import java.util.Set;

/**
 * Handles device management through the HAL (HIDL) interface.
 */
public class HalDeviceManager {
    private static final String TAG = "HalDevMgr";
    private static final boolean VDBG = false;
    private boolean mDbg = false;

    public static final long CHIP_CAPABILITY_ANY = 0L;
    private static final long CHIP_CAPABILITY_UNINITIALIZED = -1L;

    private static final int START_HAL_RETRY_INTERVAL_MS = 20;
    // Number of attempts a start() is re-tried. A value of 0 means no retries after a single
    // attempt.
    @VisibleForTesting
    public static final int START_HAL_RETRY_TIMES = 3;

    private final Clock mClock;
    private final WifiInjector mWifiInjector;
    private final Handler mEventHandler;
    private WifiDeathRecipient mIWifiDeathRecipient;
    private ServiceManagerDeathRecipient mServiceManagerDeathRecipient;

    // cache the value for supporting vendor HAL or not
    private boolean mIsVendorHalSupported = false;

    @VisibleForTesting
    public static final int HDM_CREATE_IFACE_STA = 0;
    @VisibleForTesting
    public static final int HDM_CREATE_IFACE_AP = 1;
    @VisibleForTesting
    public static final int HDM_CREATE_IFACE_AP_BRIDGE = 2;
    @VisibleForTesting
    public static final int HDM_CREATE_IFACE_P2P = 3;
    @VisibleForTesting
    public static final int HDM_CREATE_IFACE_NAN = 4;

    @IntDef(flag = false, prefix = { "HDM_CREATE_IFACE_TYPE_" }, value = {
            HDM_CREATE_IFACE_STA,
            HDM_CREATE_IFACE_AP,
            HDM_CREATE_IFACE_AP_BRIDGE,
            HDM_CREATE_IFACE_P2P,
            HDM_CREATE_IFACE_NAN,
    })
    private @interface HdmIfaceTypeForCreation {};

    private SparseIntArray mHalIfaceMap = new SparseIntArray() {{
            put(HDM_CREATE_IFACE_STA, IfaceType.STA);
            put(HDM_CREATE_IFACE_AP, IfaceType.AP);
            put(HDM_CREATE_IFACE_AP_BRIDGE, IfaceType.AP);
            put(HDM_CREATE_IFACE_P2P, IfaceType.P2P);
            put(HDM_CREATE_IFACE_NAN, IfaceType.NAN);
            }};

    // public API
    public HalDeviceManager(Clock clock, WifiInjector wifiInjector, Handler handler) {
        mClock = clock;
        mWifiInjector = wifiInjector;
        mEventHandler = handler;
        mIWifiDeathRecipient = new WifiDeathRecipient();
        mServiceManagerDeathRecipient = new ServiceManagerDeathRecipient();
    }

    /* package */ void enableVerboseLogging(int verbose) {
        if (verbose > 0) {
            mDbg = true;
        } else {
            mDbg = false;
        }
        if (VDBG) {
            mDbg = true; // just override
        }
    }

    /**
     * Actually starts the HalDeviceManager: separate from constructor since may want to phase
     * at a later time.
     *
     * TODO: if decide that no need for separating construction from initialization (e.g. both are
     * done at injector) then move to constructor.
     */
    public void initialize() {
        initializeInternal();
    }

    /**
     * Register a ManagerStatusListener to get information about the status of the manager. Use the
     * isReady() and isStarted() methods to check status immediately after registration and when
     * triggered.
     *
     * It is safe to re-register the same callback object - duplicates are detected and only a
     * single copy kept.
     *
     * @param listener ManagerStatusListener listener object.
     * @param handler Handler on which to dispatch listener. Null implies the listener will be
     *                invoked synchronously from the context of the client which triggered the
     *                state change.
     */
    public void registerStatusListener(@NonNull ManagerStatusListener listener,
            @Nullable Handler handler) {
        synchronized (mLock) {
            if (!mManagerStatusListeners.add(new ManagerStatusListenerProxy(listener, handler))) {
                Log.w(TAG, "registerStatusListener: duplicate registration ignored");
            }
        }
    }

    /**
     * Returns whether the vendor HAL is supported on this device or not.
     */
    public boolean isSupported() {
        return mIsVendorHalSupported;
    }

    /**
     * Returns the current status of the HalDeviceManager: whether or not it is ready to execute
     * commands. A return of 'false' indicates that the HAL service (IWifi) is not available. Use
     * the registerStatusListener() to listener for status changes.
     */
    public boolean isReady() {
        return mIsReady;
    }

    /**
     * Returns the current status of Wi-Fi: started (true) or stopped (false).
     *
     * Note: direct call to HIDL.
     */
    public boolean isStarted() {
        return isWifiStarted();
    }

    /**
     * Attempts to start Wi-Fi (using HIDL). Returns the success (true) or failure (false) or
     * the start operation. Will also dispatch any registered ManagerStatusCallback.onStart() on
     * success.
     *
     * Note: direct call to HIDL.
     */
    public boolean start() {
        return startWifi();
    }

    /**
     * Stops Wi-Fi. Will also dispatch any registeredManagerStatusCallback.onStop().
     *
     * Note: direct call to HIDL - failure is not-expected.
     */
    public void stop() {
        stopWifi();
        synchronized (mLock) { // prevents race condition
            mWifi = null;
        }
    }

    /**
     * HAL device manager status change listener.
     */
    public interface ManagerStatusListener {
        /**
         * Indicates that the status of the HalDeviceManager has changed. Use isReady() and
         * isStarted() to obtain status information.
         */
        void onStatusChanged();
    }

    /**
     * Return the set of supported interface types across all Wi-Fi chips on the device.
     *
     * @return A set of IfaceTypes constants (possibly empty, e.g. on error).
     */
    public Set<Integer> getSupportedIfaceTypes() {
        return getSupportedIfaceTypesInternal(null);
    }

    /**
     * Return the set of supported interface types for the specified Wi-Fi chip.
     *
     * @return A set of IfaceTypes constants  (possibly empty, e.g. on error).
     */
    public Set<Integer> getSupportedIfaceTypes(IWifiChip chip) {
        return getSupportedIfaceTypesInternal(chip);
    }

    // interface-specific behavior

    /**
     * Create a STA interface if possible. Changes chip mode and removes conflicting interfaces if
     * needed and permitted by priority.
     *
     * @param requiredChipCapabilities The bitmask of Capabilities which are required.
     *                                 See IWifiChip.hal for documentation.
     * @param destroyedListener Optional (nullable) listener to call when the allocated interface
     *                          is removed. Will only be registered and used if an interface is
     *                          created successfully.
     * @param handler Handler on which to dispatch listener. Null implies the listener will be
     *                invoked synchronously from the context of the client which triggered the
     *                iface destruction.
     * @param requestorWs Requestor worksource. This will be used to determine priority of this
     *                    interface using rules based on the requestor app's context.
     * @return A newly created interface - or null if the interface could not be created.
     */
    public IWifiStaIface createStaIface(
            long requiredChipCapabilities,
            @Nullable InterfaceDestroyedListener destroyedListener, @Nullable Handler handler,
            @NonNull WorkSource requestorWs) {
        return (IWifiStaIface) createIface(HDM_CREATE_IFACE_STA, requiredChipCapabilities,
                destroyedListener, handler, requestorWs);
    }

    /**
     * Create a STA interface if possible. Changes chip mode and removes conflicting interfaces if
     * needed and permitted by priority.
     *
     * @param destroyedListener Optional (nullable) listener to call when the allocated interface
     *                          is removed. Will only be registered and used if an interface is
     *                          created successfully.
     * @param handler Handler on which to dispatch listener. Null implies the listener will be
     *                invoked synchronously from the context of the client which triggered the
     *                iface destruction.
     * @param requestorWs Requestor worksource. This will be used to determine priority of this
     *                    interface using rules based on the requestor app's context.
     * @return A newly created interface - or null if the interface could not be created.
     */
    public IWifiStaIface createStaIface(
            @Nullable InterfaceDestroyedListener destroyedListener, @Nullable Handler handler,
            @NonNull WorkSource requestorWs) {
        return (IWifiStaIface) createStaIface(CHIP_CAPABILITY_ANY,
                destroyedListener, handler, requestorWs);
    }

    /**
     * Create AP interface if possible (see createStaIface doc).
     */
    public IWifiApIface createApIface(
            long requiredChipCapabilities,
            @Nullable InterfaceDestroyedListener destroyedListener, @Nullable Handler handler,
            @NonNull WorkSource requestorWs, boolean isBridged) {
        return (IWifiApIface) createIface(isBridged ? HDM_CREATE_IFACE_AP_BRIDGE
                : HDM_CREATE_IFACE_AP, requiredChipCapabilities, destroyedListener,
                handler, requestorWs);
    }

    /**
     * Create AP interface if possible (see createStaIface doc).
     */
    public IWifiApIface createApIface(
            @Nullable InterfaceDestroyedListener destroyedListener, @Nullable Handler handler,
            @NonNull WorkSource requestorWs, boolean isBridged) {
        return (IWifiApIface) createApIface(CHIP_CAPABILITY_ANY,
                destroyedListener, handler, requestorWs, isBridged);
    }

    /**
     * Create P2P interface if possible (see createStaIface doc).
     */
    public IWifiP2pIface createP2pIface(
            long requiredChipCapabilities,
            @Nullable InterfaceDestroyedListener destroyedListener,
            @Nullable Handler handler, @NonNull WorkSource requestorWs) {
        return (IWifiP2pIface) createIface(HDM_CREATE_IFACE_P2P, requiredChipCapabilities,
                destroyedListener, handler, requestorWs);
    }

    /**
     * Create P2P interface if possible (see createStaIface doc).
     */
    public IWifiP2pIface createP2pIface(@Nullable InterfaceDestroyedListener destroyedListener,
            @Nullable Handler handler, @NonNull WorkSource requestorWs) {
        return (IWifiP2pIface) createP2pIface(CHIP_CAPABILITY_ANY,
                destroyedListener, handler, requestorWs);
    }

    /**
     * Create NAN interface if possible (see createStaIface doc).
     */
    public IWifiNanIface createNanIface(@Nullable InterfaceDestroyedListener destroyedListener,
            @Nullable Handler handler, @NonNull WorkSource requestorWs) {
        return (IWifiNanIface) createIface(HDM_CREATE_IFACE_NAN, CHIP_CAPABILITY_ANY,
                destroyedListener, handler, requestorWs);
    }

    /**
     * Removes (releases/destroys) the given interface. Will trigger any registered
     * InterfaceDestroyedListeners.
     */
    public boolean removeIface(IWifiIface iface) {
        boolean success = removeIfaceInternal(iface);
        return success;
    }

    /**
     * Returns the IWifiChip corresponding to the specified interface (or null on error).
     *
     * Note: clients must not perform chip mode changes or interface management (create/delete)
     * operations on IWifiChip directly. However, they can use the IWifiChip interface to perform
     * other functions - e.g. calling the debug/trace methods.
     */
    public IWifiChip getChip(IWifiIface iface) {
        String name = getName(iface);
        int type = getType(iface);
        if (VDBG) Log.d(TAG, "getChip: iface(name)=" + name);

        synchronized (mLock) {
            InterfaceCacheEntry cacheEntry = mInterfaceInfoCache.get(Pair.create(name, type));
            if (cacheEntry == null) {
                Log.e(TAG, "getChip: no entry for iface(name)=" + name);
                return null;
            }

            return cacheEntry.chip;
        }
    }

    /**
     * Replace the requestorWs info for the associated info.
     *
     * When a new iface is requested via
     * {@link #createIface(int, InterfaceDestroyedListener, Handler, WorkSource)}, the clients
     * pass in a worksource which includes all the apps that triggered the iface creation. However,
     * this list of apps can change during the lifetime of the iface (as new apps request the same
     * iface or existing apps release their request for the iface). This API can be invoked multiple
     * times to replace the entire requestor info for the provided iface.
     *
     * Note: This is is wholesale replace of the requestor info. The corresponding client is
     * responsible for individual add/remove of apps in the WorkSource passed in.
     */
    public boolean replaceRequestorWs(@NonNull IWifiIface iface,
            @NonNull WorkSource newRequestorWs) {
        String name = getName(iface);
        int type = getType(iface);
        if (VDBG) {
            Log.d(TAG, "replaceRequestorWs: iface(name)=" + name + ", newRequestorWs="
                    + newRequestorWs);
        }

        synchronized (mLock) {
            InterfaceCacheEntry cacheEntry = mInterfaceInfoCache.get(Pair.create(name, type));
            if (cacheEntry == null) {
                Log.e(TAG, "replaceRequestorWs: no entry for iface(name)=" + name);
                return false;
            }
            cacheEntry.requestorWsHelper = mWifiInjector.makeWsHelper(newRequestorWs);
            return true;
        }
    }

    /**
     * Register a SubsystemRestartListener to listen to the subsystem restart event from HAL.
     * Use the action() to forward the event to SelfRecovery when receiving the event from HAL.
     *
     * @param listener SubsystemRestartListener listener object.
     * @param handler Handler on which to dispatch listener. Null implies the listener will be
     *                invoked synchronously from the context of the client which triggered the
     *                state change.
     */
    public void registerSubsystemRestartListener(@NonNull SubsystemRestartListener listener,
            @Nullable Handler handler) {
        if (listener == null) {
            Log.wtf(TAG, "registerSubsystemRestartListener with nulls!? listener=" + listener);
            return;
        }
        if (!mSubsystemRestartListener.add(new SubsystemRestartListenerProxy(listener, handler))) {
            Log.w(TAG, "registerSubsystemRestartListener: duplicate registration ignored");
        }
    }

    /**
     * Register an InterfaceDestroyedListener to the specified iface - returns true on success
     * and false on failure. This listener is in addition to the one registered when the interface
     * was created - allowing non-creators to monitor interface status.
     *
     * @param destroyedListener Listener to call when the allocated interface is removed.
     *                          Will only be registered and used if an interface is created
     *                          successfully.
     * @param handler Handler on which to dispatch listener. Null implies the listener will be
     *                invoked synchronously from the context of the client which triggered the
     *                iface destruction.
     */
    public boolean registerDestroyedListener(IWifiIface iface,
            @NonNull InterfaceDestroyedListener destroyedListener,
            @Nullable Handler handler) {
        String name = getName(iface);
        int type = getType(iface);
        if (VDBG) Log.d(TAG, "registerDestroyedListener: iface(name)=" + name);

        synchronized (mLock) {
            InterfaceCacheEntry cacheEntry = mInterfaceInfoCache.get(Pair.create(name, type));
            if (cacheEntry == null) {
                Log.e(TAG, "registerDestroyedListener: no entry for iface(name)=" + name);
                return false;
            }

            return cacheEntry.destroyedListeners.add(
                    new InterfaceDestroyedListenerProxy(name, destroyedListener, handler));
        }
    }

    /**
     * Register a callback object for RTT life-cycle events. The callback object registration
     * indicates that an RTT controller should be created whenever possible. The callback object
     * will be called with a new RTT controller whenever it is created (or at registration time
     * if an RTT controller already exists). The callback object will also be triggered whenever
     * an existing RTT controller is destroyed (the previous copies must be discarded by the
     * recipient).
     *
     * @param callback InterfaceRttControllerLifecycleCallback object.
     * @param handler Handler on which to dispatch callback
     */
    public void registerRttControllerLifecycleCallback(
            @NonNull InterfaceRttControllerLifecycleCallback callback, @NonNull Handler handler) {
        if (VDBG) {
            Log.d(TAG, "registerRttControllerLifecycleCallback: callback=" + callback + ", handler="
                    + handler);
        }

        if (callback == null || handler == null) {
            Log.wtf(TAG, "registerRttControllerLifecycleCallback with nulls!? callback=" + callback
                    + ", handler=" + handler);
            return;
        }

        synchronized (mLock) {
            InterfaceRttControllerLifecycleCallbackProxy proxy =
                    new InterfaceRttControllerLifecycleCallbackProxy(callback, handler);
            if (!mRttControllerLifecycleCallbacks.add(proxy)) {
                Log.d(TAG,
                        "registerRttControllerLifecycleCallback: registering an existing callback="
                                + callback);
                return;
            }

            if (mIWifiRttController == null) {
                mIWifiRttController = createRttControllerIfPossible();
            }
            if (mIWifiRttController != null) {
                proxy.onNewRttController(mIWifiRttController);
            }
        }
    }

    /**
     * Return the name of the input interface or null on error.
     */
    public static String getName(IWifiIface iface) {
        if (iface == null) {
            return "<null>";
        }

        Mutable<String> nameResp = new Mutable<>();
        try {
            iface.getName((WifiStatus status, String name) -> {
                if (status.code == WifiStatusCode.SUCCESS) {
                    nameResp.value = name;
                } else {
                    Log.e(TAG, "Error on getName: " + statusString(status));
                }
            });
        } catch (RemoteException e) {
            Log.e(TAG, "Exception on getName: " + e);
        }

        return nameResp.value;
    }

    /**
     * Called when subsystem restart
     */
    public interface SubsystemRestartListener {
        /**
         * Called for subsystem restart event from the HAL.
         * It will trigger recovery mechanism in framework.
         */
        void onSubsystemRestart();
    }

    /**
     * Called when interface is destroyed.
     */
    public interface InterfaceDestroyedListener {
        /**
         * Called for every interface on which registered when destroyed - whether
         * destroyed by releaseIface() or through chip mode change or through Wi-Fi
         * going down.
         *
         * Can be registered when the interface is requested with createXxxIface() - will
         * only be valid if the interface creation was successful - i.e. a non-null was returned.
         *
         * @param ifaceName Name of the interface that was destroyed.
         */
        void onDestroyed(@NonNull String ifaceName);
    }

    /**
     * Called on RTT controller lifecycle events. RTT controller is a singleton which will be
     * created when possible (after first lifecycle registration) and destroyed if necessary.
     *
     * Determination of availability is determined by the HAL. Creation attempts (if requested
     * by registration of interface) will be done on any mode changes.
     */
    public interface InterfaceRttControllerLifecycleCallback {
        /**
         * Called when an RTT controller was created (or for newly registered listeners - if it
         * was already available). The controller provided by this callback may be destroyed by
         * the HAL at which point the {@link #onRttControllerDestroyed()} will be called.
         *
         * Note: this callback can be triggered to replace an existing controller (instead of
         * calling the Destroyed callback in between).
         *
         * @param controller The RTT controller object.
         */
        void onNewRttController(@NonNull IWifiRttController controller);

        /**
         * Called when the previously provided RTT controller is destroyed. Clients must discard
         * their copy. A new copy may be provided later by
         * {@link #onNewRttController(IWifiRttController)}.
         */
        void onRttControllerDestroyed();
    }

    /**
     * Returns whether the provided Iface combo can be supported by the device.
     * Note: This only returns an answer based on the iface combination exposed by the HAL.
     * The actual iface creation/deletion rules depend on the iface priorities set in
     * {@link #allowedToDeleteIfaceTypeForRequestedType(int, WorkSource, int, WifiIfaceInfo[][])}
     *
     * @param ifaceCombo SparseArray keyed in by the iface type to number of ifaces needed.
     * @param requiredChipCapabilities The bitmask of Capabilities which are required.
     *                                 See IWifiChip.hal for documentation.
     * @return true if the device supports the provided combo, false otherwise.
     */
    public boolean canSupportIfaceCombo(SparseArray<Integer> ifaceCombo,
            long requiredChipCapabilities) {
        if (VDBG) {
            Log.d(TAG, "canSupportIfaceCombo: ifaceCombo=" + ifaceCombo
                    + ", requiredChipCapabilities=" + requiredChipCapabilities);
        }

        synchronized (mLock) {
            if (mWifi == null) return false;
            int[] ifaceComboArr = new int[IFACE_TYPES_BY_PRIORITY.length];
            for (int type : IFACE_TYPES_BY_PRIORITY) {
                ifaceComboArr[type] = ifaceCombo.get(type, 0);
            }
            WifiChipInfo[] chipInfos = getAllChipInfoCached();
            if (chipInfos == null) return false;
            return isItPossibleToCreateIfaceCombo(
                    chipInfos, requiredChipCapabilities, ifaceComboArr);
        }
    }

    /**
     * Returns whether the provided Iface combo can be supported by the device.
     * Note: This only returns an answer based on the iface combination exposed by the HAL.
     * The actual iface creation/deletion rules depend on the iface priorities set in
     * {@link #allowedToDeleteIfaceTypeForRequestedType(int, WorkSource, int, WifiIfaceInfo[][])}
     *
     * @param ifaceCombo SparseArray keyed in by the iface type to number of ifaces needed.
     * @return true if the device supports the provided combo, false otherwise.
     */
    public boolean canSupportIfaceCombo(SparseArray<Integer> ifaceCombo) {
        return canSupportIfaceCombo(ifaceCombo, CHIP_CAPABILITY_ANY);
    }

    /**
     * Returns whether the provided Iface can be requested by specifier requestor.
     *
     * @param ifaceType Type of iface requested.
     * @param requiredChipCapabilities The bitmask of Capabilities which are required.
     *                                 See IWifiChip.hal for documentation.
     * @param requestorWs Requestor worksource. This will be used to determine priority of this
     *                    interface using rules based on the requestor app's context.
     * @return true if the device supports the provided combo, false otherwise.
     */
    public boolean isItPossibleToCreateIface(
            int ifaceType, long requiredChipCapabilities, WorkSource requestorWs) {
        if (VDBG) {
            Log.d(TAG, "isItPossibleToCreateIface: ifaceType=" + ifaceType
                    + ", requiredChipCapabilities=" + requiredChipCapabilities);
        }
        synchronized (mLock) {
            if (mWifi == null) return false;
            WifiChipInfo[] chipInfos = getAllChipInfo();
            if (chipInfos == null) return false;
            if (!validateInterfaceCacheAndRetrieveRequestorWs(chipInfos)) {
                Log.e(TAG, "isItPossibleToCreateIface: local cache is invalid!");
                stopWifi(); // major error: shutting down
                return false;
            }
            return isItPossibleToCreateIface(
                    chipInfos, ifaceType, requiredChipCapabilities, requestorWs);
        }
    }

    /**
     * Returns whether the provided Iface can be requested by specifier requestor.
     *
     * @param ifaceType Type of iface requested.
     * @param requestorWs Requestor worksource. This will be used to determine priority of this
     *                    interface using rules based on the requestor app's context.
     * @return true if the device supports the provided combo, false otherwise.
     */
    public boolean isItPossibleToCreateIface(
            int ifaceType, WorkSource requestorWs) {
        return isItPossibleToCreateIface(
                ifaceType, CHIP_CAPABILITY_ANY, requestorWs);
    }

    // internal state

    /* This "PRIORITY" is not for deciding interface elimination (that is controlled by
     * allowedToDeleteIfaceTypeForRequestedType. This priority is used for:
     * - Comparing 2 configuration options
     * - Order of dispatch of available for request listeners
     */
    private static final int[] IFACE_TYPES_BY_PRIORITY =
            {IfaceType.AP, IfaceType.STA, IfaceType.P2P, IfaceType.NAN};

    private final Object mLock = new Object();

    private IServiceManager mServiceManager;
    private IWifi mWifi;
    private IWifiRttController mIWifiRttController;
    private final WifiEventCallback mWifiEventCallback = new WifiEventCallback();
    private final WifiEventCallbackV15 mWifiEventCallbackV15 = new WifiEventCallbackV15();
    private final Set<ManagerStatusListenerProxy> mManagerStatusListeners = new HashSet<>();
    private final Set<InterfaceRttControllerLifecycleCallbackProxy>
            mRttControllerLifecycleCallbacks = new HashSet<>();
    private final SparseArray<IWifiChipEventCallback.Stub> mDebugCallbacks = new SparseArray<>();
    private boolean mIsReady;
    private final Set<SubsystemRestartListenerProxy> mSubsystemRestartListener = new HashSet<>();

    /*
     * This is the only place where we cache HIDL information in this manager. Necessary since
     * we need to keep a list of registered destroyed listeners. Will be validated regularly
     * in getAllChipInfoAndValidateCache().
     */
    private final Map<Pair<String, Integer>, InterfaceCacheEntry> mInterfaceInfoCache =
            new HashMap<>();

    private class InterfaceCacheEntry {
        public IWifiChip chip;
        public int chipId;
        public String name;
        public int type;
        public Set<InterfaceDestroyedListenerProxy> destroyedListeners = new HashSet<>();
        public long creationTime;
        public WorkSourceHelper requestorWsHelper;

        @Override
        public String toString() {
            StringBuilder sb = new StringBuilder();
            sb.append("{name=").append(name).append(", type=").append(type)
                    .append(", destroyedListeners.size()=").append(destroyedListeners.size())
                    .append(", RequestorWs=").append(requestorWsHelper)
                    .append(", creationTime=").append(creationTime).append("}");
            return sb.toString();
        }
    }

    private class WifiIfaceInfo {
        public String name;
        public IWifiIface iface;
        public WorkSourceHelper requestorWsHelper;

        @Override
        public String toString() {
            return "{name=" + name + ", iface=" + iface + ", requestorWs=" + requestorWsHelper
                    + " }";
        }
    }

    private class WifiChipInfo {
        public IWifiChip chip;
        public int chipId;
        public ArrayList<IWifiChip.ChipMode> availableModes;
        public boolean currentModeIdValid;
        public int currentModeId;
        public WifiIfaceInfo[][] ifaces = new WifiIfaceInfo[IFACE_TYPES_BY_PRIORITY.length][];
        public long chipCapabilities;

        @Override
        public String toString() {
            StringBuilder sb = new StringBuilder();
            sb.append("{chipId=").append(chipId).append(", availableModes=").append(availableModes)
                    .append(", currentModeIdValid=").append(currentModeIdValid)
                    .append(", currentModeId=").append(currentModeId)
                    .append(", chipCapabilities=").append(chipCapabilities);
            for (int type: IFACE_TYPES_BY_PRIORITY) {
                sb.append(", ifaces[" + type + "].length=").append(ifaces[type].length);
            }
            sb.append("}");
            return sb.toString();
        }
    }

    /**
     * Wrapper function to access the HIDL services. Created to be mockable in unit-tests.
     */
    protected IWifi getWifiServiceMockable() {
        try {
            return IWifi.getService(true /* retry */);
        } catch (RemoteException e) {
            Log.e(TAG, "Exception getting IWifi service: " + e);
            return null;
        }
    }

    protected android.hardware.wifi.V1_5.IWifi getWifiServiceForV1_5Mockable(IWifi iWifi) {
        if (null == iWifi) return null;
        return android.hardware.wifi.V1_5.IWifi.castFrom(iWifi);
    }

    protected IServiceManager getServiceManagerMockable() {
        try {
            return IServiceManager.getService();
        } catch (RemoteException e) {
            Log.e(TAG, "Exception getting IServiceManager: " + e);
            return null;
        }
    }

    protected android.hardware.wifi.V1_5.IWifiChip getWifiChipForV1_5Mockable(IWifiChip chip) {
        if (null == chip) return null;
        return android.hardware.wifi.V1_5.IWifiChip.castFrom(chip);
    }

    // internal implementation

    private void initializeInternal() {
        initIServiceManagerIfNecessary();
        if (mIsVendorHalSupported) {
            initIWifiIfNecessary();
        }
    }

    private void teardownInternal() {
        managerStatusListenerDispatch();
        dispatchAllDestroyedListeners();

        mIWifiRttController = null;
        dispatchRttControllerLifecycleOnDestroyed();
        mRttControllerLifecycleCallbacks.clear();
    }

    private class ServiceManagerDeathRecipient implements DeathRecipient {
        @Override
        public void serviceDied(long cookie) {
            mEventHandler.post(() -> {
                Log.wtf(TAG, "IServiceManager died: cookie=" + cookie);
                synchronized (mLock) {
                    mServiceManager = null;
                    // theoretically can call initServiceManager again here - but
                    // there's no point since most likely system is going to reboot
                }
            });
        }
    }

    private final IServiceNotification mServiceNotificationCallback =
            new IServiceNotification.Stub() {
                @Override
                public void onRegistration(String fqName, String name,
                                           boolean preexisting) {
                    Log.d(TAG, "IWifi registration notification: fqName=" + fqName
                            + ", name=" + name + ", preexisting=" + preexisting);
                    synchronized (mLock) {
                        initIWifiIfNecessary();
                    }
                }
            };

    /**
     * Failures of IServiceManager are most likely system breaking in any case. Behavior here
     * will be to WTF and continue.
     */
    private void initIServiceManagerIfNecessary() {
        if (mDbg) Log.d(TAG, "initIServiceManagerIfNecessary");

        synchronized (mLock) {
            if (mServiceManager != null) {
                return;
            }

            mServiceManager = getServiceManagerMockable();
            if (mServiceManager == null) {
                Log.wtf(TAG, "Failed to get IServiceManager instance");
            } else {
                try {
                    if (!mServiceManager.linkToDeath(
                            mServiceManagerDeathRecipient, /* don't care */ 0)) {
                        Log.wtf(TAG, "Error on linkToDeath on IServiceManager");
                        mServiceManager = null;
                        return;
                    }

                    if (!mServiceManager.registerForNotifications(IWifi.kInterfaceName, "",
                            mServiceNotificationCallback)) {
                        Log.wtf(TAG, "Failed to register a listener for IWifi service");
                        mServiceManager = null;
                    }
                } catch (RemoteException e) {
                    Log.wtf(TAG, "Exception while operating on IServiceManager: " + e);
                    mServiceManager = null;
                }

                // Cache the result for the supporting vendor hal or not
                mIsVendorHalSupported = isSupportedInternal();
            }
        }
    }

    /**
     * Uses the IServiceManager to query if the vendor HAL is present in the VINTF for the device
     * or not.
     * @return true if supported, false otherwise.
     */
    private boolean isSupportedInternal() {
        if (VDBG) Log.d(TAG, "isSupportedInternal");

        synchronized (mLock) {
            if (mServiceManager == null) {
                Log.e(TAG, "isSupported: called but mServiceManager is null!?");
                return false;
            }
            try {
                List<String> wifiServices =
                        mServiceManager.listManifestByInterface(IWifi.kInterfaceName);
                return !wifiServices.isEmpty();
            } catch (RemoteException e) {
                Log.wtf(TAG, "Exception while operating on IServiceManager: " + e);
                return false;
            }
        }
    }

    private class WifiDeathRecipient implements DeathRecipient {
        @Override
        public void serviceDied(long cookie) {
            mEventHandler.post(() -> {
                Log.e(TAG, "IWifi HAL service died! Have a listener for it ... cookie=" + cookie);
                synchronized (mLock) { // prevents race condition with surrounding method
                    mWifi = null;
                    mIsReady = false;
                    teardownInternal();
                    // don't restart: wait for registration notification
                }
            });
        }
    }

    /**
     * Initialize IWifi and register death listener and event callback.
     *
     * - It is possible that IWifi is not ready - we have a listener on IServiceManager for it.
     * - It is not expected that any of the registrations will fail. Possible indication that
     *   service died after we obtained a handle to it.
     *
     * Here and elsewhere we assume that death listener will do the right thing!
    */
    private void initIWifiIfNecessary() {
        if (mDbg) Log.d(TAG, "initIWifiIfNecessary");

        synchronized (mLock) {
            if (mWifi != null) {
                return;
            }

            try {
                mWifi = getWifiServiceMockable();
                if (mWifi == null) {
                    Log.e(TAG, "IWifi not (yet) available - but have a listener for it ...");
                    return;
                }

                if (!mWifi.linkToDeath(mIWifiDeathRecipient, /* don't care */ 0)) {
                    Log.e(TAG, "Error on linkToDeath on IWifi - will retry later");
                    return;
                }

                WifiStatus status;
                android.hardware.wifi.V1_5.IWifi iWifiV15 = getWifiServiceForV1_5Mockable(mWifi);
                if (iWifiV15 != null) {
                    status = iWifiV15.registerEventCallback_1_5(mWifiEventCallbackV15);
                } else {
                    status = mWifi.registerEventCallback(mWifiEventCallback);
                }

                if (status.code != WifiStatusCode.SUCCESS) {
                    Log.e(TAG, "IWifi.registerEventCallback failed: " + statusString(status));
                    mWifi = null;
                    return;
                }
                // Stopping wifi just in case. This would also trigger the status callback.
                stopWifi();
                mIsReady = true;
            } catch (RemoteException e) {
                Log.e(TAG, "Exception while operating on IWifi: " + e);
            }
        }
    }

    /**
     * Registers event listeners on all IWifiChips after a successful start: DEBUG only!
     *
     * We don't need the listeners since any callbacks are just confirmation of status codes we
     * obtain directly from mode changes or interface creation/deletion.
     *
     * Relies (to the degree we care) on the service removing all listeners when Wi-Fi is stopped.
     */
    private void initIWifiChipDebugListeners() {
        if (VDBG) Log.d(TAG, "initIWifiChipDebugListeners");

        if (!VDBG) {
            return;
        }

        synchronized (mLock) {
            try {
                Mutable<Boolean> statusOk = new Mutable<>(false);
                Mutable<ArrayList<Integer>> chipIdsResp = new Mutable<>();

                // get all chip IDs
                mWifi.getChipIds((WifiStatus status, ArrayList<Integer> chipIds) -> {
                    statusOk.value = status.code == WifiStatusCode.SUCCESS;
                    if (statusOk.value) {
                        chipIdsResp.value = chipIds;
                    } else {
                        Log.e(TAG, "getChipIds failed: " + statusString(status));
                    }
                });
                if (!statusOk.value) {
                    return;
                }

                Log.d(TAG, "getChipIds=" + chipIdsResp.value);
                if (chipIdsResp.value.size() == 0) {
                    Log.e(TAG, "Should have at least 1 chip!");
                    return;
                }

                // register a callback for each chip
                Mutable<IWifiChip> chipResp = new Mutable<>();
                for (Integer chipId: chipIdsResp.value) {
                    mWifi.getChip(chipId, (WifiStatus status, IWifiChip chip) -> {
                        statusOk.value = status.code == WifiStatusCode.SUCCESS;
                        if (statusOk.value) {
                            chipResp.value = chip;
                        } else {
                            Log.e(TAG, "getChip failed: " + statusString(status));
                        }
                    });
                    if (!statusOk.value) {
                        continue; // still try next one?
                    }

                    IWifiChipEventCallback.Stub callback =
                            new IWifiChipEventCallback.Stub() {
                                @Override
                                public void onChipReconfigured(int modeId) throws RemoteException {
                                    Log.d(TAG, "onChipReconfigured: modeId=" + modeId);
                                }

                                @Override
                                public void onChipReconfigureFailure(WifiStatus status)
                                        throws RemoteException {
                                    Log.d(TAG, "onChipReconfigureFailure: status=" + statusString(
                                            status));
                                }

                                @Override
                                public void onIfaceAdded(int type, String name)
                                        throws RemoteException {
                                    Log.d(TAG, "onIfaceAdded: type=" + type + ", name=" + name);
                                }

                                @Override
                                public void onIfaceRemoved(int type, String name)
                                        throws RemoteException {
                                    Log.d(TAG, "onIfaceRemoved: type=" + type + ", name=" + name);
                                }

                                @Override
                                public void onDebugRingBufferDataAvailable(
                                        WifiDebugRingBufferStatus status,
                                        ArrayList<Byte> data) throws RemoteException {
                                    Log.d(TAG, "onDebugRingBufferDataAvailable");
                                }

                                @Override
                                public void onDebugErrorAlert(int errorCode,
                                        ArrayList<Byte> debugData)
                                        throws RemoteException {
                                    Log.d(TAG, "onDebugErrorAlert");
                                }
                            };
                    mDebugCallbacks.put(chipId, callback); // store to prevent GC: needed by HIDL
                    WifiStatus status = chipResp.value.registerEventCallback(callback);
                    if (status.code != WifiStatusCode.SUCCESS) {
                        Log.e(TAG, "registerEventCallback failed: " + statusString(status));
                        continue; // still try next one?
                    }
                }
            } catch (RemoteException e) {
                Log.e(TAG, "initIWifiChipDebugListeners: exception: " + e);
                return;
            }
        }
    }

    @Nullable
    private WifiChipInfo[] mCachedWifiChipInfos = null;

    /**
     * Get current information about all the chips in the system: modes, current mode (if any), and
     * any existing interfaces.
     *
     * Intended to be called for any external iface support related queries. This information is
     * cached to reduce performance overhead (unlike {@link #getAllChipInfo()}).
     */
    private WifiChipInfo[] getAllChipInfoCached() {
        if (mCachedWifiChipInfos == null) {
            mCachedWifiChipInfos = getAllChipInfo();
        }
        return mCachedWifiChipInfos;
    }

    /**
     * Get current information about all the chips in the system: modes, current mode (if any), and
     * any existing interfaces.
     *
     * Intended to be called whenever we need to configure the chips - information is NOT cached (to
     * reduce the likelihood that we get out-of-sync).
     */
    private WifiChipInfo[] getAllChipInfo() {
        if (VDBG) Log.d(TAG, "getAllChipInfo");

        synchronized (mLock) {
            if (!isWifiStarted()) {
                return null;
            }

            try {
                Mutable<Boolean> statusOk = new Mutable<>(false);
                Mutable<ArrayList<Integer>> chipIdsResp = new Mutable<>();

                // get all chip IDs
                mWifi.getChipIds((WifiStatus status, ArrayList<Integer> chipIds) -> {
                    statusOk.value = status.code == WifiStatusCode.SUCCESS;
                    if (statusOk.value) {
                        chipIdsResp.value = chipIds;
                    } else {
                        Log.e(TAG, "getChipIds failed: " + statusString(status));
                    }
                });
                if (!statusOk.value) {
                    return null;
                }

                if (VDBG) Log.d(TAG, "getChipIds=" + chipIdsResp.value);
                if (chipIdsResp.value.size() == 0) {
                    Log.e(TAG, "Should have at least 1 chip!");
                    return null;
                }

                int chipInfoIndex = 0;
                WifiChipInfo[] chipsInfo = new WifiChipInfo[chipIdsResp.value.size()];

                Mutable<IWifiChip> chipResp = new Mutable<>();
                for (Integer chipId: chipIdsResp.value) {
                    mWifi.getChip(chipId, (WifiStatus status, IWifiChip chip) -> {
                        statusOk.value = status.code == WifiStatusCode.SUCCESS;
                        if (statusOk.value) {
                            chipResp.value = chip;
                        } else {
                            Log.e(TAG, "getChip failed: " + statusString(status));
                        }
                    });
                    if (!statusOk.value) {
                        return null;
                    }

                    Mutable<ArrayList<IWifiChip.ChipMode>> availableModesResp = new Mutable<>();
                    chipResp.value.getAvailableModes(
                            (WifiStatus status, ArrayList<IWifiChip.ChipMode> modes) -> {
                                statusOk.value = status.code == WifiStatusCode.SUCCESS;
                                if (statusOk.value) {
                                    availableModesResp.value = modes;
                                } else {
                                    Log.e(TAG, "getAvailableModes failed: " + statusString(status));
                                }
                            });
                    if (!statusOk.value) {
                        return null;
                    }

                    Mutable<Boolean> currentModeValidResp = new Mutable<>(false);
                    Mutable<Integer> currentModeResp = new Mutable<>(0);
                    chipResp.value.getMode((WifiStatus status, int modeId) -> {
                        statusOk.value = status.code == WifiStatusCode.SUCCESS;
                        if (statusOk.value) {
                            currentModeValidResp.value = true;
                            currentModeResp.value = modeId;
                        } else if (status.code == WifiStatusCode.ERROR_NOT_AVAILABLE) {
                            statusOk.value = true; // valid response
                        } else {
                            Log.e(TAG, "getMode failed: " + statusString(status));
                        }
                    });
                    if (!statusOk.value) {
                        return null;
                    }

                    Mutable<Long> chipCapabilities = new Mutable<>(0L);
                    chipCapabilities.value = getChipCapabilities(chipResp.value);

                    Mutable<ArrayList<String>> ifaceNamesResp = new Mutable<>();
                    Mutable<Integer> ifaceIndex = new Mutable<>(0);

                    chipResp.value.getStaIfaceNames(
                            (WifiStatus status, ArrayList<String> ifnames) -> {
                                statusOk.value = status.code == WifiStatusCode.SUCCESS;
                                if (statusOk.value) {
                                    ifaceNamesResp.value = ifnames;
                                } else {
                                    Log.e(TAG, "getStaIfaceNames failed: " + statusString(status));
                                }
                            });
                    if (!statusOk.value) {
                        return null;
                    }

                    WifiIfaceInfo[] staIfaces = new WifiIfaceInfo[ifaceNamesResp.value.size()];
                    for (String ifaceName: ifaceNamesResp.value) {
                        chipResp.value.getStaIface(ifaceName,
                                (WifiStatus status, IWifiStaIface iface) -> {
                                    statusOk.value = status.code == WifiStatusCode.SUCCESS;
                                    if (statusOk.value) {
                                        WifiIfaceInfo ifaceInfo = new WifiIfaceInfo();
                                        ifaceInfo.name = ifaceName;
                                        ifaceInfo.iface = iface;
                                        staIfaces[ifaceIndex.value++] = ifaceInfo;
                                    } else {
                                        Log.e(TAG, "getStaIface failed: " + statusString(status));
                                    }
                                });
                        if (!statusOk.value) {
                            return null;
                        }
                    }

                    ifaceIndex.value = 0;
                    chipResp.value.getApIfaceNames(
                            (WifiStatus status, ArrayList<String> ifnames) -> {
                                statusOk.value = status.code == WifiStatusCode.SUCCESS;
                                if (statusOk.value) {
                                    ifaceNamesResp.value = ifnames;
                                } else {
                                    Log.e(TAG, "getApIfaceNames failed: " + statusString(status));
                                }
                            });
                    if (!statusOk.value) {
                        return null;
                    }

                    WifiIfaceInfo[] apIfaces = new WifiIfaceInfo[ifaceNamesResp.value.size()];
                    for (String ifaceName: ifaceNamesResp.value) {
                        chipResp.value.getApIface(ifaceName,
                                (WifiStatus status, IWifiApIface iface) -> {
                                    statusOk.value = status.code == WifiStatusCode.SUCCESS;
                                    if (statusOk.value) {
                                        WifiIfaceInfo ifaceInfo = new WifiIfaceInfo();
                                        ifaceInfo.name = ifaceName;
                                        ifaceInfo.iface = iface;
                                        apIfaces[ifaceIndex.value++] = ifaceInfo;
                                    } else {
                                        Log.e(TAG, "getApIface failed: " + statusString(status));
                                    }
                                });
                        if (!statusOk.value) {
                            return null;
                        }
                    }

                    ifaceIndex.value = 0;
                    chipResp.value.getP2pIfaceNames(
                            (WifiStatus status, ArrayList<String> ifnames) -> {
                                statusOk.value = status.code == WifiStatusCode.SUCCESS;
                                if (statusOk.value) {
                                    ifaceNamesResp.value = ifnames;
                                } else {
                                    Log.e(TAG, "getP2pIfaceNames failed: " + statusString(status));
                                }
                            });
                    if (!statusOk.value) {
                        return null;
                    }

                    WifiIfaceInfo[] p2pIfaces = new WifiIfaceInfo[ifaceNamesResp.value.size()];
                    for (String ifaceName: ifaceNamesResp.value) {
                        chipResp.value.getP2pIface(ifaceName,
                                (WifiStatus status, IWifiP2pIface iface) -> {
                                    statusOk.value = status.code == WifiStatusCode.SUCCESS;
                                    if (statusOk.value) {
                                        WifiIfaceInfo ifaceInfo = new WifiIfaceInfo();
                                        ifaceInfo.name = ifaceName;
                                        ifaceInfo.iface = iface;
                                        p2pIfaces[ifaceIndex.value++] = ifaceInfo;
                                    } else {
                                        Log.e(TAG, "getP2pIface failed: " + statusString(status));
                                    }
                                });
                        if (!statusOk.value) {
                            return null;
                        }
                    }

                    ifaceIndex.value = 0;
                    chipResp.value.getNanIfaceNames(
                            (WifiStatus status, ArrayList<String> ifnames) -> {
                                statusOk.value = status.code == WifiStatusCode.SUCCESS;
                                if (statusOk.value) {
                                    ifaceNamesResp.value = ifnames;
                                } else {
                                    Log.e(TAG, "getNanIfaceNames failed: " + statusString(status));
                                }
                            });
                    if (!statusOk.value) {
                        return null;
                    }

                    WifiIfaceInfo[] nanIfaces = new WifiIfaceInfo[ifaceNamesResp.value.size()];
                    for (String ifaceName: ifaceNamesResp.value) {
                        chipResp.value.getNanIface(ifaceName,
                                (WifiStatus status, IWifiNanIface iface) -> {
                                    statusOk.value = status.code == WifiStatusCode.SUCCESS;
                                    if (statusOk.value) {
                                        WifiIfaceInfo ifaceInfo = new WifiIfaceInfo();
                                        ifaceInfo.name = ifaceName;
                                        ifaceInfo.iface = iface;
                                        nanIfaces[ifaceIndex.value++] = ifaceInfo;
                                    } else {
                                        Log.e(TAG, "getNanIface failed: " + statusString(status));
                                    }
                                });
                        if (!statusOk.value) {
                            return null;
                        }
                    }

                    WifiChipInfo chipInfo = new WifiChipInfo();
                    chipsInfo[chipInfoIndex++] = chipInfo;

                    chipInfo.chip = chipResp.value;
                    chipInfo.chipId = chipId;
                    chipInfo.availableModes = availableModesResp.value;
                    chipInfo.currentModeIdValid = currentModeValidResp.value;
                    chipInfo.currentModeId = currentModeResp.value;
                    chipInfo.chipCapabilities = chipCapabilities.value;
                    chipInfo.ifaces[IfaceType.STA] = staIfaces;
                    chipInfo.ifaces[IfaceType.AP] = apIfaces;
                    chipInfo.ifaces[IfaceType.P2P] = p2pIfaces;
                    chipInfo.ifaces[IfaceType.NAN] = nanIfaces;
                }
                return chipsInfo;
            } catch (RemoteException e) {
                Log.e(TAG, "getAllChipInfoAndValidateCache exception: " + e);
            }
        }

        return null;
    }

    /**
     * Checks the local state of this object (the cached state) against the input 'chipInfos'
     * state (which is a live representation of the Wi-Fi firmware status - read through the HAL).
     * Returns 'true' if there are no discrepancies - 'false' otherwise.
     *
     * A discrepancy is if any local state contains references to a chip or interface which are not
     * found on the information read from the chip.
     *
     * Also, fills in the |requestorWs| corresponding to each active iface in |WifiChipInfo|.
     */
    private boolean validateInterfaceCacheAndRetrieveRequestorWs(WifiChipInfo[] chipInfos) {
        if (VDBG) Log.d(TAG, "validateInterfaceCache");

        synchronized (mLock) {
            for (InterfaceCacheEntry entry: mInterfaceInfoCache.values()) {
                // search for chip
                WifiChipInfo matchingChipInfo = null;
                for (WifiChipInfo ci: chipInfos) {
                    if (ci.chipId == entry.chipId) {
                        matchingChipInfo = ci;
                        break;
                    }
                }
                if (matchingChipInfo == null) {
                    Log.e(TAG, "validateInterfaceCache: no chip found for " + entry);
                    return false;
                }

                // search for interface
                WifiIfaceInfo[] ifaceInfoList = matchingChipInfo.ifaces[entry.type];
                if (ifaceInfoList == null) {
                    Log.e(TAG, "validateInterfaceCache: invalid type on entry " + entry);
                    return false;
                }

                boolean matchFound = false;
                for (WifiIfaceInfo ifaceInfo: ifaceInfoList) {
                    if (ifaceInfo.name.equals(entry.name)) {
                        ifaceInfo.requestorWsHelper = entry.requestorWsHelper;
                        matchFound = true;
                        break;
                    }
                }
                if (!matchFound) {
                    Log.e(TAG, "validateInterfaceCache: no interface found for " + entry);
                    return false;
                }
            }
        }

        return true;
    }

    private boolean isWifiStarted() {
        if (VDBG) Log.d(TAG, "isWifiStart");

        synchronized (mLock) {
            try {
                if (mWifi == null) {
                    return false;
                } else {
                    return mWifi.isStarted();
                }
            } catch (RemoteException e) {
                Log.e(TAG, "isWifiStarted exception: " + e);
                return false;
            }
        }
    }

    private boolean startWifi() {
        if (VDBG) Log.d(TAG, "startWifi");
        initIWifiIfNecessary();
        synchronized (mLock) {
            try {
                if (mWifi == null) {
                    Log.w(TAG, "startWifi called but mWifi is null!?");
                    return false;
                } else {
                    int triedCount = 0;
                    while (triedCount <= START_HAL_RETRY_TIMES) {
                        WifiStatus status = mWifi.start();
                        if (status.code == WifiStatusCode.SUCCESS) {
                            initIWifiChipDebugListeners();
                            managerStatusListenerDispatch();
                            if (triedCount != 0) {
                                Log.d(TAG, "start IWifi succeeded after trying "
                                         + triedCount + " times");
                            }
                            return true;
                        } else if (status.code == WifiStatusCode.ERROR_NOT_AVAILABLE) {
                            // Should retry. Hal might still be stopping.
                            Log.e(TAG, "Cannot start IWifi: " + statusString(status)
                                    + ", Retrying...");
                            try {
                                Thread.sleep(START_HAL_RETRY_INTERVAL_MS);
                            } catch (InterruptedException ignore) {
                                // no-op
                            }
                            triedCount++;
                        } else {
                            // Should not retry on other failures.
                            Log.e(TAG, "Cannot start IWifi: " + statusString(status));
                            return false;
                        }
                    }
                    Log.e(TAG, "Cannot start IWifi after trying " + triedCount + " times");
                    return false;
                }
            } catch (RemoteException e) {
                Log.e(TAG, "startWifi exception: " + e);
                return false;
            }
        }
    }

    private void stopWifi() {
        if (VDBG) Log.d(TAG, "stopWifi");

        synchronized (mLock) {
            try {
                if (mWifi == null) {
                    Log.w(TAG, "stopWifi called but mWifi is null!?");
                } else {
                    WifiStatus status = mWifi.stop();
                    if (status.code != WifiStatusCode.SUCCESS) {
                        Log.e(TAG, "Cannot stop IWifi: " + statusString(status));
                    }

                    // even on failure since WTF??
                    teardownInternal();
                }
            } catch (RemoteException e) {
                Log.e(TAG, "stopWifi exception: " + e);
            }
        }
    }

    private class WifiEventCallback extends IWifiEventCallback.Stub {
        @Override
        public void onStart() throws RemoteException {
            mEventHandler.post(() -> {
                if (VDBG) Log.d(TAG, "IWifiEventCallback.onStart");
                // NOP: only happens in reaction to my calls - will handle directly
            });
        }

        @Override
        public void onStop() throws RemoteException {
            mEventHandler.post(() -> {
                if (VDBG) Log.d(TAG, "IWifiEventCallback.onStop");
                // NOP: only happens in reaction to my calls - will handle directly
            });
        }

        @Override
        public void onFailure(WifiStatus status) throws RemoteException {
            mEventHandler.post(() -> {
                Log.e(TAG, "IWifiEventCallback.onFailure: " + statusString(status));
                synchronized (mLock) {
                    mIsReady = false;
                    teardownInternal();
                }
            });
        }
    }

    private class WifiEventCallbackV15 extends
            android.hardware.wifi.V1_5.IWifiEventCallback.Stub {
        private final WifiEventCallback mWifiEventCallback = new WifiEventCallback();
        @Override
        public void onStart() throws RemoteException {
            mWifiEventCallback.onStart();
        }

        @Override
        public void onStop() throws RemoteException {
            mWifiEventCallback.onStop();
        }

        @Override
        public void onFailure(WifiStatus status) throws RemoteException {
            mWifiEventCallback.onFailure(status);
        }

        @Override
        public void onSubsystemRestart(WifiStatus status) throws RemoteException {
            mEventHandler.post(() -> {
                synchronized (mLock) {
                    for (SubsystemRestartListenerProxy cb : mSubsystemRestartListener) {
                        cb.action();
                    }
                }
            });
        }
    }

    private void managerStatusListenerDispatch() {
        synchronized (mLock) {
            for (ManagerStatusListenerProxy cb : mManagerStatusListeners) {
                cb.trigger();
            }
        }
    }

    private class ManagerStatusListenerProxy  extends
            ListenerProxy<ManagerStatusListener> {
        ManagerStatusListenerProxy(ManagerStatusListener statusListener, Handler handler) {
            super(statusListener, handler, "ManagerStatusListenerProxy");
        }

        @Override
        protected void action() {
            mListener.onStatusChanged();
        }
    }

    private Set<Integer> getSupportedIfaceTypesInternal(IWifiChip chip) {
        Set<Integer> results = new HashSet<>();

        WifiChipInfo[] chipInfos = getAllChipInfoCached();
        if (chipInfos == null) {
            Log.e(TAG, "getSupportedIfaceTypesInternal: no chip info found");
            return results;
        }

        Mutable<Integer> chipIdIfProvided = new Mutable<>(0); // NOT using 0 as a magic value
        if (chip != null) {
            Mutable<Boolean> statusOk = new Mutable<>(false);
            try {
                chip.getId((WifiStatus status, int id) -> {
                    if (status.code == WifiStatusCode.SUCCESS) {
                        chipIdIfProvided.value = id;
                        statusOk.value = true;
                    } else {
                        Log.e(TAG, "getSupportedIfaceTypesInternal: IWifiChip.getId() error: "
                                + statusString(status));
                        statusOk.value = false;
                    }
                });
            } catch (RemoteException e) {
                Log.e(TAG, "getSupportedIfaceTypesInternal IWifiChip.getId() exception: " + e);
                return results;
            }
            if (!statusOk.value) {
                return results;
            }
        }

        for (WifiChipInfo wci: chipInfos) {
            if (chip != null && wci.chipId != chipIdIfProvided.value) {
                continue;
            }

            for (IWifiChip.ChipMode cm: wci.availableModes) {
                for (IWifiChip.ChipIfaceCombination cic: cm.availableCombinations) {
                    for (IWifiChip.ChipIfaceCombinationLimit cicl: cic.limits) {
                        for (int type: cicl.types) {
                            results.add(type);
                        }
                    }
                }
            }
        }

        return results;
    }

    private IWifiIface createIface(@HdmIfaceTypeForCreation int createIfaceType,
            long requiredChipCapabilities, InterfaceDestroyedListener destroyedListener,
            Handler handler, WorkSource requestorWs) {
        if (mDbg) {
            Log.d(TAG, "createIface: createIfaceType=" + createIfaceType
                    + ", requiredChipCapabilities=" + requiredChipCapabilities
                    + ", requestorWs=" + requestorWs);
        }

        synchronized (mLock) {
            WifiChipInfo[] chipInfos = getAllChipInfo();
            if (chipInfos == null) {
                Log.e(TAG, "createIface: no chip info found");
                stopWifi(); // major error: shutting down
                return null;
            }

            if (!validateInterfaceCacheAndRetrieveRequestorWs(chipInfos)) {
                Log.e(TAG, "createIface: local cache is invalid!");
                stopWifi(); // major error: shutting down
                return null;
            }

            return createIfaceIfPossible(
                    chipInfos, createIfaceType, requiredChipCapabilities,
                    destroyedListener, handler, requestorWs);
        }
    }

    private static boolean isChipCapabilitiesSupported(@NonNull WifiChipInfo chipInfo,
            long requiredChipCapabilities) {
        if (chipInfo == null) return false;

        if (requiredChipCapabilities == CHIP_CAPABILITY_ANY) return true;

        if (CHIP_CAPABILITY_UNINITIALIZED == chipInfo.chipCapabilities) return true;

        return (chipInfo.chipCapabilities & requiredChipCapabilities)
                == requiredChipCapabilities;
    }

    private IWifiIface createIfaceIfPossible(
            WifiChipInfo[] chipInfos, @HdmIfaceTypeForCreation int createIfaceType,
            long requiredChipCapabilities, InterfaceDestroyedListener destroyedListener,
            Handler handler, WorkSource requestorWs) {
        int targetHalIfaceType = mHalIfaceMap.get(createIfaceType);
        if (VDBG) {
            Log.d(TAG, "createIfaceIfPossible: chipInfos=" + Arrays.deepToString(chipInfos)
                    + ", createIfaceType=" + createIfaceType
                    + ", targetHalIfaceType=" + targetHalIfaceType
                    + ", requiredChipCapabilities=" + requiredChipCapabilities
                    + ", requestorWs=" + requestorWs);
        }
        synchronized (mLock) {
            IfaceCreationData bestIfaceCreationProposal = null;
            for (WifiChipInfo chipInfo: chipInfos) {
                if (!isChipCapabilitiesSupported(chipInfo, requiredChipCapabilities)) continue;
                for (IWifiChip.ChipMode chipMode: chipInfo.availableModes) {
                    for (IWifiChip.ChipIfaceCombination chipIfaceCombo : chipMode
                            .availableCombinations) {
                        int[][] expandedIfaceCombos = expandIfaceCombos(chipIfaceCombo);
                        if (VDBG) {
                            Log.d(TAG, chipIfaceCombo + " expands to "
                                    + Arrays.deepToString(expandedIfaceCombos));
                        }

                        for (int[] expandedIfaceCombo: expandedIfaceCombos) {
                            IfaceCreationData currentProposal = canIfaceComboSupportRequest(
                                    chipInfo, chipMode, expandedIfaceCombo, targetHalIfaceType,
                                    requestorWs);
                            if (compareIfaceCreationData(currentProposal,
                                    bestIfaceCreationProposal)) {
                                if (VDBG) Log.d(TAG, "new proposal accepted");
                                bestIfaceCreationProposal = currentProposal;
                            }
                        }
                    }
                }
            }

            if (bestIfaceCreationProposal != null) {
                IWifiIface iface = executeChipReconfiguration(bestIfaceCreationProposal,
                        createIfaceType);
                if (iface != null) {
                    InterfaceCacheEntry cacheEntry = new InterfaceCacheEntry();

                    cacheEntry.chip = bestIfaceCreationProposal.chipInfo.chip;
                    cacheEntry.chipId = bestIfaceCreationProposal.chipInfo.chipId;
                    cacheEntry.name = getName(iface);
                    cacheEntry.type = targetHalIfaceType;
                    cacheEntry.requestorWsHelper = mWifiInjector.makeWsHelper(requestorWs);
                    if (destroyedListener != null) {
                        cacheEntry.destroyedListeners.add(
                                new InterfaceDestroyedListenerProxy(
                                        cacheEntry.name, destroyedListener, handler));
                    }
                    cacheEntry.creationTime = mClock.getUptimeSinceBootMillis();

                    if (mDbg) Log.d(TAG, "createIfaceIfPossible: added cacheEntry=" + cacheEntry);
                    mInterfaceInfoCache.put(
                            Pair.create(cacheEntry.name, cacheEntry.type), cacheEntry);
                    return iface;
                }
            }
        }

        Log.d(TAG, "createIfaceIfPossible: Failed to create iface for ifaceType=" + createIfaceType
                + ", requestorWs=" + requestorWs);
        return null;
    }

    // similar to createIfaceIfPossible - but simpler code: not looking for best option just
    // for any option (so terminates on first one).
    private boolean isItPossibleToCreateIface(WifiChipInfo[] chipInfos,
            int ifaceType, long requiredChipCapabilities,
            WorkSource requestorWs) {
        if (VDBG) {
            Log.d(TAG, "isItPossibleToCreateIface: chipInfos=" + Arrays.deepToString(chipInfos)
                    + ", ifaceType=" + ifaceType
                    + ", requiredChipCapabilities=" + requiredChipCapabilities);
        }

        for (WifiChipInfo chipInfo: chipInfos) {
            if (!isChipCapabilitiesSupported(chipInfo, requiredChipCapabilities)) continue;
            for (IWifiChip.ChipMode chipMode: chipInfo.availableModes) {
                for (IWifiChip.ChipIfaceCombination chipIfaceCombo : chipMode
                        .availableCombinations) {
                    int[][] expandedIfaceCombos = expandIfaceCombos(chipIfaceCombo);
                    if (VDBG) {
                        Log.d(TAG, chipIfaceCombo + " expands to "
                                + Arrays.deepToString(expandedIfaceCombos));
                    }

                    for (int[] expandedIfaceCombo: expandedIfaceCombos) {
                        if (canIfaceComboSupportRequest(chipInfo, chipMode, expandedIfaceCombo,
                                ifaceType, requestorWs) != null) {
                            return true;
                        }
                    }
                }
            }
        }

        return false;
    }

    /**
     * Expands (or provides an alternative representation) of the ChipIfaceCombination as all
     * possible combinations of interface.
     *
     * Returns [# of combinations][4 (IfaceType)]
     *
     * Note: there could be duplicates - allow (inefficient but ...).
     * TODO: optimize by using a Set as opposed to a []: will remove duplicates. Will need to
     * provide correct hashes.
     */
    private int[][] expandIfaceCombos(IWifiChip.ChipIfaceCombination chipIfaceCombo) {
        int numOfCombos = 1;
        for (IWifiChip.ChipIfaceCombinationLimit limit: chipIfaceCombo.limits) {
            for (int i = 0; i < limit.maxIfaces; ++i) {
                numOfCombos *= limit.types.size();
            }
        }

        int[][] expandedIfaceCombos = new int[numOfCombos][IFACE_TYPES_BY_PRIORITY.length];

        int span = numOfCombos; // span of an individual type (or sub-tree size)
        for (IWifiChip.ChipIfaceCombinationLimit limit: chipIfaceCombo.limits) {
            for (int i = 0; i < limit.maxIfaces; ++i) {
                span /= limit.types.size();
                for (int k = 0; k < numOfCombos; ++k) {
                    expandedIfaceCombos[k][limit.types.get((k / span) % limit.types.size())]++;
                }
            }
        }

        return expandedIfaceCombos;
    }

    private class IfaceCreationData {
        public WifiChipInfo chipInfo;
        public int chipModeId;
        public List<WifiIfaceInfo> interfacesToBeRemovedFirst;

        @Override
        public String toString() {
            StringBuilder sb = new StringBuilder();
            sb.append("{chipInfo=").append(chipInfo).append(", chipModeId=").append(chipModeId)
                    .append(", interfacesToBeRemovedFirst=").append(interfacesToBeRemovedFirst)
                    .append(")");
            return sb.toString();
        }
    }

    /**
     * Checks whether the input chip-iface-combo can support the requested interface type: if not
     * then returns null, if yes then returns information containing the list of interfaces which
     * would have to be removed first before the requested interface can be created.
     *
     * Note: the list of interfaces to be removed is EMPTY if a chip mode change is required - in
     * that case ALL the interfaces on the current chip have to be removed first.
     *
     * Response determined based on:
     * - Mode configuration: i.e. could the mode support the interface type in principle
     */
    private IfaceCreationData canIfaceComboSupportRequest(WifiChipInfo chipInfo,
            IWifiChip.ChipMode chipMode, int[] chipIfaceCombo, int ifaceType,
            WorkSource requestorWs) {
        if (VDBG) {
            Log.d(TAG, "canIfaceComboSupportRequest: chipInfo=" + chipInfo + ", chipMode="
                    + chipMode + ", chipIfaceCombo=" + Arrays.toString(chipIfaceCombo)
                    + ", ifaceType=" + ifaceType + ", requestorWs=" + requestorWs);
        }

        // short-circuit: does the chipIfaceCombo even support the requested type?
        if (chipIfaceCombo[ifaceType] == 0) {
            if (VDBG) Log.d(TAG, "Requested type not supported by combo");
            return null;
        }

        boolean isChipModeChangeProposed =
                chipInfo.currentModeIdValid && chipInfo.currentModeId != chipMode.id;

        // short-circuit: can't change chip-mode if an existing interface on this chip has a higher
        // priority than the requested interface
        if (isChipModeChangeProposed) {
            for (int type: IFACE_TYPES_BY_PRIORITY) {
                if (chipInfo.ifaces[type].length != 0) {
                    if (!allowedToDeleteIfaceTypeForRequestedType(
                            ifaceType, requestorWs, type, chipInfo.ifaces)) {
                        if (VDBG) {
                            Log.d(TAG, "Couldn't delete existing type " + type
                                    + " interfaces for requested type");
                        }
                        return null;
                    }
                }
            }

            // but if priority allows the mode change then we're good to go
            IfaceCreationData ifaceCreationData = new IfaceCreationData();
            ifaceCreationData.chipInfo = chipInfo;
            ifaceCreationData.chipModeId = chipMode.id;

            return ifaceCreationData;
        }

        // possibly supported
        List<WifiIfaceInfo> interfacesToBeRemovedFirst = new ArrayList<>();

        for (int type: IFACE_TYPES_BY_PRIORITY) {
            int tooManyInterfaces = chipInfo.ifaces[type].length - chipIfaceCombo[type];

            // need to count the requested interface as well
            if (type == ifaceType) {
                tooManyInterfaces += 1;
            }

            if (tooManyInterfaces > 0) { // may need to delete some
                if (!allowedToDeleteIfaceTypeForRequestedType(
                        ifaceType, requestorWs, type, chipInfo.ifaces)) {
                    if (VDBG) {
                        Log.d(TAG, "Would need to delete some higher priority interfaces");
                    }
                    return null;
                }

                // delete the most recently created interfaces
                interfacesToBeRemovedFirst.addAll(selectInterfacesToDelete(tooManyInterfaces,
                        ifaceType, requestorWs, type, chipInfo.ifaces[type]));
            }
        }

        IfaceCreationData ifaceCreationData = new IfaceCreationData();
        ifaceCreationData.chipInfo = chipInfo;
        ifaceCreationData.chipModeId = chipMode.id;
        ifaceCreationData.interfacesToBeRemovedFirst = interfacesToBeRemovedFirst;

        return ifaceCreationData;
    }

    /**
     * Compares two options to create an interface and determines which is the 'best'. Returns
     * true if proposal 1 (val1) is better, other false.
     *
     * Note: both proposals are 'acceptable' bases on priority criteria.
     *
     * Criteria:
     * - Proposal is better if it means removing fewer high priority interfaces
     */
    private boolean compareIfaceCreationData(IfaceCreationData val1, IfaceCreationData val2) {
        if (VDBG) Log.d(TAG, "compareIfaceCreationData: val1=" + val1 + ", val2=" + val2);

        // deal with trivial case of one or the other being null
        if (val1 == null) {
            return false;
        } else if (val2 == null) {
            return true;
        }

        for (int type: IFACE_TYPES_BY_PRIORITY) {
            // # of interfaces to be deleted: the list or all interfaces of the type if mode change
            int numIfacesToDelete1 = 0;
            if (val1.chipInfo.currentModeIdValid
                    && val1.chipInfo.currentModeId != val1.chipModeId) {
                numIfacesToDelete1 = val1.chipInfo.ifaces[type].length;
            } else {
                numIfacesToDelete1 = val1.interfacesToBeRemovedFirst.size();
            }

            int numIfacesToDelete2 = 0;
            if (val2.chipInfo.currentModeIdValid
                    && val2.chipInfo.currentModeId != val2.chipModeId) {
                numIfacesToDelete2 = val2.chipInfo.ifaces[type].length;
            } else {
                numIfacesToDelete2 = val2.interfacesToBeRemovedFirst.size();
            }

            if (numIfacesToDelete1 < numIfacesToDelete2) {
                if (VDBG) {
                    Log.d(TAG, "decision based on type=" + type + ": " + numIfacesToDelete1
                            + " < " + numIfacesToDelete2);
                }
                return true;
            }
        }

        // arbitrary - flip a coin
        if (VDBG) Log.d(TAG, "proposals identical - flip a coin");
        return false;
    }

    private static final int PRIORITY_PRIVILEGED = 0;
    private static final int PRIORITY_SYSTEM = 1;
    private static final int PRIORITY_FG_APP = 2;
    private static final int PRIORITY_FG_SERVICE = 3;
    private static final int PRIORITY_BG = 4;
    private static final int PRIORITY_INTERNAL = 5;
    // Keep these in sync with any additions/deletions to above buckets.
    private static final int PRIORITY_MIN = PRIORITY_PRIVILEGED;
    private static final int PRIORITY_MAX = PRIORITY_INTERNAL;
    @IntDef(prefix = { "PRIORITY_" }, value = {
            PRIORITY_PRIVILEGED,
            PRIORITY_SYSTEM,
            PRIORITY_FG_APP,
            PRIORITY_FG_SERVICE,
            PRIORITY_BG,
            PRIORITY_INTERNAL,
    })
    @Retention(RetentionPolicy.SOURCE)
    public @interface RequestorWsPriority {}

    /**
     * Returns integer priority level for the provided |ws| based on rules mentioned in
     * {@link #allowedToDeleteIfaceTypeForRequestedType(int, WorkSource, int, WifiIfaceInfo[][])}.
     */
    private static @RequestorWsPriority int getRequestorWsPriority(WorkSourceHelper ws) {
        if (ws.hasAnyPrivilegedAppRequest()) return PRIORITY_PRIVILEGED;
        if (ws.hasAnySystemAppRequest()) return PRIORITY_SYSTEM;
        if (ws.hasAnyForegroundAppRequest()) return PRIORITY_FG_APP;
        if (ws.hasAnyForegroundServiceRequest()) return PRIORITY_FG_SERVICE;
        if (ws.hasAnyInternalRequest()) return PRIORITY_INTERNAL;
        return PRIORITY_BG;
    }

    /**
     * Returns whether interface request from |newRequestorWsPriority| is allowed to delete an
     * interface request from |existingRequestorWsPriority|.
     *
     * Rule:
     *  - If |newRequestorWsPriority| < |existingRequestorWsPriority|, then YES.
     *  - If they are at the same priority level, then
     *      - If both are privileged and not for the same interface type, then YES.
     *      - Else, NO.
     */
    private static boolean allowedToDelete(
            int requestedIfaceType, @RequestorWsPriority int newRequestorWsPriority,
            int existingIfaceType, @RequestorWsPriority int existingRequestorWsPriority) {
        if (!SdkLevel.isAtLeastS()) {
            return allowedToDeleteForR(requestedIfaceType, existingIfaceType);
        }
        // If the new request is higher priority than existing priority, then the new requestor
        // wins. This is because at all other priority levels (except privileged), existing caller
        // wins if both the requests are at the same priority level.
        if (newRequestorWsPriority < existingRequestorWsPriority) {
            return true;
        }
        if (newRequestorWsPriority == existingRequestorWsPriority) {
            // If both the requests are same priority for the same iface type, the existing
            // requestor wins.
            if (requestedIfaceType == existingIfaceType) {
                return false;
            }
            // If both the requests are privileged, the new requestor wins.
            if (newRequestorWsPriority == PRIORITY_PRIVILEGED) {
                return true;
            }
        }
        return false;
    }

    /**
     * Returns true if we're allowed to delete the existing interface type for the requested
     * interface type.
     *
     * Rules - applies in order:
     *
     * General rules:
     * 1. No interface will be destroyed for a requested interface of the same type
     *
     * Type-specific rules (but note that the general rules are applied first):
     * 2. Request for AP or STA will destroy any other interface
     * 3. Request for P2P will destroy NAN-only
     * 4. Request for NAN will destroy P2P-only
     */
    private static boolean allowedToDeleteForR(int requestedIfaceType, int existingIfaceType) {
        // rule 1
        if (existingIfaceType == requestedIfaceType) {
            return false;
        }

        // rule 2
        if (requestedIfaceType == IfaceType.P2P) {
            return existingIfaceType == IfaceType.NAN;
        }

        // rule 3
        if (requestedIfaceType == IfaceType.NAN) {
            return existingIfaceType == IfaceType.P2P;
        }

        // rule 4, the requestIfaceType is either AP or STA
        return true;
    }

    /**
     * Returns true if we're allowed to delete the existing interface type for the requested
     * interface type.
     *
     * General rules:
     * 1. Requests for interfaces have the following priority which are based on corresponding
     * requesting  app's context. Priorities in decreasing order (i.e (i) has the highest priority,
     * (v) has the lowest priority).
     *  - (i) Requests from privileged apps (i.e settings, setup wizard, connectivity stack, etc)
     *  - (ii) Requests from system apps.
     *  - (iii) Requests from foreground apps.
     *  - (iv) Requests from foreground services.
     *  - (v) Requests from everything else (lumped together as "background").
     * Note: If there are more than 1 app requesting for a particular interface, then we consider
     * the priority of the highest priority app among them.
     * For ex: If there is a system app and a foreground requesting for NAN iface, then we use the
     * system app to determine the priority of the interface request.
     * 2. If there are 2 conflicting interface requests from apps with the same priority, then
     *    - (i) If both the apps are privileged and not for the same interface type, the new request
     *          wins (last caller wins).
     *    - (ii) Else, the existing request wins (first caller wins).
     * Note: Privileged apps are the ones that the user is directly interacting with, hence we use
     * last caller wins to decide among those, for all other apps we try to minimize disruption to
     * existing requests.
     * For ex: User turns on wifi, then hotspot on legacy devices which do not support STA + AP, we
     * want the last request from the user (i.e hotspot) to be honored.
     */
    private boolean allowedToDeleteIfaceTypeForRequestedType(
            int requestedIfaceType, WorkSource requestorWs, int existingIfaceType,
            WifiIfaceInfo[][] existingIfaces) {
        WorkSourceHelper newRequestorWsHelper = mWifiInjector.makeWsHelper(requestorWs);
        WifiIfaceInfo[] ifaceInfosForExistingIfaceType = existingIfaces[existingIfaceType];
        // No ifaces of the existing type, error!
        if (ifaceInfosForExistingIfaceType.length == 0) {
            Log.wtf(TAG, "allowedToDeleteIfaceTypeForRequestedType: Num existings ifaces is 0!");
            return false;
        }
        for (WifiIfaceInfo ifaceInfo : ifaceInfosForExistingIfaceType) {
            int newRequestorWsPriority = getRequestorWsPriority(newRequestorWsHelper);
            int existingRequestorWsPriority = getRequestorWsPriority(ifaceInfo.requestorWsHelper);
            if (allowedToDelete(
                    requestedIfaceType, newRequestorWsPriority, existingIfaceType,
                    existingRequestorWsPriority)) {
                if (mDbg) {
                    Log.d(TAG, "allowedToDeleteIfaceTypeForRequestedType: Allowed to delete "
                            + "requestedIfaceType=" + requestedIfaceType
                            + "existingIfaceType=" + existingIfaceType
                            + ", newRequestorWsPriority=" + newRequestorWsHelper
                            + ", existingRequestorWsPriority" + existingRequestorWsPriority);
                }
                return true;
            }
        }
        return false;
    }

    /**
     * Selects the interfaces to delete.
     *
     * Rule:
     *  - Select interfaces that are lower priority than the request priority.
     *  - If they are at the same priority level, then
     *      - If both are privileged and different iface type, then delete existing interfaces.
     *      - Else, not allowed to delete.
     *  - Delete ifaces based on the descending requestor priority
     *    (i.e bg app requests are deleted first, privileged app requests are deleted last)
     *  - If there are > 1 ifaces within the same priority group to delete, select them randomly.
     *
     * @param excessInterfaces Number of interfaces which need to be selected.
     * @param requestedIfaceType Requested iface type.
     * @param requestorWs Requestor worksource.
     * @param existingIfaceType Existing iface type.
     * @param interfaces Array of interfaces.
     */
    private List<WifiIfaceInfo> selectInterfacesToDelete(int excessInterfaces,
            int requestedIfaceType, WorkSource requestorWs, int existingIfaceType,
            WifiIfaceInfo[] interfaces) {
        if (VDBG) {
            Log.d(TAG, "selectInterfacesToDelete: excessInterfaces=" + excessInterfaces
                    + ", requestedIfaceType=" + requestedIfaceType
                    + ", requestorWs=" + requestorWs
                    + ", existingIfaceType=" + existingIfaceType
                    + ", interfaces=" + Arrays.toString(interfaces));
        }
        WorkSourceHelper newRequestorWsHelper = mWifiInjector.makeWsHelper(requestorWs);

        boolean lookupError = false;
        // Map of priority levels to ifaces to delete.
        Map<Integer, List<WifiIfaceInfo>> ifacesToDeleteMap = new HashMap<>();
        for (WifiIfaceInfo info : interfaces) {
            InterfaceCacheEntry cacheEntry;
            synchronized (mLock) {
                cacheEntry = mInterfaceInfoCache.get(Pair.create(info.name, getType(info.iface)));
            }
            if (cacheEntry == null) {
                Log.e(TAG,
                        "selectInterfacesToDelete: can't find cache entry with name=" + info.name);
                lookupError = true;
                break;
            }
            int newRequestorWsPriority = getRequestorWsPriority(newRequestorWsHelper);
            int existingRequestorWsPriority = getRequestorWsPriority(cacheEntry.requestorWsHelper);
            if (allowedToDelete(requestedIfaceType, newRequestorWsPriority, existingIfaceType,
                    existingRequestorWsPriority)) {
                ifacesToDeleteMap.computeIfAbsent(
                        existingRequestorWsPriority, v -> new ArrayList<>()).add(info);
            }
        }

        if (lookupError) {
            Log.e(TAG, "selectInterfacesToDelete: falling back to arbitrary selection");
            return Arrays.asList(Arrays.copyOf(interfaces, excessInterfaces));
        } else {
            int numIfacesToDelete = 0;
            List<WifiIfaceInfo> ifacesToDelete = new ArrayList<>(excessInterfaces);
            // Iterate from lowest priority to highest priority ifaces.
            for (int i = PRIORITY_MAX; i >= PRIORITY_MIN; i--) {
                List<WifiIfaceInfo> ifacesToDeleteListWithinPriority =
                        ifacesToDeleteMap.getOrDefault(i, new ArrayList<>());
                int numIfacesToDeleteWithinPriority =
                        Math.min(excessInterfaces - numIfacesToDelete,
                                ifacesToDeleteListWithinPriority.size());
                ifacesToDelete.addAll(
                        ifacesToDeleteListWithinPriority.subList(
                                0, numIfacesToDeleteWithinPriority));
                numIfacesToDelete += numIfacesToDeleteWithinPriority;
                if (numIfacesToDelete == excessInterfaces) {
                    break;
                }
            }
            return ifacesToDelete;
        }
    }

    /**
     * Checks whether the input chip-iface-combo can support the requested interface type.
     */
    private boolean canIfaceComboSupportRequestedIfaceCombo(
            int[] chipIfaceCombo, int[] requestedIfaceCombo) {
        if (VDBG) {
            Log.d(TAG, "canIfaceComboSupportRequest: chipIfaceCombo=" + chipIfaceCombo
                    + ", requestedIfaceCombo=" + requestedIfaceCombo);
        }
        for (int ifaceType : IFACE_TYPES_BY_PRIORITY) {
            if (chipIfaceCombo[ifaceType] < requestedIfaceCombo[ifaceType]) {
                if (VDBG) Log.d(TAG, "Requested type not supported by combo");
                return false;
            }
        }
        return true;
    }

    // Is it possible to create iface combo just looking at the device capabilities.
    private boolean isItPossibleToCreateIfaceCombo(WifiChipInfo[] chipInfos,
            long requiredChipCapabilities, int[] ifaceCombo) {
        if (VDBG) {
            Log.d(TAG, "isItPossibleToCreateIfaceCombo: chipInfos=" + Arrays.deepToString(chipInfos)
                    + ", ifaceType=" + ifaceCombo
                    + ", requiredChipCapabilities=" + requiredChipCapabilities);
        }

        for (WifiChipInfo chipInfo: chipInfos) {
            if (!isChipCapabilitiesSupported(chipInfo, requiredChipCapabilities)) continue;
            for (IWifiChip.ChipMode chipMode: chipInfo.availableModes) {
                for (IWifiChip.ChipIfaceCombination chipIfaceCombo
                        : chipMode.availableCombinations) {
                    int[][] expandedIfaceCombos = expandIfaceCombos(chipIfaceCombo);
                    if (VDBG) {
                        Log.d(TAG, chipIfaceCombo + " expands to "
                                + Arrays.deepToString(expandedIfaceCombos));
                    }

                    for (int[] expandedIfaceCombo: expandedIfaceCombos) {
                        if (canIfaceComboSupportRequestedIfaceCombo(
                                expandedIfaceCombo, ifaceCombo)) {
                            return true;
                        }
                    }
                }
            }
        }
        return false;
    }

    /**
     * Performs chip reconfiguration per the input:
     * - Removes the specified interfaces
     * - Reconfigures the chip to the new chip mode (if necessary)
     * - Creates the new interface
     *
     * Returns the newly created interface or a null on any error.
     */
    private IWifiIface executeChipReconfiguration(IfaceCreationData ifaceCreationData,
            @HdmIfaceTypeForCreation int createIfaceType) {
        if (mDbg) {
            Log.d(TAG, "executeChipReconfiguration: ifaceCreationData=" + ifaceCreationData
                    + ", createIfaceType=" + createIfaceType);
        }
        synchronized (mLock) {
            try {
                // is this a mode change?
                boolean isModeConfigNeeded = !ifaceCreationData.chipInfo.currentModeIdValid
                        || ifaceCreationData.chipInfo.currentModeId != ifaceCreationData.chipModeId;
                if (mDbg) Log.d(TAG, "isModeConfigNeeded=" + isModeConfigNeeded);

                // first delete interfaces/change modes
                if (isModeConfigNeeded) {
                    // remove all interfaces pre mode-change
                    // TODO: is this necessary? note that even if we don't want to explicitly
                    // remove the interfaces we do need to call the onDeleted callbacks - which
                    // this does
                    for (WifiIfaceInfo[] ifaceInfos: ifaceCreationData.chipInfo.ifaces) {
                        for (WifiIfaceInfo ifaceInfo: ifaceInfos) {
                            removeIfaceInternal(ifaceInfo.iface); // ignore return value
                        }
                    }

                    WifiStatus status = ifaceCreationData.chipInfo.chip.configureChip(
                            ifaceCreationData.chipModeId);
                    updateRttControllerOnModeChange();
                    if (status.code != WifiStatusCode.SUCCESS) {
                        Log.e(TAG, "executeChipReconfiguration: configureChip error: "
                                + statusString(status));
                        return null;
                    }
                } else {
                    // remove all interfaces on the delete list
                    for (WifiIfaceInfo ifaceInfo: ifaceCreationData.interfacesToBeRemovedFirst) {
                        removeIfaceInternal(ifaceInfo.iface); // ignore return value
                    }
                }

                // create new interface
                Mutable<WifiStatus> statusResp = new Mutable<>();
                Mutable<IWifiIface> ifaceResp = new Mutable<>();
                switch (createIfaceType) {
                    case HDM_CREATE_IFACE_STA:
                        ifaceCreationData.chipInfo.chip.createStaIface(
                                (WifiStatus status, IWifiStaIface iface) -> {
                                    statusResp.value = status;
                                    ifaceResp.value = iface;
                                });
                        break;
                    case HDM_CREATE_IFACE_AP_BRIDGE:
                        android.hardware.wifi.V1_5.IWifiChip chip15 =
                                getWifiChipForV1_5Mockable(ifaceCreationData.chipInfo.chip);
                        if (chip15 != null) {
                            chip15.createBridgedApIface(
                                    (WifiStatus status,
                                    android.hardware.wifi.V1_5.IWifiApIface iface) -> {
                                        statusResp.value = status;
                                        ifaceResp.value = iface;
                                    });
                        } else {
                            Log.e(TAG, "Hal doesn't support to create AP bridge mode");
                            statusResp.value.code = WifiStatusCode.ERROR_NOT_SUPPORTED;
                        }
                        break;
                    case HDM_CREATE_IFACE_AP:
                        ifaceCreationData.chipInfo.chip.createApIface(
                                (WifiStatus status, IWifiApIface iface) -> {
                                    statusResp.value = status;
                                    ifaceResp.value = iface;
                                });
                        break;
                    case HDM_CREATE_IFACE_P2P:
                        ifaceCreationData.chipInfo.chip.createP2pIface(
                                (WifiStatus status, IWifiP2pIface iface) -> {
                                    statusResp.value = status;
                                    ifaceResp.value = iface;
                                });
                        break;
                    case HDM_CREATE_IFACE_NAN:
                        ifaceCreationData.chipInfo.chip.createNanIface(
                                (WifiStatus status, IWifiNanIface iface) -> {
                                    statusResp.value = status;
                                    ifaceResp.value = iface;
                                });
                        break;
                }

                if (statusResp.value.code != WifiStatusCode.SUCCESS) {
                    Log.e(TAG, "executeChipReconfiguration: failed to create interface"
                            + " createIfaceType=" + createIfaceType + ": "
                            + statusString(statusResp.value));
                    return null;
                }

                return ifaceResp.value;
            } catch (RemoteException e) {
                Log.e(TAG, "executeChipReconfiguration exception: " + e);
                return null;
            }
        }
    }

    private boolean removeIfaceInternal(IWifiIface iface) {
        String name = getName(iface);
        int type = getType(iface);
        if (mDbg) Log.d(TAG, "removeIfaceInternal: iface(name)=" + name + ", type=" + type);

        if (type == -1) {
            Log.e(TAG, "removeIfaceInternal: can't get type -- iface(name)=" + name);
            return false;
        }

        synchronized (mLock) {
            if (mWifi == null) {
                Log.e(TAG, "removeIfaceInternal: null IWifi -- iface(name)=" + name);
                return false;
            }

            IWifiChip chip = getChip(iface);
            if (chip == null) {
                Log.e(TAG, "removeIfaceInternal: null IWifiChip -- iface(name)=" + name);
                return false;
            }

            if (name == null) {
                Log.e(TAG, "removeIfaceInternal: can't get name");
                return false;
            }

            WifiStatus status = null;
            try {
                switch (type) {
                    case IfaceType.STA:
                        status = chip.removeStaIface(name);
                        break;
                    case IfaceType.AP:
                        status = chip.removeApIface(name);
                        break;
                    case IfaceType.P2P:
                        status = chip.removeP2pIface(name);
                        break;
                    case IfaceType.NAN:
                        status = chip.removeNanIface(name);
                        break;
                    default:
                        Log.wtf(TAG, "removeIfaceInternal: invalid type=" + type);
                        return false;
                }
            } catch (RemoteException e) {
                Log.e(TAG, "IWifiChip.removeXxxIface exception: " + e);
            }

            // dispatch listeners no matter what status
            dispatchDestroyedListeners(name, type);

            if (status != null && status.code == WifiStatusCode.SUCCESS) {
                return true;
            } else {
                Log.e(TAG, "IWifiChip.removeXxxIface failed: " + statusString(status));
                return false;
            }
        }
    }

    // dispatch all destroyed listeners registered for the specified interface AND remove the
    // cache entry
    private void dispatchDestroyedListeners(String name, int type) {
        if (VDBG) Log.d(TAG, "dispatchDestroyedListeners: iface(name)=" + name);

        synchronized (mLock) {
            InterfaceCacheEntry entry = mInterfaceInfoCache.get(Pair.create(name, type));
            if (entry == null) {
                Log.e(TAG, "dispatchDestroyedListeners: no cache entry for iface(name)=" + name);
                return;
            }

            for (InterfaceDestroyedListenerProxy listener : entry.destroyedListeners) {
                listener.trigger();
            }
            entry.destroyedListeners.clear(); // for insurance (though cache entry is removed)
            mInterfaceInfoCache.remove(Pair.create(name, type));
        }
    }

    // dispatch all destroyed listeners registered to all interfaces
    private void dispatchAllDestroyedListeners() {
        if (VDBG) Log.d(TAG, "dispatchAllDestroyedListeners");

        List<InterfaceDestroyedListenerProxy> triggerList = new ArrayList<>();
        synchronized (mLock) {
            for (InterfaceCacheEntry cacheEntry: mInterfaceInfoCache.values()) {
                for (InterfaceDestroyedListenerProxy listener : cacheEntry.destroyedListeners) {
                    triggerList.add(listener);
                }
                cacheEntry.destroyedListeners.clear(); // for insurance
            }
            mInterfaceInfoCache.clear();
        }

        for (InterfaceDestroyedListenerProxy listener : triggerList) {
            listener.trigger();
        }
    }

    private abstract class ListenerProxy<LISTENER>  {
        protected LISTENER mListener;
        private Handler mHandler;

        // override equals & hash to make sure that the container HashSet is unique with respect to
        // the contained listener
        @Override
        public boolean equals(Object obj) {
            return mListener == ((ListenerProxy<LISTENER>) obj).mListener;
        }

        @Override
        public int hashCode() {
            return mListener.hashCode();
        }

        void trigger() {
            if (mHandler != null) {
                mHandler.post(() -> {
                    action();
                });
            } else {
                action();
            }
        }

        void triggerWithArg(boolean arg) {
            if (mHandler != null) {
                mHandler.post(() -> {
                    actionWithArg(arg);
                });
            } else {
                actionWithArg(arg);
            }
        }

        protected void action() {}
        protected void actionWithArg(boolean arg) {}

        ListenerProxy(LISTENER listener, Handler handler, String tag) {
            mListener = listener;
            mHandler = handler;
        }
    }

    private class SubsystemRestartListenerProxy extends
            ListenerProxy<SubsystemRestartListener> {
        SubsystemRestartListenerProxy(@NonNull SubsystemRestartListener subsystemRestartListener,
                                        Handler handler) {
            super(subsystemRestartListener, handler, "SubsystemRestartListenerProxy");
        }

        @Override
        protected void action() {
            mListener.onSubsystemRestart();
        }
    }

    private class InterfaceDestroyedListenerProxy extends
            ListenerProxy<InterfaceDestroyedListener> {
        private final String mIfaceName;
        InterfaceDestroyedListenerProxy(@NonNull String ifaceName,
                                        InterfaceDestroyedListener destroyedListener,
                                        Handler handler) {
            super(destroyedListener, handler, "InterfaceDestroyedListenerProxy");
            mIfaceName = ifaceName;
        }

        @Override
        protected void action() {
            mListener.onDestroyed(mIfaceName);
        }
    }

    private class InterfaceRttControllerLifecycleCallbackProxy implements
            InterfaceRttControllerLifecycleCallback {
        private InterfaceRttControllerLifecycleCallback mCallback;
        private Handler mHandler;

        InterfaceRttControllerLifecycleCallbackProxy(
                InterfaceRttControllerLifecycleCallback callback, Handler handler) {
            mCallback = callback;
            mHandler = handler;
        }

        // override equals & hash to make sure that the container HashSet is unique with respect to
        // the contained listener
        @Override
        public boolean equals(Object obj) {
            return mCallback == ((InterfaceRttControllerLifecycleCallbackProxy) obj).mCallback;
        }

        @Override
        public int hashCode() {
            return mCallback.hashCode();
        }

        @Override
        public void onNewRttController(IWifiRttController controller) {
            mHandler.post(() -> mCallback.onNewRttController(controller));
        }

        @Override
        public void onRttControllerDestroyed() {
            mHandler.post(() -> mCallback.onRttControllerDestroyed());
        }
    }

    private void dispatchRttControllerLifecycleOnNew() {
        if (VDBG) {
            Log.v(TAG, "dispatchRttControllerLifecycleOnNew: # cbs="
                    + mRttControllerLifecycleCallbacks.size());
        }
        for (InterfaceRttControllerLifecycleCallbackProxy cbp : mRttControllerLifecycleCallbacks) {
            cbp.onNewRttController(mIWifiRttController);
        }
    }

    private void dispatchRttControllerLifecycleOnDestroyed() {
        for (InterfaceRttControllerLifecycleCallbackProxy cbp : mRttControllerLifecycleCallbacks) {
            cbp.onRttControllerDestroyed();
        }
    }


    /**
     * Updates the RttController when the chip mode is changed:
     * - Handles callbacks to registered listeners
     * - Handles creation of new RttController
     */
    private void updateRttControllerOnModeChange() {
        synchronized (mLock) {
            boolean controllerDestroyed = mIWifiRttController != null;
            mIWifiRttController = null;
            if (mRttControllerLifecycleCallbacks.size() == 0) {
                Log.d(TAG, "updateRttController: no one is interested in RTT controllers");
                return;
            }

            IWifiRttController newRttController = createRttControllerIfPossible();
            if (newRttController == null) {
                if (controllerDestroyed) {
                    dispatchRttControllerLifecycleOnDestroyed();
                }
            } else {
                mIWifiRttController = newRttController;
                dispatchRttControllerLifecycleOnNew();
            }
        }
    }

    /**
     * Try to create a new RttController.
     *
     * @return The new RttController - or null on failure.
     */
    private IWifiRttController createRttControllerIfPossible() {
        synchronized (mLock) {
            if (!isWifiStarted()) {
                Log.d(TAG, "createRttControllerIfPossible: Wifi is not started");
                return null;
            }

            WifiChipInfo[] chipInfos = getAllChipInfo();
            if (chipInfos == null) {
                Log.d(TAG, "createRttControllerIfPossible: no chip info found - most likely chip "
                        + "not up yet");
                return null;
            }

            for (WifiChipInfo chipInfo : chipInfos) {
                if (!chipInfo.currentModeIdValid) {
                    if (VDBG) {
                        Log.d(TAG, "createRttControllerIfPossible: chip not configured yet: "
                                + chipInfo);
                    }
                    continue;
                }

                Mutable<IWifiRttController> rttResp = new Mutable<>();
                try {
                    android.hardware.wifi.V1_4.IWifiChip chip14 =
                            android.hardware.wifi.V1_4.IWifiChip.castFrom(chipInfo.chip);
                    if (chip14 != null) {
                        chip14.createRttController_1_4(null,
                                (WifiStatus status,
                                 android.hardware.wifi.V1_4.IWifiRttController rtt) -> {
                                    if (status.code == WifiStatusCode.SUCCESS) {
                                        rttResp.value = rtt;
                                    } else {
                                        Log.e(TAG, "IWifiChip.createRttController_1_4 failed: "
                                                + statusString(status));
                                    }
                                });
                    } else {
                        chipInfo.chip.createRttController(null,
                                (WifiStatus status, IWifiRttController rtt) -> {
                                    if (status.code == WifiStatusCode.SUCCESS) {
                                        rttResp.value = rtt;
                                    } else {
                                        Log.e(TAG, "IWifiChip.createRttController failed: "
                                                + statusString(status));
                                    }
                                });
                    }
                } catch (RemoteException e) {
                    Log.e(TAG, "IWifiChip.createRttController exception: " + e);
                }
                if (rttResp.value != null) {
                    return rttResp.value;
                }
            }
        }

        Log.w(TAG, "createRttControllerIfPossible: not available from any of the chips");
        return null;
    }

    // general utilities

    private static String statusString(WifiStatus status) {
        if (status == null) {
            return "status=null";
        }
        StringBuilder sb = new StringBuilder();
        sb.append(status.code).append(" (").append(status.description).append(")");
        return sb.toString();
    }

    // Will return -1 for invalid results! Otherwise will return one of the 4 valid values.
    private static int getType(IWifiIface iface) {
        Mutable<Integer> typeResp = new Mutable<>(-1);
        try {
            iface.getType((WifiStatus status, int type) -> {
                if (status.code == WifiStatusCode.SUCCESS) {
                    typeResp.value = type;
                } else {
                    Log.e(TAG, "Error on getType: " + statusString(status));
                }
            });
        } catch (RemoteException e) {
            Log.e(TAG, "Exception on getType: " + e);
        }

        return typeResp.value;
    }

    /**
     * Checks for a successful status result.
     *
     * Failures are logged to mLog.
     *
     * @param status is the WifiStatus generated by a hal call
     * @return true for success, false for failure
     */
    private boolean ok(String method, WifiStatus status) {
        if (status.code == WifiStatusCode.SUCCESS) return true;

        Log.e(TAG, "Error on " + method + ": " + statusString(status));
        return false;
    }

    /**
     * Get the chip capabilities
     *
     * This is called before creating an interface and needs at least v1.5 HAL.
     *
     * @param wifiChip WifiChip
     * @return bitmask defined by HAL interface
     */
    public long getChipCapabilities(@NonNull IWifiChip wifiChip) {
        long featureSet = 0;
        if (wifiChip == null) return featureSet;

        try {
            final Mutable<Long> feat = new Mutable<>(CHIP_CAPABILITY_UNINITIALIZED);
            synchronized (mLock) {
                android.hardware.wifi.V1_5.IWifiChip iWifiChipV15 =
                        getWifiChipForV1_5Mockable(wifiChip);
                // HAL newer than v1.5 support getting capabilities before creating an interface.
                if (iWifiChipV15 != null) {
                    iWifiChipV15.getCapabilities_1_5((status, capabilities) -> {
                        if (!ok("getCapabilities_1_5", status)) return;
                        feat.value = (long) capabilities;
                    });
                }
            }
            featureSet = feat.value;
        } catch (RemoteException e) {
            Log.e(TAG, "Exception on getCapabilities: " + e);
            return 0;
        }
        return featureSet;
    }

    /**
     * Dump the internal state of the class.
     */
    public void dump(FileDescriptor fd, PrintWriter pw, String[] args) {
        pw.println("Dump of HalDeviceManager:");
        pw.println("  mServiceManager: " + mServiceManager);
        pw.println("  mWifi: " + mWifi);
        pw.println("  mManagerStatusListeners: " + mManagerStatusListeners);
        pw.println("  mInterfaceInfoCache: " + mInterfaceInfoCache);
        pw.println("  mDebugChipsInfo: " + Arrays.toString(getAllChipInfo()));
    }
}
