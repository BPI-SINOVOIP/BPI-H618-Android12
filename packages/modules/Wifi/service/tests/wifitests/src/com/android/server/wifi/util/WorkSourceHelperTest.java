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
package com.android.server.wifi.util;

import static android.app.ActivityManager.RunningAppProcessInfo.IMPORTANCE_BACKGROUND;
import static android.app.ActivityManager.RunningAppProcessInfo.IMPORTANCE_FOREGROUND;
import static android.app.ActivityManager.RunningAppProcessInfo.IMPORTANCE_FOREGROUND_SERVICE;

import static org.junit.Assert.assertFalse;
import static org.junit.Assert.assertTrue;
import static org.mockito.ArgumentMatchers.any;
import static org.mockito.ArgumentMatchers.anyInt;
import static org.mockito.Mockito.when;

import android.app.ActivityManager;
import android.content.pm.ApplicationInfo;
import android.content.pm.PackageManager;
import android.os.Process;
import android.os.WorkSource;

import androidx.test.filters.SmallTest;

import com.android.server.wifi.WifiBaseTest;

import org.junit.Before;
import org.junit.Test;
import org.junit.runner.RunWith;
import org.junit.runners.JUnit4;
import org.mockito.Mock;
import org.mockito.MockitoAnnotations;

/** Unit tests for {@link WorkSourceHelper}. */
@RunWith(JUnit4.class)
@SmallTest
public class WorkSourceHelperTest extends WifiBaseTest {
    private static final int TEST_UID_1 = 456456;
    private static final int TEST_UID_2 = 456456;
    private static final String TEST_PACKAGE_1 = "com.android.test.1";
    private static final String TEST_PACKAGE_2 = "com.android.test.2";

    @Mock private WifiPermissionsUtil mWifiPermissionsUtil;
    @Mock private ActivityManager mActivityManager;
    @Mock private PackageManager mPackageManager;

    private WorkSource mWorkSource;
    private WorkSourceHelper mWorkSourceHelper;

    @Before public void setUp() {
        MockitoAnnotations.initMocks(this);

        // Create a test worksource with 2 app's request.
        mWorkSource = new WorkSource();
        mWorkSource.add(new WorkSource(TEST_UID_1, TEST_PACKAGE_1));
        mWorkSource.add(new WorkSource(TEST_UID_2, TEST_PACKAGE_2));

        mWorkSourceHelper = new WorkSourceHelper(
                mWorkSource, mWifiPermissionsUtil, mActivityManager, mPackageManager);
    }

    @Test
    public void testHasAnyPrivilegedRequest() {
        assertFalse(mWorkSourceHelper.hasAnyPrivilegedAppRequest());

        when(mWifiPermissionsUtil.checkNetworkSettingsPermission(TEST_UID_1)).thenReturn(true);
        assertTrue(mWorkSourceHelper.hasAnyPrivilegedAppRequest());
    }

    @Test
    public void testHasAnySystemRequest() throws Exception {
        ApplicationInfo appInfo = new ApplicationInfo();
        when(mPackageManager.getApplicationInfoAsUser(any(), anyInt(), any())).thenReturn(appInfo);

        assertFalse(mWorkSourceHelper.hasAnySystemAppRequest());

        appInfo.flags = ApplicationInfo.FLAG_SYSTEM;
        assertTrue(mWorkSourceHelper.hasAnySystemAppRequest());
    }

    @Test
    public void testHasAnyForegroundAppRequest() throws Exception {
        // 2 from bg app.
        when(mActivityManager.getPackageImportance(TEST_PACKAGE_1))
                .thenReturn(IMPORTANCE_BACKGROUND);
        when(mActivityManager.getPackageImportance(TEST_PACKAGE_2))
                .thenReturn(IMPORTANCE_BACKGROUND);
        assertFalse(mWorkSourceHelper.hasAnyForegroundAppRequest());

        // 1 request from fg app, 1 from bg app.
        when(mActivityManager.getPackageImportance(TEST_PACKAGE_1))
                .thenReturn(IMPORTANCE_FOREGROUND);
        assertTrue(mWorkSourceHelper.hasAnyForegroundAppRequest());
    }

    @Test
    public void testHasAnyForegroundServiceRequest() throws Exception {
        // 2 from bg app.
        when(mActivityManager.getPackageImportance(TEST_PACKAGE_1))
                .thenReturn(IMPORTANCE_BACKGROUND);
        when(mActivityManager.getPackageImportance(TEST_PACKAGE_2))
                .thenReturn(IMPORTANCE_BACKGROUND);
        assertFalse(mWorkSourceHelper.hasAnyForegroundServiceRequest());

        // 1 request from fg service, 1 from bg app.
        when(mActivityManager.getPackageImportance(TEST_PACKAGE_1))
                .thenReturn(IMPORTANCE_FOREGROUND_SERVICE);
        when(mActivityManager.getPackageImportance(TEST_PACKAGE_2))
                .thenReturn(IMPORTANCE_BACKGROUND);
        assertTrue(mWorkSourceHelper.hasAnyForegroundServiceRequest());
    }


    @Test
    public void testHasAnyInternalRequest() throws Exception {
        // 2 from bg app.
        when(mActivityManager.getPackageImportance(TEST_PACKAGE_1))
                .thenReturn(IMPORTANCE_BACKGROUND);
        when(mActivityManager.getPackageImportance(TEST_PACKAGE_2))
                .thenReturn(IMPORTANCE_BACKGROUND);
        assertFalse(mWorkSourceHelper.hasAnyInternalRequest());

        // add a new internal request.
        mWorkSource.add(new WorkSource(Process.WIFI_UID, "com.android.wifi"));
        assertTrue(mWorkSourceHelper.hasAnyInternalRequest());
    }
}
