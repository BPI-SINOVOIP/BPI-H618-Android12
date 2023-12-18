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

import static com.google.common.truth.Truth.assertThat;

import static org.mockito.Mockito.verify;
import static org.mockito.Mockito.when;
import static org.testng.Assert.expectThrows;

import android.annotation.UserIdInt;
import android.os.UserHandle;
import android.util.SparseBooleanArray;
import android.util.SparseIntArray;

import org.junit.Before;
import org.junit.Test;
import org.junit.runner.RunWith;
import org.mockito.Mock;
import org.mockito.Mockito;
import org.mockito.junit.MockitoJUnitRunner;

import java.util.List;

@RunWith(MockitoJUnitRunner.class)
public class CarVolumeGroupUnitTest {
    private static final int ZONE_ID = 0;
    private static final int GROUP_ID = 0;
    private static final int STEP_VALUE = 2;
    private static final int MIN_GAIN = 3;
    private static final int MAX_GAIN = 10;
    private static final int DEFAULT_GAIN = 5;
    private static final int DEFAULT_GAIN_INDEX = (DEFAULT_GAIN - MIN_GAIN) / STEP_VALUE;
    private static final int MIN_GAIN_INDEX = 0;
    private static final int MAX_GAIN_INDEX = (MAX_GAIN - MIN_GAIN) / STEP_VALUE;
    private static final int TEST_GAIN_INDEX = 2;
    private static final int TEST_USER_10 = 10;
    private static final int TEST_USER_11 = 11;
    private static final String MEDIA_DEVICE_ADDRESS = "music";
    private static final String NAVIGATION_DEVICE_ADDRESS = "navigation";
    private static final String OTHER_ADDRESS = "other_address";

    private CarAudioDeviceInfo mMediaDeviceInfo;
    private CarAudioDeviceInfo mNavigationDeviceInfo;

    @Mock
    CarAudioSettings mSettingsMock;

    @Before
    public void setUp() {
        mMediaDeviceInfo = new InfoBuilder(MEDIA_DEVICE_ADDRESS).build();
        mNavigationDeviceInfo = new InfoBuilder(NAVIGATION_DEVICE_ADDRESS).build();
    }

    @Test
    public void setDeviceInfoForContext_associatesDeviceAddresses() {
        CarVolumeGroup.Builder builder = getBuilder();

        builder.setDeviceInfoForContext(CarAudioContext.MUSIC, mMediaDeviceInfo);
        builder.setDeviceInfoForContext(CarAudioContext.NAVIGATION, mNavigationDeviceInfo);
        CarVolumeGroup carVolumeGroup = builder.build();

        assertThat(carVolumeGroup.getAddresses()).containsExactly(MEDIA_DEVICE_ADDRESS,
                NAVIGATION_DEVICE_ADDRESS);
    }

    @Test
    public void setDeviceInfoForContext_associatesContexts() {
        CarVolumeGroup.Builder builder = getBuilder();

        builder.setDeviceInfoForContext(CarAudioContext.MUSIC, mMediaDeviceInfo);
        builder.setDeviceInfoForContext(CarAudioContext.NAVIGATION, mNavigationDeviceInfo);
        CarVolumeGroup carVolumeGroup = builder.build();

        assertThat(carVolumeGroup.getContexts()).asList().containsExactly(CarAudioContext.MUSIC,
                CarAudioContext.NAVIGATION);
    }

    @Test
    public void setDeviceInfoForContext_withDifferentStepSize_throws() {
        CarVolumeGroup.Builder builder = getBuilder();
        builder.setDeviceInfoForContext(CarAudioContext.MUSIC, mMediaDeviceInfo);
        CarAudioDeviceInfo differentStepValueDevice = new InfoBuilder(NAVIGATION_DEVICE_ADDRESS)
                .setStepValue(mMediaDeviceInfo.getStepValue() + 1).build();

        IllegalArgumentException thrown = expectThrows(IllegalArgumentException.class,
                () -> builder.setDeviceInfoForContext(CarAudioContext.NAVIGATION,
                        differentStepValueDevice));

        assertThat(thrown).hasMessageThat()
                .contains("Gain controls within one group must have same step value");
    }

    @Test
    public void setDeviceInfoForContext_withSameContext_throws() {
        CarVolumeGroup.Builder builder = getBuilder();
        builder.setDeviceInfoForContext(CarAudioContext.MUSIC, mMediaDeviceInfo);

        IllegalArgumentException thrown = expectThrows(IllegalArgumentException.class,
                () -> builder.setDeviceInfoForContext(CarAudioContext.MUSIC,
                        mNavigationDeviceInfo));

        assertThat(thrown).hasMessageThat()
                .contains("has already been set to");
    }

    @Test
    public void setDeviceInfoForContext_withFirstCall_setsMinGain() {
        CarVolumeGroup.Builder builder = getBuilder();

        builder.setDeviceInfoForContext(CarAudioContext.MUSIC, mMediaDeviceInfo);

        assertThat(builder.mMinGain).isEqualTo(mMediaDeviceInfo.getMinGain());
    }

    @Test
    public void setDeviceInfoForContext_withFirstCall_setsMaxGain() {
        CarVolumeGroup.Builder builder = getBuilder();

        builder.setDeviceInfoForContext(CarAudioContext.MUSIC, mMediaDeviceInfo);

        assertThat(builder.mMaxGain).isEqualTo(mMediaDeviceInfo.getMaxGain());
    }

    @Test
    public void setDeviceInfoForContext_withFirstCall_setsDefaultGain() {
        CarVolumeGroup.Builder builder = getBuilder();

        builder.setDeviceInfoForContext(CarAudioContext.MUSIC, mMediaDeviceInfo);

        assertThat(builder.mDefaultGain).isEqualTo(mMediaDeviceInfo.getDefaultGain());
    }

    @Test
    public void setDeviceInfoForContext_SecondCallWithSmallerMinGain_updatesMinGain() {
        CarVolumeGroup.Builder builder = getBuilder();
        builder.setDeviceInfoForContext(CarAudioContext.MUSIC, mMediaDeviceInfo);
        CarAudioDeviceInfo secondInfo = new InfoBuilder(NAVIGATION_DEVICE_ADDRESS)
                .setMinGain(mMediaDeviceInfo.getMinGain() - 1).build();

        builder.setDeviceInfoForContext(CarAudioContext.NAVIGATION, secondInfo);

        assertThat(builder.mMinGain).isEqualTo(secondInfo.getMinGain());
    }

    @Test
    public void setDeviceInfoForContext_SecondCallWithLargerMinGain_keepsFirstMinGain() {
        CarVolumeGroup.Builder builder = getBuilder();
        builder.setDeviceInfoForContext(CarAudioContext.MUSIC, mMediaDeviceInfo);
        CarAudioDeviceInfo secondInfo = new InfoBuilder(NAVIGATION_DEVICE_ADDRESS)
                .setMinGain(mMediaDeviceInfo.getMinGain() + 1).build();

        builder.setDeviceInfoForContext(CarAudioContext.NAVIGATION, secondInfo);

        assertThat(builder.mMinGain).isEqualTo(mMediaDeviceInfo.getMinGain());
    }

    @Test
    public void setDeviceInfoForContext_SecondCallWithLargerMaxGain_updatesMaxGain() {
        CarVolumeGroup.Builder builder = getBuilder();
        builder.setDeviceInfoForContext(CarAudioContext.MUSIC, mMediaDeviceInfo);
        CarAudioDeviceInfo secondInfo = new InfoBuilder(NAVIGATION_DEVICE_ADDRESS)
                .setMaxGain(mMediaDeviceInfo.getMaxGain() + 1).build();

        builder.setDeviceInfoForContext(CarAudioContext.NAVIGATION, secondInfo);

        assertThat(builder.mMaxGain).isEqualTo(secondInfo.getMaxGain());
    }

    @Test
    public void setDeviceInfoForContext_SecondCallWithSmallerMaxGain_keepsFirstMaxGain() {
        CarVolumeGroup.Builder builder = getBuilder();
        builder.setDeviceInfoForContext(CarAudioContext.MUSIC, mMediaDeviceInfo);
        CarAudioDeviceInfo secondInfo = new InfoBuilder(NAVIGATION_DEVICE_ADDRESS)
                .setMaxGain(mMediaDeviceInfo.getMaxGain() - 1).build();

        builder.setDeviceInfoForContext(CarAudioContext.NAVIGATION, secondInfo);

        assertThat(builder.mMaxGain).isEqualTo(mMediaDeviceInfo.getMaxGain());
    }

    @Test
    public void setDeviceInfoForContext_SecondCallWithLargerDefaultGain_updatesDefaultGain() {
        CarVolumeGroup.Builder builder = getBuilder();
        builder.setDeviceInfoForContext(CarAudioContext.MUSIC, mMediaDeviceInfo);
        CarAudioDeviceInfo secondInfo = new InfoBuilder(NAVIGATION_DEVICE_ADDRESS)
                .setDefaultGain(mMediaDeviceInfo.getDefaultGain() + 1).build();

        builder.setDeviceInfoForContext(CarAudioContext.NAVIGATION, secondInfo);

        assertThat(builder.mDefaultGain).isEqualTo(secondInfo.getDefaultGain());
    }

    @Test
    public void setDeviceInfoForContext_SecondCallWithSmallerDefaultGain_keepsFirstDefaultGain() {
        CarVolumeGroup.Builder builder = getBuilder();
        builder.setDeviceInfoForContext(CarAudioContext.MUSIC, mMediaDeviceInfo);
        CarAudioDeviceInfo secondInfo = new InfoBuilder(NAVIGATION_DEVICE_ADDRESS)
                .setDefaultGain(mMediaDeviceInfo.getDefaultGain() - 1).build();

        builder.setDeviceInfoForContext(CarAudioContext.NAVIGATION, secondInfo);

        assertThat(builder.mDefaultGain).isEqualTo(mMediaDeviceInfo.getDefaultGain());
    }

    @Test
    public void builderBuild_withNoCallToSetDeviceInfoForContext_throws() {
        CarVolumeGroup.Builder builder = getBuilder();

        Exception e = expectThrows(IllegalArgumentException.class, builder::build);

        assertThat(e).hasMessageThat().isEqualTo(
                "setDeviceInfoForContext has to be called at least once before building");
    }

    @Test
    public void builderBuild_withNoStoredGain_usesDefaultGain() {
        CarVolumeGroup.Builder builder = getBuilder().setDeviceInfoForContext(CarAudioContext.MUSIC,
                mMediaDeviceInfo);
        when(mSettingsMock.getStoredVolumeGainIndexForUser(UserHandle.USER_CURRENT, ZONE_ID,
                GROUP_ID)).thenReturn(-1);


        CarVolumeGroup carVolumeGroup = builder.build();

        assertThat(carVolumeGroup.getCurrentGainIndex()).isEqualTo(DEFAULT_GAIN_INDEX);
    }

    @Test
    public void builderBuild_withTooLargeStoredGain_usesDefaultGain() {
        CarVolumeGroup.Builder builder = getBuilder().setDeviceInfoForContext(CarAudioContext.MUSIC,
                mMediaDeviceInfo);
        when(mSettingsMock.getStoredVolumeGainIndexForUser(UserHandle.USER_CURRENT, ZONE_ID,
                GROUP_ID)).thenReturn(MAX_GAIN_INDEX + 1);

        CarVolumeGroup carVolumeGroup = builder.build();

        assertThat(carVolumeGroup.getCurrentGainIndex()).isEqualTo(DEFAULT_GAIN_INDEX);
    }

    @Test
    public void builderBuild_withTooSmallStoredGain_usesDefaultGain() {
        CarVolumeGroup.Builder builder = getBuilder().setDeviceInfoForContext(CarAudioContext.MUSIC,
                mMediaDeviceInfo);
        when(mSettingsMock.getStoredVolumeGainIndexForUser(UserHandle.USER_CURRENT, ZONE_ID,
                GROUP_ID)).thenReturn(MIN_GAIN_INDEX - 1);

        CarVolumeGroup carVolumeGroup = builder.build();

        assertThat(carVolumeGroup.getCurrentGainIndex()).isEqualTo(DEFAULT_GAIN_INDEX);
    }

    @Test
    public void builderBuild_withValidStoredGain_usesStoredGain() {
        CarVolumeGroup.Builder builder = getBuilder().setDeviceInfoForContext(CarAudioContext.MUSIC,
                mMediaDeviceInfo);
        when(mSettingsMock.getStoredVolumeGainIndexForUser(UserHandle.USER_CURRENT, ZONE_ID,
                GROUP_ID)).thenReturn(MAX_GAIN_INDEX - 1);

        CarVolumeGroup carVolumeGroup = builder.build();

        assertThat(carVolumeGroup.getCurrentGainIndex()).isEqualTo(MAX_GAIN_INDEX - 1);
    }

    @Test
    public void getAddressForContext_withSupportedContext_returnsAddress() {
        CarVolumeGroup carVolumeGroup = getCarVolumeGroupWithMusicBound();

        assertThat(carVolumeGroup.getAddressForContext(CarAudioContext.MUSIC))
                .isEqualTo(mMediaDeviceInfo.getAddress());
    }

    @Test
    public void getAddressForContext_withUnsupportedContext_returnsNull() {
        CarVolumeGroup carVolumeGroup = getCarVolumeGroupWithMusicBound();

        assertThat(carVolumeGroup.getAddressForContext(CarAudioContext.NAVIGATION)).isNull();
    }

    @Test
    public void isMuted_whenDefault_returnsFalse() {
        CarVolumeGroup carVolumeGroup = getCarVolumeGroupWithMusicBound();

        assertThat(carVolumeGroup.isMuted()).isFalse();
    }

    @Test
    public void isMuted_afterMuting_returnsTrue() {
        CarVolumeGroup carVolumeGroup = getCarVolumeGroupWithMusicBound();

        carVolumeGroup.setMute(true);

        assertThat(carVolumeGroup.isMuted()).isTrue();
    }

    @Test
    public void isMuted_afterUnMuting_returnsFalse() {
        CarVolumeGroup carVolumeGroup = getCarVolumeGroupWithMusicBound();

        carVolumeGroup.setMute(false);

        assertThat(carVolumeGroup.isMuted()).isFalse();
    }

    @Test
    public void setMute_withMutedState_storesValueToSetting() {
        CarAudioSettings settings = new SettingsBuilder(0, 0)
                .setMuteForUser10(false)
                .setIsPersistVolumeGroupEnabled(true)
                .build();
        CarVolumeGroup carVolumeGroup = getCarVolumeGroupWithNavigationBound(settings, true);
        carVolumeGroup.loadVolumesSettingsForUser(TEST_USER_10);

        carVolumeGroup.setMute(true);

        verify(settings)
                .storeVolumeGroupMuteForUser(TEST_USER_10, 0, 0, true);
    }

    @Test
    public void setMute_withUnMutedState_storesValueToSetting() {
        CarAudioSettings settings = new SettingsBuilder(0, 0)
                .setMuteForUser10(false)
                .setIsPersistVolumeGroupEnabled(true)
                .build();
        CarVolumeGroup carVolumeGroup = getCarVolumeGroupWithNavigationBound(settings, true);
        carVolumeGroup.loadVolumesSettingsForUser(TEST_USER_10);

        carVolumeGroup.setMute(false);

        verify(settings)
                .storeVolumeGroupMuteForUser(TEST_USER_10, 0, 0, false);
    }

    @Test
    public void getContextsForAddress_returnsContextsBoundToThatAddress() {
        CarVolumeGroup carVolumeGroup = testVolumeGroupSetup();

        List<Integer> contextsList = carVolumeGroup.getContextsForAddress(MEDIA_DEVICE_ADDRESS);

        assertThat(contextsList).containsExactly(CarAudioContext.MUSIC,
                CarAudioContext.CALL, CarAudioContext.CALL_RING);
    }

    @Test
    public void getContextsForAddress_returnsEmptyArrayIfAddressNotBound() {
        CarVolumeGroup carVolumeGroup = testVolumeGroupSetup();

        List<Integer> contextsList = carVolumeGroup.getContextsForAddress(OTHER_ADDRESS);

        assertThat(contextsList).isEmpty();
    }

    @Test
    public void getCarAudioDeviceInfoForAddress_returnsExpectedDevice() {
        CarVolumeGroup carVolumeGroup = testVolumeGroupSetup();

        CarAudioDeviceInfo actualDevice = carVolumeGroup.getCarAudioDeviceInfoForAddress(
                MEDIA_DEVICE_ADDRESS);

        assertThat(actualDevice).isEqualTo(mMediaDeviceInfo);
    }

    @Test
    public void getCarAudioDeviceInfoForAddress_returnsNullIfAddressNotBound() {
        CarVolumeGroup carVolumeGroup = testVolumeGroupSetup();

        CarAudioDeviceInfo actualDevice = carVolumeGroup.getCarAudioDeviceInfoForAddress(
                OTHER_ADDRESS);

        assertThat(actualDevice).isNull();
    }

    @Test
    public void setCurrentGainIndex_setsGainOnAllBoundDevices() {
        CarVolumeGroup carVolumeGroup = testVolumeGroupSetup();

        carVolumeGroup.setCurrentGainIndex(TEST_GAIN_INDEX);

        verify(mMediaDeviceInfo).setCurrentGain(7);
        verify(mNavigationDeviceInfo).setCurrentGain(7);
    }

    @Test
    public void setCurrentGainIndex_updatesCurrentGainIndex() {
        CarVolumeGroup carVolumeGroup = testVolumeGroupSetup();

        carVolumeGroup.setCurrentGainIndex(TEST_GAIN_INDEX);

        assertThat(carVolumeGroup.getCurrentGainIndex()).isEqualTo(TEST_GAIN_INDEX);
    }

    @Test
    public void setCurrentGainIndex_checksNewGainIsAboveMin() {
        CarVolumeGroup carVolumeGroup = testVolumeGroupSetup();

        IllegalArgumentException thrown = expectThrows(IllegalArgumentException.class,
                () -> carVolumeGroup.setCurrentGainIndex(MIN_GAIN_INDEX - 1));
        assertThat(thrown).hasMessageThat()
                .contains("Gain out of range (" + MIN_GAIN + ":" + MAX_GAIN + ")");
    }

    @Test
    public void setCurrentGainIndex_checksNewGainIsBelowMax() {
        CarVolumeGroup carVolumeGroup = testVolumeGroupSetup();

        IllegalArgumentException thrown = expectThrows(IllegalArgumentException.class,
                () -> carVolumeGroup.setCurrentGainIndex(MAX_GAIN_INDEX + 1));
        assertThat(thrown).hasMessageThat()
                .contains("Gain out of range (" + MIN_GAIN + ":" + MAX_GAIN + ")");
    }

    @Test
    public void setCurrentGainIndex_setsCurrentGainIndexForUser() {
        CarAudioSettings settings = new SettingsBuilder(0, 0)
                .setGainIndexForUser(TEST_USER_11)
                .build();
        CarVolumeGroup carVolumeGroup = getCarVolumeGroupWithNavigationBound(settings, false);
        carVolumeGroup.loadVolumesSettingsForUser(TEST_USER_11);

        carVolumeGroup.setCurrentGainIndex(MIN_GAIN);

        verify(settings).storeVolumeGainIndexForUser(TEST_USER_11, 0, 0, MIN_GAIN);
    }

    @Test
    public void setCurrentGainIndex_setsCurrentGainIndexForDefaultUser() {
        CarAudioSettings settings = new SettingsBuilder(0, 0)
                .setGainIndexForUser(UserHandle.USER_CURRENT)
                .build();
        CarVolumeGroup carVolumeGroup = getCarVolumeGroupWithNavigationBound(settings, false);

        carVolumeGroup.setCurrentGainIndex(MIN_GAIN);

        verify(settings)
                .storeVolumeGainIndexForUser(UserHandle.USER_CURRENT, 0, 0, MIN_GAIN);
    }

    @Test
    public void loadVolumesSettingsForUser_withMutedState_loadsMuteStateForUser() {
        CarVolumeGroup carVolumeGroup = getVolumeGroupWithMuteAndNavBound(true, true, true);

        carVolumeGroup.loadVolumesSettingsForUser(TEST_USER_10);

        assertThat(carVolumeGroup.isMuted()).isTrue();
    }

    @Test
    public void loadVolumesSettingsForUser_withDisabledUseVolumeGroupMute_doesNotLoadMute() {
        CarVolumeGroup carVolumeGroup = getVolumeGroupWithMuteAndNavBound(true, true, false);

        carVolumeGroup.loadVolumesSettingsForUser(TEST_USER_10);

        assertThat(carVolumeGroup.isMuted()).isFalse();
    }

    @Test
    public void loadVolumesSettingsForUser_withUnMutedState_loadsMuteStateForUser() {
        CarVolumeGroup carVolumeGroup = getVolumeGroupWithMuteAndNavBound(false, true, true);

        carVolumeGroup.loadVolumesSettingsForUser(TEST_USER_10);

        assertThat(carVolumeGroup.isMuted()).isFalse();
    }

    @Test
    public void loadVolumesSettingsForUser_withMutedStateAndNoPersist_returnsDefaultMuteState() {
        CarVolumeGroup carVolumeGroup = getVolumeGroupWithMuteAndNavBound(true, false, true);

        carVolumeGroup.loadVolumesSettingsForUser(TEST_USER_10);

        assertThat(carVolumeGroup.isMuted()).isFalse();
    }

    @Test
    public void hasCriticalAudioContexts_withoutCriticalContexts_returnsFalse() {
        CarVolumeGroup carVolumeGroup = getCarVolumeGroupWithMusicBound();

        assertThat(carVolumeGroup.hasCriticalAudioContexts()).isFalse();
    }

    @Test
    public void hasCriticalAudioContexts_withCriticalContexts_returnsTrue() {
        CarVolumeGroup carVolumeGroup = getBuilder()
                .setDeviceInfoForContext(CarAudioContext.EMERGENCY, mMediaDeviceInfo)
                .build();

        assertThat(carVolumeGroup.hasCriticalAudioContexts()).isTrue();
    }

    private CarVolumeGroup getCarVolumeGroupWithMusicBound() {
        return getBuilder()
                .setDeviceInfoForContext(CarAudioContext.MUSIC, mMediaDeviceInfo)
                .build();
    }

    private CarVolumeGroup getCarVolumeGroupWithNavigationBound(CarAudioSettings settings,
            boolean useCarVolumeGroupMute) {
        return new CarVolumeGroup.Builder(0, 0, settings, useCarVolumeGroupMute)
                .setDeviceInfoForContext(CarAudioContext.NAVIGATION, mNavigationDeviceInfo)
                .build();
    }

    CarVolumeGroup getVolumeGroupWithMuteAndNavBound(boolean isMuted, boolean persistMute,
            boolean useCarVolumeGroupMute) {
        CarAudioSettings settings = new SettingsBuilder(0, 0)
                .setMuteForUser10(isMuted)
                .setIsPersistVolumeGroupEnabled(persistMute)
                .build();
        return getCarVolumeGroupWithNavigationBound(settings, useCarVolumeGroupMute);
    }

    private CarVolumeGroup testVolumeGroupSetup() {
        CarVolumeGroup.Builder builder = getBuilder();

        builder.setDeviceInfoForContext(CarAudioContext.MUSIC, mMediaDeviceInfo);
        builder.setDeviceInfoForContext(CarAudioContext.CALL, mMediaDeviceInfo);
        builder.setDeviceInfoForContext(CarAudioContext.CALL_RING, mMediaDeviceInfo);

        builder.setDeviceInfoForContext(CarAudioContext.NAVIGATION, mNavigationDeviceInfo);
        builder.setDeviceInfoForContext(CarAudioContext.ALARM, mNavigationDeviceInfo);
        builder.setDeviceInfoForContext(CarAudioContext.NOTIFICATION, mNavigationDeviceInfo);

        return builder.build();
    }

    CarVolumeGroup.Builder getBuilder() {
        return new CarVolumeGroup.Builder(ZONE_ID, GROUP_ID, mSettingsMock, true);
    }

    private static final class SettingsBuilder {
        private final SparseIntArray mStoredGainIndexes = new SparseIntArray();
        private final SparseBooleanArray mStoreMuteStates = new SparseBooleanArray();
        private final int mZoneId;
        private final int mGroupId;

        private boolean mPersistMute;

        SettingsBuilder(int zoneId, int groupId) {
            mZoneId = zoneId;
            mGroupId = groupId;
        }

        SettingsBuilder setGainIndexForUser(@UserIdInt int userId) {
            mStoredGainIndexes.put(userId, TEST_GAIN_INDEX);
            return this;
        }

        SettingsBuilder setMuteForUser10(boolean mute) {
            mStoreMuteStates.put(CarVolumeGroupUnitTest.TEST_USER_10, mute);
            return this;
        }

        SettingsBuilder setIsPersistVolumeGroupEnabled(boolean persistMute) {
            mPersistMute = persistMute;
            return this;
        }

        CarAudioSettings build() {
            CarAudioSettings settingsMock = Mockito.mock(CarAudioSettings.class);
            for (int storeIndex = 0; storeIndex < mStoredGainIndexes.size(); storeIndex++) {
                int gainUserId = mStoredGainIndexes.keyAt(storeIndex);
                when(settingsMock
                        .getStoredVolumeGainIndexForUser(gainUserId, mZoneId,
                                mGroupId)).thenReturn(
                        mStoredGainIndexes.get(gainUserId, DEFAULT_GAIN));
            }
            for (int muteIndex = 0; muteIndex < mStoreMuteStates.size(); muteIndex++) {
                int muteUserId = mStoreMuteStates.keyAt(muteIndex);
                when(settingsMock.getVolumeGroupMuteForUser(muteUserId, mZoneId, mGroupId))
                        .thenReturn(mStoreMuteStates.get(muteUserId, false));
                when(settingsMock.isPersistVolumeGroupMuteEnabled(muteUserId))
                        .thenReturn(mPersistMute);
            }
            return settingsMock;
        }
    }

    private static final class InfoBuilder {
        private final String mAddress;

        private int mStepValue = STEP_VALUE;
        private int mDefaultGain = DEFAULT_GAIN;
        private int mMinGain = MIN_GAIN;
        private int mMaxGain = MAX_GAIN;

        InfoBuilder(String address) {
            mAddress = address;
        }

        InfoBuilder setStepValue(int stepValue) {
            mStepValue = stepValue;
            return this;
        }

        InfoBuilder setDefaultGain(int defaultGain) {
            mDefaultGain = defaultGain;
            return this;
        }

        InfoBuilder setMinGain(int minGain) {
            mMinGain = minGain;
            return this;
        }

        InfoBuilder setMaxGain(int maxGain) {
            mMaxGain = maxGain;
            return this;
        }

        CarAudioDeviceInfo build() {
            CarAudioDeviceInfo infoMock = Mockito.mock(CarAudioDeviceInfo.class);
            when(infoMock.getStepValue()).thenReturn(mStepValue);
            when(infoMock.getDefaultGain()).thenReturn(mDefaultGain);
            when(infoMock.getMaxGain()).thenReturn(mMaxGain);
            when(infoMock.getMinGain()).thenReturn(mMinGain);
            when(infoMock.getAddress()).thenReturn(mAddress);
            return infoMock;
        }
    }
}
