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
import static android.media.AudioManager.AUDIOFOCUS_GAIN;
import static android.media.AudioManager.AUDIOFOCUS_REQUEST_GRANTED;

import static com.google.common.truth.Truth.assertWithMessage;

import static org.mockito.ArgumentMatchers.eq;
import static org.mockito.Mockito.description;
import static org.mockito.Mockito.mock;
import static org.mockito.Mockito.verify;
import static org.mockito.Mockito.when;
import static org.testng.Assert.assertThrows;

import android.car.media.CarAudioManager;
import android.content.pm.PackageManager;
import android.media.AudioAttributes;
import android.media.AudioFocusInfo;
import android.media.AudioManager;
import android.media.audiopolicy.AudioPolicy;
import android.os.Build;
import android.os.Bundle;
import android.util.SparseArray;

import org.junit.Before;
import org.junit.Test;
import org.junit.runner.RunWith;
import org.mockito.ArgumentCaptor;
import org.mockito.Mock;
import org.mockito.junit.MockitoJUnitRunner;

import java.util.List;

@RunWith(MockitoJUnitRunner.class)
public final class CarZonesAudioFocusUnitTest {
    private static final String CLIENT_ID = "media-client-id";
    private static final String PACKAGE_NAME = "com.android.car.audio";
    private static final int AUDIOFOCUS_FLAG = 0;
    private static final int PRIMARY_ZONE_ID = CarAudioManager.PRIMARY_AUDIO_ZONE;
    private static final int SECONDARY_ZONE_ID = CarAudioManager.PRIMARY_AUDIO_ZONE + 1;
    private static final int CLIENT_UID = 1086753;

    private final SparseArray<CarAudioFocus> mFocusMocks = generateMockFocus();
    private final SparseArray<CarAudioZone> mMockZones = generateAudioZones();

    @Mock
    private AudioManager mMockAudioManager;
    @Mock
    private AudioPolicy mAudioPolicy;
    @Mock
    private CarAudioService mCarAudioService;
    @Mock
    private CarAudioSettings mCarAudioSettings;
    @Mock
    private CarZonesAudioFocus.CarFocusCallback mMockCarFocusCallback;
    @Mock
    private PackageManager mMockPackageManager;

    private CarZonesAudioFocus mCarZonesAudioFocus;

    @Before
    public void setUp() {
        mCarZonesAudioFocus = getCarZonesAudioFocus();
    }

    @Test
    public void newCarZonesAudioFocus_withNullAudioManager_throws() {
        assertThrows(NullPointerException.class,
                () -> CarZonesAudioFocus.createCarZonesAudioFocus(null,
                        mMockPackageManager, mMockZones, mCarAudioSettings, true,
                        mMockCarFocusCallback)
        );
    }

    @Test
    public void newCarZonesAudioFocus_withNullPackageManager_throws() {
        assertThrows(NullPointerException.class,
                () -> CarZonesAudioFocus.createCarZonesAudioFocus(mMockAudioManager,
                        null, mMockZones, mCarAudioSettings, true, mMockCarFocusCallback)
        );
    }

    @Test
    public void newCarZonesAudioFocus_withNullCarAudioZones_throws() {
        assertThrows(NullPointerException.class,
                () -> CarZonesAudioFocus.createCarZonesAudioFocus(mMockAudioManager,
                        mMockPackageManager, null, mCarAudioSettings, true, mMockCarFocusCallback)
        );
    }

    @Test
    public void newCarZonesAudioFocus_withEmptyCarAudioZones_throws() {
        assertThrows(IllegalArgumentException.class,
                () -> CarZonesAudioFocus.createCarZonesAudioFocus(mMockAudioManager,
                        mMockPackageManager, new SparseArray<>(), mCarAudioSettings, true,
                        mMockCarFocusCallback)
        );
    }

    @Test
    public void newCarZonesAudioFocus_withNullCarAudioSettings_throws() {
        assertThrows(NullPointerException.class,
                () -> CarZonesAudioFocus.createCarZonesAudioFocus(mMockAudioManager,
                        mMockPackageManager, mMockZones, null, true, mMockCarFocusCallback)
        );
    }

    @Test
    public void newCarZonesAudioFocus_withNullCarFocusCallback_succeeds() {
        CarZonesAudioFocus.createCarZonesAudioFocus(mMockAudioManager, mMockPackageManager,
                mMockZones, mCarAudioSettings, true, null);
    }

    @Test
    public void onAudioFocusRequest_withPrimaryZoneUid_passesRequestToPrimaryZone() {
        withUidRoutingToZone(PRIMARY_ZONE_ID);
        AudioFocusInfo audioFocusInfo = generateAudioFocusRequest();

        mCarZonesAudioFocus.onAudioFocusRequest(audioFocusInfo, AUDIOFOCUS_REQUEST_GRANTED);

        verify(mFocusMocks.get(PRIMARY_ZONE_ID))
                .onAudioFocusRequest(audioFocusInfo, AUDIOFOCUS_REQUEST_GRANTED);
    }

    @Test
    public void onAudioFocusRequest_withSecondaryZoneUid_passesRequestToSecondaryZone() {
        withUidRoutingToZone(SECONDARY_ZONE_ID);
        AudioFocusInfo audioFocusInfo = generateAudioFocusRequest();

        mCarZonesAudioFocus.onAudioFocusRequest(audioFocusInfo, AUDIOFOCUS_REQUEST_GRANTED);

        verify(mFocusMocks.get(SECONDARY_ZONE_ID))
                .onAudioFocusRequest(audioFocusInfo, AUDIOFOCUS_REQUEST_GRANTED);
    }

    @Test
    public void onAudioFocusRequest_withValidBundledZoneId_passesRequestToBundledZone() {
        withUidRoutingToZone(PRIMARY_ZONE_ID);
        when(mCarAudioService.isAudioZoneIdValid(SECONDARY_ZONE_ID)).thenReturn(true);
        AudioFocusInfo audioFocusInfo = generateAudioFocusInfoWithBundledZoneId(SECONDARY_ZONE_ID);

        mCarZonesAudioFocus.onAudioFocusRequest(audioFocusInfo, AUDIOFOCUS_REQUEST_GRANTED);

        verify(mFocusMocks.get(SECONDARY_ZONE_ID))
                .onAudioFocusRequest(audioFocusInfo, AUDIOFOCUS_REQUEST_GRANTED);
    }

    @Test
    public void onAudioFocusRequest_withInvalidBundledZoneId_passesRequestBasedOnUid() {
        int invalidZoneId = -1;
        withUidRoutingToZone(PRIMARY_ZONE_ID);
        when(mCarAudioService.isAudioZoneIdValid(invalidZoneId)).thenReturn(false);
        AudioFocusInfo audioFocusInfo = generateAudioFocusInfoWithBundledZoneId(invalidZoneId);

        mCarZonesAudioFocus.onAudioFocusRequest(audioFocusInfo, AUDIOFOCUS_REQUEST_GRANTED);

        verify(mFocusMocks.get(PRIMARY_ZONE_ID))
                .onAudioFocusRequest(audioFocusInfo, AUDIOFOCUS_REQUEST_GRANTED);
    }

    @Test
    public void onAudioFocusRequest_withFocusCallback_callsOnFocusChange() {
        List<AudioFocusInfo> focusHolders = List.of(generateAudioFocusRequest());
        when(mFocusMocks.get(PRIMARY_ZONE_ID).getAudioFocusHolders()).thenReturn(focusHolders);
        AudioFocusInfo audioFocusInfo = generateAudioFocusRequest();

        mCarZonesAudioFocus.onAudioFocusRequest(audioFocusInfo, AUDIOFOCUS_REQUEST_GRANTED);

        ArgumentCaptor<SparseArray<List<AudioFocusInfo>>> captor =
                ArgumentCaptor.forClass(SparseArray.class);
        verify(mMockCarFocusCallback)
                .onFocusChange(eq(new int[]{PRIMARY_ZONE_ID}), captor.capture());
        SparseArray<List<AudioFocusInfo>> results = captor.getValue();
        assertWithMessage("Number of lists returned in sparse array")
                .that(results.size()).isEqualTo(1);
        assertWithMessage("Focus holders for primary zone")
                .that(results.get(PRIMARY_ZONE_ID)).isEqualTo(focusHolders);
    }

    @Test
    public void setRestrictFocus_withTrue_restrictsFocusForAllZones() {
        mCarZonesAudioFocus.setRestrictFocus(true);

        verify(mFocusMocks.get(PRIMARY_ZONE_ID),
                description("Primary zone's CarAudioFocus#setRestrictFocus wasn't passed true"))
                .setRestrictFocus(true);
        verify(mFocusMocks.get(SECONDARY_ZONE_ID),
                description("Secondary zone's CarAudioFocus#setRestrictFocus wasn't passed true"))
                .setRestrictFocus(true);
    }

    @Test
    public void setRestrictFocus_withFalse_unrestrictsFocusForAllZones() {
        mCarZonesAudioFocus.setRestrictFocus(false);

        verify(mFocusMocks.get(PRIMARY_ZONE_ID),
                description("Primary zone's CarAudioFocus#setRestrictFocus wasn't passed false"))
                .setRestrictFocus(false);
        verify(mFocusMocks.get(SECONDARY_ZONE_ID),
                description("Secondary zone's CarAudioFocus#setRestrictFocus wasn't passed false"))
                .setRestrictFocus(false);
    }

    @Test
    public void setRestrictFocus_notifiesFocusCallbackForAllZones() {
        mCarZonesAudioFocus.setRestrictFocus(false);

        ArgumentCaptor<SparseArray<List<AudioFocusInfo>>> captor =
                ArgumentCaptor.forClass(SparseArray.class);
        int[] expectedZoneIds = new int[]{PRIMARY_ZONE_ID, SECONDARY_ZONE_ID};
        verify(mMockCarFocusCallback).onFocusChange(eq(expectedZoneIds), captor.capture());
        assertWithMessage("Number of focus holder lists")
                .that(captor.getValue().size()).isEqualTo(2);
    }

    private static SparseArray<CarAudioZone> generateAudioZones() {
        SparseArray<CarAudioZone> zones = new SparseArray<>();
        zones.put(PRIMARY_ZONE_ID, new CarAudioZone(PRIMARY_ZONE_ID, "Primary zone"));
        zones.put(SECONDARY_ZONE_ID, new CarAudioZone(SECONDARY_ZONE_ID, "Secondary zone"));
        return zones;
    }

    private static SparseArray<CarAudioFocus> generateMockFocus() {
        SparseArray<CarAudioFocus> mockFocusZones = new SparseArray<>();
        mockFocusZones.put(PRIMARY_ZONE_ID, mock(CarAudioFocus.class));
        mockFocusZones.put(SECONDARY_ZONE_ID, mock(CarAudioFocus.class));
        return mockFocusZones;
    }

    private static AudioFocusInfo generateAudioFocusRequest() {
        AudioAttributes attributes = new AudioAttributes.Builder().setUsage(USAGE_MEDIA).build();
        return generateAudioFocusInfoWithAttributes(attributes);
    }

    private static AudioFocusInfo generateAudioFocusInfoWithBundledZoneId(int zoneId) {
        Bundle bundle = new Bundle();
        bundle.putInt(CarAudioManager.AUDIOFOCUS_EXTRA_REQUEST_ZONE_ID, zoneId);

        AudioAttributes attributes = new AudioAttributes.Builder()
                .setUsage(USAGE_MEDIA)
                .addBundle(bundle)
                .build();
        return generateAudioFocusInfoWithAttributes(attributes);
    }

    private static AudioFocusInfo generateAudioFocusInfoWithAttributes(AudioAttributes attributes) {
        return new AudioFocusInfo(attributes, CLIENT_UID, CLIENT_ID,
                PACKAGE_NAME, AUDIOFOCUS_GAIN, AudioManager.AUDIOFOCUS_NONE, AUDIOFOCUS_FLAG,
                Build.VERSION.SDK_INT);
    }

    private CarZonesAudioFocus getCarZonesAudioFocus() {
        CarZonesAudioFocus carZonesAudioFocus = new CarZonesAudioFocus(mFocusMocks, true,
                mMockCarFocusCallback);
        carZonesAudioFocus.setOwningPolicy(mCarAudioService, mAudioPolicy);

        return carZonesAudioFocus;
    }

    private void withUidRoutingToZone(int zoneId) {
        when(mCarAudioService.getZoneIdForUid(CLIENT_UID)).thenReturn(zoneId);
    }
}
