/*
 * Copyright (C) 2020 The Android Open Source Project
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

package com.android.car.hardware.power;

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

import static com.google.common.truth.Truth.assertThat;

import static org.mockito.Mockito.doReturn;
import static org.mockito.Mockito.when;
import static org.testng.Assert.assertThrows;

import android.annotation.NonNull;
import android.car.Car;
import android.car.hardware.power.CarPowerManager;
import android.car.hardware.power.CarPowerPolicy;
import android.car.hardware.power.CarPowerPolicyFilter;
import android.car.hardware.power.PowerComponent;
import android.car.test.mocks.AbstractExtendedMockitoTestCase;
import android.car.test.mocks.JavaMockitoHelper;
import android.content.Context;
import android.content.pm.PackageManager;
import android.content.res.Resources;
import android.frameworks.automotive.powerpolicy.internal.ICarPowerPolicySystemNotification;
import android.hardware.automotive.vehicle.V2_0.VehicleApPowerStateReq;
import android.hardware.automotive.vehicle.V2_0.VehicleApPowerStateShutdownParam;
import android.test.suitebuilder.annotation.SmallTest;
import android.util.AtomicFile;
import android.util.Log;

import androidx.test.platform.app.InstrumentationRegistry;

import com.android.car.R;
import com.android.car.hal.MockedPowerHalService;
import com.android.car.hal.PowerHalService;
import com.android.car.hal.PowerHalService.PowerState;
import com.android.car.power.CarPowerManagementService;
import com.android.car.power.PowerComponentHandler;
import com.android.car.systeminterface.DisplayInterface;
import com.android.car.systeminterface.SystemInterface;
import com.android.car.systeminterface.SystemStateInterface;
import com.android.car.test.utils.TemporaryFile;
import com.android.car.user.CarUserService;
import com.android.internal.annotations.GuardedBy;
import com.android.internal.app.IVoiceInteractionManagerService;

import org.junit.After;
import org.junit.Before;
import org.junit.Test;
import org.mockito.Mock;
import org.mockito.Spy;

import java.time.Duration;
import java.util.concurrent.CountDownLatch;
import java.util.concurrent.Executor;
import java.util.concurrent.Semaphore;
import java.util.concurrent.TimeUnit;

@SmallTest
public class CarPowerManagerUnitTest extends AbstractExtendedMockitoTestCase {
    private static final String TAG = CarPowerManagerUnitTest.class.getSimpleName();
    private static final long WAIT_TIMEOUT_MS = 5_000;
    private static final long WAIT_TIMEOUT_LONG_MS = 10_000;
    // A shorter value for use when the test is expected to time out
    private static final long WAIT_WHEN_TIMEOUT_EXPECTED_MS = 100;

    private final MockDisplayInterface mDisplayInterface = new MockDisplayInterface();
    private final MockSystemStateInterface mSystemStateInterface = new MockSystemStateInterface();

    @Spy
    private final Context mContext =
            InstrumentationRegistry.getInstrumentation().getTargetContext();
    private final Executor mExecutor = mContext.getMainExecutor();
    private final TemporaryFile mComponentStateFile;

    private MockedPowerHalService mPowerHal;
    private SystemInterface mSystemInterface;
    private CarPowerManagementService mService;
    private CarPowerManager mCarPowerManager;
    private PowerComponentHandler mPowerComponentHandler;

    @Mock
    private Resources mResources;
    @Mock
    private Car mCar;
    @Mock
    private CarUserService mCarUserService;
    @Mock
    private IVoiceInteractionManagerService mVoiceInteractionManagerService;
    @Mock
    private ICarPowerPolicySystemNotification mPowerPolicyDaemon;

    public CarPowerManagerUnitTest() throws Exception {
        mComponentStateFile = new TemporaryFile("COMPONENT_STATE_FILE");
    }

    @Before
    public void setUp() throws Exception {
        mPowerHal = new MockedPowerHalService(true /*isPowerStateSupported*/,
                true /*isDeepSleepAllowed*/, true /*isTimedWakeupAllowed*/);
        mSystemInterface = SystemInterface.Builder.defaultSystemInterface(mContext)
            .withDisplayInterface(mDisplayInterface)
            .withSystemStateInterface(mSystemStateInterface)
            .build();
        setService();
        mCarPowerManager = new CarPowerManager(mCar, mService);
    }

    @After
    public void tearDown() throws Exception {
        if (mService != null) {
            mService.release();
        }
    }

    @Test
    public void testRequestShutdownOnNextSuspend_positive() throws Exception {
        setPowerOn();
        // Tell it to shutdown
        mCarPowerManager.requestShutdownOnNextSuspend();
        // Request suspend
        setPowerState(VehicleApPowerStateReq.SHUTDOWN_PREPARE,
                VehicleApPowerStateShutdownParam.CAN_SLEEP);
        // Verify shutdown
        assertStateReceivedForShutdownOrSleepWithPostpone(PowerHalService.SET_SHUTDOWN_START, 0);
    }

    @Test
    public void testRequestShutdownOnNextSuspend_negative() throws Exception {
        setPowerOn();

        // Do not tell it to shutdown

        // Request suspend
        setPowerState(VehicleApPowerStateReq.SHUTDOWN_PREPARE,
                VehicleApPowerStateShutdownParam.CAN_SLEEP);
        // Verify suspend
        assertStateReceivedForShutdownOrSleepWithPostpone(PowerHalService.SET_DEEP_SLEEP_ENTRY, 0);
    }

    @Test
    public void testScheduleNextWakeupTime() throws Exception {
        setPowerOn();

        int wakeTime = 1234;
        mCarPowerManager.scheduleNextWakeupTime(wakeTime);

        setPowerState(VehicleApPowerStateReq.SHUTDOWN_PREPARE,
                VehicleApPowerStateShutdownParam.CAN_SLEEP);

        // Verify that we suspended with the requested wake-up time
        assertStateReceivedForShutdownOrSleepWithPostpone(
                PowerHalService.SET_DEEP_SLEEP_ENTRY, wakeTime);
    }

    @Test
    public void testSetListener() throws Exception {
        setPowerOn();

        WaitablePowerStateListener listener = new WaitablePowerStateListener(2);

        setPowerState(VehicleApPowerStateReq.SHUTDOWN_PREPARE,
                VehicleApPowerStateShutdownParam.CAN_SLEEP);

        assertStateReceivedForShutdownOrSleepWithPostpone(PowerHalService.SET_DEEP_SLEEP_ENTRY, 0);

        int finalState = listener.await();
        assertThat(finalState).isEqualTo(PowerHalService.SET_DEEP_SLEEP_ENTRY);
    }

    @Test
    public void testSetListenerWithCompletion() throws Exception {
        setPowerOn();

        WaitablePowerStateListenerWithCompletion listener =
                new WaitablePowerStateListenerWithCompletion(2);

        setPowerState(VehicleApPowerStateReq.SHUTDOWN_PREPARE,
                VehicleApPowerStateShutdownParam.CAN_SLEEP);
        assertStateReceivedForShutdownOrSleepWithPostpone(PowerHalService.SET_DEEP_SLEEP_ENTRY, 0);

        int finalState = listener.await();
        assertThat(finalState).isEqualTo(PowerHalService.SET_DEEP_SLEEP_ENTRY);
    }

    @Test
    public void testClearListener() throws Exception {
        setPowerOn();

        // Set a listener with a short timeout, because we expect the timeout to happen
        WaitablePowerStateListener listener =
                new WaitablePowerStateListener(1, WAIT_WHEN_TIMEOUT_EXPECTED_MS);

        mCarPowerManager.clearListener();

        setPowerState(VehicleApPowerStateReq.SHUTDOWN_PREPARE,
                VehicleApPowerStateShutdownParam.CAN_SLEEP);

        assertStateReceivedForShutdownOrSleepWithPostpone(PowerHalService.SET_DEEP_SLEEP_ENTRY, 0);
        // Verify that the listener didn't run
        assertThrows(IllegalStateException.class, () -> listener.await());
    }

    @Test
    public void testGetPowerState() throws Exception {
        setPowerOn();
        assertThat(mCarPowerManager.getPowerState()).isEqualTo(PowerHalService.SET_ON);

        // Request suspend
        setPowerState(VehicleApPowerStateReq.SHUTDOWN_PREPARE,
                        VehicleApPowerStateShutdownParam.CAN_SLEEP);
        assertStateReceivedForShutdownOrSleepWithPostpone(PowerHalService.SET_DEEP_SLEEP_ENTRY, 0);
        assertThat(mCarPowerManager.getPowerState())
                .isEqualTo(PowerHalService.SET_DEEP_SLEEP_ENTRY);
    }

    @Test
    public void testGetCurrentPowerPolicy() throws Exception {
        grantPowerPolicyPermission();
        CarPowerPolicy expected = new CarPowerPolicy("test_policy4",
                new int[]{AUDIO, MEDIA, DISPLAY, INPUT, CPU},
                new int[]{BLUETOOTH, CELLULAR, ETHERNET, LOCATION, MICROPHONE, NFC, PROJECTION,
                        TRUSTED_DEVICE_DETECTION, VISUAL_INTERACTION, VOICE_INTERACTION, WIFI});
        PolicyDefinition[] policyDefinitions = new PolicyDefinition[]{
                new PolicyDefinition("test_policy1", new String[]{"WIFI"}, new String[]{"AUDIO"}),
                new PolicyDefinition("test_policy2", new String[]{"WIFI", "DISPLAY"},
                        new String[]{"NFC"}),
                new PolicyDefinition("test_policy3", new String[]{"CPU", "INPUT"},
                        new String[]{"WIFI"}),
                new PolicyDefinition("test_policy4", new String[]{"MEDIA", "AUDIO"},
                        new String[]{})};
        for (PolicyDefinition definition : policyDefinitions) {
            mService.definePowerPolicy(definition.policyId, definition.enabledComponents,
                    definition.disabledComponents);
        }

        for (PolicyDefinition definition : policyDefinitions) {
            mCarPowerManager.applyPowerPolicy(definition.policyId);
        }

        assertPolicyIdentical(expected, mCarPowerManager.getCurrentPowerPolicy());
    }

    @Test
    public void testApplyPowerPolicy() throws Exception {
        grantPowerPolicyPermission();
        String policyId = "no_change_policy";
        mService.definePowerPolicy(policyId, new String[0], new String[0]);

        mCarPowerManager.applyPowerPolicy(policyId);

        assertThat(mCarPowerManager.getCurrentPowerPolicy().getPolicyId()).isEqualTo(policyId);
    }

    @Test
    public void testApplyPowerPolicy_invalidId() throws Exception {
        grantPowerPolicyPermission();
        String policyId = "invalid_power_policy";

        assertThrows(IllegalArgumentException.class,
                () -> mCarPowerManager.applyPowerPolicy(policyId));
    }

    @Test
    public void testApplyPowerPolicy_nullPolicyId() throws Exception {
        grantPowerPolicyPermission();
        assertThrows(IllegalArgumentException.class, () -> mCarPowerManager.applyPowerPolicy(null));
    }

    @Test
    public void testAddPowerPolicyListener() throws Exception {
        grantPowerPolicyPermission();
        String policyId = "audio_on_wifi_off";
        mService.definePowerPolicy(policyId, new String[]{"AUDIO"}, new String[]{"WIFI"});
        MockedPowerPolicyListener listenerAudio = new MockedPowerPolicyListener();
        MockedPowerPolicyListener listenerWifi = new MockedPowerPolicyListener();
        MockedPowerPolicyListener listenerLocation = new MockedPowerPolicyListener();
        CarPowerPolicyFilter filterAudio = new CarPowerPolicyFilter.Builder()
                .setComponents(PowerComponent.AUDIO).build();
        CarPowerPolicyFilter filterWifi = new CarPowerPolicyFilter.Builder()
                .setComponents(PowerComponent.WIFI).build();
        CarPowerPolicyFilter filterLocation = new CarPowerPolicyFilter.Builder()
                .setComponents(PowerComponent.LOCATION).build();

        mCarPowerManager.addPowerPolicyListener(mExecutor, filterAudio, listenerAudio);
        mCarPowerManager.addPowerPolicyListener(mExecutor, filterWifi, listenerWifi);
        mCarPowerManager.addPowerPolicyListener(mExecutor, filterLocation, listenerLocation);
        mCarPowerManager.applyPowerPolicy(policyId);

        assertThat(listenerAudio.getCurrentPolicyId()).isEqualTo(policyId);
        assertThat(listenerWifi.getCurrentPolicyId()).isEqualTo(policyId);
        assertThat(listenerLocation.getCurrentPolicyId()).isNull();
    }

    @Test
    public void testAddPowerPolicyListener_Twice_WithDifferentFilters() throws Exception {
        grantPowerPolicyPermission();
        String policyId = "audio_on_wifi_off";
        mService.definePowerPolicy(policyId, new String[]{"AUDIO"}, new String[]{"WIFI"});
        MockedPowerPolicyListener listener = new MockedPowerPolicyListener();
        CarPowerPolicyFilter filterAudio = new CarPowerPolicyFilter.Builder()
                .setComponents(PowerComponent.AUDIO).build();
        CarPowerPolicyFilter filterLocation = new CarPowerPolicyFilter.Builder()
                .setComponents(PowerComponent.LOCATION).build();

        mCarPowerManager.addPowerPolicyListener(mExecutor, filterAudio, listener);
        mCarPowerManager.addPowerPolicyListener(mExecutor, filterLocation, listener);
        mCarPowerManager.applyPowerPolicy(policyId);

        assertThat(listener.getCurrentPolicyId()).isNull();
    }

    @Test
    public void testAddPowerPolicyListener_nullListener() throws Exception {
        MockedPowerPolicyListener listener = new MockedPowerPolicyListener();
        CarPowerPolicyFilter filter = new CarPowerPolicyFilter.Builder()
                .setComponents(PowerComponent.AUDIO).build();

        assertThrows(NullPointerException.class,
                () -> mCarPowerManager.addPowerPolicyListener(null, filter, listener));
        assertThrows(NullPointerException.class,
                () -> mCarPowerManager.addPowerPolicyListener(mExecutor, filter, null));
        assertThrows(NullPointerException.class,
                () -> mCarPowerManager.addPowerPolicyListener(mExecutor, null, listener));
    }

    @Test
    public void testRemovePowerPolicyListener() throws Exception {
        grantPowerPolicyPermission();
        String policyId = "audio_on_wifi_off";
        mService.definePowerPolicy(policyId, new String[]{"AUDIO"}, new String[]{"WIFI"});
        MockedPowerPolicyListener listenerOne = new MockedPowerPolicyListener();
        MockedPowerPolicyListener listenerTwo = new MockedPowerPolicyListener();
        CarPowerPolicyFilter filterAudio = new CarPowerPolicyFilter.Builder()
                .setComponents(PowerComponent.AUDIO).build();

        mCarPowerManager.addPowerPolicyListener(mExecutor, filterAudio, listenerOne);
        mCarPowerManager.addPowerPolicyListener(mExecutor, filterAudio, listenerTwo);
        mCarPowerManager.removePowerPolicyListener(listenerOne);
        mCarPowerManager.applyPowerPolicy(policyId);

        assertThat(listenerOne.getCurrentPolicyId()).isNull();
        assertThat(listenerTwo.getCurrentPolicyId()).isEqualTo(policyId);
    }

    @Test
    public void testRemovePowerPolicyListener_Twice() throws Exception {
        grantPowerPolicyPermission();
        MockedPowerPolicyListener listener = new MockedPowerPolicyListener();
        CarPowerPolicyFilter filter = new CarPowerPolicyFilter.Builder()
                .setComponents(PowerComponent.AUDIO).build();

        // Remove unregistered listener should not throw an exception.
        mCarPowerManager.removePowerPolicyListener(listener);

        mCarPowerManager.addPowerPolicyListener(mExecutor, filter, listener);
        mCarPowerManager.removePowerPolicyListener(listener);
        // Remove the same listener twice should nont throw an exception.
        mCarPowerManager.removePowerPolicyListener(listener);
    }

    @Test
    public void testRemovePowerPolicyListener_nullListener() throws Exception {
        assertThrows(NullPointerException.class,
                () -> mCarPowerManager.removePowerPolicyListener(null));
    }

    /**
     * Helper method to create mService and initialize a test case
     */
    private void setService() throws Exception {
        Log.i(TAG, "setService(): overridden overlay properties: "
                + ", maxGarageModeRunningDurationInSecs="
                + mResources.getInteger(R.integer.maxGarageModeRunningDurationInSecs));
        mPowerComponentHandler = new PowerComponentHandler(mContext, mSystemInterface,
                mVoiceInteractionManagerService, new AtomicFile(mComponentStateFile.getFile()));
        mService = new CarPowerManagementService(mContext, mResources, mPowerHal, mSystemInterface,
                null, mCarUserService, mPowerPolicyDaemon, mPowerComponentHandler,
                /* silentModeHwStatePath= */ null, /* silentModeKernelStatePath= */ null,
                /* bootReason= */ null);
        mService.init();
        mService.setShutdownTimersForTest(0, 0);
        assertStateReceived(MockedPowerHalService.SET_WAIT_FOR_VHAL, 0);
    }

    private void assertStateReceived(int expectedState, int expectedParam) throws Exception {
        int[] state = mPowerHal.waitForSend(WAIT_TIMEOUT_MS);
        assertThat(state).asList().containsExactly(expectedState, expectedParam).inOrder();
    }

    /**
     * Helper method to get the system into ON
     */
    private void setPowerOn() throws Exception {
        setPowerState(VehicleApPowerStateReq.ON, 0);
        int[] state = mPowerHal.waitForSend(WAIT_TIMEOUT_MS);
        assertThat(state[0]).isEqualTo(PowerHalService.SET_ON);
    }

    /**
     * Helper to set the PowerHal state
     *
     * @param stateEnum Requested state enum
     * @param stateParam Addition state parameter
     */
    private void setPowerState(int stateEnum, int stateParam) {
        mPowerHal.setCurrentPowerState(new PowerState(stateEnum, stateParam));
    }

    private void assertStateReceivedForShutdownOrSleepWithPostpone(
            int lastState, int stateParameter) throws Exception {
        long startTime = System.currentTimeMillis();
        while (true) {
            int[] state = mPowerHal.waitForSend(WAIT_TIMEOUT_LONG_MS);
            if (state[0] == lastState) {
                assertThat(state[1]).isEqualTo(stateParameter);
                return;
            }
            assertThat(state[0]).isEqualTo(PowerHalService.SET_SHUTDOWN_POSTPONE);
            assertThat(System.currentTimeMillis() - startTime).isLessThan(WAIT_TIMEOUT_LONG_MS);
        }
    }

    private void grantPowerPolicyPermission() {
        when(mCar.getContext()).thenReturn(mContext);
        doReturn(PackageManager.PERMISSION_GRANTED).when(mContext)
                .checkCallingOrSelfPermission(Car.PERMISSION_CONTROL_CAR_POWER_POLICY);
        doReturn(PackageManager.PERMISSION_GRANTED).when(mContext)
                .checkCallingOrSelfPermission(Car.PERMISSION_READ_CAR_POWER_POLICY);
    }

    private static final class MockDisplayInterface implements DisplayInterface {
        private final Object mLock = new Object();
        @GuardedBy("mLock")
        private boolean mDisplayOn = true;
        private final Semaphore mDisplayStateWait = new Semaphore(0);

        @Override
        public void setDisplayBrightness(int brightness) {}

        @Override
        public void setDisplayState(boolean on) {
            synchronized (mLock) {
                mDisplayOn = on;
            }
            mDisplayStateWait.release();
        }

        public boolean waitForDisplayStateChange(long timeoutMs) throws Exception {
            JavaMockitoHelper.await(mDisplayStateWait, timeoutMs);
            synchronized (mLock) {
                return mDisplayOn;
            }
        }

        @Override
        public void startDisplayStateMonitoring(CarPowerManagementService service) {}

        @Override
        public void stopDisplayStateMonitoring() {}

        @Override
        public void refreshDisplayBrightness() {}

        @Override
        public boolean isDisplayEnabled() {
            synchronized (mLock) {
                return mDisplayOn;
            }
        }
    }

    /**
     * Helper class to set a power-state listener,
     * verify that the listener gets called the
     * right number of times, and return the final
     * power state.
     */
    private final class WaitablePowerStateListener {
        private final CountDownLatch mLatch;
        private int mListenedState = -1;
        private long mTimeoutValue = WAIT_TIMEOUT_MS;

        WaitablePowerStateListener(int initialCount, long customTimeout) {
            this(initialCount);
            mTimeoutValue = customTimeout;
        }

        WaitablePowerStateListener(int initialCount) {
            mLatch = new CountDownLatch(initialCount);
            mCarPowerManager.setListener(
                    (state) -> {
                        mListenedState = state;
                        mLatch.countDown();
                    });
        }

        int await() throws Exception {
            JavaMockitoHelper.await(mLatch, WAIT_TIMEOUT_MS);
            return mListenedState;
        }
    }

    /**
     * Helper class to set a power-state listener with completion,
     * verify that the listener gets called the right number of times,
     * verify that the CompletableFuture is provided, complete the
     * CompletableFuture, and return the final power state.
     */
    private final class WaitablePowerStateListenerWithCompletion {
        private final CountDownLatch mLatch;
        private int mListenedState = -1;
        WaitablePowerStateListenerWithCompletion(int initialCount) {
            mLatch = new CountDownLatch(initialCount);
            mCarPowerManager.setListenerWithCompletion(
                    (state, future) -> {
                        mListenedState = state;
                        if (state == PowerHalService.SET_SHUTDOWN_PREPARE) {
                            assertThat(future).isNotNull();
                            future.complete(null);
                        } else {
                            assertThat(future).isNull();
                        }
                        mLatch.countDown();
                    });
        }

        int await() throws Exception {
            JavaMockitoHelper.await(mLatch, WAIT_TIMEOUT_MS);
            return mListenedState;
        }
    }

    private static final class MockSystemStateInterface implements SystemStateInterface {
        private final Semaphore mShutdownWait = new Semaphore(0);
        private final Semaphore mSleepWait = new Semaphore(0);
        private final Semaphore mSleepExitWait = new Semaphore(0);
        private boolean mWakeupCausedByTimer = false;

        @Override
        public void shutdown() {
            mShutdownWait.release();
        }

        public void waitForShutdown(long timeoutMs) throws Exception {
            JavaMockitoHelper.await(mShutdownWait, timeoutMs);
        }

        @Override
        public boolean enterDeepSleep() {
            mSleepWait.release();
            try {
                mSleepExitWait.acquire();
            } catch (InterruptedException e) {
            }
            return true;
        }

        public void waitForSleepEntryAndWakeup(long timeoutMs) throws Exception {
            JavaMockitoHelper.await(mSleepWait, timeoutMs);
            mSleepExitWait.release();
        }

        @Override
        public void scheduleActionForBootCompleted(Runnable action, Duration delay) {}

        @Override
        public boolean isWakeupCausedByTimer() {
            Log.i(TAG, "isWakeupCausedByTimer:" + mWakeupCausedByTimer);
            return mWakeupCausedByTimer;
        }

        public synchronized void setWakeupCausedByTimer(boolean set) {
            mWakeupCausedByTimer = set;
        }

        @Override
        public boolean isSystemSupportingDeepSleep() {
            return true;
        }
    }

    private final class MockedPowerPolicyListener implements
            CarPowerManager.CarPowerPolicyListener {
        private static final int MAX_LISTENER_WAIT_TIME_SEC = 1;

        private final CountDownLatch mLatch = new CountDownLatch(1);
        private String mCurrentPolicyId;

        @Override
        public void onPolicyChanged(@NonNull CarPowerPolicy policy) {
            mCurrentPolicyId = policy.getPolicyId();
            mLatch.countDown();
        }

        public String getCurrentPolicyId() throws Exception {
            if (mLatch.await(MAX_LISTENER_WAIT_TIME_SEC, TimeUnit.SECONDS)) {
                return mCurrentPolicyId;
            }
            return null;
        }
    }

    private static final class PolicyDefinition {
        public final String policyId;
        public final String[] enabledComponents;
        public final String[] disabledComponents;

        private PolicyDefinition(String policyId, String[] enabledComponents,
                String[] disabledComponents) {
            this.policyId = policyId;
            this.enabledComponents = enabledComponents;
            this.disabledComponents = disabledComponents;
        }
    }
}
