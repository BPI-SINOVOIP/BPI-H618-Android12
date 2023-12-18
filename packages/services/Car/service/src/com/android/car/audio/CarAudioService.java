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
package com.android.car.audio;

import static android.car.media.CarAudioManager.AUDIO_FEATURE_DYNAMIC_ROUTING;
import static android.car.media.CarAudioManager.AUDIO_FEATURE_VOLUME_GROUP_MUTING;
import static android.car.media.CarAudioManager.CarAudioFeature;
import static android.car.media.CarAudioManager.INVALID_VOLUME_GROUP_ID;
import static android.car.media.CarAudioManager.PRIMARY_AUDIO_ZONE;
import static android.media.AudioManager.FLAG_PLAY_SOUND;

import static com.android.car.audio.CarVolume.VERSION_TWO;
import static com.android.car.audio.hal.AudioControlWrapper.AUDIOCONTROL_FEATURE_AUDIO_DUCKING;
import static com.android.car.audio.hal.AudioControlWrapper.AUDIOCONTROL_FEATURE_AUDIO_FOCUS;
import static com.android.car.internal.ExcludeFromCodeCoverageGeneratedReport.DUMP_INFO;

import android.annotation.NonNull;
import android.annotation.Nullable;
import android.annotation.UserIdInt;
import android.car.Car;
import android.car.CarOccupantZoneManager;
import android.car.CarOccupantZoneManager.OccupantZoneConfigChangeListener;
import android.car.media.CarAudioManager;
import android.car.media.CarAudioPatchHandle;
import android.car.media.ICarAudio;
import android.car.media.ICarVolumeCallback;
import android.content.BroadcastReceiver;
import android.content.Context;
import android.content.Intent;
import android.content.IntentFilter;
import android.content.pm.PackageManager;
import android.media.AudioAttributes;
import android.media.AudioAttributes.AttributeSystemUsage;
import android.media.AudioAttributes.AttributeUsage;
import android.media.AudioDeviceAttributes;
import android.media.AudioDeviceInfo;
import android.media.AudioDevicePort;
import android.media.AudioFocusInfo;
import android.media.AudioFormat;
import android.media.AudioGain;
import android.media.AudioGainConfig;
import android.media.AudioManager;
import android.media.AudioPatch;
import android.media.AudioPlaybackConfiguration;
import android.media.AudioPortConfig;
import android.media.audiopolicy.AudioPolicy;
import android.os.IBinder;
import android.os.Looper;
import android.os.SystemClock;
import android.os.UserHandle;
import android.telephony.Annotation.CallState;
import android.telephony.TelephonyManager;
import android.text.TextUtils;
import android.util.IndentingPrintWriter;
import android.util.Log;
import android.util.Slog;
import android.util.SparseArray;
import android.util.SparseIntArray;
import android.view.KeyEvent;

import com.android.car.CarLocalServices;
import com.android.car.CarLog;
import com.android.car.CarOccupantZoneService;
import com.android.car.CarServiceBase;
import com.android.car.R;
import com.android.car.audio.CarAudioContext.AudioContext;
import com.android.car.audio.hal.AudioControlFactory;
import com.android.car.audio.hal.AudioControlWrapper;
import com.android.car.audio.hal.AudioControlWrapperV1;
import com.android.car.audio.hal.HalAudioFocus;
import com.android.car.internal.ExcludeFromCodeCoverageGeneratedReport;
import com.android.internal.annotations.GuardedBy;
import com.android.internal.util.Preconditions;
import com.android.server.utils.Slogf;

import org.xmlpull.v1.XmlPullParserException;

import java.io.File;
import java.io.FileInputStream;
import java.io.IOException;
import java.io.InputStream;
import java.util.ArrayList;
import java.util.Arrays;
import java.util.HashMap;
import java.util.HashSet;
import java.util.List;
import java.util.Map;
import java.util.Objects;
import java.util.Set;
import java.util.stream.Collectors;

/**
 * Service responsible for interaction with car's audio system.
 */
public class CarAudioService extends ICarAudio.Stub implements CarServiceBase {

    // Enable to allowed for delayed audio focus in car audio service.
    private static final boolean ENABLE_DELAYED_AUDIO_FOCUS = true;

    static final @AttributeUsage int DEFAULT_AUDIO_USAGE = AudioAttributes.USAGE_MEDIA;
    static final @AudioContext int DEFAULT_AUDIO_CONTEXT = CarAudioContext.getContextForUsage(
            CarAudioService.DEFAULT_AUDIO_USAGE);

    // CarAudioService reads configuration from the following paths respectively.
    // If the first one is found, all others are ignored.
    // If no one is found, it fallbacks to car_volume_groups.xml resource file.
    private static final String[] AUDIO_CONFIGURATION_PATHS = new String[] {
            "/vendor/etc/car_audio_configuration.xml",
            "/system/etc/car_audio_configuration.xml"
    };

    private static final @AttributeSystemUsage int[] SYSTEM_USAGES = new int[] {
            AudioAttributes.USAGE_CALL_ASSISTANT,
            AudioAttributes.USAGE_EMERGENCY,
            AudioAttributes.USAGE_SAFETY,
            AudioAttributes.USAGE_VEHICLE_STATUS,
            AudioAttributes.USAGE_ANNOUNCEMENT
    };

    private final Object mImplLock = new Object();

    private final Context mContext;
    private final TelephonyManager mTelephonyManager;
    private final AudioManager mAudioManager;
    private final boolean mUseDynamicRouting;
    private final boolean mUseCarVolumeGroupMuting;
    private final boolean mUseHalDuckingSignals;
    private final @CarVolume.CarVolumeListVersion int mAudioVolumeAdjustmentContextsVersion;
    private final boolean mPersistMasterMuteState;
    private final CarAudioSettings mCarAudioSettings;
    private final CarVolume mCarVolume;
    private final int mKeyEventTimeoutMs;
    private AudioControlWrapper mAudioControlWrapper;
    private CarDucking mCarDucking;
    private CarVolumeGroupMuting mCarVolumeGroupMuting;
    private HalAudioFocus mHalAudioFocus;

    private CarOccupantZoneService mOccupantZoneService;

    private CarOccupantZoneManager mOccupantZoneManager;

    /**
     * Simulates {@link ICarVolumeCallback} when it's running in legacy mode.
     * This receiver assumes the intent is sent to {@link CarAudioManager#PRIMARY_AUDIO_ZONE}.
     */
    private final BroadcastReceiver mLegacyVolumeChangedReceiver = new BroadcastReceiver() {
        @Override
        public void onReceive(Context context, Intent intent) {
            final int zoneId = CarAudioManager.PRIMARY_AUDIO_ZONE;
            switch (intent.getAction()) {
                case AudioManager.VOLUME_CHANGED_ACTION:
                    int streamType = intent.getIntExtra(AudioManager.EXTRA_VOLUME_STREAM_TYPE, -1);
                    int groupId = getVolumeGroupIdForStreamType(streamType);
                    if (groupId == -1) {
                        Slog.w(CarLog.TAG_AUDIO, "Unknown stream type: " + streamType);
                    } else {
                        callbackGroupVolumeChange(zoneId, groupId, 0);
                    }
                    break;
                case AudioManager.MASTER_MUTE_CHANGED_ACTION:
                    callbackMasterMuteChange(zoneId, 0);
                    break;
            }
        }
    };

    private AudioPolicy mAudioPolicy;
    private CarZonesAudioFocus mFocusHandler;
    private String mCarAudioConfigurationPath;
    private SparseIntArray mAudioZoneIdToOccupantZoneIdMapping;
    @GuardedBy("mImplLock")
    private SparseArray<CarAudioZone> mCarAudioZones;
    private final CarVolumeCallbackHandler mCarVolumeCallbackHandler;
    private final SparseIntArray mAudioZoneIdToUserIdMapping;
    private final SystemClockWrapper mClock = new SystemClockWrapper();


    // TODO do not store uid mapping here instead use the uid
    //  device affinity in audio policy when available
    private Map<Integer, Integer> mUidToZoneMap;
    private OccupantZoneConfigChangeListener
            mOccupantZoneConfigChangeListener = new CarAudioOccupantConfigChangeListener();
    private CarAudioPlaybackCallback mCarAudioPlaybackCallback;
    private CarAudioPowerListener mCarAudioPowerListener;

    public CarAudioService(Context context) {
        mContext = context;
        mTelephonyManager = (TelephonyManager) mContext.getSystemService(Context.TELEPHONY_SERVICE);
        mAudioManager = (AudioManager) mContext.getSystemService(Context.AUDIO_SERVICE);

        mUseDynamicRouting = mContext.getResources().getBoolean(R.bool.audioUseDynamicRouting);
        mKeyEventTimeoutMs =
                mContext.getResources().getInteger(R.integer.audioVolumeKeyEventTimeoutMs);
        mUseHalDuckingSignals = mContext.getResources().getBoolean(
                R.bool.audioUseHalDuckingSignals);

        mUidToZoneMap = new HashMap<>();
        mCarVolumeCallbackHandler = new CarVolumeCallbackHandler();
        mCarAudioSettings = new CarAudioSettings(mContext.getContentResolver());
        mAudioZoneIdToUserIdMapping = new SparseIntArray();
        mAudioVolumeAdjustmentContextsVersion =
                mContext.getResources().getInteger(R.integer.audioVolumeAdjustmentContextsVersion);
        mCarVolume = new CarVolume(mClock,
                mAudioVolumeAdjustmentContextsVersion, mKeyEventTimeoutMs);
        boolean useCarVolumeGroupMuting = mUseDynamicRouting && mContext.getResources().getBoolean(
                R.bool.audioUseCarVolumeGroupMuting);
        if (mAudioVolumeAdjustmentContextsVersion != VERSION_TWO && useCarVolumeGroupMuting) {
            throw new IllegalArgumentException("audioUseCarVolumeGroupMuting is enabled but "
                    + "this requires audioVolumeAdjustmentContextsVersion 2,"
                    + " instead version " + mAudioVolumeAdjustmentContextsVersion + " was found");
        }
        mUseCarVolumeGroupMuting = useCarVolumeGroupMuting;
        mPersistMasterMuteState = !mUseCarVolumeGroupMuting && mContext.getResources().getBoolean(
                R.bool.audioPersistMasterMuteState);
    }

    /**
     * Dynamic routing and volume groups are set only if
     * {@link #mUseDynamicRouting} is {@code true}. Otherwise, this service runs in legacy mode.
     */
    @Override
    public void init() {
        synchronized (mImplLock) {
            mOccupantZoneService = CarLocalServices.getService(CarOccupantZoneService.class);
            Car car = new Car(mContext, /* service= */null, /* handler= */ null);
            mOccupantZoneManager = new CarOccupantZoneManager(car, mOccupantZoneService);
            if (mUseDynamicRouting) {
                setupDynamicRoutingLocked();
                setupHalAudioFocusListenerLocked();
                setupAudioConfigurationCallbackLocked();
                setupPowerPolicyListener();
            } else {
                Slog.i(CarLog.TAG_AUDIO, "Audio dynamic routing not enabled, run in legacy mode");
                setupLegacyVolumeChangedListener();
            }

            mAudioManager.setSupportedSystemUsages(SYSTEM_USAGES);
        }

        restoreMasterMuteState();
    }

    private void setupPowerPolicyListener() {
        mCarAudioPowerListener = CarAudioPowerListener.newCarAudioPowerListener(this);
        mCarAudioPowerListener.startListeningForPolicyChanges();
    }

    private void restoreMasterMuteState() {
        if (mUseCarVolumeGroupMuting) {
            return;
        }
        // Restore master mute state if applicable
        if (mPersistMasterMuteState) {
            boolean storedMasterMute = mCarAudioSettings.getMasterMute();
            setMasterMute(storedMasterMute, 0);
        }
    }

    @Override
    public void release() {
        synchronized (mImplLock) {
            if (mUseDynamicRouting) {
                if (mAudioPolicy != null) {
                    mAudioManager.unregisterAudioPolicyAsync(mAudioPolicy);
                    mAudioPolicy = null;
                    mFocusHandler.setOwningPolicy(null, null);
                    mFocusHandler = null;
                }
            } else {
                mContext.unregisterReceiver(mLegacyVolumeChangedReceiver);
            }

            mCarVolumeCallbackHandler.release();

            if (mHalAudioFocus != null) {
                mHalAudioFocus.unregisterFocusListener();
            }

            if (mAudioControlWrapper != null) {
                mAudioControlWrapper.unlinkToDeath();
                mAudioControlWrapper = null;
            }

            if (mCarAudioPowerListener != null) {
                mCarAudioPowerListener.stopListeningForPolicyChanges();
            }
        }
    }

    @Override
    @ExcludeFromCodeCoverageGeneratedReport(reason = DUMP_INFO)
    public void dump(IndentingPrintWriter writer) {
        writer.println("*CarAudioService*");
        writer.increaseIndent();

        writer.println("Configurations:");
        writer.increaseIndent();
        writer.printf("Run in legacy mode? %b\n", !mUseDynamicRouting);
        writer.printf("Persist master mute state? %b\n", mPersistMasterMuteState);
        writer.printf("Use hal ducking signals %b\n", mUseHalDuckingSignals);
        writer.printf("Volume context priority list version: %d\n",
                mAudioVolumeAdjustmentContextsVersion);
        writer.printf("Volume key event timeout ms: %d\n", mKeyEventTimeoutMs);
        if (mCarAudioConfigurationPath != null) {
            writer.printf("Car audio configuration path: %s\n", mCarAudioConfigurationPath);
        }
        writer.decreaseIndent();
        writer.println();

        writer.println("Current State:");
        writer.increaseIndent();
        writer.printf("Master muted? %b\n", mAudioManager.isMasterMute());
        if (mCarAudioPowerListener != null) {
            writer.printf("Audio enabled? %b\n", mCarAudioPowerListener.isAudioEnabled());
        }
        writer.decreaseIndent();
        writer.println();

        if (mUseDynamicRouting) {
            writer.printf("Volume Group Mute Enabled? %b\n", mUseCarVolumeGroupMuting);
            synchronized (mImplLock) {
                for (int i = 0; i < mCarAudioZones.size(); i++) {
                    CarAudioZone zone = mCarAudioZones.valueAt(i);
                    zone.dump(writer);
                }
            }
            writer.println();
            writer.println("UserId to Zone Mapping:");
            writer.increaseIndent();
            for (int index = 0; index < mAudioZoneIdToUserIdMapping.size(); index++) {
                int audioZoneId = mAudioZoneIdToUserIdMapping.keyAt(index);
                writer.printf("UserId %d mapped to zone %d\n",
                        mAudioZoneIdToUserIdMapping.get(audioZoneId),
                        audioZoneId);
            }
            writer.decreaseIndent();
            writer.println();
            writer.println("Audio Zone to Occupant Zone Mapping:");
            writer.increaseIndent();
            for (int index = 0; index < mAudioZoneIdToOccupantZoneIdMapping.size(); index++) {
                int audioZoneId = mAudioZoneIdToOccupantZoneIdMapping.keyAt(index);
                writer.printf("AudioZoneId %d mapped to OccupantZoneId %d\n", audioZoneId,
                        mAudioZoneIdToOccupantZoneIdMapping.get(audioZoneId));
            }
            writer.decreaseIndent();
            writer.println();
            writer.println("UID to Zone Mapping:");
            writer.increaseIndent();
            for (int callingId : mUidToZoneMap.keySet()) {
                writer.printf("UID %d mapped to zone %d\n",
                        callingId,
                        mUidToZoneMap.get(callingId));
            }
            writer.decreaseIndent();

            writer.println();
            mFocusHandler.dump(writer);

            writer.println();
            getAudioControlWrapperLocked().dump(writer);

            if (mHalAudioFocus != null) {
                writer.println();
                mHalAudioFocus.dump(writer);
            } else {
                writer.println("No HalAudioFocus instance\n");
            }
            if (mCarDucking != null) {
                writer.println();
                mCarDucking.dump(writer);
            }
            if (mCarVolumeGroupMuting != null) {
                mCarVolumeGroupMuting.dump(writer);
            }

        }
        writer.decreaseIndent();
    }

    @Override
    public boolean isAudioFeatureEnabled(@CarAudioFeature int audioFeatureType) {
        switch (audioFeatureType) {
            case AUDIO_FEATURE_DYNAMIC_ROUTING:
                return mUseDynamicRouting;
            case AUDIO_FEATURE_VOLUME_GROUP_MUTING:
                return mUseCarVolumeGroupMuting;
            default:
                throw new IllegalArgumentException("Unknown Audio Feature type: "
                        + audioFeatureType);
        }
    }

    /**
     * @see {@link android.car.media.CarAudioManager#setGroupVolume(int, int, int, int)}
     */
    @Override
    public void setGroupVolume(int zoneId, int groupId, int index, int flags) {
        enforcePermission(Car.PERMISSION_CAR_CONTROL_AUDIO_VOLUME);
        callbackGroupVolumeChange(zoneId, groupId, flags);
        // For legacy stream type based volume control
        if (!mUseDynamicRouting) {
            mAudioManager.setStreamVolume(
                    CarAudioDynamicRouting.STREAM_TYPES[groupId], index, flags);
            return;
        }
        synchronized (mImplLock) {
            CarVolumeGroup group = getCarVolumeGroupLocked(zoneId, groupId);
            group.setCurrentGainIndex(index);
        }
    }

    private void callbackGroupVolumeChange(int zoneId, int groupId, int flags) {
        if (mUseDynamicRouting && !isPlaybackOnVolumeGroupActive(zoneId, groupId)) {
            flags |= FLAG_PLAY_SOUND;
        }
        mCarVolumeCallbackHandler.onVolumeGroupChange(zoneId, groupId, flags);
    }

    private void callbackGroupMuteChanged(int zoneId, int groupId, int flags) {
        mCarVolumeCallbackHandler.onGroupMuteChange(zoneId, groupId, flags);
    }

    void setMasterMute(boolean mute, int flags) {
        mAudioManager.setMasterMute(mute, flags);

        // Master Mute only appliers to primary zone
        callbackMasterMuteChange(PRIMARY_AUDIO_ZONE, flags);

        // When the master mute is turned ON, we want the playing app to get a "pause" command.
        // When the volume is unmuted, we want to resume playback.
        int keycode = mute ? KeyEvent.KEYCODE_MEDIA_PAUSE : KeyEvent.KEYCODE_MEDIA_PLAY;

        dispatchMediaKeyEvent(keycode);
    }

    private void dispatchMediaKeyEvent(int keycode) {
        long currentTime = SystemClock.uptimeMillis();
        KeyEvent keyDown = new KeyEvent(/* downTime= */ currentTime, /* eventTime= */ currentTime,
                KeyEvent.ACTION_DOWN, keycode, /* repeat= */ 0);
        mAudioManager.dispatchMediaKeyEvent(keyDown);

        KeyEvent keyUp = new KeyEvent(/* downTime= */ currentTime, /* eventTime= */ currentTime,
                KeyEvent.ACTION_UP, keycode, /* repeat= */ 0);
        mAudioManager.dispatchMediaKeyEvent(keyUp);
    }

    void callbackMasterMuteChange(int zoneId, int flags) {
        mCarVolumeCallbackHandler.onMasterMuteChanged(zoneId, flags);

        // Persists master mute state if applicable
        if (mPersistMasterMuteState) {
            mCarAudioSettings.storeMasterMute(mAudioManager.isMasterMute());
        }
    }

    /**
     * @see {@link android.car.media.CarAudioManager#getGroupMaxVolume(int, int)}
     */
    @Override
    public int getGroupMaxVolume(int zoneId, int groupId) {
        synchronized (mImplLock) {
            enforcePermission(Car.PERMISSION_CAR_CONTROL_AUDIO_VOLUME);

            // For legacy stream type based volume control
            if (!mUseDynamicRouting) {
                return mAudioManager.getStreamMaxVolume(
                        CarAudioDynamicRouting.STREAM_TYPES[groupId]);
            }

            CarVolumeGroup group = getCarVolumeGroupLocked(zoneId, groupId);
            return group.getMaxGainIndex();
        }
    }

    /**
     * @see {@link android.car.media.CarAudioManager#getGroupMinVolume(int, int)}
     */
    @Override
    public int getGroupMinVolume(int zoneId, int groupId) {
        synchronized (mImplLock) {
            enforcePermission(Car.PERMISSION_CAR_CONTROL_AUDIO_VOLUME);

            // For legacy stream type based volume control
            if (!mUseDynamicRouting) {
                return mAudioManager.getStreamMinVolume(
                        CarAudioDynamicRouting.STREAM_TYPES[groupId]);
            }

            CarVolumeGroup group = getCarVolumeGroupLocked(zoneId, groupId);
            return group.getMinGainIndex();
        }
    }

    /**
     * @see {@link android.car.media.CarAudioManager#getGroupVolume(int, int)}
     */
    @Override
    public int getGroupVolume(int zoneId, int groupId) {
        synchronized (mImplLock) {
            enforcePermission(Car.PERMISSION_CAR_CONTROL_AUDIO_VOLUME);

            // For legacy stream type based volume control
            if (!mUseDynamicRouting) {
                return mAudioManager.getStreamVolume(
                        CarAudioDynamicRouting.STREAM_TYPES[groupId]);
            }

            CarVolumeGroup group = getCarVolumeGroupLocked(zoneId, groupId);
            return group.getCurrentGainIndex();
        }
    }

    @GuardedBy("mImplLock")
    private CarVolumeGroup getCarVolumeGroupLocked(int zoneId, int groupId) {
        return getCarAudioZoneLocked(zoneId).getVolumeGroup(groupId);
    }

    private void setupLegacyVolumeChangedListener() {
        IntentFilter intentFilter = new IntentFilter();
        intentFilter.addAction(AudioManager.VOLUME_CHANGED_ACTION);
        intentFilter.addAction(AudioManager.MASTER_MUTE_CHANGED_ACTION);
        mContext.registerReceiver(mLegacyVolumeChangedReceiver, intentFilter);
    }

    private List<CarAudioDeviceInfo> generateCarAudioDeviceInfos() {
        AudioDeviceInfo[] deviceInfos = mAudioManager.getDevices(
                AudioManager.GET_DEVICES_OUTPUTS);

        return Arrays.stream(deviceInfos)
                .filter(info -> info.getType() == AudioDeviceInfo.TYPE_BUS)
                .map(CarAudioDeviceInfo::new)
                .collect(Collectors.toList());
    }

    private AudioDeviceInfo[] getAllInputDevices() {
        return mAudioManager.getDevices(
                AudioManager.GET_DEVICES_INPUTS);
    }

    private SparseArray<CarAudioZone> loadCarAudioConfigurationLocked(
            List<CarAudioDeviceInfo> carAudioDeviceInfos) {
        AudioDeviceInfo[] inputDevices = getAllInputDevices();
        try (InputStream inputStream = new FileInputStream(mCarAudioConfigurationPath)) {
            CarAudioZonesHelper zonesHelper = new CarAudioZonesHelper(mCarAudioSettings,
                    inputStream, carAudioDeviceInfos, inputDevices, mUseCarVolumeGroupMuting);
            mAudioZoneIdToOccupantZoneIdMapping =
                    zonesHelper.getCarAudioZoneIdToOccupantZoneIdMapping();
            return zonesHelper.loadAudioZones();
        } catch (IOException | XmlPullParserException e) {
            throw new RuntimeException("Failed to parse audio zone configuration", e);
        }
    }

    private SparseArray<CarAudioZone> loadVolumeGroupConfigurationWithAudioControlLocked(
            List<CarAudioDeviceInfo> carAudioDeviceInfos) {
        AudioControlWrapper audioControlWrapper = getAudioControlWrapperLocked();
        if (!(audioControlWrapper instanceof AudioControlWrapperV1)) {
            throw new IllegalStateException(
                    "Updated version of IAudioControl no longer supports CarAudioZonesHelperLegacy."
                    + " Please provide car_audio_configuration.xml.");
        }
        CarAudioZonesHelperLegacy legacyHelper = new CarAudioZonesHelperLegacy(mContext,
                R.xml.car_volume_groups, carAudioDeviceInfos,
                (AudioControlWrapperV1) audioControlWrapper, mCarAudioSettings);
        return legacyHelper.loadAudioZones();
    }

    @GuardedBy("mImplLock")
    private void loadCarAudioZonesLocked() {
        List<CarAudioDeviceInfo> carAudioDeviceInfos = generateCarAudioDeviceInfos();

        mCarAudioConfigurationPath = getAudioConfigurationPath();
        if (mCarAudioConfigurationPath != null) {
            mCarAudioZones = loadCarAudioConfigurationLocked(carAudioDeviceInfos);
        } else {
            mCarAudioZones = loadVolumeGroupConfigurationWithAudioControlLocked(
                    carAudioDeviceInfos);
        }

        CarAudioZonesValidator.validate(mCarAudioZones);
    }

    @GuardedBy("mImplLock")
    private void setupDynamicRoutingLocked() {
        final AudioPolicy.Builder builder = new AudioPolicy.Builder(mContext);
        builder.setLooper(Looper.getMainLooper());

        loadCarAudioZonesLocked();

        for (int i = 0; i < mCarAudioZones.size(); i++) {
            CarAudioZone zone = mCarAudioZones.valueAt(i);
            // Ensure HAL gets our initial value
            zone.synchronizeCurrentGainIndex();
            Slog.v(CarLog.TAG_AUDIO, "Processed audio zone: " + zone);
        }

        CarAudioDynamicRouting.setupAudioDynamicRouting(builder, mCarAudioZones);

        // Attach the {@link AudioPolicyVolumeCallback}
        CarAudioPolicyVolumeCallback
                .addVolumeCallbackToPolicy(builder, this, mAudioManager,
                        mUseCarVolumeGroupMuting);


        AudioControlWrapper audioControlWrapper = getAudioControlWrapperLocked();
        if (mUseHalDuckingSignals) {
            if (audioControlWrapper.supportsFeature(AUDIOCONTROL_FEATURE_AUDIO_DUCKING)) {
                mCarDucking = new CarDucking(mCarAudioZones, audioControlWrapper);
            }
        }

        if (mUseCarVolumeGroupMuting) {
            mCarVolumeGroupMuting = new CarVolumeGroupMuting(mCarAudioZones, audioControlWrapper);
        }

        // Configure our AudioPolicy to handle focus events.
        // This gives us the ability to decide which audio focus requests to accept and bypasses
        // the framework ducking logic.
        mFocusHandler = CarZonesAudioFocus.createCarZonesAudioFocus(mAudioManager,
                mContext.getPackageManager(),
                mCarAudioZones,
                mCarAudioSettings,
                ENABLE_DELAYED_AUDIO_FOCUS,
                mCarDucking);
        builder.setAudioPolicyFocusListener(mFocusHandler);
        builder.setIsAudioFocusPolicy(true);

        mAudioPolicy = builder.build();

        // Connect the AudioPolicy and the focus listener
        mFocusHandler.setOwningPolicy(this, mAudioPolicy);

        int r = mAudioManager.registerAudioPolicy(mAudioPolicy);
        if (r != AudioManager.SUCCESS) {
            throw new RuntimeException("registerAudioPolicy failed " + r);
        }

        setupOccupantZoneInfo();
    }

    private void setupAudioConfigurationCallbackLocked() {
        mCarAudioPlaybackCallback =
                new CarAudioPlaybackCallback(getCarAudioZone(PRIMARY_AUDIO_ZONE),
                        mClock, mKeyEventTimeoutMs);
        mAudioManager.registerAudioPlaybackCallback(mCarAudioPlaybackCallback, null);
    }

    private void setupOccupantZoneInfo() {
        CarOccupantZoneService occupantZoneService;
        CarOccupantZoneManager occupantZoneManager;
        SparseIntArray audioZoneIdToOccupantZoneMapping;
        OccupantZoneConfigChangeListener listener;
        synchronized (mImplLock) {
            audioZoneIdToOccupantZoneMapping = mAudioZoneIdToOccupantZoneIdMapping;
            occupantZoneService = mOccupantZoneService;
            occupantZoneManager = mOccupantZoneManager;
            listener = mOccupantZoneConfigChangeListener;
        }
        occupantZoneService.setAudioZoneIdsForOccupantZoneIds(audioZoneIdToOccupantZoneMapping);
        occupantZoneManager.registerOccupantZoneConfigChangeListener(listener);
    }

    private void setupHalAudioFocusListenerLocked() {
        AudioControlWrapper audioControlWrapper = getAudioControlWrapperLocked();
        if (!audioControlWrapper.supportsFeature(AUDIOCONTROL_FEATURE_AUDIO_FOCUS)) {
            Slog.d(CarLog.TAG_AUDIO, "HalAudioFocus is not supported on this device");
            return;
        }

        mHalAudioFocus = new HalAudioFocus(mAudioManager, mAudioControlWrapper, getAudioZoneIds());
        mHalAudioFocus.registerFocusListener();
    }

    /**
     * Read from {@link #AUDIO_CONFIGURATION_PATHS} respectively.
     * @return File path of the first hit in {@link #AUDIO_CONFIGURATION_PATHS}
     */
    @Nullable
    private String getAudioConfigurationPath() {
        for (String path : AUDIO_CONFIGURATION_PATHS) {
            File configuration = new File(path);
            if (configuration.exists()) {
                return path;
            }
        }
        return null;
    }

    @Override
    public void setFadeTowardFront(float value) {
        synchronized (mImplLock) {
            enforcePermission(Car.PERMISSION_CAR_CONTROL_AUDIO_VOLUME);
            getAudioControlWrapperLocked().setFadeTowardFront(value);
        }
    }

    @Override
    public void setBalanceTowardRight(float value) {
        synchronized (mImplLock) {
            enforcePermission(Car.PERMISSION_CAR_CONTROL_AUDIO_VOLUME);
            getAudioControlWrapperLocked().setBalanceTowardRight(value);
        }
    }

    /**
     * @return Array of accumulated device addresses, empty array if we found nothing
     */
    @Override
    public @NonNull String[] getExternalSources() {
        synchronized (mImplLock) {
            enforcePermission(Car.PERMISSION_CAR_CONTROL_AUDIO_SETTINGS);
            List<String> sourceAddresses = new ArrayList<>();

            AudioDeviceInfo[] devices = mAudioManager.getDevices(AudioManager.GET_DEVICES_INPUTS);
            if (devices.length == 0) {
                Slog.w(CarLog.TAG_AUDIO, "getExternalSources, no input devices found.");
            }

            // Collect the list of non-microphone input ports
            for (AudioDeviceInfo info : devices) {
                switch (info.getType()) {
                    // TODO:  Can we trim this set down? Especially duplicates like FM vs FM_TUNER?
                    case AudioDeviceInfo.TYPE_FM:
                    case AudioDeviceInfo.TYPE_FM_TUNER:
                    case AudioDeviceInfo.TYPE_TV_TUNER:
                    case AudioDeviceInfo.TYPE_HDMI:
                    case AudioDeviceInfo.TYPE_AUX_LINE:
                    case AudioDeviceInfo.TYPE_LINE_ANALOG:
                    case AudioDeviceInfo.TYPE_LINE_DIGITAL:
                    case AudioDeviceInfo.TYPE_USB_ACCESSORY:
                    case AudioDeviceInfo.TYPE_USB_DEVICE:
                    case AudioDeviceInfo.TYPE_USB_HEADSET:
                    case AudioDeviceInfo.TYPE_IP:
                    case AudioDeviceInfo.TYPE_BUS:
                        String address = info.getAddress();
                        if (TextUtils.isEmpty(address)) {
                            Slog.w(CarLog.TAG_AUDIO,
                                    "Discarded device with empty address, type=" + info.getType());
                        } else {
                            sourceAddresses.add(address);
                        }
                }
            }

            return sourceAddresses.toArray(new String[0]);
        }
    }

    @Override
    public CarAudioPatchHandle createAudioPatch(String sourceAddress,
            @AttributeUsage int usage, int gainInMillibels) {
        synchronized (mImplLock) {
            enforcePermission(Car.PERMISSION_CAR_CONTROL_AUDIO_SETTINGS);
            return createAudioPatchLocked(sourceAddress, usage, gainInMillibels);
        }
    }

    @Override
    public void releaseAudioPatch(CarAudioPatchHandle carPatch) {
        synchronized (mImplLock) {
            enforcePermission(Car.PERMISSION_CAR_CONTROL_AUDIO_SETTINGS);
            releaseAudioPatchLocked(carPatch);
        }
    }

    private CarAudioPatchHandle createAudioPatchLocked(String sourceAddress,
            @AttributeUsage int usage, int gainInMillibels) {
        // Find the named source port
        AudioDeviceInfo sourcePortInfo = null;
        AudioDeviceInfo[] deviceInfos = mAudioManager.getDevices(AudioManager.GET_DEVICES_INPUTS);
        for (AudioDeviceInfo info : deviceInfos) {
            if (sourceAddress.equals(info.getAddress())) {
                // This is the one for which we're looking
                sourcePortInfo = info;
                break;
            }
        }
        Objects.requireNonNull(sourcePortInfo,
                "Specified source is not available: " + sourceAddress);

        // Find the output port associated with the given carUsage
        AudioDevicePort sinkPort = Objects.requireNonNull(getAudioPort(usage),
                "Sink not available for usage: " + AudioAttributes.usageToString(usage));

        // {@link android.media.AudioPort#activeConfig()} is valid for mixer port only,
        // since audio framework has no clue what's active on the device ports.
        // Therefore we construct an empty / default configuration here, which the audio HAL
        // implementation should ignore.
        AudioPortConfig sinkConfig = sinkPort.buildConfig(0,
                AudioFormat.CHANNEL_OUT_DEFAULT, AudioFormat.ENCODING_DEFAULT, null);
        Slog.d(CarLog.TAG_AUDIO, "createAudioPatch sinkConfig: " + sinkConfig);

        // Configure the source port to match the output port except for a gain adjustment
        final CarAudioDeviceInfo helper = new CarAudioDeviceInfo(sourcePortInfo);
        AudioGain audioGain = Objects.requireNonNull(helper.getAudioGain(),
                "Gain controller not available for source port");

        // size of gain values is 1 in MODE_JOINT
        AudioGainConfig audioGainConfig = audioGain.buildConfig(AudioGain.MODE_JOINT,
                audioGain.channelMask(), new int[] { gainInMillibels }, 0);
        // Construct an empty / default configuration excepts gain config here and it's up to the
        // audio HAL how to interpret this configuration, which the audio HAL
        // implementation should ignore.
        AudioPortConfig sourceConfig = sourcePortInfo.getPort().buildConfig(0,
                AudioFormat.CHANNEL_IN_DEFAULT, AudioFormat.ENCODING_DEFAULT, audioGainConfig);

        // Create an audioPatch to connect the two ports
        AudioPatch[] patch = new AudioPatch[] { null };
        int result = AudioManager.createAudioPatch(patch,
                new AudioPortConfig[] { sourceConfig },
                new AudioPortConfig[] { sinkConfig });
        if (result != AudioManager.SUCCESS) {
            throw new RuntimeException("createAudioPatch failed with code " + result);
        }

        Objects.requireNonNull(patch[0],
                "createAudioPatch didn't provide expected single handle");
        Slog.d(CarLog.TAG_AUDIO, "Audio patch created: " + patch[0]);

        // Ensure the initial volume on output device port
        int groupId = getVolumeGroupIdForUsage(CarAudioManager.PRIMARY_AUDIO_ZONE, usage);
        setGroupVolume(CarAudioManager.PRIMARY_AUDIO_ZONE, groupId,
                getGroupVolume(CarAudioManager.PRIMARY_AUDIO_ZONE, groupId), 0);

        return new CarAudioPatchHandle(patch[0]);
    }

    private void releaseAudioPatchLocked(CarAudioPatchHandle carPatch) {
        Objects.requireNonNull(carPatch);
        // NOTE:  AudioPolicyService::removeNotificationClient will take care of this automatically
        //        if the client that created a patch quits.
        ArrayList<AudioPatch> patches = new ArrayList<>();
        int result = mAudioManager.listAudioPatches(patches);
        if (result != AudioManager.SUCCESS) {
            throw new RuntimeException("listAudioPatches failed with code " + result);
        }

        // Look for a patch that matches the provided user side handle
        for (AudioPatch patch : patches) {
            if (carPatch.represents(patch)) {
                // Found it!
                result = AudioManager.releaseAudioPatch(patch);
                if (result != AudioManager.SUCCESS) {
                    throw new RuntimeException("releaseAudioPatch failed with code " + result);
                }
                return;
            }
        }

        // If we didn't find a match, then something went awry, but it's probably not fatal...
        Slog.e(CarLog.TAG_AUDIO, "releaseAudioPatch found no match for " + carPatch);
    }

    @Override
    public int getVolumeGroupCount(int zoneId) {
        synchronized (mImplLock) {
            enforcePermission(Car.PERMISSION_CAR_CONTROL_AUDIO_VOLUME);
            // For legacy stream type based volume control
            if (!mUseDynamicRouting) return CarAudioDynamicRouting.STREAM_TYPES.length;

            return getCarAudioZoneLocked(zoneId).getVolumeGroupCount();
        }
    }

    @Override
    public int getVolumeGroupIdForUsage(int zoneId, @AttributeUsage int usage) {
        synchronized (mImplLock) {
            enforcePermission(Car.PERMISSION_CAR_CONTROL_AUDIO_VOLUME);

            if (!mUseDynamicRouting) {
                for (int i = 0; i < CarAudioDynamicRouting.STREAM_TYPE_USAGES.length; i++) {
                    if (usage == CarAudioDynamicRouting.STREAM_TYPE_USAGES[i]) {
                        return i;
                    }
                }

                return INVALID_VOLUME_GROUP_ID;
            }

            @AudioContext int audioContext = CarAudioContext.getContextForUsage(usage);
            return getVolumeGroupIdForAudioContextLocked(zoneId, audioContext);
        }
    }

    @GuardedBy("mImplLock")
    private int getVolumeGroupIdForAudioContextLocked(int zoneId, @AudioContext int audioContext) {
        CarVolumeGroup[] groups = getCarAudioZoneLocked(zoneId).getVolumeGroups();
        for (int i = 0; i < groups.length; i++) {
            int[] groupAudioContexts = groups[i].getContexts();
            for (int groupAudioContext : groupAudioContexts) {
                if (audioContext == groupAudioContext) {
                    return i;
                }
            }
        }
        return INVALID_VOLUME_GROUP_ID;
    }

    @Override
    public @NonNull int[] getUsagesForVolumeGroupId(int zoneId, int groupId) {
        synchronized (mImplLock) {
            enforcePermission(Car.PERMISSION_CAR_CONTROL_AUDIO_VOLUME);

            // For legacy stream type based volume control
            if (!mUseDynamicRouting) {
                return new int[] { CarAudioDynamicRouting.STREAM_TYPE_USAGES[groupId] };
            }

            CarVolumeGroup group = getCarVolumeGroupLocked(zoneId, groupId);
            Set<Integer> contexts =
                    Arrays.stream(group.getContexts()).boxed().collect(Collectors.toSet());
            final List<Integer> usages = new ArrayList<>();
            for (@AudioContext int context : contexts) {
                int[] usagesForContext = CarAudioContext.getUsagesForContext(context);
                for (@AttributeUsage int usage : usagesForContext) {
                    usages.add(usage);
                }
            }
            return usages.stream().mapToInt(i -> i).toArray();
        }
    }

    @Override
    public boolean isPlaybackOnVolumeGroupActive(int zoneId, int groupId) {
        enforcePermission(Car.PERMISSION_CAR_CONTROL_AUDIO_VOLUME);
        requireDynamicRouting();
        Preconditions.checkArgument(isAudioZoneIdValid(zoneId),
                "Invalid audio zone id %d", zoneId);

        return CarVolume.isAnyContextActive(getContextsForVolumeGroupId(zoneId, groupId),
                getActiveContextsFromPlaybackConfigurations(zoneId), getCallStateForZone(zoneId),
                getActiveHalUsagesForZone(zoneId));
    }

    private @CallState int getCallStateForZone(int zoneId) {
        synchronized (mImplLock) {
            // Only driver can use telephony stack
            if (getUserIdForZoneLocked(zoneId) == mOccupantZoneService.getDriverUserId()) {
                return mTelephonyManager.getCallState();
            }
        }
        return TelephonyManager.CALL_STATE_IDLE;
    }

    private List<Integer> getActiveContextsFromPlaybackConfigurations(int zoneId) {
        List<AudioPlaybackConfiguration> configurations = mAudioManager
                .getActivePlaybackConfigurations();
        return getCarAudioZone(zoneId).findActiveContextsFromPlaybackConfigurations(configurations);
    }


    private @NonNull @AudioContext int[] getContextsForVolumeGroupId(int zoneId, int groupId) {
        synchronized (mImplLock) {
            CarVolumeGroup group = getCarVolumeGroupLocked(zoneId, groupId);
            return group.getContexts();
        }
    }

    /**
     * Gets the ids of all available audio zones
     *
     * @return Array of available audio zones ids
     */
    @Override
    public @NonNull int[] getAudioZoneIds() {
        enforcePermission(Car.PERMISSION_CAR_CONTROL_AUDIO_SETTINGS);
        requireDynamicRouting();
        synchronized (mImplLock) {
            int[] zoneIds = new int[mCarAudioZones.size()];
            for (int i = 0; i < mCarAudioZones.size(); i++) {
                zoneIds[i] = mCarAudioZones.keyAt(i);
            }
            return zoneIds;
        }
    }

    /**
     * Gets the audio zone id currently mapped to uid,
     *
     * <p><b>Note:</b> Will use uid mapping first, followed by uid's {@userId} mapping.
     * defaults to PRIMARY_AUDIO_ZONE if no mapping exist
     *
     * @param uid The uid
     * @return zone id mapped to uid
     */
    @Override
    public int getZoneIdForUid(int uid) {
        enforcePermission(Car.PERMISSION_CAR_CONTROL_AUDIO_SETTINGS);
        requireDynamicRouting();
        synchronized (mImplLock) {
            if (mUidToZoneMap.containsKey(uid)) {
                return mUidToZoneMap.get(uid);
            }
            int userId = UserHandle.getUserId(uid);
            return getZoneIdForUserIdLocked(userId);
        }
    }

    private int getZoneIdForUserIdLocked(@UserIdInt int userId) {
        int audioZoneId = mOccupantZoneService.getAudioZoneIdForOccupant(
                mOccupantZoneService.getOccupantZoneIdForUserId(userId));
        if (audioZoneId != CarAudioManager.INVALID_AUDIO_ZONE) {
            return audioZoneId;
        }
        Slog.w(CarLog.TAG_AUDIO,
                "getZoneIdForUid userId " + userId
                        + " does not have a zone. Defaulting to PRIMARY_AUDIO_ZONE:"
                        + CarAudioManager.PRIMARY_AUDIO_ZONE);
        return CarAudioManager.PRIMARY_AUDIO_ZONE;
    }

    /**
     * Maps the audio zone id to uid
     *
     * @param zoneId The audio zone id
     * @param uid The uid to map
     *
     * <p><b>Note:</b> Will throw if occupant zone mapping exist, as uid and occupant zone mapping
     * do not work in conjunction.
     *
     * @return true if the device affinities, for devices in zone, are successfully set
     */
    @Override
    public boolean setZoneIdForUid(int zoneId, int uid) {
        enforcePermission(Car.PERMISSION_CAR_CONTROL_AUDIO_SETTINGS);
        requireDynamicRouting();
        synchronized (mImplLock) {
            checkAudioZoneIdLocked(zoneId);
            Slog.i(CarLog.TAG_AUDIO, "setZoneIdForUid Calling uid "
                    + uid + " mapped to : "
                    + zoneId);

            // If occupant mapping exist uid routing can not be used
            requiredOccupantZoneMappingDisabledLocked();

            // Figure out if anything is currently holding focus,
            // This will change the focus to transient loss while we are switching zones
            Integer currentZoneId = mUidToZoneMap.get(uid);
            ArrayList<AudioFocusInfo> currentFocusHoldersForUid = new ArrayList<>();
            ArrayList<AudioFocusInfo> currentFocusLosersForUid = new ArrayList<>();
            if (currentZoneId != null) {
                currentFocusHoldersForUid = mFocusHandler.getAudioFocusHoldersForUid(uid,
                        currentZoneId.intValue());
                currentFocusLosersForUid = mFocusHandler.getAudioFocusLosersForUid(uid,
                        currentZoneId.intValue());
                if (!currentFocusHoldersForUid.isEmpty() || !currentFocusLosersForUid.isEmpty()) {
                    // Order matters here: Remove the focus losers first
                    // then do the current holder to prevent loser from popping up while
                    // the focus is being remove for current holders
                    // Remove focus for current focus losers
                    mFocusHandler.transientlyLoseInFocusInZone(currentFocusLosersForUid,
                            currentZoneId.intValue());
                    // Remove focus for current holders
                    mFocusHandler.transientlyLoseInFocusInZone(currentFocusHoldersForUid,
                            currentZoneId.intValue());
                }
            }

            // if the current uid is in the list
            // remove it from the list

            if (checkAndRemoveUidLocked(uid)) {
                if (setZoneIdForUidNoCheckLocked(zoneId, uid)) {
                    // Order matters here: Regain focus for
                    // Previously lost focus holders then regain
                    // focus for holders that had it last
                    // Regain focus for the focus losers from previous zone
                    if (!currentFocusLosersForUid.isEmpty()) {
                        regainAudioFocusLocked(currentFocusLosersForUid, zoneId);
                    }
                    // Regain focus for the focus holders from previous zone
                    if (!currentFocusHoldersForUid.isEmpty()) {
                        regainAudioFocusLocked(currentFocusHoldersForUid, zoneId);
                    }
                    return true;
                }
            }
            return false;
        }
    }

    @Override
    public String getOutputDeviceAddressForUsage(int zoneId, @AttributeUsage int usage) {
        enforcePermission(Car.PERMISSION_CAR_CONTROL_AUDIO_SETTINGS);
        requireDynamicRouting();
        int contextForUsage = CarAudioContext.getContextForUsage(usage);
        Preconditions.checkArgument(contextForUsage != CarAudioContext.INVALID,
                "Invalid audio attribute usage %d", usage);
        return getCarAudioZone(zoneId).getAddressForContext(contextForUsage);
    }

    /**
     * Regain focus for the focus list passed in
     * @param afiList focus info list to regain
     * @param zoneId zone id where the focus holder belong
     */
    void regainAudioFocusLocked(ArrayList<AudioFocusInfo> afiList, int zoneId) {
        for (AudioFocusInfo info : afiList) {
            if (mFocusHandler.reevaluateAndRegainAudioFocus(info)
                    != AudioManager.AUDIOFOCUS_REQUEST_GRANTED) {
                Slog.i(CarLog.TAG_AUDIO,
                        " Focus could not be granted for entry "
                                + info.getClientId()
                                + " uid " + info.getClientUid()
                                + " in zone " + zoneId);
            }
        }
    }

    /**
     * Removes the current mapping of the uid, focus will be lost in zone
     * @param uid The uid to remove
     *
     * <p><b>Note:</b> Will throw if occupant zone mapping exist, as uid and occupant zone mapping
     * do not work in conjunction.
     *
     * return true if all the devices affinities currently
     *            mapped to uid are successfully removed
     */
    @Override
    public boolean clearZoneIdForUid(int uid) {
        enforcePermission(Car.PERMISSION_CAR_CONTROL_AUDIO_SETTINGS);
        requireDynamicRouting();
        synchronized (mImplLock) {
            // Throw so as to not set the wrong expectation,
            // that routing will be changed if clearZoneIdForUid is called.
            requiredOccupantZoneMappingDisabledLocked();

            return checkAndRemoveUidLocked(uid);
        }
    }

    /**
     * Sets the zone id for uid
     * @param zoneId zone id to map to uid
     * @param uid uid to map
     * @return true if setting uid device affinity is successful
     */
    @GuardedBy("mImplLock")
    private boolean setZoneIdForUidNoCheckLocked(int zoneId, int uid) {
        Slog.d(CarLog.TAG_AUDIO, "setZoneIdForUidNoCheck Calling uid "
                + uid + " mapped to " + zoneId);
        //Request to add uid device affinity
        List<AudioDeviceInfo> deviceInfos = getCarAudioZoneLocked(zoneId).getAudioDeviceInfos();
        if (mAudioPolicy.setUidDeviceAffinity(uid, deviceInfos)) {
            // TODO do not store uid mapping here instead use the uid
            //  device affinity in audio policy when available
            mUidToZoneMap.put(uid, zoneId);
            return true;
        }
        Slog.w(CarLog.TAG_AUDIO, "setZoneIdForUidNoCheck Failed set device affinity for uid "
                + uid + " in zone " + zoneId);
        return false;
    }

    /**
     * Check if uid is attached to a zone and remove it
     * @param uid unique id to remove
     * @return true if the uid was successfully removed or mapping was not assigned
     */
    private boolean checkAndRemoveUidLocked(int uid) {
        Integer zoneId = mUidToZoneMap.get(uid);
        if (zoneId != null) {
            Slog.i(CarLog.TAG_AUDIO, "checkAndRemoveUid removing Calling uid "
                    + uid + " from zone " + zoneId);
            if (mAudioPolicy.removeUidDeviceAffinity(uid)) {
                // TODO use the uid device affinity in audio policy when available
                mUidToZoneMap.remove(uid);
                return true;
            }
            //failed to remove device affinity from zone devices
            Slog.w(CarLog.TAG_AUDIO,
                    "checkAndRemoveUid Failed remove device affinity for uid "
                            + uid + " in zone " +  zoneId);
            return false;
        }
        return true;
    }

    @Override
    public void registerVolumeCallback(@NonNull IBinder binder) {
        synchronized (mImplLock) {
            enforcePermission(Car.PERMISSION_CAR_CONTROL_AUDIO_VOLUME);
            mCarVolumeCallbackHandler.registerCallback(binder);
        }
    }

    @Override
    public void unregisterVolumeCallback(@NonNull IBinder binder) {
        synchronized (mImplLock) {
            enforcePermission(Car.PERMISSION_CAR_CONTROL_AUDIO_VOLUME);
            mCarVolumeCallbackHandler.unregisterCallback(binder);
        }
    }

    /**
     * @see {@link android.car.media.CarAudioManager#isVolumeGroupMuted(int, int)}
     */
    @Override
    public boolean isVolumeGroupMuted(int zoneId, int groupId) {
        enforcePermission(Car.PERMISSION_CAR_CONTROL_AUDIO_VOLUME);
        requireDynamicRouting();
        if (!mUseCarVolumeGroupMuting) {
            return false;
        }
        synchronized (mImplLock) {
            CarVolumeGroup group = getCarVolumeGroupLocked(zoneId, groupId);
            return group.isMuted();
        }
    }

    /**
     * @see {@link android.car.media.CarAudioManager#setVolumeGroupMute(int, int, boolean, int)}
     */
    @Override
    public void setVolumeGroupMute(int zoneId, int groupId, boolean mute, int flags) {
        enforcePermission(Car.PERMISSION_CAR_CONTROL_AUDIO_VOLUME);
        requireDynamicRouting();
        requireVolumeGroupMuting();
        synchronized (mImplLock) {
            CarVolumeGroup group = getCarVolumeGroupLocked(zoneId, groupId);
            group.setMute(mute);
            callbackGroupMuteChanged(zoneId, groupId, flags);
        }
        mCarVolumeGroupMuting.carMuteChanged();
    }

    @Override
    public @NonNull List<AudioDeviceAttributes> getInputDevicesForZoneId(int zoneId) {
        enforcePermission(Car.PERMISSION_CAR_CONTROL_AUDIO_SETTINGS);
        requireDynamicRouting();

        return getCarAudioZone(zoneId).getInputAudioDevices();
    }

    void setAudioEnabled(boolean isAudioEnabled) {
        if (Slogf.isLoggable(CarLog.TAG_AUDIO, Log.DEBUG)) {
            Slogf.d(CarLog.TAG_AUDIO, "Setting isAudioEnabled to %b", isAudioEnabled);
        }

        mFocusHandler.setRestrictFocus(/* isFocusRestricted= */ !isAudioEnabled);
        if (mUseCarVolumeGroupMuting) {
            mCarVolumeGroupMuting.setRestrictMuting(/* isMutingRestricted= */ !isAudioEnabled);
        }
        // TODO(b/176258537) if not using group volume, then set master mute accordingly
    }

    private void enforcePermission(String permissionName) {
        if (mContext.checkCallingOrSelfPermission(permissionName)
                != PackageManager.PERMISSION_GRANTED) {
            throw new SecurityException("requires permission " + permissionName);
        }
    }

    private void requireDynamicRouting() {
        Preconditions.checkState(mUseDynamicRouting, "Dynamic routing is required");
    }

    private void requireVolumeGroupMuting() {
        Preconditions.checkState(mUseCarVolumeGroupMuting,
                "Car Volume Group Muting is required");
    }

    private void requiredOccupantZoneMappingDisabledLocked() {
        if (isOccupantZoneMappingAvailableLocked()) {
            throw new IllegalStateException(
                    "UID based routing is not supported while using occupant zone mapping");
        }
    }

    /**
     * @return {@link AudioDevicePort} that handles the given car audio usage.
     * Multiple usages may share one {@link AudioDevicePort}
     */
    private @Nullable AudioDevicePort getAudioPort(@AttributeUsage int usage) {
        int zoneId = CarAudioManager.PRIMARY_AUDIO_ZONE;
        final int groupId = getVolumeGroupIdForUsage(zoneId, usage);
        final CarVolumeGroup group = Objects.requireNonNull(
                getCarVolumeGroupLocked(zoneId, groupId),
                "Can not find CarVolumeGroup by usage: "
                        + AudioAttributes.usageToString(usage));
        return group.getAudioDevicePortForContext(CarAudioContext.getContextForUsage(usage));
    }

    @AudioContext int getSuggestedAudioContextForPrimaryZone() {
        int zoneId = PRIMARY_AUDIO_ZONE;
        return mCarVolume.getSuggestedAudioContextAndSaveIfFound(
                getAllActiveContextsForPrimaryZone(), getCallStateForZone(zoneId),
                getActiveHalUsagesForZone(zoneId));
    }

    private int[] getActiveHalUsagesForZone(int zoneId) {
        if (mHalAudioFocus == null) {
            return new int[0];
        }
        return mHalAudioFocus.getActiveUsagesForZone(zoneId);
    }

    /**
     * Gets volume group by a given legacy stream type
     * @param streamType Legacy stream type such as {@link AudioManager#STREAM_MUSIC}
     * @return volume group id mapped from stream type
     */
    private int getVolumeGroupIdForStreamType(int streamType) {
        int groupId = INVALID_VOLUME_GROUP_ID;
        for (int i = 0; i < CarAudioDynamicRouting.STREAM_TYPES.length; i++) {
            if (streamType == CarAudioDynamicRouting.STREAM_TYPES[i]) {
                groupId = i;
                break;
            }
        }
        return groupId;
    }

    private void handleOccupantZoneUserChanged() {
        int driverUserId = mOccupantZoneService.getDriverUserId();
        synchronized (mImplLock) {
            if (!isOccupantZoneMappingAvailableLocked()) {
                adjustZonesToUserIdLocked(driverUserId);
                return;
            }
            int occupantZoneForDriver =  getOccupantZoneIdForDriver();
            Set<Integer> assignedZones = new HashSet<Integer>();
            for (int index = 0; index < mAudioZoneIdToOccupantZoneIdMapping.size(); index++) {
                int audioZoneId = mAudioZoneIdToOccupantZoneIdMapping.keyAt(index);
                int occupantZoneId = mAudioZoneIdToOccupantZoneIdMapping.get(audioZoneId);
                assignedZones.add(audioZoneId);
                updateUserForOccupantZoneLocked(occupantZoneId, audioZoneId, driverUserId,
                        occupantZoneForDriver);
            }

            assignMissingZonesToDriverLocked(driverUserId, assignedZones);
        }
        restoreVolumeGroupMuteState();
    }

    private void restoreVolumeGroupMuteState() {
        if (!mUseCarVolumeGroupMuting) {
            return;
        }
        mCarVolumeGroupMuting.carMuteChanged();
    }

    private void assignMissingZonesToDriverLocked(@UserIdInt int driverUserId,
            Set<Integer> assignedZones) {
        for (int i = 0; i < mCarAudioZones.size(); i++) {
            CarAudioZone zone = mCarAudioZones.valueAt(i);
            if (assignedZones.contains(zone.getId())) {
                continue;
            }
            assignUserIdToAudioZoneLocked(zone, driverUserId);
        }
    }

    private void adjustZonesToUserIdLocked(@UserIdInt int userId) {
        for (int i = 0; i < mCarAudioZones.size(); i++) {
            CarAudioZone zone = mCarAudioZones.valueAt(i);
            assignUserIdToAudioZoneLocked(zone, userId);
        }
    }

    private void assignUserIdToAudioZoneLocked(CarAudioZone zone, @UserIdInt int userId) {
        if (userId == getUserIdForZoneLocked(zone.getId())) {
            if (Log.isLoggable(CarLog.TAG_AUDIO, Log.DEBUG)) {
                Slog.d(CarLog.TAG_AUDIO,
                        "assignUserIdToAudioZone userId(" + userId
                                + ") already assigned to audioZoneId("
                                + zone.getId() + ")");
            }
            return;
        }
        if (Log.isLoggable(CarLog.TAG_AUDIO, Log.DEBUG)) {
            Slog.d(CarLog.TAG_AUDIO,
                    "assignUserIdToAudioZone assigning userId("
                            + userId + ") to audioZoneId("
                            + zone.getId() + ")");
        }
        zone.updateVolumeGroupsSettingsForUser(userId);
        mFocusHandler.updateUserForZoneId(zone.getId(), userId);
        setUserIdForAudioZoneLocked(userId, zone.getId());
    }

    private boolean isOccupantZoneMappingAvailableLocked() {
        return mAudioZoneIdToOccupantZoneIdMapping.size() > 0;
    }

    @GuardedBy("mImplLock")
    private void updateUserForOccupantZoneLocked(int occupantZoneId, int audioZoneId,
            @UserIdInt int driverUserId, int occupantZoneForDriver) {
        CarAudioZone audioZone = getCarAudioZoneLocked(audioZoneId);
        int userId = mOccupantZoneService.getUserForOccupant(occupantZoneId);
        int prevUserId = getUserIdForZoneLocked(audioZoneId);

        if (userId == prevUserId) {
            if (Log.isLoggable(CarLog.TAG_AUDIO, Log.DEBUG)) {
                Slog.d(CarLog.TAG_AUDIO, "updateUserForOccupantZone userId(" + userId
                        + ") already assigned to audioZoneId(" + audioZoneId + ")");
            }
            return;
        }
        if (Log.isLoggable(CarLog.TAG_AUDIO, Log.DEBUG)) {
            Slog.d(CarLog.TAG_AUDIO,
                    "updateUserForOccupantZone assigning userId("
                            + userId + ") to audioZoneId(" + audioZoneId + ")");
        }
        // If the user has changed, be sure to remove from current routing
        // This would be true even if the new user is UserHandle.USER_NULL,
        // as that indicates the user has logged out.
        removeUserIdDeviceAffinitiesLocked(prevUserId);

        if (userId == UserHandle.USER_NULL) {
            // Reset zone back to driver user id
            resetZoneToDefaultUser(audioZone, driverUserId);
            setUserIdForAudioZoneLocked(userId, audioZoneId);
            return;
        }

        // Only set user id device affinities for driver when it is the driver's occupant zone
        if (userId != driverUserId || occupantZoneId == occupantZoneForDriver) {
            setUserIdDeviceAffinitiesLocked(audioZone, userId, audioZoneId);
        }
        audioZone.updateVolumeGroupsSettingsForUser(userId);
        mFocusHandler.updateUserForZoneId(audioZoneId, userId);
        setUserIdForAudioZoneLocked(userId, audioZoneId);
    }

    private int getOccupantZoneIdForDriver() {
        List<CarOccupantZoneManager.OccupantZoneInfo> occupantZoneInfos =
                mOccupantZoneManager.getAllOccupantZones();
        for (CarOccupantZoneManager.OccupantZoneInfo info: occupantZoneInfos) {
            if (info.occupantType == CarOccupantZoneManager.OCCUPANT_TYPE_DRIVER) {
                return info.zoneId;
            }
        }
        return CarOccupantZoneManager.OccupantZoneInfo.INVALID_ZONE_ID;
    }

    private void setUserIdDeviceAffinitiesLocked(CarAudioZone zone, @UserIdInt int userId,
            int audioZoneId) {
        if (!mAudioPolicy.setUserIdDeviceAffinity(userId, zone.getAudioDeviceInfos())) {
            throw new IllegalStateException(String.format(
                    "setUserIdDeviceAffinity for userId %d in zone %d Failed,"
                            + " could not set audio routing.",
                    userId, audioZoneId));
        }
    }

    private void resetZoneToDefaultUser(CarAudioZone zone, @UserIdInt int driverUserId) {
        resetCarZonesAudioFocus(zone.getId(), driverUserId);
        zone.updateVolumeGroupsSettingsForUser(driverUserId);
    }

    private void resetCarZonesAudioFocus(int audioZoneId, @UserIdInt int driverUserId) {
        mFocusHandler.updateUserForZoneId(audioZoneId, driverUserId);
    }

    private void removeUserIdDeviceAffinitiesLocked(@UserIdInt int userId) {
        if (Log.isLoggable(CarLog.TAG_AUDIO, Log.DEBUG)) {
            Slog.d(CarLog.TAG_AUDIO,
                    "removeUserIdDeviceAffinities(" + userId + ") Succeeded");
        }
        if (userId == UserHandle.USER_NULL) {
            return;
        }
        if (!mAudioPolicy.removeUserIdDeviceAffinity(userId)) {
            Slog.e(CarLog.TAG_AUDIO, "removeUserIdDeviceAffinities(" + userId + ") Failed");
            return;
        }
    }

    private @UserIdInt int getUserIdForZoneLocked(int audioZoneId) {
        return mAudioZoneIdToUserIdMapping.get(audioZoneId, UserHandle.USER_NULL);
    }

    private void setUserIdForAudioZoneLocked(@UserIdInt int userId, int audioZoneId) {
        mAudioZoneIdToUserIdMapping.put(audioZoneId, userId);
    }

    private AudioControlWrapper getAudioControlWrapperLocked() {
        if (mAudioControlWrapper == null) {
            mAudioControlWrapper = AudioControlFactory.newAudioControl();
            mAudioControlWrapper.linkToDeath(this::resetHalAudioFocus);
        }
        return mAudioControlWrapper;
    }

    private void resetHalAudioFocus() {
        if (mHalAudioFocus != null) {
            mHalAudioFocus.reset();
            mHalAudioFocus.registerFocusListener();
        }
    }

    boolean isAudioZoneIdValid(int zoneId) {
        synchronized (mImplLock) {
            return mCarAudioZones.contains(zoneId);
        }
    }

    private CarAudioZone getCarAudioZone(int zoneId) {
        synchronized (mImplLock) {
            return getCarAudioZoneLocked(zoneId);
        }
    }

    @GuardedBy("mImplLock")
    private CarAudioZone getCarAudioZoneLocked(int zoneId) {
        checkAudioZoneIdLocked(zoneId);
        return mCarAudioZones.get(zoneId);
    }

    @GuardedBy("mImplLock")
    private void checkAudioZoneIdLocked(int zoneId) {
        Preconditions.checkArgument(mCarAudioZones.contains(zoneId),
                "Invalid audio zone Id " + zoneId);
    }

    int getVolumeGroupIdForAudioContext(int zoneId, int suggestedContext) {
        synchronized (mImplLock) {
            return getVolumeGroupIdForAudioContextLocked(zoneId, suggestedContext);
        }
    }

    /**
     * Resets the last selected volume context.
     */
    public void resetSelectedVolumeContext() {
        enforcePermission(Car.PERMISSION_CAR_CONTROL_AUDIO_VOLUME);
        mCarVolume.resetSelectedVolumeContext();
        synchronized (mImplLock) {
            mCarAudioPlaybackCallback.resetStillActiveContexts();
        }
    }

    private class CarAudioOccupantConfigChangeListener implements OccupantZoneConfigChangeListener {
        @Override
        public void onOccupantZoneConfigChanged(int flags) {
            if (Log.isLoggable(CarLog.TAG_AUDIO, Log.DEBUG)) {
                Slog.d(CarLog.TAG_AUDIO,
                        "onOccupantZoneConfigChanged(" + flags + ")");
            }
            if (((flags & CarOccupantZoneManager.ZONE_CONFIG_CHANGE_FLAG_USER)
                    == CarOccupantZoneManager.ZONE_CONFIG_CHANGE_FLAG_USER)
                    || ((flags & CarOccupantZoneManager.ZONE_CONFIG_CHANGE_FLAG_DISPLAY)
                    == CarOccupantZoneManager.ZONE_CONFIG_CHANGE_FLAG_DISPLAY)) {
                handleOccupantZoneUserChanged();
            }
        }
    }

    private List<Integer> getAllActiveContextsForPrimaryZone() {
        synchronized (mImplLock) {
            return mCarAudioPlaybackCallback.getAllActiveContextsForPrimaryZone();
        }
    }

    static final class SystemClockWrapper {
        public long uptimeMillis() {
            return SystemClock.uptimeMillis();
        }
    }
}
