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

package android.car.testapi;

import android.car.media.CarAudioPatchHandle;
import android.car.media.ICarAudio;
import android.media.AudioDeviceAttributes;
import android.os.IBinder;

import java.util.Collections;
import java.util.List;

/**
 * Fake service that is used by {@link FakeCar} to provide an implementation of {@link ICarAudio}.
 * The reason we couldn't use a mock version of this service is that {@link AudioDeviceAttributes}
 * is annotated with @hide, and Mockito fails to create a mock instance.
 */
final class FakeCarAudioService extends ICarAudio.Stub {
    @Override
    public boolean isAudioFeatureEnabled(int feature) {
        return false;
    }

    @Override
    public void setGroupVolume(int zoneId, int groupId, int index, int flags) {
    }

    @Override
    public int getGroupMaxVolume(int zoneId, int groupId) {
        return 0;
    }

    @Override
    public int getGroupMinVolume(int zoneId, int groupId) {
        return 0;
    }

    @Override
    public int getGroupVolume(int zoneId, int groupId) {
        return 0;
    }

    @Override
    public void setFadeTowardFront(float value) {
    }

    @Override
    public void setBalanceTowardRight(float value) {
    }

    @Override
    public String[] getExternalSources() {
        return new String[] {};
    }

    @Override
    public CarAudioPatchHandle createAudioPatch(String sourceAddress, int usage,
            int gainInMillibels) {
        return null;
    }

    @Override
    public void releaseAudioPatch(CarAudioPatchHandle patch) {
    }

    @Override
    public int getVolumeGroupCount(int zoneId) {
        return 0;
    }

    @Override
    public int getVolumeGroupIdForUsage(int zoneId, int usage) {
        return 0;
    }

    @Override
    public int[] getUsagesForVolumeGroupId(int zoneId, int groupId) {
        return new int[] {};
    }

    @Override
    public int[] getAudioZoneIds() {
        return new int[] {};
    }

    @Override
    public int getZoneIdForUid(int uid) {
        return 0;
    }

    @Override
    public boolean setZoneIdForUid(int zoneId, int uid) {
        return false;
    }

    @Override
    public boolean clearZoneIdForUid(int uid) {
        return false;
    }

    @Override
    public boolean isVolumeGroupMuted(int zoneId, int groupId) {
        return false;
    }

    @Override
    public void setVolumeGroupMute(int zoneId, int groupId, boolean mute, int flags) {
    }

    @Override
    public String getOutputDeviceAddressForUsage(int zoneId, int usage) {
        return "";
    }

    @Override
    public List<AudioDeviceAttributes> getInputDevicesForZoneId(int zoneId) {
        return Collections.emptyList();
    }

    @Override
    public boolean isPlaybackOnVolumeGroupActive(int volumeGroupId, int audioZoneId) {
        return false;
    }

    @Override
    public void registerVolumeCallback(IBinder binder) {
    }

    @Override
    public void unregisterVolumeCallback(IBinder binder) {
    }
}
