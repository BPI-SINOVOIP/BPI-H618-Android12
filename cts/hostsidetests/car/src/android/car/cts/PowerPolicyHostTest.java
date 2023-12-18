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

import android.car.cts.powerpolicy.CpmsFrameworkLayerStateInfo;
import android.car.cts.powerpolicy.CpmsSystemLayerStateInfo;
import android.car.cts.powerpolicy.LocationInfo;
import android.car.cts.powerpolicy.PowerPolicyConstants;
import android.car.cts.powerpolicy.PowerPolicyDef;
import android.car.cts.powerpolicy.PowerPolicyGroups;
import android.car.cts.powerpolicy.PowerPolicyTestAnalyzer;
import android.car.cts.powerpolicy.PowerPolicyTestHelper;
import android.car.cts.powerpolicy.PowerPolicyTestResult;
import android.car.cts.powerpolicy.SilentModeInfo;
import android.car.cts.powerpolicy.SystemInfoParser;
import android.car.cts.powerpolicy.WifiInfo;

import com.android.compatibility.common.util.CommonTestUtils;
import com.android.tradefed.log.LogUtil.CLog;
import com.android.tradefed.testtype.DeviceJUnit4ClassRunner;

import org.junit.After;
import org.junit.Assume;
import org.junit.Before;
import org.junit.Test;
import org.junit.runner.RunWith;

import java.lang.reflect.Method;

@RunWith(DeviceJUnit4ClassRunner.class)
public final class PowerPolicyHostTest extends CarHostJUnit4TestCase {
    private static final String ANDROID_CLIENT_PKG = "android.car.cts.app";
    private static final String ANDROID_CLIENT_ACTIVITY = ANDROID_CLIENT_PKG
            + "/.PowerPolicyTestActivity";
    private static final String TEST_COMMAND_HEADER =
            "am start -n " + ANDROID_CLIENT_ACTIVITY + " --es powerpolicy ";

    private static final int DEFAULT_TIMEOUT_SEC = 20;

    private final PowerPolicyTestAnalyzer mTestAnalyzer = new PowerPolicyTestAnalyzer(this);

    @Before
    public void checkPrecondition() throws Exception {
        checkDefaultPowerPolicySet("pre-condition");
    }

    @After
    public void restoreInitCondition() throws Exception {
        applyPowerPolicy(PowerPolicyDef.IdSet.DEFAULT_ALL_ON);
    }

    @Test
    public void testPowerPolicySilentMode() throws Exception {
        String testcase = "testPowerPolicySilentModeFull:";
        String teststep;
        PowerPolicyTestHelper testHelper;

        SilentModeInfo smInfo = getSilentModeInfo();
        Assume.assumeTrue("HW does not support silent mode. Skip the test",
                smInfo.getMonitoringHWStateSignal());

        teststep = "switch to forced silent";
        enterForcedSilentMode();
        testHelper = getTestHelper(testcase, 1, teststep);
        testHelper.checkCurrentState(PowerPolicyConstants.CarPowerState.ON);
        testHelper.checkCurrentPolicy(PowerPolicyDef.IdSet.NO_USER_INTERACTION);
        testHelper.checkSilentModeStatus(true);
        testHelper.checkSilentModeFull(SilentModeInfo.FORCED_SILENT);
        testHelper.checkCurrentPowerComponents(PowerPolicyDef.PolicySet.NO_USER_INTERACT);

        teststep = "restore to normal mode";
        leaveForcedSilentMode();
        testHelper = getTestHelper(testcase, 2, teststep);
        testHelper.checkCurrentState(PowerPolicyConstants.CarPowerState.ON);
        testHelper.checkCurrentPolicy(PowerPolicyDef.IdSet.DEFAULT_ALL_ON);
        testHelper.checkSilentModeStatus(false);
        testHelper.checkSilentModeFull(SilentModeInfo.NO_SILENT);
        testHelper.checkCurrentPowerComponents(PowerPolicyDef.PolicySet.DEFAULT_ALL_ON);
    }

    /**
     * Tests the error conditions for CPMS at the ON state.
     *
     * <p>All other VHAL events but {@code SHUTDOWN_PREPARE} shall not have any impact
     * to CPMS power state.
     */
    @Test
    public void testDefaultStateMachineAtONState() throws Exception {
        String testcase = "testDefaultStateMachineAtONState:";
        String[] stepNames = {
            "trigger VHAL ON event",
            "trigger VHAL CANCEL_SHUTDOWN",
            "trigger VHAL FINISHED"
        };
        int[] vhalReqs = {
            PowerPolicyConstants.VhalPowerStateReq.ON,
            PowerPolicyConstants.VhalPowerStateReq.CANCEL_SHUTDOWN,
            PowerPolicyConstants.VhalPowerStateReq.FINISHED
        };

        for (int i = 0; i < stepNames.length; i++) {
            triggerVhalPowerStateReq(vhalReqs[i], PowerPolicyConstants.ShutdownParam.NOT_USED);
            PowerPolicyTestHelper testHelper = getTestHelper(testcase, i + 1, stepNames[i]);
            testHelper.checkCurrentState(PowerPolicyConstants.CarPowerState.ON);
            testHelper.checkCurrentPolicy(PowerPolicyDef.IdSet.DEFAULT_ALL_ON);
        }
    }

    @Test
    public void testPowerPolicyChange() throws Exception {
        String testcase = "testPowerPolicyChange:";
        int expectedTotalPolicies = PowerPolicyDef.PolicySet.TOTAL_DEFAULT_REGISTERED_POLICIES;
        int stepNo = 0;
        String teststep;
        PowerPolicyTestHelper testHelper;

        teststep = "check the inital power policies";
        testHelper = getTestHelper(testcase, stepNo++, teststep);
        testHelper.checkCurrentState(PowerPolicyConstants.CarPowerState.ON);
        testHelper.checkCurrentPolicy(PowerPolicyDef.IdSet.DEFAULT_ALL_ON);
        testHelper.checkTotalRegisteredPolicies(expectedTotalPolicies);

        // create two power policies, test1 and test2, for power policy change test
        defineAndCheckPolicyTest1(testcase, stepNo++, ++expectedTotalPolicies);
        defineAndCheckPolicyTest2(testcase, stepNo++, ++expectedTotalPolicies);

        teststep = "apply power policy test1";
        applyPowerPolicy(PowerPolicyDef.IdSet.TEST1);
        testHelper = getTestHelper(testcase, stepNo++, teststep);
        testHelper.checkCurrentPolicy(PowerPolicyDef.IdSet.TEST1);

        teststep = "apply power policy test2";
        applyPowerPolicy(PowerPolicyDef.IdSet.TEST2);
        testHelper = getTestHelper(testcase, stepNo++, teststep);
        testHelper.checkCurrentPolicy(PowerPolicyDef.IdSet.TEST2);

        teststep = "revert power policy back to the default";
        applyPowerPolicy(PowerPolicyDef.IdSet.DEFAULT_ALL_ON);
        testHelper = getTestHelper(testcase, stepNo++, teststep);
        testHelper.checkCurrentPolicy(PowerPolicyDef.IdSet.DEFAULT_ALL_ON);

        // add "test power policy listener" here so that one reboot clears all
        defineAndCheckPolicyListenerTest(testcase, stepNo++, ++expectedTotalPolicies);
        String clientTestcase = "PowerPolicyListenerTest";
        PowerPolicyTestResult testResult = new PowerPolicyTestResult(mTestAnalyzer);
        String clientAction = "DUMP_LISTENER";
        String component = "AUDIO";

        setClientTestcase(clientTestcase);
        int currentNumberListeners = getNumberPolicyListeners();
        registerPowerPolicyListener(component);
        resetPowerPolicyListeners();
        waitUntilNumberPolicyListenersEquals(++currentNumberListeners);
        applyPowerPolicy(PowerPolicyDef.IdSet.LISTENER_TEST);
        waitPowerPolicyListenersUpdated();

        dumpPowerPolicyListener(component);
        testResult.checkLastTestResultEntry(clientTestcase, clientAction,
                component, PowerPolicyDef.PolicySet.LISTENER_TEST);

        unregisterPowerPolicyListener(component);
        applyPowerPolicy(PowerPolicyDef.IdSet.DEFAULT_ALL_ON);
        waitPowerPolicyListenersUpdated();

        dumpPowerPolicyListener(component);
        testResult.checkLastTestResultEntry(clientTestcase, clientAction,
                component, "not_registered");
        clearClientTestcase();

        // add respect to user setting test case here to utilize a single device reboot
        testPowerPolicyAndComponentUserSetting();

        // add power policy group test here to utilize added test1 and test2 policies
        teststep = "check default power policy group";
        PowerPolicyGroups emptyGroups = new PowerPolicyGroups();
        testHelper = getTestHelper(testcase, stepNo++, teststep);
        testHelper.checkCurrentPolicyGroupId(null);
        testHelper.checkPowerPolicyGroups(emptyGroups);

        teststep = "define power policy group";
        definePowerPolicyGroup(PowerPolicyGroups.TestSet.POLICY_GROUP_DEF1.toShellCommandString());
        definePowerPolicyGroup(PowerPolicyGroups.TestSet.POLICY_GROUP_DEF2.toShellCommandString());
        testHelper = getTestHelper(testcase, stepNo++, teststep);
        testHelper.checkPowerPolicyGroups(PowerPolicyGroups.TestSet.POLICY_GROUPS1);

        teststep = "set power policy group";
        setPowerPolicyGroup(PowerPolicyGroups.TestSet.GROUP_ID1);
        testHelper = getTestHelper(testcase, stepNo++, teststep);
        testHelper.checkCurrentPolicyGroupId(PowerPolicyGroups.TestSet.GROUP_ID1);

        // reboot device to clear created TEST1 and TEST2 test cases.
        // need to find a way to move reboot device into AfterAll
        rebootDevice();
        teststep = "reboot to clear added test power policies";
        testHelper = getTestHelper(testcase, stepNo++, teststep);
        expectedTotalPolicies = PowerPolicyDef.PolicySet.TOTAL_DEFAULT_REGISTERED_POLICIES;
        testHelper.checkCurrentState(PowerPolicyConstants.CarPowerState.ON);
        testHelper.checkCurrentPolicy(PowerPolicyDef.IdSet.DEFAULT_ALL_ON);
        testHelper.checkTotalRegisteredPolicies(expectedTotalPolicies);
    }

    public String fetchActivityDumpsys() throws Exception {
        return executeCommand("dumpsys activity %s", ANDROID_CLIENT_ACTIVITY);
    }

    private PowerPolicyTestHelper getTestHelper(String testcase, int stepNo, String stepName)
            throws Exception {
        CpmsSystemLayerStateInfo cpmsSystemInfo = getCpmsSystemLayerStateInfo();
        CpmsFrameworkLayerStateInfo cpmsFrameworkInfo = getCpmsFrameworkLayerStateInfo();
        String normalizedStepName = String.format("%d. %s", stepNo, stepName);
        return new PowerPolicyTestHelper(testcase, normalizedStepName,
                cpmsFrameworkInfo, cpmsSystemInfo, getSilentModeInfo());
    }

    private void triggerVhalPowerStateReq(int reqNo, int param) throws Exception {
        executeCommand("cmd car_service inject-vhal-event %d %d,%d",
                PowerPolicyConstants.VHAL_POWER_STATE_REQ_PROPERTY_ID, reqNo, param);
    }

    private SilentModeInfo getSilentModeInfo() throws Exception {
        return executeAndParseCommand(
                new SystemInfoParser<SilentModeInfo>(SilentModeInfo.class),
                SilentModeInfo.COMMAND);
    }

    private CpmsFrameworkLayerStateInfo getCpmsFrameworkLayerStateInfo() throws Exception {
        return executeAndParseCommand(new SystemInfoParser<CpmsFrameworkLayerStateInfo>(
                CpmsFrameworkLayerStateInfo.class), CpmsFrameworkLayerStateInfo.COMMAND);
    }

    private CpmsSystemLayerStateInfo getCpmsSystemLayerStateInfo() throws Exception {
        return executeAndParseCommand(new SystemInfoParser<CpmsSystemLayerStateInfo>(
                CpmsSystemLayerStateInfo.class), CpmsSystemLayerStateInfo.COMMAND);
    }

    private void rebootDevice() throws Exception {
        executeCommand("svc power reboot");
        waitForDeviceAvailable();
    }

    private void enterForcedSilentMode() throws Exception {
        executeCommand("cmd car_service silent-mode forced-silent");
        waitUntilForcedSilentModeChangeTo(true);
    }

    private void leaveForcedSilentMode() throws Exception {
        executeCommand("cmd car_service silent-mode forced-non-silent");
        executeCommand("cmd car_service silent-mode non-forced-silent-mode");
        waitUntilForcedSilentModeChangeTo(false);
    }

    private void definePowerPolicy(String policyStr) throws Exception {
        CLog.d("definePowerPolicy: %s", policyStr);
        executeCommand("cmd car_service define-power-policy %s", policyStr);
    }

    private void applyPowerPolicy(String policyId) throws Exception {
        executeCommand("cmd car_service apply-power-policy %s", policyId);
    }

    private void definePowerPolicyGroup(String policyGroupStr) throws Exception {
        executeCommand("cmd car_service define-power-policy-group %s", policyGroupStr);
    }

    private void setPowerPolicyGroup(String policyGroupId) throws Exception {
        executeCommand("cmd car_service set-power-policy-group %s", policyGroupId);
    }

    private void setClientTestcase(String testcase) throws Exception {
        executeCommand("%s settest,%s", TEST_COMMAND_HEADER, testcase);
    }

    private void clearClientTestcase() throws Exception {
        executeCommand("%s cleartest", TEST_COMMAND_HEADER);
    }

    private void registerPowerPolicyListener(String componentName) throws Exception {
        executeCommand("%s addlistener,%s", TEST_COMMAND_HEADER, componentName);
    }

    private void unregisterPowerPolicyListener(String componentName) throws Exception {
        executeCommand("%s removelistener,%s", TEST_COMMAND_HEADER, componentName);
    }

    private void dumpPowerPolicyListener(String componentName) throws Exception {
        executeCommand("%s dumplistener,%s", TEST_COMMAND_HEADER, componentName);
    }

    private void waitPowerPolicyListenersUpdated() throws Exception {
        executeCommand("%s waitlisteners", TEST_COMMAND_HEADER);
    }

    private void resetPowerPolicyListeners() throws Exception {
        executeCommand("%s resetlisteners", TEST_COMMAND_HEADER);
    }

    private int getNumberPolicyListeners() throws Exception {
        return getCpmsFrameworkLayerStateInfo().getNumberPolicyListeners();
    }

    private void waitUntilNumberPolicyListenersEquals(int numListeners) throws Exception {
        CommonTestUtils.waitUntil("timed out (" + DEFAULT_TIMEOUT_SEC
                + "s) getting number policy listeners", DEFAULT_TIMEOUT_SEC,
                () -> (getNumberPolicyListeners() == numListeners));
    }

    private void waitUntilForcedSilentModeChangeTo(boolean expected) throws Exception {
        String timeoutMsg = String.format("timed out (%ds) waiting for forced silent mode "
                + "to be %b", DEFAULT_TIMEOUT_SEC, expected);
        CommonTestUtils.waitUntil(timeoutMsg, DEFAULT_TIMEOUT_SEC,
                () -> {
                    SilentModeInfo silentInfo = getSilentModeInfo();
                    CpmsFrameworkLayerStateInfo cpmsInfo = getCpmsFrameworkLayerStateInfo();
                    return (silentInfo.getForcedSilentMode() == expected)
                            && (cpmsInfo.getForcedSilentMode() == expected);
                });
    }

    private void waitForDeviceAvailable() throws Exception {
        try {
            getDevice().waitForDeviceAvailable();
        } catch (Exception e) {
            CLog.w("device is not available, trying one more time");
            getDevice().waitForDeviceAvailable();
        }
    }

    private void checkDefaultPowerPolicySet(String testcase) throws Exception {
        String teststep = "check if the car power is on the ON state";
        PowerPolicyTestHelper testHelper = new PowerPolicyTestHelper(testcase, teststep,
                getCpmsFrameworkLayerStateInfo(), getCpmsSystemLayerStateInfo(), null);
        testHelper.checkCurrentState(PowerPolicyConstants.CarPowerState.ON);
        testHelper.checkRegisteredPolicy(PowerPolicyDef.PolicySet.INITIAL_ALL_ON);
        testHelper.checkRegisteredPolicy(PowerPolicyDef.PolicySet.DEFAULT_ALL_ON);
        testHelper.checkCurrentPolicy(PowerPolicyDef.IdSet.DEFAULT_ALL_ON);
    }

    private void defineAndCheckPolicyTest1(String testcase, int stepNo,
            int expectedTotalPolicies) throws Exception {
        String teststep = stepNo + ". define a new power policy with id test1";
        definePowerPolicy(PowerPolicyDef.PolicySet.TEST1.toString());
        PowerPolicyTestHelper testHelper = getTestHelper(testcase, stepNo, teststep);
        testHelper.checkRegisteredPolicy(PowerPolicyDef.PolicySet.TEST1);
        testHelper.checkTotalRegisteredPolicies(expectedTotalPolicies);
    }

    private void defineAndCheckPolicyTest2(String testcase, int stepNo,
            int expectedTotalPolicies) throws Exception {
        String teststep = stepNo + ". define a new power policy with id test2";
        definePowerPolicy(PowerPolicyDef.PolicySet.TEST2.toString());
        PowerPolicyTestHelper testHelper = getTestHelper(testcase, stepNo, teststep);
        testHelper.checkRegisteredPolicy(PowerPolicyDef.PolicySet.TEST2);
        testHelper.checkTotalRegisteredPolicies(expectedTotalPolicies);
    }

    private void defineAndCheckPolicyListenerTest(String testcase, int stepNo,
            int expectedTotalPolicies) throws Exception {
        String teststep = stepNo + ". define a new power policy with id listener_test";
        definePowerPolicy(PowerPolicyDef.PolicySet.LISTENER_TEST.toString());
        PowerPolicyTestHelper testHelper = getTestHelper(testcase, stepNo, teststep);
        testHelper.checkRegisteredPolicy(PowerPolicyDef.PolicySet.LISTENER_TEST);
        testHelper.checkTotalRegisteredPolicies(expectedTotalPolicies);
    }

    private void testPowerPolicyAndComponentUserSetting() throws Exception {
        ComponentTestHelper[] testHelpers = {
            new ComponentTestHelper<WifiInfo>(this, "WIFI", WifiInfo.class),
            new ComponentTestHelper<LocationInfo>(this, "LOCATION", LocationInfo.class),
        };

        for (int i = 0; i < testHelpers.length; i++) {
            testComponent(testHelpers[i]);
        }
    }

    private static final class ComponentTestHelper<T> {
        private final PowerPolicyHostTest mHostTest;
        private final String mComponentName;
        private final String mInfoRetrieveCommand;
        private final String[] mEnableCommands;
        private final String[] mDisableCommands;
        private final Class mTypeClass;

        ComponentTestHelper(PowerPolicyHostTest hostTest, String componentName, Class typeClass)
                throws Exception {
            mHostTest = hostTest;
            mComponentName = componentName;
            mTypeClass = typeClass;
            mInfoRetrieveCommand = (String) mTypeClass.getField("COMMAND").get(null);
            mEnableCommands = (String[]) mTypeClass.getField("ENABLE_COMMANDS").get(null);
            mDisableCommands = (String[]) mTypeClass.getField("DISABLE_COMMANDS").get(null);
        }

        private void turnOnComponent() throws Exception {
            for (int i = 0; i < mEnableCommands.length; i++) {
                mHostTest.executeCommand(mEnableCommands[i]);
            }
        }

        private void turnOffComponent() throws Exception {
            for (int i = 0; i < mDisableCommands.length; i++) {
                mHostTest.executeCommand(mDisableCommands[i]);
            }
        }

        private T tryGetComponentInfo() throws Exception {
            return mHostTest.executeAndParseCommand(new SystemInfoParser<T>(mTypeClass),
                    mInfoRetrieveCommand);
        }

        private void waitUntilPowerStateChangeTo(boolean expected) throws Exception {
            CommonTestUtils.waitUntil("timed out with " + mComponentName
                    + "enabled expected: " + expected, DEFAULT_TIMEOUT_SEC,
                    () -> {
                        T info = tryGetComponentInfo();
                        if (info != null) {
                            Method isPowerOn = mTypeClass.getMethod("isPowerOn");
                            Boolean result = (Boolean) isPowerOn.invoke(info);
                            return result.booleanValue() == expected;
                        }
                        return false;
                    });
        }
    }

    private void testComponent(ComponentTestHelper testHelper)
            throws Exception {
        PowerPolicyDef powerOffPolicy =
                PowerPolicyDef.createWithComponentOff(testHelper.mComponentName);
        definePowerPolicy(powerOffPolicy.toString());

        testHelper.turnOffComponent();
        testHelper.waitUntilPowerStateChangeTo(false);

        testHelper.turnOnComponent();
        testHelper.waitUntilPowerStateChangeTo(true);

        applyPowerPolicy(powerOffPolicy.getPolicyId());
        testHelper.turnOnComponent();
        testHelper.waitUntilPowerStateChangeTo(true);

        testHelper.turnOffComponent();
        testHelper.waitUntilPowerStateChangeTo(false);
    }
}
