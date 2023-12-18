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

package android.bugreport.cts;

import static com.android.compatibility.common.util.SystemUtil.runShellCommand;

import static com.google.common.truth.Truth.assertThat;

import android.content.BroadcastReceiver;
import android.content.Context;
import android.content.Intent;
import android.content.IntentFilter;
import android.content.pm.PackageManager;
import android.os.BugreportManager;
import android.os.BugreportParams;
import android.os.UserManager;
import android.util.Pair;

import androidx.test.InstrumentationRegistry;
import androidx.test.filters.LargeTest;
import androidx.test.runner.AndroidJUnit4;

import org.junit.After;
import org.junit.Before;
import org.junit.Test;
import static org.junit.Assume.assumeFalse;
import org.junit.runner.RunWith;

import java.util.concurrent.CountDownLatch;
import java.util.concurrent.TimeUnit;

/**
 * Device-side tests for Bugreport Manager API
 */
@RunWith(AndroidJUnit4.class)
public class BugreportManagerTest {
    private static final long BUGREPORT_TIMEOUT_MS = TimeUnit.MINUTES.toMillis(4);
    // Sent by Shell when bugreport finishes (contains final bugreport/screenshot file name
    // associated to this bugreport)
    private static final String INTENT_BUGREPORT_FINISHED =
            "com.android.internal.intent.action.BUGREPORT_FINISHED";
    private static final String INTENT_REMOTE_BUGREPORT_DISPATCH =
            "android.intent.action.REMOTE_BUGREPORT_DISPATCH";
    private static final String REMOTE_BUGREPORT_MIMETYPE = "application/vnd.android.bugreport";
    private static final String EXTRA_BUGREPORT = "android.intent.extra.BUGREPORT";
    private static final String EXTRA_SCREENSHOT = "android.intent.extra.SCREENSHOT";
    private static final String BUGREPORT_SERVICE = "bugreportd";

    private Context mContext;
    private BugreportManager mBugreportManager;

    private boolean mIsTv;

    @Before
    public void setup() {
        assumeFalse("Bugreport cannot run in headless system user mode",
                          UserManager.isHeadlessSystemUserMode());
        mContext = InstrumentationRegistry.getInstrumentation().getTargetContext();
        mBugreportManager = mContext.getSystemService(BugreportManager.class);
        mIsTv = mContext.getPackageManager().hasSystemFeature(PackageManager.FEATURE_LEANBACK);
        // Kill current bugreport, so that it does not interfere with future bugreports.
        runShellCommand("setprop ctl.stop " + BUGREPORT_SERVICE);
    }

    @After
    public void tearDown() {
        // Kill current bugreport, so that it does not interfere with future bugreports.
        runShellCommand("setprop ctl.stop " + BUGREPORT_SERVICE);
    }

    @Test
    public void testBugreportParams_getMode() throws Exception {
        int expected_mode = BugreportParams.BUGREPORT_MODE_FULL;
        BugreportParams bp = new BugreportParams(expected_mode);
        assertThat(bp.getMode()).isEqualTo(expected_mode);
    }

    @LargeTest
    @Test
    public void testTelephonyBugreport() throws Exception {
        Pair<String, String> brFiles = triggerBugreport(BugreportParams.BUGREPORT_MODE_TELEPHONY);
        String bugreport = brFiles.first;
        String screenshot = brFiles.second;

        assertBugreportFileNameCorrect(bugreport, "-telephony-" /* suffixName */);
        assertThatFileisNotEmpty(bugreport);
        // telephony bugreport does not take any screenshot
        assertThat(screenshot).isNull();
    }

    @LargeTest
    @Test
    public void testFullBugreport() throws Exception {
        Pair<String, String> brFiles = triggerBugreport(BugreportParams.BUGREPORT_MODE_FULL);
        String bugreport = brFiles.first;
        String screenshot = brFiles.second;

        assertBugreportFileNameCorrect(bugreport, null /* suffixName */);
        assertThatFileisNotEmpty(bugreport);
        // full bugreport takes a default screenshot
        assertScreenshotFileNameCorrect(screenshot);
        assertThatFileisNotEmpty(screenshot);
    }

    @LargeTest
    @Test
    public void testInteractiveBugreport() throws Exception {
        Pair<String, String> brFiles = triggerBugreport(BugreportParams.BUGREPORT_MODE_INTERACTIVE);
        String bugreport = brFiles.first;
        String screenshot = brFiles.second;

        assertBugreportFileNameCorrect(bugreport, null /* suffixName */);
        assertThatFileisNotEmpty(bugreport);
        // tv does not support screenshot button in the ui, interactive bugreport takes a
        // default screenshot.
        if (mIsTv) {
            assertScreenshotFileNameCorrect(screenshot);
            assertThatFileisNotEmpty(screenshot);
        } else {
            assertThat(screenshot).isNull();
        }
    }

    @LargeTest
    @Test
    public void testWifiBugreport() throws Exception {
        Pair<String, String> brFiles = triggerBugreport(BugreportParams.BUGREPORT_MODE_WIFI);
        String bugreport = brFiles.first;
        String screenshot = brFiles.second;

        assertBugreportFileNameCorrect(bugreport, "-wifi-" /* suffixName */);
        assertThatFileisNotEmpty(bugreport);
        // wifi bugreport does not take any screenshot
        assertThat(screenshot).isNull();
    }

    @LargeTest
    @Test
    public void testRemoteBugreport() throws Exception {
        Pair<String, String> brFiles = triggerBugreport(BugreportParams.BUGREPORT_MODE_REMOTE);
        String bugreport = brFiles.first;
        String screenshot = brFiles.second;

        assertBugreportFileNameCorrect(bugreport, null /* suffixName */);
        assertThatFileisNotEmpty(bugreport);
        // remote bugreport does not take any screenshot
        assertThat(screenshot).isNull();
    }

    @LargeTest
    @Test
    public void testWearBugreport() throws Exception {
        Pair<String, String> brFiles = triggerBugreport(BugreportParams.BUGREPORT_MODE_WEAR);
        String bugreport = brFiles.first;
        String screenshot = brFiles.second;

        assertBugreportFileNameCorrect(bugreport, null /* suffixName */);
        assertThatFileisNotEmpty(bugreport);
        // wear bugreport takes a default screenshot
        assertScreenshotFileNameCorrect(screenshot);
        assertThatFileisNotEmpty(screenshot);
    }

    private void assertBugreportFileNameCorrect(String fileName, String suffixName) {
        assertThat(fileName).startsWith(
                "/data/user_de/0/com.android.shell/files/bugreports/bugreport-");
        assertThat(fileName).endsWith(".zip");
        if (suffixName != null) {
            assertThat(fileName).contains(suffixName);
        }
    }

    private void assertScreenshotFileNameCorrect(String fileName) {
        assertThat(fileName).startsWith(
                "/data/user_de/0/com.android.shell/files/bugreports/screenshot-");
        assertThat(fileName).endsWith("-default.png");
    }

    private void assertThatFileisNotEmpty(String file) throws Exception {
        String[] fileInfo = runShellCommand("ls -l " + file).split(" ");
        // Example output of ls -l: -rw------- 1 shell shell 27039619 2020-04-27 12:36 fileName.zip
        assertThat(fileInfo.length).isEqualTo(8);
        long fileSize = Long.parseLong(fileInfo[4]);
        assertThat(fileSize).isGreaterThan(0L);
    }

    private class BugreportBroadcastReceiver extends BroadcastReceiver {
        Intent bugreportFinishedIntent = null;
        final CountDownLatch latch;

        BugreportBroadcastReceiver() {
            latch = new CountDownLatch(1);
        }

        @Override
        public void onReceive(Context context, Intent intent) {
            setBugreportFinishedIntent(intent);
            latch.countDown();
        }

        private void setBugreportFinishedIntent(Intent intent) {
            bugreportFinishedIntent = intent;
        }

        public Intent getBugreportFinishedIntent() {
            return bugreportFinishedIntent;
        }

        public void waitForBugreportFinished() throws Exception {
            if (!latch.await(BUGREPORT_TIMEOUT_MS, TimeUnit.MILLISECONDS)) {
                throw new Exception("Failed to receive BUGREPORT_FINISHED in "
                        + BUGREPORT_TIMEOUT_MS + " ms.");
            }
        }
    }

    private Pair<String, String> triggerBugreport(int type) throws Exception {
        BugreportBroadcastReceiver br = new BugreportBroadcastReceiver();
        final IntentFilter intentFilter;
        if (type == BugreportParams.BUGREPORT_MODE_REMOTE) {
            intentFilter = new IntentFilter(INTENT_REMOTE_BUGREPORT_DISPATCH,
                    REMOTE_BUGREPORT_MIMETYPE);
        } else {
            intentFilter = new IntentFilter(INTENT_BUGREPORT_FINISHED);
        }
        mContext.registerReceiver(br, intentFilter);
        final BugreportParams params = new BugreportParams(type);
        mBugreportManager.requestBugreport(params, "" /* shareTitle */, "" /* shareDescription */);

        try {
            br.waitForBugreportFinished();
        } finally {
            // The latch may fail for a number of reasons but we still need to unregister the
            // BroadcastReceiver.
            mContext.unregisterReceiver(br);
        }

        Intent response = br.getBugreportFinishedIntent();
        assertThat(response.getAction()).isEqualTo(intentFilter.getAction(0));

        String bugreport = response.getStringExtra(EXTRA_BUGREPORT);
        String screenshot = response.getStringExtra(EXTRA_SCREENSHOT);
        return new Pair<String, String>(bugreport, screenshot);
    }
}
