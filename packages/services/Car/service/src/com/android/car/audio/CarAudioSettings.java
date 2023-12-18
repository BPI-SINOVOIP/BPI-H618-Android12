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
package com.android.car.audio;

import android.annotation.NonNull;
import android.annotation.UserIdInt;
import android.car.settings.CarSettings;
import android.content.ContentResolver;
import android.provider.Settings;

import java.util.Objects;

/**
 * Use to save/load car volume settings
 */
public class CarAudioSettings {

    // The trailing slash forms a directory-liked hierarchy and
    // allows listening for both GROUP/MEDIA and GROUP/NAVIGATION.
    private static final String VOLUME_SETTINGS_KEY_FOR_GROUP_PREFIX = "android.car.VOLUME_GROUP/";

    // The trailing slash forms a directory-liked hierarchy and
    // allows listening for both GROUP/MEDIA and GROUP/NAVIGATION.
    private static final String VOLUME_SETTINGS_KEY_FOR_GROUP_MUTE_PREFIX =
            "android.car.VOLUME_GROUP_MUTE/";

    // Key to persist master mute state in system settings
    private static final String VOLUME_SETTINGS_KEY_MASTER_MUTE = "android.car.MASTER_MUTE";

    private final ContentResolver mContentResolver;

    CarAudioSettings(@NonNull ContentResolver contentResolver) {
        mContentResolver = Objects.requireNonNull(contentResolver);
    }

    int getStoredVolumeGainIndexForUser(int userId, int zoneId, int groupId) {
        return Settings.System.getIntForUser(mContentResolver,
                getVolumeSettingsKeyForGroup(zoneId, groupId), -1, userId);
    }

    void storeVolumeGainIndexForUser(int userId, int zoneId, int groupId, int gainIndex) {
        Settings.System.putIntForUser(mContentResolver,
                getVolumeSettingsKeyForGroup(zoneId, groupId),
                gainIndex, userId);
    }

    void storeMasterMute(Boolean masterMuteValue) {
        Settings.Global.putInt(mContentResolver,
                VOLUME_SETTINGS_KEY_MASTER_MUTE,
                masterMuteValue ? 1 : 0);
    }

    boolean getMasterMute() {
        return Settings.Global.getInt(mContentResolver,
                VOLUME_SETTINGS_KEY_MASTER_MUTE, 0) != 0;
    }

    void storeVolumeGroupMuteForUser(@UserIdInt int userId, int zoneId, int groupId,
            boolean isMuted) {
        Settings.System.putIntForUser(mContentResolver,
                getMuteSettingsKeyForGroup(zoneId, groupId),
                isMuted ? 1 : 0, userId);
    }

    boolean getVolumeGroupMuteForUser(@UserIdInt int userId, int zoneId, int groupId) {
        return Settings.System.getIntForUser(mContentResolver,
                getMuteSettingsKeyForGroup(zoneId, groupId),
                /*disabled by default*/ 0, userId) != 0;
    }

    boolean isPersistVolumeGroupMuteEnabled(@UserIdInt int userId) {
        return Settings.Secure.getIntForUser(mContentResolver,
                CarSettings.Secure.KEY_AUDIO_PERSIST_VOLUME_GROUP_MUTE_STATES,
                /*disabled by default*/ 0, userId) == 1;
    }

    /**
     * Determines if for a given userId the reject navigation on call setting is enabled
     */
    public boolean isRejectNavigationOnCallEnabledInSettings(@UserIdInt int userId) {
        return Settings.Secure.getIntForUser(mContentResolver,
                CarSettings.Secure.KEY_AUDIO_FOCUS_NAVIGATION_REJECTED_DURING_CALL,
                /*disabled by default*/ 0, userId) == 1;
    }

    private static String getVolumeSettingsKeyForGroup(int zoneId, int groupId) {
        return VOLUME_SETTINGS_KEY_FOR_GROUP_PREFIX
                + getFormattedZoneIdAndGroupIdKey(zoneId, groupId);
    }

    private static String getMuteSettingsKeyForGroup(int zoneId, int groupId) {
        return VOLUME_SETTINGS_KEY_FOR_GROUP_MUTE_PREFIX
                + getFormattedZoneIdAndGroupIdKey(zoneId, groupId);
    }

    private static String getFormattedZoneIdAndGroupIdKey(int zoneId, int groupId) {
        return new StringBuilder().append(zoneId).append('/').append(groupId).toString();
    }

    public ContentResolver getContentResolver() {
        return mContentResolver;
    }
}
