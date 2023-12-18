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

package android.app.cts;

import static junit.framework.Assert.assertEquals;
import static junit.framework.Assert.assertNull;

import android.app.BroadcastOptions;
import android.os.Build;
import android.os.Bundle;
import android.os.PowerExemptionManager;

import androidx.test.ext.junit.runners.AndroidJUnit4;

import org.junit.Test;
import org.junit.runner.RunWith;

@RunWith(AndroidJUnit4.class)
public class BroadcastOptionsTest {

    /**
     * Creates a clone of BroadcastOptions, using toBundle().
     */
    private BroadcastOptions cloneViaBundle(BroadcastOptions bo) {
        final Bundle b = bo.toBundle();

        // If toBundle() returns null, that means the BroadcastOptions was the default values.
        return b == null ? BroadcastOptions.makeBasic() : new BroadcastOptions(b);
    }

    private void assertBroadcastOptionTemporaryAppAllowList(
            BroadcastOptions bo,
            long expectedDuration,
            int expectedAllowListType,
            int expectedReasonCode,
            String expectedReason) {
        assertEquals(expectedAllowListType, bo.getTemporaryAppAllowlistType());
        assertEquals(expectedDuration, bo.getTemporaryAppAllowlistDuration());
        assertEquals(expectedReasonCode, bo.getTemporaryAppAllowlistReasonCode());
        assertEquals(expectedReason, bo.getTemporaryAppAllowlistReason());

        // Clone the BO and check it too.
        BroadcastOptions cloned = cloneViaBundle(bo);
        assertEquals(expectedAllowListType, cloned.getTemporaryAppAllowlistType());
        assertEquals(expectedDuration, cloned.getTemporaryAppAllowlistDuration());
        assertEquals(expectedReasonCode, cloned.getTemporaryAppAllowlistReasonCode());
        assertEquals(expectedReason, cloned.getTemporaryAppAllowlistReason());
    }

    private void assertBroadcastOption_noTemporaryAppAllowList(BroadcastOptions bo) {
        assertBroadcastOptionTemporaryAppAllowList(bo,
                /* duration= */ 0,
                PowerExemptionManager.TEMPORARY_ALLOW_LIST_TYPE_NONE,
                PowerExemptionManager.REASON_UNKNOWN,
                /* reason= */ null);
    }

    @Test
    public void testTemporaryAppAllowlistBroadcastOptions_defaultValues() {
        BroadcastOptions bo;

        bo = BroadcastOptions.makeBasic();

        // If no options are set, toBundle() should return null
        assertNull(bo.toBundle());

        // Check the default values about temp-allowlist.
        assertBroadcastOption_noTemporaryAppAllowList(bo);
    }

    @Test
    public void testSetTemporaryAppWhitelistDuration_legacyApi() {
        BroadcastOptions bo;

        bo = BroadcastOptions.makeBasic();

        bo.setTemporaryAppWhitelistDuration(10);

        assertBroadcastOptionTemporaryAppAllowList(bo,
                /* duration= */ 10,
                PowerExemptionManager.TEMPORARY_ALLOW_LIST_TYPE_FOREGROUND_SERVICE_ALLOWED,
                PowerExemptionManager.REASON_UNKNOWN,
                /* reason= */ null);

        // Clear the temp-allowlist.
        bo.setTemporaryAppWhitelistDuration(0);

        // Check the default values about temp-allowlist.
        assertBroadcastOption_noTemporaryAppAllowList(bo);
    }

    @Test
    public void testSetTemporaryAppWhitelistDuration() {
        BroadcastOptions bo;

        bo = BroadcastOptions.makeBasic();

        bo.setTemporaryAppAllowlist(10,
                PowerExemptionManager.TEMPORARY_ALLOW_LIST_TYPE_FOREGROUND_SERVICE_NOT_ALLOWED,
                PowerExemptionManager.REASON_GEOFENCING,
                null);

        assertBroadcastOptionTemporaryAppAllowList(bo,
                /* duration= */ 10,
                PowerExemptionManager.TEMPORARY_ALLOW_LIST_TYPE_FOREGROUND_SERVICE_NOT_ALLOWED,
                PowerExemptionManager.REASON_GEOFENCING,
                /* reason= */ null);

        // Setting duration 0 will clear the previous call.
        bo.setTemporaryAppAllowlist(0,
                PowerExemptionManager.TEMPORARY_ALLOW_LIST_TYPE_FOREGROUND_SERVICE_NOT_ALLOWED,
                PowerExemptionManager.REASON_ACTIVITY_RECOGNITION,
                "reason");
        assertBroadcastOption_noTemporaryAppAllowList(bo);

        // Set again.
        bo.setTemporaryAppAllowlist(20,
                PowerExemptionManager.TEMPORARY_ALLOW_LIST_TYPE_FOREGROUND_SERVICE_ALLOWED,
                PowerExemptionManager.REASON_GEOFENCING,
                "reason");

        assertBroadcastOptionTemporaryAppAllowList(bo,
                /* duration= */ 20,
                PowerExemptionManager.TEMPORARY_ALLOW_LIST_TYPE_FOREGROUND_SERVICE_ALLOWED,
                PowerExemptionManager.REASON_GEOFENCING,
                /* reason= */ "reason");

        // Set to NONE will clear the previous call too.
        bo.setTemporaryAppAllowlist(10,
                PowerExemptionManager.TEMPORARY_ALLOW_LIST_TYPE_NONE,
                PowerExemptionManager.REASON_ACTIVITY_RECOGNITION,
                "reason");

        assertBroadcastOption_noTemporaryAppAllowList(bo);
    }

    @Test
    public void testMaxManifestReceiverApiLevel() {
        final BroadcastOptions bo = BroadcastOptions.makeBasic();
        // No MaxManifestReceiverApiLevel set, the default value should be CUR_DEVELOPMENT.
        assertEquals(Build.VERSION_CODES.CUR_DEVELOPMENT, bo.getMaxManifestReceiverApiLevel());

        // Set MaxManifestReceiverApiLevel to P.
        bo.setMaxManifestReceiverApiLevel(Build.VERSION_CODES.P);
        assertEquals(Build.VERSION_CODES.P, bo.getMaxManifestReceiverApiLevel());

        // Clone the BroadcastOptions and check it too.
        final BroadcastOptions cloned = cloneViaBundle(bo);
        assertEquals(Build.VERSION_CODES.P, bo.getMaxManifestReceiverApiLevel());
    }
}
