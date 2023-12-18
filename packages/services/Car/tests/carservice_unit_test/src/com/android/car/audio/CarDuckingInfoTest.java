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

import static android.media.AudioAttributes.USAGE_MEDIA;
import static android.media.AudioAttributes.USAGE_NOTIFICATION;

import static com.google.common.truth.Truth.assertThat;

import static org.testng.Assert.assertThrows;

import android.audio.policy.configuration.V7_0.AudioUsage;
import android.hardware.automotive.audiocontrol.DuckingInfo;

import androidx.test.ext.junit.runners.AndroidJUnit4;

import org.junit.Test;
import org.junit.runner.RunWith;

import java.util.List;

@RunWith(AndroidJUnit4.class)
public class CarDuckingInfoTest {
    private static final int ZONE_ID = 0;
    private static final List<String> sAddressesToDuck = List.of("address1", "address2");
    private static final List<String> sAddressesToUnduck = List.of("address3", "address4");
    private static final int[] sUsagesHoldingFocus = {USAGE_MEDIA, USAGE_NOTIFICATION};

    @Test
    public void constructor_nullAddressesToDuck_throws() {
        assertThrows(NullPointerException.class, () -> new CarDuckingInfo(ZONE_ID, null,
                sAddressesToUnduck, sUsagesHoldingFocus));
    }

    @Test
    public void constructor_nullAddressesToUnduck_throws() {
        assertThrows(NullPointerException.class, () -> new CarDuckingInfo(ZONE_ID, sAddressesToDuck,
                null, sUsagesHoldingFocus));
    }

    @Test
    public void constructor_nullusagesHoldingFocus_throws() {
        assertThrows(NullPointerException.class, () -> new CarDuckingInfo(ZONE_ID, sAddressesToDuck,
                sAddressesToUnduck, null));
    }

    @Test
    public void constructor_validInputs_succeeds() {
        CarDuckingInfo duckingInfo = getCarDuckingInfo();

        assertThat(duckingInfo.mZoneId).isEqualTo(ZONE_ID);
        assertThat(duckingInfo.mAddressesToDuck).containsExactlyElementsIn(sAddressesToDuck);
        assertThat(duckingInfo.mAddressesToUnduck).containsExactlyElementsIn(sAddressesToUnduck);
        assertThat(duckingInfo.mUsagesHoldingFocus).asList()
                .containsExactly(USAGE_MEDIA, USAGE_NOTIFICATION);
    }

    @Test
    public void generateDuckingInfo_includesSameAddressesToDuck() {
        CarDuckingInfo carDuckingInfo = getCarDuckingInfo();

        DuckingInfo duckingInfo = carDuckingInfo.generateDuckingInfo();

        assertThat(duckingInfo.deviceAddressesToDuck).asList()
                .containsExactlyElementsIn(carDuckingInfo.mAddressesToDuck);
    }

    @Test
    public void generateDuckingInfo_includesSameAddressesToUnduck() {
        CarDuckingInfo carDuckingInfo = getCarDuckingInfo();

        DuckingInfo duckingInfo = carDuckingInfo.generateDuckingInfo();

        assertThat(duckingInfo.deviceAddressesToUnduck).asList()
                .containsExactlyElementsIn(carDuckingInfo.mAddressesToUnduck);
    }

    @Test
    public void generateDuckingInfo_includesSameUsagesHoldingFocus() {
        CarDuckingInfo carDuckingInfo = getCarDuckingInfo();

        DuckingInfo duckingInfo = carDuckingInfo.generateDuckingInfo();

        assertThat(duckingInfo.usagesHoldingFocus).asList()
                .containsExactly(AudioUsage.AUDIO_USAGE_MEDIA.toString(),
                        AudioUsage.AUDIO_USAGE_NOTIFICATION.toString());
    }

    private CarDuckingInfo getCarDuckingInfo() {
        return new CarDuckingInfo(ZONE_ID, sAddressesToDuck, sAddressesToUnduck,
                sUsagesHoldingFocus);
    }
}
