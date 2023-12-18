/*
 * Copyright (C) 2018 The Android Open Source Project
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

package android.telecom.cts;

import android.content.Context;
import android.telecom.TelecomManager;
import android.test.InstrumentationTestCase;
import android.util.Log;

/**
 * Verifies correct operation of TelecomManager APIs when the correct permissions have not been
 * granted.
 */
public class TelecomManagerNoPermissionsTest extends InstrumentationTestCase {
    private Context mContext;
    private TelecomManager mTelecomManager;

    @Override
    protected void setUp() throws Exception {
        super.setUp();
        mContext = getInstrumentation().getContext();
        if (!TestUtils.shouldTestTelecom(mContext)) {
            return;
        }
        TestUtils.PACKAGE = mContext.getPackageName();
        mTelecomManager = (TelecomManager) mContext.getSystemService(Context.TELECOM_SERVICE);
    }

    @Override
    protected void tearDown() throws Exception {
        super.tearDown();
    }

    public void testCannotEndCall() throws Exception {
        if (!TestUtils.shouldTestTelecom(mContext)) {
            return;
        }
        try {
            mTelecomManager.endCall();
            fail("Shouldn't be able to call endCall without permission grant.");
        } catch (SecurityException se) {
        }
    }

    public void testCallStateCompatPermissions() throws Exception {
        if (!TestUtils.shouldTestTelecom(mContext)) {
            return;
        }

        try {
            TelecomManager tm = mContext.getSystemService(TelecomManager.class);
            assertNotNull(tm);

            TestUtils.enableCompatCommand(getInstrumentation(),
                    TestUtils.ENABLE_GET_CALL_STATE_PERMISSION_PROTECTION_STRING);
            try {

                tm.getCallState();
                fail("TelecomManager#getCallState must require READ_PHONE_STATE when "
                        + "TelecomManager#ENABLE_GET_CALL_STATE_PERMISSION_PROTECTION is enabled");
            } catch (SecurityException e) {
                // expected
            }
        } finally {
            TestUtils.resetCompatCommand(getInstrumentation(),
                    TestUtils.ENABLE_GET_CALL_STATE_PERMISSION_PROTECTION_STRING);
        }
    }

    public void testGetPhoneAccountCompatPermissions() throws Exception {
        if (!TestUtils.shouldTestTelecom(mContext)) {
            return;
        }

        try {
            TestUtils.enableCompatCommand(getInstrumentation(),
                    TestUtils.ENABLE_GET_PHONE_ACCOUNT_PERMISSION_PROTECTION_STRING);

            try {
                mTelecomManager.getPhoneAccount(TestUtils.TEST_DEFAULT_PHONE_ACCOUNT_HANDLE_1);
                fail("TelecomManager#getPhoneAccount should require READ_PHONE_NUMBERS or "
                        + "READ_PRIVILEGED_PHONE_STATE when "
                        + "ENABLE_GET_PHONE_ACCOUNT_PERMISSION_PROTECTION is enabled");
            } catch (SecurityException e) {
                //expected
            }
        } finally {
            TestUtils.resetCompatCommand(getInstrumentation(),
                    TestUtils.ENABLE_GET_PHONE_ACCOUNT_PERMISSION_PROTECTION_STRING);
        }
    }
}
