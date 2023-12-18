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

import static com.google.common.truth.Truth.assertThat;

import static org.testng.Assert.assertThrows;

import android.car.hardware.power.CarPowerPolicy;
import android.content.res.Resources;
import android.hardware.automotive.vehicle.V2_0.VehicleApPowerStateReport;

import androidx.test.platform.app.InstrumentationRegistry;

import com.android.car.carservice_unittest.R;

import org.junit.Before;
import org.junit.Test;

import java.io.InputStream;

public final class PolicyReaderUnitTest {
    private static final String TAG = PolicyReaderUnitTest.class.getSimpleName();

    private static final String POLICY_ID_NOT_EXIST = "policy_id_not_exist";
    private static final String POLICY_ID_OTHER_OFF = "policy_id_other_off";
    private static final String POLICY_ID_OTHER_ON = "policy_id_other_on";
    private static final String POLICY_ID_OTHER_UNTOUCHED = "policy_id_other_untouched";
    private static final String POLICY_ID_OTHER_NONE = "policy_id_other_none";
    private static final String POLICY_GROUP_ID_NOT_EXIST = "policy_group_id_not_exist";
    private static final String POLICY_GROUP_ID_BASIC = "basic_policy_group";
    private static final String POLICY_GROUP_ID_NO_DEFAULT_POLICY = "no_default_policy_group";
    private static final String POLICY_GROUP_ID_MIXED = "mixed_policy_group";
    private static final String NO_USER_INTERACTION_POLICY_ID =
            "system_power_policy_no_user_interaction";

    private static final CarPowerPolicy POLICY_OTHER_OFF = new CarPowerPolicy(POLICY_ID_OTHER_OFF,
            new int[]{WIFI},
            new int[]{AUDIO, MEDIA, DISPLAY, BLUETOOTH, CELLULAR, ETHERNET, PROJECTION, NFC, INPUT,
                    VOICE_INTERACTION, VISUAL_INTERACTION, TRUSTED_DEVICE_DETECTION, LOCATION,
                    MICROPHONE, CPU});
    private static final CarPowerPolicy POLICY_OTHER_ON = new CarPowerPolicy(POLICY_ID_OTHER_ON,
            new int[]{MEDIA, DISPLAY, BLUETOOTH, WIFI, CELLULAR, ETHERNET, PROJECTION, NFC, INPUT,
                    LOCATION, MICROPHONE, CPU},
            new int[]{AUDIO, VOICE_INTERACTION, VISUAL_INTERACTION, TRUSTED_DEVICE_DETECTION});
    private static final CarPowerPolicy POLICY_OTHER_UNTOUCHED =
            new CarPowerPolicy(POLICY_ID_OTHER_UNTOUCHED,
                    new int[]{AUDIO, DISPLAY, BLUETOOTH, WIFI, VOICE_INTERACTION,
                            VISUAL_INTERACTION, TRUSTED_DEVICE_DETECTION},
                    new int[]{});
    private static final CarPowerPolicy POLICY_OTHER_NONE = new CarPowerPolicy(POLICY_ID_OTHER_NONE,
            new int[]{WIFI},
            new int[]{AUDIO, VOICE_INTERACTION, VISUAL_INTERACTION, TRUSTED_DEVICE_DETECTION});
    private static final CarPowerPolicy SYSTEM_POWER_POLICY_NO_USER_INTERACTION =
            new CarPowerPolicy(NO_USER_INTERACTION_POLICY_ID,
                    new int[]{WIFI, CELLULAR, ETHERNET, TRUSTED_DEVICE_DETECTION, CPU},
                    new int[]{AUDIO, MEDIA, DISPLAY, BLUETOOTH, PROJECTION, NFC, INPUT,
                            VOICE_INTERACTION, VISUAL_INTERACTION, LOCATION, MICROPHONE});
    private static final CarPowerPolicy SYSTEM_POWER_POLICY_MODIFIED =
            new CarPowerPolicy(NO_USER_INTERACTION_POLICY_ID,
                    new int[]{BLUETOOTH, WIFI, CELLULAR, ETHERNET, NFC, CPU},
                    new int[]{AUDIO, MEDIA, DISPLAY, PROJECTION, INPUT, VOICE_INTERACTION,
                            VISUAL_INTERACTION, TRUSTED_DEVICE_DETECTION, LOCATION, MICROPHONE});

    private final Resources mResources =
            InstrumentationRegistry.getInstrumentation().getTargetContext().getResources();

    private final PolicyReader mPolicyReader = new PolicyReader();

    @Before
    public void setUp() throws Exception {
        mPolicyReader.initPolicies();
    }

    @Test
    public void testSystemPowerPolicyNoUserInteraction() throws Exception {
        assertSystemPowerPolicy(SYSTEM_POWER_POLICY_NO_USER_INTERACTION);
    }

    @Test
    public void testValidXml_powerPolicy() throws Exception {
        readPowerPolicyXml(R.raw.valid_power_policy);

        assertValidPolicyPart();
        assertValidPolicyPart();
        assertSystemPowerPolicy(SYSTEM_POWER_POLICY_MODIFIED);
    }

    @Test
    public void testValidXml_noPowerPolicyGroups() throws Exception {
        readPowerPolicyXml(R.raw.valid_power_policy_no_power_policy_groups);

        assertValidPolicyPart();
        assertNoPolicyGroupPart();
        assertSystemPowerPolicy(SYSTEM_POWER_POLICY_MODIFIED);
    }

    @Test
    public void testValidXml_noSystemPowerPolicy() throws Exception {
        readPowerPolicyXml(R.raw.valid_power_policy_no_system_power_policy);

        assertValidPolicyPart();
        assertValidPolicyGroupPart();
        assertSystemPowerPolicy(SYSTEM_POWER_POLICY_NO_USER_INTERACTION);
    }

    @Test
    public void testValidXml_policiesOnly() throws Exception {
        readPowerPolicyXml(R.raw.valid_power_policy_policies_only);

        assertValidPolicyPart();
        assertNoPolicyGroupPart();
        assertSystemPowerPolicy(SYSTEM_POWER_POLICY_NO_USER_INTERACTION);
    }

    @Test
    public void testValidXml_systemPowerPolicyOnly() throws Exception {
        readPowerPolicyXml(R.raw.valid_power_policy_system_power_policy_only);

        assertNoPolicyPart();
        assertNoPolicyGroupPart();
        assertSystemPowerPolicy(SYSTEM_POWER_POLICY_MODIFIED);
    }

    @Test
    public void testInvalidXml_incorrectGroupState() throws Exception {
        assertInvalidXml(R.raw.invalid_power_policy_group_incorrect_state);
    }

    @Test
    public void testInvalidXml_missingGroupPolicy() throws Exception {
        assertInvalidXml(R.raw.invalid_power_policy_group_missing_policy);
    }

    @Test
    public void testInvalidXml_incorrectPolicyId() throws Exception {
        assertInvalidXml(R.raw.invalid_power_policy_incorrect_id);
    }

    @Test
    public void testInvalidXml_incorrectOtherComponent() throws Exception {
        assertInvalidXml(R.raw.invalid_power_policy_incorrect_othercomponent);
    }

    @Test
    public void testInvalidXml_incorrectValue() throws Exception {
        assertInvalidXml(R.raw.invalid_power_policy_incorrect_value);
    }

    @Test
    public void testInvalidXml_unknownComponent() throws Exception {
        assertInvalidXml(R.raw.invalid_power_policy_unknown_component);
    }

    @Test
    public void testInvalidXml_incorrectSystemPolicyComponent() throws Exception {
        assertInvalidXml(R.raw.invalid_system_power_policy_incorrect_component);
    }

    @Test
    public void testInvalidXml_incorrectSystemPolicyId() throws Exception {
        assertInvalidXml(R.raw.invalid_system_power_policy_incorrect_id);
    }

    private void assertValidPolicyPart() throws Exception {
        assertThat(mPolicyReader.getPowerPolicy(POLICY_ID_NOT_EXIST)).isNull();
        checkPolicy(POLICY_ID_OTHER_OFF, POLICY_OTHER_OFF);
        checkPolicy(POLICY_ID_OTHER_UNTOUCHED, POLICY_OTHER_UNTOUCHED);
        checkPolicy(POLICY_ID_OTHER_ON, POLICY_OTHER_ON);
        checkPolicy(POLICY_ID_OTHER_NONE, POLICY_OTHER_NONE);
    }

    private void assertNoPolicyPart() throws Exception {
        assertThat(mPolicyReader.getPowerPolicy(POLICY_ID_NOT_EXIST)).isNull();
        assertThat(mPolicyReader.getPowerPolicy(POLICY_ID_OTHER_OFF)).isNull();
        assertThat(mPolicyReader.getPowerPolicy(POLICY_ID_OTHER_UNTOUCHED)).isNull();
        assertThat(mPolicyReader.getPowerPolicy(POLICY_ID_OTHER_ON)).isNull();
        assertThat(mPolicyReader.getPowerPolicy(POLICY_ID_OTHER_NONE)).isNull();
    }

    private void assertValidPolicyGroupPart() throws Exception {
        assertThat(mPolicyReader.getDefaultPowerPolicyForState(POLICY_GROUP_ID_NOT_EXIST,
                VehicleApPowerStateReport.WAIT_FOR_VHAL)).isNull();
        checkPolicyGroup(POLICY_GROUP_ID_MIXED, VehicleApPowerStateReport.WAIT_FOR_VHAL,
                POLICY_OTHER_ON);
        checkPolicyGroup(POLICY_GROUP_ID_MIXED, VehicleApPowerStateReport.ON,
                POLICY_OTHER_UNTOUCHED);
        assertThat(mPolicyReader.getDefaultPowerPolicyForState(POLICY_GROUP_ID_MIXED,
                VehicleApPowerStateReport.DEEP_SLEEP_ENTRY)).isNull();
        assertThat(mPolicyReader.getDefaultPowerPolicyForState(POLICY_GROUP_ID_MIXED,
                VehicleApPowerStateReport.SHUTDOWN_START)).isNull();
        assertThat(mPolicyReader.getDefaultPowerPolicyForState(POLICY_GROUP_ID_NO_DEFAULT_POLICY,
                VehicleApPowerStateReport.WAIT_FOR_VHAL)).isNull();
        assertThat(mPolicyReader.getDefaultPowerPolicyForState(POLICY_GROUP_ID_NO_DEFAULT_POLICY,
                VehicleApPowerStateReport.ON)).isNull();
        assertThat(mPolicyReader.getDefaultPowerPolicyForState(POLICY_GROUP_ID_NO_DEFAULT_POLICY,
                VehicleApPowerStateReport.DEEP_SLEEP_ENTRY)).isNull();
        assertThat(mPolicyReader.getDefaultPowerPolicyForState(POLICY_GROUP_ID_NO_DEFAULT_POLICY,
                VehicleApPowerStateReport.SHUTDOWN_START)).isNull();
    }

    private void assertNoPolicyGroupPart() throws Exception {
        assertThat(mPolicyReader.getDefaultPowerPolicyForState(POLICY_GROUP_ID_NOT_EXIST,
                VehicleApPowerStateReport.WAIT_FOR_VHAL)).isNull();
        assertThat(mPolicyReader.getDefaultPowerPolicyForState(POLICY_GROUP_ID_BASIC,
                VehicleApPowerStateReport.WAIT_FOR_VHAL)).isNull();
        assertThat(mPolicyReader.getDefaultPowerPolicyForState(POLICY_GROUP_ID_BASIC,
                VehicleApPowerStateReport.ON)).isNull();
    }

    private void assertSystemPowerPolicy(CarPowerPolicy expectedSystemPolicy) throws Exception {
        CarPowerPolicy systemPolicy = mPolicyReader.getPreemptivePowerPolicy(
                NO_USER_INTERACTION_POLICY_ID);
        assertThat(systemPolicy).isNotNull();
        assertPolicyIdentical(systemPolicy, expectedSystemPolicy);
    }

    private void assertInvalidXml(int id) throws Exception {
        assertThrows(PolicyReader.PolicyXmlException.class, () -> readPowerPolicyXml(id));
    }

    private void checkPolicy(String policyId, CarPowerPolicy expectedPolicy) throws Exception {
        CarPowerPolicy actualPolicy = mPolicyReader.getPowerPolicy(policyId);
        assertThat(actualPolicy).isNotNull();
        assertPolicyIdentical(actualPolicy, expectedPolicy);
    }

    private void checkPolicyGroup(String groupId, int state, CarPowerPolicy expectedPolicy)
            throws Exception {
        CarPowerPolicy actualPolicy = mPolicyReader.getDefaultPowerPolicyForState(groupId, state);
        assertThat(actualPolicy).isNotNull();
        assertPolicyIdentical(expectedPolicy, actualPolicy);
    }

    private void readPowerPolicyXml(int id) throws Exception {
        try (InputStream inputStream = mResources.openRawResource(id)) {
            mPolicyReader.readPowerPolicyFromXml(inputStream);
        }
    }
}
