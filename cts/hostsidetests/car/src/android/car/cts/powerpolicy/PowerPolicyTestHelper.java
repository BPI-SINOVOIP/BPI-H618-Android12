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

package android.car.cts.powerpolicy;

import static com.google.common.truth.Truth.assertThat;
import static com.google.common.truth.Truth.assertWithMessage;

import com.android.tradefed.log.LogUtil.CLog;

public final class PowerPolicyTestHelper {
    private final CpmsFrameworkLayerStateInfo mFrameCpms;
    private final CpmsSystemLayerStateInfo mSystemCpms;
    private final SilentModeInfo mSilentMode;
    private final String mStep;
    private final String mTestcase;

    public static final String CURRENT_STATE_ASSERT_MSG = "current state";
    public static final String CURRENT_POLICY_ASSERT_MSG = "current policy";
    public static final String CURRENT_POWER_COMPONENT_ASSERT_MSG = "current power components";
    public static final String REGISTERED_POLICY_ASSERT_MSG = "registered policy";
    public static final String SILENT_MODE_FULL_ASSERT_MSG = "silent mode in full";
    public static final String SILENT_MODE_STATUS_ASSERT_MSG = "silent mode status";
    public static final String PENDING_POLICY_ASSERT_MSG = "pending policy id";
    public static final String TOTAL_REGISTERED_POLICIES_ASSERT_MSG =
            "the total number of registered policies";

    public PowerPolicyTestHelper(String testcase, String step,
            CpmsFrameworkLayerStateInfo frameCpms, CpmsSystemLayerStateInfo sysCpms,
            SilentModeInfo silentMode) {
        mStep = step;
        mTestcase = testcase;
        mFrameCpms = frameCpms;
        mSystemCpms = sysCpms;
        mSilentMode = silentMode;
    }

    public void checkCurrentState(int expected) {
        assertWithMessage(CURRENT_STATE_ASSERT_MSG)
                .that(expected == mFrameCpms.getCurrentState()).isTrue();
    }

    public void checkCurrentPolicy(String expectedPolicyId) {
        boolean expected = expectedPolicyId.equals(mFrameCpms.getCurrentPolicyId());
        if (!expected) {
            CLog.d("expectedPolicyId: " + expectedPolicyId);
            CLog.d("currentPolicyId: " + mFrameCpms.getCurrentPolicyId());
        }
        assertWithMessage(CURRENT_POLICY_ASSERT_MSG).that(expected).isTrue();
    }

    public void checkSilentModeStatus(boolean expected) {
        assertWithMessage(SILENT_MODE_STATUS_ASSERT_MSG)
                .that(mFrameCpms.getForcedSilentMode() == expected).isTrue();
    }

    public void checkSilentModeFull(SilentModeInfo expected) {
        boolean status = expected.equals(mSilentMode);
        if (!status) {
            CLog.e("PowerPolicyTestHelper expected silent mode: %s", expected.toString());
            CLog.e("PowerPolicyTestHelper got tested silent mode: %s", mSilentMode.toString());
        }
        assertWithMessage(SILENT_MODE_FULL_ASSERT_MSG).that(status).isTrue();
    }

    public void checkRegisteredPolicy(PowerPolicyDef expectedPolicy) {
        boolean status = false;
        for (PowerPolicyDef def : mSystemCpms.getRegisteredPolicies()) {
            if (def.getPolicyId().equals(expectedPolicy.getPolicyId())) {
                status = expectedPolicy.equals(def);
                if (!status) {
                    CLog.e("PowerPolicyTestHelper expected policy: %s", expectedPolicy.toString());
                    CLog.e("PowerPolicyTestHelper got result policy: %s", def.toString());
                }
                break;
            }
        }
        assertWithMessage(REGISTERED_POLICY_ASSERT_MSG).that(status).isTrue();
    }

    public void checkRegisteredPolicy(String policyId) {
        boolean status = false;
        for (PowerPolicyDef def : mSystemCpms.getRegisteredPolicies()) {
            if (def.getPolicyId().equals(policyId)) {
                status = true;
                break;
            }
        }
        assertWithMessage(REGISTERED_POLICY_ASSERT_MSG).that(status).isTrue();
    }

    public void checkPendingPolicyId(String id) {
        boolean status = false;
        if (id == null) {
            if (mSystemCpms.getPendingPolicyId() != null) {
                CLog.w("PowerPolicyTestHelper expected non null pending policy");
            } else {
                status = true;
            }
        } else {
            status = id.equals(mSystemCpms.getPendingPolicyId());
        }
        assertWithMessage(PENDING_POLICY_ASSERT_MSG).that(status).isTrue();
    }

    public void checkTotalRegisteredPolicies(int totalNum) {
        assertWithMessage(TOTAL_REGISTERED_POLICIES_ASSERT_MSG)
                .that(mSystemCpms.getRegisteredPolicies().size() == totalNum).isTrue();
    }

    public void checkCurrentPowerComponents(PowerPolicyDef expected) throws Exception {
        assertThat(mFrameCpms.getCurrentEnabledComponents()).asList()
                .containsExactlyElementsIn(expected.getEnables());
        assertThat(mFrameCpms.getCurrentDisabledComponents()).asList()
                .containsExactlyElementsIn(expected.getDisables());
    }

    public void checkCurrentPolicyGroupId(String expected) {
        if (expected == null) {
            expected = "null";
        }
        assertWithMessage("checkCurrentPolicyGroupId")
                .that(expected.equals(mFrameCpms.getCurrentPolicyGroupId())).isTrue();
    }

    public void checkPowerPolicyGroups(PowerPolicyGroups expected) {
        assertWithMessage("checkPowerPolicyGroups")
                .that(expected.equals(mFrameCpms.getPowerPolicyGroups())).isTrue();
    }
}
