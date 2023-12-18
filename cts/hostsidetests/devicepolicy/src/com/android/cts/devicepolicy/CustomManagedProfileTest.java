/*
 * Copyright (C) 2016 The Android Open Source Project
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
package com.android.cts.devicepolicy;

import static com.android.cts.devicepolicy.DeviceAdminFeaturesCheckerRule.FEATURE_MANAGED_USERS;

import com.android.cts.devicepolicy.DeviceAdminFeaturesCheckerRule.DoesNotRequireFeature;
import com.android.cts.devicepolicy.DeviceAdminFeaturesCheckerRule.RequiresAdditionalFeatures;
import com.android.tradefed.device.DeviceNotAvailableException;
import com.android.tradefed.log.LogUtil.CLog;

import org.junit.Test;

// We need multi user to be supported in order to create a profile of the user owner.
@RequiresAdditionalFeatures({FEATURE_MANAGED_USERS})
public class CustomManagedProfileTest extends BaseDevicePolicyTest {

    private static final String MANAGED_PROFILE_PKG = "com.android.cts.managedprofile";
    private static final String MANAGED_PROFILE_APK = "CtsManagedProfileApp.apk";

    @DoesNotRequireFeature
    @Test
    public void testIsProvisioningAllowed() throws Exception {
        // Must install the apk since the test runs in the ManagedProfile apk.
        installAppAsUser(MANAGED_PROFILE_APK, mPrimaryUserId);
        try {
            if (mFeaturesCheckerRule.hasRequiredFeatures()) {
                // Since we assume, in ManagedProfileTest, provisioning has to be successful,
                // DevicePolicyManager.isProvisioningAllowed must return true
                assertIsProvisioningAllowed(true, mPrimaryUserId);
            } else {
                // Test the case when feature flag is off
                assertIsProvisioningAllowed(false, mPrimaryUserId);
            }
        } finally {
            getDevice().uninstallPackage(MANAGED_PROFILE_PKG);
        }
    }

    private void assertIsProvisioningAllowed(boolean expected, int userId)
            throws DeviceNotAvailableException {
        final String testName = expected ? "testIsProvisioningAllowedTrue"
                : "testIsProvisioningAllowedFalse";
        CLog.d("Running test %s on user %d", testName, userId);
        runDeviceTestsAsUser(MANAGED_PROFILE_PKG, ".PreManagedProfileTest", testName, userId);
    }
}
