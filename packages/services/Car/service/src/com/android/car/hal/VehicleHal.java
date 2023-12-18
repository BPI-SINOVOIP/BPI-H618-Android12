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

package com.android.car.hal;

import static com.android.car.CarServiceUtils.toByteArray;
import static com.android.car.CarServiceUtils.toFloatArray;
import static com.android.car.CarServiceUtils.toIntArray;
import static com.android.car.internal.ExcludeFromCodeCoverageGeneratedReport.DUMP_INFO;

import static java.lang.Integer.toHexString;

import android.annotation.CheckResult;
import android.annotation.Nullable;
import android.car.VehiclePropertyIds;
import android.car.hardware.property.CarPropertyManager;
import android.content.Context;
import android.hardware.automotive.vehicle.V2_0.IVehicle;
import android.hardware.automotive.vehicle.V2_0.IVehicleCallback;
import android.hardware.automotive.vehicle.V2_0.SubscribeFlags;
import android.hardware.automotive.vehicle.V2_0.SubscribeOptions;
import android.hardware.automotive.vehicle.V2_0.VehicleAreaConfig;
import android.hardware.automotive.vehicle.V2_0.VehiclePropConfig;
import android.hardware.automotive.vehicle.V2_0.VehiclePropValue;
import android.hardware.automotive.vehicle.V2_0.VehicleProperty;
import android.hardware.automotive.vehicle.V2_0.VehiclePropertyAccess;
import android.hardware.automotive.vehicle.V2_0.VehiclePropertyChangeMode;
import android.hardware.automotive.vehicle.V2_0.VehiclePropertyType;
import android.os.HandlerThread;
import android.os.RemoteException;
import android.os.ServiceSpecificException;
import android.os.SystemClock;
import android.util.ArraySet;
import android.util.Slog;
import android.util.SparseArray;

import com.android.car.CarLog;
import com.android.car.CarServiceUtils;
import com.android.car.internal.ExcludeFromCodeCoverageGeneratedReport;
import com.android.internal.annotations.GuardedBy;
import com.android.internal.annotations.VisibleForTesting;

import com.google.android.collect.Lists;

import java.io.PrintWriter;
import java.lang.ref.WeakReference;
import java.util.ArrayList;
import java.util.Arrays;
import java.util.Collection;
import java.util.HashMap;
import java.util.List;
import java.util.Map;
import java.util.Timer;
import java.util.TimerTask;
import java.util.concurrent.TimeUnit;
import java.util.stream.Collectors;

/**
 * Abstraction for vehicle HAL. This class handles interface with native HAL and do basic parsing
 * of received data (type check). Then each event is sent to corresponding {@link HalServiceBase}
 * implementation. It is responsibility of {@link HalServiceBase} to convert data to corresponding
 * Car*Service for Car*Manager API.
 */
public class VehicleHal extends IVehicleCallback.Stub {

    private static final boolean DBG = false;

    /**
     * Used in {@link VehicleHal#dumpVehiclePropValue} method when copying {@link VehiclePropValue}.
     */
    private static final int MAX_BYTE_SIZE = 20;

    public static final int NO_AREA = -1;
    public static final float NO_SAMPLE_RATE = -1;

    private final HandlerThread mHandlerThread = CarServiceUtils.getHandlerThread(
            VehicleHal.class.getSimpleName());
    private final PowerHalService mPowerHal;
    private final PropertyHalService mPropertyHal;
    private final InputHalService mInputHal;
    private final VmsHalService mVmsHal;
    private final UserHalService mUserHal;
    private final DiagnosticHalService mDiagnosticHal;
    private final ClusterHalService mClusterHalService;
    private final EvsHalService mEvsHal;

    private final Object mLock = new Object();

    /** Might be re-assigned if Vehicle HAL is reconnected. */
    private volatile HalClient mHalClient;

    /** Stores handler for each HAL property. Property events are sent to handler. */
    @GuardedBy("mLock")
    private final SparseArray<HalServiceBase> mPropertyHandlers = new SparseArray<>();
    /** This is for iterating all HalServices with fixed order. */
    @GuardedBy("mLock")
    private final ArrayList<HalServiceBase> mAllServices = new ArrayList<>();
    @GuardedBy("mLock")
    private final HashMap<Integer, SubscribeOptions> mSubscribedProperties = new HashMap<>();
    @GuardedBy("mLock")
    private final HashMap<Integer, VehiclePropConfig> mAllProperties = new HashMap<>();

    @GuardedBy("mLock")
    private final HashMap<Integer, VehiclePropertyEventInfo> mEventLog = new HashMap<>();

    // Used by injectVHALEvent for testing purposes.  Delimiter for an array of data
    private static final String DATA_DELIMITER = ",";

    /**
     * Constructs a new {@link VehicleHal} object given the {@link Context} and {@link IVehicle}
     * both passed as parameters.
     */
    public VehicleHal(Context context, IVehicle vehicle) {
        mPowerHal = new PowerHalService(this);
        mPropertyHal = new PropertyHalService(this);
        mInputHal = new InputHalService(this);
        mVmsHal = new VmsHalService(context, this);
        mUserHal = new UserHalService(this);
        mDiagnosticHal = new DiagnosticHalService(this);
        mClusterHalService = new ClusterHalService(this);
        mEvsHal = new EvsHalService(this);
        mAllServices.addAll(Arrays.asList(mPowerHal,
                mInputHal,
                mDiagnosticHal,
                mVmsHal,
                mUserHal,
                mClusterHalService,
                mEvsHal,
                mPropertyHal)); // mPropertyHal should be the last.
        mHalClient = new HalClient(vehicle, mHandlerThread.getLooper(),
                /* callback= */ this);
    }

    /**
     * Constructs a new {@link VehicleHal} object given the services and {@link HalClient} factory
     * function passed as parameters. This method must be used by tests only.
     */
    @VisibleForTesting
    VehicleHal(PowerHalService powerHal,
            PropertyHalService propertyHal,
            InputHalService inputHal,
            VmsHalService vmsHal,
            UserHalService userHal,
            DiagnosticHalService diagnosticHal,
            ClusterHalService clusterHalService,
            HalClient halClient) {
        mPowerHal = powerHal;
        mPropertyHal = propertyHal;
        mInputHal = inputHal;
        mVmsHal = vmsHal;
        mUserHal = userHal;
        mDiagnosticHal = diagnosticHal;
        mClusterHalService = clusterHalService;
        mEvsHal = new EvsHalService(this);
        mAllServices.addAll(Arrays.asList(mPowerHal,
                mInputHal,
                mDiagnosticHal,
                mVmsHal,
                mUserHal,
                mEvsHal,
                mPropertyHal));
        mHalClient = halClient;
    }

    /** Called when connection to Vehicle HAL was restored. */
    public void vehicleHalReconnected(IVehicle vehicle) {
        synchronized (mLock) {
            mHalClient = new HalClient(vehicle, mHandlerThread.getLooper(),
                    this /*IVehicleCallback*/);
            SubscribeOptions[] options = mSubscribedProperties.values()
                    .toArray(new SubscribeOptions[0]);
            try {
                mHalClient.subscribe(options);
            } catch (RemoteException e) {
                throw new RuntimeException("Failed to subscribe: " + Arrays.asList(options), e);
            }
        }
    }

    private void fetchAllPropConfigs() {
        synchronized (mLock) {
            if (!mAllProperties.isEmpty()) { // already set
                Slog.i(CarLog.TAG_HAL, "fetchAllPropConfigs already fetched");
                return;
            }
        }
        ArrayList<VehiclePropConfig> configs;
        try {
            configs = mHalClient.getAllPropConfigs();
            if (configs == null || configs.size() == 0) {
                Slog.e(CarLog.TAG_HAL, "getAllPropConfigs returned empty configs");
                return;
            }
        } catch (RemoteException e) {
            throw new RuntimeException("Unable to retrieve vehicle property configuration", e);
        }

        synchronized (mLock) {
            // Create map of all properties
            for (VehiclePropConfig p : configs) {
                if (DBG) {
                    Slog.i(CarLog.TAG_HAL, "Add config for prop:" + Integer.toHexString(p.prop)
                            + " config:" + p);
                }
                mAllProperties.put(p.prop, p);
            }
        }
    }

    /**
     * Inits the vhal configurations.
     *
     * <p><Note that {@link #getIfAvailableOrFailForEarlyStage(int, int)}
     * can be called before {@code init()}.
     */
    public void init() {
        fetchAllPropConfigs();

        // PropertyHalService will take most properties, so make it big enough.
        ArrayList<VehiclePropConfig> configsForService = new ArrayList<>(mAllServices.size());
        for (int i = 0; i < mAllServices.size(); i++) {
            HalServiceBase service = mAllServices.get(i);
            int[] supportedProps =  service.getAllSupportedProperties();
            configsForService.clear();
            synchronized (mLock) {
                if (supportedProps.length == 0) {
                    for (Integer propId : mAllProperties.keySet()) {
                        if (service.isSupportedProperty(propId)) {
                            VehiclePropConfig config = mAllProperties.get(propId);
                            mPropertyHandlers.append(propId, service);
                            configsForService.add(config);
                        }
                    }
                } else {
                    for (int prop : supportedProps) {
                        VehiclePropConfig config = mAllProperties.get(prop);
                        if (config == null) {
                            continue;
                        }
                        mPropertyHandlers.append(prop, service);
                        configsForService.add(config);
                    }
                }
            }
            service.takeProperties(configsForService);
            service.init();
        }
    }

    /**
     * Releases all connected services (power management service, input service, etc).
     */
    public void release() {
        // release in reverse order from init
        for (int i = mAllServices.size() - 1; i >= 0; i--) {
            mAllServices.get(i).release();
        }
        synchronized (mLock) {
            for (int p : mSubscribedProperties.keySet()) {
                try {
                    mHalClient.unsubscribe(p);
                } catch (RemoteException e) {
                    //  Ignore exceptions on shutdown path.
                    Slog.w(CarLog.TAG_HAL, "Failed to unsubscribe", e);
                }
            }
            mSubscribedProperties.clear();
            mAllProperties.clear();
        }
        // keep the looper thread as should be kept for the whole life cycle.
    }

    public DiagnosticHalService getDiagnosticHal() {
        return mDiagnosticHal;
    }

    public PowerHalService getPowerHal() {
        return mPowerHal;
    }

    public PropertyHalService getPropertyHal() {
        return mPropertyHal;
    }

    public InputHalService getInputHal() {
        return mInputHal;
    }

    public UserHalService getUserHal() {
        return mUserHal;
    }

    public VmsHalService getVmsHal() {
        return mVmsHal;
    }

    public ClusterHalService getClusterHal() {
        return mClusterHalService;
    }

    public EvsHalService getEvsHal() {
        return mEvsHal;
    }

    private void assertServiceOwnerLocked(HalServiceBase service, int property) {
        if (service != mPropertyHandlers.get(property)) {
            throw new IllegalArgumentException("Property 0x" + toHexString(property)
                    + " is not owned by service: " + service);
        }
    }

    /**
     * Subscribes given properties with sampling rate defaults to 0 and no special flags provided.
     *
     * @see #subscribeProperty(HalServiceBase, int, float, int)
     */
    public void subscribeProperty(HalServiceBase service, int property)
            throws IllegalArgumentException {
        subscribeProperty(service, property, /* samplingRateHz= */ 0f,
                SubscribeFlags.EVENTS_FROM_CAR);
    }

    /**
     * Subscribes given properties with default subscribe flag.
     *
     * @see #subscribeProperty(HalServiceBase, int, float, int)
     */
    public void subscribeProperty(HalServiceBase service, int property, float sampleRateHz)
            throws IllegalArgumentException {
        subscribeProperty(service, property, sampleRateHz, SubscribeFlags.EVENTS_FROM_CAR);
    }

    /**
     * Subscribe given property. Only Hal service owning the property can subscribe it.
     *
     * @param service HalService that owns this property
     * @param property property id (VehicleProperty)
     * @param samplingRateHz sampling rate in Hz for continuous properties
     * @param flags flags from {@link android.hardware.automotive.vehicle.V2_0.SubscribeFlags}
     * @throws IllegalArgumentException thrown if property is not supported by VHAL
     */
    public void subscribeProperty(HalServiceBase service, int property,
            float samplingRateHz, int flags) throws IllegalArgumentException {
        if (DBG) {
            Slog.i(CarLog.TAG_HAL, "subscribeProperty, service:" + service
                    + ", " + toCarPropertyLog(property));
        }
        VehiclePropConfig config;
        synchronized (mLock) {
            config = mAllProperties.get(property);
        }

        if (config == null) {
            throw new IllegalArgumentException("subscribe error: config is null for property 0x"
                    + toHexString(property));
        } else if (isPropertySubscribable(config)) {
            SubscribeOptions opts = new SubscribeOptions();
            opts.propId = property;
            opts.sampleRate = samplingRateHz;
            opts.flags = flags;
            synchronized (mLock) {
                assertServiceOwnerLocked(service, property);
                mSubscribedProperties.put(property, opts);
            }
            try {
                mHalClient.subscribe(opts);
            } catch (RemoteException e) {
                Slog.e(CarLog.TAG_HAL, "Failed to subscribe to " + toCarPropertyLog(property),
                        e);
            }
        } else {
            Slog.e(CarLog.TAG_HAL, "Cannot subscribe to " + toCarPropertyLog(property));
        }
    }

    /**
     * Unsubscribes from receiving notifications for the property and HAL services passed
     * as parameters.
     */
    public void unsubscribeProperty(HalServiceBase service, int property) {
        if (DBG) {
            Slog.i(CarLog.TAG_HAL, "unsubscribeProperty, service:" + service
                    + ", " + toCarPropertyLog(property));
        }
        VehiclePropConfig config;
        synchronized (mLock) {
            config = mAllProperties.get(property);
        }

        if (config == null) {
            Slog.e(CarLog.TAG_HAL, "unsubscribeProperty " + toCarPropertyLog(property)
                    + " does not exist");
        } else if (isPropertySubscribable(config)) {
            synchronized (mLock) {
                assertServiceOwnerLocked(service, property);
                mSubscribedProperties.remove(property);
            }
            try {
                mHalClient.unsubscribe(property);
            } catch (RemoteException e) {
                Slog.e(CarLog.TAG_SERVICE, "Failed to unsubscribe: "
                        + toCarPropertyLog(property), e);
            }
        } else {
            Slog.e(CarLog.TAG_HAL, "Cannot unsubscribe " + toCarPropertyLog(property));
        }
    }

    /**
     * Indicates if the property passed as parameter is supported.
     */
    public boolean isPropertySupported(int propertyId) {
        synchronized (mLock) {
            return mAllProperties.containsKey(propertyId);
        }
    }

    /**
     * Gets given property with retries.
     *
     * <p>If getting the property fails after all retries, it will throw
     * {@code IllegalStateException}. If the property does not exist, it will simply return
     * {@code null}.
     */
    public @Nullable VehiclePropValue getIfAvailableOrFail(int propertyId, int numberOfRetries) {
        if (!isPropertySupported(propertyId)) {
            return null;
        }
        VehiclePropValue value;
        for (int i = 0; i < numberOfRetries; i++) {
            try {
                return get(propertyId);
            } catch (ServiceSpecificException e) {
                Slog.e(CarLog.TAG_HAL, "Cannot get " + toCarPropertyLog(propertyId), e);
            }
        }
        throw new IllegalStateException("Cannot get property: 0x" + toHexString(propertyId)
                + " after " + numberOfRetries + " retries");
    }

    /**
     * This works similar to {@link #getIfAvailableOrFail(int, int)} except that this can be called
     * before {@code init()} is called.
     *
     * <p>This call will check if requested vhal property is supported by querying directly to vhal
     * and can have worse performance. Use this only for accessing vhal properties before
     * {@code ICarImpl.init()} phase.
     */
    public @Nullable VehiclePropValue getIfAvailableOrFailForEarlyStage(int propertyId,
            int numberOfRetries) {
        fetchAllPropConfigs();
        return getIfAvailableOrFail(propertyId, numberOfRetries);
    }

    /**
     * Returns the property's {@link VehiclePropValue} for the property id passed as parameter and
     * not specified area.
     */
    public VehiclePropValue get(int propertyId) {
        return get(propertyId, NO_AREA);
    }

    /**
     * Returns the property's {@link VehiclePropValue} for the property id and area id passed as
     * parameters.
     */
    public VehiclePropValue get(int propertyId, int areaId) {
        if (DBG) {
            Slog.i(CarLog.TAG_HAL, "get, " + toCarPropertyLog(propertyId)
                    + toCarAreaLog(areaId));
        }
        return mHalClient.getValue(createPropValue(propertyId, areaId));
    }

    /**
     * Returns the property object value for the class and property id passed as parameter and
     * no area specified.
     */
    public <T> T get(Class clazz, int propertyId) {
        return get(clazz, createPropValue(propertyId, NO_AREA));
    }

    /**
     * Returns the property object value for the class, property id, and area id passed as
     * parameter.
     */
    public <T> T get(Class clazz, int propertyId, int areaId) {
        return get(clazz, createPropValue(propertyId, areaId));
    }

    /**
     * Returns the property object value for the class and requested property value passed as
     * parameter.
     */
    @SuppressWarnings("unchecked")
    public <T> T get(Class clazz, VehiclePropValue requestedPropValue) {
        VehiclePropValue propValue;
        propValue = mHalClient.getValue(requestedPropValue);

        if (clazz == Integer.class || clazz == int.class) {
            return (T) propValue.value.int32Values.get(0);
        } else if (clazz == Boolean.class || clazz == boolean.class) {
            return (T) Boolean.valueOf(propValue.value.int32Values.get(0) == 1);
        } else if (clazz == Float.class || clazz == float.class) {
            return (T) propValue.value.floatValues.get(0);
        } else if (clazz == Integer[].class) {
            Integer[] intArray = new Integer[propValue.value.int32Values.size()];
            return (T) propValue.value.int32Values.toArray(intArray);
        } else if (clazz == Float[].class) {
            Float[] floatArray = new Float[propValue.value.floatValues.size()];
            return (T) propValue.value.floatValues.toArray(floatArray);
        } else if (clazz == int[].class) {
            return (T) toIntArray(propValue.value.int32Values);
        } else if (clazz == float[].class) {
            return (T) toFloatArray(propValue.value.floatValues);
        } else if (clazz == byte[].class) {
            return (T) toByteArray(propValue.value.bytes);
        } else if (clazz == String.class) {
            return (T) propValue.value.stringValue;
        } else {
            throw new IllegalArgumentException("Unexpected type: " + clazz);
        }
    }

    /**
     * Returns the vehicle's {@link VehiclePropValue} for the requested property value passed
     * as parameter.
     */
    public VehiclePropValue get(VehiclePropValue requestedPropValue) {
        return mHalClient.getValue(requestedPropValue);
    }

    /**
     * Returns the sample rate for a subscribed property. Returns {@link VehicleHal#NO_SAMPLE_RATE}
     * if the property id passed as parameter is not linked to any subscribed property.
     */
    public float getSampleRate(int propId) {
        SubscribeOptions opts = mSubscribedProperties.get(propId);
        if (opts == null) {
            // No sample rate for this property
            return NO_SAMPLE_RATE;
        } else {
            return opts.sampleRate;
        }
    }

    protected void set(VehiclePropValue propValue) {
        mHalClient.setValue(propValue);
    }

    @CheckResult
    VehiclePropValueSetter set(int propId) {
        return new VehiclePropValueSetter(mHalClient, propId, NO_AREA);
    }

    @CheckResult
    VehiclePropValueSetter set(int propId, int areaId) {
        return new VehiclePropValueSetter(mHalClient, propId, areaId);
    }

    static boolean isPropertySubscribable(VehiclePropConfig config) {
        if ((config.access & VehiclePropertyAccess.READ) == 0
                || (config.changeMode == VehiclePropertyChangeMode.STATIC)) {
            return false;
        }
        return true;
    }

    private final ArraySet<HalServiceBase> mServicesToDispatch = new ArraySet<>();

    @Override
    public void onPropertyEvent(ArrayList<VehiclePropValue> propValues) {
        synchronized (mLock) {
            for (VehiclePropValue v : propValues) {
                HalServiceBase service = mPropertyHandlers.get(v.prop);
                if (service == null) {
                    Slog.e(CarLog.TAG_HAL, "HalService not found for prop: 0x"
                            + toHexString(v.prop));
                    continue;
                }
                service.getDispatchList().add(v);
                mServicesToDispatch.add(service);
                VehiclePropertyEventInfo info = mEventLog.get(v.prop);
                if (info == null) {
                    info = new VehiclePropertyEventInfo(v);
                    mEventLog.put(v.prop, info);
                } else {
                    info.addNewEvent(v);
                }
            }
        }
        for (HalServiceBase s : mServicesToDispatch) {
            s.onHalEvents(s.getDispatchList());
            s.getDispatchList().clear();
        }
        mServicesToDispatch.clear();
    }

    @Override
    public void onPropertySet(VehiclePropValue value) {
        // No need to handle on-property-set events in HAL service yet.
    }

    @Override
    public void onPropertySetError(@CarPropertyManager.CarSetPropertyErrorCode int errorCode,
            int propId, int areaId) {
        Slog.e(CarLog.TAG_HAL, String.format("onPropertySetError, errorCode: %d, prop: 0x%x, "
                + "area: 0x%x", errorCode, propId, areaId));
        if (propId != VehicleProperty.INVALID) {
            HalServiceBase service = mPropertyHandlers.get(propId);
            if (service != null) {
                service.onPropertySetError(propId, areaId, errorCode);
            }
        }
    }

    /**
     * Dumps HAL service info using the print writer passed as parameter.
     */
    @ExcludeFromCodeCoverageGeneratedReport(reason = DUMP_INFO)
    public void dump(PrintWriter writer) {
        writer.println("**dump HAL services**");
        for (HalServiceBase service: mAllServices) {
            service.dump(writer);
        }
        // Dump all VHAL property configure.
        dumpPropertyConfigs(writer, -1);
        writer.printf("**All Events, now ns:%d**\n",
                SystemClock.elapsedRealtimeNanos());
        for (VehiclePropertyEventInfo info : mEventLog.values()) {
            writer.printf("event count:%d, lastEvent: ", info.mEventCount);
            dumpVehiclePropValue(writer, info.mLastEvent);
        }

        writer.println("**Property handlers**");
        for (int i = 0; i < mPropertyHandlers.size(); i++) {
            int propId = mPropertyHandlers.keyAt(i);
            HalServiceBase service = mPropertyHandlers.valueAt(i);
            writer.printf("Property Id: %d // 0x%x name: %s, service: %s\n", propId, propId,
                    VehiclePropertyIds.toString(propId), service);
        }
    }

    /**
     * Dumps the list of HALs.
     */
    public void dumpListHals(PrintWriter writer) {
        for (HalServiceBase service: mAllServices) {
            writer.println(service.getClass().getName());
        }
    }

    /**
     * Dumps the given HALs.
     */
    public void dumpSpecificHals(PrintWriter writer, String... halNames) {
        Map<String, HalServiceBase> byName = mAllServices.stream()
                .collect(Collectors.toMap(s -> s.getClass().getSimpleName(), s -> s));
        for (String halName : halNames) {
            HalServiceBase service = byName.get(halName);
            if (service == null) {
                writer.printf("No HAL named %s. Valid options are: %s\n", halName, byName.keySet());
                continue;
            }
            service.dump(writer);
        }
    }

    /**
     * Dumps vehicle property values.
     * @param writer
     * @param propId property id, dump all properties' value if it is empty string.
     * @param areaId areaId of the property, dump the property for all areaIds in the config
     * if it is empty string.
     */
    public void dumpPropertyValueByCommend(PrintWriter writer, int propId, int areaId) {
        if (propId == -1) {
            writer.println("**All property values**");
            for (VehiclePropConfig config : mAllProperties.values()) {
                dumpPropertyValueByConfig(writer, config);
            }
        } else if (areaId == -1) {
            VehiclePropConfig config = mAllProperties.get(propId);
            if (config == null) {
                writer.print("Property ");
                dumpPropHelper(writer, propId);
                writer.print(" not supported by HAL\n");
                return;
            }
            dumpPropertyValueByConfig(writer, config);
        } else {
            try {
                VehiclePropValue value = get(propId, areaId);
                dumpVehiclePropValue(writer, value);
            } catch (RuntimeException e) {
                writer.printf("Can not get property value for property: %d // 0x%x "
                                + "in areaId: %d // 0x%x.\n", propId, propId, areaId, areaId);
            }
        }
    }

    private static void dumpPropHelper(PrintWriter pw, int propId) {
        pw.printf("Id: %d // 0x%x, name: %s ", propId, propId, VehiclePropertyIds.toString(propId));
    }

    private void dumpPropertyValueByConfig(PrintWriter writer, VehiclePropConfig config) {
        if (config.areaConfigs.isEmpty()) {
            try {
                VehiclePropValue value = get(config.prop);
                dumpVehiclePropValue(writer, value);
            } catch (RuntimeException e) {
                writer.printf("Can not get property value for property: %d // 0x%x,"
                                + " areaId: 0 \n", config.prop, config.prop);
            }
        } else {
            for (VehicleAreaConfig areaConfig : config.areaConfigs) {
                int area = areaConfig.areaId;
                try {
                    VehiclePropValue value = get(config.prop, area);
                    dumpVehiclePropValue(writer, value);
                } catch (RuntimeException e) {
                    writer.printf("Can not get property value for property: %d // 0x%x "
                            + "in areaId: %d // 0x%x\n", config.prop, config.prop , area, area);
                }
            }
        }
    }

    /**
     * Dump VHAL property configs.
     *
     * @param writer
     * @param propId Property ID. If propid is empty string, dump all properties.
     */
    public void dumpPropertyConfigs(PrintWriter writer, int propId) {
        List<VehiclePropConfig> configList;
        synchronized (mLock) {
            configList = new ArrayList<>(mAllProperties.values());
        }

        if (propId == -1) {
            writer.println("**All properties**");
            for (VehiclePropConfig config : configList) {
                dumpPropertyConfigsHelp(writer, config);
            }
            return;
        }
        for (VehiclePropConfig config : configList) {
            if (config.prop == propId) {
                dumpPropertyConfigsHelp(writer, config);
                return;
            }
        }

    }

    /** Dumps VehiclePropertyConfigs */
    private static void dumpPropertyConfigsHelp(PrintWriter writer, VehiclePropConfig config) {
        writer.printf("Property:0x%x, Property name:%s, access:0x%x, changeMode:0x%x, "
                        + "config:%s, fs min:%f, fs max:%f\n",
                config.prop, VehiclePropertyIds.toString(config.prop), config.access,
                config.changeMode, Arrays.toString(config.configArray.toArray()),
                config.minSampleRate, config.maxSampleRate);
        for (VehicleAreaConfig area : config.areaConfigs) {
            writer.printf("\tareaId:0x%x, f min:%f, f max:%f, i min:%d, i max:%d,"
                    + " i64 min:%d, i64 max:%d\n",
                    area.areaId, area.minFloatValue, area.maxFloatValue, area.minInt32Value,
                    area.maxInt32Value, area.minInt64Value, area.maxInt64Value);
        }
    }

    /**
     * Inject a VHAL event
     *
     * @param property the Vehicle property Id as defined in the HAL
     * @param zone     Zone that this event services
     * @param value    Data value of the event
     * @param delayTime Add a certain duration to event timestamp
     */
    public void injectVhalEvent(int property, int zone, String value, int delayTime)
            throws NumberFormatException {

        VehiclePropValue v = createPropValueForInjecting(property, zone,
                Arrays.asList(value.split(DATA_DELIMITER)));
        if (v == null) {
            return;
        }
        // update timestamp
        v.timestamp = SystemClock.elapsedRealtimeNanos() + TimeUnit.SECONDS.toNanos(delayTime);
        onPropertyEvent(Lists.newArrayList(v));
    }

    /**
     * Injects continuous VHAL events.
     *
     * @param property the Vehicle property Id as defined in the HAL
     * @param zone Zone that this event services
     * @param value Data value of the event
     * @param sampleRate Sample Rate for events in Hz
     * @param timeDurationInSec The duration for injecting events in seconds
     */
    public void injectContinuousVhalEvent(int property, int zone, String value,
            float sampleRate, long timeDurationInSec) {

        VehiclePropValue v = createPropValueForInjecting(property, zone,
                new ArrayList<>(Arrays.asList(value.split(DATA_DELIMITER))));
        if (v == null) {
            return;
        }
        // rate in Hz
        if (sampleRate <= 0) {
            Slog.e(CarLog.TAG_HAL, "Inject events at an invalid sample rate: " + sampleRate);
            return;
        }
        long period = (long) (1000 / sampleRate);
        long stopTime = timeDurationInSec * 1000 + SystemClock.elapsedRealtime();
        Timer timer = new Timer();
        timer.schedule(new TimerTask() {
            @Override
            public void run() {
                if (stopTime < SystemClock.elapsedRealtime()) {
                    timer.cancel();
                    timer.purge();
                } else {
                    // Avoid the fake events be covered by real Event
                    v.timestamp = SystemClock.elapsedRealtimeNanos()
                            + TimeUnit.SECONDS.toNanos(timeDurationInSec);
                    onPropertyEvent(Lists.newArrayList(v));
                }
            }
        }, /* delay= */0, period);
    }

    // Returns null if the property type is unsupported.
    @Nullable
    private static VehiclePropValue createPropValueForInjecting(int propId, int zoneId,
            List<String> dataList) {
        VehiclePropValue v = createPropValue(propId, zoneId);
        int propertyType = propId & VehiclePropertyType.MASK;
        // Values can be comma separated list
        switch (propertyType) {
            case VehiclePropertyType.BOOLEAN:
                boolean boolValue = Boolean.parseBoolean(dataList.get(0));
                v.value.int32Values.add(boolValue ? 1 : 0);
                break;
            case VehiclePropertyType.INT32:
            case VehiclePropertyType.INT32_VEC:
                for (String s : dataList) {
                    v.value.int32Values.add(Integer.decode(s));
                }
                break;
            case VehiclePropertyType.FLOAT:
            case VehiclePropertyType.FLOAT_VEC:
                for (String s : dataList) {
                    v.value.floatValues.add(Float.parseFloat(s));
                }
                break;
            default:
                Slog.e(CarLog.TAG_HAL, "Property type unsupported:" + propertyType);
                return null;
        }
        return v;
    }

    private static class VehiclePropertyEventInfo {
        private int mEventCount;
        private VehiclePropValue mLastEvent;

        private VehiclePropertyEventInfo(VehiclePropValue event) {
            mEventCount = 1;
            mLastEvent = event;
        }

        private void addNewEvent(VehiclePropValue event) {
            mEventCount++;
            mLastEvent = event;
        }
    }

    final class VehiclePropValueSetter {
        final WeakReference<HalClient> mClient;
        final VehiclePropValue mPropValue;

        private VehiclePropValueSetter(HalClient client, int propId, int areaId) {
            mClient = new WeakReference<>(client);
            mPropValue = new VehiclePropValue();
            mPropValue.prop = propId;
            mPropValue.areaId = areaId;
        }

        void to(boolean value) {
            to(value ? 1 : 0);
        }

        void to(int value) {
            mPropValue.value.int32Values.add(value);
            submit();
        }

        void to(int[] values) {
            for (int value : values) {
                mPropValue.value.int32Values.add(value);
            }
            submit();
        }

        void to(Collection<Integer> values) {
            mPropValue.value.int32Values.addAll(values);
            submit();
        }

        void submit() {
            HalClient client =  mClient.get();
            if (client != null) {
                if (DBG) {
                    Slog.i(CarLog.TAG_HAL, "set, " + toCarPropertyLog(mPropValue.prop)
                            + toCarAreaLog(mPropValue.areaId));
                }
                client.setValue(mPropValue);
            }
        }
    }

    private static void dumpVehiclePropValue(PrintWriter writer, VehiclePropValue value) {
        String bytesString = "";
        if (value.value.bytes.size() > MAX_BYTE_SIZE) {
            Object[] bytes = Arrays.copyOf(value.value.bytes.toArray(), MAX_BYTE_SIZE);
            bytesString = Arrays.toString(bytes);
        } else {
            bytesString = Arrays.toString(value.value.bytes.toArray());
        }

        writer.printf("Property:0x%x, status: %d, timestamp: %d, zone: 0x%x, "
                + "floatValues: %s, int32Values: %s, int64Values: %s, bytes: %s, string: %s\n",
                value.prop, value.status, value.timestamp, value.areaId,
                Arrays.toString(value.value.floatValues.toArray()),
                Arrays.toString(value.value.int32Values.toArray()),
                Arrays.toString(value.value.int64Values.toArray()),
                bytesString, value.value.stringValue);
    }

    private static VehiclePropValue createPropValue(int propId, int areaId) {
        VehiclePropValue propValue = new VehiclePropValue();
        propValue.prop = propId;
        propValue.areaId = areaId;
        return propValue;
    }

    private static String toCarPropertyLog(int propId) {
        return String.format("property Id: %d // 0x%x, property name: %s ", propId, propId,
                VehiclePropertyIds.toString(propId));
    }

    private static String toCarAreaLog(int areaId) {
        return String.format("areaId: %d // 0x%x", areaId, areaId);
    }
}
