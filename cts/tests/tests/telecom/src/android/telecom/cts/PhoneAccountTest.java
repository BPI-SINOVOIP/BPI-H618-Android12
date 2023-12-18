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

package android.telecom.cts;

import android.telecom.PhoneAccount;
import android.test.AndroidTestCase;

import static org.junit.Assert.assertFalse;
import static org.junit.Assert.assertTrue;

public class PhoneAccountTest extends AndroidTestCase {
    public static final String ACCOUNT_LABEL = "CTSPhoneAccountTest";

    public void testPhoneAccountCapabilitiesForCallComposer() {
        PhoneAccount testPhoneAccount = PhoneAccount.builder(
                TestUtils.TEST_PHONE_ACCOUNT_HANDLE, ACCOUNT_LABEL)
                .setCapabilities(PhoneAccount.CAPABILITY_CALL_COMPOSER)
                .build();
        assertTrue(testPhoneAccount.hasCapabilities(PhoneAccount.CAPABILITY_CALL_COMPOSER));

        testPhoneAccount = PhoneAccount.builder(
                TestUtils.TEST_PHONE_ACCOUNT_HANDLE, ACCOUNT_LABEL)
                .setCapabilities(PhoneAccount.CAPABILITY_VIDEO_CALLING)
                .build();
        assertFalse(testPhoneAccount.hasCapabilities(PhoneAccount.CAPABILITY_CALL_COMPOSER));
    }
}