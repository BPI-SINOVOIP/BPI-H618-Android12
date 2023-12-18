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

package com.android.helpers.tests;

import static org.junit.Assert.assertFalse;
import static org.junit.Assert.assertTrue;
import static org.mockito.ArgumentMatchers.matches;
import static org.mockito.Mockito.doReturn;

import androidx.test.InstrumentationRegistry;
import androidx.test.runner.AndroidJUnit4;
import androidx.test.uiautomator.UiDevice;
import com.android.helpers.PinnerHelper;

import java.io.IOException;
import java.util.List;
import java.util.Map;
import java.util.Set;
import java.util.stream.Collectors;

import org.junit.Before;
import org.junit.After;
import org.junit.Test;
import org.junit.runner.RunWith;
import org.mockito.MockitoAnnotations;
import org.mockito.Spy;

/**
 * Android Unit tests for {@link PinnerHelper}.
 *
 * To run:
 * atest CollectorsHelperTest:com.android.helpers.tests.PinnerHelperTest
 */
@RunWith(AndroidJUnit4.class)
public class PinnerHelperTest {
    private static final String TAG = PinnerHelperTest.class.getSimpleName();
    private static final String HOME_APP_PREFIX = "pinner_Home_";
    // Valid output file
    private static final String VALID_OUTPUT_DIR = "/sdcard/test_results";
    // Invalid output file (no permissions to write)
    private static final String INVALID_OUTPUT_DIR = "/data/local/tmp";
    private static final String REMOVE_CMD = "rm -rf %s";

    private PinnerHelper mPinnerHelper;
    private @Spy PinnerHelper mMockedPinnerHelper;

    @Before
    public void setUp() {
        mPinnerHelper = new PinnerHelper();
        MockitoAnnotations.initMocks(this);
    }

    @After
    public void tearDown() throws IOException {
        UiDevice uiDevice = UiDevice.getInstance(InstrumentationRegistry.getInstrumentation());
        uiDevice.executeShellCommand(String.format(REMOVE_CMD, "/sdcard/test_results"));
    }

    /**
     * Test start collecting returns false if the helper has not been properly set up.
     */
    @Test
    public void testSetUpNotCalled() {
        assertFalse(mPinnerHelper.startCollecting());
    }

    /**
     * Test writing file to a valid output directory.
     */
    @Test
    public void testValidFile() {
        mPinnerHelper.setUp(VALID_OUTPUT_DIR);
        assertTrue(mPinnerHelper.startCollecting());
    }

    /**
     * Test writing file to a invalid directory.
     */
    @Test
    public void testInvalidFile() {
        mPinnerHelper.setUp(INVALID_OUTPUT_DIR);
        assertFalse(mPinnerHelper.startCollecting());
    }

    /**
     * Test the pinner snapshot file is added in the result map.
     */
    @Test
    public void testPinnerSnapshotDumpFile() {
        mPinnerHelper.setUp(VALID_OUTPUT_DIR);
        assertTrue(mPinnerHelper.startCollecting());
        Map<String, String> metrics = mPinnerHelper.getMetrics();
        assertTrue(metrics.size() > 2);
        assertTrue(metrics.containsKey(PinnerHelper.OUTPUT_FILE_PATH_KEY));
    }

    /**
     * Test the total pinned files size is added in the result map.
     */
    @Test
    public void testPinnerTotalFileSizeMetric() {
        mPinnerHelper.setUp(VALID_OUTPUT_DIR);
        assertTrue(mPinnerHelper.startCollecting());
        Map<String, String> metrics = mPinnerHelper.getMetrics();
        assertTrue(metrics.size() > 2);
        assertTrue(metrics.containsKey(PinnerHelper.TOTAL_SIZE_BYTES_KEY));
    }

    /**
     * Test at least one pinned system file is present
     */
    @Test
    public void testSystemFileSizeMetrics() {
        mPinnerHelper.setUp(VALID_OUTPUT_DIR);
        assertTrue(mPinnerHelper.startCollecting());
        Map<String, String> metrics = mPinnerHelper.getMetrics();
        List<String> systemMetricKeys = metrics.keySet()
                .stream()
                .filter(s -> s.startsWith(PinnerHelper.SYSTEM_HEADER_NAME + "_"))
                .collect(Collectors.toList());
        assertTrue(systemMetricKeys.size() > 1);
    }

    /**
     * Test at least one individual app pinned system file is present.
     */
    @Test
    public void testIndividualAppFileSizeMetrics() {
        mPinnerHelper.setUp(VALID_OUTPUT_DIR);
        assertTrue(mPinnerHelper.startCollecting());
        Map<String, String> metrics = mPinnerHelper.getMetrics();
        List<String> appPinnedFilesKeys = metrics.keySet()
                .stream()
                .filter(s -> s.startsWith(HOME_APP_PREFIX))
                .collect(Collectors.toList());
        assertTrue(appPinnedFilesKeys.size() > 1);
    }

    /**
     * Test total file size count is sum of individual app level and system file size counts.
     */
    @Test
    public void testValidateTotalFileSizeCount() {
        mPinnerHelper.setUp(VALID_OUTPUT_DIR);
        assertTrue(mPinnerHelper.startCollecting());
        Map<String, String> metrics = mPinnerHelper.getMetrics();
        Set<String> appLevelFilesCount = metrics.keySet()
                .stream()
                .filter(s -> (s.endsWith(PinnerHelper.PINNER_FILES_COUNT_SUFFIX)
                        && !s.startsWith(PinnerHelper.TOTAL_FILE_COUNT_KEY)))
                .collect(Collectors.toSet());
        int totalFilesCount = metrics.entrySet().stream().filter(
                s -> (s.getKey().endsWith(PinnerHelper.PINNER_FILES_COUNT_SUFFIX) && !s.getKey()
                        .startsWith(PinnerHelper.TOTAL_FILE_COUNT_KEY)))
                .map(e -> e.getValue()).mapToInt(Integer::valueOf).sum();
        assertTrue(
                totalFilesCount == Integer
                        .parseInt(metrics.get(PinnerHelper.TOTAL_FILE_COUNT_KEY)));
    }

    /**
     * Test the parser works if the pinner dump contains only the system metrics.
     */
    @Test
    public void testOnlySystemPinnerInfoIsPresent() throws IOException {
        String pinnerSampleOutput = "/system/framework/framework.jar 35004416\n" +
                "/system/framework/arm64/boot-framework.vdex 323584\n" +
                "/system/framework/arm64/boot-framework.oat 9003008\n" +
                "/system/framework/services.jar 18300928\n" +
                "/system/framework/oat/arm64/services.vdex 208896\n" +
                "/system/framework/oat/arm64/services.odex 39137280\n" +
                "/apex/com.android.art/javalib/core-oj.jar 5107712\n" +
                "/apex/com.android.art/javalib/arm64/boot.vdex 49152\n" +
                "/apex/com.android.art/javalib/arm64/boot.oat 4132864\n" +
                "/apex/com.android.art/javalib/core-libart.jar 552960\n" +
                "/apex/com.android.art/javalib/arm64/boot-core-libart.vdex 8192\n" +
                "/apex/com.android.art/javalib/arm64/boot-core-libart.oat 385024\n" +
                "/apex/com.android.media/javalib/updatable-media.jar 499712\n" +
                "/system_ext/priv-app/SystemUIGoogle/SystemUIGoogle.apk 48803840\n" +
                "/system_ext/priv-app/SystemUIGoogle/oat/arm64/SystemUIGoogle.vdex 319488\n" +
                "/system_ext/priv-app/SystemUIGoogle/oat/arm64/SystemUIGoogle.odex 26501120\n" +
                "/system/bin/surfaceflinger 7098368\n"
                + "Total size: 202407936";
        doReturn(pinnerSampleOutput).when(mMockedPinnerHelper)
                .executeShellCommand(matches(PinnerHelper.PINNER_CMD));
        Map<String, String> metrics = mMockedPinnerHelper.getMetrics();
        assertTrue(metrics.containsKey(PinnerHelper.TOTAL_SIZE_BYTES_KEY));
        List<String> systemMetricKeys = metrics.keySet()
                .stream()
                .filter(s -> s.startsWith(PinnerHelper.SYSTEM_HEADER_NAME + "_"))
                .collect(Collectors.toList());
        assertTrue(systemMetricKeys.size() > 1);
        List<String> appPinnedFilesKeys = metrics.keySet()
                .stream()
                .filter(s -> s.startsWith(HOME_APP_PREFIX))
                .collect(Collectors.toList());
        assertTrue(appPinnedFilesKeys.size() == 0);
    }

    /**
     * Test the parser works if the pinner dump contains only the app level metrics.
     */
    @Test
    public void testOnlyAppPinnerMetricsInfoIsPresent() throws IOException {
        String pinnerSampleOutput = "Home uid=10217 active=true\n" +
                "  /system_ext/priv-app/NexusLauncherRelease/NexusLauncherRelease.apk 6291456\n" +
                "  /data/NexusLauncherRelease.apk@classes.vdex 126976\n" +
                "  /data/NexusLauncherRelease@NexusLauncherRelease.apk@classes.dex 552960\n"
                + "Total size: 202407936";
        doReturn(pinnerSampleOutput).when(mMockedPinnerHelper)
                .executeShellCommand(matches(PinnerHelper.PINNER_CMD));
        Map<String, String> metrics = mMockedPinnerHelper.getMetrics();
        List<String> appPinnedFilesKeys = metrics.keySet()
                .stream()
                .filter(s -> s.startsWith(HOME_APP_PREFIX))
                .collect(Collectors.toList());
        assertTrue(appPinnedFilesKeys.size() > 1);
        List<String> systemMetricKeys = metrics.keySet()
                .stream()
                .filter(s -> (s.startsWith(PinnerHelper.SYSTEM_HEADER_NAME + "_")
                        && s.endsWith("_bytes")))
                .collect(Collectors.toList());
        assertTrue(systemMetricKeys.size() == 0);
        assertTrue(metrics.containsKey(PinnerHelper.TOTAL_SIZE_BYTES_KEY));
    }

    /**
     * Test result metric map is reset in between multiple get metrics from device methods calls.
     */
    @Test
    public void testMetricMapResetOnMultipleGetMetrics() throws IOException {
        String pinnerSampleOutput = "/system/framework/framework.jar 35004416\n" +
                "/system/framework/arm64/boot-framework.vdex 323584\n" +
                "/system/framework/arm64/boot-framework.oat 9003008\n" +
                "/system/framework/services.jar 18300928\n" +
                "/system/framework/oat/arm64/services.vdex 208896\n" +
                "/system/framework/oat/arm64/services.odex 39137280\n" +
                "/apex/com.android.art/javalib/core-oj.jar 5107712\n" +
                "/apex/com.android.art/javalib/arm64/boot.vdex 49152\n" +
                "/apex/com.android.art/javalib/arm64/boot.oat 4132864\n" +
                "/apex/com.android.art/javalib/core-libart.jar 552960\n" +
                "/apex/com.android.art/javalib/arm64/boot-core-libart.vdex 8192\n" +
                "/apex/com.android.art/javalib/arm64/boot-core-libart.oat 385024\n" +
                "/apex/com.android.media/javalib/updatable-media.jar 499712\n" +
                "/system_ext/priv-app/SystemUIGoogle/SystemUIGoogle.apk 48803840\n" +
                "/system_ext/priv-app/SystemUIGoogle/oat/arm64/SystemUIGoogle.vdex 319488\n" +
                "/system_ext/priv-app/SystemUIGoogle/oat/arm64/SystemUIGoogle.odex 26501120\n" +
                "/system/bin/surfaceflinger 7098368\n"
                + "Total size: 202407936";
        doReturn(pinnerSampleOutput).when(mMockedPinnerHelper)
                .executeShellCommand(matches(PinnerHelper.PINNER_CMD));
        Map<String, String> metrics = mMockedPinnerHelper.getMetrics();
        List<String> systemMetricKeys = metrics.keySet()
                .stream()
                .filter(s -> s.startsWith(PinnerHelper.SYSTEM_HEADER_NAME + "_"))
                .collect(Collectors.toList());
        assertTrue(systemMetricKeys.size() > 1);

        String pinnerSampleOutput2 = "Home uid=10217 active=true\n" +
                "  /system_ext/priv-app/NexusLauncherRelease/NexusLauncherRelease.apk 6291456\n" +
                "  /data/NexusLauncherRelease.apk@classes.vdex 126976\n" +
                "  /data/NexusLauncherRelease@NexusLauncherRelease.apk@classes.dex 552960\n" +
                "Total size: 202407936";
        doReturn(pinnerSampleOutput2).when(mMockedPinnerHelper)
                .executeShellCommand(matches(PinnerHelper.PINNER_CMD));
        Map<String, String> metrics2 = mMockedPinnerHelper.getMetrics();
        List<String> appPinnedFilesKeys2 = metrics2.keySet()
                .stream()
                .filter(s -> s.startsWith(HOME_APP_PREFIX))
                .collect(Collectors.toList());
        assertTrue(appPinnedFilesKeys2.size() > 1);
        List<String> systemMetricKeys2 = metrics2.keySet()
                .stream()
                .filter(s -> (s.startsWith(PinnerHelper.SYSTEM_HEADER_NAME + "_")
                        && s.endsWith("_bytes")))
                .collect(Collectors.toList());
        assertTrue(systemMetricKeys2.size() == 0);
        assertTrue(metrics.containsKey(PinnerHelper.TOTAL_SIZE_BYTES_KEY));
    }
}
