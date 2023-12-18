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

package android.jobscheduler.cts;

import static android.jobscheduler.cts.JobThrottlingTest.setTestPackageStandbyBucket;
import static android.jobscheduler.cts.TestAppInterface.TEST_APP_PACKAGE;

import static org.junit.Assert.assertTrue;

import android.app.ActivityManager;
import android.app.AppOpsManager;
import android.content.Context;
import android.jobscheduler.cts.jobtestapp.TestJobSchedulerReceiver;
import android.os.SystemClock;
import android.os.UserHandle;
import android.support.test.uiautomator.UiDevice;

import androidx.test.InstrumentationRegistry;
import androidx.test.runner.AndroidJUnit4;

import com.android.compatibility.common.util.AppOpsUtils;

import org.junit.After;
import org.junit.Before;
import org.junit.Test;
import org.junit.runner.RunWith;

import java.util.Collections;
import java.util.Map;

@RunWith(AndroidJUnit4.class)
public class ExpeditedJobTest {
    private static final long DEFAULT_WAIT_TIMEOUT_MS = 2_000;
    private static final String APP_OP_GET_USAGE_STATS = "android:get_usage_stats";

    private Context mContext;
    private UiDevice mUiDevice;
    private int mTestJobId;
    private TestAppInterface mTestAppInterface;

    @Before
    public void setUp() throws Exception {
        mContext = InstrumentationRegistry.getTargetContext();
        mUiDevice = UiDevice.getInstance(InstrumentationRegistry.getInstrumentation());
        mTestJobId = (int) (SystemClock.uptimeMillis() / 1000);
        mTestAppInterface = new TestAppInterface(mContext, mTestJobId);
        setTestPackageStandbyBucket(mUiDevice, JobThrottlingTest.Bucket.ACTIVE);
        AppOpsUtils.setOpMode(TEST_APP_PACKAGE, APP_OP_GET_USAGE_STATS,
                AppOpsManager.MODE_ALLOWED);
    }

    @After
    public void tearDown() throws Exception {
        mTestAppInterface.cleanup();
        AppOpsUtils.reset(TEST_APP_PACKAGE);
    }

    @Test
    public void testJobUidState() throws Exception {
        mTestAppInterface.scheduleJob(Map.of(
                TestJobSchedulerReceiver.EXTRA_AS_EXPEDITED, true,
                TestJobSchedulerReceiver.EXTRA_REQUEST_JOB_UID_STATE, true
        ), Collections.emptyMap());
        forceRunJob();
        assertTrue("Job did not start after scheduling",
                mTestAppInterface.awaitJobStart(DEFAULT_WAIT_TIMEOUT_MS));
        mTestAppInterface.assertJobUidState(ActivityManager.PROCESS_STATE_TRANSIENT_BACKGROUND,
                ActivityManager.PROCESS_CAPABILITY_NETWORK,
                225 /* ProcessList.PERCEPTIBLE_MEDIUM_APP_ADJ */);
    }

    /** Forces JobScheduler to run the job */
    private void forceRunJob() throws Exception {
        mUiDevice.executeShellCommand("cmd jobscheduler run -f"
                + " -u " + UserHandle.myUserId() + " " + TEST_APP_PACKAGE + " " + mTestJobId);
    }
}
