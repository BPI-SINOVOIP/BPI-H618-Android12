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

package android.security.cts;

import android.platform.test.annotations.AppModeInstant;
import android.platform.test.annotations.AppModeFull;
import android.util.Log;
import android.platform.test.annotations.AsbSecurityTest;

import com.android.tradefed.testtype.DeviceJUnit4ClassRunner;
import com.android.tradefed.testtype.junit4.BaseHostJUnit4Test;
import com.android.tradefed.log.LogUtil.CLog;

import org.junit.After;
import org.junit.Assert;
import org.junit.Before;
import org.junit.Test;
import org.junit.runner.RunWith;

import static org.junit.Assert.*;
import static org.hamcrest.CoreMatchers.*;

/**
 * Test that collects test results from test package android.security.cts.CVE_2021_0481.
 *
 * When this test builds, it also builds a support APK containing
 * {@link android.sample.cts.CVE_2021_0481.SampleDeviceTest}, the results of which are
 * collected from the hostside and reported accordingly.
 */
@RunWith(DeviceJUnit4ClassRunner.class)
public class CVE_2021_0481 extends BaseHostJUnit4Test {
    private static final String TEST_PKG = "android.security.cts.CVE_2021_0481";
    private static final String TEST_CLASS = TEST_PKG + "." + "DeviceTest";
    private static final String TEST_APP = "CVE-2021-0481.apk";

    private static final String DEVICE_DIR1 = "/data/user_de/0/com.android.settings/shared_prefs/";
    private static final String DEVICE_DIR2 = "/data/user_de/0/com.android.settings/cache/";

    //defined originally as
    //private static final String TAKE_PICTURE_FILE_NAME = "TakeEditUserPhoto2.jpg";
    //in com.android.settings.users.EditUserPhotoController class
    private static final String TAKE_PICTURE_FILE_NAME = "TakeEditUserPhoto2.jpg";
    private static final String TEST_FILE_NAME = "cve_2021_0481.txt";

    @Before
    public void setUp() throws Exception {
        uninstallPackage(getDevice(), TEST_PKG);
    }

    @Test
    @AsbSecurityTest(cveBugId = 172939189)
    @AppModeFull
    public void testRunDeviceTest() throws Exception {
        AdbUtils.pushResource("/" + TEST_FILE_NAME, DEVICE_DIR1 + TEST_FILE_NAME, getDevice());
        String cmd = "rm " + DEVICE_DIR2 + TAKE_PICTURE_FILE_NAME;
        AdbUtils.runCommandLine(cmd, getDevice());

        installPackage();

        //ensure the screen is woken up.
        //(we need to do this twice. once wakes up the screen, and another unlocks the lock screen)
        getDevice().executeShellCommand("input keyevent KEYCODE_WAKEUP");
        getDevice().executeShellCommand("input keyevent KEYCODE_WAKEUP");
        Assert.assertTrue(runDeviceTests(TEST_PKG, TEST_CLASS, "testUserPhotoSetUp"));

        //Check if TEST_FILE_NAME has been copied by "Evil activity"
        //If the file has been copied then it means the vulnerability is active so the test fails.
        cmd = "cmp -s " + DEVICE_DIR1 + TEST_FILE_NAME + " " + DEVICE_DIR2 + TAKE_PICTURE_FILE_NAME + "; echo $?";
        String result =  AdbUtils.runCommandLine(cmd, getDevice()).trim();
        CLog.i(cmd + " -->" + result);
        assertThat(result, not(is("0")));
    }

    private void installPackage() throws Exception {
        installPackage(TEST_APP, new String[0]);
    }
}

