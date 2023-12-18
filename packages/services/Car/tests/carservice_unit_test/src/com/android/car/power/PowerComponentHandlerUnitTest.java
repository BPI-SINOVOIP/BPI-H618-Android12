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

package com.android.car.power;

import static android.car.hardware.power.PowerComponent.AUDIO;
import static android.car.hardware.power.PowerComponent.BLUETOOTH;
import static android.car.hardware.power.PowerComponent.CELLULAR;
import static android.car.hardware.power.PowerComponent.CPU;
import static android.car.hardware.power.PowerComponent.DISPLAY;
import static android.car.hardware.power.PowerComponent.ETHERNET;
import static android.car.hardware.power.PowerComponent.INPUT;
import static android.car.hardware.power.PowerComponent.LOCATION;
import static android.car.hardware.power.PowerComponent.MEDIA;
import static android.car.hardware.power.PowerComponent.MICROPHONE;
import static android.car.hardware.power.PowerComponent.NFC;
import static android.car.hardware.power.PowerComponent.PROJECTION;
import static android.car.hardware.power.PowerComponent.TRUSTED_DEVICE_DETECTION;
import static android.car.hardware.power.PowerComponent.VISUAL_INTERACTION;
import static android.car.hardware.power.PowerComponent.VOICE_INTERACTION;
import static android.car.hardware.power.PowerComponent.WIFI;

import static com.android.car.test.power.CarPowerPolicyUtil.assertPolicyIdentical;

import android.car.hardware.power.CarPowerPolicy;
import android.content.Context;
import android.util.AtomicFile;

import androidx.test.platform.app.InstrumentationRegistry;

import com.android.car.systeminterface.SystemInterface;
import com.android.car.test.utils.TemporaryFile;
import com.android.internal.app.IVoiceInteractionManagerService;

import org.junit.Before;
import org.junit.Test;
import org.junit.runner.RunWith;
import org.mockito.Mock;
import org.mockito.junit.MockitoJUnitRunner;

/**
 * Tests for {@link PowerComponentHandler}.
 */
@RunWith(MockitoJUnitRunner.class)
public final class PowerComponentHandlerUnitTest {
    private final Context mContext = InstrumentationRegistry.getInstrumentation().getContext();

    @Mock
    private SystemInterface mSystemInterface;
    @Mock
    private IVoiceInteractionManagerService mVoiceInteractionManagerService;
    private PowerComponentHandler mHandler;
    private TemporaryFile mComponentStateFile;

    @Before
    public void setUp() throws Exception {
        mComponentStateFile = new TemporaryFile("COMPONENT_STATE_FILE");
        mHandler = new PowerComponentHandler(mContext, mSystemInterface,
                mVoiceInteractionManagerService, new AtomicFile(mComponentStateFile.getFile()));
        mHandler.init();
    }

    @Test
    public void testGetAccumulatedPolicy_firstTime() throws Exception {
        CarPowerPolicy policy = mHandler.getAccumulatedPolicy();
        CarPowerPolicy expected = new CarPowerPolicy("", new int[]{},
                new int[]{AUDIO, BLUETOOTH, CELLULAR, CPU, DISPLAY, ETHERNET, INPUT, LOCATION,
                        MEDIA, MICROPHONE, NFC, PROJECTION, TRUSTED_DEVICE_DETECTION,
                        VISUAL_INTERACTION, VOICE_INTERACTION, WIFI});

        assertPolicyIdentical(expected, policy);
    }

    @Test
    public void testApplyPowerPolicy_oneTime() throws Exception {
        CarPowerPolicy policy = new CarPowerPolicy("test_policy1", new int[]{WIFI},
                new int[]{AUDIO});
        CarPowerPolicy expected = new CarPowerPolicy("test_policy1", new int[]{WIFI},
                new int[]{AUDIO, BLUETOOTH, CELLULAR, CPU, DISPLAY, ETHERNET, INPUT, LOCATION,
                        MEDIA, MICROPHONE, NFC, PROJECTION, TRUSTED_DEVICE_DETECTION,
                        VISUAL_INTERACTION, VOICE_INTERACTION});

        mHandler.applyPowerPolicy(policy);

        assertPolicyIdentical(expected, mHandler.getAccumulatedPolicy());
    }

    @Test
    public void testApplyPowerPolicy_multipleTimes() throws Exception {
        CarPowerPolicy[] policies = new CarPowerPolicy[]{
                new CarPowerPolicy("test_policy1", new int[]{WIFI}, new int[]{AUDIO}),
                new CarPowerPolicy("test_policy2", new int[]{WIFI, DISPLAY}, new int[]{NFC}),
                new CarPowerPolicy("test_policy3", new int[]{CPU, INPUT}, new int[]{WIFI}),
                new CarPowerPolicy("test_policy4", new int[]{MEDIA, AUDIO}, new int[]{})};
        CarPowerPolicy expected = new CarPowerPolicy("test_policy4",
                new int[]{AUDIO, MEDIA, DISPLAY, INPUT, CPU},
                new int[]{BLUETOOTH, CELLULAR, ETHERNET, LOCATION, MICROPHONE, NFC, PROJECTION,
                        TRUSTED_DEVICE_DETECTION, VISUAL_INTERACTION, VOICE_INTERACTION, WIFI});

        for (CarPowerPolicy policy : policies) {
            mHandler.applyPowerPolicy(policy);
        }

        assertPolicyIdentical(expected, mHandler.getAccumulatedPolicy());
    }
}
