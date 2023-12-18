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

package android.devicepolicy.cts;

import static org.testng.Assert.assertThrows;

import android.app.admin.DevicePolicyManager;
import android.content.Context;
import android.content.pm.PackageManager;

import androidx.test.core.app.ApplicationProvider;
import androidx.test.filters.SmallTest;

import com.android.bedstead.harrier.BedsteadJUnit4;
import com.android.bedstead.harrier.DeviceState;
import com.android.bedstead.harrier.annotations.RequireFeature;

import org.junit.ClassRule;
import org.junit.Rule;
import org.junit.Test;
import org.junit.runner.RunWith;

/**
 * Test that certain DevicePolicyManager APIs aren't available to non-owner apps and that they throw
 * SecurityException when invoked by such apps. For most of the older APIs that accept an explicit
 * ComponentName admin argument, this is tested in android.admin.cts.DevicePolicyManagerTest by
 * passing an admin that is not owner, but for newer APIs authorization is done based on caller UID,
 * so it is critical that the app is not owner. These APIs are tested here.
 */
@SmallTest
@RunWith(BedsteadJUnit4.class)
public class NegativeCallAuthorizationTest {
    private static final String ALIAS = "some-alias";
    private static final Context sContext = ApplicationProvider.getApplicationContext();
    private static final DevicePolicyManager sDpm =
            sContext.getSystemService(DevicePolicyManager.class);

    @ClassRule @Rule
    public static final DeviceState sDeviceState = new DeviceState();

    @Test
    @RequireFeature(PackageManager.FEATURE_DEVICE_ADMIN)
    public void testHasKeyPair_failIfNotOwner() {
        assertThrows(SecurityException.class, () -> sDpm.hasKeyPair(ALIAS));
    }

    @Test
    @RequireFeature(PackageManager.FEATURE_DEVICE_ADMIN)
    public void testGetKeyPairGrants_failIfNotOwner() {
        assertThrows(SecurityException.class, () -> sDpm.getKeyPairGrants(ALIAS));
    }
}
