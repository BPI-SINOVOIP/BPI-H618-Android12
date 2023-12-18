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

import static android.net.NetworkCapabilities.NET_CAPABILITY_INTERNET;
import static android.net.NetworkCapabilities.NET_CAPABILITY_VALIDATED;

import android.app.job.JobInfo;
import android.content.ClipData;
import android.content.Intent;
import android.net.NetworkRequest;
import android.net.Uri;
import android.os.Bundle;
import android.os.PersistableBundle;
import android.provider.ContactsContract;
import android.provider.MediaStore;

/**
 * Tests related to created and reading JobInfo objects.
 */
public class JobInfoTest extends BaseJobSchedulerTest {
    private static final int JOB_ID = JobInfoTest.class.hashCode();

    @Override
    public void tearDown() throws Exception {
        mJobScheduler.cancel(JOB_ID);

        // The super method should be called at the end.
        super.tearDown();
    }

    public void testBackoffCriteria() {
        JobInfo ji = new JobInfo.Builder(JOB_ID, kJobServiceComponent)
                .setBackoffCriteria(12345, JobInfo.BACKOFF_POLICY_LINEAR)
                .build();
        assertEquals(12345, ji.getInitialBackoffMillis());
        assertEquals(JobInfo.BACKOFF_POLICY_LINEAR, ji.getBackoffPolicy());
        // Confirm JobScheduler accepts the JobInfo object.
        mJobScheduler.schedule(ji);

        ji = new JobInfo.Builder(JOB_ID, kJobServiceComponent)
                .setBackoffCriteria(54321, JobInfo.BACKOFF_POLICY_EXPONENTIAL)
                .build();
        assertEquals(54321, ji.getInitialBackoffMillis());
        assertEquals(JobInfo.BACKOFF_POLICY_EXPONENTIAL, ji.getBackoffPolicy());
        // Confirm JobScheduler accepts the JobInfo object.
        mJobScheduler.schedule(ji);
    }

    public void testBatteryNotLow() {
        JobInfo ji = new JobInfo.Builder(JOB_ID, kJobServiceComponent)
                .setRequiresBatteryNotLow(true)
                .build();
        assertTrue(ji.isRequireBatteryNotLow());
        // Confirm JobScheduler accepts the JobInfo object.
        mJobScheduler.schedule(ji);

        ji = new JobInfo.Builder(JOB_ID, kJobServiceComponent)
                .setRequiresBatteryNotLow(false)
                .build();
        assertFalse(ji.isRequireBatteryNotLow());
        // Confirm JobScheduler accepts the JobInfo object.
        mJobScheduler.schedule(ji);
    }

    public void testCharging() {
        JobInfo ji = new JobInfo.Builder(JOB_ID, kJobServiceComponent)
                .setRequiresCharging(true)
                .build();
        assertTrue(ji.isRequireCharging());
        // Confirm JobScheduler accepts the JobInfo object.
        mJobScheduler.schedule(ji);

        ji = new JobInfo.Builder(JOB_ID, kJobServiceComponent)
                .setRequiresCharging(false)
                .build();
        assertFalse(ji.isRequireCharging());
        // Confirm JobScheduler accepts the JobInfo object.
        mJobScheduler.schedule(ji);
    }

    public void testClipData() {
        final ClipData clipData = ClipData.newPlainText("test", "testText");
        JobInfo ji = new JobInfo.Builder(JOB_ID, kJobServiceComponent)
                .setClipData(clipData, Intent.FLAG_GRANT_READ_URI_PERMISSION)
                .build();
        assertEquals(clipData, ji.getClipData());
        assertEquals(Intent.FLAG_GRANT_READ_URI_PERMISSION, ji.getClipGrantFlags());
        // Confirm JobScheduler accepts the JobInfo object.
        mJobScheduler.schedule(ji);

        ji = new JobInfo.Builder(JOB_ID, kJobServiceComponent)
                .setClipData(null, 0)
                .build();
        assertNull(ji.getClipData());
        assertEquals(0, ji.getClipGrantFlags());
        // Confirm JobScheduler accepts the JobInfo object.
        mJobScheduler.schedule(ji);
    }

    public void testDeviceIdle() {
        JobInfo ji = new JobInfo.Builder(JOB_ID, kJobServiceComponent)
                .setRequiresDeviceIdle(true)
                .build();
        assertTrue(ji.isRequireDeviceIdle());
        // Confirm JobScheduler accepts the JobInfo object.
        mJobScheduler.schedule(ji);

        ji = new JobInfo.Builder(JOB_ID, kJobServiceComponent)
                .setRequiresDeviceIdle(false)
                .build();
        assertFalse(ji.isRequireDeviceIdle());
        // Confirm JobScheduler accepts the JobInfo object.
        mJobScheduler.schedule(ji);
    }

    public void testEstimatedNetworkBytes() {
        assertBuildFails(
                "Successfully built a JobInfo specifying estimated network bytes without"
                        + " requesting network",
                new JobInfo.Builder(JOB_ID, kJobServiceComponent)
                        .setEstimatedNetworkBytes(500, 1000));

        JobInfo ji = new JobInfo.Builder(JOB_ID, kJobServiceComponent)
                .setRequiredNetworkType(JobInfo.NETWORK_TYPE_ANY)
                .setEstimatedNetworkBytes(500, 1000)
                .build();
        assertEquals(500, ji.getEstimatedNetworkDownloadBytes());
        assertEquals(1000, ji.getEstimatedNetworkUploadBytes());
        // Confirm JobScheduler accepts the JobInfo object.
        mJobScheduler.schedule(ji);
    }

    public void testExtras() {
        final PersistableBundle pb = new PersistableBundle();
        pb.putInt("random_key", 42);
        JobInfo ji = new JobInfo.Builder(JOB_ID, kJobServiceComponent)
                .setPersisted(true)
                .setExtras(pb)
                .build();
        final PersistableBundle extras = ji.getExtras();
        assertNotNull(extras);
        assertEquals(1, extras.keySet().size());
        assertEquals(42, extras.getInt("random_key"));
        // Confirm JobScheduler accepts the JobInfo object.
        mJobScheduler.schedule(ji);
    }

    public void testExpeditedJob() {
        // Test all allowed constraints.
        JobInfo ji = new JobInfo.Builder(JOB_ID, kJobServiceComponent)
                .setExpedited(true)
                .setPersisted(true)
                .setRequiredNetworkType(JobInfo.NETWORK_TYPE_ANY)
                .setRequiresStorageNotLow(true)
                .build();
        assertTrue(ji.isExpedited());
        // Confirm JobScheduler accepts the JobInfo object.
        mJobScheduler.schedule(ji);

        // Test disallowed constraints.
        final String failureMessage =
                "Successfully built an expedited JobInfo object with disallowed constraints";
        assertBuildFails(failureMessage,
                new JobInfo.Builder(JOB_ID, kJobServiceComponent)
                        .setExpedited(true)
                        .setMinimumLatency(100));
        assertBuildFails(failureMessage,
                new JobInfo.Builder(JOB_ID, kJobServiceComponent)
                        .setExpedited(true)
                        .setOverrideDeadline(200));
        assertBuildFails(failureMessage,
                new JobInfo.Builder(JOB_ID, kJobServiceComponent)
                        .setExpedited(true)
                        .setPeriodic(15 * 60_000));
        assertBuildFails(failureMessage,
                new JobInfo.Builder(JOB_ID, kJobServiceComponent)
                        .setExpedited(true)
                        .setImportantWhileForeground(true));
        assertBuildFails(failureMessage,
                new JobInfo.Builder(JOB_ID, kJobServiceComponent)
                        .setExpedited(true)
                        .setPrefetch(true));
        assertBuildFails(failureMessage,
                new JobInfo.Builder(JOB_ID, kJobServiceComponent)
                        .setExpedited(true)
                        .setRequiresDeviceIdle(true));
        assertBuildFails(failureMessage,
                new JobInfo.Builder(JOB_ID, kJobServiceComponent)
                        .setExpedited(true)
                        .setRequiresBatteryNotLow(true));
        assertBuildFails(failureMessage,
                new JobInfo.Builder(JOB_ID, kJobServiceComponent)
                        .setExpedited(true)
                        .setRequiresCharging(true));
        final JobInfo.TriggerContentUri tcu = new JobInfo.TriggerContentUri(
                Uri.parse("content://" + MediaStore.AUTHORITY + "/"),
                JobInfo.TriggerContentUri.FLAG_NOTIFY_FOR_DESCENDANTS);
        assertBuildFails(failureMessage,
                new JobInfo.Builder(JOB_ID, kJobServiceComponent)
                        .setExpedited(true)
                        .addTriggerContentUri(tcu));
    }

    public void testImportantWhileForeground() {
        // Assert the default value is false
        JobInfo ji = new JobInfo.Builder(JOB_ID, kJobServiceComponent)
                .build();
        assertFalse(ji.isImportantWhileForeground());
        // Confirm JobScheduler accepts the JobInfo object.
        mJobScheduler.schedule(ji);

        ji = new JobInfo.Builder(JOB_ID, kJobServiceComponent)
                .setImportantWhileForeground(true)
                .build();
        assertTrue(ji.isImportantWhileForeground());
        // Confirm JobScheduler accepts the JobInfo object.
        mJobScheduler.schedule(ji);

        ji = new JobInfo.Builder(JOB_ID, kJobServiceComponent)
                .setImportantWhileForeground(false)
                .build();
        assertFalse(ji.isImportantWhileForeground());
        // Confirm JobScheduler accepts the JobInfo object.
        mJobScheduler.schedule(ji);
    }

    public void testMinimumLatency() {
        JobInfo ji = new JobInfo.Builder(JOB_ID, kJobServiceComponent)
                .setMinimumLatency(1337)
                .build();
        assertEquals(1337, ji.getMinLatencyMillis());
        // Confirm JobScheduler accepts the JobInfo object.
        mJobScheduler.schedule(ji);
    }

    public void testOverrideDeadline() {
        JobInfo ji = new JobInfo.Builder(JOB_ID, kJobServiceComponent)
                .setOverrideDeadline(7357)
                .build();
        // ...why are the set/get methods named differently?? >.>
        assertEquals(7357, ji.getMaxExecutionDelayMillis());
    }

    public void testPeriodic() {
        JobInfo ji = new JobInfo.Builder(JOB_ID, kJobServiceComponent)
                .setPeriodic(60 * 60 * 1000L)
                .build();
        assertTrue(ji.isPeriodic());
        assertEquals(60 * 60 * 1000L, ji.getIntervalMillis());
        assertEquals(60 * 60 * 1000L, ji.getFlexMillis());
        // Confirm JobScheduler accepts the JobInfo object.
        mJobScheduler.schedule(ji);

        ji = new JobInfo.Builder(JOB_ID, kJobServiceComponent)
                .setPeriodic(120 * 60 * 1000L, 20 * 60 * 1000L)
                .build();
        assertTrue(ji.isPeriodic());
        assertEquals(120 * 60 * 1000L, ji.getIntervalMillis());
        assertEquals(20 * 60 * 1000L, ji.getFlexMillis());
        // Confirm JobScheduler accepts the JobInfo object.
        mJobScheduler.schedule(ji);
    }

    public void testPersisted() {
        // Assert the default value is false
        JobInfo ji = new JobInfo.Builder(JOB_ID, kJobServiceComponent)
                .build();
        assertFalse(ji.isPersisted());
        // Confirm JobScheduler accepts the JobInfo object.
        mJobScheduler.schedule(ji);

        ji = new JobInfo.Builder(JOB_ID, kJobServiceComponent)
                .setPersisted(true)
                .build();
        assertTrue(ji.isPersisted());
        // Confirm JobScheduler accepts the JobInfo object.
        mJobScheduler.schedule(ji);

        ji = new JobInfo.Builder(JOB_ID, kJobServiceComponent)
                .setPersisted(false)
                .build();
        assertFalse(ji.isPersisted());
        // Confirm JobScheduler accepts the JobInfo object.
        mJobScheduler.schedule(ji);
    }

    public void testPrefetch() {
        // Assert the default value is false
        JobInfo ji = new JobInfo.Builder(JOB_ID, kJobServiceComponent)
                .build();
        assertFalse(ji.isPrefetch());
        // Confirm JobScheduler accepts the JobInfo object.
        mJobScheduler.schedule(ji);

        ji = new JobInfo.Builder(JOB_ID, kJobServiceComponent)
                .setPrefetch(true)
                .build();
        assertTrue(ji.isPrefetch());
        // Confirm JobScheduler accepts the JobInfo object.
        mJobScheduler.schedule(ji);

        ji = new JobInfo.Builder(JOB_ID, kJobServiceComponent)
                .setPrefetch(false)
                .build();
        assertFalse(ji.isPrefetch());
        // Confirm JobScheduler accepts the JobInfo object.
        mJobScheduler.schedule(ji);
    }

    public void testRequiredNetwork() {
        final NetworkRequest nr = new NetworkRequest.Builder()
                .addCapability(NET_CAPABILITY_INTERNET)
                .addCapability(NET_CAPABILITY_VALIDATED)
                .build();
        JobInfo ji = new JobInfo.Builder(JOB_ID, kJobServiceComponent)
                .setRequiredNetwork(nr)
                .build();
        assertEquals(nr, ji.getRequiredNetwork());
        // Confirm JobScheduler accepts the JobInfo object.
        mJobScheduler.schedule(ji);

        ji = new JobInfo.Builder(JOB_ID, kJobServiceComponent)
                .setRequiredNetwork(null)
                .build();
        assertNull(ji.getRequiredNetwork());
        // Confirm JobScheduler accepts the JobInfo object.
        mJobScheduler.schedule(ji);
    }

    @SuppressWarnings("deprecation")
    public void testRequiredNetworkType() {
        // Assert the default value is NONE
        JobInfo ji = new JobInfo.Builder(JOB_ID, kJobServiceComponent)
                .build();
        assertEquals(JobInfo.NETWORK_TYPE_NONE, ji.getNetworkType());
        // Confirm JobScheduler accepts the JobInfo object.
        mJobScheduler.schedule(ji);

        ji = new JobInfo.Builder(JOB_ID, kJobServiceComponent)
                .setRequiredNetworkType(JobInfo.NETWORK_TYPE_ANY)
                .build();
        assertEquals(JobInfo.NETWORK_TYPE_ANY, ji.getNetworkType());
        // Confirm JobScheduler accepts the JobInfo object.
        mJobScheduler.schedule(ji);

        ji = new JobInfo.Builder(JOB_ID, kJobServiceComponent)
                .setRequiredNetworkType(JobInfo.NETWORK_TYPE_UNMETERED)
                .build();
        assertEquals(JobInfo.NETWORK_TYPE_UNMETERED, ji.getNetworkType());
        // Confirm JobScheduler accepts the JobInfo object.
        mJobScheduler.schedule(ji);

        ji = new JobInfo.Builder(JOB_ID, kJobServiceComponent)
                .setRequiredNetworkType(JobInfo.NETWORK_TYPE_NOT_ROAMING)
                .build();
        assertEquals(JobInfo.NETWORK_TYPE_NOT_ROAMING, ji.getNetworkType());
        // Confirm JobScheduler accepts the JobInfo object.
        mJobScheduler.schedule(ji);

        ji = new JobInfo.Builder(JOB_ID, kJobServiceComponent)
                .setRequiredNetworkType(JobInfo.NETWORK_TYPE_CELLULAR)
                .build();
        assertEquals(JobInfo.NETWORK_TYPE_CELLULAR, ji.getNetworkType());
        // Confirm JobScheduler accepts the JobInfo object.
        mJobScheduler.schedule(ji);

        ji = new JobInfo.Builder(JOB_ID, kJobServiceComponent)
                .setRequiredNetworkType(JobInfo.NETWORK_TYPE_NONE)
                .build();
        assertEquals(JobInfo.NETWORK_TYPE_NONE, ji.getNetworkType());
        // Confirm JobScheduler accepts the JobInfo object.
        mJobScheduler.schedule(ji);
    }

    public void testStorageNotLow() {
        JobInfo ji = new JobInfo.Builder(JOB_ID, kJobServiceComponent)
                .setRequiresStorageNotLow(true)
                .build();
        assertTrue(ji.isRequireStorageNotLow());
        // Confirm JobScheduler accepts the JobInfo object.
        mJobScheduler.schedule(ji);

        ji = new JobInfo.Builder(JOB_ID, kJobServiceComponent)
                .setRequiresStorageNotLow(false)
                .build();
        assertFalse(ji.isRequireStorageNotLow());
        // Confirm JobScheduler accepts the JobInfo object.
        mJobScheduler.schedule(ji);
    }

    public void testTransientExtras() {
        final Bundle b = new Bundle();
        b.putBoolean("random_bool", true);
        assertBuildFails("Successfully built a persisted JobInfo object with transient extras",
                new JobInfo.Builder(JOB_ID, kJobServiceComponent)
                        .setPersisted(true)
                        .setTransientExtras(b));

        JobInfo ji = new JobInfo.Builder(JOB_ID, kJobServiceComponent)
                .setTransientExtras(b)
                .build();
        assertEquals(b.size(), ji.getTransientExtras().size());
        for (String key : b.keySet()) {
            assertEquals(b.get(key), ji.getTransientExtras().get(key));
        }
        // Confirm JobScheduler accepts the JobInfo object.
        mJobScheduler.schedule(ji);
    }

    public void testTriggerContentMaxDelay() {
        JobInfo ji = new JobInfo.Builder(JOB_ID, kJobServiceComponent)
                .setTriggerContentMaxDelay(1337)
                .build();
        assertEquals(1337, ji.getTriggerContentMaxDelay());
        // Confirm JobScheduler accepts the JobInfo object.
        mJobScheduler.schedule(ji);
    }

    public void testTriggerContentUpdateDelay() {
        JobInfo ji = new JobInfo.Builder(JOB_ID, kJobServiceComponent)
                .setTriggerContentUpdateDelay(1337)
                .build();
        assertEquals(1337, ji.getTriggerContentUpdateDelay());
        // Confirm JobScheduler accepts the JobInfo object.
        mJobScheduler.schedule(ji);
    }

    public void testTriggerContentUri() {
        final Uri u = Uri.parse("content://" + MediaStore.AUTHORITY + "/");
        final JobInfo.TriggerContentUri tcu = new JobInfo.TriggerContentUri(
                u, JobInfo.TriggerContentUri.FLAG_NOTIFY_FOR_DESCENDANTS);
        assertEquals(u, tcu.getUri());
        assertEquals(JobInfo.TriggerContentUri.FLAG_NOTIFY_FOR_DESCENDANTS, tcu.getFlags());
        JobInfo ji = new JobInfo.Builder(JOB_ID, kJobServiceComponent)
                .addTriggerContentUri(tcu)
                .build();
        assertEquals(1, ji.getTriggerContentUris().length);
        assertEquals(tcu, ji.getTriggerContentUris()[0]);
        // Confirm JobScheduler accepts the JobInfo object.
        mJobScheduler.schedule(ji);

        final Uri u2 = Uri.parse("content://" + ContactsContract.AUTHORITY + "/");
        final JobInfo.TriggerContentUri tcu2 = new JobInfo.TriggerContentUri(u2, 0);
        assertEquals(u2, tcu2.getUri());
        assertEquals(0, tcu2.getFlags());
        ji = new JobInfo.Builder(JOB_ID, kJobServiceComponent)
                .addTriggerContentUri(tcu)
                .addTriggerContentUri(tcu2)
                .build();
        assertEquals(2, ji.getTriggerContentUris().length);
        assertEquals(tcu, ji.getTriggerContentUris()[0]);
        assertEquals(tcu2, ji.getTriggerContentUris()[1]);
        // Confirm JobScheduler accepts the JobInfo object.
        mJobScheduler.schedule(ji);
    }

    private void assertBuildFails(String message, JobInfo.Builder builder) {
        try {
            builder.build();
            fail(message);
        } catch (IllegalArgumentException e) {
            // Expected
        }
    }
}
