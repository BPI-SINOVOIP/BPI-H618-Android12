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

package android.car.apitest.media;

import static com.google.common.truth.Truth.assertThat;

import static org.junit.Assume.assumeTrue;

import android.car.Car;
import android.car.apitest.CarApiTestBase;
import android.car.media.CarAudioManager;
import android.media.AudioAttributes;
import android.media.AudioDeviceInfo;
import android.os.Process;

import androidx.test.ext.junit.runners.AndroidJUnit4;

import org.junit.Before;
import org.junit.Test;
import org.junit.runner.RunWith;

import java.util.List;

@RunWith(AndroidJUnit4.class)
public class CarAudioManagerTest extends CarApiTestBase {

    private CarAudioManager mCarAudioManager;

    @Before
    public void setUp() throws Exception {
        mCarAudioManager = (CarAudioManager) getCar().getCarManager(Car.AUDIO_SERVICE);
        assertThat(mCarAudioManager).isNotNull();
    }

    @Test
    public void test_getAudioZoneIds() throws Exception {
        assumeDynamicRoutingIsEnabled();

        List<Integer> zoneIds = mCarAudioManager.getAudioZoneIds();
        assertThat(zoneIds).isNotEmpty();
        assertThat(zoneIds).contains(CarAudioManager.PRIMARY_AUDIO_ZONE);
    }

    @Test
    public void test_isAudioFeatureEnabled() throws Exception {
        // nothing to assert. Just call the API.
        mCarAudioManager.isAudioFeatureEnabled(CarAudioManager.AUDIO_FEATURE_DYNAMIC_ROUTING);
        mCarAudioManager.isAudioFeatureEnabled(CarAudioManager.AUDIO_FEATURE_VOLUME_GROUP_MUTING);
    }

    @Test
    public void test_getVolumeGroupCount() throws Exception {
        int primaryZoneCount = mCarAudioManager.getVolumeGroupCount();
        assertThat(
                mCarAudioManager.getVolumeGroupCount(CarAudioManager.PRIMARY_AUDIO_ZONE)).isEqualTo(
                primaryZoneCount);
    }

    @Test
    public void test_getGroupVolume() throws Exception {
        int groudId = 0;
        int volume = mCarAudioManager.getGroupVolume(groudId);
        assertThat(mCarAudioManager.getGroupVolume(CarAudioManager.PRIMARY_AUDIO_ZONE,
                groudId)).isEqualTo(volume);
        int maxVolume = mCarAudioManager.getGroupMaxVolume(groudId);
        assertThat(mCarAudioManager.getGroupMaxVolume(CarAudioManager.PRIMARY_AUDIO_ZONE,
                groudId)).isEqualTo(maxVolume);
        int minVolume = mCarAudioManager.getGroupMinVolume(groudId);
        assertThat(mCarAudioManager.getGroupMinVolume(CarAudioManager.PRIMARY_AUDIO_ZONE,
                groudId)).isEqualTo(minVolume);
        assertThat(volume).isAtLeast(minVolume);
        assertThat(volume).isAtMost(maxVolume);
    }

    @Test
    public void test_setGroupVolume() throws Exception {
        int groudId = 0;
        int volume = mCarAudioManager.getGroupVolume(groudId);
        mCarAudioManager.setGroupVolume(groudId, volume, 0);
        mCarAudioManager.setGroupVolume(CarAudioManager.PRIMARY_AUDIO_ZONE, groudId, volume, 0);
        assertThat(mCarAudioManager.getGroupVolume(groudId)).isEqualTo(volume);
    }

    @Test
    public void test_getInputDevicesForZoneId() throws Exception {
        assumeDynamicRoutingIsEnabled();

        List<AudioDeviceInfo> info = mCarAudioManager.getInputDevicesForZoneId(
                CarAudioManager.PRIMARY_AUDIO_ZONE);
        assertThat(info).isNotEmpty();
    }

    @Test
    public void test_getOutputDeviceForUsage() throws Exception {
        assumeDynamicRoutingIsEnabled();

        AudioDeviceInfo device = mCarAudioManager.getOutputDeviceForUsage(
                CarAudioManager.PRIMARY_AUDIO_ZONE, AudioAttributes.USAGE_MEDIA);
        assertThat(device).isNotNull();
    }

    @Test
    public void test_getVolumeGroupIdForUsage() throws Exception {
        int groupId = mCarAudioManager.getVolumeGroupIdForUsage(AudioAttributes.USAGE_MEDIA);
        assertThat(mCarAudioManager.getVolumeGroupIdForUsage(CarAudioManager.PRIMARY_AUDIO_ZONE,
                AudioAttributes.USAGE_MEDIA)).isEqualTo(groupId);
        int primaryZoneCount = mCarAudioManager.getVolumeGroupCount();
        assertThat(groupId).isLessThan(primaryZoneCount);
    }

    @Test
    public void test_getZoneIdForUid() throws Exception {
        assumeDynamicRoutingIsEnabled();

        assertThat(mCarAudioManager.getZoneIdForUid(Process.myUid())).isEqualTo(
                CarAudioManager.PRIMARY_AUDIO_ZONE);
    }

    @Test
    public void test_isPlaybackOnVolumeGroupActive() throws Exception {
        assumeDynamicRoutingIsEnabled();

        // TODO(b/191660867): Better to change this to play something and asert true.
        assertThat(
                mCarAudioManager.isPlaybackOnVolumeGroupActive(CarAudioManager.PRIMARY_AUDIO_ZONE,
                        0)).isFalse();
    }

    private void assumeDynamicRoutingIsEnabled() {
        assumeTrue(mCarAudioManager.isAudioFeatureEnabled(
                CarAudioManager.AUDIO_FEATURE_DYNAMIC_ROUTING));
    }
}
