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

package android.car.cts;

import static com.google.common.truth.Truth.assertWithMessage;

import android.car.Car;
import android.car.hardware.power.CarPowerManager;
import android.car.hardware.power.CarPowerPolicy;
import android.car.hardware.power.CarPowerPolicyFilter;
import android.car.hardware.power.PowerComponent;
import android.platform.test.annotations.AppModeFull;
import android.test.suitebuilder.annotation.SmallTest;

import androidx.annotation.Nullable;

import com.android.compatibility.common.util.SystemUtil;

import com.google.common.base.Strings;

import org.junit.After;
import org.junit.Before;
import org.junit.Test;

import java.util.concurrent.CountDownLatch;
import java.util.concurrent.Executor;
import java.util.concurrent.TimeUnit;

@SmallTest
@AppModeFull(reason = "Instant Apps cannot get car related permissions")
public final class CarPowerManagerTest extends CarApiTestBase {
    private static String TAG = CarPowerManagerTest.class.getSimpleName();
    private static final int LISTENER_WAIT_TIME_MS = 1000;
    private static final int NO_WAIT = 0;

    private CarPowerManager mCarPowerManager;
    private String mInitialPowerPolicyId;
    private final Executor mExecutor = mContext.getMainExecutor();

    @Override
    @Before
    public void setUp() throws Exception {
        super.setUp();
        mCarPowerManager = (CarPowerManager) getCar().getCarManager(Car.POWER_SERVICE);
        mInitialPowerPolicyId = mCarPowerManager.getCurrentPowerPolicy().getPolicyId();
    }

    @After
    public void teardown() throws Exception {
        CarPowerPolicy policy = mCarPowerManager.getCurrentPowerPolicy();
        if (!mInitialPowerPolicyId.equals(policy.getPolicyId())) {
            applyPowerPolicy(mInitialPowerPolicyId);
        }
    }

    /**
     * This test verifies 1) if the current power policy is set to applied one, 2) if proper power
     * policy change listeners are invoked, 3) unrelated power policy listeners are not invoked,
     * when a new power policy is applied.
     */
    @Test
    public void testApplyNewPowerPolicy() throws Exception {
        PowerPolicyListenerImpl listenerAudioOne = new PowerPolicyListenerImpl();
        PowerPolicyListenerImpl listenerAudioTwo = new PowerPolicyListenerImpl();
        PowerPolicyListenerImpl listenerWifi = new PowerPolicyListenerImpl();
        PowerPolicyListenerImpl listenerLocation = new PowerPolicyListenerImpl();
        CarPowerPolicyFilter filterAudio = new CarPowerPolicyFilter.Builder()
                .setComponents(PowerComponent.AUDIO).build();
        CarPowerPolicyFilter filterWifi = new CarPowerPolicyFilter.Builder()
                .setComponents(PowerComponent.WIFI).build();
        CarPowerPolicyFilter filterLocation = new CarPowerPolicyFilter.Builder()
                .setComponents(PowerComponent.LOCATION).build();
        String policyId = "audio_on_wifi_off";

        definePowerPolicy(policyId, "AUDIO", "WIFI");
        mCarPowerManager.addPowerPolicyListener(mExecutor, filterAudio, listenerAudioOne);
        mCarPowerManager.addPowerPolicyListener(mExecutor, filterAudio, listenerAudioTwo);
        mCarPowerManager.addPowerPolicyListener(mExecutor, filterWifi, listenerWifi);
        mCarPowerManager.addPowerPolicyListener(mExecutor, filterLocation, listenerLocation);
        mCarPowerManager.removePowerPolicyListener(listenerAudioTwo);
        applyPowerPolicy(policyId);

        CarPowerPolicy policy = mCarPowerManager.getCurrentPowerPolicy();
        assertWithMessage("Current power policy").that(policy).isNotNull();
        assertWithMessage("Current power policy ID").that(policy.getPolicyId()).isEqualTo(policyId);
        assertWithMessage("Added audio listener's current policy ID")
                .that(listenerAudioOne.getCurrentPolicyId(LISTENER_WAIT_TIME_MS))
                .isEqualTo(policyId);
        makeSureExecutorReady();
        assertWithMessage("Removed audio listener's current policy")
                .that(listenerAudioTwo.getCurrentPolicyId(NO_WAIT)).isNull();
        assertWithMessage("Added Wifi listener's current policy ID")
                .that(listenerWifi.getCurrentPolicyId(LISTENER_WAIT_TIME_MS)).isEqualTo(policyId);
        makeSureExecutorReady();
        assertWithMessage("Added location listener's current policy")
                .that(listenerLocation.getCurrentPolicyId(NO_WAIT)).isNull();

        applyPowerPolicy(mInitialPowerPolicyId);
    }

    private void makeSureExecutorReady() throws Exception {
        CountDownLatch latch = new CountDownLatch(1);
        mExecutor.execute(() -> {
            latch.countDown();
        });
        latch.await();
    }

    private static void definePowerPolicy(String policyId, String enabledComponents,
            String disabledComponents) throws Exception {
        String command = "cmd car_service define-power-policy " + policyId;
        if (!Strings.isNullOrEmpty(enabledComponents)) {
            command += " --enable " + enabledComponents;
        }
        if (!Strings.isNullOrEmpty(disabledComponents)) {
            command += " --disable " + disabledComponents;
        }
        executeShellCommandWithPermission(android.Manifest.permission.DEVICE_POWER, command);
    }

    private static void applyPowerPolicy(String policyId) throws Exception {
        executeShellCommandWithPermission(android.Manifest.permission.DEVICE_POWER,
                "cmd car_service apply-power-policy %s", policyId);
    }

    private final class PowerPolicyListenerImpl implements
            CarPowerManager.CarPowerPolicyListener {

        private final CountDownLatch mLatch = new CountDownLatch(1);
        private String mCurrentPolicyId;

        @Override
        public void onPolicyChanged(CarPowerPolicy policy) {
            mCurrentPolicyId = policy.getPolicyId();
            mLatch.countDown();
        }

        @Nullable
        public String getCurrentPolicyId(long waitTimeMs) throws Exception {
            if (mLatch.await(waitTimeMs, TimeUnit.MILLISECONDS)) {
                return mCurrentPolicyId;
            }
            return null;
        }
    }
}
