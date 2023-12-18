/*
 * Copyright (C) 2017 The Android Open Source Project
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
package com.android.cts.delegate;

import static android.app.admin.DevicePolicyManager.DELEGATION_APP_RESTRICTIONS;

import static com.android.cts.delegate.DelegateTestUtils.assertExpectException;

import android.app.admin.DevicePolicyManager;
import android.content.BroadcastReceiver;
import android.content.ComponentName;
import android.content.Context;
import android.content.Intent;
import android.content.IntentFilter;
import android.os.Bundle;
import android.os.Process;
import android.util.Log;

import java.util.List;
import java.util.concurrent.Semaphore;
import java.util.concurrent.TimeUnit;

/**
 * Test that an app given the {@link DevicePolicyManager#DELEGATION_APP_RESTRICTIONS} scope via
 * {@link DevicePolicyManager#setDelegatedScopes} can manage app restrictions.
 */
public class AppRestrictionsDelegateTest extends BaseJUnit3TestCase  {

    private static final String TAG = AppRestrictionsDelegateTest.class.getSimpleName();

    private static final String APP_RESTRICTIONS_TARGET_PKG =
            "com.android.cts.apprestrictions.targetapp";
    private static final String APP_RESTRICTIONS_ACTIVITY_NAME =
            APP_RESTRICTIONS_TARGET_PKG + ".ApplicationRestrictionsActivity";
    private static final String ACTION_RESTRICTIONS_VALUE =
            "com.android.cts.apprestrictions.targetapp.RESTRICTIONS_VALUE";

    private static final Bundle BUNDLE_0 = createBundle0();
    private static final Bundle BUNDLE_1 = createBundle1();

    private static final long TIMEOUT_SECONDS = 10;

    private final BroadcastReceiver mReceiver = new BroadcastReceiver() {
        @Override
        public void onReceive(Context context, Intent intent) {
            String action = intent.getAction();
            Log.d(TAG, "onReceive(): intent " + action + " on uid " + Process.myUid());
            if (ACTION_RESTRICTIONS_VALUE.equals(action)) {
                mReceivedRestrictions = intent.getBundleExtra("value");
                mOnRestrictionsSemaphore.release();
            }
        }
    };

    private final Semaphore mOnRestrictionsSemaphore = new Semaphore(0);
    private Bundle mReceivedRestrictions;

    @Override
    protected void setUp() throws Exception {
        super.setUp();

        mContext.registerReceiver(mReceiver, new IntentFilter(ACTION_RESTRICTIONS_VALUE));
    }

    @Override
    protected void tearDown() throws Exception {
        mContext.unregisterReceiver(mReceiver);
        super.tearDown();
    }

    public void testCannotAccessApis() {
        assertFalse("DelegateApp should not be an app restrictions delegate",
                amIAppRestrictionsDelegate());

        assertExpectException(SecurityException.class,
                "Calling identity is not authorized", () -> {
                    mDpm.setApplicationRestrictions(null, APP_RESTRICTIONS_TARGET_PKG, null);
                });

        assertExpectException(SecurityException.class,
                "Calling identity is not authorized", () -> {
                    mDpm.getApplicationRestrictions(null, APP_RESTRICTIONS_TARGET_PKG);
                });
    }

    public void testCanAccessApis() throws InterruptedException {
        assertTrue("DelegateApp is not an app restrictions delegate", amIAppRestrictionsDelegate());
        try {
            mDpm.setApplicationRestrictions(null, APP_RESTRICTIONS_TARGET_PKG, BUNDLE_0);
            assertBundle0(mDpm.getApplicationRestrictions(null, APP_RESTRICTIONS_TARGET_PKG));

            // Check that the target app can retrieve the same restrictions.
            assertBundle0(waitForChangedRestriction());

            // Test overwriting
            mDpm.setApplicationRestrictions(null, APP_RESTRICTIONS_TARGET_PKG, BUNDLE_1);
            assertBundle1(mDpm.getApplicationRestrictions(null, APP_RESTRICTIONS_TARGET_PKG));
            assertBundle1(waitForChangedRestriction());
        } finally {
            mDpm.setApplicationRestrictions(null, APP_RESTRICTIONS_TARGET_PKG, new Bundle());
            assertTrue(
                mDpm.getApplicationRestrictions(null, APP_RESTRICTIONS_TARGET_PKG).isEmpty());
        }
    }

    // Should be consistent with assertBundle0
    private static Bundle createBundle0() {
        Bundle result = new Bundle();
        result.putString("placeholderString", "value");
        return result;
    }

    // Should be consistent with createBundle0
    private void assertBundle0(Bundle bundle) {
        assertEquals(1, bundle.size());
        assertEquals("value", bundle.getString("placeholderString"));
    }

    // Should be consistent with assertBundle1
    private static Bundle createBundle1() {
        Bundle result = new Bundle();
        result.putInt("placeholderInt", 1);
        return result;
    }

    // Should be consistent with createBundle1
    private void assertBundle1(Bundle bundle) {
        assertEquals(1, bundle.size());
        assertEquals(1, bundle.getInt("placeholderInt"));
    }

    private void startTestActivity() {
        ComponentName component = new ComponentName(
                APP_RESTRICTIONS_TARGET_PKG, APP_RESTRICTIONS_ACTIVITY_NAME);
        Log.d(TAG, "Starting activity " + component.flattenToShortString() + " on user "
                + Process.myUserHandle());
        mContext.startActivity(new Intent()
                .setComponent(component)
                .addFlags(Intent.FLAG_ACTIVITY_SINGLE_TOP | Intent.FLAG_ACTIVITY_NEW_TASK));
    }

    private Bundle waitForChangedRestriction() throws InterruptedException {
        startTestActivity();
        assertTrue("App restrictions target app did not respond in time",
                mOnRestrictionsSemaphore.tryAcquire(TIMEOUT_SECONDS, TimeUnit.SECONDS));
        return mReceivedRestrictions;
    }

    private boolean amIAppRestrictionsDelegate() {
        final List<String> scopes = mDpm.getDelegatedScopes(null, mContext.getPackageName());
        return scopes.contains(DELEGATION_APP_RESTRICTIONS);
    }
}
