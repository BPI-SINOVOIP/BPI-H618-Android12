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
package com.android.car;

import static android.car.Car.PERMISSION_CAR_CONTROL_AUDIO_VOLUME;
import static android.car.Car.PERMISSION_CAR_POWER;
import static android.car.Car.PERMISSION_CONTROL_CAR_WATCHDOG_CONFIG;
import static android.hardware.automotive.vehicle.V2_0.UserIdentificationAssociationSetValue.ASSOCIATE_CURRENT_USER;
import static android.hardware.automotive.vehicle.V2_0.UserIdentificationAssociationSetValue.DISASSOCIATE_ALL_USERS;
import static android.hardware.automotive.vehicle.V2_0.UserIdentificationAssociationSetValue.DISASSOCIATE_CURRENT_USER;
import static android.hardware.automotive.vehicle.V2_0.UserIdentificationAssociationType.CUSTOM_1;
import static android.hardware.automotive.vehicle.V2_0.UserIdentificationAssociationType.CUSTOM_2;
import static android.hardware.automotive.vehicle.V2_0.UserIdentificationAssociationType.CUSTOM_3;
import static android.hardware.automotive.vehicle.V2_0.UserIdentificationAssociationType.CUSTOM_4;
import static android.hardware.automotive.vehicle.V2_0.UserIdentificationAssociationType.KEY_FOB;
import static android.media.AudioManager.FLAG_SHOW_UI;

import static com.android.car.power.PolicyReader.POWER_STATE_ON;
import static com.android.car.power.PolicyReader.POWER_STATE_WAIT_FOR_VHAL;

import android.annotation.NonNull;
import android.annotation.Nullable;
import android.annotation.UserIdInt;
import android.app.ActivityManager;
import android.app.ActivityOptions;
import android.app.UiModeManager;
import android.car.Car;
import android.car.CarOccupantZoneManager;
import android.car.VehiclePropertyIds;
import android.car.input.CarInputManager;
import android.car.input.CustomInputEvent;
import android.car.input.RotaryEvent;
import android.car.user.CarUserManager;
import android.car.user.UserCreationResult;
import android.car.user.UserIdentificationAssociationResponse;
import android.car.user.UserRemovalResult;
import android.car.user.UserSwitchResult;
import android.car.userlib.HalCallback;
import android.car.userlib.UserHalHelper;
import android.car.util.concurrent.AsyncFuture;
import android.car.watchdog.CarWatchdogManager;
import android.car.watchdog.IoOveruseConfiguration;
import android.car.watchdog.PerStateBytes;
import android.car.watchdog.ResourceOveruseConfiguration;
import android.content.ComponentName;
import android.content.Context;
import android.content.Intent;
import android.hardware.automotive.vehicle.V2_0.CreateUserRequest;
import android.hardware.automotive.vehicle.V2_0.CreateUserStatus;
import android.hardware.automotive.vehicle.V2_0.InitialUserInfoResponse;
import android.hardware.automotive.vehicle.V2_0.InitialUserInfoResponseAction;
import android.hardware.automotive.vehicle.V2_0.RemoveUserRequest;
import android.hardware.automotive.vehicle.V2_0.SwitchUserMessageType;
import android.hardware.automotive.vehicle.V2_0.SwitchUserRequest;
import android.hardware.automotive.vehicle.V2_0.SwitchUserStatus;
import android.hardware.automotive.vehicle.V2_0.UserFlags;
import android.hardware.automotive.vehicle.V2_0.UserIdentificationAssociation;
import android.hardware.automotive.vehicle.V2_0.UserIdentificationAssociationSetValue;
import android.hardware.automotive.vehicle.V2_0.UserIdentificationAssociationType;
import android.hardware.automotive.vehicle.V2_0.UserIdentificationAssociationValue;
import android.hardware.automotive.vehicle.V2_0.UserIdentificationGetRequest;
import android.hardware.automotive.vehicle.V2_0.UserIdentificationResponse;
import android.hardware.automotive.vehicle.V2_0.UserIdentificationSetAssociation;
import android.hardware.automotive.vehicle.V2_0.UserIdentificationSetRequest;
import android.hardware.automotive.vehicle.V2_0.UserInfo;
import android.hardware.automotive.vehicle.V2_0.UsersInfo;
import android.hardware.automotive.vehicle.V2_0.VehicleArea;
import android.hardware.automotive.vehicle.V2_0.VehicleDisplay;
import android.hardware.automotive.vehicle.V2_0.VehicleGear;
import android.os.Binder;
import android.os.Build;
import android.os.Process;
import android.os.RemoteException;
import android.os.ShellCommand;
import android.os.SystemClock;
import android.os.UserHandle;
import android.os.UserManager;
import android.text.TextUtils;
import android.util.ArrayMap;
import android.util.IndentingPrintWriter;
import android.util.Slog;
import android.util.SparseArray;
import android.view.KeyEvent;

import com.android.car.am.FixedActivityService;
import com.android.car.audio.CarAudioService;
import com.android.car.evs.CarEvsService;
import com.android.car.garagemode.GarageModeService;
import com.android.car.hal.InputHalService;
import com.android.car.hal.UserHalService;
import com.android.car.hal.VehicleHal;
import com.android.car.pm.CarPackageManagerService;
import com.android.car.power.CarPowerManagementService;
import com.android.car.systeminterface.SystemInterface;
import com.android.car.user.CarUserService;
import com.android.car.watchdog.CarWatchdogService;
import com.android.internal.util.Preconditions;

import java.util.ArrayList;
import java.util.Arrays;
import java.util.Collections;
import java.util.List;
import java.util.Objects;
import java.util.concurrent.CountDownLatch;
import java.util.concurrent.ExecutionException;
import java.util.concurrent.TimeUnit;
import java.util.concurrent.TimeoutException;
import java.util.concurrent.atomic.AtomicBoolean;

final class CarShellCommand extends ShellCommand {

    private static final String NO_INITIAL_USER = "N/A";

    private static final String TAG = CarLog.tagFor(CarShellCommand.class);
    private static final boolean VERBOSE = false;

    private static final String COMMAND_HELP = "-h";
    private static final String COMMAND_DAY_NIGHT_MODE = "day-night-mode";
    private static final String COMMAND_INJECT_VHAL_EVENT = "inject-vhal-event";
    private static final String COMMAND_INJECT_ERROR_EVENT = "inject-error-event";
    private static final String COMMAND_INJECT_CONTINUOUS_EVENT = "inject-continuous-events";
    private static final String COMMAND_ENABLE_UXR = "enable-uxr";
    private static final String COMMAND_GARAGE_MODE = "garage-mode";
    private static final String COMMAND_GET_DO_ACTIVITIES = "get-do-activities";
    private static final String COMMAND_GET_CARPROPERTYCONFIG = "get-carpropertyconfig";
    private static final String COMMAND_GET_PROPERTY_VALUE = "get-property-value";
    private static final String COMMAND_PROJECTION_AP_TETHERING = "projection-tethering";
    private static final String COMMAND_PROJECTION_UI_MODE = "projection-ui-mode";
    private static final String COMMAND_RESUME = "resume";
    private static final String COMMAND_SUSPEND = "suspend";
    private static final String COMMAND_SET_UID_TO_ZONE = "set-audio-zone-for-uid";
    private static final String COMMAND_RESET_VOLUME_CONTEXT = "reset-selected-volume-context";
    private static final String COMMAND_SET_MUTE_CAR_VOLUME_GROUP = "set-mute-car-volume-group";
    private static final String COMMAND_SET_GROUP_VOLUME = "set-group-volume";
    private static final String COMMAND_START_FIXED_ACTIVITY_MODE = "start-fixed-activity-mode";
    private static final String COMMAND_STOP_FIXED_ACTIVITY_MODE = "stop-fixed-activity-mode";
    private static final String COMMAND_ENABLE_FEATURE = "enable-feature";
    private static final String COMMAND_DISABLE_FEATURE = "disable-feature";
    private static final String COMMAND_INJECT_KEY = "inject-key";
    private static final String COMMAND_INJECT_ROTARY = "inject-rotary";
    private static final String COMMAND_INJECT_CUSTOM_INPUT = "inject-custom-input";
    private static final String COMMAND_GET_INITIAL_USER_INFO = "get-initial-user-info";
    private static final String COMMAND_SWITCH_USER = "switch-user";
    private static final String COMMAND_REMOVE_USER = "remove-user";
    private static final String COMMAND_CREATE_USER = "create-user";
    private static final String COMMAND_GET_INITIAL_USER = "get-initial-user";
    private static final String COMMAND_SET_USER_ID_TO_OCCUPANT_ZONE =
            "set-occupant-zone-for-user";
    private static final String COMMAND_RESET_USER_ID_IN_OCCUPANT_ZONE =
            "reset-user-in-occupant-zone";
    private static final String COMMAND_GET_USER_AUTH_ASSOCIATION =
            "get-user-auth-association";
    private static final String COMMAND_SET_USER_AUTH_ASSOCIATION =
            "set-user-auth-association";
    private static final String COMMAND_SET_START_BG_USERS_ON_GARAGE_MODE =
            "set-start-bg-users-on-garage-mode";
    private static final String COMMAND_DEFINE_POWER_POLICY = "define-power-policy";
    private static final String COMMAND_APPLY_POWER_POLICY = "apply-power-policy";
    private static final String COMMAND_DEFINE_POWER_POLICY_GROUP = "define-power-policy-group";
    private static final String COMMAND_SET_POWER_POLICY_GROUP = "set-power-policy-group";
    private static final String COMMAND_POWER_OFF = "power-off";
    private static final String POWER_OFF_SKIP_GARAGEMODE = "--skip-garagemode";
    private static final String POWER_OFF_SHUTDOWN = "--shutdown";
    private static final String COMMAND_SILENT_MODE = "silent-mode";
    // Used with COMMAND_SILENT_MODE for forced silent: "forced-silent"
    private static final String SILENT_MODE_FORCED_SILENT =
            CarPowerManagementService.SILENT_MODE_FORCED_SILENT;
    // Used with COMMAND_SILENT_MODE for forced non silent: "forced-non-silent"
    private static final String SILENT_MODE_FORCED_NON_SILENT =
            CarPowerManagementService.SILENT_MODE_FORCED_NON_SILENT;
    // Used with COMMAND_SILENT_MODE for non forced silent mode: "non-forced-silent-mode"
    private static final String SILENT_MODE_NON_FORCED =
            CarPowerManagementService.SILENT_MODE_NON_FORCED;

    private static final String COMMAND_EMULATE_DRIVING_STATE = "emulate-driving-state";
    private static final String DRIVING_STATE_DRIVE = "drive";
    private static final String DRIVING_STATE_PARK = "park";
    private static final String DRIVING_STATE_REVERSE = "reverse";

    private static final String COMMAND_SET_REARVIEW_CAMERA_ID = "set-rearview-camera-id";
    private static final String COMMAND_GET_REARVIEW_CAMERA_ID = "get-rearview-camera-id";

    private static final String COMMAND_WATCHDOG_IO_SET_3P_FOREGROUND_BYTES =
            "watchdog-io-set-3p-foreground-bytes";
    private static final String COMMAND_WATCHDOG_IO_GET_3P_FOREGROUND_BYTES =
            "watchdog-io-get-3p-foreground-bytes";

    private static final String[] CREATE_OR_MANAGE_USERS_PERMISSIONS = new String[] {
            android.Manifest.permission.CREATE_USERS,
            android.Manifest.permission.MANAGE_USERS
    };

    // List of commands allowed in user build. All these command should be protected with
    // a permission. K: command, V: required permissions (must have at least 1).
    // Only commands with permission already granted to shell user should be allowed.
    // Commands that can affect safety should be never allowed in user build.
    //
    // This map is looked up first, then USER_BUILD_COMMAND_TO_PERMISSION_MAP
    private static final ArrayMap<String, String[]> USER_BUILD_COMMAND_TO_PERMISSIONS_MAP;
    static {
        USER_BUILD_COMMAND_TO_PERMISSIONS_MAP = new ArrayMap<>(7);
        USER_BUILD_COMMAND_TO_PERMISSIONS_MAP.put(COMMAND_GET_INITIAL_USER_INFO,
                CREATE_OR_MANAGE_USERS_PERMISSIONS);
        USER_BUILD_COMMAND_TO_PERMISSIONS_MAP.put(COMMAND_SWITCH_USER,
                CREATE_OR_MANAGE_USERS_PERMISSIONS);
        USER_BUILD_COMMAND_TO_PERMISSIONS_MAP.put(COMMAND_REMOVE_USER,
                CREATE_OR_MANAGE_USERS_PERMISSIONS);
        USER_BUILD_COMMAND_TO_PERMISSIONS_MAP.put(COMMAND_CREATE_USER,
                CREATE_OR_MANAGE_USERS_PERMISSIONS);
        USER_BUILD_COMMAND_TO_PERMISSIONS_MAP.put(COMMAND_GET_USER_AUTH_ASSOCIATION,
                CREATE_OR_MANAGE_USERS_PERMISSIONS);
        USER_BUILD_COMMAND_TO_PERMISSIONS_MAP.put(COMMAND_SET_USER_AUTH_ASSOCIATION,
                CREATE_OR_MANAGE_USERS_PERMISSIONS);
        USER_BUILD_COMMAND_TO_PERMISSIONS_MAP.put(COMMAND_SET_START_BG_USERS_ON_GARAGE_MODE,
                CREATE_OR_MANAGE_USERS_PERMISSIONS);
    }

    // List of commands allowed in user build. All these command should be protected with
    // a permission. K: command, V: required permission.
    // Only commands with permission already granted to shell user should be allowed.
    // Commands that can affect safety should be never allowed in user build.
    private static final ArrayMap<String, String> USER_BUILD_COMMAND_TO_PERMISSION_MAP;
    static {
        USER_BUILD_COMMAND_TO_PERMISSION_MAP = new ArrayMap<>(8);
        USER_BUILD_COMMAND_TO_PERMISSION_MAP.put(COMMAND_GARAGE_MODE,
                android.Manifest.permission.DEVICE_POWER);
        USER_BUILD_COMMAND_TO_PERMISSION_MAP.put(COMMAND_RESUME,
                android.Manifest.permission.DEVICE_POWER);
        USER_BUILD_COMMAND_TO_PERMISSION_MAP.put(COMMAND_SUSPEND,
                android.Manifest.permission.DEVICE_POWER);
        USER_BUILD_COMMAND_TO_PERMISSION_MAP.put(COMMAND_DEFINE_POWER_POLICY,
                android.Manifest.permission.DEVICE_POWER);
        USER_BUILD_COMMAND_TO_PERMISSION_MAP.put(COMMAND_APPLY_POWER_POLICY,
                android.Manifest.permission.DEVICE_POWER);
        USER_BUILD_COMMAND_TO_PERMISSION_MAP.put(COMMAND_DEFINE_POWER_POLICY_GROUP,
                android.Manifest.permission.DEVICE_POWER);
        USER_BUILD_COMMAND_TO_PERMISSION_MAP.put(COMMAND_SET_POWER_POLICY_GROUP,
                android.Manifest.permission.DEVICE_POWER);
        USER_BUILD_COMMAND_TO_PERMISSION_MAP.put(COMMAND_SILENT_MODE,
                PERMISSION_CAR_POWER);
        USER_BUILD_COMMAND_TO_PERMISSION_MAP.put(COMMAND_GET_INITIAL_USER,
                android.Manifest.permission.INTERACT_ACROSS_USERS_FULL);
        USER_BUILD_COMMAND_TO_PERMISSION_MAP.put(COMMAND_DAY_NIGHT_MODE,
                android.Manifest.permission.MODIFY_DAY_NIGHT_MODE);
        USER_BUILD_COMMAND_TO_PERMISSION_MAP.put(COMMAND_RESET_VOLUME_CONTEXT,
                PERMISSION_CAR_CONTROL_AUDIO_VOLUME);
        USER_BUILD_COMMAND_TO_PERMISSION_MAP.put(COMMAND_SET_MUTE_CAR_VOLUME_GROUP,
                PERMISSION_CAR_CONTROL_AUDIO_VOLUME);
        USER_BUILD_COMMAND_TO_PERMISSION_MAP.put(COMMAND_SET_GROUP_VOLUME,
                PERMISSION_CAR_CONTROL_AUDIO_VOLUME);
        USER_BUILD_COMMAND_TO_PERMISSION_MAP.put(COMMAND_INJECT_KEY,
                android.Manifest.permission.INJECT_EVENTS);
        USER_BUILD_COMMAND_TO_PERMISSION_MAP.put(COMMAND_INJECT_ROTARY,
                android.Manifest.permission.INJECT_EVENTS);
        USER_BUILD_COMMAND_TO_PERMISSION_MAP.put(COMMAND_WATCHDOG_IO_SET_3P_FOREGROUND_BYTES,
                PERMISSION_CONTROL_CAR_WATCHDOG_CONFIG);
        USER_BUILD_COMMAND_TO_PERMISSION_MAP.put(COMMAND_WATCHDOG_IO_GET_3P_FOREGROUND_BYTES,
                PERMISSION_CONTROL_CAR_WATCHDOG_CONFIG);
    }

    private static final String PARAM_DAY_MODE = "day";
    private static final String PARAM_NIGHT_MODE = "night";
    private static final String PARAM_SENSOR_MODE = "sensor";
    private static final String PARAM_VEHICLE_PROPERTY_AREA_GLOBAL = "0";
    private static final String PARAM_INJECT_EVENT_DEFAULT_RATE = "10";
    private static final String PARAM_INJECT_EVENT_DEFAULT_DURATION = "60";
    private static final String PARAM_ALL_PROPERTIES_OR_AREA = "-1";
    private static final String PARAM_ON_MODE = "on";
    private static final String PARAM_OFF_MODE = "off";
    private static final String PARAM_QUERY_MODE = "query";
    private static final String PARAM_REBOOT = "reboot";
    private static final String PARAM_MUTE = "mute";
    private static final String PARAM_UNMUTE = "unmute";


    private static final int RESULT_OK = 0;
    private static final int RESULT_ERROR = -1; // Arbitrary value, any non-0 is fine

    private static final int DEFAULT_HAL_TIMEOUT_MS = 1_000;

    private static final int DEFAULT_CAR_USER_SERVICE_TIMEOUT_MS = 60_000;

    private static final int INVALID_USER_AUTH_TYPE_OR_VALUE = -1;

    private static final SparseArray<String> VALID_USER_AUTH_TYPES;
    private static final String VALID_USER_AUTH_TYPES_HELP;

    private static final SparseArray<String> VALID_USER_AUTH_SET_VALUES;
    private static final String VALID_USER_AUTH_SET_VALUES_HELP;

    private static final ArrayMap<String, Integer> CUSTOM_INPUT_FUNCTION_ARGS;

    static {
        VALID_USER_AUTH_TYPES = new SparseArray<>(5);
        VALID_USER_AUTH_TYPES.put(KEY_FOB, UserIdentificationAssociationType.toString(KEY_FOB));
        VALID_USER_AUTH_TYPES.put(CUSTOM_1, UserIdentificationAssociationType.toString(CUSTOM_1));
        VALID_USER_AUTH_TYPES.put(CUSTOM_2, UserIdentificationAssociationType.toString(CUSTOM_2));
        VALID_USER_AUTH_TYPES.put(CUSTOM_3, UserIdentificationAssociationType.toString(CUSTOM_3));
        VALID_USER_AUTH_TYPES.put(CUSTOM_4, UserIdentificationAssociationType.toString(CUSTOM_4));
        VALID_USER_AUTH_TYPES_HELP = getHelpString("types", VALID_USER_AUTH_TYPES);

        VALID_USER_AUTH_SET_VALUES = new SparseArray<>(3);
        VALID_USER_AUTH_SET_VALUES.put(ASSOCIATE_CURRENT_USER,
                UserIdentificationAssociationSetValue.toString(ASSOCIATE_CURRENT_USER));
        VALID_USER_AUTH_SET_VALUES.put(DISASSOCIATE_CURRENT_USER,
                UserIdentificationAssociationSetValue.toString(DISASSOCIATE_CURRENT_USER));
        VALID_USER_AUTH_SET_VALUES.put(DISASSOCIATE_ALL_USERS,
                UserIdentificationAssociationSetValue.toString(DISASSOCIATE_ALL_USERS));
        VALID_USER_AUTH_SET_VALUES_HELP = getHelpString("values", VALID_USER_AUTH_SET_VALUES);

        CUSTOM_INPUT_FUNCTION_ARGS = new ArrayMap<>(10);
        CUSTOM_INPUT_FUNCTION_ARGS.put("f1", CustomInputEvent.INPUT_CODE_F1);
        CUSTOM_INPUT_FUNCTION_ARGS.put("f2", CustomInputEvent.INPUT_CODE_F2);
        CUSTOM_INPUT_FUNCTION_ARGS.put("f3", CustomInputEvent.INPUT_CODE_F3);
        CUSTOM_INPUT_FUNCTION_ARGS.put("f4", CustomInputEvent.INPUT_CODE_F4);
        CUSTOM_INPUT_FUNCTION_ARGS.put("f5", CustomInputEvent.INPUT_CODE_F5);
        CUSTOM_INPUT_FUNCTION_ARGS.put("f6", CustomInputEvent.INPUT_CODE_F6);
        CUSTOM_INPUT_FUNCTION_ARGS.put("f7", CustomInputEvent.INPUT_CODE_F7);
        CUSTOM_INPUT_FUNCTION_ARGS.put("f8", CustomInputEvent.INPUT_CODE_F8);
        CUSTOM_INPUT_FUNCTION_ARGS.put("f9", CustomInputEvent.INPUT_CODE_F9);
        CUSTOM_INPUT_FUNCTION_ARGS.put("f10", CustomInputEvent.INPUT_CODE_F10);
    }

    @NonNull
    private static String getHelpString(@NonNull String name, @NonNull SparseArray<String> values) {
        StringBuilder help = new StringBuilder("Valid ").append(name).append(" are: ");
        int size = values.size();
        for (int i = 0; i < size; i++) {
            help.append(values.valueAt(i));
            if (i != size - 1) {
                help.append(", ");
            }
        }
        return help.append('.').toString();
    }

    private final Context mContext;
    private final VehicleHal mHal;
    private final CarAudioService mCarAudioService;
    private final CarPackageManagerService mCarPackageManagerService;
    private final CarProjectionService mCarProjectionService;
    private final CarPowerManagementService mCarPowerManagementService;
    private final FixedActivityService mFixedActivityService;
    private final CarFeatureController mFeatureController;
    private final CarInputService mCarInputService;
    private final CarNightService mCarNightService;
    private final SystemInterface mSystemInterface;
    private final GarageModeService mGarageModeService;
    private final CarUserService mCarUserService;
    private final CarOccupantZoneService mCarOccupantZoneService;
    private final CarEvsService mCarEvsService;
    private final CarWatchdogService mCarWatchdogService;
    private long mKeyDownTime;

    CarShellCommand(Context context,
            VehicleHal hal,
            CarAudioService carAudioService,
            CarPackageManagerService carPackageManagerService,
            CarProjectionService carProjectionService,
            CarPowerManagementService carPowerManagementService,
            FixedActivityService fixedActivityService,
            CarFeatureController featureController,
            CarInputService carInputService,
            CarNightService carNightService,
            SystemInterface systemInterface,
            GarageModeService garageModeService,
            CarUserService carUserService,
            CarOccupantZoneService carOccupantZoneService,
            CarEvsService carEvsService,
            CarWatchdogService carWatchdogService) {
        mContext = context;
        mHal = hal;
        mCarAudioService = carAudioService;
        mCarPackageManagerService = carPackageManagerService;
        mCarProjectionService = carProjectionService;
        mCarPowerManagementService = carPowerManagementService;
        mFixedActivityService = fixedActivityService;
        mFeatureController = featureController;
        mCarInputService = carInputService;
        mCarNightService = carNightService;
        mSystemInterface = systemInterface;
        mGarageModeService = garageModeService;
        mCarUserService = carUserService;
        mCarOccupantZoneService = carOccupantZoneService;
        mCarEvsService = carEvsService;
        mCarWatchdogService = carWatchdogService;
    }

    @Override
    public int onCommand(String cmd) {
        if (cmd == null) {
            onHelp();
            return RESULT_ERROR;
        }
        ArrayList<String> argsList = new ArrayList<>();
        argsList.add(cmd);
        String arg = null;
        do {
            arg = getNextArg();
            if (arg != null) {
                argsList.add(arg);
            }
        } while (arg != null);
        String[] args = new String[argsList.size()];
        argsList.toArray(args);
        try (IndentingPrintWriter pw = new IndentingPrintWriter(getOutPrintWriter())) {
            return exec(args, pw);
        }
    }

    @Override
    public void onHelp() {
        try (IndentingPrintWriter pw = new IndentingPrintWriter(getOutPrintWriter())) {
            showHelp(pw);
        }
    }

    private static void showHelp(IndentingPrintWriter pw) {
        pw.println("Car service commands:");
        pw.println("\t-h");
        pw.println("\t  Print this help text.");
        pw.println("\tday-night-mode [day|night|sensor]");
        pw.println("\t  Force into day/night mode or restore to auto.");
        pw.println("\tinject-vhal-event <PROPERTY_ID in Hex or Decimal> [zone] "
                + "data(can be comma separated list) "
                + "[-t delay_time_seconds]");
        pw.println("\t  Inject a vehicle property for testing.");
        pw.println("\t  delay_time_seconds: the event timestamp is increased by certain second.");
        pw.println("\t  If not specified, it will be 0.");
        pw.println("\tinject-error-event <PROPERTY_ID in Hex or Decimal> zone <errorCode>");
        pw.println("\t  Inject an error event from VHAL for testing.");
        pw.println("\tinject-continuous-events <PROPERTY_ID in Hex or Decimal> "
                + "data(can be comma separated list) "
                + "[-z zone]  [-s SampleRate in Hz] [-d time duration in seconds]");
        pw.println("\t  Inject continuous vehicle events for testing.");
        pw.printf("\t  If not specified, CarService will inject fake events with areaId:%s "
                        + "at sample rate %s for %s seconds.", PARAM_VEHICLE_PROPERTY_AREA_GLOBAL,
                PARAM_INJECT_EVENT_DEFAULT_RATE, PARAM_INJECT_EVENT_DEFAULT_DURATION);
        pw.println("\tenable-uxr true|false");
        pw.println("\t  Enable/Disable UX restrictions and App blocking.");
        pw.println("\tgarage-mode [on|off|query|reboot]");
        pw.println("\t  Force into or out of garage mode, or check status.");
        pw.println("\t  With 'reboot', enter garage mode, then reboot when it completes.");
        pw.println("\tget-do-activities pkgname");
        pw.println("\t  Get Distraction Optimized activities in given package.");
        pw.println("\tget-carpropertyconfig [PROPERTY_ID in Hex or Decimal]");
        pw.println("\t  Get a CarPropertyConfig by Id or list all CarPropertyConfigs");
        pw.println("\tget-property-value [PROPERTY_ID in Hex or Decimal] [areaId]");
        pw.println("\t  Get a vehicle property value by property id and areaId");
        pw.println("\t  or list all property values for all areaId");
        pw.println("\tsuspend");
        pw.println("\t  Suspend the system to Deep Sleep.");
        pw.println("\tresume");
        pw.println("\t  Wake the system up after a 'suspend.'");
        pw.println("\tprojection-tethering [true|false]");
        pw.println("\t  Whether tethering should be used when creating access point for"
                + " wireless projection");
        pw.println("\t--metrics");
        pw.println("\t  When used with dumpsys, only metrics will be in the dumpsys output.");
        pw.printf("\t%s [zoneid] [uid]\n", COMMAND_SET_UID_TO_ZONE);
        pw.println("\t  Maps the audio zoneid to uid.");
        pw.printf("\t%s\n", COMMAND_RESET_VOLUME_CONTEXT);
        pw.println("\t  Resets the last selected volume context for volume changes.");
        pw.printf("\t%s [zoneId] [groupId] [%s\\%s]\n", COMMAND_SET_MUTE_CAR_VOLUME_GROUP,
                PARAM_MUTE, PARAM_UNMUTE);
        pw.printf("\t  %s\\%s groupId in zoneId\n", PARAM_MUTE, PARAM_UNMUTE);
        pw.printf("\t%s [zoneId] [groupId] [volume]\n", COMMAND_SET_GROUP_VOLUME);
        pw.println("\t  sets the group volume for [groupId] in [zoneId] to %volume,");
        pw.println("\t  [volume] must be an integer between 0 to 100");
        pw.println("\tstart-fixed-activity displayId packageName activityName");
        pw.println("\t  Start an Activity the specified display as fixed mode");
        pw.println("\tstop-fixed-mode displayId");
        pw.println("\t  Stop fixed Activity mode for the given display. "
                + "The Activity will not be restarted upon crash.");
        pw.println("\tenable-feature featureName");
        pw.println("\t  Enable the requested feature. Change will happen after reboot.");
        pw.println("\t  This requires root/su.");
        pw.println("\tdisable-feature featureName");
        pw.println("\t  Disable the requested feature. Change will happen after reboot");
        pw.println("\t  This requires root/su.");
        pw.println("\tinject-key [-d display] [-t down_delay_ms | -a down|up] key_code");
        pw.println("\t  inject key down and/or up event to car service");
        pw.println("\t  display: 0 for main, 1 for cluster. If not specified, it will be 0.");
        pw.println("\t  down_delay_ms: delay from down to up key event. If not specified,");
        pw.println("\t                 it will be 0");
        pw.println("\t  key_code: int key code defined in android KeyEvent");
        pw.println("\t  If -a isn't specified, both down and up will be injected.");
        pw.println("\tinject-rotary [-d display] [-i input_type] [-c clockwise]");
        pw.println("\t              [-dt delta_times_ms]");
        pw.println("\t  inject rotary input event to car service.");
        pw.println("\t  display: 0 for main, 1 for cluster. If not specified, it will be 0.");
        pw.println("\t  input_type: 10 for navigation controller input, 11 for volume");
        pw.println("\t              controller input. If not specified, it will be 10.");
        pw.println("\t  clockwise: true if the event is clockwise, false if the event is");
        pw.println("\t             counter-clockwise. If not specified, it will be false.");
        pw.println("\t  delta_times_ms: a list of delta time (current time minus event time)");
        pw.println("\t                  in descending order. If not specified, it will be 0.");
        pw.println("\tinject-custom-input [-d display] [-r repeatCounter] EVENT");
        pw.println("\t  display: 0 for main, 1 for cluster. If not specified, it will be 0.");
        pw.println("\t  repeatCounter: number of times the button was hit (default value is 1)");
        pw.println("\t  EVENT: mandatory last argument. Possible values for for this flag are ");
        pw.println("\t         F1, F2, up to F10 (functions to defined by OEM partners)");
        pw.printf("\t%s <REQ_TYPE> [--timeout TIMEOUT_MS]\n", COMMAND_GET_INITIAL_USER_INFO);
        pw.println("\t  Calls the Vehicle HAL to get the initial boot info, passing the given");
        pw.println("\t  REQ_TYPE (which could be either FIRST_BOOT, FIRST_BOOT_AFTER_OTA, ");
        pw.println("\t  COLD_BOOT, RESUME, or any numeric value that would be passed 'as-is')");
        pw.println("\t  and an optional TIMEOUT_MS to wait for the HAL response (if not set,");
        pw.println("\t  it will use a  default value).");
        pw.println("\t  The --hal-only option only calls HAL, without using CarUserService.");

        pw.printf("\t%s <USER_ID> [--hal-only] [--timeout TIMEOUT_MS]\n", COMMAND_SWITCH_USER);
        pw.println("\t  Switches to user USER_ID using the HAL integration.");
        pw.println("\t  The --hal-only option only calls HAL, without switching the user,");
        pw.println("\t  while the --timeout defines how long to wait for the response.");

        pw.printf("\t%s <USER_ID> [--hal-only]\n", COMMAND_REMOVE_USER);
        pw.println("\t  Removes user with USER_ID using the HAL integration.");
        pw.println("\t  The --hal-only option only calls HAL, without removing the user,");

        pw.printf("\t%s [--hal-only] [--timeout TIMEOUT_MS] [--type TYPE] [--flags FLAGS] [NAME]\n",
                COMMAND_CREATE_USER);
        pw.println("\t  Creates a new user using the HAL integration.");
        pw.println("\t  The --hal-only uses UserManager to create the user,");
        pw.println("\t  while the --timeout defines how long to wait for the response.");

        pw.printf("\t%s\n", COMMAND_GET_INITIAL_USER);
        pw.printf("\t  Gets the id of the initial user (or %s when it's not available)\n",
                NO_INITIAL_USER);

        pw.printf("\t%s [occupantZoneId] [userId]\n", COMMAND_SET_USER_ID_TO_OCCUPANT_ZONE);
        pw.println("\t  Maps the occupant zone id to user id.");
        pw.printf("\t%s [occupantZoneId]\n", COMMAND_RESET_USER_ID_IN_OCCUPANT_ZONE);
        pw.println("\t  Unmaps the user assigned to occupant zone id.");

        pw.printf("\t%s [--hal-only] [--user USER_ID] TYPE1 [..TYPE_N]\n",
                COMMAND_GET_USER_AUTH_ASSOCIATION);
        pw.println("\t  Gets the N user authentication values for the N types for the given user");
        pw.println("\t  (or current user when not specified).");
        pw.println("\t  By defautt it calls CarUserManager, but using --hal-only will call just "
                + "UserHalService.");

        pw.printf("\t%s [--hal-only] [--user USER_ID] TYPE1 VALUE1 [..TYPE_N VALUE_N]\n",
                COMMAND_SET_USER_AUTH_ASSOCIATION);
        pw.println("\t  Sets the N user authentication types with the N values for the given user");
        pw.println("\t  (or current user when not specified).");
        pw.println("\t  By default it calls CarUserManager, but using --hal-only will call just "
                + "UserHalService.");
        pw.printf("\t  %s\n", VALID_USER_AUTH_TYPES_HELP);
        pw.printf("\t  %s\n", VALID_USER_AUTH_SET_VALUES_HELP);

        pw.printf("\t%s [true|false]\n", COMMAND_SET_START_BG_USERS_ON_GARAGE_MODE);
        pw.println("\t  Controls backgroud user start and stop during garage mode.");
        pw.println("\t  If false, garage mode operations (background users start at garage mode"
                + " entry and background users stop at garage mode exit) will be skipped.");

        pw.printf("\t  %s [%s|%s|%s|%s]\n", COMMAND_SILENT_MODE, SILENT_MODE_FORCED_SILENT,
                SILENT_MODE_FORCED_NON_SILENT, SILENT_MODE_NON_FORCED, PARAM_QUERY_MODE);
        pw.println("\t  Forces silent mode silent or non-silent. With query (or no command) "
                + "displays the silent state");
        pw.println("\t  and shows how many listeners are monitoring the state.");

        pw.printf("\t%s [%s|%s|%s]\n", COMMAND_EMULATE_DRIVING_STATE, DRIVING_STATE_DRIVE,
                DRIVING_STATE_PARK, DRIVING_STATE_REVERSE);
        pw.println("\t  Emulates the giving driving state.");

        pw.printf("\t%s <POLICY_ID> [--enable COMP1,COMP2,...] [--disable COMP1,COMP2,...]\n",
                COMMAND_DEFINE_POWER_POLICY);
        pw.println("\t  Defines a power policy. Components not specified in --enable or --disable");
        pw.println("\t  are unchanged when the policy is applied.");
        pw.println("\t  Components should be comma-separated without space.");

        pw.printf("\t%s <POLICY_ID>\n", COMMAND_APPLY_POWER_POLICY);
        pw.println("\t  Applies power policy which is defined in /vendor/etc/power_policy.xml or");
        pw.printf("\t  by %s command\n", COMMAND_DEFINE_POWER_POLICY);

        pw.printf("\t%s <POLICY_GROUP_ID> [%s:<POLICY_ID>] [%s:<POLICY_ID>]\n",
                COMMAND_DEFINE_POWER_POLICY_GROUP, POWER_STATE_WAIT_FOR_VHAL, POWER_STATE_ON);
        pw.println("\t  Defines a power policy group. The policy ID must be defined in advance.");

        pw.printf("\t%s <POLICY_GROUP_ID>\n", COMMAND_SET_POWER_POLICY_GROUP);
        pw.println("\t  Sets power policy group which is defined in /vendor/etc/power_policy.xml ");
        pw.printf("\t  or by %s command\n", COMMAND_DEFINE_POWER_POLICY_GROUP);

        pw.printf("\t%s [%s] [%s]\n", COMMAND_POWER_OFF, POWER_OFF_SKIP_GARAGEMODE,
                POWER_OFF_SHUTDOWN);
        pw.println("\t  Powers off the car.");

        pw.printf("\t%s <CAMERA_ID>\n", COMMAND_SET_REARVIEW_CAMERA_ID);
        pw.println("\t  Configures a target camera device CarEvsService to use.");
        pw.println("\t  If CAMEAR_ID is \"default\", this command will configure CarEvsService ");
        pw.println("\t  to use its default camera device.");

        pw.printf("\t%s\n", COMMAND_GET_REARVIEW_CAMERA_ID);
        pw.println("\t  Gets the name of the camera device CarEvsService is using for " +
                "the rearview.");

        pw.printf("\t%s <FOREGROUND_MODE_BYTES>\n", COMMAND_WATCHDOG_IO_SET_3P_FOREGROUND_BYTES);
        pw.println("\t  Sets third-party apps foreground I/O overuse threshold");

        pw.printf("\t%s\n", COMMAND_WATCHDOG_IO_GET_3P_FOREGROUND_BYTES);
        pw.println("\t  Gets third-party apps foreground I/O overuse threshold");
    }

    private static int showInvalidArguments(IndentingPrintWriter pw) {
        pw.println("Incorrect number of arguments.");
        showHelp(pw);
        return RESULT_ERROR;
    }

    private void runSetZoneIdForUid(String zoneString, String uidString) {
        int uid = Integer.parseInt(uidString);
        int zoneId = Integer.parseInt(zoneString);
        mCarAudioService.setZoneIdForUid(zoneId, uid);
    }

    private void runSetMuteCarVolumeGroup(String zoneString, String groupIdString,
            String muteString) {
        int groupId = Integer.parseInt(groupIdString);
        int zoneId = Integer.parseInt(zoneString);
        if (!PARAM_MUTE.equalsIgnoreCase(muteString)
                && !PARAM_UNMUTE.equalsIgnoreCase(muteString)) {
            throw new IllegalArgumentException("Failed to set volume group mute for "
                    + groupIdString + " in zone " + zoneString
                    + ", bad mute argument: " + muteString);
        }
        boolean muteState = PARAM_MUTE.equalsIgnoreCase(muteString);
        mCarAudioService.setVolumeGroupMute(zoneId, groupId, muteState, FLAG_SHOW_UI);
    }


    private void runSetGroupVolume(String zoneIdString, String groupIdString, String volumeString) {
        int groupId = Integer.parseInt(groupIdString);
        int zoneId = Integer.parseInt(zoneIdString);
        int percentVolume = Integer.parseInt(volumeString);
        Preconditions.checkArgumentInRange(percentVolume, 0, 100,
                "%volume for group " + groupIdString + " in zone " + zoneIdString);
        int minIndex = mCarAudioService.getGroupMinVolume(zoneId, groupId);
        int maxIndex = mCarAudioService.getGroupMaxVolume(zoneId, groupId);
        int index = minIndex
                + (int) ((float) (maxIndex - minIndex) * ((float) percentVolume / 100.0f));
        mCarAudioService.setGroupVolume(zoneId, groupId, index, FLAG_SHOW_UI);
    }

    private void runResetSelectedVolumeContext() {
        mCarAudioService.resetSelectedVolumeContext();
    }

    private void runSetOccupantZoneIdForUserId(String occupantZoneIdString,
            String userIdString) {
        int userId = Integer.parseInt(userIdString);
        int occupantZoneId = Integer.parseInt(occupantZoneIdString);
        if (!mCarOccupantZoneService.assignProfileUserToOccupantZone(occupantZoneId, userId)) {
            throw new IllegalStateException("Failed to set userId " + userId + " to occupantZoneId "
                    + occupantZoneIdString);
        }
    }

    private void runResetOccupantZoneId(String occupantZoneIdString) {
        int occupantZoneId = Integer.parseInt(occupantZoneIdString);
        if (!mCarOccupantZoneService
                .assignProfileUserToOccupantZone(occupantZoneId, UserHandle.USER_NULL)) {
            throw new IllegalStateException("Failed to reset occupantZoneId "
                    + occupantZoneIdString);
        }
    }

    private void assertHasAtLeastOnePermission(String cmd, String[] requiredPermissions) {
        for (String requiredPermission : requiredPermissions) {
            if (ICarImpl.hasPermission(mContext, requiredPermission)) return;
        }
        if (requiredPermissions.length == 1) {
            throw new SecurityException("The command '" + cmd + "' requires permission:"
                    + requiredPermissions[0]);
        }
        throw new SecurityException(
                "The command " + cmd + " requires one of the following permissions:"
                        + Arrays.toString(requiredPermissions));
    }

    int exec(String[] args, IndentingPrintWriter writer) {
        String cmd = args[0];
        String[] requiredPermissions = USER_BUILD_COMMAND_TO_PERMISSIONS_MAP.get(cmd);
        if (requiredPermissions == null) {
            String requiredPermission = USER_BUILD_COMMAND_TO_PERMISSION_MAP.get(cmd);
            if (requiredPermission != null) {
                requiredPermissions = new String[] { requiredPermission };
            }

        }
        if (VERBOSE) {
            Slog.v(TAG, "cmd: " + cmd + ", requiredPermissions: "
                    + Arrays.toString(requiredPermissions));
        }
        if (Build.IS_USER && requiredPermissions == null) {
            throw new SecurityException("The command '" + cmd + "' requires non-user build");
        }
        if (requiredPermissions != null) {
            assertHasAtLeastOnePermission(cmd, requiredPermissions);
        }

        switch (cmd) {
            case COMMAND_HELP:
                showHelp(writer);
                break;
            case COMMAND_DAY_NIGHT_MODE: {
                String value = args.length < 2 ? "" : args[1];
                forceDayNightMode(value, writer);
                break;
            }
            case COMMAND_GARAGE_MODE: {
                String value = args.length < 2 ? "" : args[1];
                forceGarageMode(value, writer);
                break;
            }
            case COMMAND_INJECT_VHAL_EVENT:
                String zone = PARAM_VEHICLE_PROPERTY_AREA_GLOBAL;
                String data;
                int argNum = args.length;
                if (argNum < 3 || argNum > 6) {
                    return showInvalidArguments(writer);
                }
                String delayTime = args[argNum - 2].equals("-t") ?  args[argNum - 1] : "0";
                if (argNum == 4 || argNum == 6) {
                    // Zoned
                    zone = args[2];
                    data = args[3];
                } else {
                    // Global
                    data = args[2];
                }
                injectVhalEvent(args[1], zone, data, false, delayTime, writer);
                break;
            case COMMAND_INJECT_CONTINUOUS_EVENT:
                injectContinuousEvents(args, writer);
                break;
            case COMMAND_INJECT_ERROR_EVENT:
                if (args.length != 4) {
                    return showInvalidArguments(writer);
                }
                String errorAreaId = args[2];
                String errorCode = args[3];
                injectVhalEvent(args[1], errorAreaId, errorCode, true, "0", writer);
                break;
            case COMMAND_ENABLE_UXR:
                if (args.length != 2) {
                    return showInvalidArguments(writer);
                }
                boolean enableBlocking = Boolean.valueOf(args[1]);
                if (mCarPackageManagerService != null) {
                    mCarPackageManagerService.setEnableActivityBlocking(enableBlocking);
                }
                break;
            case COMMAND_GET_DO_ACTIVITIES:
                if (args.length != 2) {
                    return showInvalidArguments(writer);
                }
                String pkgName = args[1].toLowerCase();
                if (mCarPackageManagerService != null) {
                    String[] doActivities =
                            mCarPackageManagerService.getDistractionOptimizedActivities(
                                    pkgName);
                    if (doActivities != null) {
                        writer.println("DO Activities for " + pkgName);
                        for (String a : doActivities) {
                            writer.println(a);
                        }
                    } else {
                        writer.println("No DO Activities for " + pkgName);
                    }
                }
                break;
            case COMMAND_GET_CARPROPERTYCONFIG:
                String propertyId = args.length < 2 ? PARAM_ALL_PROPERTIES_OR_AREA : args[1];
                mHal.dumpPropertyConfigs(writer, Integer.decode(propertyId));
                break;
            case COMMAND_GET_PROPERTY_VALUE:
                String propId = args.length < 2 ? PARAM_ALL_PROPERTIES_OR_AREA : args[1];
                String areaId = args.length < 3 ? PARAM_ALL_PROPERTIES_OR_AREA : args[2];
                mHal.dumpPropertyValueByCommend(writer, Integer.decode(propId),
                        Integer.decode(areaId));
                break;
            case COMMAND_PROJECTION_UI_MODE:
                if (args.length != 2) {
                    return showInvalidArguments(writer);
                }
                mCarProjectionService.setUiMode(Integer.valueOf(args[1]));
                break;
            case COMMAND_PROJECTION_AP_TETHERING:
                if (args.length != 2) {
                    return showInvalidArguments(writer);
                }
                mCarProjectionService.setAccessPointTethering(Boolean.valueOf(args[1]));
                break;
            case COMMAND_RESUME:
                mCarPowerManagementService.forceSimulatedResume();
                writer.println("Resume: Simulating resuming from Deep Sleep");
                break;
            case COMMAND_SUSPEND:
                mCarPowerManagementService.forceSuspendAndMaybeReboot(false);
                writer.println("Suspend: Simulating powering down to Deep Sleep");
                break;
            case COMMAND_SET_UID_TO_ZONE:
                if (args.length != 3) {
                    return showInvalidArguments(writer);
                }
                runSetZoneIdForUid(args[1], args[2]);
                break;
            case COMMAND_RESET_VOLUME_CONTEXT:
                if (args.length > 1) {
                    return showInvalidArguments(writer);
                }
                runResetSelectedVolumeContext();
                break;
            case COMMAND_SET_MUTE_CAR_VOLUME_GROUP:
                if (args.length != 4) {
                    return showInvalidArguments(writer);
                }
                runSetMuteCarVolumeGroup(args[1], args[2], args[3]);
                break;
            case COMMAND_SET_GROUP_VOLUME:
                if (args.length != 4) {
                    return showInvalidArguments(writer);
                }
                runSetGroupVolume(args[1], args[2], args[3]);
                break;
            case COMMAND_SET_USER_ID_TO_OCCUPANT_ZONE:
                if (args.length != 3) {
                    return showInvalidArguments(writer);
                }
                runSetOccupantZoneIdForUserId(args[1], args[2]);
                break;
            case COMMAND_SILENT_MODE: {
                String value = args.length < 2 ? ""
                        : args.length == 2 ? args[1] : "too many arguments";
                runSilentCommand(value, writer);
                break;
            }
            case COMMAND_RESET_USER_ID_IN_OCCUPANT_ZONE:
                if (args.length != 2) {
                    return showInvalidArguments(writer);
                }
                runResetOccupantZoneId(args[1]);
                break;
            case COMMAND_START_FIXED_ACTIVITY_MODE:
                startFixedActivity(args, writer);
                break;
            case COMMAND_STOP_FIXED_ACTIVITY_MODE:
                stopFixedMode(args, writer);
                break;
            case COMMAND_ENABLE_FEATURE:
                if (args.length != 2) {
                    return showInvalidArguments(writer);
                }
                enableDisableFeature(args, writer, /* enable= */ true);
                break;
            case COMMAND_DISABLE_FEATURE:
                if (args.length != 2) {
                    return showInvalidArguments(writer);
                }
                enableDisableFeature(args, writer, /* enable= */ false);
                break;
            case COMMAND_INJECT_KEY:
                if (args.length < 2) {
                    return showInvalidArguments(writer);
                }
                injectKey(args, writer);
                break;
            case COMMAND_INJECT_ROTARY:
                if (args.length < 1) {
                    return showInvalidArguments(writer);
                }
                injectRotary(args, writer);
                break;
            case COMMAND_INJECT_CUSTOM_INPUT:
                if (args.length < 2) {
                    return showInvalidArguments(writer);
                }
                injectCustomInputEvent(args, writer);
                break;
            case COMMAND_GET_INITIAL_USER_INFO:
                getInitialUserInfo(args, writer);
                break;
            case COMMAND_SWITCH_USER:
                switchUser(args, writer);
                break;
            case COMMAND_REMOVE_USER:
                removeUser(args, writer);
                break;
            case COMMAND_CREATE_USER:
                createUser(args, writer);
                break;
            case COMMAND_GET_INITIAL_USER:
                getInitialUser(writer);
                break;
            case COMMAND_GET_USER_AUTH_ASSOCIATION:
                getUserAuthAssociation(args, writer);
                break;
            case COMMAND_SET_USER_AUTH_ASSOCIATION:
                setUserAuthAssociation(args, writer);
                break;
            case COMMAND_SET_START_BG_USERS_ON_GARAGE_MODE:
                setStartBackgroundUsersOnGarageMode(args, writer);
                break;
            case COMMAND_EMULATE_DRIVING_STATE:
                emulateDrivingState(args, writer);
                break;
            case COMMAND_DEFINE_POWER_POLICY:
                return definePowerPolicy(args, writer);
            case COMMAND_APPLY_POWER_POLICY:
                return applyPowerPolicy(args, writer);
            case COMMAND_DEFINE_POWER_POLICY_GROUP:
                return definePowerPolicyGroup(args, writer);
            case COMMAND_SET_POWER_POLICY_GROUP:
                return setPowerPolicyGroup(args, writer);
            case COMMAND_POWER_OFF:
                powerOff(args, writer);
                break;
            case COMMAND_SET_REARVIEW_CAMERA_ID:
                setRearviewCameraId(args, writer);
                break;
            case COMMAND_GET_REARVIEW_CAMERA_ID:
                getRearviewCameraId(writer);
                break;
            case COMMAND_WATCHDOG_IO_SET_3P_FOREGROUND_BYTES:
                setWatchdogIoThirdPartyForegroundBytes(args, writer);
                break;
            case COMMAND_WATCHDOG_IO_GET_3P_FOREGROUND_BYTES:
                getWatchdogIoThirdPartyForegroundBytes(writer);
                break;

            default:
                writer.println("Unknown command: \"" + cmd + "\"");
                showHelp(writer);
                return RESULT_ERROR;
        }
        return RESULT_OK;
    }

    private void setStartBackgroundUsersOnGarageMode(String[] args, IndentingPrintWriter writer) {
        if (args.length < 2) {
            writer.println("Insufficient number of args");
            return;
        }

        boolean enabled = Boolean.parseBoolean(args[1]);
        Slog.d(TAG, "setStartBackgroundUsersOnGarageMode(): " + (enabled ? "enabled" : "disabled"));
        mCarUserService.setStartBackgroundUsersOnGarageMode(enabled);
        writer.printf("StartBackgroundUsersOnGarageMode set to %b\n", enabled);
    }

    private void startFixedActivity(String[] args, IndentingPrintWriter writer) {
        if (args.length != 4) {
            writer.println("Incorrect number of arguments");
            showHelp(writer);
            return;
        }
        int displayId;
        try {
            displayId = Integer.parseInt(args[1]);
        } catch (NumberFormatException e) {
            writer.println("Wrong display id:" + args[1]);
            return;
        }
        String packageName = args[2];
        String activityName = args[3];
        Intent intent = new Intent();
        intent.setComponent(new ComponentName(packageName, activityName));
        intent.addFlags(Intent.FLAG_ACTIVITY_NEW_TASK | Intent.FLAG_ACTIVITY_CLEAR_TASK);
        ActivityOptions options = ActivityOptions.makeBasic();
        options.setLaunchDisplayId(displayId);
        if (!mFixedActivityService.startFixedActivityModeForDisplayAndUser(intent, options,
                displayId, ActivityManager.getCurrentUser())) {
            writer.println("Failed to start");
            return;
        }
        writer.println("Succeeded");
    }

    private void stopFixedMode(String[] args, IndentingPrintWriter writer) {
        if (args.length != 2) {
            writer.println("Incorrect number of arguments");
            showHelp(writer);
            return;
        }
        int displayId;
        try {
            displayId = Integer.parseInt(args[1]);
        } catch (NumberFormatException e) {
            writer.println("Wrong display id:" + args[1]);
            return;
        }
        mFixedActivityService.stopFixedActivityMode(displayId);
    }

    private void enableDisableFeature(String[] args, IndentingPrintWriter writer, boolean enable) {
        if (Binder.getCallingUid() != Process.ROOT_UID) {
            writer.println("Only allowed to root/su");
            return;
        }
        String featureName = args[1];
        long id = Binder.clearCallingIdentity();
        // no permission check here
        int r;
        if (enable) {
            r = mFeatureController.enableFeature(featureName);
        } else {
            r = mFeatureController.disableFeature(featureName);
        }
        switch (r) {
            case Car.FEATURE_REQUEST_SUCCESS:
                if (enable) {
                    writer.println("Enabled feature:" + featureName);
                } else {
                    writer.println("Disabled feature:" + featureName);
                }
                break;
            case Car.FEATURE_REQUEST_ALREADY_IN_THE_STATE:
                if (enable) {
                    writer.println("Already enabled:" + featureName);
                } else {
                    writer.println("Already disabled:" + featureName);
                }
                break;
            case Car.FEATURE_REQUEST_MANDATORY:
                writer.println("Cannot change mandatory feature:" + featureName);
                break;
            case Car.FEATURE_REQUEST_NOT_EXISTING:
                writer.println("Non-existing feature:" + featureName);
                break;
            default:
                writer.println("Unknown error:" + r);
                break;
        }
        Binder.restoreCallingIdentity(id);
    }

    private void injectKey(String[] args, IndentingPrintWriter writer) {
        int i = 1; // 0 is command itself
        int display = CarOccupantZoneManager.DISPLAY_TYPE_MAIN;
        int delayMs = 0;
        int keyCode = KeyEvent.KEYCODE_UNKNOWN;
        int action = -1;
        try {
            while (i < args.length) {
                switch (args[i]) {
                    case "-d":
                        i++;
                        int vehicleDisplay = Integer.parseInt(args[i]);
                        if (!checkVehicleDisplay(vehicleDisplay, writer)) {
                            return;
                        }
                        display = InputHalService.convertDisplayType(vehicleDisplay);
                        break;
                    case "-t":
                        i++;
                        delayMs = Integer.parseInt(args[i]);
                        break;
                    case "-a":
                        i++;
                        if (args[i].equalsIgnoreCase("down")) {
                            action = KeyEvent.ACTION_DOWN;
                        } else if (args[i].equalsIgnoreCase("up")) {
                            action = KeyEvent.ACTION_UP;
                        } else {
                            throw new IllegalArgumentException("Invalid action: " + args[i]);
                        }
                        break;
                    default:
                        if (keyCode != KeyEvent.KEYCODE_UNKNOWN) {
                            throw new IllegalArgumentException("key_code already set:"
                                    + keyCode);
                        }
                        keyCode = Integer.parseInt(args[i]);
                }
                i++;
            }
        } catch (NumberFormatException e) {
            writer.println("Invalid args:" + e);
            showHelp(writer);
            return;
        }
        if (keyCode == KeyEvent.KEYCODE_UNKNOWN) {
            writer.println("Missing key code or invalid keycode");
            showHelp(writer);
            return;
        }
        if (delayMs < 0) {
            writer.println("Invalid delay:" + delayMs);
            showHelp(writer);

            return;
        }
        if (action == -1) {
            injectKeyEvent(KeyEvent.ACTION_DOWN, keyCode, display);
            SystemClock.sleep(delayMs);
            injectKeyEvent(KeyEvent.ACTION_UP, keyCode, display);
        } else {
            injectKeyEvent(action, keyCode, display);
        }
        writer.println("Succeeded");
    }

    private void injectKeyEvent(int action, int keyCode, int display) {
        long currentTime = SystemClock.uptimeMillis();
        if (action == KeyEvent.ACTION_DOWN) mKeyDownTime = currentTime;
        long token = Binder.clearCallingIdentity();
        try {
            mCarInputService.injectKeyEvent(
                    new KeyEvent(/* downTime= */ mKeyDownTime, /* eventTime= */ currentTime,
                            action, keyCode, /* repeat= */ 0), display);
        } finally {
            Binder.restoreCallingIdentity(token);
        }
    }

    private void injectRotary(String[] args, IndentingPrintWriter writer) {
        int i = 1; // 0 is command itself
        int display = CarOccupantZoneManager.DISPLAY_TYPE_MAIN;
        int inputType = CarInputManager.INPUT_TYPE_ROTARY_NAVIGATION;
        boolean clockwise = false;
        List<Long> deltaTimeMs = new ArrayList<>();
        try {
            while (i < args.length) {
                switch (args[i]) {
                    case "-d":
                        i++;
                        int vehicleDisplay = Integer.parseInt(args[i]);
                        if (!checkVehicleDisplay(vehicleDisplay, writer)) {
                            return;
                        }
                        display = InputHalService.convertDisplayType(vehicleDisplay);
                        break;
                    case "-i":
                        i++;
                        inputType = Integer.parseInt(args[i]);
                        break;
                    case "-c":
                        i++;
                        clockwise = Boolean.parseBoolean(args[i]);
                        break;
                    case "-dt":
                        i++;
                        while (i < args.length) {
                            deltaTimeMs.add(Long.parseLong(args[i]));
                            i++;
                        }
                        break;
                    default:
                        writer.println("Invalid option at index " + i + ": " + args[i]);
                        return;
                }
                i++;
            }
        } catch (NumberFormatException e) {
            writer.println("Invalid args:" + e);
            showHelp(writer);
            return;
        }
        if (deltaTimeMs.isEmpty()) {
            deltaTimeMs.add(0L);
        }
        for (int j = 0; j < deltaTimeMs.size(); j++) {
            if (deltaTimeMs.get(j) < 0) {
                writer.println("Delta time shouldn't be negative: " + deltaTimeMs.get(j));
                showHelp(writer);
                return;
            }
            if (j > 0 && deltaTimeMs.get(j) > deltaTimeMs.get(j - 1)) {
                writer.println("Delta times should be in descending order");
                showHelp(writer);
                return;
            }
        }
        long[] uptimeMs = new long[deltaTimeMs.size()];
        long currentUptime = SystemClock.uptimeMillis();
        for (int j = 0; j < deltaTimeMs.size(); j++) {
            uptimeMs[j] = currentUptime - deltaTimeMs.get(j);
        }
        RotaryEvent rotaryEvent = new RotaryEvent(inputType, clockwise, uptimeMs);
        mCarInputService.onRotaryEvent(rotaryEvent, display);
        writer.println("Succeeded in injecting: " + rotaryEvent);
    }

    private void injectCustomInputEvent(String[] args, IndentingPrintWriter writer) {
        int display = CarOccupantZoneManager.DISPLAY_TYPE_MAIN;
        int repeatCounter = 1;

        int argIdx = 1;
        for (; argIdx < args.length - 1; argIdx++) {
            switch (args[argIdx]) {
                case "-d":
                    int vehicleDisplay = Integer.parseInt(args[++argIdx]);
                    if (!checkVehicleDisplay(vehicleDisplay, writer)) {
                        return;
                    }
                    display = InputHalService.convertDisplayType(vehicleDisplay);
                    break;
                case "-r":
                    repeatCounter = Integer.parseInt(args[++argIdx]);
                    break;
                default:
                    writer.printf("Unrecognized argument: {%s}\n", args[argIdx]);
                    writer.println("Pass -help to see the full list of options");
                    return;
            }
        }

        if (argIdx == args.length) {
            writer.println("Last mandatory argument (fn) not passed.");
            writer.println("Pass -help to see the full list of options");
            return;
        }

        // Processing the last remaining argument (expected to be 'f1', 'f2', ..., 'f10').
        String eventValue = args[argIdx].toLowerCase();
        Integer inputCode = CUSTOM_INPUT_FUNCTION_ARGS.get(eventValue);
        if (inputCode == null) {
            writer.printf("Invalid input event value {%s}, valid values are f1, f2, ..., f10\n",
                    eventValue);
            writer.println("Pass -help to see the full list of options");
            return;
        }

        CustomInputEvent event = new CustomInputEvent(inputCode, display, repeatCounter);
        mCarInputService.onCustomInputEvent(event);
        writer.printf("Succeeded in injecting {%s}\n", event);
    }

    private boolean checkVehicleDisplay(int vehicleDisplay, IndentingPrintWriter writer) {
        if (vehicleDisplay != VehicleDisplay.MAIN
                && vehicleDisplay != VehicleDisplay.INSTRUMENT_CLUSTER) {
            writer.println("Invalid display:" + vehicleDisplay);
            showHelp(writer);
            return false;
        }
        return true;
    }

    private void getInitialUserInfo(String[] args, IndentingPrintWriter writer) {
        if (args.length < 2) {
            writer.println("Insufficient number of args");
            return;
        }

        // Gets the request type
        String typeArg = args[1];
        int requestType = UserHalHelper.parseInitialUserInfoRequestType(typeArg);

        int timeout = DEFAULT_HAL_TIMEOUT_MS;
        for (int i = 2; i < args.length; i++) {
            String arg = args[i];
            switch (arg) {
                case "--timeout":
                    timeout = Integer.parseInt(args[++i]);
                    break;
                default:
                    writer.println("Invalid option at index " + i + ": " + arg);
                    return;

            }
        }

        Slog.d(TAG, "handleGetInitialUserInfo(): type=" + requestType + " (" + typeArg
                + "), timeout=" + timeout);

        CountDownLatch latch = new CountDownLatch(1);
        HalCallback<InitialUserInfoResponse> callback = (status, resp) -> {
            try {
                Slog.d(TAG, "GetUserInfoResponse: status=" + status + ", resp=" + resp);
                writer.printf("Call status: %s\n",
                        UserHalHelper.halCallbackStatusToString(status));
                if (status != HalCallback.STATUS_OK) {
                    return;
                }
                writer.printf("Request id: %d\n", resp.requestId);
                writer.printf("Action: %s\n",
                        InitialUserInfoResponseAction.toString(resp.action));
                if (!TextUtils.isEmpty(resp.userNameToCreate)) {
                    writer.printf("User name: %s\n", resp.userNameToCreate);
                }
                if (resp.userToSwitchOrCreate.userId != UserHandle.USER_NULL) {
                    writer.printf("User id: %d\n", resp.userToSwitchOrCreate.userId);
                }
                if (resp.userToSwitchOrCreate.flags != UserFlags.NONE) {
                    writer.printf("User flags: %s\n",
                            UserHalHelper.userFlagsToString(resp.userToSwitchOrCreate.flags));
                }
                if (!TextUtils.isEmpty(resp.userLocales)) {
                    writer.printf("User locales: %s\n", resp.userLocales);
                }
            } finally {
                latch.countDown();
            }
        };
        UsersInfo usersInfo = generateUsersInfo();
        mHal.getUserHal().getInitialUserInfo(requestType, timeout, usersInfo, callback);
        waitForHal(writer, latch, timeout);
    }

    private UsersInfo generateUsersInfo() {
        return UserHalHelper.newUsersInfo(UserManager.get(mContext));
    }

    private int getUserHalFlags(@UserIdInt int userId) {
        return UserHalHelper.getFlags(UserManager.get(mContext), userId);
    }

    private static void waitForHal(IndentingPrintWriter writer, CountDownLatch latch,
            int timeoutMs) {
        try {
            if (!latch.await(timeoutMs, TimeUnit.MILLISECONDS)) {
                writer.printf("HAL didn't respond in %dms\n", timeoutMs);
            }
        } catch (InterruptedException e) {
            Thread.currentThread().interrupt();
            writer.println("Interrupted waiting for HAL");
        }
        return;
    }

    private void switchUser(String[] args, IndentingPrintWriter writer) {
        if (args.length < 2) {
            writer.println("Insufficient number of args");
            return;
        }

        int targetUserId = Integer.parseInt(args[1]);
        int timeout = DEFAULT_HAL_TIMEOUT_MS + DEFAULT_CAR_USER_SERVICE_TIMEOUT_MS;
        boolean halOnly = false;

        for (int i = 2; i < args.length; i++) {
            String arg = args[i];
            switch (arg) {
                case "--timeout":
                    timeout = Integer.parseInt(args[++i]);
                    break;
                case "--hal-only":
                    halOnly = true;
                    break;
                default:
                    writer.println("Invalid option at index " + i + ": " + arg);
                    return;
            }
        }

        Slog.d(TAG, "switchUser(): target=" + targetUserId + ", halOnly=" + halOnly
                + ", timeout=" + timeout);

        if (halOnly) {
            CountDownLatch latch = new CountDownLatch(1);
            UserHalService userHal = mHal.getUserHal();
            UserInfo targetUserInfo = new UserInfo();
            targetUserInfo.userId = targetUserId;
            targetUserInfo.flags = getUserHalFlags(targetUserId);

            SwitchUserRequest request = new SwitchUserRequest();
            request.targetUser = targetUserInfo;
            request.usersInfo = generateUsersInfo();

            userHal.switchUser(request, timeout, (status, resp) -> {
                try {
                    Slog.d(TAG, "SwitchUserResponse: status=" + status + ", resp=" + resp);
                    writer.printf("Call Status: %s\n",
                            UserHalHelper.halCallbackStatusToString(status));
                    if (status != HalCallback.STATUS_OK) {
                        return;
                    }
                    writer.printf("Request id: %d\n", resp.requestId);
                    writer.printf("Message type: %s\n",
                            SwitchUserMessageType.toString(resp.messageType));
                    writer.printf("Switch Status: %s\n", SwitchUserStatus.toString(resp.status));
                    String errorMessage = resp.errorMessage;
                    if (!TextUtils.isEmpty(errorMessage)) {
                        writer.printf("Error message: %s", errorMessage);
                    }
                    // If HAL returned OK, make a "post-switch" call to the HAL indicating an
                    // Android error. This is to "rollback" the HAL switch.
                    if (status == HalCallback.STATUS_OK
                            && resp.status == SwitchUserStatus.SUCCESS) {
                        userHal.postSwitchResponse(request);
                    }
                } finally {
                    latch.countDown();
                }
            });
            waitForHal(writer, latch, timeout);
            return;
        }
        CarUserManager carUserManager = getCarUserManager(mContext);
        AsyncFuture<UserSwitchResult> future = carUserManager.switchUser(targetUserId);
        UserSwitchResult result = waitForFuture(writer, future, timeout);
        if (result == null) return;
        writer.printf("UserSwitchResult: status=%s",
                UserSwitchResult.statusToString(result.getStatus()));
        String msg = result.getErrorMessage();
        if (!TextUtils.isEmpty(msg)) {
            writer.printf(", errorMessage=%s", msg);
        }
        writer.println();
    }

    private void createUser(String[] args, IndentingPrintWriter writer) {
        int timeout = DEFAULT_HAL_TIMEOUT_MS + DEFAULT_CAR_USER_SERVICE_TIMEOUT_MS;
        int flags = 0;
        boolean halOnly = false;
        String name = null;
        String userType = null;

        for (int i = 1; i < args.length; i++) {
            String arg = args[i];
            switch (arg) {
                case "--timeout":
                    timeout = Integer.parseInt(args[++i]);
                    break;
                case "--hal-only":
                    halOnly = true;
                    break;
                case "--flags":
                    flags = Integer.parseInt(args[++i]);
                    break;
                case "--type":
                    userType = args[++i];
                    break;
                default:
                    if (name != null) {
                        writer.println("Invalid option at index " + i + ": " + arg);
                        return;
                    }
                    name = arg;
            }
        }

        if (userType == null) {
            userType = android.content.pm.UserInfo.getDefaultUserType(flags);
        }

        Slog.d(TAG, "createUser(): name=" + name + ", userType=" + userType
                + ", flags=" + android.content.pm.UserInfo.flagsToString(flags)
                + ", halOnly=" + halOnly + ", timeout=" + timeout);

        if (!halOnly) {
            CarUserManager carUserManager = getCarUserManager(mContext);
            AsyncFuture<UserCreationResult> future = carUserManager
                    .createUser(name, userType, flags);

            UserCreationResult result = waitForFuture(writer, future, timeout);
            if (result == null) return;

            android.content.pm.UserInfo user = result.getUser();
            writer.printf("UserCreationResult: status=%s, user=%s",
                    UserCreationResult.statusToString(result.getStatus()),
                    user == null ? "N/A" : user.toFullString());
            String msg = result.getErrorMessage();
            if (!TextUtils.isEmpty(msg)) {
                writer.printf(", errorMessage=%s", msg);
            }
            writer.println();
            return;
        }

        CountDownLatch latch = new CountDownLatch(1);
        UserHalService userHal = mHal.getUserHal();

        CreateUserRequest request = new CreateUserRequest();

        UserManager um = UserManager.get(mContext);
        android.content.pm.UserInfo newUser = um.createUser(name, userType, flags);
        if (newUser == null) {
            writer.printf("Failed to create user");
            return;
        }
        writer.printf("New user: %s\n", newUser.toFullString());
        Slog.i(TAG, "Created new user: " + newUser.toFullString());

        request.newUserInfo.userId = newUser.id;
        request.newUserInfo.flags = UserHalHelper.convertFlags(newUser);

        request.usersInfo = generateUsersInfo();

        AtomicBoolean halOk = new AtomicBoolean(false);
        try {
            userHal.createUser(request, timeout, (status, resp) -> {
                Slog.d(TAG, "CreateUserResponse: status=" + status + ", resp=" + resp);
                writer.printf("Call Status: %s\n",
                        UserHalHelper.halCallbackStatusToString(status));
                if (status == HalCallback.STATUS_OK) {
                    halOk.set(resp.status == CreateUserStatus.SUCCESS);
                    writer.printf("Request id: %d\n", resp.requestId);
                    writer.printf("Create Status: %s\n", CreateUserStatus.toString(resp.status));
                    String errorMessage = resp.errorMessage;
                    if (!TextUtils.isEmpty(errorMessage)) {
                        writer.printf("Error message: %s", errorMessage);
                    }
                }
                latch.countDown();
            });
            waitForHal(writer, latch, timeout);
        } catch (RuntimeException e) {
            writer.printf("HAL failed: %s\n", e);
        } finally {
            if (!halOk.get()) {
                writer.printf("Removing user %d due to HAL failure\n", newUser.id);
                boolean removed = um.removeUser(newUser.id);
                writer.printf("User removed: %b\n", removed);
            }
        }
    }

    private void removeUser(String[] args, IndentingPrintWriter writer) {
        if (args.length < 2) {
            writer.println("Insufficient number of args");
            return;
        }

        int userId = Integer.parseInt(args[1]);
        boolean halOnly = false;

        for (int i = 2; i < args.length; i++) {
            String arg = args[i];
            switch (arg) {
                case "--hal-only":
                    halOnly = true;
                    break;
                default:
                    writer.println("Invalid option at index " + i + ": " + arg);
                    return;
            }
        }

        Slog.d(TAG, "handleRemoveUser(): User to remove=" + userId + ", halOnly=" + halOnly);

        if (halOnly) {
            UserHalService userHal = mHal.getUserHal();
            UsersInfo usersInfo = generateUsersInfo();
            UserInfo userInfo = new UserInfo();
            userInfo.userId = userId;
            userInfo.flags = getUserHalFlags(userId);

            RemoveUserRequest request = new RemoveUserRequest();
            request.removedUserInfo = userInfo;
            request.usersInfo = usersInfo;

            userHal.removeUser(request);
            writer.printf("User removal sent for HAL only.\n");
            return;
        }

        CarUserManager carUserManager = getCarUserManager(mContext);
        UserRemovalResult result = carUserManager.removeUser(userId);
        writer.printf("UserRemovalResult: status = %s\n",
                UserRemovalResult.statusToString(result.getStatus()));
    }

    private static <T> T waitForFuture(@NonNull IndentingPrintWriter writer,
            @NonNull AsyncFuture<T> future, int timeoutMs) {
        T result = null;
        try {
            result = future.get(timeoutMs, TimeUnit.MILLISECONDS);
            if (result == null) {
                writer.printf("Service didn't respond in %d ms", timeoutMs);
            }
        } catch (InterruptedException e) {
            Thread.currentThread().interrupt();
        } catch (ExecutionException | TimeoutException e) {
            writer.printf("Exception getting future: %s",  e);
        }
        return result;
    }

    private void getInitialUser(IndentingPrintWriter writer) {
        android.content.pm.UserInfo user = mCarUserService.getInitialUser();
        writer.println(user == null ? NO_INITIAL_USER : user.id);
    }

    private void getUserAuthAssociation(String[] args, IndentingPrintWriter writer) {
        if (args.length < 2) {
            writer.println("invalid usage, must pass at least 1 argument");
            return;
        }

        boolean halOnly = false;
        int userId = UserHandle.USER_CURRENT;

        UserIdentificationGetRequest request = new UserIdentificationGetRequest();
        for (int i = 1; i < args.length; i++) {
            String arg = args[i];
            switch (arg) {
                case "--user":
                    try {
                        userId = Integer.parseInt(args[++i]);
                    } catch (NumberFormatException e) {
                        writer.printf("Invalid user id at index %d (from %s): %s\n", i + 1,
                                Arrays.toString(args), arg);
                    }
                    break;
                case "--hal-only":
                    halOnly = true;
                    break;
                default:
                    int type = parseAuthArg(VALID_USER_AUTH_TYPES, arg);
                    if (type == INVALID_USER_AUTH_TYPE_OR_VALUE) {
                        writer.printf("Invalid type at index %d (from %s): %s. %s\n", i + 1,
                                Arrays.toString(args), arg, VALID_USER_AUTH_TYPES_HELP);
                        return;
                    }
                    request.associationTypes.add(type);
            }

        }
        if (userId == UserHandle.USER_CURRENT) {
            userId = ActivityManager.getCurrentUser();
        }
        int requestSize = request.associationTypes.size();
        if (halOnly) {
            request.numberAssociationTypes = requestSize;
            request.userInfo.userId = userId;
            request.userInfo.flags = getUserHalFlags(userId);

            Slog.d(TAG, "getUserAuthAssociation(): user=" + userId + ", halOnly=" + halOnly
                    + ", request=" + request);
            UserIdentificationResponse response = mHal.getUserHal().getUserAssociation(request);
            Slog.d(TAG, "getUserAuthAssociation(): response=" + response);
            showResponse(writer, response);
            return;
        }

        CarUserManager carUserManager = getCarUserManager(writer, userId);
        int[] types = new int[requestSize];
        for (int i = 0; i < requestSize; i++) {
            types[i] = request.associationTypes.get(i);
        }
        UserIdentificationAssociationResponse response = carUserManager
                .getUserIdentificationAssociation(types);
        showResponse(writer, response);
    }

    private CarUserManager getCarUserManager(@NonNull IndentingPrintWriter writer,
            @UserIdInt int userId) {
        Context context;
        if (userId == mContext.getUserId()) {
            context = mContext;
        } else {
            context = mContext.createContextAsUser(UserHandle.of(userId), /* flags= */ 0);
        }
        int actualUserId = Binder.getCallingUid();
        if (actualUserId != userId) {
            writer.printf("Emulating call for user id %d, but caller's user id is %d, so that's "
                    + "what CarUserService will use when calling HAL.\n", userId, actualUserId);
        }

        return getCarUserManager(context);
    }

    private CarUserManager getCarUserManager(@NonNull Context context) {
        Car car = Car.createCar(context);
        CarUserManager carUserManager = (CarUserManager) car.getCarManager(Car.CAR_USER_SERVICE);
        return carUserManager;
    }

    private void showResponse(@NonNull IndentingPrintWriter writer,
            @NonNull UserIdentificationResponse response) {
        if (response == null) {
            writer.println("null response");
            return;
        }

        if (!TextUtils.isEmpty(response.errorMessage)) {
            writer.printf("Error message: %s\n", response.errorMessage);
        }
        int numberAssociations = response.associations.size();
        writer.printf("%d associations:\n", numberAssociations);
        for (int i = 0; i < numberAssociations; i++) {
            UserIdentificationAssociation association = response.associations.get(i);
            writer.printf("  %s\n", association);
        }
    }

    private void showResponse(@NonNull IndentingPrintWriter writer,
            @NonNull UserIdentificationAssociationResponse response) {
        if (response == null) {
            writer.println("null response");
            return;
        }
        if (!response.isSuccess()) {
            writer.printf("failed response: %s\n", response);
            return;
        }
        String errorMessage = response.getErrorMessage();
        if (!TextUtils.isEmpty(errorMessage)) {
            writer.printf("Error message: %s\n", errorMessage);
        }
        int[] values = response.getValues();
        if (values == null) {
            writer.printf("no associations on %s\n", response);
            return;
        }
        writer.printf("%d associations:\n", values.length);
        for (int i = 0; i < values.length; i++) {
            writer.printf("  %s\n", UserIdentificationAssociationValue.toString(values[i]));
        }
    }

    private void setUserAuthAssociation(String[] args, IndentingPrintWriter writer) {
        if (args.length < 3) {
            writer.println("invalid usage, must pass at least 4 arguments");
            return;
        }

        boolean halOnly = false;
        int timeout = DEFAULT_HAL_TIMEOUT_MS;
        int userId = UserHandle.USER_CURRENT;

        UserIdentificationSetRequest request = new UserIdentificationSetRequest();
        for (int i = 1; i < args.length; i++) {
            String arg = args[i];
            switch (arg) {
                case "--user":
                    try {
                        userId = Integer.parseInt(args[++i]);
                    } catch (NumberFormatException e) {
                        writer.printf("Invalid user id at index %d (from %s): %s\n", i + 1,
                                Arrays.toString(args), arg);
                    }
                    break;
                case "--hal-only":
                    halOnly = true;
                    break;
                case "--timeout":
                    timeout = Integer.parseInt(args[++i]);
                    break;
                default:
                    UserIdentificationSetAssociation association =
                            new UserIdentificationSetAssociation();
                    association.type = parseAuthArg(VALID_USER_AUTH_TYPES, arg);
                    if (association.type == INVALID_USER_AUTH_TYPE_OR_VALUE) {
                        writer.printf("Invalid type at index %d (from %s): %s. %s\n", i + 1,
                                Arrays.toString(args), arg, VALID_USER_AUTH_TYPES_HELP);
                        return;
                    }
                    association.value = parseAuthArg(VALID_USER_AUTH_SET_VALUES, args[++i]);
                    if (association.value == INVALID_USER_AUTH_TYPE_OR_VALUE) {
                        writer.printf("Invalid value at index %d (from %s): %s. %s\n", i + 1,
                                Arrays.toString(args), arg, VALID_USER_AUTH_SET_VALUES_HELP);
                        return;
                    }
                    request.associations.add(association);
            }

        }
        if (userId == UserHandle.USER_CURRENT) {
            userId = ActivityManager.getCurrentUser();
        }
        int requestSize = request.associations.size();
        if (halOnly) {
            request.numberAssociations = requestSize;
            request.userInfo.userId = userId;
            request.userInfo.flags = getUserHalFlags(userId);

            Slog.d(TAG, "setUserAuthAssociation(): user=" + userId + ", halOnly=" + halOnly
                    + ", request=" + request);
            CountDownLatch latch = new CountDownLatch(1);
            mHal.getUserHal().setUserAssociation(timeout, request, (status, response) -> {
                Slog.d(TAG, "setUserAuthAssociation(): response=" + response);
                try {
                    showResponse(writer, response);
                } finally {
                    latch.countDown();
                }
            });
            waitForHal(writer, latch, timeout);
            return;
        }
        CarUserManager carUserManager = getCarUserManager(writer, userId);
        int[] types = new int[requestSize];
        int[] values = new int[requestSize];
        for (int i = 0; i < requestSize; i++) {
            UserIdentificationSetAssociation association = request.associations.get(i);
            types[i] = association.type;
            values[i] = association.value;
        }
        AsyncFuture<UserIdentificationAssociationResponse> future = carUserManager
                .setUserIdentificationAssociation(types, values);
        UserIdentificationAssociationResponse response = waitForFuture(writer, future, timeout);
        if (response != null) {
            showResponse(writer, response);
        }
    }

    private static int parseAuthArg(@NonNull SparseArray<String> types, @NonNull String type) {
        for (int i = 0; i < types.size(); i++) {
            if (types.valueAt(i).equals(type)) {
                return types.keyAt(i);
            }
        }
        return INVALID_USER_AUTH_TYPE_OR_VALUE;
    }

    private void forceDayNightMode(String arg, IndentingPrintWriter writer) {
        int mode;
        switch (arg) {
            case PARAM_DAY_MODE:
                mode = CarNightService.FORCED_DAY_MODE;
                break;
            case PARAM_NIGHT_MODE:
                mode = CarNightService.FORCED_NIGHT_MODE;
                break;
            case PARAM_SENSOR_MODE:
                mode = CarNightService.FORCED_SENSOR_MODE;
                break;
            default:
                writer.printf("Unknown value: %s. Valid argument: %s|%s|%s\n",
                        arg, PARAM_DAY_MODE, PARAM_NIGHT_MODE, PARAM_SENSOR_MODE);
                return;
        }
        int current = mCarNightService.forceDayNightMode(mode);
        String currentMode = null;
        switch (current) {
            case UiModeManager.MODE_NIGHT_AUTO:
                currentMode = PARAM_SENSOR_MODE;
                break;
            case UiModeManager.MODE_NIGHT_YES:
                currentMode = PARAM_NIGHT_MODE;
                break;
            case UiModeManager.MODE_NIGHT_NO:
                currentMode = PARAM_DAY_MODE;
                break;
        }
        writer.println("DayNightMode changed to: " + currentMode);
    }

    private void forceGarageMode(String arg, IndentingPrintWriter writer) {
        switch (arg) {
            case PARAM_ON_MODE:
                mSystemInterface.setDisplayState(false);
                mGarageModeService.forceStartGarageMode();
                writer.println("Garage mode: " + mGarageModeService.isGarageModeActive());
                break;
            case PARAM_OFF_MODE:
                mSystemInterface.setDisplayState(true);
                mGarageModeService.stopAndResetGarageMode();
                writer.println("Garage mode: " + mGarageModeService.isGarageModeActive());
                break;
            case PARAM_QUERY_MODE:
                mGarageModeService.dump(writer);
                break;
            case PARAM_REBOOT:
                mCarPowerManagementService.forceSuspendAndMaybeReboot(true);
                writer.println("Entering Garage Mode. Will reboot when it completes.");
                break;
            default:
                writer.printf("Unknown value: %s. Valid argument: %s|%s|%s|%s\n",
                        arg, PARAM_ON_MODE, PARAM_OFF_MODE, PARAM_QUERY_MODE, PARAM_REBOOT);
        }
    }

    private void runSilentCommand(String arg, IndentingPrintWriter writer) {
        switch (arg) {
            case SILENT_MODE_FORCED_SILENT:
                writer.println("Forcing silent mode to silent");
                mCarPowerManagementService.setSilentMode(SILENT_MODE_FORCED_SILENT);
                break;
            case SILENT_MODE_FORCED_NON_SILENT:
                writer.println("Forcing silent mode to non-silent");
                mCarPowerManagementService.setSilentMode(SILENT_MODE_FORCED_NON_SILENT);
                break;
            case SILENT_MODE_NON_FORCED:
                writer.println("Not forcing silent mode");
                mCarPowerManagementService.setSilentMode(SILENT_MODE_NON_FORCED);
                break;
            case PARAM_QUERY_MODE:
                mCarPowerManagementService.dumpSilentMode(writer);
                break;
            default:
                writer.printf("Unknown value: %s. Valid argument: %s|%s|%s|%s\n", arg,
                        SILENT_MODE_FORCED_SILENT, SILENT_MODE_FORCED_NON_SILENT,
                        SILENT_MODE_NON_FORCED, PARAM_QUERY_MODE);
        }
    }

    private void emulateDrivingState(String[] args, IndentingPrintWriter writer) {
        if (args.length != 2) {
            writer.println("invalid usage, must pass driving state");
            return;
        }
        String mode = args[1];
        switch (mode) {
            case DRIVING_STATE_DRIVE:
                emulateDrive();
                break;
            case DRIVING_STATE_PARK:
                emulatePark();
                break;
            case DRIVING_STATE_REVERSE:
                emulateReverse();
                break;
            default:
                writer.printf("invalid driving mode %s; must be %s or %s\n", mode,
                        DRIVING_STATE_DRIVE, DRIVING_STATE_PARK);
        }
    }

    /**
     * Emulates driving mode. Called by
     * {@code adb shell cmd car_service emulate-driving-state drive}.
     */
    private void emulateDrive() {
        Slog.i(TAG, "Emulating driving mode (speed=80mph, gear=8)");
        mHal.injectVhalEvent(VehiclePropertyIds.PERF_VEHICLE_SPEED,
                /* zone= */ 0, /* value= */ "80", /* delayTime= */ 2000);
        mHal.injectVhalEvent(VehiclePropertyIds.GEAR_SELECTION,
                /* zone= */ 0, Integer.toString(VehicleGear.GEAR_8), /* delayTime= */ 0);
        mHal.injectVhalEvent(VehiclePropertyIds.PARKING_BRAKE_ON,
                /* zone= */ 0, /* value= */ "false", /* delayTime= */ 0);
    }

    /**
     * Emulates reverse driving mode. Called by
     * {@code adb shell cmd car_service emulate-driving-state reverse}.
     */
    private void emulateReverse() {
        Slog.i(TAG, "Emulating reverse driving mode (speed=5mph)");
        mHal.injectVhalEvent(VehiclePropertyIds.PERF_VEHICLE_SPEED,
                /* zone= */ 0, /* value= */ "5", /* delayTime= */ 2000);
        mHal.injectVhalEvent(VehiclePropertyIds.GEAR_SELECTION,
                /* zone= */ 0, Integer.toString(VehicleGear.GEAR_REVERSE), /* delayTime= */ 0);
        mHal.injectVhalEvent(VehiclePropertyIds.PARKING_BRAKE_ON,
                /* zone= */ 0, /* value= */ "false", /* delayTime= */ 0);
    }

    /**
     * Emulates parking mode. Called by
     * {@code adb shell cmd car_service emulate-driving-state park}.
     */
    private void emulatePark() {
        Slog.i(TAG, "Emulating parking mode");
        mHal.injectVhalEvent(VehiclePropertyIds.PERF_VEHICLE_SPEED,
                /* zone= */ 0, /* value= */ "0", /* delayTime= */ 0);
        mHal.injectVhalEvent(VehiclePropertyIds.GEAR_SELECTION,
                /* zone= */ 0, Integer.toString(VehicleGear.GEAR_PARK), /* delayTime= */ 0);
    }

    private int definePowerPolicy(String[] args, IndentingPrintWriter writer) {
        boolean result = mCarPowerManagementService.definePowerPolicyFromCommand(args, writer);
        if (result) return RESULT_OK;
        writer.printf("\nUsage: cmd car_service %s <POLICY_ID> [--enable COMP1,COMP2,...] "
                + "[--disable COMP1,COMP2,...]\n", COMMAND_DEFINE_POWER_POLICY);
        return RESULT_ERROR;
    }

    private int applyPowerPolicy(String[] args, IndentingPrintWriter writer) {
        boolean result = mCarPowerManagementService.applyPowerPolicyFromCommand(args, writer);
        if (result) return RESULT_OK;
        writer.printf("\nUsage: cmd car_service %s <POLICY_ID>\n", COMMAND_APPLY_POWER_POLICY);
        return RESULT_ERROR;
    }

    private int definePowerPolicyGroup(String[] args, IndentingPrintWriter writer) {
        boolean result = mCarPowerManagementService.definePowerPolicyGroupFromCommand(args, writer);
        if (result) return RESULT_OK;
        writer.printf("\nUsage: cmd car_service %s <POLICY_GROUP_ID> [%s:<POLICY_ID>] "
                + "[%s:<POLICY_ID>]\n", COMMAND_DEFINE_POWER_POLICY_GROUP,
                POWER_STATE_WAIT_FOR_VHAL, POWER_STATE_ON);
        return RESULT_ERROR;
    }

    private int setPowerPolicyGroup(String[] args, IndentingPrintWriter writer) {
        boolean result = mCarPowerManagementService.setPowerPolicyGroupFromCommand(args, writer);
        if (result) return RESULT_OK;
        writer.printf("\nUsage: cmd car_service %s <POLICY_GROUP_ID>\n",
                COMMAND_SET_POWER_POLICY_GROUP);
        return RESULT_ERROR;
    }

    private void powerOff(String[] args, IndentingPrintWriter writer) {
        int index = 1;
        boolean skipGarageMode = false;
        boolean shutdown = false;
        while (index < args.length) {
            switch (args[index]) {
                case POWER_OFF_SKIP_GARAGEMODE:
                    skipGarageMode = true;
                    break;
                case POWER_OFF_SHUTDOWN:
                    shutdown = true;
                    break;
                default:
                    writer.printf("Not supported option: %s\n", args[index]);
                    return;
            }
            index++;
        }
        mCarPowerManagementService.powerOffFromCommand(skipGarageMode, shutdown);
    }

    /**
     * Inject a fake  VHAL event
     *
     * @param property the Vehicle property Id as defined in the HAL
     * @param zone     Zone that this event services
     * @param isErrorEvent indicates the type of event
     * @param value    Data value of the event
     * @param delayTime the event timestamp is increased by delayTime
     * @param writer   IndentingPrintWriter
     */
    private void injectVhalEvent(String property, String zone, String value,
            boolean isErrorEvent, String delayTime, IndentingPrintWriter writer) {
        Slog.i(TAG, "Injecting VHAL event: prop="  + property + ", zone=" + zone + ", value="
                + value + ", isError=" + isErrorEvent
                + (TextUtils.isEmpty(delayTime) ?  "" : ", delayTime=" + delayTime));
        if (zone.equalsIgnoreCase(PARAM_VEHICLE_PROPERTY_AREA_GLOBAL)) {
            if (!isPropertyAreaTypeGlobal(property)) {
                writer.printf("Property area type inconsistent with given zone: %s \n", zone);
                return;
            }
        }
        try {
            if (isErrorEvent) {
                mHal.onPropertySetError(Integer.decode(value), Integer.decode(property),
                        Integer.decode(zone));
            } else {
                mHal.injectVhalEvent(Integer.decode(property), Integer.decode(zone), value,
                        Integer.decode(delayTime));
            }
        } catch (NumberFormatException e) {
            writer.printf("Invalid property Id zone Id or value: %s \n", e);
            showHelp(writer);
        }
    }

    // Inject continuous vhal events.
    private void injectContinuousEvents(String[] args, IndentingPrintWriter writer) {
        if (args.length < 3 || args.length > 8) {
            showInvalidArguments(writer);
            return;
        }
        String areaId = PARAM_VEHICLE_PROPERTY_AREA_GLOBAL;
        String sampleRate = PARAM_INJECT_EVENT_DEFAULT_RATE;
        String durationTime = PARAM_INJECT_EVENT_DEFAULT_DURATION;
        String propId = args[1];
        String data = args[2];
        // scan input
        for (int i = 3; i < args.length - 1; i++) {
            switch (args[i]) {
                case "-d":
                    durationTime = args[++i];
                    break;
                case "-z" :
                    areaId = args[++i];
                    break;
                case "-s" :
                    sampleRate = args[++i];
                    break;
                default:
                    writer.printf("%s is an invalid flag.\n", args[i]);
                    showHelp(writer);
                    return;
            }
        }
        try {
            float sampleRateFloat = Float.parseFloat(sampleRate);
            if (sampleRateFloat <= 0) {
                writer.printf("SampleRate: %s is an invalid value. "
                        + "SampleRate must be greater than 0.\n", sampleRate);
                showHelp(writer);
                return;
            }
            mHal.injectContinuousVhalEvent(Integer.decode(propId),
                    Integer.decode(areaId), data,
                    sampleRateFloat, Long.parseLong(durationTime));
        } catch (NumberFormatException e) {
            writer.printf("Invalid arguments: %s\n", e);
            showHelp(writer);
        }

    }

    // Set a target camera device for the rearview
    private void setRearviewCameraId(String[] args, IndentingPrintWriter writer) {
        if (args.length != 2) {
            showInvalidArguments(writer);
            return;
        }

        if (!mCarEvsService.setRearviewCameraIdFromCommand(args[1])) {
            writer.println("Failed to set CarEvsService rearview camera device id.");
        } else {
            writer.printf("CarEvsService is set to use %s.\n", args[1]);
        }
    }

    private void getRearviewCameraId(IndentingPrintWriter writer) {
        writer.printf("CarEvsService is using %s for the rearview.\n",
                mCarEvsService.getRearviewCameraIdFromCommand());
    }

    // Set third-party foreground I/O threshold for car watchdog
    private void setWatchdogIoThirdPartyForegroundBytes(String[] args,
            IndentingPrintWriter writer) {
        if (args.length != 2) {
            showInvalidArguments(writer);
            return;
        }
        try {
            long newForegroundModeBytes = Long.parseLong(args[1]);
            ResourceOveruseConfiguration configuration =
                    getThirdPartyResourceOveruseConfiguration(
                            CarWatchdogManager.FLAG_RESOURCE_OVERUSE_IO);
            if (configuration == null) {
                writer.println("Failed to get third-party resource overuse configurations.");
                return;
            }
            ResourceOveruseConfiguration newConfiguration = setComponentLevelForegroundIoBytes(
                    configuration, newForegroundModeBytes);
            int result = mCarWatchdogService.setResourceOveruseConfigurations(
                    Collections.singletonList(newConfiguration),
                    CarWatchdogManager.FLAG_RESOURCE_OVERUSE_IO);
            if (result == CarWatchdogManager.RETURN_CODE_SUCCESS) {
                writer.printf(
                        "Successfully set third-party I/O overuse foreground threshold. { "
                                + "foregroundModeBytes = %d } \n",
                        newForegroundModeBytes);
            } else {
                writer.println("Failed to set third-party I/O overuse foreground threshold.");
            }
        } catch (NumberFormatException e) {
            writer.println("The argument provided does not contain a parsable long.");
            writer.println("Failed to set third-party I/O overuse foreground threshold.");
        } catch (RemoteException e) {
            writer.printf("Failed to set third-party I/O overuse foreground threshold: %s",
                    e.getMessage());
        }
    }

    private void getWatchdogIoThirdPartyForegroundBytes(IndentingPrintWriter writer) {
        ResourceOveruseConfiguration configuration =
                getThirdPartyResourceOveruseConfiguration(
                        CarWatchdogManager.FLAG_RESOURCE_OVERUSE_IO);
        try {
            IoOveruseConfiguration ioOveruseConfiguration = Objects.requireNonNull(
                    configuration).getIoOveruseConfiguration();
            PerStateBytes componentLevelThresholds = Objects.requireNonNull(ioOveruseConfiguration)
                    .getComponentLevelThresholds();
            long foregroundBytes = Objects.requireNonNull(
                    componentLevelThresholds).getForegroundModeBytes();
            writer.printf("foregroundModeBytes = %d \n", foregroundBytes);
        } catch (NullPointerException e) {
            writer.println("Failed to get third-party I/O overuse foreground threshold.");
        }
    }

    private ResourceOveruseConfiguration getThirdPartyResourceOveruseConfiguration(
            int resourceOveruseFlag) {
        for (ResourceOveruseConfiguration configuration :
                mCarWatchdogService.getResourceOveruseConfigurations(resourceOveruseFlag)) {
            if (configuration.getComponentType()
                    == ResourceOveruseConfiguration.COMPONENT_TYPE_THIRD_PARTY) {
                return configuration;
            }
        }
        return null;
    }

    private ResourceOveruseConfiguration setComponentLevelForegroundIoBytes(
            ResourceOveruseConfiguration configuration, long foregroundModeBytes) {
        IoOveruseConfiguration ioOveruseConfiguration = configuration.getIoOveruseConfiguration();
        PerStateBytes componentLevelThresholds =
                ioOveruseConfiguration.getComponentLevelThresholds();
        return constructResourceOveruseConfigurationBuilder(
                configuration).setIoOveruseConfiguration(
                new IoOveruseConfiguration.Builder(
                        new PerStateBytes(foregroundModeBytes,
                                componentLevelThresholds.getBackgroundModeBytes(),
                                componentLevelThresholds.getGarageModeBytes()),
                        ioOveruseConfiguration.getPackageSpecificThresholds(),
                        ioOveruseConfiguration.getAppCategorySpecificThresholds(),
                        ioOveruseConfiguration.getSystemWideThresholds())
                        .build())
                .build();
    }

    private ResourceOveruseConfiguration.Builder constructResourceOveruseConfigurationBuilder(
            ResourceOveruseConfiguration configuration) {
        return new ResourceOveruseConfiguration.Builder(configuration.getComponentType(),
                configuration.getSafeToKillPackages(),
                configuration.getVendorPackagePrefixes(),
                configuration.getPackagesToAppCategoryTypes())
                .setIoOveruseConfiguration(configuration.getIoOveruseConfiguration());
    }

    // Check if the given property is global
    private static boolean isPropertyAreaTypeGlobal(@Nullable String property) {
        if (property == null) {
            return false;
        }
        return (Integer.decode(property) & VehicleArea.MASK) == VehicleArea.GLOBAL;
    }
}
