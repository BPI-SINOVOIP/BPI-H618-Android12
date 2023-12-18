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

import org.junit.Before;
import org.junit.Test;

public final class DeviceAdminAddHeaderPreferenceControllerTest extends
        BaseDeviceAdminAddPreferenceControllerTestCase
                <DeviceAdminAddHeaderPreferenceController> {

    private DeviceAdminAddHeaderPreferenceController mController;

    @Before
    public void setController() {
        mController = new DeviceAdminAddHeaderPreferenceController(mSpiedContext,
                mPreferenceKey, mFragmentController, mUxRestrictions);
        mController.setDeviceAdmin(mDefaultDeviceAdminInfo);
    }

    @Test
    public void testUpdateState_adminWithNoProperties() throws Exception {
        mController.updateState(mPreference);

        verifyPreferenceTitleSet(DefaultDeviceAdminReceiver.class.getName());
        verifyPreferenceSummaryNeverSet();
        verifyPreferenceIconSet();
    }

    @Test
    public void testUpdateState_adminWithAllProperties() throws Exception {
        mController.setDeviceAdmin(mFancyDeviceAdminInfo);

        mController.updateState(mPreference);

        verifyPreferenceTitleSet("LordOfTheSevenReceiverKingdoms");
        verifyPreferenceSummarySet("One Receiver to Rule them All");
        verifyPreferenceIconSet();
    }
}
