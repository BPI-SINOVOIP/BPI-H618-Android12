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

import static android.media.AudioAttributes.USAGE_ASSISTANCE_NAVIGATION_GUIDANCE;
import static android.media.AudioAttributes.USAGE_MEDIA;
import static android.media.AudioManager.AUDIOFOCUS_GAIN_TRANSIENT;

import static com.android.car.audio.CarAudioContext.MUSIC;
import static com.android.car.audio.CarAudioContext.NAVIGATION;

import static com.google.common.truth.Truth.assertThat;

import static org.mockito.Mockito.mock;
import static org.mockito.Mockito.times;
import static org.mockito.Mockito.verify;
import static org.mockito.Mockito.when;

import android.media.AudioAttributes;
import android.media.AudioFocusInfo;
import android.util.SparseArray;

import com.android.car.audio.hal.AudioControlWrapper;

import org.junit.Before;
import org.junit.Test;
import org.junit.runner.RunWith;
import org.mockito.ArgumentCaptor;
import org.mockito.Captor;
import org.mockito.Mock;
import org.mockito.junit.MockitoJUnitRunner;

import java.util.List;

@RunWith(MockitoJUnitRunner.class)
public final class CarDuckingTest {
    private static final int PRIMARY_ZONE_ID = 0;
    private static final int PASSENGER_ZONE_ID = 1;
    private static final int[] ONE_ZONE_CHANGE = new int[]{PRIMARY_ZONE_ID};
    private static final int REAR_ZONE_ID = 2;
    private static final String PRIMARY_MEDIA_ADDRESS = "primary_media";
    private static final String PRIMARY_NAVIGATION_ADDRESS = "primary_navigation_address";
    private static final String REAR_MEDIA_ADDRESS = "rear_media";

    private final SparseArray<CarAudioZone> mCarAudioZones = generateZoneMocks();
    private final AudioFocusInfo mMediaFocusInfo = generateAudioFocusInfoForUsage(USAGE_MEDIA);
    private final AudioFocusInfo mNavigationFocusInfo =
            generateAudioFocusInfoForUsage(USAGE_ASSISTANCE_NAVIGATION_GUIDANCE);
    private final SparseArray<List<AudioFocusInfo>> mMediaFocusHolders = new SparseArray<>();
    private final SparseArray<List<AudioFocusInfo>> mMediaNavFocusHolders = new SparseArray<>();

    @Mock
    private AudioControlWrapper mMockAudioControlWrapper;

    @Captor
    private ArgumentCaptor<List<CarDuckingInfo>> mCarDuckingInfosCaptor;

    private CarDucking mCarDucking;

    @Before
    public void setUp() {
        mCarDucking = new CarDucking(mCarAudioZones, mMockAudioControlWrapper);
        mMediaFocusHolders.put(PRIMARY_ZONE_ID, List.of(mMediaFocusInfo));
        mMediaNavFocusHolders.put(PRIMARY_ZONE_ID, List.of(mMediaFocusInfo, mNavigationFocusInfo));
    }

    @Test
    public void constructor_initializesEmptyDuckingInfoForZones() {
        SparseArray<CarDuckingInfo> currentDuckingInfo = mCarDucking.getCurrentDuckingInfo();

        assertThat(currentDuckingInfo.size()).isEqualTo(mCarAudioZones.size());
        for (int i = 0; i < mCarAudioZones.size(); i++) {
            int zoneId = mCarAudioZones.keyAt(i);
            CarDuckingInfo duckingInfo = currentDuckingInfo.get(zoneId);
            assertThat(duckingInfo).isNotNull();
            assertThat(duckingInfo.mUsagesHoldingFocus).isEmpty();
            assertThat(duckingInfo.mAddressesToDuck).isEmpty();
            assertThat(duckingInfo.mAddressesToUnduck).isEmpty();
        }
    }

    @Test
    public void onFocusChange_forPrimaryZone_updatesUsagesHoldingFocus() {
        mCarDucking.onFocusChange(ONE_ZONE_CHANGE, mMediaFocusHolders);

        SparseArray<CarDuckingInfo> newDuckingInfo = mCarDucking.getCurrentDuckingInfo();
        assertThat(newDuckingInfo.get(PRIMARY_ZONE_ID).mUsagesHoldingFocus)
                .asList().containsExactly(USAGE_MEDIA);
    }

    @Test
    public void onFocusChange_forPrimaryZone_doesNotUpdateSecondaryZones() {
        mCarDucking.onFocusChange(ONE_ZONE_CHANGE, mMediaFocusHolders);

        SparseArray<CarDuckingInfo> newDuckingInfo = mCarDucking.getCurrentDuckingInfo();
        assertThat(newDuckingInfo.get(PASSENGER_ZONE_ID).mUsagesHoldingFocus).isEmpty();
        assertThat(newDuckingInfo.get(REAR_ZONE_ID).mUsagesHoldingFocus).isEmpty();
    }

    @Test
    public void onFocusChange_withMultipleFocusHolders_updatesAddressesToDuck() {
        mCarDucking.onFocusChange(ONE_ZONE_CHANGE, mMediaNavFocusHolders);

        SparseArray<CarDuckingInfo> newDuckingInfo = mCarDucking.getCurrentDuckingInfo();
        assertThat(newDuckingInfo.get(PRIMARY_ZONE_ID).mAddressesToDuck)
                .containsExactly(PRIMARY_MEDIA_ADDRESS);
    }

    @Test
    public void onFocusChange_withDuckedDevices_updatesAddressesToUnduck() {
        mCarDucking.onFocusChange(ONE_ZONE_CHANGE, mMediaNavFocusHolders);

        mCarDucking.onFocusChange(ONE_ZONE_CHANGE, mMediaFocusHolders);

        SparseArray<CarDuckingInfo> newDuckingInfo = mCarDucking.getCurrentDuckingInfo();
        assertThat(newDuckingInfo.get(PRIMARY_ZONE_ID).mAddressesToUnduck)
                .containsExactly(PRIMARY_MEDIA_ADDRESS);
    }

    @Test
    public void onFocusChange_notifiesHalOfUsagesHoldingFocus() {
        mCarDucking.onFocusChange(ONE_ZONE_CHANGE, mMediaFocusHolders);

        verify(mMockAudioControlWrapper).onDevicesToDuckChange(mCarDuckingInfosCaptor.capture());
        int[] usagesHoldingFocus = mCarDuckingInfosCaptor.getValue().get(0).mUsagesHoldingFocus;
        assertThat(usagesHoldingFocus).asList().containsExactly(USAGE_MEDIA);
    }

    @Test
    public void onFocusChange_notifiesHalOfAddressesToDuck() {
        mCarDucking.onFocusChange(ONE_ZONE_CHANGE, mMediaNavFocusHolders);

        verify(mMockAudioControlWrapper).onDevicesToDuckChange(mCarDuckingInfosCaptor.capture());
        List<String> addressesToDuck = mCarDuckingInfosCaptor.getValue().get(0).mAddressesToDuck;
        assertThat(addressesToDuck).containsExactly(PRIMARY_MEDIA_ADDRESS);
    }

    @Test
    public void onFocusChange_notifiesHalOfAddressesToUnduck() {
        mCarDucking.onFocusChange(ONE_ZONE_CHANGE, mMediaNavFocusHolders);

        mCarDucking.onFocusChange(ONE_ZONE_CHANGE, mMediaFocusHolders);

        verify(mMockAudioControlWrapper, times(2))
                .onDevicesToDuckChange(mCarDuckingInfosCaptor.capture());
        List<String> addressesToUnduck = mCarDuckingInfosCaptor.getValue().get(0)
                .mAddressesToUnduck;
        assertThat(addressesToUnduck).containsExactly(PRIMARY_MEDIA_ADDRESS);
    }

    @Test
    public void onFocusChange_withMultipleZones_notifiesForEachZone() {
        int[] zoneIds = new int[]{PRIMARY_ZONE_ID, PASSENGER_ZONE_ID};
        SparseArray<List<AudioFocusInfo>> focusChanges = new SparseArray<>();
        focusChanges.put(PRIMARY_ZONE_ID, List.of(mMediaFocusInfo));
        focusChanges.put(PASSENGER_ZONE_ID, List.of(mNavigationFocusInfo));

        mCarDucking.onFocusChange(zoneIds, focusChanges);

        verify(mMockAudioControlWrapper).onDevicesToDuckChange(mCarDuckingInfosCaptor.capture());
        assertThat(mCarDuckingInfosCaptor.getValue().size()).isEqualTo(zoneIds.length);
    }

    private AudioFocusInfo generateAudioFocusInfoForUsage(int usage) {
        AudioAttributes attributes = new AudioAttributes.Builder().setUsage(usage).build();
        return new AudioFocusInfo(attributes, 0, "client_id", "package.name",
                AUDIOFOCUS_GAIN_TRANSIENT, 0, 0, 0);
    }

    private static SparseArray<CarAudioZone> generateZoneMocks() {
        SparseArray<CarAudioZone> zones = new SparseArray<>();
        CarAudioZone primaryZone = mock(CarAudioZone.class);
        when(primaryZone.getId()).thenReturn(PRIMARY_ZONE_ID);
        when(primaryZone.getAddressForContext(MUSIC)).thenReturn(PRIMARY_MEDIA_ADDRESS);
        when(primaryZone.getAddressForContext(NAVIGATION)).thenReturn(PRIMARY_NAVIGATION_ADDRESS);
        zones.append(PRIMARY_ZONE_ID, primaryZone);

        CarAudioZone passengerZone = mock(CarAudioZone.class);
        when(passengerZone.getId()).thenReturn(PASSENGER_ZONE_ID);
        zones.append(PASSENGER_ZONE_ID, passengerZone);

        CarAudioZone rearZone = mock(CarAudioZone.class);
        when(rearZone.getId()).thenReturn(REAR_ZONE_ID);
        when(rearZone.getAddressForContext(MUSIC)).thenReturn(REAR_MEDIA_ADDRESS);
        zones.append(REAR_ZONE_ID, rearZone);

        return zones;
    }
}
