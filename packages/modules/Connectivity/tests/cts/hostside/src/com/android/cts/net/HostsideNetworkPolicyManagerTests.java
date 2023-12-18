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

package com.android.cts.net;

public class HostsideNetworkPolicyManagerTests extends HostsideNetworkTestCase {
    @Override
    protected void setUp() throws Exception {
        super.setUp();
        uninstallPackage(TEST_APP2_PKG, false);
        installPackage(TEST_APP2_APK);
    }

    @Override
    protected void tearDown() throws Exception {
        super.tearDown();
        uninstallPackage(TEST_APP2_PKG, true);
    }

    public void testIsUidNetworkingBlocked_withUidNotBlocked() throws Exception {
        runDeviceTests(TEST_PKG,
                TEST_PKG + ".NetworkPolicyManagerTest",
                "testIsUidNetworkingBlocked_withUidNotBlocked");
    }

    public void testIsUidNetworkingBlocked_withSystemUid() throws Exception {
        runDeviceTests(TEST_PKG,
                TEST_PKG + ".NetworkPolicyManagerTest", "testIsUidNetworkingBlocked_withSystemUid");
    }

    public void testIsUidNetworkingBlocked_withDataSaverMode() throws Exception {
        runDeviceTests(TEST_PKG,
                TEST_PKG + ".NetworkPolicyManagerTest",
                "testIsUidNetworkingBlocked_withDataSaverMode");
    }

    public void testIsUidNetworkingBlocked_withRestrictedNetworkingMode() throws Exception {
        runDeviceTests(TEST_PKG,
                TEST_PKG + ".NetworkPolicyManagerTest",
                "testIsUidNetworkingBlocked_withRestrictedNetworkingMode");
    }

    public void testIsUidNetworkingBlocked_withPowerSaverMode() throws Exception {
        runDeviceTests(TEST_PKG,
                TEST_PKG + ".NetworkPolicyManagerTest",
                "testIsUidNetworkingBlocked_withPowerSaverMode");
    }

    public void testIsUidRestrictedOnMeteredNetworks() throws Exception {
        runDeviceTests(TEST_PKG,
                TEST_PKG + ".NetworkPolicyManagerTest", "testIsUidRestrictedOnMeteredNetworks");
    }
}
