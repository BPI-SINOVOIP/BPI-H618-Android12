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

import static org.mockito.ArgumentMatchers.any;
import static org.mockito.ArgumentMatchers.anyBoolean;
import static org.mockito.ArgumentMatchers.anyInt;
import static org.mockito.ArgumentMatchers.eq;
import static org.mockito.Mockito.never;
import static org.mockito.Mockito.spy;
import static org.mockito.Mockito.verify;
import static org.mockito.Mockito.when;

import android.app.admin.DeviceAdminInfo;
import android.app.admin.DevicePolicyManager;
import android.content.ComponentName;
import android.content.Context;
import android.content.pm.ActivityInfo;
import android.content.pm.PackageManager;
import android.os.UserManager;

import androidx.test.core.app.ApplicationProvider;
import androidx.test.ext.junit.runners.AndroidJUnit4;

import org.junit.Before;
import org.junit.Rule;
import org.junit.runner.RunWith;
import org.mockito.Mock;
import org.mockito.junit.MockitoJUnit;
import org.mockito.junit.MockitoRule;

import java.util.Arrays;

@RunWith(AndroidJUnit4.class)
abstract class BaseEnterpriseTestCase {

    @Rule
    public final MockitoRule mockitorule = MockitoJUnit.rule();

    protected final Context mRealContext = ApplicationProvider.getApplicationContext();
    protected final Context mSpiedContext = spy(mRealContext);

    protected final String mPackageName = mRealContext.getPackageName();

    private final PackageManager mRealPm = mRealContext.getPackageManager();
    protected PackageManager mSpiedPm = spy(mRealPm);

    protected final ComponentName mDefaultAdmin =
            new ComponentName(mSpiedContext, DefaultDeviceAdminReceiver.class);
    protected final ComponentName mFancyAdmin =
            new ComponentName(mSpiedContext, FancyDeviceAdminReceiver.class);

    protected DeviceAdminInfo mDefaultDeviceAdminInfo;
    protected DeviceAdminInfo mFancyDeviceAdminInfo;

    @Mock
    private DevicePolicyManager mDpm;

    @Mock
    private UserManager mUm;

    @Before
    public final void setFixtures() throws Exception {
        when(mSpiedContext.getSystemService(DevicePolicyManager.class)).thenReturn(mDpm);
        when(mSpiedContext.getSystemService(PackageManager.class)).thenReturn(mSpiedPm);
        when(mSpiedContext.getPackageManager()).thenReturn(mSpiedPm);
        when(mSpiedContext.getSystemService(UserManager.class)).thenReturn(mUm);

        ActivityInfo defaultInfo = mRealPm.getReceiverInfo(mDefaultAdmin,
                PackageManager.GET_META_DATA);
        mDefaultDeviceAdminInfo = new DeviceAdminInfo(mRealContext, defaultInfo);

        ActivityInfo fancyInfo = mRealPm.getReceiverInfo(mFancyAdmin, PackageManager.GET_META_DATA);
        mFancyDeviceAdminInfo = new DeviceAdminInfo(mRealContext, fancyInfo);
    }

    protected final void mockProfileOwner() {
        mockActiveAdmin();
        when(mDpm.getProfileOwner()).thenReturn(mDefaultAdmin);
    }

    protected final void mockDeviceOwner() {
        mockActiveAdmin();
        when(mDpm.getDeviceOwnerComponentOnCallingUser()).thenReturn(mDefaultAdmin);
        when(mDpm.getDeviceOwnerComponentOnAnyUser()).thenReturn(mDefaultAdmin);
    }

    protected final void mockFinancialDevice() {
        when(mDpm.isDeviceManaged()).thenReturn(true);
        when(mDpm.getDeviceOwnerType(mDefaultAdmin))
                .thenReturn(DevicePolicyManager.DEVICE_OWNER_TYPE_FINANCED);
    }

    protected final void mockActiveAdmin() {
        when(mDpm.isAdminActive(mDefaultAdmin)).thenReturn(true);
    }

    protected final void mockInactiveAdmin() {
        when(mDpm.isAdminActive(mDefaultAdmin)).thenReturn(false);
    }

    protected final void mockActiveAdmin(ComponentName admin) {
        when(mDpm.getActiveAdmins()).thenReturn(Arrays.asList(admin));
    }

    protected final void mockGetLongSupportMessageForUser(CharSequence message) {
        when(mDpm.getLongSupportMessageForUser(eq(mDefaultAdmin), anyInt())).thenReturn(message);
    }

    protected final void mockHasDeviceAdminFeature() {
        when(mSpiedPm.hasSystemFeature(PackageManager.FEATURE_DEVICE_ADMIN)).thenReturn(true);
    }

    protected final void mockNoDeviceAdminFeature() {
        when(mSpiedPm.hasSystemFeature(PackageManager.FEATURE_DEVICE_ADMIN)).thenReturn(false);
    }

    protected final void verifyAdminActivated() {
        verify(mDpm).setActiveAdmin(eq(mDefaultAdmin), anyBoolean());
    }

    protected final void verifyAdminNeverActivated() {
        verify(mDpm, never()).setActiveAdmin(any(), anyBoolean());
    }

    protected final void verifyAdminDeactivated() {
        verify(mDpm).removeActiveAdmin(mDefaultAdmin);
    }

    protected final void verifyAdminNeverDeactivated() {
        verify(mDpm, never()).removeActiveAdmin(any());
    }

    protected final void mockAdminUser() {
        when(mUm.isAdminUser()).thenReturn(true);
    }

    protected final void mockNonAdminUser() {
        when(mUm.isAdminUser()).thenReturn(false);
    }
}
