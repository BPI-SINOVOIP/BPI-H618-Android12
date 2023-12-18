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

package com.android.car.settings.applications;

import static android.provider.DeviceConfig.NAMESPACE_APP_HIBERNATION;

import static com.android.car.settings.applications.ApplicationsUtils.PROPERTY_APP_HIBERNATION_ENABLED;

import static com.google.common.truth.Truth.assertThat;

import static org.mockito.ArgumentMatchers.anyInt;
import static org.mockito.ArgumentMatchers.anyLong;
import static org.mockito.ArgumentMatchers.anyString;
import static org.mockito.Mockito.mock;
import static org.mockito.Mockito.spy;
import static org.mockito.Mockito.when;

import android.app.usage.IUsageStatsManager;
import android.app.usage.UsageStats;
import android.app.usage.UsageStatsManager;
import android.apphibernation.AppHibernationManager;
import android.content.Context;
import android.content.pm.PackageInfo;
import android.content.pm.PackageManager;
import android.content.pm.ParceledListSlice;
import android.content.res.Resources;
import android.os.RemoteException;
import android.provider.DeviceConfig;

import androidx.test.core.app.ApplicationProvider;
import androidx.test.ext.junit.runners.AndroidJUnit4;

import org.junit.Before;
import org.junit.Test;
import org.junit.runner.RunWith;
import org.mockito.Mock;
import org.mockito.MockitoAnnotations;

import java.util.Arrays;
import java.util.concurrent.CountDownLatch;
import java.util.concurrent.TimeUnit;

@RunWith(AndroidJUnit4.class)
public class HibernatedAppsItemManagerTest {
    private static final int CALLBACK_TIMEOUT_MS = 100;

    private static final String HIBERNATED_PACKAGE_NAME = "hibernated_package";
    private static final String AUTO_REVOKED_PACKAGE_NAME = "auto_revoked_package";
    private static final String PERMISSION = "permission";
    private final CountDownLatch mCountDownLatch = new CountDownLatch(1);
    private final TestListener mHibernatedAppsCountListener = new TestListener();
    @Mock
    private PackageManager mPackageManager;
    @Mock
    private AppHibernationManager mAppHibernationManager;
    @Mock
    private IUsageStatsManager mIUsageStatsManager;
    private Context mContext;
    private HibernatedAppsItemManager mManager;

    @Before
    public void setUp() {
        MockitoAnnotations.initMocks(this);
        DeviceConfig.setProperty(NAMESPACE_APP_HIBERNATION, PROPERTY_APP_HIBERNATION_ENABLED,
                "true", false);
        mContext = spy(ApplicationProvider.getApplicationContext());
        when(mContext.getPackageManager()).thenReturn(mPackageManager);
        when(mContext.getSystemService(AppHibernationManager.class))
                .thenReturn(mAppHibernationManager);
        when(mContext.getSystemService(UsageStatsManager.class)).thenReturn(
                new UsageStatsManager(mContext, mIUsageStatsManager));
        mManager = new HibernatedAppsItemManager(mContext);
        mManager.setListener(mHibernatedAppsCountListener);
    }


    @Test
    public void getSummary_getsRightCountForHibernatedPackage() throws Exception {
        PackageInfo hibernatedPkg = getHibernatedPackage();
        when(mPackageManager.getInstalledPackages(anyInt())).thenReturn(
                Arrays.asList(hibernatedPkg, new PackageInfo()));
        when(mContext.getResources()).thenReturn(mock(Resources.class));

        mManager.startLoading();
        mCountDownLatch.await(CALLBACK_TIMEOUT_MS, TimeUnit.MILLISECONDS);

        assertThat(mHibernatedAppsCountListener.mResult).isEqualTo(1);
    }

    @Test
    public void getSummary_getsRightCountForUnusedAutoRevokedPackage() throws Exception {
        PackageInfo autoRevokedPkg = getAutoRevokedPackage();
        when(mPackageManager.getInstalledPackages(anyInt())).thenReturn(
                Arrays.asList(autoRevokedPkg, new PackageInfo()));
        when(mContext.getResources()).thenReturn(mock(Resources.class));

        mManager.startLoading();
        mCountDownLatch.await(CALLBACK_TIMEOUT_MS, TimeUnit.MILLISECONDS);

        assertThat(mHibernatedAppsCountListener.mResult).isEqualTo(1);
    }

    @Test
    public void getSummary_getsRightCountForUsedAutoRevokedPackage() throws Exception {
        PackageInfo usedAutoRevokedPkg = getAutoRevokedPackage();
        setAutoRevokedPackageUsageStats();
        when(mPackageManager.getInstalledPackages(anyInt())).thenReturn(
                Arrays.asList(usedAutoRevokedPkg, new PackageInfo()));
        when(mContext.getResources()).thenReturn(mock(Resources.class));

        mManager.startLoading();
        mCountDownLatch.await(CALLBACK_TIMEOUT_MS, TimeUnit.MILLISECONDS);

        assertThat(mHibernatedAppsCountListener.mResult).isEqualTo(0);
    }


    private PackageInfo getHibernatedPackage() {
        PackageInfo pi = new PackageInfo();
        pi.packageName = HIBERNATED_PACKAGE_NAME;
        pi.requestedPermissions = new String[]{PERMISSION};
        when(mAppHibernationManager.getHibernatingPackagesForUser())
                .thenReturn(Arrays.asList(pi.packageName));
        when(mPackageManager.getPermissionFlags(
                pi.requestedPermissions[0], pi.packageName, mContext.getUser()))
                .thenReturn(PackageManager.FLAG_PERMISSION_AUTO_REVOKED);
        return pi;
    }

    private PackageInfo getAutoRevokedPackage() {
        PackageInfo pi = new PackageInfo();
        pi.packageName = AUTO_REVOKED_PACKAGE_NAME;
        pi.requestedPermissions = new String[]{PERMISSION};
        when(mPackageManager.getPermissionFlags(
                pi.requestedPermissions[0], pi.packageName, mContext.getUser()))
                .thenReturn(PackageManager.FLAG_PERMISSION_AUTO_REVOKED);
        return pi;
    }

    private void setAutoRevokedPackageUsageStats() {
        UsageStats us = new UsageStats();
        us.mPackageName = AUTO_REVOKED_PACKAGE_NAME;
        us.mLastTimeVisible = System.currentTimeMillis();
        try {
            when(mIUsageStatsManager.queryUsageStats(
                    anyInt(), anyLong(), anyLong(), anyString(), anyInt()))
                    .thenReturn(new ParceledListSlice(Arrays.asList(us)));
        } catch (RemoteException e) {
            // no-op
        }
    }

    private class TestListener implements HibernatedAppsItemManager.HibernatedAppsCountListener {
        int mResult;

        @Override
        public void onHibernatedAppsCountLoaded(int hibernatedAppsCount) {
            mResult = hibernatedAppsCount;
            mCountDownLatch.countDown();
        }
    };
}
