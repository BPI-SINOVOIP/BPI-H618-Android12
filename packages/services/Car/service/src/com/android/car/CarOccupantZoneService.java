/*
 * Copyright (C) 2019 The Android Open Source Project
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

import android.annotation.NonNull;
import android.annotation.Nullable;
import android.annotation.UserIdInt;
import android.app.ActivityManager;
import android.car.Car;
import android.car.CarInfoManager;
import android.car.CarOccupantZoneManager;
import android.car.CarOccupantZoneManager.DisplayTypeEnum;
import android.car.CarOccupantZoneManager.OccupantTypeEnum;
import android.car.CarOccupantZoneManager.OccupantZoneInfo;
import android.car.ICarOccupantZone;
import android.car.ICarOccupantZoneCallback;
import android.car.VehicleAreaSeat;
import android.car.media.CarAudioManager;
import android.car.user.CarUserManager;
import android.car.user.CarUserManager.UserLifecycleListener;
import android.content.ComponentName;
import android.content.Context;
import android.content.pm.PackageManager;
import android.content.pm.UserInfo;
import android.content.res.Resources;
import android.hardware.display.DisplayManager;
import android.os.Handler;
import android.os.Looper;
import android.os.RemoteCallbackList;
import android.os.RemoteException;
import android.os.UserHandle;
import android.os.UserManager;
import android.util.ArrayMap;
import android.util.ArraySet;
import android.util.IndentingPrintWriter;
import android.util.IntArray;
import android.util.SparseArray;
import android.util.SparseIntArray;
import android.view.Display;
import android.view.DisplayAddress;

import com.android.car.internal.ICarServiceHelper;
import com.android.car.user.CarUserService;
import com.android.internal.annotations.GuardedBy;
import com.android.internal.annotations.VisibleForTesting;
import com.android.server.utils.Slogf;

import java.util.ArrayList;
import java.util.List;
import java.util.Objects;

/**
 * Service to implement CarOccupantZoneManager API.
 */
public final class CarOccupantZoneService extends ICarOccupantZone.Stub
        implements CarServiceBase {

    private static final String TAG = CarLog.tagFor(CarOccupantZoneService.class);
    private static final String ALL_COMPONENTS = "*";
    private static final int INVALID_PORT = -1;

    private final Object mLock = new Object();
    private final Context mContext;
    private final DisplayManager mDisplayManager;
    private final UserManager mUserManager;

    private final boolean mEnableProfileUserAssignmentForMultiDisplay;

    private boolean mEnableSourcePreferred;
    private ArrayList<ComponentName> mSourcePreferredComponents;

    /**
     * Stores android user id of profile users for the current user.
     */
    @GuardedBy("mLock")
    private final ArraySet<Integer> mProfileUsers = new ArraySet<>();

    /** key: zone id */
    @GuardedBy("mLock")
    private final SparseArray<OccupantZoneInfo> mOccupantsConfig = new SparseArray<>();

    @VisibleForTesting
    static class DisplayConfig {
        public final int displayType;
        public final int occupantZoneId;

        DisplayConfig(int displayType, int occupantZoneId) {
            this.displayType = displayType;
            this.occupantZoneId = occupantZoneId;
        }

        @Override
        public String toString() {
            // do not include type as this is only used for dump
            StringBuilder b = new StringBuilder(64);
            b.append("{displayType=");
            b.append(Integer.toHexString(displayType));
            b.append(" occupantZoneId=");
            b.append(occupantZoneId);
            b.append("}");
            return b.toString();
        }
    }

    /** key: display port address */
    @GuardedBy("mLock")
    private final SparseArray<DisplayConfig> mDisplayPortConfigs = new SparseArray<>();

    /** key: displayUniqueId */
    @GuardedBy("mLock")
    private final ArrayMap<String, DisplayConfig> mDisplayUniqueIdConfigs = new ArrayMap<>();

    /** key: audio zone id */
    @GuardedBy("mLock")
    private final SparseIntArray mAudioZoneIdToOccupantZoneIdMapping = new SparseIntArray();

    @VisibleForTesting
    static class DisplayInfo {
        public final Display display;
        public final int displayType;

        DisplayInfo(Display display, int displayType) {
            this.display = display;
            this.displayType = displayType;
        }

        @Override
        public String toString() {
            // do not include type as this is only used for dump
            StringBuilder b = new StringBuilder(64);
            b.append("{displayId=");
            b.append(display.getDisplayId());
            b.append(" displayType=");
            b.append(displayType);
            b.append("}");
            return b.toString();
        }
    }

    @VisibleForTesting
    static class OccupantConfig {
        public int userId = UserHandle.USER_NULL;
        public final ArrayList<DisplayInfo> displayInfos = new ArrayList<>();
        public int audioZoneId = CarAudioManager.INVALID_AUDIO_ZONE;

        @Override
        public String toString() {
            // do not include type as this is only used for dump
            StringBuilder b = new StringBuilder(128);
            b.append("{userId=");
            b.append(userId);
            b.append(" displays=");
            for (int i = 0; i < displayInfos.size(); i++) {
                b.append(displayInfos.get(i).toString());
            }
            b.append(" audioZoneId=");
            if (audioZoneId != CarAudioManager.INVALID_AUDIO_ZONE) {
                b.append(audioZoneId);
            } else {
                b.append("none");
            }
            b.append("}");
            return b.toString();
        }
    }

    /** key : zoneId */
    @GuardedBy("mLock")
    private final SparseArray<OccupantConfig> mActiveOccupantConfigs = new SparseArray<>();

    @GuardedBy("mLock")
    private ICarServiceHelper mICarServiceHelper;

    @GuardedBy("mLock")
    private int mDriverZoneId = OccupantZoneInfo.INVALID_ZONE_ID;

    @VisibleForTesting
    final UserLifecycleListener mUserLifecycleListener = event -> {
        Slogf.d(TAG, "onEvent(%s)", event);
        if (CarUserManager.USER_LIFECYCLE_EVENT_TYPE_SWITCHING == event.getEventType()) {
            handleUserChange();
        }
    };

    final CarUserService.PassengerCallback mPassengerCallback =
            new CarUserService.PassengerCallback() {
                @Override
                public void onPassengerStarted(@UserIdInt int passengerId, int zoneId) {
                    handlePassengerStarted(passengerId, zoneId);
                }

                @Override
                public void onPassengerStopped(@UserIdInt int passengerId) {
                    handlePassengerStopped(passengerId);
                }
            };

    @VisibleForTesting
    final DisplayManager.DisplayListener mDisplayListener =
            new DisplayManager.DisplayListener() {
                @Override
                public void onDisplayAdded(int displayId) {
                    handleDisplayChange();
                }

                @Override
                public void onDisplayRemoved(int displayId) {
                    handleDisplayChange();
                }

                @Override
                public void onDisplayChanged(int displayId) {
                    // nothing to do
                }
            };

    private final RemoteCallbackList<ICarOccupantZoneCallback> mClientCallbacks =
            new RemoteCallbackList<>();

    @GuardedBy("mLock")
    private int mDriverSeat = VehicleAreaSeat.SEAT_UNKNOWN;

    public CarOccupantZoneService(Context context) {
        this(context, context.getSystemService(DisplayManager.class),
                context.getSystemService(UserManager.class),
                context.getResources().getBoolean(
                        R.bool.enableProfileUserAssignmentForMultiDisplay)
                        && context.getPackageManager().hasSystemFeature(
                        PackageManager.FEATURE_MANAGED_USERS));
    }

    @VisibleForTesting
    public CarOccupantZoneService(Context context, DisplayManager displayManager,
            UserManager userManager, boolean enableProfileUserAssignmentForMultiDisplay) {
        mContext = context;
        mDisplayManager = displayManager;
        mUserManager = userManager;
        mEnableProfileUserAssignmentForMultiDisplay = enableProfileUserAssignmentForMultiDisplay;
    }

    @Override
    public void init() {
        // This does not require connection as binder will be passed directly.
        Car car = new Car(mContext, /* service= */null, /* handler= */ null);
        CarInfoManager infoManager = new CarInfoManager(car, CarLocalServices.getService(
                CarPropertyService.class));
        int driverSeat = infoManager.getDriverSeat();
        synchronized (mLock) {
            mDriverSeat = driverSeat;
            parseOccupantZoneConfigsLocked();
            parseDisplayConfigsLocked();
            handleActiveDisplaysLocked();
            handleAudioZoneChangesLocked();
            handleUserChangesLocked();
        }
        CarUserService userService = CarLocalServices.getService(CarUserService.class);
        userService.addUserLifecycleListener(mUserLifecycleListener);
        userService.addPassengerCallback(mPassengerCallback);
        mDisplayManager.registerDisplayListener(mDisplayListener,
                new Handler(Looper.getMainLooper()));
        CarUserService.ZoneUserBindingHelper helper = new CarUserService.ZoneUserBindingHelper() {
            @Override
            @NonNull
            public List<OccupantZoneInfo> getOccupantZones(@OccupantTypeEnum int occupantType) {
                List<OccupantZoneInfo> zones = new ArrayList<OccupantZoneInfo>();
                for (OccupantZoneInfo ozi : getAllOccupantZones()) {
                    if (ozi.occupantType == occupantType) {
                        zones.add(ozi);
                    }
                }
                return zones;
            }

            @Override
            public boolean assignUserToOccupantZone(@UserIdInt int userId, int zoneId) {
                // Check if the user is already assigned to the other zone.
                synchronized (mLock) {
                    for (int i = 0; i < mActiveOccupantConfigs.size(); ++i) {
                        OccupantConfig config = mActiveOccupantConfigs.valueAt(i);
                        if (config.userId == userId && zoneId != mActiveOccupantConfigs.keyAt(i)) {
                            Slogf.w(TAG, "cannot assign user to two different zone simultaneously");
                            return false;
                        }
                    }
                    OccupantConfig zoneConfig = mActiveOccupantConfigs.get(zoneId);
                    if (zoneConfig == null) {
                        Slogf.w(TAG, "cannot find the zone(%d)", zoneId);
                        return false;
                    }
                    if (zoneConfig.userId != UserHandle.USER_NULL && zoneConfig.userId != userId) {
                        Slogf.w(TAG, "other user already occupies the zone(%d)", zoneId);
                        return false;
                    }
                    zoneConfig.userId = userId;
                    return true;
                }
            }

            @Override
            public boolean unassignUserFromOccupantZone(@UserIdInt int userId) {
                synchronized (mLock) {
                    for (int i = 0; i < mActiveOccupantConfigs.size(); ++i) {
                        OccupantConfig config = mActiveOccupantConfigs.valueAt(i);
                        if (config.userId == userId) {
                            config.userId = UserHandle.USER_NULL;
                            break;
                        }
                    }
                    return true;
                }
            }

            @Override
            public boolean isPassengerDisplayAvailable() {
                for (OccupantZoneInfo ozi : getAllOccupantZones()) {
                    if (getDisplayForOccupant(ozi.zoneId,
                            CarOccupantZoneManager.DISPLAY_TYPE_MAIN) != Display.INVALID_DISPLAY
                            && ozi.occupantType != CarOccupantZoneManager.OCCUPANT_TYPE_DRIVER) {
                        return true;
                    }
                }
                return false;
            }
        };
        userService.setZoneUserBindingHelper(helper);
    }

    @Override
    public void release() {
        mDisplayManager.unregisterDisplayListener(mDisplayListener);
        CarUserService userService = CarLocalServices.getService(CarUserService.class);
        userService.removeUserLifecycleListener(mUserLifecycleListener);
        userService.removePassengerCallback(mPassengerCallback);
        synchronized (mLock) {
            mOccupantsConfig.clear();
            mDisplayPortConfigs.clear();
            mDisplayUniqueIdConfigs.clear();
            mAudioZoneIdToOccupantZoneIdMapping.clear();
            mActiveOccupantConfigs.clear();
        }
    }

    /** Return cloned mOccupantsConfig for testing */
    @VisibleForTesting
    @NonNull
    public SparseArray<OccupantZoneInfo> getOccupantsConfig() {
        synchronized (mLock) {
            return mOccupantsConfig.clone();
        }
    }

    /** Return cloned mDisplayPortConfigs for testing */
    @VisibleForTesting
    @NonNull
    public SparseArray<DisplayConfig> getDisplayPortConfigs() {
        synchronized (mLock) {
            return mDisplayPortConfigs.clone();
        }
    }

    /** Return cloned mDisplayUniqueIdConfigs for testing */
    @VisibleForTesting
    @NonNull
    ArrayMap<String, DisplayConfig> getDisplayUniqueIdConfigs() {
        synchronized (mLock) {
            return new ArrayMap<>(mDisplayUniqueIdConfigs);
        }
    }

    /** Return cloned mAudioConfigs for testing */
    @VisibleForTesting
    @NonNull
    SparseIntArray getAudioConfigs() {
        synchronized (mLock) {
            return mAudioZoneIdToOccupantZoneIdMapping.clone();
        }
    }

    /** Return cloned mActiveOccupantConfigs for testing */
    @VisibleForTesting
    @NonNull
    public SparseArray<OccupantConfig> getActiveOccupantConfigs() {
        synchronized (mLock) {
            return mActiveOccupantConfigs.clone();
        }
    }

    @Override
    public void dump(IndentingPrintWriter writer) {
        writer.println("*OccupantZoneService*");
        synchronized (mLock) {
            writer.println("**mOccupantsConfig**");
            for (int i = 0; i < mOccupantsConfig.size(); ++i) {
                writer.println(" zoneId=" + mOccupantsConfig.keyAt(i)
                        + " info=" + mOccupantsConfig.valueAt(i));
            }
            writer.println("**mDisplayConfigs**");
            for (int i = 0; i < mDisplayPortConfigs.size(); ++i) {
                writer.println(" port=" + mDisplayPortConfigs.keyAt(i)
                        + " config=" + mDisplayPortConfigs.valueAt(i));
            }
            for (int i = 0; i < mDisplayUniqueIdConfigs.size(); ++i) {
                writer.println(" uniqueId=" + mDisplayUniqueIdConfigs.keyAt(i)
                        + " config=" + mDisplayUniqueIdConfigs.valueAt(i));
            }
            writer.println("**mAudioZoneIdToOccupantZoneIdMapping**");
            for (int index = 0; index < mAudioZoneIdToOccupantZoneIdMapping.size(); index++) {
                int audioZoneId = mAudioZoneIdToOccupantZoneIdMapping.keyAt(index);
                writer.println(" audioZoneId=" + Integer.toHexString(audioZoneId)
                        + " zoneId=" + mAudioZoneIdToOccupantZoneIdMapping.valueAt(index));
            }
            writer.println("**mActiveOccupantConfigs**");
            for (int i = 0; i < mActiveOccupantConfigs.size(); ++i) {
                writer.println(" zoneId=" + mActiveOccupantConfigs.keyAt(i)
                        + " config=" + mActiveOccupantConfigs.valueAt(i));
            }
            writer.println("mEnableProfileUserAssignmentForMultiDisplay:"
                    + mEnableProfileUserAssignmentForMultiDisplay);
            writer.println("mEnableSourcePreferred:"
                    + mEnableSourcePreferred);
            writer.append("mSourcePreferredComponents: [");
            if (mSourcePreferredComponents != null) {
                for (int i = 0; i < mSourcePreferredComponents.size(); ++i) {
                    if (i > 0) writer.append(' ');
                    writer.append(mSourcePreferredComponents.get(i).toString());
                }
            }
            writer.println(']');
        }
    }

    @Override
    public List<OccupantZoneInfo> getAllOccupantZones() {
        synchronized (mLock) {
            List<OccupantZoneInfo> infos = new ArrayList<>();
            for (int i = 0; i < mActiveOccupantConfigs.size(); ++i) {
                int zoneId = mActiveOccupantConfigs.keyAt(i);
                // no need for deep copy as OccupantZoneInfo itself is static.
                infos.add(mOccupantsConfig.get(zoneId));
            }
            return infos;
        }
    }

    @Override
    public int[] getAllDisplaysForOccupantZone(int occupantZoneId) {
        synchronized (mLock) {
            OccupantConfig config = mActiveOccupantConfigs.get(occupantZoneId);
            if (config == null) {
                return new int[0];
            }
            int[] displayIds = new int[config.displayInfos.size()];
            for (int i = 0; i < config.displayInfos.size(); i++) {
                displayIds[i] = config.displayInfos.get(i).display.getDisplayId();
            }
            return displayIds;
        }
    }

    @Override
    public int getDisplayForOccupant(int occupantZoneId, int displayType) {
        synchronized (mLock) {
            OccupantConfig config = mActiveOccupantConfigs.get(occupantZoneId);
            if (config == null) {
                return Display.INVALID_DISPLAY;
            }
            for (int i = 0; i < config.displayInfos.size(); i++) {
                if (displayType == config.displayInfos.get(i).displayType) {
                    return config.displayInfos.get(i).display.getDisplayId();
                }
            }
        }
        return Display.INVALID_DISPLAY;
    }

    @Override
    public int getDisplayIdForDriver(@DisplayTypeEnum int displayType) {
        enforcePermission(Car.ACCESS_PRIVATE_DISPLAY_ID);
        synchronized (mLock) {
            int driverUserId = getDriverUserId();
            DisplayInfo displayInfo = findDisplayForDriverLocked(driverUserId, displayType);
            if (displayInfo == null) {
                return Display.INVALID_DISPLAY;
            }
            return displayInfo.display.getDisplayId();
        }
    }

    @Nullable
    private DisplayInfo findDisplayForDriverLocked(int driverUserId,
            @DisplayTypeEnum int displayType) {
        for (OccupantZoneInfo zoneInfo : getAllOccupantZones()) {
            if (zoneInfo.occupantType == CarOccupantZoneManager.OCCUPANT_TYPE_DRIVER) {
                OccupantConfig config = mActiveOccupantConfigs.get(zoneInfo.zoneId);
                if (config == null) {
                    //No active display for zone, just continue...
                    continue;
                }

                if (config.userId == driverUserId) {
                    for (DisplayInfo displayInfo : config.displayInfos) {
                        if (displayInfo.displayType == displayType) {
                            return displayInfo;
                        }
                    }
                }
            }
        }
        return null;
    }

    @Override
    public int getAudioZoneIdForOccupant(int occupantZoneId) {
        enforcePermission(Car.PERMISSION_CAR_CONTROL_AUDIO_SETTINGS);
        synchronized (mLock) {
            OccupantConfig config = mActiveOccupantConfigs.get(occupantZoneId);
            if (config != null) {
                return config.audioZoneId;
            }
            // check if the occupant id exist at all
            if (!mOccupantsConfig.contains(occupantZoneId)) {
                return CarAudioManager.INVALID_AUDIO_ZONE;
            }
            // Exist but not active
            return getAudioZoneIdForOccupantLocked(occupantZoneId);
        }
    }

    private int getAudioZoneIdForOccupantLocked(int occupantZoneId) {
        for (int index = 0; index < mAudioZoneIdToOccupantZoneIdMapping.size(); index++) {
            int audioZoneId = mAudioZoneIdToOccupantZoneIdMapping.keyAt(index);
            if (occupantZoneId == mAudioZoneIdToOccupantZoneIdMapping.get(audioZoneId)) {
                return audioZoneId;
            }
        }
        return CarAudioManager.INVALID_AUDIO_ZONE;
    }

    @Override
    public CarOccupantZoneManager.OccupantZoneInfo getOccupantForAudioZoneId(int audioZoneId) {
        enforcePermission(Car.PERMISSION_CAR_CONTROL_AUDIO_SETTINGS);
        synchronized (mLock) {
            int occupantZoneId = mAudioZoneIdToOccupantZoneIdMapping.get(audioZoneId,
                    OccupantZoneInfo.INVALID_ZONE_ID);
            if (occupantZoneId == OccupantZoneInfo.INVALID_ZONE_ID) {
                return null;
            }
            // To support headless zones return the occupant configuration.
            return mOccupantsConfig.get(occupantZoneId);
        }
    }

    @Nullable
    private DisplayConfig findDisplayConfigForDisplayIdLocked(int displayId) {
        Display display = mDisplayManager.getDisplay(displayId);
        if (display == null) {
            return null;
        }
        return findDisplayConfigForDisplayLocked(display);
    }

    @Nullable
    private DisplayConfig findDisplayConfigForDisplayLocked(Display display) {
        int portAddress = getPortAddress(display);
        if (portAddress != INVALID_PORT) {
            DisplayConfig config = mDisplayPortConfigs.get(portAddress);
            if (config != null) {
                return config;
            }
        }
        return mDisplayUniqueIdConfigs.get(display.getUniqueId());
    }

    @Override
    public int getDisplayType(int displayId) {
        synchronized (mLock) {
            DisplayConfig config = findDisplayConfigForDisplayIdLocked(displayId);
            if (config != null) {
                return config.displayType;
            }
        }
        return CarOccupantZoneManager.DISPLAY_TYPE_UNKNOWN;
    }

    @Override
    public int getUserForOccupant(int occupantZoneId) {
        synchronized (mLock) {
            OccupantConfig config = mActiveOccupantConfigs.get(occupantZoneId);
            if (config == null) {
                return UserHandle.USER_NULL;
            }
            return config.userId;
        }
    }

    @Override
    public int getOccupantZoneIdForUserId(int userId) {
        synchronized (mLock) {
            for (int i = 0; i < mActiveOccupantConfigs.size(); ++i) {
                OccupantConfig config = mActiveOccupantConfigs.valueAt(i);
                if (config.userId == userId) {
                    return mActiveOccupantConfigs.keyAt(i);
                }
            }
            Slogf.w(TAG, "Could not find occupantZoneId for userId%d returning invalid "
                    + "occupant zone id %d", userId, OccupantZoneInfo.INVALID_ZONE_ID);
            return OccupantZoneInfo.INVALID_ZONE_ID;
        }
    }

    /**
     * returns the current driver user id.
     */
    public @UserIdInt int getDriverUserId() {
        return getCurrentUser();
    }

    /**
     * Sets the mapping for audio zone id to occupant zone id.
     *
     * @param audioZoneIdToOccupantZoneMapping map for audio zone id, where key is the audio zone id
     * and value is the occupant zone id.
     */
    public void setAudioZoneIdsForOccupantZoneIds(
            @NonNull SparseIntArray audioZoneIdToOccupantZoneMapping) {
        Objects.requireNonNull(audioZoneIdToOccupantZoneMapping,
                "audioZoneIdToOccupantZoneMapping can not be null");
        synchronized (mLock) {
            validateOccupantZoneIdsLocked(audioZoneIdToOccupantZoneMapping);
            mAudioZoneIdToOccupantZoneIdMapping.clear();
            for (int index = 0; index < audioZoneIdToOccupantZoneMapping.size(); index++) {
                int audioZoneId = audioZoneIdToOccupantZoneMapping.keyAt(index);
                mAudioZoneIdToOccupantZoneIdMapping.put(audioZoneId,
                        audioZoneIdToOccupantZoneMapping.get(audioZoneId));
            }
            //If there are any active displays for the zone send change event
            handleAudioZoneChangesLocked();
        }
        sendConfigChangeEvent(CarOccupantZoneManager.ZONE_CONFIG_CHANGE_FLAG_AUDIO);
    }

    private void validateOccupantZoneIdsLocked(SparseIntArray audioZoneIdToOccupantZoneMapping) {
        for (int i = 0; i < audioZoneIdToOccupantZoneMapping.size(); i++) {
            int occupantZoneId =
                    audioZoneIdToOccupantZoneMapping.get(audioZoneIdToOccupantZoneMapping.keyAt(i));
            if (!mOccupantsConfig.contains(occupantZoneId)) {
                throw new IllegalArgumentException("occupantZoneId " + occupantZoneId
                        + " does not exist.");
            }
        }
    }

    @Override
    public void registerCallback(ICarOccupantZoneCallback callback) {
        mClientCallbacks.register(callback);
    }

    @Override
    public void unregisterCallback(ICarOccupantZoneCallback callback) {
        mClientCallbacks.unregister(callback);
    }

    @Override
    public boolean assignProfileUserToOccupantZone(int occupantZoneId, int userId) {
        enforcePermission(android.Manifest.permission.MANAGE_USERS);
        if (!mEnableProfileUserAssignmentForMultiDisplay) {
            throw new IllegalStateException("feature not enabled");
        }
        int currentUser = getCurrentUser();

        synchronized (mLock) {
            if (occupantZoneId == mDriverZoneId) {
                throw new IllegalArgumentException("Driver zone cannot have profile user");
            }
            updateEnabledProfilesLocked(currentUser);

            if (!mProfileUsers.contains(userId) && userId != UserHandle.USER_NULL) {
                // current user can change while this call is happening, so return false rather
                // than throwing exception
                Slogf.w(TAG, "Invalid profile user id: %d", userId);
                return false;
            }
            if (!mUserManager.isUserRunning(userId)) {
                Slogf.w(TAG, "User%d is not running.", userId);
                return false;
            }
            OccupantConfig config = mActiveOccupantConfigs.get(occupantZoneId);
            if (config == null) {
                throw new IllegalArgumentException("Invalid occupantZoneId:" + occupantZoneId);
            }
            if (config.userId == userId && userId != UserHandle.USER_NULL) {
                Slogf.w(TAG, "assignProfileUserToOccupantZone zone:%d already set to user:%",
                        occupantZoneId, userId);
                return true;
            }
            if (userId == UserHandle.USER_NULL) {
                config.userId = currentUser;
            } else {
                config.userId = userId;
            }
        }
        sendConfigChangeEvent(CarOccupantZoneManager.ZONE_CONFIG_CHANGE_FLAG_USER);
        return true;
    }

    /**
     * Sets {@code ICarServiceHelper}.
     */
    public void setCarServiceHelper(ICarServiceHelper helper) {
        doSyncWithCarServiceHelper(helper, /* updateDisplay= */ true, /* updateUser= */ true,
                /* updateConfig= */ true);
    }

    private void doSyncWithCarServiceHelper(@Nullable ICarServiceHelper helper,
            boolean updateDisplay, boolean updateUser, boolean updateConfig) {
        int[] passengerDisplays = null;
        ArrayMap<Integer, IntArray> allowlists = null;
        ICarServiceHelper helperToUse = helper;
        synchronized (mLock) {
            if (helper == null) {
                if (mICarServiceHelper == null) { // helper not set yet.
                    return;
                }
                helperToUse = mICarServiceHelper;
            } else {
                mICarServiceHelper = helper;
            }
            if (updateDisplay) {
                passengerDisplays = getAllActivePassengerDisplaysLocked();
            }
            if (updateUser) {
                allowlists = createDisplayAllowlistsLocked();
            }
        }
        if (updateDisplay) {
            updatePassengerDisplays(helperToUse, passengerDisplays);
        }
        if (updateUser) {
            updateUserAssignmentForDisplays(helperToUse, allowlists);
        }
        if (updateConfig) {
            Resources res = mContext.getResources();
            String[] components = res.getStringArray(R.array.config_sourcePreferredComponents);
            updateSourcePreferredComponents(helperToUse, components);
        }
    }

    private int[] getAllActivePassengerDisplaysLocked() {
        IntArray displays = new IntArray();
        for (int j = 0; j < mActiveOccupantConfigs.size(); ++j) {
            int zoneId = mActiveOccupantConfigs.keyAt(j);
            if (zoneId == mDriverZoneId) {
                continue;
            }
            OccupantConfig config = mActiveOccupantConfigs.valueAt(j);
            for (int i = 0; i < config.displayInfos.size(); i++) {
                displays.add(config.displayInfos.get(i).display.getDisplayId());
            }
        }
        return displays.toArray();
    }

    private void updatePassengerDisplays(ICarServiceHelper helper, int[] passengerDisplayIds) {
        if (passengerDisplayIds == null) {
            return;
        }
        try {
            helper.setPassengerDisplays(passengerDisplayIds);
        } catch (RemoteException e) {
            Slogf.e(TAG, "ICarServiceHelper.setPassengerDisplays failed", e);
        }
    }

    private void updateSourcePreferredComponents(ICarServiceHelper helper, String[] components) {
        boolean enableSourcePreferred;
        ArrayList<ComponentName> componentNames = null;
        if (components == null || components.length == 0) {
            enableSourcePreferred = false;
            Slogf.i(TAG, "CarLaunchParamsModifier: disable source-preferred");
        } else if (components.length == 1 && components[0].equals(ALL_COMPONENTS)) {
            enableSourcePreferred = true;
            Slogf.i(TAG, "CarLaunchParamsModifier: enable source-preferred for all Components");
        } else {
            componentNames = new ArrayList<>((components.length));
            for (String item : components) {
                ComponentName name = ComponentName.unflattenFromString(item);
                if (name == null) {
                    Slogf.e(TAG, "CarLaunchParamsModifier: Wrong ComponentName=" + item);
                    return;
                }
                componentNames.add(name);
            }
            enableSourcePreferred = true;
        }
        try {
            helper.setSourcePreferredComponents(enableSourcePreferred, componentNames);
            mEnableSourcePreferred = enableSourcePreferred;
            mSourcePreferredComponents = componentNames;
        } catch (RemoteException e) {
            Slogf.e(TAG, "ICarServiceHelper.setSourcePreferredComponents failed");
        }
    }

    private ArrayMap<Integer, IntArray> createDisplayAllowlistsLocked() {
        ArrayMap<Integer, IntArray> allowlists = new ArrayMap<>();
        for (int j = 0; j < mActiveOccupantConfigs.size(); ++j) {
            int zoneId = mActiveOccupantConfigs.keyAt(j);
            if (zoneId == mDriverZoneId) {
                continue;
            }
            OccupantConfig config = mActiveOccupantConfigs.valueAt(j);
            if (config.displayInfos.isEmpty()) {
                continue;
            }
            // user like driver can have multiple zones assigned, so add them all.
            IntArray displays = allowlists.get(config.userId);
            if (displays == null) {
                displays = new IntArray();
                allowlists.put(config.userId, displays);
            }
            for (int i = 0; i < config.displayInfos.size(); i++) {
                displays.add(config.displayInfos.get(i).display.getDisplayId());
            }
        }
        return allowlists;
    }

    private void updateUserAssignmentForDisplays(ICarServiceHelper helper,
            ArrayMap<Integer, IntArray> allowlists) {
        if (allowlists == null || allowlists.isEmpty()) {
            return;
        }
        try {
            for (int i = 0; i < allowlists.size(); i++) {
                int userId = allowlists.keyAt(i);
                helper.setDisplayAllowlistForUser(userId, allowlists.valueAt(i).toArray());
            }
        } catch (RemoteException e) {
            Slogf.e(TAG, "ICarServiceHelper.setDisplayAllowlistForUser failed", e);
        }
    }

    private void throwFormatErrorInOccupantZones(String msg) {
        throw new RuntimeException("Format error in config_occupant_zones resource:" + msg);
    }

    // For overriding in test
    @VisibleForTesting
    int getDriverSeat() {
        synchronized (mLock) {
            return mDriverSeat;
        }
    }

    private void parseOccupantZoneConfigsLocked() {
        final Resources res = mContext.getResources();
        // examples:
        // <item>occupantZoneId=0,occupantType=DRIVER,seatRow=1,seatSide=driver</item>
        // <item>occupantZoneId=1,occupantType=FRONT_PASSENGER,seatRow=1,
        // searSide=oppositeDriver</item>
        boolean hasDriver = false;
        int driverSeat = getDriverSeat();
        int driverSeatSide = VehicleAreaSeat.SIDE_LEFT; // default LHD : Left Hand Drive
        if (driverSeat == VehicleAreaSeat.SEAT_ROW_1_RIGHT) {
            driverSeatSide = VehicleAreaSeat.SIDE_RIGHT;
        }
        int maxZoneId = OccupantZoneInfo.INVALID_ZONE_ID;
        for (String config : res.getStringArray(R.array.config_occupant_zones)) {
            int zoneId = OccupantZoneInfo.INVALID_ZONE_ID;
            int type = CarOccupantZoneManager.OCCUPANT_TYPE_INVALID;
            int seatRow = 0; // invalid row
            int seatSide = VehicleAreaSeat.SIDE_LEFT;
            String[] entries = config.split(",");
            for (String entry : entries) {
                String[] keyValuePair = entry.split("=");
                if (keyValuePair.length != 2) {
                    throwFormatErrorInOccupantZones("No key/value pair:" + entry);
                }
                switch (keyValuePair[0]) {
                    case "occupantZoneId":
                        zoneId = Integer.parseInt(keyValuePair[1]);
                        break;
                    case "occupantType":
                        switch (keyValuePair[1]) {
                            case "DRIVER":
                                type = CarOccupantZoneManager.OCCUPANT_TYPE_DRIVER;
                                break;
                            case "FRONT_PASSENGER":
                                type = CarOccupantZoneManager.OCCUPANT_TYPE_FRONT_PASSENGER;
                                break;
                            case "REAR_PASSENGER":
                                type = CarOccupantZoneManager.OCCUPANT_TYPE_REAR_PASSENGER;
                                break;
                            default:
                                throwFormatErrorInOccupantZones("Unrecognized type:" + entry);
                                break;
                        }
                        break;
                    case "seatRow":
                        seatRow = Integer.parseInt(keyValuePair[1]);
                        break;
                    case "seatSide":
                        switch (keyValuePair[1]) {
                            case "driver":
                                seatSide = driverSeatSide;
                                break;
                            case "oppositeDriver":
                                seatSide = -driverSeatSide;
                                break;
                            case "left":
                                seatSide = VehicleAreaSeat.SIDE_LEFT;
                                break;
                            case "center":
                                seatSide = VehicleAreaSeat.SIDE_CENTER;
                                break;
                            case "right":
                                seatSide = VehicleAreaSeat.SIDE_RIGHT;
                                break;
                            default:
                                throwFormatErrorInOccupantZones("Unregognized seatSide:" + entry);
                                break;

                        }
                        break;
                    default:
                        throwFormatErrorInOccupantZones("Unrecognized key:" + entry);
                        break;
                }
            }
            if (zoneId == OccupantZoneInfo.INVALID_ZONE_ID) {
                throwFormatErrorInOccupantZones("Missing zone id:" + config);
            }
            if (zoneId > maxZoneId) {
                maxZoneId = zoneId;
            }
            if (type == CarOccupantZoneManager.OCCUPANT_TYPE_INVALID) {
                throwFormatErrorInOccupantZones("Missing type:" + config);
            }
            if (type == CarOccupantZoneManager.OCCUPANT_TYPE_DRIVER) {
                if (hasDriver) {
                    throwFormatErrorInOccupantZones("Multiple driver:" + config);
                } else {
                    hasDriver = true;
                    mDriverZoneId = zoneId;
                }
            }
            int seat = VehicleAreaSeat.fromRowAndSide(seatRow, seatSide);
            if (seat == VehicleAreaSeat.SEAT_UNKNOWN) {
                throwFormatErrorInOccupantZones("Invalid seat:" + config);
            }
            OccupantZoneInfo info = new OccupantZoneInfo(zoneId, type, seat);
            if (mOccupantsConfig.contains(zoneId)) {
                throwFormatErrorInOccupantZones("Duplicate zone id:" + config);
            }
            mOccupantsConfig.put(zoneId, info);
        }
        if (!hasDriver) {
            maxZoneId++;
            mDriverZoneId = maxZoneId;
            Slogf.w(TAG, "No driver zone, add one:%d", mDriverZoneId);
            OccupantZoneInfo info = new OccupantZoneInfo(mDriverZoneId,
                    CarOccupantZoneManager.OCCUPANT_TYPE_DRIVER, getDriverSeat());
            mOccupantsConfig.put(mDriverZoneId, info);
        }
    }

    private void throwFormatErrorInDisplayMapping(String msg) {
        throw new RuntimeException(
                "Format error in config_occupant_display_mapping resource:" + msg);
    }

    private void parseDisplayConfigsLocked() {
        final Resources res = mContext.getResources();
        // examples:
        // <item>displayPort=0,displayType=MAIN,occupantZoneId=0</item>
        // <item>displayPort=1,displayType=INSTRUMENT_CLUSTER,occupantZoneId=0</item>
        for (String config : res.getStringArray(R.array.config_occupant_display_mapping)) {
            int port = INVALID_PORT;
            String uniqueId = null;
            int type = CarOccupantZoneManager.DISPLAY_TYPE_UNKNOWN;
            int zoneId = OccupantZoneInfo.INVALID_ZONE_ID;
            String[] entries = config.split(",");
            for (String entry : entries) {
                String[] keyValuePair = entry.split("=");
                if (keyValuePair.length != 2) {
                    throwFormatErrorInDisplayMapping("No key/value pair:" + entry);
                }
                switch (keyValuePair[0]) {
                    case "displayPort":
                        port = Integer.parseInt(keyValuePair[1]);
                        break;
                    case "displayUniqueId":
                        uniqueId = keyValuePair[1];
                        break;
                    case "displayType":
                        switch (keyValuePair[1]) {
                            case "MAIN":
                                type = CarOccupantZoneManager.DISPLAY_TYPE_MAIN;
                                break;
                            case "INSTRUMENT_CLUSTER":
                                type = CarOccupantZoneManager.DISPLAY_TYPE_INSTRUMENT_CLUSTER;
                                break;
                            case "HUD":
                                type = CarOccupantZoneManager.DISPLAY_TYPE_HUD;
                                break;
                            case "INPUT":
                                type = CarOccupantZoneManager.DISPLAY_TYPE_INPUT;
                                break;
                            case "AUXILIARY":
                                type = CarOccupantZoneManager.DISPLAY_TYPE_AUXILIARY;
                                break;
                            default:
                                throwFormatErrorInDisplayMapping(
                                        "Unrecognized display type:" + entry);
                                break;
                        }
                        break;
                    case "occupantZoneId":
                        zoneId = Integer.parseInt(keyValuePair[1]);
                        break;
                    default:
                        throwFormatErrorInDisplayMapping("Unrecognized key:" + entry);
                        break;

                }
            }
            // Now check validity
            if (port == INVALID_PORT && uniqueId == null) {
                throwFormatErrorInDisplayMapping(
                        "Missing or invalid displayPort and displayUniqueId:" + config);
            }

            if (type == CarOccupantZoneManager.DISPLAY_TYPE_UNKNOWN) {
                throwFormatErrorInDisplayMapping("Missing or invalid displayType:" + config);
            }
            if (zoneId == OccupantZoneInfo.INVALID_ZONE_ID) {
                throwFormatErrorInDisplayMapping("Missing or invalid occupantZoneId:" + config);
            }
            if (!mOccupantsConfig.contains(zoneId)) {
                throwFormatErrorInDisplayMapping(
                        "Missing or invalid occupantZoneId:" + config);
            }
            DisplayConfig displayConfig = new DisplayConfig(type, zoneId);
            if (port != INVALID_PORT) {
                if (mDisplayPortConfigs.contains(port)) {
                    throwFormatErrorInDisplayMapping("Duplicate displayPort:" + config);
                }
                mDisplayPortConfigs.put(port, displayConfig);
            } else {
                if (mDisplayUniqueIdConfigs.containsKey(uniqueId)) {
                    throwFormatErrorInDisplayMapping("Duplicate displayUniqueId:" + config);
                }
                mDisplayUniqueIdConfigs.put(uniqueId, displayConfig);
            }
        }
    }

    private int getPortAddress(Display display) {
        DisplayAddress address = display.getAddress();
        if (address instanceof DisplayAddress.Physical) {
            DisplayAddress.Physical physicalAddress = (DisplayAddress.Physical) address;
            if (physicalAddress != null) {
                return physicalAddress.getPort();
            }
        }
        return INVALID_PORT;
    }

    private void addDisplayInfoToOccupantZoneLocked(int zoneId, DisplayInfo info) {
        OccupantConfig occupantConfig = mActiveOccupantConfigs.get(zoneId);
        if (occupantConfig == null) {
            occupantConfig = new OccupantConfig();
            mActiveOccupantConfigs.put(zoneId, occupantConfig);
        }
        occupantConfig.displayInfos.add(info);
    }

    private void handleActiveDisplaysLocked() {
        mActiveOccupantConfigs.clear();
        boolean hasDefaultDisplayConfig = false;
        for (Display display : mDisplayManager.getDisplays()) {
            DisplayConfig displayConfig = findDisplayConfigForDisplayLocked(display);
            if (displayConfig == null) {
                Slogf.w(TAG, "Display id:%d does not have configurations",
                        display.getDisplayId());
                continue;
            }
            if (display.getDisplayId() == Display.DEFAULT_DISPLAY) {
                if (displayConfig.occupantZoneId != mDriverZoneId) {
                    throw new IllegalStateException(
                            "Default display should be only assigned to driver zone");
                }
                hasDefaultDisplayConfig = true;
            }
            addDisplayInfoToOccupantZoneLocked(displayConfig.occupantZoneId,
                    new DisplayInfo(display, displayConfig.displayType));
        }
        if (!hasDefaultDisplayConfig) {
            // Can reach here if default display has no port / no config
            Slogf.w(TAG, "Default display not assigned, will assign to driver zone");
            addDisplayInfoToOccupantZoneLocked(mDriverZoneId, new DisplayInfo(
                    mDisplayManager.getDisplay(Display.DEFAULT_DISPLAY),
                    CarOccupantZoneManager.DISPLAY_TYPE_MAIN));
        }
    }

    @VisibleForTesting
    int getCurrentUser() {
        return ActivityManager.getCurrentUser();
    }

    private void updateEnabledProfilesLocked(int userId) {
        mProfileUsers.clear();
        List<UserInfo> profileUsers = mUserManager.getEnabledProfiles(userId);
        for (UserInfo userInfo : profileUsers) {
            if (userInfo.id != userId) {
                mProfileUsers.add(userInfo.id);
            }
        }
    }

    private void handleUserChangesLocked() {
        int driverUserId = getCurrentUser();

        if (mEnableProfileUserAssignmentForMultiDisplay) {
            updateEnabledProfilesLocked(driverUserId);
        }

        for (int i = 0; i < mActiveOccupantConfigs.size(); ++i) {
            int zoneId = mActiveOccupantConfigs.keyAt(i);
            OccupantConfig config = mActiveOccupantConfigs.valueAt(i);
            // mProfileUsers empty if not supported
            if (mProfileUsers.contains(config.userId)) {
                Slogf.i(TAG, "Profile user:%d already assigned for occupant zone:%d",
                        config.userId, zoneId);
            } else {
                config.userId = driverUserId;
            }
        }
    }

    private void handleAudioZoneChangesLocked() {
        for (int index = 0; index < mAudioZoneIdToOccupantZoneIdMapping.size(); index++) {
            int audioZoneId = mAudioZoneIdToOccupantZoneIdMapping.keyAt(index);
            int occupantZoneId = mAudioZoneIdToOccupantZoneIdMapping.get(audioZoneId);
            OccupantConfig occupantConfig = mActiveOccupantConfigs.get(occupantZoneId);
            if (occupantConfig == null) {
                //no active display for zone just continue
                continue;
            }
            // Found an active configuration, add audio to it.
            occupantConfig.audioZoneId = audioZoneId;
        }
    }

    private void sendConfigChangeEvent(int changeFlags) {
        boolean updateDisplay = false;
        boolean updateUser = false;
        if ((changeFlags & CarOccupantZoneManager.ZONE_CONFIG_CHANGE_FLAG_DISPLAY) != 0) {
            updateDisplay = true;
            updateUser = true;
        } else if ((changeFlags & CarOccupantZoneManager.ZONE_CONFIG_CHANGE_FLAG_USER) != 0) {
            updateUser = true;
        }
        doSyncWithCarServiceHelper(/* helper= */ null, updateDisplay, updateUser,
                /* updateConfig= */ false);

        final int n = mClientCallbacks.beginBroadcast();
        for (int i = 0; i < n; i++) {
            ICarOccupantZoneCallback callback = mClientCallbacks.getBroadcastItem(i);
            try {
                callback.onOccupantZoneConfigChanged(changeFlags);
            } catch (RemoteException ignores) {
                // ignore
            }
        }
        mClientCallbacks.finishBroadcast();
    }

    private void handleUserChange() {
        synchronized (mLock) {
            handleUserChangesLocked();
        }
        sendConfigChangeEvent(CarOccupantZoneManager.ZONE_CONFIG_CHANGE_FLAG_USER);
    }

    private void handlePassengerStarted(@UserIdInt int passengerId, int zoneId) {
        sendConfigChangeEvent(CarOccupantZoneManager.ZONE_CONFIG_CHANGE_FLAG_USER);
    }

    private void handlePassengerStopped(@UserIdInt int passengerId) {
        sendConfigChangeEvent(CarOccupantZoneManager.ZONE_CONFIG_CHANGE_FLAG_USER);
    }

    private void handleDisplayChange() {
        synchronized (mLock) {
            handleActiveDisplaysLocked();
            //audio zones should be re-checked for changed display
            handleAudioZoneChangesLocked();
            // user should be re-checked for changed displays
            handleUserChangesLocked();
        }
        sendConfigChangeEvent(CarOccupantZoneManager.ZONE_CONFIG_CHANGE_FLAG_DISPLAY);
    }

    private void enforcePermission(String permissionName) {
        if (mContext.checkCallingOrSelfPermission(permissionName)
                != PackageManager.PERMISSION_GRANTED) {
            throw new SecurityException("requires permission " + permissionName);
        }
    }
}
