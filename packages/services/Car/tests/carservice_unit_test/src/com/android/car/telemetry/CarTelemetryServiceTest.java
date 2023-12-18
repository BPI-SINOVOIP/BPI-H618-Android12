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

package com.android.car.telemetry;

import static com.google.common.truth.Truth.assertThat;

import android.car.telemetry.CarTelemetryManager;
import android.car.telemetry.ManifestKey;
import android.content.Context;

import androidx.test.filters.SmallTest;

import org.junit.Before;
import org.junit.Rule;
import org.junit.Test;
import org.mockito.Mock;
import org.mockito.junit.MockitoJUnit;
import org.mockito.junit.MockitoRule;

@SmallTest
public class CarTelemetryServiceTest {
    @Rule
    public MockitoRule mMockitoRule = MockitoJUnit.rule();
    @Mock
    private Context mContext;

    private final ManifestKey mManifestKeyV1 = new ManifestKey("Name", 1);
    private final ManifestKey mManifestKeyV2 = new ManifestKey("Name", 2);
    private final TelemetryProto.MetricsConfig mMetricsConfig =
            TelemetryProto.MetricsConfig.newBuilder().setScript("no-op").build();

    private CarTelemetryService mService;

    @Before
    public void setUp() {
        mService = new CarTelemetryService(mContext);
    }

    @Test
    public void testAddManifest_newManifest_shouldSucceed() {
        int result = mService.addManifest(mManifestKeyV1, mMetricsConfig.toByteArray());

        assertThat(result).isEqualTo(CarTelemetryManager.ERROR_NONE);
    }

    @Test
    public void testAddManifest_duplicateManifest_shouldFail() {
        mService.addManifest(mManifestKeyV1, mMetricsConfig.toByteArray());

        int result = mService.addManifest(mManifestKeyV1, mMetricsConfig.toByteArray());

        assertThat(result).isEqualTo(CarTelemetryManager.ERROR_SAME_MANIFEST_EXISTS);
    }

    @Test
    public void testAddManifest_invalidManifest_shouldFail() {
        int result = mService.addManifest(mManifestKeyV1, "bad manifest".getBytes());

        assertThat(result).isEqualTo(CarTelemetryManager.ERROR_PARSE_MANIFEST_FAILED);
    }

    @Test
    public void testAddManifest_olderManifest_shouldFail() {
        mService.addManifest(mManifestKeyV2, mMetricsConfig.toByteArray());

        int result = mService.addManifest(mManifestKeyV1, mMetricsConfig.toByteArray());

        assertThat(result).isEqualTo(CarTelemetryManager.ERROR_NEWER_MANIFEST_EXISTS);
    }

    @Test
    public void testAddManifest_newerManifest_shouldReplace() {
        mService.addManifest(mManifestKeyV1, mMetricsConfig.toByteArray());

        int result = mService.addManifest(mManifestKeyV2, mMetricsConfig.toByteArray());

        assertThat(result).isEqualTo(CarTelemetryManager.ERROR_NONE);
    }

    @Test
    public void testRemoveManifest_manifestExists_shouldSucceed() {
        mService.addManifest(mManifestKeyV1, mMetricsConfig.toByteArray());

        boolean result = mService.removeManifest(mManifestKeyV1);

        assertThat(result).isTrue();
    }

    @Test
    public void testRemoveManifest_manifestDoesNotExist_shouldFail() {
        boolean result = mService.removeManifest(mManifestKeyV1);

        assertThat(result).isFalse();
    }

    @Test
    public void testRemoveAllManifests_shouldSucceed() {
        mService.addManifest(mManifestKeyV1, mMetricsConfig.toByteArray());
        mService.addManifest(mManifestKeyV2, mMetricsConfig.toByteArray());

        mService.removeAllManifests();

        // verify that the manifests are cleared by adding them again, should succeed
        int result = mService.addManifest(mManifestKeyV1, mMetricsConfig.toByteArray());
        assertThat(result).isEqualTo(CarTelemetryManager.ERROR_NONE);
        result = mService.addManifest(mManifestKeyV2, mMetricsConfig.toByteArray());
        assertThat(result).isEqualTo(CarTelemetryManager.ERROR_NONE);
    }
}
