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

package com.android.car.audio;

import static com.android.car.audio.CarAudioService.DEFAULT_AUDIO_CONTEXT;
import static com.android.car.audio.CarAudioService.SystemClockWrapper;
import static com.android.car.audio.CarAudioUtils.hasExpired;

import android.annotation.IntDef;
import android.annotation.NonNull;
import android.media.AudioAttributes.AttributeUsage;
import android.media.AudioPlaybackConfiguration;
import android.telephony.Annotation.CallState;
import android.telephony.TelephonyManager;
import android.util.SparseIntArray;

import com.android.car.CarLog;
import com.android.car.audio.CarAudioContext.AudioContext;
import com.android.internal.annotations.GuardedBy;
import com.android.internal.util.Preconditions;

import java.lang.annotation.Retention;
import java.lang.annotation.RetentionPolicy;
import java.util.List;
import java.util.Objects;
import java.util.Set;

/**
 * CarVolume is responsible for determining which audio contexts to prioritize when adjusting volume
 */
final class CarVolume {
    private static final String TAG = CarLog.tagFor(CarVolume.class);
    private static final int CONTEXT_HIGHEST_PRIORITY = 0;
    private static final int CONTEXT_NOT_PRIORITIZED = -1;

    static final int VERSION_ONE = 1;
    private static final int[] AUDIO_CONTEXT_VOLUME_PRIORITY_V1 = {
            CarAudioContext.NAVIGATION,
            CarAudioContext.CALL,
            CarAudioContext.MUSIC,
            CarAudioContext.ANNOUNCEMENT,
            CarAudioContext.VOICE_COMMAND,
            CarAudioContext.CALL_RING,
            CarAudioContext.SYSTEM_SOUND,
            CarAudioContext.SAFETY,
            CarAudioContext.ALARM,
            CarAudioContext.NOTIFICATION,
            CarAudioContext.VEHICLE_STATUS,
            CarAudioContext.EMERGENCY,
            // CarAudioContext.INVALID is intentionally not prioritized as it is not routed by
            // CarAudioService and is not expected to be used.
    };

    static final int VERSION_TWO = 2;
    private static final int[] AUDIO_CONTEXT_VOLUME_PRIORITY_V2 = {
            CarAudioContext.CALL,
            CarAudioContext.MUSIC,
            CarAudioContext.ANNOUNCEMENT,
            CarAudioContext.VOICE_COMMAND,
    };

    private final SparseIntArray mVolumePriorityByAudioContext = new SparseIntArray();
    private final SystemClockWrapper mClock;
    private final Object mLock = new Object();
    private final int mVolumeKeyEventTimeoutMs;
    private final int mLowestPriority;
    @GuardedBy("mLock")
    @AudioContext private int mLastActiveContext;
    @GuardedBy("mLock")
    private long mLastActiveContextStartTime;

    /**
     * Creates car volume for management of volume priority and last selected audio context.
     * @param clockWrapper time keeper for expiration of last selected context.
     * @param audioVolumeAdjustmentContextsVersion audio priority list version number, can be
     *      any version defined in {@link CarVolumeListVersion}
     * @param volumeKeyEventTimeoutMs timeout in ms used to measure expiration of last selected
     *      context
     */
    CarVolume(@NonNull SystemClockWrapper clockWrapper,
            @CarVolumeListVersion int audioVolumeAdjustmentContextsVersion,
            int volumeKeyEventTimeoutMs) {
        mClock = Objects.requireNonNull(clockWrapper, "Clock must not be null.");
        mVolumeKeyEventTimeoutMs = Preconditions.checkArgumentNonnegative(volumeKeyEventTimeoutMs);
        mLastActiveContext = CarAudioContext.INVALID;
        mLastActiveContextStartTime = mClock.uptimeMillis();
        @AudioContext int[] contextVolumePriority =
                getContextPriorityList(audioVolumeAdjustmentContextsVersion);

        for (int priority = CONTEXT_HIGHEST_PRIORITY;
                priority < contextVolumePriority.length; priority++) {
            mVolumePriorityByAudioContext.append(contextVolumePriority[priority], priority);
        }

        mLowestPriority = CONTEXT_HIGHEST_PRIORITY + mVolumePriorityByAudioContext.size();

    }

    private static int[] getContextPriorityList(int audioVolumeAdjustmentContextsVersion) {
        Preconditions.checkArgumentInRange(audioVolumeAdjustmentContextsVersion, 1, 2,
                "audioVolumeAdjustmentContextsVersion");
        if (audioVolumeAdjustmentContextsVersion == VERSION_TWO) {
            return AUDIO_CONTEXT_VOLUME_PRIORITY_V2;
        }
        return AUDIO_CONTEXT_VOLUME_PRIORITY_V1;
    }

    /**
     * @see {@link CarAudioService#resetSelectedVolumeContext()}
     */
    public void resetSelectedVolumeContext() {
        setAudioContextStillActive(CarAudioContext.INVALID);
    }

    /**
     * Finds a {@link AudioContext} that should be adjusted based on the current
     * {@link AudioPlaybackConfiguration}s, {@link CallState}, and active HAL usages. If an active
     * context is found it be will saved and retrieved later on.
     */
    @AudioContext int getSuggestedAudioContextAndSaveIfFound(
            @NonNull List<Integer> activePlaybackContexts, @CallState int callState,
            @NonNull @AttributeUsage int[] activeHalUsages) {

        int activeContext = getAudioContextStillActive();
        if (activeContext != CarAudioContext.INVALID) {
            setAudioContextStillActive(activeContext);
            return activeContext;
        }

        Set<Integer> activeContexts = getActiveContexts(activePlaybackContexts, callState,
                activeHalUsages);


        @AudioContext int context =
                findActiveContextWithHighestPriority(activeContexts, mVolumePriorityByAudioContext);

        setAudioContextStillActive(context);

        return context;
    }

    private @AudioContext int findActiveContextWithHighestPriority(
            Set<Integer> activeContexts, SparseIntArray contextPriorities) {
        int currentContext = DEFAULT_AUDIO_CONTEXT;
        int currentPriority = mLowestPriority;

        for (@AudioContext int context : activeContexts) {
            int priority = contextPriorities.get(context, CONTEXT_NOT_PRIORITIZED);
            if (priority == CONTEXT_NOT_PRIORITIZED) {
                continue;
            }

            if (priority < currentPriority) {
                currentContext = context;
                currentPriority = priority;
                // If the highest priority has been found, break early.
                if (currentPriority == CONTEXT_HIGHEST_PRIORITY) {
                    break;
                }
            }
        }

        return currentContext;
    }

    private void setAudioContextStillActive(@AudioContext int context) {
        synchronized (mLock) {
            mLastActiveContext = context;
            mLastActiveContextStartTime = mClock.uptimeMillis();
        }
    }

    public static boolean isAnyContextActive(@NonNull @AudioContext int [] contexts,
            @NonNull List<Integer> activePlaybackContext, @CallState int callState,
            @NonNull @AttributeUsage int[] activeHalUsages) {
        Objects.nonNull(contexts);
        Preconditions.checkArgument(contexts.length != 0,
                "contexts can not be empty.");
        Set<Integer> activeContexts = getActiveContexts(activePlaybackContext,
                callState, activeHalUsages);
        for (@AudioContext int context : contexts) {
            if (activeContexts.contains(context)) {
                return true;
            }
        }
        return false;
    }

    private static Set<Integer> getActiveContexts(@NonNull List<Integer> activePlaybackContexts,
            @CallState int callState, @NonNull @AttributeUsage int[] activeHalUsages) {
        Objects.nonNull(activePlaybackContexts);
        Objects.nonNull(activeHalUsages);

        Set<Integer> contexts = CarAudioContext.getUniqueContextsForUsages(activeHalUsages);

        switch (callState) {
            case TelephonyManager.CALL_STATE_RINGING:
                contexts.add(CarAudioContext.CALL_RING);
                break;
            case TelephonyManager.CALL_STATE_OFFHOOK:
                contexts.add(CarAudioContext.CALL);
                break;
        }

        contexts.addAll(activePlaybackContexts);
        return contexts;
    }

    private @AudioContext int getAudioContextStillActive() {
        @AudioContext int context;
        long contextStartTime;
        synchronized (mLock) {
            context = mLastActiveContext;
            contextStartTime = mLastActiveContextStartTime;
        }

        if (context == CarAudioContext.INVALID) {
            return CarAudioContext.INVALID;
        }

        if (hasExpired(contextStartTime, mClock.uptimeMillis(), mVolumeKeyEventTimeoutMs)) {
            return CarAudioContext.INVALID;
        }

        return context;
    }

    @IntDef({
            VERSION_ONE,
            VERSION_TWO
    })
    @Retention(RetentionPolicy.SOURCE)
    public @interface CarVolumeListVersion {
    }
}
