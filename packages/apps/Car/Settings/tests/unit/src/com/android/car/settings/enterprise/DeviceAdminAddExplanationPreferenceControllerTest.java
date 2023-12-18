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
package com.android.car.settings.enterprise;

import com.android.car.settings.common.PreferenceController;

import org.junit.Before;
import org.junit.Test;

public final class DeviceAdminAddExplanationPreferenceControllerTest extends
        BaseDeviceAdminAddPreferenceControllerTestCase
                <DeviceAdminAddExplanationPreferenceController> {

    private DeviceAdminAddExplanationPreferenceController mController;

    @Before
    public void setController() {
        mController = new DeviceAdminAddExplanationPreferenceController(mSpiedContext,
                mPreferenceKey, mFragmentController, mUxRestrictions);
        mController.setDeviceAdmin(mDefaultDeviceAdminInfo);
    }

    @Test
    public void testGetAvailabilityStatus_noAdmin() throws Exception {
        DeviceAdminAddExplanationPreferenceController controller =
                new DeviceAdminAddExplanationPreferenceController(mSpiedContext, mPreferenceKey,
                        mFragmentController, mUxRestrictions);

        assertAvailability(controller.getAvailabilityStatus(),
                PreferenceController.CONDITIONALLY_UNAVAILABLE);
    }

    @Test
    public void testGetAvailabilityStatus_deviceOwner() throws Exception {
        mockDeviceOwner();
        mController.setExplanation("To conquer the universe");

        assertAvailability(mController.getAvailabilityStatus(),
                PreferenceController.DISABLED_FOR_PROFILE);
    }

    @Test
    public void testGetAvailabilityStatus_profileOwner() throws Exception {
        mockProfileOwner();
        mController.setExplanation("To conquer the universe");

        assertAvailability(mController.getAvailabilityStatus(),
                PreferenceController.DISABLED_FOR_PROFILE);
    }

    @Test
    public void testGetRealAvailabilityStatus_nullReason() throws Exception {
        mController.setExplanation(null);

        assertAvailability(mController.getAvailabilityStatus(),
                PreferenceController.CONDITIONALLY_UNAVAILABLE);
    }

    @Test
    public void testGetAvailabilityStatus_emptyReason() throws Exception {
        mController.setExplanation("");

        assertAvailability(mController.getAvailabilityStatus(),
                PreferenceController.CONDITIONALLY_UNAVAILABLE);
    }

    @Test
    public void testGetAvailabilityStatus_validReason() throws Exception {
        mController.setExplanation("To conquer the universe");

        assertAvailability(mController.getAvailabilityStatus(), PreferenceController.AVAILABLE);
    }

    @Test
    public void testUpdateState() throws Exception {
        mController.setExplanation("To conquer the universe");

        mController.updateState(mPreference);

        verifyPreferenceTitleSet("To conquer the universe");
        verifyPreferenceSummaryNeverSet();
        verifyPreferenceIconNeverSet();
    }
}
