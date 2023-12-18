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

package android.jobscheduler.cts;

import android.app.job.JobInfo;
import android.app.job.JobWorkItem;
import android.content.Intent;
import android.jobscheduler.MockJobService;

import java.util.List;

/**
 * Tests related to created and reading JobWorkItem objects.
 */
public class JobWorkItemTest extends BaseJobSchedulerTest {
    private static final int JOB_ID = JobWorkItemTest.class.hashCode();
    private static final Intent TEST_INTENT = new Intent("some.random.action");

    public void testIntentOnlyItem() {
        JobWorkItem jwi = new JobWorkItem(TEST_INTENT);

        assertEquals(TEST_INTENT, jwi.getIntent());
        assertEquals(JobInfo.NETWORK_BYTES_UNKNOWN, jwi.getEstimatedNetworkDownloadBytes());
        assertEquals(JobInfo.NETWORK_BYTES_UNKNOWN, jwi.getEstimatedNetworkUploadBytes());
        // JobWorkItem hasn't been scheduled yet. Delivery count should be 0.
        assertEquals(0, jwi.getDeliveryCount());
    }

    public void testItemWithEstimatedBytes() {
        JobWorkItem jwi = new JobWorkItem(TEST_INTENT, 10, 20);

        assertEquals(TEST_INTENT, jwi.getIntent());
        assertEquals(10, jwi.getEstimatedNetworkDownloadBytes());
        assertEquals(20, jwi.getEstimatedNetworkUploadBytes());
        // JobWorkItem hasn't been scheduled yet. Delivery count should be 0.
        assertEquals(0, jwi.getDeliveryCount());
    }

    public void testDeliveryCountBumped() throws Exception {
        JobInfo jobInfo = new JobInfo.Builder(JOB_ID, kJobServiceComponent)
                .setOverrideDeadline(0)
                .build();
        JobWorkItem jwi = new JobWorkItem(TEST_INTENT, 10, 20);
        // JobWorkItem hasn't been scheduled yet. Delivery count should be 0.
        assertEquals(0, jwi.getDeliveryCount());

        kTestEnvironment.setExpectedExecutions(1);
        kTestEnvironment.setExpectedWork(new MockJobService.TestWorkItem[]{
                new MockJobService.TestWorkItem(TEST_INTENT)});
        kTestEnvironment.readyToWork();
        mJobScheduler.enqueue(jobInfo, jwi);
        runSatisfiedJob(JOB_ID);
        assertTrue("Job didn't fire immediately", kTestEnvironment.awaitExecution());

        List<JobWorkItem> executedJWIs = kTestEnvironment.getLastReceivedWork();
        assertEquals(1, executedJWIs.size());
        assertEquals(1, executedJWIs.get(0).getDeliveryCount());
    }
}
