/*
 * Copyright (C) 2015 The Android Open Source Project
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
package com.android.cts.deviceandprofileowner;

import static com.google.common.truth.Truth.assertWithMessage;

import android.app.admin.DevicePolicyManager;
import android.content.BroadcastReceiver;
import android.content.Context;
import android.content.Intent;
import android.content.IntentFilter;
import android.net.Uri;
import android.util.Log;

import com.android.bedstead.dpmwrapper.Utils;

import java.util.Set;
import java.util.concurrent.Semaphore;
import java.util.concurrent.TimeUnit;

/**
 * Tests for {@link DevicePolicyManager#setApplicationHidden} and
 * {@link DevicePolicyManager#isApplicationHidden} APIs.
 */
public class ApplicationHiddenTest extends BaseDeviceAdminTest {

    private static final String PACKAGE_TO_HIDE = "com.android.cts.permissionapp";
    private static final String NONEXISTING_PACKAGE_NAME = "a.b.c.d";

    private static final IntentFilter PACKAGE_INTENT_FILTER;
    static {
        PACKAGE_INTENT_FILTER = new IntentFilter();
        PACKAGE_INTENT_FILTER.addAction(Intent.ACTION_PACKAGE_ADDED);
        PACKAGE_INTENT_FILTER.addAction(Intent.ACTION_PACKAGE_REMOVED);
        PACKAGE_INTENT_FILTER.addDataScheme("package");
    }
    private final ApplicationHiddenReceiver mReceiver = new ApplicationHiddenReceiver();

    @Override
    protected void setUp() throws Exception {
        super.setUp();

        mContext.registerReceiver(mReceiver, PACKAGE_INTENT_FILTER);
    }

    @Override
    protected void tearDown() throws Exception {
        mContext.unregisterReceiver(mReceiver);
        mDevicePolicyManager.setApplicationHidden(ADMIN_RECEIVER_COMPONENT, PACKAGE_TO_HIDE, false);
        super.tearDown();
    }

    public void testSetApplicationHidden() throws Exception {
        assertWithMessage("setApplicationHidden(%s, %s, true)", ADMIN_RECEIVER_COMPONENT,
                PACKAGE_TO_HIDE)
                        .that(mDevicePolicyManager.setApplicationHidden(ADMIN_RECEIVER_COMPONENT,
                                PACKAGE_TO_HIDE, true))
                        .isTrue();
        assertWithMessage("isApplicationHidden(%s, %s)", ADMIN_RECEIVER_COMPONENT, PACKAGE_TO_HIDE)
                .that(mDevicePolicyManager
                        .isApplicationHidden(ADMIN_RECEIVER_COMPONENT, PACKAGE_TO_HIDE))
                .isTrue();
        mReceiver.waitForRemovedBroadcast();
        assertWithMessage("setApplicationHidden(%s, %s, false)", ADMIN_RECEIVER_COMPONENT,
                PACKAGE_TO_HIDE)
                        .that(mDevicePolicyManager.setApplicationHidden(ADMIN_RECEIVER_COMPONENT,
                                PACKAGE_TO_HIDE, false))
                        .isTrue();
        assertWithMessage("isApplicationHidden(%s, %s)", ADMIN_RECEIVER_COMPONENT, PACKAGE_TO_HIDE)
                .that(mDevicePolicyManager
                        .isApplicationHidden(ADMIN_RECEIVER_COMPONENT, PACKAGE_TO_HIDE))
                .isFalse();
        mReceiver.waitForAddedBroadcast();
    }

    public void testCannotHideActiveAdmin() throws Exception {
        assertWithMessage("setApplicationHidden(%s, %s, true)", ADMIN_RECEIVER_COMPONENT,
                PACKAGE_NAME)
                        .that(mDevicePolicyManager.setApplicationHidden(ADMIN_RECEIVER_COMPONENT,
                                PACKAGE_NAME, true))
                        .isFalse();
    }

    public void testCannotHideNonExistingPackage() throws Exception {
        assertWithMessage("setApplicationHidden(%s, %s, true)", ADMIN_RECEIVER_COMPONENT,
                NONEXISTING_PACKAGE_NAME)
                        .that(mDevicePolicyManager.setApplicationHidden(ADMIN_RECEIVER_COMPONENT,
                                NONEXISTING_PACKAGE_NAME, true))
                        .isFalse();
    }

    public void testCannotHidePolicyExemptApps() throws Exception {
        Set<String> policyExemptApps = mDevicePolicyManager.getPolicyExemptApps();
        Log.v(mTag, "policyExemptApps: " + policyExemptApps);
        if (policyExemptApps.isEmpty()) return;

        policyExemptApps.forEach((app) -> {
            try {
                boolean hidden = mDevicePolicyManager.setApplicationHidden(ADMIN_RECEIVER_COMPONENT,
                        app, true);

                assertWithMessage("setApplicationHidden(%s, true)", app).that(hidden).isFalse();
            } finally {
                maybeUnhideApp(app);
            }
        });
    }

    private void maybeUnhideApp(String app) {
        if (mDevicePolicyManager.isApplicationHidden(ADMIN_RECEIVER_COMPONENT, app)) {
            mDevicePolicyManager.setApplicationHidden(ADMIN_RECEIVER_COMPONENT, app, false);
        }
    }

    private final class ApplicationHiddenReceiver extends BroadcastReceiver {
        private static final int TIMEOUT_SECONDS = 60;
        private final Semaphore mAddedSemaphore = new Semaphore(0);
        private final Semaphore mRemovedSemaphore = new Semaphore(0);

        @Override
        public void onReceive(Context context, Intent intent) {
            Log.v(mTag, "Received intent on user " + context.getUserId() + ": "
                    + Utils.toString(intent));
            Uri uri = intent.getData();
            if (uri == null) {
                return;
            }
            String pkgName = uri.getSchemeSpecificPart();
            if (!PACKAGE_TO_HIDE.equals(pkgName)) {
                return;
            }
            String action = intent.getAction();
            switch(action) {
                case Intent.ACTION_PACKAGE_ADDED:
                    Log.d(mTag, "Received PACKAGE_ADDED broadcast");
                    mAddedSemaphore.release();
                    break;
                case Intent.ACTION_PACKAGE_REMOVED:
                    Log.d(mTag, "Received ACTION_PACKAGE_REMOVED broadcast");
                    mRemovedSemaphore.release();
                    break;
                default:
                    Log.w(mTag, "received invalid intent: " + action);
            }
        }

        public void waitForAddedBroadcast() throws Exception {
            if (!mAddedSemaphore.tryAcquire(TIMEOUT_SECONDS, TimeUnit.SECONDS)) {
                failBroadcastNotReceived(Intent.ACTION_PACKAGE_ADDED);
            }
        }

        public void waitForRemovedBroadcast() throws Exception {
            if (!mRemovedSemaphore.tryAcquire(TIMEOUT_SECONDS, TimeUnit.SECONDS)) {
                failBroadcastNotReceived(Intent.ACTION_PACKAGE_REMOVED);
            }
        }

        private void failBroadcastNotReceived(String broadcast) {
            fail("Did not receive " + broadcast + " broadcast on user " + mContext.getUserId()
                    + " in " + TIMEOUT_SECONDS + "s.");
        }
    }
}
