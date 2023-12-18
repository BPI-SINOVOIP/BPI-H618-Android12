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

import static org.mockito.ArgumentMatchers.any;
import static org.mockito.Mockito.never;
import static org.mockito.Mockito.verify;
import static org.mockito.Mockito.when;
import static org.testng.Assert.assertThrows;

import android.car.hardware.power.CarPowerPolicy;
import android.car.hardware.power.CarPowerPolicyFilter;
import android.car.hardware.power.ICarPowerPolicyListener;
import android.car.hardware.power.PowerComponent;

import com.android.car.power.CarPowerManagementService;

import org.junit.Test;
import org.junit.runner.RunWith;
import org.mockito.ArgumentCaptor;
import org.mockito.Mock;
import org.mockito.junit.MockitoJUnitRunner;

@RunWith(MockitoJUnitRunner.class)
public class CarAudioPowerListenerTest {
    private static final String POLICY_ID = "POLICY_ID";
    private static final int[] EMPTY_COMPONENTS = new int[0];
    private static final int[] COMPONENTS_WITH_AUDIO = {PowerComponent.AUDIO};
    private static final CarPowerPolicy ENABLED_POLICY = new CarPowerPolicy(POLICY_ID,
            COMPONENTS_WITH_AUDIO, EMPTY_COMPONENTS);
    private static final CarPowerPolicy DISABLED_POLICY = new CarPowerPolicy(POLICY_ID,
            EMPTY_COMPONENTS, COMPONENTS_WITH_AUDIO);
    private static final CarPowerPolicy EMPTY_POLICY = new CarPowerPolicy(POLICY_ID,
            EMPTY_COMPONENTS, EMPTY_COMPONENTS);

    @Mock
    CarAudioService mMockCarAudioService;

    @Mock
    CarPowerManagementService mMockCarPowerService;

    @Test
    public void constructor_nullCarAudioService_throws() {
        assertThrows(NullPointerException.class,
                () -> new CarAudioPowerListener(null, mMockCarPowerService));
    }

    @Test
    public void startListeningForPolicyChanges_withoutPowerService_enablesAudio() {
        CarAudioPowerListener listener = new CarAudioPowerListener(mMockCarAudioService, null);

        listener.startListeningForPolicyChanges();

        verify(mMockCarAudioService).setAudioEnabled(true);
    }

    @Test
    public void startListeningForPolicyChanges_addsPowerPolicyListener() {
        CarAudioPowerListener listener = new CarAudioPowerListener(mMockCarAudioService,
                mMockCarPowerService);

        listener.startListeningForPolicyChanges();

        ArgumentCaptor<CarPowerPolicyFilter> captor = ArgumentCaptor.forClass(
                CarPowerPolicyFilter.class);
        verify(mMockCarPowerService).addPowerPolicyListener(
                captor.capture(), any(ICarPowerPolicyListener.class));
        assertThat(captor.getValue().getComponents()).asList()
                .containsExactly(PowerComponent.AUDIO);

    }

    @Test
    public void startListeningForPolicyChanges_withNullPolicy_enablesAudio() {
        when(mMockCarPowerService.getCurrentPowerPolicy()).thenReturn(null);
        CarAudioPowerListener listener = new CarAudioPowerListener(mMockCarAudioService,
                mMockCarPowerService);

        listener.startListeningForPolicyChanges();

        verify(mMockCarAudioService).setAudioEnabled(true);
    }

    @Test
    public void startListeningForPolicyChanges_withPowerEnabled_enablesAudio() {
        withAudioInitiallyEnabled();
        CarAudioPowerListener listener = new CarAudioPowerListener(mMockCarAudioService,
                mMockCarPowerService);

        listener.startListeningForPolicyChanges();

        verify(mMockCarAudioService).setAudioEnabled(true);
    }

    @Test
    public void startListeningForPolicyChanges_withPowerDisabled_disablesAudio() {
        withAudioInitiallyDisabled();
        CarAudioPowerListener listener = new CarAudioPowerListener(mMockCarAudioService,
                mMockCarPowerService);

        listener.startListeningForPolicyChanges();

        verify(mMockCarAudioService).setAudioEnabled(false);
    }

    @Test
    public void onPolicyChange_withPowerSwitchingToEnabled_enablesAudio() throws Exception {
        withAudioInitiallyDisabled();
        ICarPowerPolicyListener changeListener = registerAndGetChangeListener();
        verify(mMockCarAudioService, never()).setAudioEnabled(true);

        changeListener.onPolicyChanged(EMPTY_POLICY, ENABLED_POLICY);

        verify(mMockCarAudioService).setAudioEnabled(true);
    }

    @Test
    public void onPolicyChange_withPowerRemainingEnabled_doesNothing() throws Exception {
        withAudioInitiallyEnabled();
        ICarPowerPolicyListener changeListener = registerAndGetChangeListener();
        verify(mMockCarAudioService).setAudioEnabled(true);

        changeListener.onPolicyChanged(EMPTY_POLICY, ENABLED_POLICY);

        verify(mMockCarAudioService).setAudioEnabled(true);
        verify(mMockCarAudioService, never()).setAudioEnabled(false);
    }

    @Test
    public void onPolicyChange_withPowerSwitchingToDisabled_disablesAudio() throws Exception {
        withAudioInitiallyEnabled();
        ICarPowerPolicyListener changeListener = registerAndGetChangeListener();
        verify(mMockCarAudioService, never()).setAudioEnabled(false);

        changeListener.onPolicyChanged(EMPTY_POLICY, DISABLED_POLICY);

        verify(mMockCarAudioService).setAudioEnabled(false);
    }

    @Test
    public void onPolicyChange_withPowerStayingDisabled_doesNothing() throws Exception {
        withAudioInitiallyDisabled();
        ICarPowerPolicyListener changeListener = registerAndGetChangeListener();
        verify(mMockCarAudioService).setAudioEnabled(false);

        changeListener.onPolicyChanged(EMPTY_POLICY, DISABLED_POLICY);

        verify(mMockCarAudioService).setAudioEnabled(false);
        verify(mMockCarAudioService, never()).setAudioEnabled(true);
    }

    private void withAudioInitiallyEnabled() {
        when(mMockCarPowerService.getCurrentPowerPolicy()).thenReturn(ENABLED_POLICY);
    }

    private void withAudioInitiallyDisabled() {
        when(mMockCarPowerService.getCurrentPowerPolicy()).thenReturn(DISABLED_POLICY);
    }

    private ICarPowerPolicyListener registerAndGetChangeListener() {
        CarAudioPowerListener listener = new CarAudioPowerListener(mMockCarAudioService,
                mMockCarPowerService);
        listener.startListeningForPolicyChanges();
        ArgumentCaptor<ICarPowerPolicyListener> captor = ArgumentCaptor.forClass(
                ICarPowerPolicyListener.class);
        verify(mMockCarPowerService).addPowerPolicyListener(
                any(), captor.capture());

        return captor.getValue();
    }
}
