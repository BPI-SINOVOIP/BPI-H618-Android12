/**
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

import android.platform.test.annotations.AsbSecurityTest;
import com.android.tradefed.device.ITestDevice;
import com.android.tradefed.testtype.DeviceJUnit4ClassRunner;
import java.util.regex.Pattern;
import java.util.regex.Matcher;
import org.junit.Test;
import org.junit.runner.RunWith;

import static org.hamcrest.core.Is.is;
import static org.junit.Assert.assertThat;

@RunWith(DeviceJUnit4ClassRunner.class)
public class CVE_2021_0523 extends SecurityTestCase {

    private static void extractInt(String str, int[] displaySize) {
        str = ((str.replaceAll("[^\\d]", " ")).trim()).replaceAll(" +", " ");
        if (str.equals("")) {
            return;
        }
        String s[] = str.split(" ");
        for (int i = 0; i < s.length; ++i) {
            displaySize[i] = Integer.parseInt(s[i]);
        }
    }

    /**
     * b/174047492
     */
    @Test
    @AsbSecurityTest(cveBugId = 174047492)
    public void testPocCVE_2021_0523() throws Exception {
        final int SLEEP_INTERVAL_MILLISEC = 30 * 1000;
        String apkName = "CVE-2021-0523.apk";
        String appPath = AdbUtils.TMP_PATH + apkName;
        String packageName = "android.security.cts.cve_2021_0523";
        String crashPattern =
            "Device is vulnerable to b/174047492 hence any app with " +
            "SYSTEM_ALERT_WINDOW can overlay the WifiScanModeActivity screen";
        ITestDevice device = getDevice();

        try {
            /* Push the app to /data/local/tmp */
            pocPusher.appendBitness(false);
            pocPusher.pushFile(apkName, appPath);

            /* Wake up the screen */
            AdbUtils.runCommandLine("input keyevent KEYCODE_WAKEUP", device);
            AdbUtils.runCommandLine("input keyevent KEYCODE_MENU", device);
            AdbUtils.runCommandLine("input keyevent KEYCODE_HOME", device);

            /* Install the application */
            AdbUtils.runCommandLine("pm install " + appPath, device);

            /* Grant "Draw over other apps" permission */
            AdbUtils.runCommandLine(
                    "pm grant " + packageName + " android.permission.SYSTEM_ALERT_WINDOW", device);

            /* Start the application */
            AdbUtils.runCommandLine("am start -n " + packageName + "/.PocActivity", getDevice());
            Thread.sleep(SLEEP_INTERVAL_MILLISEC);

            /* Get screen width and height */
            int[] displaySize = new int[2];
            extractInt(AdbUtils.runCommandLine("wm size", device), displaySize);
            int width = displaySize[0];
            int height = displaySize[1];

            /* Give a tap command for center of screen */
            AdbUtils.runCommandLine("input tap " + width / 2 + " " + height / 2, device);
        } catch (Exception e) {
            e.printStackTrace();
        } finally {
            /* Un-install the app after the test */
            AdbUtils.runCommandLine("pm uninstall " + packageName, device);

            /* Detection of crash pattern in the logs */
            String logcat = AdbUtils.runCommandLine("logcat -d *:S AndroidRuntime:E", device);
            Pattern pattern = Pattern.compile(crashPattern, Pattern.MULTILINE);
            assertThat(crashPattern, pattern.matcher(logcat).find(), is(false));
        }
    }
}
