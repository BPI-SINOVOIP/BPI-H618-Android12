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

package com.android.phone;

import static org.mockito.Mockito.doReturn;
import static org.mockito.Mockito.never;
import static org.mockito.Mockito.verify;

import android.content.pm.PackageManager;
import android.telephony.RadioAccessFamily;
import android.telephony.TelephonyManager;

import androidx.test.annotation.UiThreadTest;
import androidx.test.ext.junit.runners.AndroidJUnit4;

import com.android.TelephonyTestBase;
import com.android.internal.telephony.Phone;
import com.android.internal.telephony.RILConstants;

import org.junit.Before;
import org.junit.Test;
import org.junit.runner.RunWith;
import org.mockito.Mock;

/**
 * Unit Test for CarrierConfigLoader.
 */
@RunWith(AndroidJUnit4.class)
public class PhoneInterfaceManagerTest extends TelephonyTestBase {
    private PhoneInterfaceManager mPhoneInterfaceManager;

    @Mock
    PhoneGlobals mPhoneGlobals;
    @Mock
    Phone mPhone;
    @Mock
    PackageManager mPackageManager;

    @Before
    @UiThreadTest
    public void setUp() throws Exception {
        super.setUp();
        doReturn(mPackageManager).when(mPhoneGlobals).getPackageManager();
        doReturn(false).when(mPackageManager).hasSystemFeature(
                PackageManager.FEATURE_TELEPHONY_IMS);
        mPhoneInterfaceManager = PhoneInterfaceManager.init(mPhoneGlobals);
    }

    @Test
    public void cleanUpAllowedNetworkTypes_validPhoneAndSubId_doSetAllowedNetwork() {
        long defaultNetworkType = RadioAccessFamily.getRafFromNetworkType(
                RILConstants.PREFERRED_NETWORK_MODE);

        mPhoneInterfaceManager.cleanUpAllowedNetworkTypes(mPhone, 1);

        verify(mPhone).loadAllowedNetworksFromSubscriptionDatabase();
        verify(mPhone).setAllowedNetworkTypes(TelephonyManager.ALLOWED_NETWORK_TYPES_REASON_USER,
                defaultNetworkType, null);
    }

    @Test
    public void cleanUpAllowedNetworkTypes_validPhoneAndInvalidSubId_doNotSetAllowedNetwork() {
        long defaultNetworkType = RadioAccessFamily.getRafFromNetworkType(
                RILConstants.PREFERRED_NETWORK_MODE);

        mPhoneInterfaceManager.cleanUpAllowedNetworkTypes(mPhone, -1);

        verify(mPhone, never()).loadAllowedNetworksFromSubscriptionDatabase();
        verify(mPhone, never()).setAllowedNetworkTypes(
                TelephonyManager.ALLOWED_NETWORK_TYPES_REASON_USER, defaultNetworkType, null);
    }
}
