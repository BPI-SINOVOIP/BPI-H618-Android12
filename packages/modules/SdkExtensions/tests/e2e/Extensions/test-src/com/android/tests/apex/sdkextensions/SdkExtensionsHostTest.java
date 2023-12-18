/*
 * Copyright (C) 2019 The Android Open Source Project
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

package com.android.tests.apex.sdkextensions;

import static com.google.common.truth.Truth.assertWithMessage;

import static org.junit.Assert.assertEquals;
import static org.junit.Assert.assertFalse;
import static org.junit.Assert.assertNull;
import static org.junit.Assert.assertTrue;
import static org.junit.Assume.assumeTrue;

import android.cts.install.lib.host.InstallUtilsHost;

import com.android.tests.rollback.host.AbandonSessionsRule;
import com.android.tradefed.device.ITestDevice.ApexInfo;
import com.android.tradefed.testtype.DeviceJUnit4ClassRunner;
import com.android.tradefed.testtype.junit4.BaseHostJUnit4Test;
import com.android.tradefed.util.CommandResult;

import org.junit.After;
import org.junit.Before;
import org.junit.Rule;
import org.junit.Test;
import org.junit.runner.RunWith;

import java.io.File;
import java.lang.NumberFormatException;
import java.time.Duration;
import java.util.regex.Matcher;
import java.util.regex.Pattern;

@RunWith(DeviceJUnit4ClassRunner.class)
public class SdkExtensionsHostTest extends BaseHostJUnit4Test {

    private static final String APP_FILENAME = "sdkextensions_e2e_test_app.apk";
    private static final String APP_PACKAGE = "com.android.tests.apex.sdkextensions";
    private static final String APP_R12_FILENAME = "sdkextensions_e2e_test_app_req_r12.apk";
    private static final String APP_R12_PACKAGE = "com.android.tests.apex.sdkextensions.r12";
    private static final String APP_S12_FILENAME = "sdkextensions_e2e_test_app_req_s12.apk";
    private static final String APP_S12_PACKAGE = "com.android.tests.apex.sdkextensions.s12";
    private static final String APP_R45_FILENAME = "sdkextensions_e2e_test_app_req_r45.apk";
    private static final String APP_R45_PACKAGE = "com.android.tests.apex.sdkextensions.r45";
    private static final String APP_S45_FILENAME = "sdkextensions_e2e_test_app_req_s45.apk";
    private static final String APP_S45_PACKAGE = "com.android.tests.apex.sdkextensions.s45";
    private static final String MEDIA_FILENAME = "test_com.android.media.apex";
    private static final String SDKEXTENSIONS_FILENAME = "test_com.android.sdkext.apex";

    private static final Duration BOOT_COMPLETE_TIMEOUT = Duration.ofMinutes(2);

    private final InstallUtilsHost mInstallUtils = new InstallUtilsHost(this);

    private Boolean mIsAtLeastS = null;

    @Rule
    public AbandonSessionsRule mHostTestRule = new AbandonSessionsRule(this);

    @Before
    public void setUp() throws Exception {
        assumeTrue("Updating APEX is not supported", mInstallUtils.isApexUpdateSupported());
    }

    @Before
    public void installTestApp() throws Exception {
        File testAppFile = mInstallUtils.getTestFile(APP_FILENAME);
        String installResult = getDevice().installPackage(testAppFile, true);
        assertNull(installResult);
    }

    @Before // Generally not needed, but local test devices are sometimes in a "bad" start state.
    @After
    public void cleanup() throws Exception {
        getDevice().uninstallPackage(APP_PACKAGE);
        uninstallApexes(SDKEXTENSIONS_FILENAME, MEDIA_FILENAME);
    }

    @Test
    public void testDefault() throws Exception {
        assertVersionDefault();
    }

    @Test
    public void upgradeOneApexWithBump()  throws Exception {
        assertVersionDefault();
        mInstallUtils.installApexes(SDKEXTENSIONS_FILENAME);
        reboot();

        // R-Version 12 requires sdkext, which is fulfilled
        // R-Version 45 requires sdkext + media, which isn't fulfilled
        // S-Version 45 does not have any particular requirements.
        assertRVersionEquals(12);
        assertSVersionEquals(45);
        assertTrue(broadcastForBoolean("MAKE_CALLS_45", null)); // 45 APIs are available on 12 too.
    }

    @Test
    public void upgradeOneApex() throws Exception {
        // Version 45 requires updated sdkext and media, so updating just media changes nothing.
        assertVersionDefault();
        mInstallUtils.installApexes(MEDIA_FILENAME);
        reboot();
        assertVersionDefault();
    }

    @Test
    public void upgradeTwoApexes() throws Exception {
        // Updating sdkext and media bumps the version to 45.
        assertVersionDefault();
        mInstallUtils.installApexes(MEDIA_FILENAME, SDKEXTENSIONS_FILENAME);
        reboot();
        assertVersion45();
    }

    private boolean canInstallApp(String filename, String packageName) throws Exception {
        File appFile = mInstallUtils.getTestFile(filename);
        String installResult = getDevice().installPackage(appFile, true);
        if (installResult != null) {
            return false;
        }
        assertNull(getDevice().uninstallPackage(packageName));
        return true;
    }

    private String getExtensionVersionFromSysprop(String v) throws Exception {
        String command = "getprop build.version.extensions." + v;
        CommandResult res = getDevice().executeShellV2Command(command);
        assertEquals(0, (int) res.getExitCode());
        return res.getStdout().replace("\n", "");
    }

    private String broadcast(String action, String extra) throws Exception {
        String command = getBroadcastCommand(action, extra);
        CommandResult res = getDevice().executeShellV2Command(command);
        assertEquals(0, (int) res.getExitCode());
        Matcher matcher = Pattern.compile("data=\"([^\"]+)\"").matcher(res.getStdout());
        assertTrue("Unexpected output from am broadcast: " + res.getStdout(), matcher.find());
        return matcher.group(1);
    }

    private boolean broadcastForBoolean(String action, String extra) throws Exception {
        String result = broadcast(action, extra);
        if (result.equals("true") || result.equals("false")) {
            return result.equals("true");
        }
        throw getAppParsingError(result);
    }

    private int broadcastForInt(String action, String extra) throws Exception {
        String result = broadcast(action, extra);
        try {
            return Integer.parseInt(result);
        } catch (NumberFormatException e) {
            throw getAppParsingError(result);
        }
    }

    private Error getAppParsingError(String result) {
        String message = "App error! Full stack trace in logcat (grep for SdkExtensionsE2E): ";
        return new AssertionError(message + result);
    }

    private void assertVersionDefault() throws Exception {
        assertRVersionEquals(1);
        assertSVersionEquals(1);
        assertTrue(broadcastForBoolean("MAKE_CALLS_DEFAULT", null));
    }

    private void assertVersion45() throws Exception {
        assertRVersionEquals(45);
        assertSVersionEquals(45);
        assertTrue(broadcastForBoolean("MAKE_CALLS_45", null));
    }

    private void assertRVersionEquals(int version) throws Exception {
        int appValue = broadcastForInt("GET_SDK_VERSION", "r");
        String syspropValue = getExtensionVersionFromSysprop("r");
        assertEquals(version, appValue);
        assertEquals(String.valueOf(version), syspropValue);
        assertEquals(version >= 12, canInstallApp(APP_R12_FILENAME, APP_R12_PACKAGE));
        assertEquals(version >= 45, canInstallApp(APP_R45_FILENAME, APP_R45_PACKAGE));
    }

    private void assertSVersionEquals(int version) throws Exception {
        int appValue = broadcastForInt("GET_SDK_VERSION", "s");
        String syspropValue = getExtensionVersionFromSysprop("s");
        if (isAtLeastS()) {
            assertEquals(version, appValue);
            assertEquals(String.valueOf(version), syspropValue);

            // These APKs require the same R version as they do S version.
            int minVersion = Math.min(version, broadcastForInt("GET_SDK_VERSION", "r"));
            assertEquals(minVersion >= 12, canInstallApp(APP_S12_FILENAME, APP_S12_PACKAGE));
            assertEquals(minVersion >= 45, canInstallApp(APP_S45_FILENAME, APP_S45_PACKAGE));
        } else {
            assertEquals(0, appValue);
            assertEquals("", syspropValue);
            assertFalse(canInstallApp(APP_S12_FILENAME, APP_S12_PACKAGE));
            assertFalse(canInstallApp(APP_S45_FILENAME, APP_S45_PACKAGE));
        }
    }

    private static String getBroadcastCommand(String action, String extra) {
        String cmd = "am broadcast";
        cmd += " -a com.android.tests.apex.sdkextensions." + action;
        if (extra != null) {
            cmd += " -e extra " + extra;
        }
        cmd += " -n com.android.tests.apex.sdkextensions/.Receiver";
        return cmd;
    }

    private boolean isAtLeastS() throws Exception {
        if (mIsAtLeastS == null) {
            mIsAtLeastS = broadcastForBoolean("IS_AT_LEAST", "s");
        }
        return mIsAtLeastS;
    }

    private boolean uninstallApexes(String... filenames) throws Exception {
        boolean reboot = false;
        for (String filename : filenames) {
            ApexInfo apex = mInstallUtils.getApexInfo(mInstallUtils.getTestFile(filename));
            String res = getDevice().uninstallPackage(apex.name);
            // res is null for successful uninstalls (non-null likely implesfactory version).
            reboot |= res == null;
        }
        if (reboot) {
            reboot();
            return true;
        }
        return false;
    }

    private void reboot() throws Exception {
        getDevice().reboot();
        boolean success = getDevice().waitForBootComplete(BOOT_COMPLETE_TIMEOUT.toMillis());
        assertWithMessage("Device didn't boot in %s", BOOT_COMPLETE_TIMEOUT).that(success).isTrue();
    }
}
