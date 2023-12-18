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
import android.hardware.automotive.audiocontrol.DuckingInfo;
import android.media.AudioAttributes;
import android.util.IndentingPrintWriter;

import com.android.car.internal.ExcludeFromCodeCoverageGeneratedReport;

import java.util.List;
import java.util.Objects;

/**
 * Ducking information for a given car audio zone based on its focus state.
 */
public final class CarDuckingInfo {
    public final int mZoneId;
    public final List<String> mAddressesToDuck;
    public final List<String> mAddressesToUnduck;
    public final int[] mUsagesHoldingFocus;

    public CarDuckingInfo(int zoneId, @NonNull List<String> addressesToDuck,
            @NonNull List<String> addressesToUnduck, @NonNull int[] usagesHoldingFocus) {
        mZoneId = zoneId;
        mAddressesToDuck = Objects.requireNonNull(addressesToDuck);
        mAddressesToUnduck = Objects.requireNonNull(addressesToUnduck);
        mUsagesHoldingFocus = Objects.requireNonNull(usagesHoldingFocus);
    }

    /**
     * Creates {@link DuckingInfo} instance from contents of {@link CarDuckingInfo}.
     *
     * <p>Converts usages to XSD strings as part of this process.
     */
    public DuckingInfo generateDuckingInfo() {
        DuckingInfo duckingInfo = new DuckingInfo();
        duckingInfo.zoneId = mZoneId;
        duckingInfo.deviceAddressesToDuck = mAddressesToDuck.toArray(new String[0]);
        duckingInfo.deviceAddressesToUnduck = mAddressesToUnduck.toArray(new String[0]);
        String[] usageStrings = new String[mUsagesHoldingFocus.length];
        for (int i = 0; i < mUsagesHoldingFocus.length; i++) {
            usageStrings[i] = AudioAttributes.usageToXsdString(mUsagesHoldingFocus[i]);
        }
        duckingInfo.usagesHoldingFocus = usageStrings;

        return duckingInfo;
    }

    @ExcludeFromCodeCoverageGeneratedReport(reason = DUMP_INFO)
    void dump(IndentingPrintWriter writer) {
        writer.printf("Ducking Info for zone %d \n", mZoneId);
        writer.increaseIndent();
        writer.printf("Addresses to duck: %s\n",
                String.join(", ", mAddressesToDuck));
        writer.printf("Addresses to unduck: %s\n",
                String.join(", ", mAddressesToUnduck));
        writer.println("Usages holding focus:");
        writer.increaseIndent();
        for (int usage : mUsagesHoldingFocus) {
            writer.printf("%s, ", AudioAttributes.usageToXsdString(usage));
        }
        writer.decreaseIndent();
        writer.println();
        writer.decreaseIndent();
    }
}
