/*
 * Copyright (C) 2017 The Android Open Source Project
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
 * limitations under the License
 */
package com.android.cts.devicepolicy;

import static com.android.cts.devicepolicy.DeviceAdminFeaturesCheckerRule.FEATURE_MANAGED_USERS;

import android.platform.test.annotations.FlakyTest;

import com.android.cts.devicepolicy.DeviceAdminFeaturesCheckerRule.RequiresAdditionalFeatures;

import org.junit.Ignore;
import org.junit.Test;

/**
 * This class tests the provisioning flow with an APK that declares a single receiver with
 * BIND_DEVICE_ADMIN permissions, which was a requirement for the app sending the
 * ACTION_PROVISION_MANAGED_PROFILE intent before Android M.
 */
// We need multi user to be supported in order to create a profile of the user owner.
@RequiresAdditionalFeatures({FEATURE_MANAGED_USERS})
public class ManagedProfileProvisioningSingleAdminTest extends BaseDevicePolicyTest {

    private static final String SINGLE_ADMIN_PKG = "com.android.cts.devicepolicy.singleadmin";
    private static final String SINGLE_ADMIN_APP_APK = "CtsDevicePolicySingleAdminTestApp.apk";

    private int mProfileUserId;

    @Override
    public void setUp() throws Exception {
        super.setUp();


        removeTestUsers();
        installAppAsUser(SINGLE_ADMIN_APP_APK, mPrimaryUserId);
        mProfileUserId = 0;
    }

    @Override
    public void tearDown() throws Exception {
        if (mProfileUserId != 0) {
            removeUser(mProfileUserId);
        }
        getDevice().uninstallPackage(SINGLE_ADMIN_PKG);

        super.tearDown();
    }

    @FlakyTest
    @Test
    @Ignore("b/183395856 Figure out if it should be removed or converted to a device side test.")
    public void testEXTRA_PROVISIONING_DEVICE_ADMIN_PACKAGE_NAME() throws Exception {
        runDeviceTestsAsUser(SINGLE_ADMIN_PKG, ".ProvisioningSingleAdminTest",
                "testManagedProfileProvisioning", mPrimaryUserId);

        mProfileUserId = getFirstManagedProfileUserId();
    }
}
