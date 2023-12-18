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

package com.android.server.cts.device.batterystats;

import static com.android.compatibility.common.util.SystemUtil.runShellCommand;

import static junit.framework.Assert.assertTrue;

import static org.testng.Assert.assertThrows;

import android.app.Instrumentation;
import android.app.UiAutomation;
import android.content.Context;
import android.net.ConnectivityManager;
import android.net.NetworkCapabilities;
import android.os.BatteryStatsManager;
import android.os.SystemClock;
import android.system.Os;

import androidx.test.InstrumentationRegistry;
import androidx.test.runner.AndroidJUnit4;

import org.junit.Before;
import org.junit.Test;
import org.junit.runner.RunWith;

import java.util.regex.Pattern;

@RunWith(AndroidJUnit4.class)
public class BatteryStatsRadioPowerStateTest {
    private static final int UID = Os.getuid();
    private static final String PATTERN_MOBILE = "modem-data";
    private static final String PATTERN_WIFI = "wifi-data";
    private BatteryStatsManager mBsm;
    private ConnectivityManager mCm;
    private Instrumentation mInstrumentation;
    private Context mContext;
    private UiAutomation mUiAutomation;

    @Before
    public void setUp() throws Exception {
        mInstrumentation = InstrumentationRegistry.getInstrumentation();
        mContext = mInstrumentation.getContext();
        mUiAutomation =  mInstrumentation.getUiAutomation();
        mBsm = mContext.getSystemService(BatteryStatsManager.class);
        mCm = mContext.getSystemService(ConnectivityManager.class);
    }

    @Test
    public void testReportMobileRadioPowerState() throws Exception {
        // Expect fail w/o UPDATE_DEVICE_STATS permission.
        assertThrows(SecurityException.class, () -> mBsm.reportMobileRadioPowerState(true, UID));

        mUiAutomation.adoptShellPermissionIdentity();
        final NetworkCapabilities activeNc = mCm.getNetworkCapabilities(mCm.getActiveNetwork());
        final boolean expectedCurrentMobileRadio = (activeNc == null) ? false :
                activeNc.hasTransport(NetworkCapabilities.TRANSPORT_CELLULAR);
        try {
            // Skip other information before testing to reduce the information contained in the dump
            // result.
            final long time = SystemClock.elapsedRealtime();
            final String cmd = "dumpsys batterystats --history-start " + time;
            // Mobile radio power stats only updates when:
            // 1. the radio state is changed, and
            // 2. the radio power is inactive.
            // Thus, trigger twice with different radio power state to ensure that it updates.
            mBsm.reportMobileRadioPowerState(!expectedCurrentMobileRadio, UID);
            mBsm.reportMobileRadioPowerState(expectedCurrentMobileRadio, UID);
            final String dump = runShellCommand(mInstrumentation, cmd);
            assertTrue(Pattern.compile(PATTERN_MOBILE).matcher(dump).find());
        } finally {
            // Restore state
            mBsm.reportMobileRadioPowerState(expectedCurrentMobileRadio, UID);
            mUiAutomation.dropShellPermissionIdentity();
        }
    }

    @Test
    public void testReportWifiRadioPowerState() throws Exception {
        // Expect fail w/o UPDATE_DEVICE_STATS permission.
        assertThrows(SecurityException.class, () -> mBsm.reportWifiRadioPowerState(true, UID));

        mUiAutomation.adoptShellPermissionIdentity();
        final NetworkCapabilities activeNc = mCm.getNetworkCapabilities(mCm.getActiveNetwork());
        final boolean expectedCurrentWifiRadio = (activeNc == null) ? false :
                activeNc.hasTransport(NetworkCapabilities.TRANSPORT_WIFI);
        try {
            // Skip other information before testing to reduce the information contained in the dump
            // result.
            final long time = SystemClock.elapsedRealtime();
            final String cmd = "dumpsys batterystats --history-start " + time;
            mBsm.reportWifiRadioPowerState(!expectedCurrentWifiRadio, UID);
            final String dump = runShellCommand(mInstrumentation, cmd);
            assertTrue(Pattern.compile(PATTERN_WIFI).matcher(dump).find());
        } finally {
            // Restore state
            mBsm.reportWifiRadioPowerState(expectedCurrentWifiRadio, UID);
            mUiAutomation.dropShellPermissionIdentity();
        }
    }
}
