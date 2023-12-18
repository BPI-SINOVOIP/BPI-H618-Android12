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

import static com.android.car.internal.ExcludeFromCodeCoverageGeneratedReport.DUMP_INFO;

import android.annotation.NonNull;
import android.media.AudioFocusInfo;
import android.util.IndentingPrintWriter;
import android.util.SparseArray;

import com.android.car.audio.CarZonesAudioFocus.CarFocusCallback;
import com.android.car.audio.hal.AudioControlWrapper;
import com.android.car.internal.ExcludeFromCodeCoverageGeneratedReport;
import com.android.internal.annotations.GuardedBy;
import com.android.internal.annotations.VisibleForTesting;

import java.util.ArrayList;
import java.util.List;
import java.util.Objects;

final class CarDucking implements CarFocusCallback {
    private static final String TAG = CarDucking.class.getSimpleName();

    private final SparseArray<CarAudioZone> mCarAudioZones;
    private final AudioControlWrapper mAudioControlWrapper;
    private final Object mLock = new Object();

    @GuardedBy("mLock")
    private final SparseArray<CarDuckingInfo> mCurrentDuckingInfo = new SparseArray<>();

    CarDucking(@NonNull SparseArray<CarAudioZone> carAudioZones,
            @NonNull AudioControlWrapper audioControlWrapper) {
        mCarAudioZones = Objects.requireNonNull(carAudioZones);
        mAudioControlWrapper = Objects.requireNonNull(audioControlWrapper);
        for (int i = 0; i < carAudioZones.size(); i++) {
            int zoneId = carAudioZones.keyAt(i);
            mCurrentDuckingInfo.put(zoneId,
                    new CarDuckingInfo(zoneId, new ArrayList<>(), new ArrayList<>(), new int[0]));
        }
    }

    @VisibleForTesting
    SparseArray<CarDuckingInfo> getCurrentDuckingInfo() {
        synchronized (mLock) {
            return mCurrentDuckingInfo;
        }
    }

    @Override
    public void onFocusChange(int[] audioZoneIds,
            @NonNull SparseArray<List<AudioFocusInfo>> focusHoldersByZoneId) {
        synchronized (mLock) {
            List<CarDuckingInfo> newDuckingInfos = new ArrayList<>(audioZoneIds.length);
            for (int i = 0; i < audioZoneIds.length; i++) {
                int zoneId = audioZoneIds[i];
                List<AudioFocusInfo> focusHolders = focusHoldersByZoneId.get(zoneId);
                CarDuckingInfo newDuckingInfo = updateDuckingForZoneIdLocked(zoneId, focusHolders);
                newDuckingInfos.add(newDuckingInfo);
            }
            mAudioControlWrapper.onDevicesToDuckChange(newDuckingInfos);
        }
    }

    @GuardedBy("mLock")
    private CarDuckingInfo updateDuckingForZoneIdLocked(int zoneId,
            List<AudioFocusInfo> focusHolders) {
        CarDuckingInfo oldDuckingInfo = mCurrentDuckingInfo.get(zoneId);
        CarDuckingInfo newDuckingInfo = generateNewDuckingInfoLocked(oldDuckingInfo,
                focusHolders);
        mCurrentDuckingInfo.put(zoneId, newDuckingInfo);
        return newDuckingInfo;
    }

    @ExcludeFromCodeCoverageGeneratedReport(reason = DUMP_INFO)
    public void dump(IndentingPrintWriter writer) {
        writer.printf("*%s*\n", TAG);
        writer.increaseIndent();
        synchronized (mLock) {
            for (int i = 0; i < mCurrentDuckingInfo.size(); i++) {
                mCurrentDuckingInfo.valueAt(i).dump(writer);
            }
        }
        writer.decreaseIndent();
    }

    @GuardedBy("mLock")
    private CarDuckingInfo generateNewDuckingInfoLocked(CarDuckingInfo oldDuckingInfo,
            List<AudioFocusInfo> focusHolders) {
        int zoneId = oldDuckingInfo.mZoneId;
        CarAudioZone zone = mCarAudioZones.get(zoneId);

        int[] usagesHoldingFocus = CarDuckingUtils.getUsagesHoldingFocus(focusHolders);
        List<String> addressesToDuck = CarDuckingUtils.getAddressesToDuck(usagesHoldingFocus, zone);
        List<String> addressesToUnduck = CarDuckingUtils.getAddressesToUnduck(addressesToDuck,
                oldDuckingInfo.mAddressesToDuck);

        return new CarDuckingInfo(zoneId, addressesToDuck, addressesToUnduck, usagesHoldingFocus);
    }
}
