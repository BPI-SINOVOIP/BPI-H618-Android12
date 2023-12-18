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

package com.android.car;

import static com.android.car.CarService.CAR_SERVICE_INIT_TIMING_MIN_DURATION_MS;
import static com.android.car.CarService.CAR_SERVICE_INIT_TIMING_TAG;
import static com.android.car.internal.ExcludeFromCodeCoverageGeneratedReport.DEPRECATED_CODE;
import static com.android.car.internal.ExcludeFromCodeCoverageGeneratedReport.DUMP_INFO;
import static com.android.car.internal.SystemConstants.ICAR_SYSTEM_SERVER_CLIENT;

import android.annotation.MainThread;
import android.annotation.Nullable;
import android.app.ActivityManager;
import android.car.Car;
import android.car.CarFeatures;
import android.car.ICar;
import android.car.user.CarUserManager;
import android.content.Context;
import android.content.pm.PackageManager;
import android.content.pm.UserInfo;
import android.content.res.Resources;
import android.frameworks.automotive.powerpolicy.internal.ICarPowerPolicySystemNotification;
import android.hardware.automotive.vehicle.V2_0.IVehicle;
import android.hardware.automotive.vehicle.V2_0.VehiclePropValue;
import android.hardware.automotive.vehicle.V2_0.VehicleProperty;
import android.os.Binder;
import android.os.Build;
import android.os.Bundle;
import android.os.IBinder;
import android.os.Process;
import android.os.RemoteException;
import android.os.ResultReceiver;
import android.os.ShellCallback;
import android.os.Trace;
import android.os.UserManager;
import android.util.EventLog;
import android.util.IndentingPrintWriter;
import android.util.Slog;
import android.util.TimingsTraceLog;

import com.android.car.admin.CarDevicePolicyService;
import com.android.car.admin.FactoryResetActivity;
import com.android.car.am.FixedActivityService;
import com.android.car.audio.CarAudioService;
import com.android.car.cluster.ClusterHomeService;
import com.android.car.cluster.ClusterNavigationService;
import com.android.car.cluster.InstrumentClusterService;
import com.android.car.evs.CarEvsService;
import com.android.car.garagemode.GarageModeService;
import com.android.car.hal.VehicleHal;
import com.android.car.internal.ExcludeFromCodeCoverageGeneratedReport;
import com.android.car.internal.ICarServiceHelper;
import com.android.car.internal.ICarSystemServerClient;
import com.android.car.internal.common.EventLogTags;
import com.android.car.pm.CarPackageManagerService;
import com.android.car.power.CarPowerManagementService;
import com.android.car.stats.CarStatsService;
import com.android.car.systeminterface.SystemInterface;
import com.android.car.telemetry.CarTelemetryService;
import com.android.car.user.CarUserNoticeService;
import com.android.car.user.CarUserService;
import com.android.car.util.LimitedTimingsTraceLog;
import com.android.car.vms.VmsBrokerService;
import com.android.car.watchdog.CarWatchdogService;
import com.android.internal.annotations.GuardedBy;
import com.android.internal.annotations.VisibleForTesting;
import com.android.internal.os.IResultReceiver;

import java.io.FileDescriptor;
import java.io.PrintWriter;
import java.util.ArrayList;
import java.util.Arrays;
import java.util.List;
import java.util.concurrent.Callable;

public class ICarImpl extends ICar.Stub {

    public static final String INTERNAL_INPUT_SERVICE = "internal_input";
    public static final String INTERNAL_SYSTEM_ACTIVITY_MONITORING_SERVICE =
            "system_activity_monitoring";

    private static final int INITIAL_VHAL_GET_RETRY = 2;

    private final Context mContext;
    private final VehicleHal mHal;

    private final CarFeatureController mFeatureController;

    private final SystemInterface mSystemInterface;

    private final SystemActivityMonitoringService mSystemActivityMonitoringService;
    private final CarPowerManagementService mCarPowerManagementService;
    private final CarPackageManagerService mCarPackageManagerService;
    private final CarInputService mCarInputService;
    private final CarDrivingStateService mCarDrivingStateService;
    private final CarUxRestrictionsManagerService mCarUXRestrictionsService;
    private final OccupantAwarenessService mOccupantAwarenessService;
    private final CarAudioService mCarAudioService;
    private final CarProjectionService mCarProjectionService;
    private final CarPropertyService mCarPropertyService;
    private final CarNightService mCarNightService;
    private final AppFocusService mAppFocusService;
    private final FixedActivityService mFixedActivityService;
    private final GarageModeService mGarageModeService;
    private final ClusterNavigationService mClusterNavigationService;
    private final InstrumentClusterService mInstrumentClusterService;
    private final CarLocationService mCarLocationService;
    private final CarBluetoothService mCarBluetoothService;
    private final PerUserCarServiceHelper mPerUserCarServiceHelper;
    private final CarDiagnosticService mCarDiagnosticService;
    private final CarStorageMonitoringService mCarStorageMonitoringService;
    private final CarMediaService mCarMediaService;
    private final CarUserService mCarUserService;
    private final CarOccupantZoneService mCarOccupantZoneService;
    private final CarUserNoticeService mCarUserNoticeService;
    private final VmsBrokerService mVmsBrokerService;
    private final CarBugreportManagerService mCarBugreportManagerService;
    private final CarStatsService mCarStatsService;
    private final CarExperimentalFeatureServiceController mCarExperimentalFeatureServiceController;
    private final CarWatchdogService mCarWatchdogService;
    private final CarDevicePolicyService mCarDevicePolicyService;
    private final ClusterHomeService mClusterHomeService;
    private final CarEvsService mCarEvsService;
    private final CarTelemetryService mCarTelemetryService;

    private final CarServiceBase[] mAllServices;

    private static final String TAG = CarLog.tagFor(ICarImpl.class);

    private static final boolean DBG = true; // TODO(b/154033860): STOPSHIP if true

    private TimingsTraceLog mBootTiming;

    private final Object mLock = new Object();

    /** Test only service. Populate it only when necessary. */
    @GuardedBy("mLock")
    private CarTestService mCarTestService;

    @GuardedBy("mLock")
    private ICarServiceHelper mICarServiceHelper;

    private final String mVehicleInterfaceName;

    private final ICarSystemServerClientImpl mICarSystemServerClientImpl;

    public ICarImpl(Context serviceContext, IVehicle vehicle, SystemInterface systemInterface,
            String vehicleInterfaceName) {
        this(serviceContext, vehicle, systemInterface, vehicleInterfaceName,
                /* carUserService= */ null, /* carWatchdogService= */ null,
                /* powerPolicyDaemon= */ null);
    }

    @VisibleForTesting
    ICarImpl(Context serviceContext, IVehicle vehicle, SystemInterface systemInterface,
            String vehicleInterfaceName,
            @Nullable CarUserService carUserService,
            @Nullable CarWatchdogService carWatchdogService,
            @Nullable ICarPowerPolicySystemNotification powerPolicyDaemon) {
        LimitedTimingsTraceLog t = new LimitedTimingsTraceLog(
                CAR_SERVICE_INIT_TIMING_TAG, Trace.TRACE_TAG_SYSTEM_SERVER,
                CAR_SERVICE_INIT_TIMING_MIN_DURATION_MS);
        t.traceBegin("ICarImpl.constructor");

        mContext = serviceContext;
        mSystemInterface = systemInterface;
        CarLocalServices.addService(SystemInterface.class, mSystemInterface);
        mHal = constructWithTrace(t, VehicleHal.class,
                () -> new VehicleHal(serviceContext, vehicle));

        t.traceBegin("VHAL.earlyInit");
        // Do this before any other service components to allow feature check. It should work
        // even without init. For that, vhal get is retried as it can be too early.
        VehiclePropValue disabledOptionalFeatureValue = mHal.getIfAvailableOrFailForEarlyStage(
                VehicleProperty.DISABLED_OPTIONAL_FEATURES, INITIAL_VHAL_GET_RETRY);
        t.traceEnd();

        String[] disabledFeaturesFromVhal = null;
        if (disabledOptionalFeatureValue != null) {
            String disabledFeatures = disabledOptionalFeatureValue.value.stringValue;
            if (disabledFeatures != null && !disabledFeatures.isEmpty()) {
                disabledFeaturesFromVhal = disabledFeatures.split(",");
            }
        }
        if (disabledFeaturesFromVhal == null) {
            disabledFeaturesFromVhal = new String[0];
        }
        Resources res = mContext.getResources();
        String[] defaultEnabledFeatures = res.getStringArray(
                R.array.config_allowed_optional_car_features);
        final String[] disabledFromVhal = disabledFeaturesFromVhal;
        mFeatureController = constructWithTrace(t, CarFeatureController.class,
                () -> new CarFeatureController(serviceContext, defaultEnabledFeatures,
                        disabledFromVhal, mSystemInterface.getSystemCarDir()));
        mVehicleInterfaceName = vehicleInterfaceName;
        mCarPropertyService = constructWithTrace(
                t, CarPropertyService.class,
                () -> new CarPropertyService(serviceContext, mHal.getPropertyHal()));
        mCarDrivingStateService = constructWithTrace(
                t, CarDrivingStateService.class,
                () -> new CarDrivingStateService(serviceContext, mCarPropertyService));
        mCarUXRestrictionsService = constructWithTrace(t, CarUxRestrictionsManagerService.class,
                () -> new CarUxRestrictionsManagerService(serviceContext, mCarDrivingStateService,
                        mCarPropertyService));
        if (carUserService != null) {
            mCarUserService = carUserService;
            CarLocalServices.addService(CarUserService.class, carUserService);
        } else {
            UserManager userManager =
                    (UserManager) serviceContext.getSystemService(Context.USER_SERVICE);
            int maxRunningUsers = res.getInteger(
                    com.android.internal.R.integer.config_multiuserMaxRunningUsers);
            mCarUserService = constructWithTrace(t, CarUserService.class,
                    () -> new CarUserService(serviceContext, mHal.getUserHal(), userManager,
                            ActivityManager.getService(), maxRunningUsers,
                            mCarUXRestrictionsService));
        }
        mCarOccupantZoneService = constructWithTrace(t, CarOccupantZoneService.class,
                () -> new CarOccupantZoneService(serviceContext));
        mSystemActivityMonitoringService = constructWithTrace(
                t, SystemActivityMonitoringService.class,
                () -> new SystemActivityMonitoringService(serviceContext));
        mCarPowerManagementService = constructWithTrace(
                t, CarPowerManagementService.class,
                () -> new CarPowerManagementService(mContext, mHal.getPowerHal(),
                        systemInterface, mCarUserService, powerPolicyDaemon));
        if (mFeatureController.isFeatureEnabled(CarFeatures.FEATURE_CAR_USER_NOTICE_SERVICE)) {
            mCarUserNoticeService = constructWithTrace(
                    t, CarUserNoticeService.class, () -> new CarUserNoticeService(serviceContext));
        } else {
            mCarUserNoticeService = null;
        }
        if (mFeatureController.isFeatureEnabled(Car.OCCUPANT_AWARENESS_SERVICE)) {
            mOccupantAwarenessService = constructWithTrace(t, OccupantAwarenessService.class,
                    () -> new OccupantAwarenessService(serviceContext));
        } else {
            mOccupantAwarenessService = null;
        }
        mCarPackageManagerService = constructWithTrace(t, CarPackageManagerService.class,
                () -> new CarPackageManagerService(serviceContext, mCarUXRestrictionsService,
                        mSystemActivityMonitoringService));
        mPerUserCarServiceHelper = constructWithTrace(
                t, PerUserCarServiceHelper.class,
                () -> new PerUserCarServiceHelper(serviceContext, mCarUserService));
        mCarBluetoothService = constructWithTrace(t, CarBluetoothService.class,
                () -> new CarBluetoothService(serviceContext, mPerUserCarServiceHelper));
        mCarInputService = constructWithTrace(t, CarInputService.class,
                () -> new CarInputService(serviceContext, mHal.getInputHal(), mCarUserService,
                        mCarOccupantZoneService));
        mCarProjectionService = constructWithTrace(t, CarProjectionService.class,
                () -> new CarProjectionService(serviceContext, null /* handler */, mCarInputService,
                        mCarBluetoothService));
        mGarageModeService = constructWithTrace(t, GarageModeService.class,
                () -> new GarageModeService(mContext));
        mAppFocusService = constructWithTrace(t, AppFocusService.class,
                () -> new AppFocusService(serviceContext, mSystemActivityMonitoringService));
        mCarAudioService = constructWithTrace(t, CarAudioService.class,
                () -> new CarAudioService(serviceContext));
        mCarNightService = constructWithTrace(t, CarNightService.class,
                () -> new CarNightService(serviceContext, mCarPropertyService));
        mFixedActivityService = constructWithTrace(
                t, FixedActivityService.class, () -> new FixedActivityService(serviceContext));
        mClusterNavigationService = constructWithTrace(
                t, ClusterNavigationService.class,
                () -> new ClusterNavigationService(serviceContext, mAppFocusService));
        if (mFeatureController.isFeatureEnabled(Car.CAR_INSTRUMENT_CLUSTER_SERVICE)) {
            mInstrumentClusterService = constructWithTrace(t, InstrumentClusterService.class,
                    () -> new InstrumentClusterService(serviceContext,
                            mClusterNavigationService, mCarInputService));
        } else {
            mInstrumentClusterService = null;
        }
        mCarStatsService = constructWithTrace(t, CarStatsService.class, () -> {
            // This service should be initialized here.
            CarStatsService service = new CarStatsService(serviceContext);
            service.init();
            return service;
        });
        if (mFeatureController.isFeatureEnabled(Car.VEHICLE_MAP_SERVICE)) {
            mVmsBrokerService = constructWithTrace(t, VmsBrokerService.class,
                    () -> new VmsBrokerService(mContext, mCarStatsService));
        } else {
            mVmsBrokerService = null;
        }
        if (mFeatureController.isFeatureEnabled(Car.DIAGNOSTIC_SERVICE)) {
            mCarDiagnosticService = constructWithTrace(t, CarDiagnosticService.class,
                    () -> new CarDiagnosticService(serviceContext,
                            mHal.getDiagnosticHal()));
        } else {
            mCarDiagnosticService = null;
        }
        if (mFeatureController.isFeatureEnabled(Car.STORAGE_MONITORING_SERVICE)) {
            mCarStorageMonitoringService = constructWithTrace(
                    t, CarStorageMonitoringService.class,
                    () -> new CarStorageMonitoringService(serviceContext,
                            systemInterface));
        } else {
            mCarStorageMonitoringService = null;
        }
        mCarLocationService = constructWithTrace(t, CarLocationService.class,
                () -> new CarLocationService(serviceContext));
        mCarMediaService = constructWithTrace(t, CarMediaService.class,
                () -> new CarMediaService(serviceContext, mCarUserService));
        mCarBugreportManagerService = constructWithTrace(t, CarBugreportManagerService.class,
                () -> new CarBugreportManagerService(serviceContext));
        if (!Build.IS_USER) {
            mCarExperimentalFeatureServiceController = constructWithTrace(
                    t, CarExperimentalFeatureServiceController.class,
                    () -> new CarExperimentalFeatureServiceController(serviceContext));
        } else {
            mCarExperimentalFeatureServiceController = null;
        }
        if (carWatchdogService == null) {
            mCarWatchdogService = constructWithTrace(t, CarWatchdogService.class,
                    () -> new CarWatchdogService(serviceContext));
        } else {
            mCarWatchdogService = carWatchdogService;
        }
        mCarDevicePolicyService = constructWithTrace(
                t, CarDevicePolicyService.class, () -> new CarDevicePolicyService(mCarUserService));
        if (mFeatureController.isFeatureEnabled(Car.CLUSTER_HOME_SERVICE)) {
            if (!mFeatureController.isFeatureEnabled(Car.CAR_INSTRUMENT_CLUSTER_SERVICE)) {
                mClusterHomeService = constructWithTrace(
                        t, ClusterHomeService.class,
                        () -> new ClusterHomeService(serviceContext, mHal.getClusterHal(),
                        mClusterNavigationService, mCarOccupantZoneService, mFixedActivityService));
            } else {
                Slog.w(TAG, "Can't init ClusterHomeService, since Old cluster service is running");
                mClusterHomeService = null;
            }
        } else {
            mClusterHomeService = null;
        }

        if (mFeatureController.isFeatureEnabled(Car.CAR_EVS_SERVICE)) {
            mCarEvsService = constructWithTrace(t, CarEvsService.class,
                    () -> new CarEvsService(serviceContext, mHal.getEvsHal(), mCarPropertyService));
        } else {
            mCarEvsService = null;
        }

        if (mFeatureController.isFeatureEnabled(Car.CAR_TELEMETRY_SERVICE)) {
            mCarTelemetryService = new CarTelemetryService(serviceContext);
        } else {
            mCarTelemetryService = null;
        }

        // Be careful with order. Service depending on other service should be inited later.
        List<CarServiceBase> allServices = new ArrayList<>();
        allServices.add(mFeatureController);
        allServices.add(mCarUXRestrictionsService); // mCarUserService depends on it
        allServices.add(mCarUserService);
        allServices.add(mSystemActivityMonitoringService);
        allServices.add(mCarPowerManagementService);
        allServices.add(mCarPropertyService);
        allServices.add(mCarDrivingStateService);
        allServices.add(mCarOccupantZoneService);
        addServiceIfNonNull(allServices, mOccupantAwarenessService);
        allServices.add(mCarPackageManagerService);
        allServices.add(mCarInputService);
        allServices.add(mGarageModeService);
        addServiceIfNonNull(allServices, mCarUserNoticeService);
        allServices.add(mAppFocusService);
        allServices.add(mCarAudioService);
        allServices.add(mCarNightService);
        allServices.add(mFixedActivityService);
        allServices.add(mClusterNavigationService);
        addServiceIfNonNull(allServices, mInstrumentClusterService);
        allServices.add(mPerUserCarServiceHelper);
        allServices.add(mCarBluetoothService);
        allServices.add(mCarProjectionService);
        addServiceIfNonNull(allServices, mCarDiagnosticService);
        addServiceIfNonNull(allServices, mCarStorageMonitoringService);
        addServiceIfNonNull(allServices, mVmsBrokerService);
        allServices.add(mCarMediaService);
        allServices.add(mCarLocationService);
        allServices.add(mCarBugreportManagerService);
        allServices.add(mCarWatchdogService);
        allServices.add(mCarDevicePolicyService);
        addServiceIfNonNull(allServices, mClusterHomeService);
        addServiceIfNonNull(allServices, mCarEvsService);
        addServiceIfNonNull(allServices, mCarTelemetryService);

        // Always put mCarExperimentalFeatureServiceController in last.
        addServiceIfNonNull(allServices, mCarExperimentalFeatureServiceController);
        mAllServices = allServices.toArray(new CarServiceBase[allServices.size()]);

        mICarSystemServerClientImpl = new ICarSystemServerClientImpl();

        t.traceEnd(); // "ICarImpl.constructor"
    }

    private void addServiceIfNonNull(List<CarServiceBase> services, CarServiceBase service) {
        if (service != null) {
            services.add(service);
        }
    }

    @MainThread
    void init() {
        LimitedTimingsTraceLog t = new LimitedTimingsTraceLog(CAR_SERVICE_INIT_TIMING_TAG,
                Trace.TRACE_TAG_SYSTEM_SERVER, CAR_SERVICE_INIT_TIMING_MIN_DURATION_MS);

        t.traceBegin("ICarImpl.init");

        t.traceBegin("VHAL.init");
        mHal.init();
        t.traceEnd();

        t.traceBegin("CarService.initAllServices");
        for (CarServiceBase service : mAllServices) {
            t.traceBegin(service.getClass().getSimpleName());
            service.init();
            t.traceEnd();
        }
        t.traceEnd(); // "CarService.initAllServices"

        t.traceEnd(); // "ICarImpl.init"
    }

    void release() {
        // release done in opposite order from init
        for (int i = mAllServices.length - 1; i >= 0; i--) {
            mAllServices[i].release();
        }
        mHal.release();
    }

    void vehicleHalReconnected(IVehicle vehicle) {
        EventLog.writeEvent(EventLogTags.CAR_SERVICE_VHAL_RECONNECTED, mAllServices.length);
        mHal.vehicleHalReconnected(vehicle);
        for (CarServiceBase service : mAllServices) {
            service.vehicleHalReconnected();
        }
    }

    @Override
    public void setSystemServerConnections(IBinder helper, IBinder receiver) {
        Bundle bundle;
        try {
            EventLog.writeEvent(EventLogTags.CAR_SERVICE_SET_CAR_SERVICE_HELPER,
                    Binder.getCallingPid());
            assertCallingFromSystemProcess();
            ICarServiceHelper carServiceHelper = ICarServiceHelper.Stub.asInterface(helper);
            synchronized (mLock) {
                mICarServiceHelper = carServiceHelper;
            }
            // TODO(b/173030628) create a proxy wrapping access to CarServiceHelper instead
            mSystemInterface.setCarServiceHelper(carServiceHelper);
            mCarOccupantZoneService.setCarServiceHelper(carServiceHelper);
            mCarUserService.setCarServiceHelper(carServiceHelper);

            bundle = new Bundle();
            bundle.putBinder(ICAR_SYSTEM_SERVER_CLIENT, mICarSystemServerClientImpl.asBinder());
        } catch (Exception e) {
            // send back a null response
            Slog.w(TAG, "Exception in setSystemServerConnections", e);
            bundle = null;
        }

        try {
            IResultReceiver resultReceiver = IResultReceiver.Stub.asInterface(receiver);
            resultReceiver.send(/* unused */ 0, bundle);
        } catch (RemoteException e) {
            Slog.w(TAG, "RemoteException from CarServiceHelperService", e);
        }
    }

    @Override
    public boolean isFeatureEnabled(String featureName) {
        return mFeatureController.isFeatureEnabled(featureName);
    }

    @Override
    public int enableFeature(String featureName) {
        // permission check inside the controller
        return mFeatureController.enableFeature(featureName);
    }

    @Override
    public int disableFeature(String featureName) {
        // permission check inside the controller
        return mFeatureController.disableFeature(featureName);
    }

    @Override
    public List<String> getAllEnabledFeatures() {
        // permission check inside the controller
        return mFeatureController.getAllEnabledFeatures();
    }

    @Override
    public List<String> getAllPendingDisabledFeatures() {
        // permission check inside the controller
        return mFeatureController.getAllPendingDisabledFeatures();
    }

    @Override
    public List<String> getAllPendingEnabledFeatures() {
        // permission check inside the controller
        return mFeatureController.getAllPendingEnabledFeatures();
    }

    @Override
    public String getCarManagerClassForFeature(String featureName) {
        if (mCarExperimentalFeatureServiceController == null) {
            return null;
        }
        return mCarExperimentalFeatureServiceController.getCarManagerClassForFeature(featureName);
    }

    static void assertCallingFromSystemProcess() {
        int uid = Binder.getCallingUid();
        if (uid != Process.SYSTEM_UID) {
            throw new SecurityException("Only allowed from system");
        }
    }

    /**
     * Assert if binder call is coming from system process like system server or if it is called
     * from its own process even if it is not system. The latter can happen in test environment.
     * Note that car service runs as system user but test like car service test will not.
     */
    public static void assertCallingFromSystemProcessOrSelf() {
        if (isCallingFromSystemProcessOrSelf()) {
            throw new SecurityException("Only allowed from system or self");
        }
    }

    /**
     * @return true if binder call is coming from system process like system server or if it is
     * called from its own process even if it is not system.
     */
    public static boolean isCallingFromSystemProcessOrSelf() {
        int uid = Binder.getCallingUid();
        int pid = Binder.getCallingPid();
        return uid != Process.SYSTEM_UID && pid != Process.myPid();
    }

    @Override
    public IBinder getCarService(String serviceName) {
        if (!mFeatureController.isFeatureEnabled(serviceName)) {
            Slog.w(CarLog.TAG_SERVICE, "getCarService for disabled service:" + serviceName);
            return null;
        }
        switch (serviceName) {
            case Car.AUDIO_SERVICE:
                return mCarAudioService;
            case Car.APP_FOCUS_SERVICE:
                return mAppFocusService;
            case Car.PACKAGE_SERVICE:
                return mCarPackageManagerService;
            case Car.DIAGNOSTIC_SERVICE:
                assertAnyDiagnosticPermission(mContext);
                return mCarDiagnosticService;
            case Car.POWER_SERVICE:
                return mCarPowerManagementService;
            case Car.CABIN_SERVICE:
            case Car.HVAC_SERVICE:
            case Car.INFO_SERVICE:
            case Car.PROPERTY_SERVICE:
            case Car.SENSOR_SERVICE:
            case Car.VENDOR_EXTENSION_SERVICE:
                return mCarPropertyService;
            case Car.CAR_NAVIGATION_SERVICE:
                assertNavigationManagerPermission(mContext);
                return mClusterNavigationService;
            case Car.CAR_INSTRUMENT_CLUSTER_SERVICE:
                assertClusterManagerPermission(mContext);
                return mInstrumentClusterService.getManagerService();
            case Car.PROJECTION_SERVICE:
                return mCarProjectionService;
            case Car.VEHICLE_MAP_SERVICE:
                assertAnyVmsPermission(mContext);
                return mVmsBrokerService;
            case Car.VMS_SUBSCRIBER_SERVICE:
                assertVmsSubscriberPermission(mContext);
                return mVmsBrokerService;
            case Car.TEST_SERVICE: {
                assertPermission(mContext, Car.PERMISSION_CAR_TEST_SERVICE);
                synchronized (mLock) {
                    if (mCarTestService == null) {
                        mCarTestService = new CarTestService(mContext, this);
                    }
                    return mCarTestService;
                }
            }
            case Car.BLUETOOTH_SERVICE:
                return mCarBluetoothService;
            case Car.STORAGE_MONITORING_SERVICE:
                assertPermission(mContext, Car.PERMISSION_STORAGE_MONITORING);
                return mCarStorageMonitoringService;
            case Car.CAR_DRIVING_STATE_SERVICE:
                assertDrivingStatePermission(mContext);
                return mCarDrivingStateService;
            case Car.CAR_UX_RESTRICTION_SERVICE:
                return mCarUXRestrictionsService;
            case Car.OCCUPANT_AWARENESS_SERVICE:
                return mOccupantAwarenessService;
            case Car.CAR_MEDIA_SERVICE:
                return mCarMediaService;
            case Car.CAR_OCCUPANT_ZONE_SERVICE:
                return mCarOccupantZoneService;
            case Car.CAR_BUGREPORT_SERVICE:
                return mCarBugreportManagerService;
            case Car.CAR_USER_SERVICE:
                return mCarUserService;
            case Car.CAR_WATCHDOG_SERVICE:
                return mCarWatchdogService;
            case Car.CAR_INPUT_SERVICE:
                return mCarInputService;
            case Car.CAR_DEVICE_POLICY_SERVICE:
                return mCarDevicePolicyService;
            case Car.CLUSTER_HOME_SERVICE:
                return mClusterHomeService;
            case Car.CAR_EVS_SERVICE:
                return mCarEvsService;
            case Car.CAR_TELEMETRY_SERVICE:
                return mCarTelemetryService;
            default:
                IBinder service = null;
                if (mCarExperimentalFeatureServiceController != null) {
                    service = mCarExperimentalFeatureServiceController.getCarService(serviceName);
                }
                if (service == null) {
                    Slog.w(CarLog.TAG_SERVICE, "getCarService for unknown service:"
                            + serviceName);
                }
                return service;
        }
    }

    @Override
    @ExcludeFromCodeCoverageGeneratedReport(reason = DEPRECATED_CODE)
    public int getCarConnectionType() {
        return Car.CONNECTION_TYPE_EMBEDDED;
    }

    public static void assertVehicleHalMockPermission(Context context) {
        assertPermission(context, Car.PERMISSION_MOCK_VEHICLE_HAL);
    }

    public static void assertNavigationManagerPermission(Context context) {
        assertPermission(context, Car.PERMISSION_CAR_NAVIGATION_MANAGER);
    }

    public static void assertClusterManagerPermission(Context context) {
        assertPermission(context, Car.PERMISSION_CAR_INSTRUMENT_CLUSTER_CONTROL);
    }

    public static void assertPowerPermission(Context context) {
        assertPermission(context, Car.PERMISSION_CAR_POWER);
    }

    public static void assertProjectionPermission(Context context) {
        assertPermission(context, Car.PERMISSION_CAR_PROJECTION);
    }

    /** Verify the calling context has the {@link Car#PERMISSION_CAR_PROJECTION_STATUS} */
    public static void assertProjectionStatusPermission(Context context) {
        assertPermission(context, Car.PERMISSION_CAR_PROJECTION_STATUS);
    }

    public static void assertAnyDiagnosticPermission(Context context) {
        assertAnyPermission(context,
                Car.PERMISSION_CAR_DIAGNOSTIC_READ_ALL,
                Car.PERMISSION_CAR_DIAGNOSTIC_CLEAR);
    }

    public static void assertDrivingStatePermission(Context context) {
        assertPermission(context, Car.PERMISSION_CAR_DRIVING_STATE);
    }

    /**
     * Verify the calling context has either {@link Car#PERMISSION_VMS_SUBSCRIBER} or
     * {@link Car#PERMISSION_VMS_PUBLISHER}
     */
    public static void assertAnyVmsPermission(Context context) {
        assertAnyPermission(context,
                Car.PERMISSION_VMS_SUBSCRIBER,
                Car.PERMISSION_VMS_PUBLISHER);
    }

    public static void assertVmsPublisherPermission(Context context) {
        assertPermission(context, Car.PERMISSION_VMS_PUBLISHER);
    }

    public static void assertVmsSubscriberPermission(Context context) {
        assertPermission(context, Car.PERMISSION_VMS_SUBSCRIBER);
    }

    public static void assertPermission(Context context, String permission) {
        if (context.checkCallingOrSelfPermission(permission) != PackageManager.PERMISSION_GRANTED) {
            throw new SecurityException("requires " + permission);
        }
    }

    /**
     * Checks to see if the caller has a permission.
     *
     * @return boolean TRUE if caller has the permission.
     */
    public static boolean hasPermission(Context context, String permission) {
        return context.checkCallingOrSelfPermission(permission)
                == PackageManager.PERMISSION_GRANTED;
    }

    public static void assertAnyPermission(Context context, String... permissions) {
        for (String permission : permissions) {
            if (context.checkCallingOrSelfPermission(permission)
                    == PackageManager.PERMISSION_GRANTED) {
                return;
            }
        }
        throw new SecurityException("requires any of " + Arrays.toString(permissions));
    }

    @Override
    @ExcludeFromCodeCoverageGeneratedReport(reason = DUMP_INFO)
    protected void dump(FileDescriptor fd, PrintWriter writer, String[] args) {
        if (mContext.checkCallingOrSelfPermission(android.Manifest.permission.DUMP)
                != PackageManager.PERMISSION_GRANTED) {
            writer.println("Permission Denial: can't dump CarService from from pid="
                    + Binder.getCallingPid() + ", uid=" + Binder.getCallingUid()
                    + " without permission " + android.Manifest.permission.DUMP);
            return;
        }

        try (IndentingPrintWriter pw = new IndentingPrintWriter(writer)) {
            dumpIndenting(fd, pw, args);
        }
    }

    @ExcludeFromCodeCoverageGeneratedReport(reason = DUMP_INFO)
    private void dumpIndenting(FileDescriptor fd, IndentingPrintWriter writer, String[] args) {
        if (args == null || args.length == 0 || (args.length > 0 && "-a".equals(args[0]))) {
            writer.println("*Dump car service*");
            dumpAllServices(writer);
            dumpAllHals(writer);
        } else if ("--list".equals(args[0])) {
            dumpListOfServices(writer);
            return;
        } else if ("--services".equals(args[0])) {
            if (args.length < 2) {
                writer.println("Must pass services to dump when using --services");
                return;
            }
            int length = args.length - 1;
            String[] services = new String[length];
            System.arraycopy(args, 1, services, 0, length);
            dumpIndividualServices(writer, services);
            return;
        } else if ("--metrics".equals(args[0])) {
            // Strip the --metrics flag when passing dumpsys arguments to CarStatsService
            // allowing for nested flag selection
            mCarStatsService.dump(writer, Arrays.copyOfRange(args, 1, args.length));
        } else if ("--vms-hal".equals(args[0])) {
            mHal.getVmsHal().dumpMetrics(fd);
        } else if ("--hal".equals(args[0])) {
            if (args.length == 1) {
                dumpAllHals(writer);
                return;
            }
            int length = args.length - 1;
            String[] halNames = new String[length];
            System.arraycopy(args, 1, halNames, 0, length);
            mHal.dumpSpecificHals(writer, halNames);

        } else if ("--list-hals".equals(args[0])) {
            mHal.dumpListHals(writer);
            return;
        } else if ("--help".equals(args[0])) {
            showDumpHelp(writer);
        } else {
            execShellCmd(args, writer);
        }
    }

    @ExcludeFromCodeCoverageGeneratedReport(reason = DUMP_INFO)
    private void dumpAllHals(IndentingPrintWriter writer) {
        writer.println("*Dump Vehicle HAL*");
        writer.println("Vehicle HAL Interface: " + mVehicleInterfaceName);
        try {
            // TODO dump all feature flags by creating a dumpable interface
            mHal.dump(writer);
        } catch (Exception e) {
            writer.println("Failed dumping: " + mHal.getClass().getName());
            e.printStackTrace(writer);
        }
    }

    @ExcludeFromCodeCoverageGeneratedReport(reason = DUMP_INFO)
    private void showDumpHelp(IndentingPrintWriter writer) {
        writer.println("Car service dump usage:");
        writer.println("[NO ARG]");
        writer.println("\t  dumps everything (all services and HALs)");
        writer.println("--help");
        writer.println("\t  shows this help");
        writer.println("--list");
        writer.println("\t  lists the name of all services");
        writer.println("--list-hals");
        writer.println("\t  lists the name of all HALs");
        writer.println("--services <SVC1> [SVC2] [SVCN]");
        writer.println("\t  dumps just the specific services, where SVC is just the service class");
        writer.println("\t  name (like CarUserService)");
        writer.println("--vms-hal");
        writer.println("\t  dumps the VMS HAL metrics");
        writer.println("--hal [HAL1] [HAL2] [HALN]");
        writer.println("\t  dumps just the specified HALs (or all of them if none specified),");
        writer.println("\t  where HAL is just the class name (like UserHalService)");
        writer.println("--user-metrics");
        writer.println("\t  dumps user switching and stopping metrics ");
        writer.println("--first-user-metrics");
        writer.println("\t  dumps how long it took to unlock first user since Android started\n");
        writer.println("\t  (or -1 if not unlocked)");
        writer.println("-h");
        writer.println("\t  shows commands usage (NOTE: commands are not available on USER builds");
        writer.println("[ANYTHING ELSE]");
        writer.println("\t  runs the given command (use --h to see the available commands)");
    }

    @Override
    public void onShellCommand(FileDescriptor in, FileDescriptor out, FileDescriptor err,
            String[] args, ShellCallback callback, ResultReceiver resultReceiver)
                    throws RemoteException {
        newCarShellCommand().exec(this, in, out, err, args, callback, resultReceiver);
    }

    private CarShellCommand newCarShellCommand() {
        return new CarShellCommand(mContext, mHal, mCarAudioService, mCarPackageManagerService,
                mCarProjectionService, mCarPowerManagementService, mFixedActivityService,
                mFeatureController, mCarInputService, mCarNightService, mSystemInterface,
                mGarageModeService, mCarUserService, mCarOccupantZoneService, mCarEvsService,
                mCarWatchdogService);
    }

    @ExcludeFromCodeCoverageGeneratedReport(reason = DUMP_INFO)
    private void dumpListOfServices(IndentingPrintWriter writer) {
        for (CarServiceBase service : mAllServices) {
            writer.println(service.getClass().getName());
        }
    }

    @ExcludeFromCodeCoverageGeneratedReport(reason = DUMP_INFO)
    private void dumpAllServices(IndentingPrintWriter writer) {
        writer.println("*Dump all services*");
        for (CarServiceBase service : mAllServices) {
            dumpService(service, writer);
        }
        if (mCarTestService != null) {
            dumpService(mCarTestService, writer);
        }
    }

    @ExcludeFromCodeCoverageGeneratedReport(reason = DUMP_INFO)
    private void dumpIndividualServices(IndentingPrintWriter writer, String... serviceNames) {
        for (String serviceName : serviceNames) {
            writer.printf("** Dumping %s\n\n", serviceName);
            CarServiceBase service = getCarServiceBySubstring(serviceName);
            if (service == null) {
                writer.println("No such service!");
            } else {
                dumpService(service, writer);
            }
            writer.println();
        }
    }

    @Nullable
    private CarServiceBase getCarServiceBySubstring(String className) {
        return Arrays.asList(mAllServices).stream()
                .filter(s -> s.getClass().getSimpleName().equals(className))
                .findFirst().orElse(null);
    }

    @ExcludeFromCodeCoverageGeneratedReport(reason = DUMP_INFO)
    private void dumpService(CarServiceBase service, IndentingPrintWriter writer) {
        try {
            service.dump(writer);
        } catch (Exception e) {
            writer.println("Failed dumping: " + service.getClass().getName());
            e.printStackTrace(writer);
        }
    }

    void execShellCmd(String[] args, IndentingPrintWriter writer) {
        newCarShellCommand().exec(args, writer);
    }

    private <T> T constructWithTrace(LimitedTimingsTraceLog t, Class<T> cls, Callable<T> callable) {
        t.traceBegin(cls.getSimpleName());
        T constructed;
        try {
            constructed = callable.call();
            CarLocalServices.addService(cls, constructed);
        } catch (Exception e) {
            throw new RuntimeException("Crash while constructing:" + cls.getSimpleName(), e);
        } finally {
            t.traceEnd();
        }
        return constructed;
    }

    private final class ICarSystemServerClientImpl extends ICarSystemServerClient.Stub {
        @Override
        public void onUserLifecycleEvent(int eventType, int fromUserId, int toUserId)
                throws RemoteException {
            assertCallingFromSystemProcess();
            EventLog.writeEvent(EventLogTags.CAR_SERVICE_ON_USER_LIFECYCLE, eventType, fromUserId,
                    toUserId);
            if (DBG) {
                Slog.d(TAG,
                        "onUserLifecycleEvent("
                                + CarUserManager.lifecycleEventTypeToString(eventType) + ", "
                                + toUserId + ")");
            }
            mCarUserService.onUserLifecycleEvent(eventType, fromUserId, toUserId);
        }

        @Override
        public void initBootUser() throws RemoteException {
            assertCallingFromSystemProcess();
            EventLog.writeEvent(EventLogTags.CAR_SERVICE_INIT_BOOT_USER);
            if (DBG) Slog.d(TAG, "initBootUser(): ");
            mCarUserService.initBootUser();
        }

        @Override
        public void onUserRemoved(UserInfo user) throws RemoteException {
            assertCallingFromSystemProcess();
            EventLog.writeEvent(EventLogTags.CAR_SERVICE_ON_USER_REMOVED, user.id);
            if (DBG) Slog.d(TAG, "onUserRemoved(): " + user.toFullString());
            mCarUserService.onUserRemoved(user);
        }

        @Override
        public void onFactoryReset(IResultReceiver callback) {
            assertCallingFromSystemProcess();

            mCarPowerManagementService.setFactoryResetCallback(callback);
            FactoryResetActivity.sendNotification(mContext, callback);
        }
    }
}
