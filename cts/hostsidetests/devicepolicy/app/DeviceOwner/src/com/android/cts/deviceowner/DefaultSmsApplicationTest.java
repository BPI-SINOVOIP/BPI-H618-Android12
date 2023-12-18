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

package com.android.cts.deviceowner;

import static com.google.common.truth.Truth.assertWithMessage;

import android.app.admin.DevicePolicyManager;
import android.provider.Telephony;
import android.util.Log;

public class DefaultSmsApplicationTest extends BaseDeviceOwnerTest {

    private static final String TAG = DefaultSmsApplicationTest.class.getSimpleName();

    public void testSetDefaultSmsApplication() {
        // Must use a DPM associated with the current user because the Telephony.Sms methods will
        // return the app for the calling user, and on headless system user, mDevicePolicyManager
        // wraps the calls to the DeviceOwner user (which fails the test because the value didn't
        // change as expected).
        DevicePolicyManager dpm = mContext.getSystemService(DevicePolicyManager.class);

        String previousSmsAppName = Telephony.Sms.getDefaultSmsPackage(mContext);
        String newSmsAppName = "android.telephony.cts.sms.simplesmsapp";
        Log.v(TAG, "testSetDefaultSmsApplication(): previous=" + previousSmsAppName
                + ", new=" + newSmsAppName + ", user=" + mContext.getUserId()
                + ", isAffiliated=" + dpm.isAffiliatedUser());

        dpm.setDefaultSmsApplication(getWho(), newSmsAppName);
        String defaultSmsApp = Telephony.Sms.getDefaultSmsPackage(mContext);
        assertWithMessage("default app returned by Telephony.Sms after set by DPM")
                .that(defaultSmsApp).isNotNull();
        assertWithMessage("default app returned by Telephony.Sms after set by DPM")
                .that(defaultSmsApp).isEqualTo(newSmsAppName);

        // Restore previous default sms application
        dpm.setDefaultSmsApplication(getWho(), previousSmsAppName);
        defaultSmsApp = Telephony.Sms.getDefaultSmsPackage(mContext);
        assertWithMessage("default app returned by Telephony.Sms after restored by DPM")
                .that(defaultSmsApp).isNotNull();
        assertWithMessage("default app returned by Telephony.Sms after restored by DPM")
                .that(defaultSmsApp).isEqualTo(previousSmsAppName);
    }
}
