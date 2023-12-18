/*
 * Copyright (C) 2021 The Android Open Source Project
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

import static com.android.car.audio.CarAudioContext.AudioContext;
import static com.android.car.audio.CarAudioService.SystemClockWrapper;
import static com.android.car.audio.CarAudioUtils.hasExpired;

import android.annotation.NonNull;
import android.media.AudioManager;
import android.media.AudioPlaybackConfiguration;
import android.util.SparseLongArray;

import com.android.internal.annotations.GuardedBy;
import com.android.internal.util.Preconditions;

import java.util.ArrayList;
import java.util.HashMap;
import java.util.List;
import java.util.Map;
import java.util.Objects;

final class CarAudioPlaybackCallback extends AudioManager.AudioPlaybackCallback {
    private final Object mLock = new Object();
    @GuardedBy("mLock")
    private final SparseLongArray mContextStartTime = new SparseLongArray();
    @GuardedBy("mLock")
    private final Map<String, AudioPlaybackConfiguration> mLastActiveConfigs = new HashMap<>();
    private final CarAudioZone mCarPrimaryAudioZone;
    private final SystemClockWrapper mClock;
    private final int mVolumeKeyEventTimeoutMs;

    CarAudioPlaybackCallback(@NonNull CarAudioZone carPrimaryAudioZone,
            @NonNull SystemClockWrapper clock,
            int volumeKeyEventTimeoutMs) {
        mCarPrimaryAudioZone = Objects.requireNonNull(carPrimaryAudioZone);
        mClock = Objects.requireNonNull(clock);
        mVolumeKeyEventTimeoutMs = Preconditions.checkArgumentNonnegative(volumeKeyEventTimeoutMs);
    }

    @Override
    public void onPlaybackConfigChanged(List<AudioPlaybackConfiguration> configurations) {
        Map<String, AudioPlaybackConfiguration> newActiveConfigs =
                filterNewActiveConfiguration(configurations);

        synchronized (mLock) {
            List<AudioPlaybackConfiguration> newlyInactiveConfigurations =
                    getNewlyInactiveConfigurationsLocked(newActiveConfigs);

            mLastActiveConfigs.clear();
            mLastActiveConfigs.putAll(newActiveConfigs);

            startTimersForContextThatBecameInactiveLocked(newlyInactiveConfigurations);
        }
    }

    /**
     * Returns all active contexts for the primary zone
     * @return all active audio contexts, including those that recently became inactive but are
     * considered active due to the audio playback timeout.
     */
    public List<Integer> getAllActiveContextsForPrimaryZone() {
        synchronized (mLock) {
            List<Integer> activeContexts = getCurrentlyActiveContextsLocked();
            activeContexts
                    .addAll(getStillActiveContextAndRemoveExpiredContextsLocked());
            return activeContexts;
        }
    }

    private void startTimersForContextThatBecameInactiveLocked(
            List<AudioPlaybackConfiguration> inactiveConfigs) {
        List<Integer> activeContexts = mCarPrimaryAudioZone
                .findActiveContextsFromPlaybackConfigurations(inactiveConfigs);

        for (int activeContext : activeContexts) {
            mContextStartTime.put(activeContext, mClock.uptimeMillis());
        }
    }

    private List<AudioPlaybackConfiguration> getNewlyInactiveConfigurationsLocked(
            Map<String, AudioPlaybackConfiguration> newActiveConfigurations) {
        List<AudioPlaybackConfiguration> newlyInactiveConfigurations = new ArrayList<>();
        for (String address : mLastActiveConfigs.keySet()) {
            if (newActiveConfigurations.containsKey(address)) {
                continue;
            }
            newlyInactiveConfigurations.add(mLastActiveConfigs.get(address));
        }
        return newlyInactiveConfigurations;
    }

    private Map<String, AudioPlaybackConfiguration> filterNewActiveConfiguration(
            List<AudioPlaybackConfiguration> configurations) {
        Map<String, AudioPlaybackConfiguration> newActiveConfigs = new HashMap<>();
        for (int index = 0; index < configurations.size(); index++) {
            AudioPlaybackConfiguration configuration = configurations.get(index);
            if (!configuration.isActive()) {
                continue;
            }
            if (mCarPrimaryAudioZone
                    .isAudioDeviceInfoValidForZone(configuration.getAudioDeviceInfo())) {
                newActiveConfigs.put(
                        configuration.getAudioDeviceInfo().getAddress(), configuration);
            }
        }
        return newActiveConfigs;
    }

    private List<Integer> getCurrentlyActiveContextsLocked() {
        return mCarPrimaryAudioZone.findActiveContextsFromPlaybackConfigurations(
                new ArrayList<>(mLastActiveConfigs.values()));
    }

    private List<Integer> getStillActiveContextAndRemoveExpiredContextsLocked() {
        List<Integer> contextsToRemove = new ArrayList<>();
        List<Integer> stillActiveContexts = new ArrayList<>();
        for (int index = 0; index < mContextStartTime.size(); index++) {
            @AudioContext int context = mContextStartTime.keyAt(index);
            if (hasExpired(mContextStartTime.valueAt(index),
                    mClock.uptimeMillis(), mVolumeKeyEventTimeoutMs)) {
                contextsToRemove.add(context);
                continue;
            }
            stillActiveContexts.add(context);
        }

        for (int indexToRemove = 0; indexToRemove < contextsToRemove.size(); indexToRemove++) {
            mContextStartTime.delete(contextsToRemove.get(indexToRemove));
        }
        return stillActiveContexts;
    }

    void resetStillActiveContexts() {
        synchronized (mLock) {
            mContextStartTime.clear();
        }
    }
}
