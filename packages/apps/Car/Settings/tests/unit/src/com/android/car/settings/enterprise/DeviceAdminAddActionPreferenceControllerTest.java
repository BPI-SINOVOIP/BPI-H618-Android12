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

import static com.google.common.truth.Truth.assertWithMessage;

import com.android.car.settings.R;
import com.android.car.settings.common.PreferenceController;

import org.junit.Before;
import org.junit.Test;

public final class DeviceAdminAddActionPreferenceControllerTest extends
        BaseDeviceAdminAddPreferenceControllerTestCase
                <DeviceAdminAddActionPreferenceController> {

    private DeviceAdminAddActionPreferenceController mController;

    @Before
    public void setController() {
        mController = new DeviceAdminAddActionPreferenceController(mSpiedContext,
                mPreferenceKey, mFragmentController, mUxRestrictions);
        mController.setDeviceAdmin(mDefaultDeviceAdminInfo);
    }

    @Test
    public void testGetAvailabilityStatus_noAdmin() throws Exception {
        DeviceAdminAddActionPreferenceController controller =
                new DeviceAdminAddActionPreferenceController(mSpiedContext, mPreferenceKey,
                        mFragmentController, mUxRestrictions);

        assertAvailability(controller.getAvailabilityStatus(),
                PreferenceController.CONDITIONALLY_UNAVAILABLE);
    }

    @Test
    public void testUpdateStatus_deviceOwner() throws Exception {
        mockDeviceOwner();

        mController.updateState(mPreference);

        verifyPreferenceDisabled();
        verifyPreferenceTitleSet(R.string.remove_device_admin);
    }

    @Test
    public void testUpdateStatus_profileOwner() throws Exception {
        mockProfileOwner();

        mController.updateState(mPreference);

        verifyPreferenceDisabled();
        verifyPreferenceTitleSet(R.string.remove_device_admin);
    }

    @Test
    public void testUpdateStatus_activeAdmin() throws Exception {
        mockActiveAdmin();

        mController.updateState(mPreference);

        verifyPreferenceEnabled();
        verifyPreferenceTitleSet(R.string.remove_device_admin);
    }

    @Test
    public void testUpdateStatus_inactiveAdmin() throws Exception {
        mockInactiveAdmin();

        mController.updateState(mPreference);

        verifyPreferenceEnabled();
        verifyPreferenceTitleSet(R.string.add_device_admin);
    }

    @Test
    public void testHandlePreferenceClicked_deviceOwner() throws Exception {
        mockDeviceOwner();
        mController.setIsActive();

        handlePreferenceClicked();

        verifyAdminNeverActivated();
        verifyAdminNeverDeactivated();
        verifyGoBack();
    }

    @Test
    public void testHandlePreferenceClicked_profileOwner() throws Exception {
        mockProfileOwner();
        mController.setIsActive();

        handlePreferenceClicked();

        verifyAdminNeverActivated();
        verifyAdminNeverDeactivated();
        verifyGoBack();
    }

    @Test
    public void testHandlePreferenceClicked_activeAdmin() throws Exception {
        mockActiveAdmin();
        mController.setIsActive();

        handlePreferenceClicked();

        verifyAdminNeverActivated();
        verifyAdminDeactivated();
        verifyGoBack();
    }

    @Test
    public void testHandlePreferenceClicked_inactiveAdmin() throws Exception {
        mockInactiveAdmin();
        mController.setIsActive();

        handlePreferenceClicked();

        verifyAdminActivated();
        verifyAdminNeverDeactivated();
        verifyGoBack();
    }

    private void handlePreferenceClicked() {
        boolean handled = mController.handlePreferenceClicked(mPreference);
        assertWithMessage("handlePreferenceClicked() result").that(handled).isTrue();
    }
}
