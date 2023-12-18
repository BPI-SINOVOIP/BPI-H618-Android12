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

package com.android.gki.tests;

import static org.hamcrest.Matchers.anyOf;
import static org.hamcrest.Matchers.containsString;
import static org.hamcrest.Matchers.either;
import static org.hamcrest.Matchers.everyItem;
import static org.hamcrest.Matchers.greaterThan;
import static org.hamcrest.Matchers.is;
import static org.hamcrest.Matchers.isIn;
import static org.hamcrest.Matchers.notNullValue;
import static org.hamcrest.io.FileMatchers.aFileWithSize;
import static org.junit.Assert.assertNotNull;
import static org.junit.Assert.assertNull;
import static org.junit.Assert.assertThat;
import static org.junit.Assert.assertTrue;
import static org.junit.Assume.assumeThat;
import static org.junit.Assert.fail;
import static org.junit.Assume.assumeTrue;

import static java.util.stream.Collectors.toList;

import android.cts.host.utils.DeviceJUnit4ClassRunnerWithParameters;
import android.cts.host.utils.DeviceJUnit4Parameterized;

import com.android.compatibility.common.tradefed.build.CompatibilityBuildHelper;
import com.android.tradefed.device.ITestDevice;
import com.android.tradefed.device.ITestDevice.ApexInfo;
import com.android.tradefed.log.LogUtil.CLog;
import com.android.tradefed.testtype.junit4.BaseHostJUnit4Test;

import org.junit.After;
import org.junit.Before;
import org.junit.Test;
import org.junit.runner.RunWith;
import org.junit.runners.Parameterized.Parameter;
import org.junit.runners.Parameterized.Parameters;
import org.junit.runners.Parameterized.UseParametersRunnerFactory;

import java.nio.file.Path;
import java.nio.file.Paths;
import java.io.File;
import java.util.ArrayList;
import java.util.HashSet;
import java.util.List;
import java.util.Set;
import java.util.Scanner;
import java.util.regex.Matcher;
import java.util.regex.Pattern;

@RunWith(DeviceJUnit4Parameterized.class)
@UseParametersRunnerFactory(DeviceJUnit4ClassRunnerWithParameters.RunnerFactory.class)
public class GkiInstallTest extends BaseHostJUnit4Test {

    // Keep in sync with gki.go.
    private static final String HIGH_SUFFIX = "_test_high.apex";
    private static final String LOW_SUFFIX = "_test_low.apex";
    private static final long TEST_HIGH_VERSION = 1000000000L;

    // Timeout between device online for adb commands and boot completed flag is set.
    private static final long DEVICE_AVAIL_TIMEOUT_MS = 180000; // 3mins
    // Timeout for `adb install`.
    private static final long INSTALL_TIMEOUT_MS = 600000; // 10mins

    @Parameter
    public String mFileName;

    private String mPackageName;
    private File mApexFile;
    private boolean mExpectInstallSuccess;
    private final Set<String> mOverlayfs = new HashSet();

    @Parameters(name = "{0}")
    public static Iterable<String> getTestFileNames() {
        try (Scanner scanner = new Scanner(
                GkiInstallTest.class.getClassLoader().getResourceAsStream(
                        "gki_install_test_file_list.txt"))) {
            List<String> list = new ArrayList<>();
            scanner.forEachRemaining(list::add);
            return list;
        }
    }

    @Before
    public void setUp() throws Exception {
        inferPackageName();
        skipTestIfPackageNotInstalled();
        skipTestIfWrongKernelVersion();
        findTestApexFile();
        prepareOverlayfs();
    }

    /** Set mPackageName and mExpectInstallSuccess according to mFileName. */
    private void inferPackageName() throws Exception {
        if (mFileName.endsWith(HIGH_SUFFIX)) {
            mPackageName = mFileName.substring(0, mFileName.length() - HIGH_SUFFIX.length());
            mExpectInstallSuccess = true;
        } else if (mFileName.endsWith(LOW_SUFFIX)) {
            mPackageName = mFileName.substring(0, mFileName.length() - LOW_SUFFIX.length());
            mExpectInstallSuccess = false;
        } else {
            fail("Unrecognized test data file: " + mFileName);
        }
    }

    /** Skip the test if mPackageName is not installed on the device. */
    private void skipTestIfPackageNotInstalled() throws Exception {
        CLog.i("Wait for device to be available for %d ms...", DEVICE_AVAIL_TIMEOUT_MS);
        getDevice().waitForDeviceAvailable(DEVICE_AVAIL_TIMEOUT_MS);
        CLog.i("Device is available after %d ms", DEVICE_AVAIL_TIMEOUT_MS);

        // Skip if the device does not support this APEX package.
        CLog.i("Checking if %s is installed on the device.", mPackageName);
        ApexInfo oldApexInfo = getApexInfo(getDevice(), mPackageName);
        assumeThat(oldApexInfo, is(notNullValue()));
        assumeThat(oldApexInfo.name, is(mPackageName));
    }

    /**
     * Skip the test if APEX package name does not match kernel version.
     *
     * Due to b/186566367, on mixed builds, the wrong GKI APEX may be installed. In that case, just
     * skip the test.
     *
     * As an exception, the package may contain "unstable" as the generation. When this is the
     * case, any generation number in kernel release is considered a match.
     *
     * @throws Exception
     */
    private void skipTestIfWrongKernelVersion() throws Exception {
        Pattern packagePattern = Pattern.compile(
                "^com\\.android\\.gki\\.kmi_(?<w>\\d+)_(?<x>\\d+)_(?<z>android\\d+)_" +
                "(?<k>\\d+|unstable)$");
        Matcher packageMatcher = packagePattern.matcher(mPackageName);
        assertTrue(packageMatcher.matches());

        Pattern kernelPattern = Pattern.compile(
                "^Linux version (?<fullrel>(?<w>\\d+)\\.(?<x>\\d+)\\.(?<y>\\d+)-(?<z>android\\d+)"
                        + "-(?<k>\\d+))");
        String kernel = getDevice().executeShellCommand("cat /proc/version");
        Matcher kernelMatcher = kernelPattern.matcher(kernel);
        assumeTrue("Not GKI: " + kernel, kernelMatcher.find());

        String desc = String.format("package %s vs kernel release %s", mPackageName,
                kernelMatcher.group("fullrel"));

        CLog.i("Checking: %s", desc);

        assumeThat(desc, packageMatcher.group("w"), is(kernelMatcher.group("w")));
        assumeThat(desc, packageMatcher.group("x"), is(kernelMatcher.group("x")));
        assumeThat(desc, packageMatcher.group("z"), is(kernelMatcher.group("z")));
        assumeThat(desc, packageMatcher.group("k"),
                anyOf(is("unstable"), is(kernelMatcher.group("k"))));
    }

    /** Find the corresponding APEX test file with mFileName. */
    private void findTestApexFile() throws Exception {
        // Find the APEX file.
        CompatibilityBuildHelper buildHelper = new CompatibilityBuildHelper(getBuild());
        mApexFile = buildHelper.getTestFile(mFileName);

        // There may be empty .apex files in the directory for disabled APEXes. But if the device
        // is known to install the package, the test must be built with non-empty APEXes for this
        // particular package.
        assertThat("Test is not built properly. It does not contain a non-empty " + mFileName,
                mApexFile, is(aFileWithSize(greaterThan(0L))));
    }

    /**
     * Record what partitions have overlayfs set up. Then, tear down overlayfs because it may
     * make OTA fail.
     *
     * Usually, the test does not require root to run, but if the device has overlayfs set up,
     * the test assumes that the device has root functionality, and attempts to tear down
     * overlayfs before the test starts.
     * Note that this function immediately reboots after enabling adb root to ensure the test runs
     * with the same permission before it is called.
     */
    private void prepareOverlayfs() throws Exception {
        mOverlayfs.addAll(getOverlayfsState(getDevice()));

        if (!mOverlayfs.isEmpty()) {
            getDevice().enableAdbRoot();
            getDevice().executeAdbCommand("enable-verity");
            rebootUntilAvailable(getDevice(), DEVICE_AVAIL_TIMEOUT_MS);
        }
    }

    @Test
    public void testInstallAndReboot() throws Exception {
        CLog.i("Installing %s with %d ms timeout", mApexFile, INSTALL_TIMEOUT_MS);
        String result = getDevice().installPackage(mApexFile, false,
                "--staged-ready-timeout", String.valueOf(INSTALL_TIMEOUT_MS));
        if (!mExpectInstallSuccess) {
            assertNotNull("Should not be able to install downgrade package", result);
            assertThat(result, either(containsString("Downgrade of APEX package " + mPackageName
                    + " is not allowed.")).or(containsString("INSTALL_FAILED_VERSION_DOWNGRADE")));
            return;
        }

        assertNull("Installation failed with " + result, result);
        rebootUntilAvailable(getDevice(), DEVICE_AVAIL_TIMEOUT_MS);

        ApexInfo newApexInfo = getApexInfo(getDevice(), mPackageName);
        assertNotNull(newApexInfo);
        assertThat(newApexInfo.versionCode, is(TEST_HIGH_VERSION));
    }

    /**
     * Restore overlayfs on partitions.
     *
     * Usually, tearDown() does not require root to run, but if the device had overlayfs set up
     * before the test has started,
     * the test assumes that the device has root functionality, and attempts to re-set up
     * overlayfs after the test ends.
     * Note that tearDown() immediately reboots after enabling adb root to ensure the test ends up
     * with the same permission before the test has started.
     */
    @After
    public void tearDown() throws Exception {
        // Restore overlayfs for partitions that the test knows of.
        CLog.i("Test ends, now restoring overlayfs partitions %s.", mOverlayfs);
        if (mOverlayfs.contains("system")) {
            getDevice().enableAdbRoot();
            getDevice().remountSystemWritable();
        }
        if (mOverlayfs.contains("vendor")) {
            getDevice().enableAdbRoot();
            getDevice().remountVendorWritable();
        }
        CLog.i("Restoring overlayfs partition ends, now rebooting.");

        // Reboot device no matter what to avoid interference.
        rebootUntilAvailable(getDevice(), DEVICE_AVAIL_TIMEOUT_MS);

        // remount*Writable should have enabled overlayfs for all necessary partitions. If not,
        // throw an error.
        Set<String> newOverlayfsState = getOverlayfsState(getDevice());
        assertThat("Some partitions did not restore overlayfs properly. Before test: " + mOverlayfs
                        + ", after test: " + newOverlayfsState, mOverlayfs,
                everyItem(isIn(newOverlayfsState)));
        CLog.i("All overlayfs states are restored.");
    }

    /**
     * @param device the device under test
     * @param packageName the package name to look for
     * @return The {@link ApexInfo} of the APEX named {@code packageName} on the
     * {@code device}, or {@code null} if the device does not have the APEX installed.
     * @throws Exception an error has occurred.
     */
    private static ApexInfo getApexInfo(ITestDevice device, String packageName)
            throws Exception {
        assertNotNull(packageName);
        List<ApexInfo> list = device.getActiveApexes().stream().filter(
                apexInfo -> packageName.equals(apexInfo.name)).collect(toList());
        if (list.isEmpty()) return null;
        assertThat(list.size(), is(1));
        return list.get(0);
    }

    /**
     * Similar to device.reboot(), but with a timeout on waitForDeviceAvailable. Note that
     * the timeout does not include the rebootUntilOnline() call.
     *
     * @param device    the device under test
     * @param timeoutMs timeout for waitForDeviceAvailable() call
     * @throws Exception an error has occurred.
     */
    private static void rebootUntilAvailable(ITestDevice device, long timeoutMs)
            throws Exception {
        CLog.i("Reboot and waiting for device to be online");
        device.rebootUntilOnline();
        CLog.i("Device online, wait for device to be available for %d ms...", timeoutMs);
        device.waitForDeviceAvailable(timeoutMs);
        CLog.i("Device is available after %d ms", timeoutMs);
    }
    
    /**
     * Get all partitions that have overlayfs setup. Parse /proc/mounts and if it finds lines like:
     * {@code overlayfs /vendor ...}, then put {@code vendor} in the returned set.
     * @param device the device under test
     * @return a list of partitions like {@code system}, {@code vendor} that has overlayfs set up
     * @throws Exception an error has occurred.
     */
    private static Set<String> getOverlayfsState(ITestDevice device) throws Exception {
        Set<String> ret = new HashSet();
        File mounts = device.pullFile("/proc/mounts");
        try (Scanner scanner = new Scanner(mounts)) {
            while (scanner.hasNextLine()) {
                String line = scanner.nextLine();
                String[] tokens = line.split("\\s");
                if (tokens.length < 2) continue;
                if (!"overlay".equals(tokens[0])) continue;
                Path path = Paths.get(tokens[1]);
                if (path.getNameCount() == 0) continue;
                ret.add(path.getName(0).toString());
            }
        }
        CLog.i("Device has overlayfs set up on partitions %s", ret);
        return ret;
    }
}
