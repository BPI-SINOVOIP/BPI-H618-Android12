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

package com.android.managedprovisioning.common;

import static android.app.admin.DevicePolicyManager.ACTION_ADMIN_POLICY_COMPLIANCE;
import static android.app.admin.DevicePolicyManager.ACTION_PROVISION_MANAGED_DEVICE_FROM_TRUSTED_SOURCE;

import static com.google.common.truth.Truth.assertThat;

import static org.robolectric.Shadows.shadowOf;

import android.app.Activity;
import android.app.Service;
import android.content.ComponentName;
import android.content.Context;
import android.content.Intent;
import android.content.pm.ResolveInfo;
import android.os.UserHandle;

import androidx.test.core.app.ApplicationProvider;

import com.android.managedprovisioning.analytics.MetricsWriterFactory;
import com.android.managedprovisioning.analytics.ProvisioningAnalyticsTracker;
import com.android.managedprovisioning.finalization.SendDpcBroadcastService;
import com.android.managedprovisioning.model.ProvisioningParams;

import org.junit.Test;
import org.junit.runner.RunWith;
import org.robolectric.Robolectric;
import org.robolectric.RobolectricTestRunner;

@RunWith(RobolectricTestRunner.class)
public class PolicyComplianceUtilsTest {

    private static final String TEST_MDM_PACKAGE_NAME = "mdm.package.name";
    private static final String TEST_MDM_ADMIN_RECEIVER = TEST_MDM_PACKAGE_NAME + ".AdminReceiver";
    private static final ComponentName TEST_MDM_ADMIN = new ComponentName(TEST_MDM_PACKAGE_NAME,
            TEST_MDM_ADMIN_RECEIVER);
    private static final int POLICY_COMPLIANCE_ACTIVITY_REQUEST_CODE = 123;
    private static final int NO_FLAGS = 0;

    private final Context mContext = ApplicationProvider.getApplicationContext();
    private PolicyComplianceUtils mPolicyComplianceUtils = new PolicyComplianceUtils();
    private Utils mUtils = new Utils();
    private final ManagedProvisioningSharedPreferences mManagedProvisioningSharedPreferences =
            new ManagedProvisioningSharedPreferences(mContext);
    private final ProvisioningAnalyticsTracker mProvisioningAnalyticsTracker =
            new ProvisioningAnalyticsTracker(
                    MetricsWriterFactory.getMetricsWriter(mContext, new SettingsFacade()),
                    mManagedProvisioningSharedPreferences);
    private final TransitionHelper mTransitionHelper = new TransitionHelper();

    @Test
    public void isPolicyComplianceActivityResolvableForSystemUser_activityExists_returnsTrue() {
        Intent intent = createPolicyComplianceIntent();
        shadowOf(mContext.getPackageManager()).addResolveInfoForIntent(intent, new ResolveInfo());

        boolean result = mPolicyComplianceUtils.isPolicyComplianceActivityResolvableForUser(
                mContext,
                createTrustedSourceParamsBuilder().build(),
                mUtils,
                UserHandle.SYSTEM);

        assertThat(result).isTrue();
    }

    @Test
    public void isPolicyComplianceActivityResolvableForSystemUser_activityExists_returnsFalse() {
        boolean result = mPolicyComplianceUtils.isPolicyComplianceActivityResolvableForUser(
                mContext,
                createTrustedSourceParamsBuilder().build(),
                mUtils,
                UserHandle.SYSTEM);

        assertThat(result).isFalse();
    }

    @Test
    public void startPolicyComplianceActivityForResultIfResolved_activityExists_isStarted() {
        Intent intent = createPolicyComplianceIntent();
        shadowOf(mContext.getPackageManager()).addResolveInfoForIntent(intent, new ResolveInfo());
        Activity parentActivity = Robolectric.buildActivity(Activity.class).create().get();

        boolean result = mPolicyComplianceUtils.startPolicyComplianceActivityForResultIfResolved(
                parentActivity,
                createTrustedSourceParamsBuilder().build(),
                POLICY_COMPLIANCE_ACTIVITY_REQUEST_CODE,
                mUtils,
                mProvisioningAnalyticsTracker,
                mTransitionHelper);

        Intent startedIntent = shadowOf(parentActivity).peekNextStartedActivity();
        assertThat(startedIntent.getAction()).isEqualTo(ACTION_ADMIN_POLICY_COMPLIANCE);
        assertThat(startedIntent.getPackage()).isEqualTo(TEST_MDM_PACKAGE_NAME);
        assertThat(startedIntent.getFlags()).isEqualTo(NO_FLAGS);
        assertThat(result).isTrue();
    }

    @Test
    public void startPolicyComplianceActivityForResultIfResolved_activityDoesNotExist_notStarted() {
        Activity parentActivity = Robolectric.buildActivity(Activity.class).create().get();

        boolean result = mPolicyComplianceUtils.startPolicyComplianceActivityForResultIfResolved(
                parentActivity,
                createTrustedSourceParamsBuilder().build(),
                POLICY_COMPLIANCE_ACTIVITY_REQUEST_CODE,
                mUtils,
                mProvisioningAnalyticsTracker,
                mTransitionHelper);

        Intent startedIntent = shadowOf(parentActivity).peekNextStartedActivity();
        assertThat(startedIntent).isNull();
        assertThat(result).isFalse();
    }

    @Test
    public void startPolicyComplianceActivityIfResolved_activityExists_isStarted() {
        Intent intent = createPolicyComplianceIntent();
        shadowOf(mContext.getPackageManager()).addResolveInfoForIntent(intent, new ResolveInfo());
        Activity parentActivity = Robolectric.buildActivity(Activity.class).create().get();

        boolean result = mPolicyComplianceUtils.startPolicyComplianceActivityIfResolved(
                parentActivity,
                createTrustedSourceParamsBuilder().build(),
                mUtils,
                mProvisioningAnalyticsTracker);

        Intent startedIntent = shadowOf(parentActivity).peekNextStartedActivity();
        assertThat(startedIntent.getAction()).isEqualTo(ACTION_ADMIN_POLICY_COMPLIANCE);
        assertThat(startedIntent.getPackage()).isEqualTo(TEST_MDM_PACKAGE_NAME);
        assertThat(startedIntent.getFlags()).isEqualTo(NO_FLAGS);
        assertThat(result).isTrue();
    }

    @Test
    public void startPolicyComplianceActivityIfResolved_activityDoesNotExist_notStarted() {
        Activity parentActivity = Robolectric.buildActivity(Activity.class).create().get();

        boolean result = mPolicyComplianceUtils.startPolicyComplianceActivityIfResolved(
                parentActivity,
                createTrustedSourceParamsBuilder().build(),
                mUtils,
                mProvisioningAnalyticsTracker);

        Intent startedIntent = shadowOf(parentActivity).peekNextStartedActivity();
        assertThat(startedIntent).isNull();
        assertThat(result).isFalse();
    }

    @Test
    public void startPolicyComplianceActivityIfResolved_fromNonActivityContext_isStartedWithNewTaskFlag() {
        Intent intent = createPolicyComplianceIntent();
        shadowOf(mContext.getPackageManager()).addResolveInfoForIntent(intent, new ResolveInfo());
        Service service = Robolectric.buildService(SendDpcBroadcastService.class).create().get();

        boolean result = mPolicyComplianceUtils.startPolicyComplianceActivityIfResolved(
                service,
                createTrustedSourceParamsBuilder().build(),
                mUtils,
                mProvisioningAnalyticsTracker);

        Intent startedIntent = shadowOf(service).peekNextStartedActivity();
        assertThat(startedIntent.getAction()).isEqualTo(ACTION_ADMIN_POLICY_COMPLIANCE);
        assertThat(startedIntent.getPackage()).isEqualTo(TEST_MDM_PACKAGE_NAME);
        assertThat(startedIntent.getFlags()).isEqualTo(Intent.FLAG_ACTIVITY_NEW_TASK);
        assertThat(result).isTrue();
    }

    private Intent createPolicyComplianceIntent() {
        Intent intent = new Intent(ACTION_ADMIN_POLICY_COMPLIANCE);
        intent.setPackage(TEST_MDM_PACKAGE_NAME);
        return intent;
    }

    private static ProvisioningParams.Builder createTrustedSourceParamsBuilder() {
        return ProvisioningParams.Builder.builder()
                .setProvisioningAction(ACTION_PROVISION_MANAGED_DEVICE_FROM_TRUSTED_SOURCE)
                .setDeviceAdminComponentName(TEST_MDM_ADMIN)
                .setStartedByTrustedSource(true);
    }
}
