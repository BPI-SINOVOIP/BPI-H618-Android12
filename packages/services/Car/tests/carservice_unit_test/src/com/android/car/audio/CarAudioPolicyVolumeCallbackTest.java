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

import static android.car.media.CarAudioManager.PRIMARY_AUDIO_ZONE;
import static android.media.AudioManager.ADJUST_LOWER;
import static android.media.AudioManager.ADJUST_MUTE;
import static android.media.AudioManager.ADJUST_RAISE;
import static android.media.AudioManager.ADJUST_SAME;
import static android.media.AudioManager.ADJUST_TOGGLE_MUTE;
import static android.media.AudioManager.ADJUST_UNMUTE;
import static android.media.AudioManager.FLAG_FROM_KEY;
import static android.media.AudioManager.FLAG_SHOW_UI;

import static com.android.car.audio.CarAudioContext.VOICE_COMMAND;

import static org.mockito.ArgumentMatchers.any;
import static org.mockito.ArgumentMatchers.anyInt;
import static org.mockito.Mockito.never;
import static org.mockito.Mockito.verify;
import static org.mockito.Mockito.when;
import static org.testng.Assert.assertThrows;

import android.media.AudioManager;
import android.media.audiopolicy.AudioPolicy.Builder;

import org.junit.Before;
import org.junit.Rule;
import org.junit.Test;
import org.junit.runner.RunWith;
import org.mockito.Mock;
import org.mockito.junit.MockitoJUnit;
import org.mockito.junit.MockitoJUnitRunner;
import org.mockito.junit.MockitoRule;

@RunWith(MockitoJUnitRunner.class)
public class CarAudioPolicyVolumeCallbackTest {

    private static final int TEST_VOLUME_GROUP = 0;
    private static final int TEST_VOLUME = 5;
    private static final int TEST_MIN_VOLUME = 0;
    private static final int TEST_MAX_VOLUME = 10;

    private static final int TEST_EXPECTED_FLAGS = FLAG_FROM_KEY | FLAG_SHOW_UI;

    @Rule
    public MockitoRule rule = MockitoJUnit.rule();
    @Mock
    private CarAudioService mMockCarAudioService;
    @Mock
    AudioManager mMockAudioManager;
    @Mock
    Builder mMockBuilder;

    private CarAudioPolicyVolumeCallback mCarAudioPolicyVolumeCallback;

    @Before
    public void setUp() {
        mCarAudioPolicyVolumeCallback =
                new CarAudioPolicyVolumeCallback(mMockCarAudioService, mMockAudioManager, false);
        when(mMockCarAudioService.getSuggestedAudioContextForPrimaryZone())
                .thenReturn(VOICE_COMMAND);
        when(mMockCarAudioService.getVolumeGroupIdForAudioContext(anyInt(), anyInt()))
                .thenReturn(TEST_VOLUME_GROUP);
        when(mMockCarAudioService.getGroupMaxVolume(anyInt(), anyInt()))
                .thenReturn(TEST_MAX_VOLUME);
        when(mMockCarAudioService.getGroupMinVolume(anyInt(), anyInt()))
                .thenReturn(TEST_MIN_VOLUME);
    }

    @Test
    public void addVolumeCallbackToPolicy_withNullPolicyBuilder_fails() {
        assertThrows(NullPointerException.class, () ->
                CarAudioPolicyVolumeCallback.addVolumeCallbackToPolicy(
                        null, mMockCarAudioService, mMockAudioManager, false));
    }

    @Test
    public void addVolumeCallbackToPolicy_withNullCarAudioService_fails() {
        assertThrows(NullPointerException.class, () ->
                CarAudioPolicyVolumeCallback.addVolumeCallbackToPolicy(mMockBuilder,
                        null, mMockAudioManager, false));
    }

    @Test
    public void addVolumeCallbackToPolicy_withNullAudioManager_fails() {
        assertThrows(NullPointerException.class, () ->
                CarAudioPolicyVolumeCallback.addVolumeCallbackToPolicy(mMockBuilder,
                        mMockCarAudioService, null, false));
    }


    @Test
    public void addVolumeCallbackToPolicy_registersVolumePolicy() {
        CarAudioPolicyVolumeCallback.addVolumeCallbackToPolicy(mMockBuilder,
                mMockCarAudioService, mMockAudioManager, false);

        verify(mMockBuilder).setAudioPolicyVolumeCallback(any());
    }

    @Test
    public void onVolumeAdjustment_withAdjustRaise_increasesGroupVolume() {
        setGroupVolume(TEST_VOLUME);

        mCarAudioPolicyVolumeCallback.onVolumeAdjustment(ADJUST_RAISE);

        verify(mMockCarAudioService).setGroupVolume(PRIMARY_AUDIO_ZONE,
                TEST_VOLUME_GROUP, TEST_VOLUME + 1, TEST_EXPECTED_FLAGS);
    }

    @Test
    public void onVolumeAdjustment_withAdjustLower_decreasesGroupVolume() {
        setGroupVolume(TEST_VOLUME);

        mCarAudioPolicyVolumeCallback.onVolumeAdjustment(ADJUST_LOWER);

        verify(mMockCarAudioService).setGroupVolume(PRIMARY_AUDIO_ZONE,
                TEST_VOLUME_GROUP, TEST_VOLUME - 1, TEST_EXPECTED_FLAGS);
    }

    @Test
    public void onVolumeAdjustment_withAdjustLower_atMinVolume_setsGroupVolumeToMin() {
        setGroupVolume(TEST_MIN_VOLUME);

        mCarAudioPolicyVolumeCallback.onVolumeAdjustment(ADJUST_LOWER);

        verify(mMockCarAudioService).setGroupVolume(PRIMARY_AUDIO_ZONE,
                TEST_VOLUME_GROUP, TEST_MIN_VOLUME, TEST_EXPECTED_FLAGS);
    }

    @Test
    public void onVolumeAdjustment_withAdjustRaise_atMaxVolume_setsGroupVolumeToMax() {
        setGroupVolume(TEST_MAX_VOLUME);

        mCarAudioPolicyVolumeCallback.onVolumeAdjustment(ADJUST_RAISE);

        verify(mMockCarAudioService).setGroupVolume(PRIMARY_AUDIO_ZONE,
                TEST_VOLUME_GROUP, TEST_MAX_VOLUME, TEST_EXPECTED_FLAGS);
    }

    @Test
    public void onVolumeAdjustment_withAdjustSame_doesNothing() {
        setGroupVolume(TEST_VOLUME);

        mCarAudioPolicyVolumeCallback.onVolumeAdjustment(ADJUST_SAME);

        verify(mMockCarAudioService, never())
                .setGroupVolume(anyInt(), anyInt(), anyInt(), anyInt());
    }

    @Test
    public void onVolumeAdjustment_withAdjustMute_mutesMasterVolume() {
        mCarAudioPolicyVolumeCallback.onVolumeAdjustment(ADJUST_MUTE);

        verify(mMockCarAudioService).setMasterMute(true, TEST_EXPECTED_FLAGS);
    }

    @Test
    public void onVolumeAdjustment_withAdjustUnMute_unMutesMasterVolume() {
        mCarAudioPolicyVolumeCallback.onVolumeAdjustment(ADJUST_UNMUTE);

        verify(mMockCarAudioService).setMasterMute(false, TEST_EXPECTED_FLAGS);
    }

    @Test
    public void onVolumeAdjustment_withToggleMute_whileMuted_unMutesMasterVolume() {
        when(mMockAudioManager.isMasterMute()).thenReturn(true);

        mCarAudioPolicyVolumeCallback.onVolumeAdjustment(ADJUST_TOGGLE_MUTE);

        verify(mMockCarAudioService).setMasterMute(false, TEST_EXPECTED_FLAGS);
    }

    @Test
    public void onVolumeAdjustment_withToggleMute_whileUnMuted_mutesMasterVolume() {
        when(mMockAudioManager.isMasterMute()).thenReturn(false);

        mCarAudioPolicyVolumeCallback.onVolumeAdjustment(ADJUST_TOGGLE_MUTE);

        verify(mMockCarAudioService).setMasterMute(true, TEST_EXPECTED_FLAGS);
    }

    @Test
    public void onVolumeAdjustment_forGroupMute_withAdjustMute_mutesVolumeGroup() {
        CarAudioPolicyVolumeCallback callback =
                new CarAudioPolicyVolumeCallback(mMockCarAudioService, mMockAudioManager, true);

        callback.onVolumeAdjustment(ADJUST_MUTE);

        verify(mMockCarAudioService).setVolumeGroupMute(PRIMARY_AUDIO_ZONE,
                TEST_VOLUME_GROUP, true, TEST_EXPECTED_FLAGS);
    }

    @Test
    public void onVolumeAdjustment_forGroupMute_withAdjustToggleMute_togglesMutesVolumeGroup() {
        when(mMockCarAudioService.isVolumeGroupMuted(anyInt(), anyInt()))
                .thenReturn(true);
        CarAudioPolicyVolumeCallback callback =
                new CarAudioPolicyVolumeCallback(mMockCarAudioService, mMockAudioManager, true);

        callback.onVolumeAdjustment(ADJUST_TOGGLE_MUTE);

        verify(mMockCarAudioService).setVolumeGroupMute(PRIMARY_AUDIO_ZONE,
                TEST_VOLUME_GROUP, false, TEST_EXPECTED_FLAGS);
    }

    @Test
    public void onVolumeAdjustment_forGroupMute_withAdjustUnMute_UnMutesVolumeGroup() {
        when(mMockCarAudioService.isVolumeGroupMuted(anyInt(), anyInt())).thenReturn(false);
        CarAudioPolicyVolumeCallback callback =
                new CarAudioPolicyVolumeCallback(mMockCarAudioService, mMockAudioManager, true);

        callback.onVolumeAdjustment(ADJUST_UNMUTE);

        verify(mMockCarAudioService).setVolumeGroupMute(PRIMARY_AUDIO_ZONE,
                TEST_VOLUME_GROUP, false, TEST_EXPECTED_FLAGS);
    }

    private void setGroupVolume(int groupVolume) {
        when(mMockCarAudioService.getGroupVolume(anyInt(), anyInt()))
                .thenReturn(groupVolume);
    }
}
