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

package android.devicepolicy.cts;

import static android.app.ActivityManager.LOCK_TASK_MODE_LOCKED;
import static android.app.ActivityManager.LOCK_TASK_MODE_NONE;
import static android.app.admin.DevicePolicyManager.LOCK_TASK_FEATURE_BLOCK_ACTIVITY_START_IN_TASK;
import static android.app.admin.DevicePolicyManager.LOCK_TASK_FEATURE_GLOBAL_ACTIONS;
import static android.app.admin.DevicePolicyManager.LOCK_TASK_FEATURE_HOME;
import static android.app.admin.DevicePolicyManager.LOCK_TASK_FEATURE_KEYGUARD;
import static android.app.admin.DevicePolicyManager.LOCK_TASK_FEATURE_NONE;
import static android.app.admin.DevicePolicyManager.LOCK_TASK_FEATURE_NOTIFICATIONS;
import static android.app.admin.DevicePolicyManager.LOCK_TASK_FEATURE_OVERVIEW;
import static android.app.admin.DevicePolicyManager.LOCK_TASK_FEATURE_SYSTEM_INFO;
import static android.content.Intent.ACTION_DIAL;
import static android.content.Intent.FLAG_ACTIVITY_CLEAR_TASK;
import static android.content.Intent.FLAG_ACTIVITY_NEW_TASK;
import static android.content.pm.PackageManager.FEATURE_TELEPHONY;

import static com.android.queryable.queries.StringQuery.string;

import static com.google.common.truth.Truth.assertThat;
import static com.google.common.truth.Truth.assertWithMessage;

import static org.junit.Assume.assumeFalse;
import static org.testng.Assert.assertThrows;

import android.app.ActivityOptions;
import android.app.admin.DevicePolicyManager;
import android.content.ComponentName;
import android.content.Intent;
import android.content.pm.PackageManager;
import android.content.pm.ResolveInfo;
import android.os.Bundle;
import android.stats.devicepolicy.EventId;
import android.telecom.TelecomManager;

import com.android.bedstead.harrier.BedsteadJUnit4;
import com.android.bedstead.harrier.DeviceState;
import com.android.bedstead.harrier.annotations.Postsubmit;
import com.android.bedstead.harrier.annotations.RequireFeature;
import com.android.bedstead.harrier.annotations.enterprise.CannotSetPolicyTest;
import com.android.bedstead.harrier.annotations.enterprise.NegativePolicyTest;
import com.android.bedstead.harrier.annotations.enterprise.PositivePolicyTest;
import com.android.bedstead.harrier.policies.LockTask;
import com.android.bedstead.metricsrecorder.EnterpriseMetricsRecorder;
import com.android.bedstead.nene.TestApis;
import com.android.bedstead.nene.activities.Activity;
import com.android.bedstead.nene.packages.ComponentReference;
import com.android.bedstead.nene.packages.PackageReference;
import com.android.bedstead.remotedpc.RemoteDpc;
import com.android.bedstead.testapp.TestApp;
import com.android.bedstead.testapp.TestAppActivity;
import com.android.bedstead.testapp.TestAppActivityReference;
import com.android.bedstead.testapp.TestAppInstanceReference;
import com.android.bedstead.testapp.TestAppProvider;
import com.android.compatibility.common.util.PollingCheck;
import com.android.eventlib.EventLogs;
import com.android.eventlib.events.activities.ActivityDestroyedEvent;
import com.android.eventlib.events.activities.ActivityStartedEvent;

import org.junit.ClassRule;
import org.junit.Rule;
import org.junit.Test;
import org.junit.runner.RunWith;

import java.util.Set;

@RunWith(BedsteadJUnit4.class)
public class LockTaskTest {

    private static final String PACKAGE_NAME = "com.android.package.test";

    @ClassRule @Rule
    public static final DeviceState sDeviceState = new DeviceState();

    private static final TestApis sTestApis = new TestApis();

    private static final DevicePolicyManager sLocalDevicePolicyManager =
            sTestApis.context().instrumentedContext().getSystemService(DevicePolicyManager.class);

    private static final int[] INDIVIDUALLY_SETTABLE_FLAGS = new int[] {
            LOCK_TASK_FEATURE_SYSTEM_INFO,
            LOCK_TASK_FEATURE_HOME,
            LOCK_TASK_FEATURE_GLOBAL_ACTIONS,
            LOCK_TASK_FEATURE_KEYGUARD
    };

    private static final int[] FLAGS_SETTABLE_WITH_HOME = new int[] {
            LOCK_TASK_FEATURE_SYSTEM_INFO,
            LOCK_TASK_FEATURE_OVERVIEW,
            LOCK_TASK_FEATURE_NOTIFICATIONS,
            LOCK_TASK_FEATURE_GLOBAL_ACTIONS,
            LOCK_TASK_FEATURE_KEYGUARD
    };

    private static final TestAppProvider sTestAppProvider = new TestAppProvider();
    private static final TestApp sLockTaskTestApp = sTestAppProvider.query()
            .wherePackageName().isEqualTo("android.LockTaskApp")
            .get(); // TODO(scottjonathan): filter by containing activity not by package name
    private static final TestApp sTestApp = sTestAppProvider.any();

    private static final TestApp sSecondTestApp = sTestAppProvider.any();

    private static final ComponentReference BLOCKED_ACTIVITY_COMPONENT =
            sTestApis.packages().component(new ComponentName(
                    "android", "com.android.internal.app.BlockedAppActivity"));

    private static final String ACTION_EMERGENCY_DIAL = "com.android.phone.EmergencyDialer.DIAL";

    @Test
    @Postsubmit(reason = "New test")
    @PositivePolicyTest(policy = LockTask.class)
    public void setLockTaskPackages_lockTaskPackagesIsSet() {
        String[] originalLockTaskPackages =
                sDeviceState.dpc().devicePolicyManager().getLockTaskPackages();

        sDeviceState.dpc().devicePolicyManager().setLockTaskPackages(new String[]{PACKAGE_NAME});

        try {
            assertThat(sDeviceState.dpc().devicePolicyManager().getLockTaskPackages()).asList()
                    .containsExactly(PACKAGE_NAME);
        } finally {
            sDeviceState.dpc().devicePolicyManager().setLockTaskPackages(originalLockTaskPackages);
        }
    }

    @Test
    @Postsubmit(reason = "New test")
    @PositivePolicyTest(policy = LockTask.class)
    public void startLockTask_recordsMetric() {
        String[] originalLockTaskPackages =
                sDeviceState.dpc().devicePolicyManager().getLockTaskPackages();

        try (EnterpriseMetricsRecorder metrics = EnterpriseMetricsRecorder.create();
             TestAppInstanceReference testApp = sTestApp.install(sTestApis.users().instrumented())){
            sDeviceState.dpc().devicePolicyManager().setLockTaskPackages(new String[]{sTestApp.packageName()});
            Activity<TestAppActivity> activity = testApp.activities().any().start();

            try {
                activity.startLockTask();

                // TODO(b/191745956): Improve metrics query interface
                assertThat(metrics.query()
                        .whereType().isEqualTo(EventId.SET_LOCKTASK_MODE_ENABLED_VALUE)
                        .whereAdminPackageName().isEqualTo(
                                RemoteDpc.DPC_COMPONENT_NAME.getPackageName())
                        .whereBoolean().isTrue()
                        .whereStrings().contains(
                                string().isEqualTo(sTestApp.packageName())
                        )
                        .poll()).isNotNull();
            } finally {
                activity.stopLockTask();
            }
        } finally {
            sDeviceState.dpc().devicePolicyManager().setLockTaskPackages(originalLockTaskPackages);
        }
    }

    @Test
    @Postsubmit(reason = "New test")
    @CannotSetPolicyTest(policy = LockTask.class)
    public void getLockTaskPackages_policyIsNotAllowedToBeFetched_throwsException() {
        assertThrows(SecurityException.class,
                () -> sDeviceState.dpc().devicePolicyManager().getLockTaskPackages());
    }

    @Test
    @Postsubmit(reason = "New test")
    @PositivePolicyTest(policy = LockTask.class)
    public void setLockTaskPackages_empty_lockTaskPackagesIsSet() {
        String[] originalLockTaskPackages =
                sDeviceState.dpc().devicePolicyManager().getLockTaskPackages();

        sDeviceState.dpc().devicePolicyManager().setLockTaskPackages(new String[]{});

        try {
            assertThat(sDeviceState.dpc().devicePolicyManager().getLockTaskPackages()).asList()
                    .isEmpty();
        } finally {
            sDeviceState.dpc().devicePolicyManager().setLockTaskPackages(originalLockTaskPackages);
        }
    }

    @Test
    @Postsubmit(reason = "New test")
    @PositivePolicyTest(policy = LockTask.class)
    public void setLockTaskPackages_includesPolicyExemptApp_lockTaskPackagesIsSet() {
        Set<String> policyExemptApps = sTestApis.devicePolicy().getPolicyExemptApps();
        assumeFalse("OEM does not define any policy-exempt apps",
                policyExemptApps.isEmpty());
        String[] originalLockTaskPackages =
                sDeviceState.dpc().devicePolicyManager().getLockTaskPackages();
        String policyExemptApp = policyExemptApps.iterator().next();

        sDeviceState.dpc().devicePolicyManager().setLockTaskPackages(new String[]{policyExemptApp});

        try {
            assertThat(sDeviceState.dpc().devicePolicyManager().getLockTaskPackages()).asList()
                    .containsExactly(policyExemptApp);
        } finally {
            sDeviceState.dpc().devicePolicyManager().setLockTaskPackages(originalLockTaskPackages);
        }
    }

    @Test
    @Postsubmit(reason = "New test")
    @CannotSetPolicyTest(policy = LockTask.class)
    public void setLockTaskPackages_policyIsNotAllowedToBeSet_throwsException() {
        assertThrows(SecurityException.class,
                () -> sDeviceState.dpc().devicePolicyManager().setLockTaskPackages(new String[]{}));
    }

    @Test
    @Postsubmit(reason = "New test")
    @PositivePolicyTest(policy = LockTask.class)
    public void isLockTaskPermitted_lockTaskPackageIsSet_returnsTrue() {
        String[] originalLockTaskPackages =
                sDeviceState.dpc().devicePolicyManager().getLockTaskPackages();

        sDeviceState.dpc().devicePolicyManager().setLockTaskPackages(new String[]{PACKAGE_NAME});

        try {
            assertThat(sLocalDevicePolicyManager.isLockTaskPermitted(PACKAGE_NAME)).isTrue();
        } finally {
            sDeviceState.dpc().devicePolicyManager().setLockTaskPackages(originalLockTaskPackages);
        }
    }

    @Test
    @Postsubmit(reason = "New test")
    @NegativePolicyTest(policy = LockTask.class)
    // TODO(scottjonathan): Confirm expected behaviour here
    public void isLockTaskPermitted_lockTaskPackageIsSet_policyDoesntApply_returnsFalse() {
        String[] originalLockTaskPackages =
                sDeviceState.dpc().devicePolicyManager().getLockTaskPackages();

        sDeviceState.dpc().devicePolicyManager().setLockTaskPackages(new String[]{PACKAGE_NAME});

        try {
            assertThat(sLocalDevicePolicyManager.isLockTaskPermitted(PACKAGE_NAME)).isFalse();
        } finally {
            sDeviceState.dpc().devicePolicyManager().setLockTaskPackages(originalLockTaskPackages);
        }
    }

    @Test
    @Postsubmit(reason = "New test")
    @PositivePolicyTest(policy = LockTask.class)
    public void isLockTaskPermitted_lockTaskPackageIsNotSet_returnsFalse() {
        String[] originalLockTaskPackages =
                sDeviceState.dpc().devicePolicyManager().getLockTaskPackages();

        sDeviceState.dpc().devicePolicyManager().setLockTaskPackages(new String[]{});

        try {
            assertThat(sLocalDevicePolicyManager.isLockTaskPermitted(PACKAGE_NAME)).isFalse();
        } finally {
            sDeviceState.dpc().devicePolicyManager().setLockTaskPackages(originalLockTaskPackages);
        }
    }

    @Test
    @Postsubmit(reason = "New test")
    @PositivePolicyTest(policy = LockTask.class)
    public void isLockTaskPermitted_includesPolicyExemptApps() {
        Set<String> policyExemptApps = sTestApis.devicePolicy().getPolicyExemptApps();
        // TODO(b/188035301): Add a unit test which ensures this actually gets tested
        assumeFalse("OEM does not define any policy-exempt apps",
                policyExemptApps.isEmpty());
        String[] originalLockTaskPackages =
                sDeviceState.dpc().devicePolicyManager().getLockTaskPackages();

        try {
            sDeviceState.dpc().devicePolicyManager().setLockTaskPackages(new String[]{});

            for (String app : policyExemptApps) {
                assertWithMessage("isLockTaskPermitted(%s)", app)
                        .that(sLocalDevicePolicyManager.isLockTaskPermitted(app)).isTrue();
            }
        } finally {
            sDeviceState.dpc().devicePolicyManager().setLockTaskPackages(originalLockTaskPackages);
        }
    }

    @Test
    @Postsubmit(reason = "New test")
    @PositivePolicyTest(policy = LockTask.class)
    // TODO(scottjonathan): Support additional parameterization for cases like this
    public void setLockTaskFeatures_overviewFeature_setsFeature() {
        int originalLockTaskFeatures =
                sDeviceState.dpc().devicePolicyManager().getLockTaskFeatures();

        try {
            for (int flag : INDIVIDUALLY_SETTABLE_FLAGS) {
                sDeviceState.dpc().devicePolicyManager().setLockTaskFeatures(flag);

                assertThat(sDeviceState.dpc().devicePolicyManager().getLockTaskFeatures())
                        .isEqualTo(flag);
            }
        } finally {
            sDeviceState.dpc().devicePolicyManager().setLockTaskFeatures(originalLockTaskFeatures);
        }
    }


    @Test
    @Postsubmit(reason = "New test")
    @PositivePolicyTest(policy = LockTask.class)
    public void setLockTaskFeatures_overviewFeature_throwsException() {
        // Overview can only be used in combination with home
        int originalLockTaskFeatures =
                sDeviceState.dpc().devicePolicyManager().getLockTaskFeatures();

        try {
            assertThrows(IllegalArgumentException.class, () -> {
                sDeviceState.dpc().devicePolicyManager()
                        .setLockTaskFeatures(LOCK_TASK_FEATURE_OVERVIEW);
            });
        } finally {
            sDeviceState.dpc().devicePolicyManager().setLockTaskFeatures(originalLockTaskFeatures);
        }
    }

    @Test
    @Postsubmit(reason = "New test")
    @PositivePolicyTest(policy = LockTask.class)
    public void setLockTaskFeatures_notificationsFeature_throwsException() {
        // Notifications can only be used in combination with home
        int originalLockTaskFeatures =
                sDeviceState.dpc().devicePolicyManager().getLockTaskFeatures();

        try {
            assertThrows(IllegalArgumentException.class, () -> {
                sDeviceState.dpc().devicePolicyManager()
                        .setLockTaskFeatures(LOCK_TASK_FEATURE_NOTIFICATIONS);
            });
        } finally {
            sDeviceState.dpc().devicePolicyManager().setLockTaskFeatures(originalLockTaskFeatures);
        }
    }

    @Test
    @Postsubmit(reason = "New test")
    @PositivePolicyTest(policy = LockTask.class)
    // TODO(scottjonathan): Support additional parameterization for cases like this
    public void setLockTaskFeatures_multipleFeatures_setsFeatures() {
        int originalLockTaskFeatures =
                sDeviceState.dpc().devicePolicyManager().getLockTaskFeatures();

        try {
            for (int flag : FLAGS_SETTABLE_WITH_HOME) {
                sDeviceState.dpc().devicePolicyManager().setLockTaskFeatures(
                        LOCK_TASK_FEATURE_HOME | flag);

                assertThat(sDeviceState.dpc().devicePolicyManager().getLockTaskFeatures())
                        .isEqualTo(LOCK_TASK_FEATURE_HOME | flag);
            }
        } finally {
            sDeviceState.dpc().devicePolicyManager().setLockTaskFeatures(originalLockTaskFeatures);
        }
    }

    @Test
    @Postsubmit(reason = "New test")
    @CannotSetPolicyTest(policy = LockTask.class)
    public void setLockTaskFeatures_policyIsNotAllowedToBeSet_throwsException() {
        assertThrows(SecurityException.class, () ->
                sDeviceState.dpc().devicePolicyManager().setLockTaskFeatures(LOCK_TASK_FEATURE_NONE));
    }

    @Test
    @Postsubmit(reason = "New test")
    @CannotSetPolicyTest(policy = LockTask.class)
    public void getLockTaskFeatures_policyIsNotAllowedToBeFetched_throwsException() {
        assertThrows(SecurityException.class, () ->
                sDeviceState.dpc().devicePolicyManager().getLockTaskFeatures());
    }

    @Test
    @Postsubmit(reason = "New test")
    @PositivePolicyTest(policy = LockTask.class)
    public void startLockTask_includedInLockTaskPackages_taskIsLocked() {
        String[] originalLockTaskPackages =
                sDeviceState.dpc().devicePolicyManager().getLockTaskPackages();
        sDeviceState.dpc().devicePolicyManager().setLockTaskPackages(
                new String[]{sTestApp.packageName()});
        try (TestAppInstanceReference testApp =
                     sTestApp.install(sTestApis.users().instrumented())) {
            Activity<TestAppActivity> activity = testApp.activities().any().start();

            activity.startLockTask();

            try {
                assertThat(sTestApis.activities().foregroundActivity()).isEqualTo(
                        activity.activity().component());
                assertThat(sTestApis.activities().getLockTaskModeState()).isEqualTo(
                        LOCK_TASK_MODE_LOCKED);
            } finally {
                activity.stopLockTask();
            }
        } finally {
            sDeviceState.dpc().devicePolicyManager().setLockTaskPackages(originalLockTaskPackages);
        }
    }

    @Test
    @Postsubmit(reason = "New test")
    @PositivePolicyTest(policy = LockTask.class)
    public void startLockTask_notIncludedInLockTaskPackages_taskIsNotLocked() {
        String[] originalLockTaskPackages =
                sDeviceState.dpc().devicePolicyManager().getLockTaskPackages();
        sDeviceState.dpc().devicePolicyManager().setLockTaskPackages(new String[]{});
        try (TestAppInstanceReference testApp =
                     sTestApp.install(sTestApis.users().instrumented())) {
            Activity<TestAppActivity> activity = testApp.activities().any().start();

            activity.activity().startLockTask();

            try {
                assertThat(sTestApis.activities().foregroundActivity()).isEqualTo(
                        activity.activity().component());
                assertThat(sTestApis.activities().getLockTaskModeState()).isNotEqualTo(
                        LOCK_TASK_MODE_LOCKED);
            } finally {
                activity.stopLockTask();
            }
        } finally {
            sDeviceState.dpc().devicePolicyManager().setLockTaskPackages(originalLockTaskPackages);
        }
    }

    @Test
    @Postsubmit(reason = "New test")
    @NegativePolicyTest(policy = LockTask.class)
    public void startLockTask_includedInLockTaskPackages_policyShouldNotApply_taskIsNotLocked() {
        String[] originalLockTaskPackages =
                sDeviceState.dpc().devicePolicyManager().getLockTaskPackages();
        sDeviceState.dpc().devicePolicyManager().setLockTaskPackages(
                new String[]{sTestApp.packageName()});
        try (TestAppInstanceReference testApp =
                     sTestApp.install(sTestApis.users().instrumented())) {
            Activity<TestAppActivity> activity = testApp.activities().any().start();

            activity.activity().startLockTask();

            try {
                assertThat(sTestApis.activities().foregroundActivity()).isEqualTo(
                        activity.activity().component());
                assertThat(sTestApis.activities().getLockTaskModeState()).isNotEqualTo(
                        LOCK_TASK_MODE_LOCKED);
            } finally {
                activity.stopLockTask();
            }
        } finally {
            sDeviceState.dpc().devicePolicyManager().setLockTaskPackages(originalLockTaskPackages);
        }
    }

    @Test
    @Postsubmit(reason = "New test")
    @PositivePolicyTest(policy = LockTask.class)
    public void finish_isLocked_doesNotFinish() {
        String[] originalLockTaskPackages =
                sDeviceState.dpc().devicePolicyManager().getLockTaskPackages();
        sDeviceState.dpc().devicePolicyManager().setLockTaskPackages(
                new String[]{sTestApp.packageName()});
        try (TestAppInstanceReference testApp =
                     sTestApp.install(sTestApis.users().instrumented())) {
            Activity<TestAppActivity> activity = testApp.activities().any().start();
            activity.startLockTask();

            activity.activity().finish();

            try {
                // We don't actually watch for the Destroyed event because that'd be waiting for a
                // non occurrence of an event which is slow
                assertThat(sTestApis.activities().foregroundActivity()).isEqualTo(
                        activity.activity().component());
                assertThat(sTestApis.activities().getLockTaskModeState()).isEqualTo(
                        LOCK_TASK_MODE_LOCKED);
            } finally {
                activity.stopLockTask();
            }
        } finally {
            sDeviceState.dpc().devicePolicyManager().setLockTaskPackages(originalLockTaskPackages);
        }
    }

    @Test
    @Postsubmit(reason = "New test")
    @PositivePolicyTest(policy = LockTask.class)
    public void finish_hasStoppedLockTask_doesFinish() {
        String[] originalLockTaskPackages =
                sDeviceState.dpc().devicePolicyManager().getLockTaskPackages();
        sDeviceState.dpc().devicePolicyManager().setLockTaskPackages(
                new String[]{sTestApp.packageName()});
        try (TestAppInstanceReference testApp =
                     sTestApp.install(sTestApis.users().instrumented())) {
            Activity<TestAppActivity> activity = testApp.activities().any().start();
            activity.startLockTask();
            activity.stopLockTask();

            activity.activity().finish();

            // TODO(b/189327037): Replace with more direct integration between TestApp and EventLib
            EventLogs<ActivityDestroyedEvent> events =
                    ActivityDestroyedEvent.queryPackage(sTestApp.packageName())
                    .whereActivity().activityClass().className().isEqualTo(
                            activity.activity().component().className());
            assertThat(events.poll()).isNotNull();
            assertThat(sTestApis.activities().foregroundActivity()).isNotEqualTo(
                    activity.activity().component());
        } finally {
            sDeviceState.dpc().devicePolicyManager().setLockTaskPackages(originalLockTaskPackages);
        }
    }

    @Test
    @Postsubmit(reason = "New test")
    @PositivePolicyTest(policy = LockTask.class)
    public void setLockTaskPackages_removingCurrentlyLockedTask_taskFinishes() {
        String[] originalLockTaskPackages =
                sDeviceState.dpc().devicePolicyManager().getLockTaskPackages();
        sDeviceState.dpc().devicePolicyManager().setLockTaskPackages(
                new String[]{sTestApp.packageName()});
        try (TestAppInstanceReference testApp =
                     sTestApp.install(sTestApis.users().instrumented())) {
            Activity<TestAppActivity> activity = testApp.activities().any().start();
            activity.startLockTask();

            sDeviceState.dpc().devicePolicyManager().setLockTaskPackages(new String[]{});

            // TODO(b/189327037): Replace with more direct integration between TestApp and EventLib
            EventLogs<ActivityDestroyedEvent> events =
                    ActivityDestroyedEvent.queryPackage(sTestApp.packageName())
                            .whereActivity().activityClass().className().isEqualTo(
                                    activity.activity().component().className());
            assertThat(events.poll()).isNotNull();
            assertThat(sTestApis.activities().foregroundActivity()).isNotEqualTo(
                    activity.activity().component());
        } finally {
            sDeviceState.dpc().devicePolicyManager().setLockTaskPackages(originalLockTaskPackages);
        }
    }

    @Test
    @Postsubmit(reason = "New test")
    @PositivePolicyTest(policy = LockTask.class)
    public void setLockTaskPackages_removingCurrentlyLockedTask_otherLockedTasksRemainLocked() {
        String[] originalLockTaskPackages =
                sDeviceState.dpc().devicePolicyManager().getLockTaskPackages();
        sDeviceState.dpc().devicePolicyManager().setLockTaskPackages(
                new String[]{sTestApp.packageName(), sSecondTestApp.packageName()});
        try (TestAppInstanceReference testApp =
                     sTestApp.install(sTestApis.users().instrumented());
             TestAppInstanceReference testApp2 =
                     sSecondTestApp.install(sTestApis.users().instrumented())) {
            Activity<TestAppActivity> activity = testApp.activities().any().start();
            activity.startLockTask();
            Activity<TestAppActivity> activity2 = testApp2.activities().any().start();
            activity2.startLockTask();

            try {
                sDeviceState.dpc().devicePolicyManager().setLockTaskPackages(
                        new String[]{sTestApp.packageName()});

                // TODO(b/189327037): Replace with more direct integration between TestApp and EventLib
                EventLogs<ActivityDestroyedEvent> events =
                        ActivityDestroyedEvent.queryPackage(sSecondTestApp.packageName())
                                .whereActivity().activityClass().className().isEqualTo(
                                activity2.activity().component().className());
                assertThat(events.poll()).isNotNull();
                assertThat(sTestApis.activities().getLockTaskModeState()).isEqualTo(
                        LOCK_TASK_MODE_LOCKED);
                assertThat(sTestApis.activities().foregroundActivity()).isEqualTo(
                        activity.activity().component());
            } finally {
                activity.stopLockTask();
            }
        } finally {
            sDeviceState.dpc().devicePolicyManager().setLockTaskPackages(originalLockTaskPackages);
        }
    }

    @Test
    @Postsubmit(reason = "New test")
    @PositivePolicyTest(policy = LockTask.class)
    public void startActivity_withinSameTask_startsActivity() {
        String[] originalLockTaskPackages =
                sDeviceState.dpc().devicePolicyManager().getLockTaskPackages();
        try (TestAppInstanceReference testApp =
                     sTestApp.install(sTestApis.users().instrumented());
             TestAppInstanceReference testApp2 =
                     sSecondTestApp.install(sTestApis.users().instrumented())) {
            sDeviceState.dpc().devicePolicyManager().setLockTaskPackages(
                    new String[]{sTestApp.packageName()});
            Activity<TestAppActivity> firstActivity = testApp.activities().any().start();
            TestAppActivityReference secondActivity = testApp2.activities().any();
            Intent secondActivityIntent = new Intent();
            // TODO(scottjonathan): Add filter to ensure no taskAffinity or launchMode which would
            //  stop launching in same task
            secondActivityIntent.setComponent(secondActivity.component().componentName());

            firstActivity.startActivity(secondActivityIntent);

            EventLogs<ActivityStartedEvent> events =
                    ActivityStartedEvent.queryPackage(sSecondTestApp.packageName())
                            .whereActivity().activityClass().className().isEqualTo(
                                    secondActivity.component().className());
            assertThat(events.poll()).isNotNull();
            assertThat(sTestApis.activities().foregroundActivity()).isEqualTo(secondActivity.component());
        } finally {
            sDeviceState.dpc().devicePolicyManager().setLockTaskPackages(originalLockTaskPackages);
        }
    }

    @Test
    @Postsubmit(reason = "New test")
    @PositivePolicyTest(policy = LockTask.class)
    public void startActivity_withinSameTask_blockStartInTask_doesNotStart() {
        String[] originalLockTaskPackages =
                sDeviceState.dpc().devicePolicyManager().getLockTaskPackages();
        int originalLockTaskFeatures =
                sDeviceState.dpc().devicePolicyManager().getLockTaskFeatures();
        try (TestAppInstanceReference testApp =
                     sTestApp.install(sTestApis.users().instrumented());
             TestAppInstanceReference testApp2 =
                     sSecondTestApp.install(sTestApis.users().instrumented())) {
            try {
                sDeviceState.dpc().devicePolicyManager().setLockTaskPackages(
                        new String[]{sTestApp.packageName()});
                sDeviceState.dpc().devicePolicyManager().setLockTaskFeatures(
                        LOCK_TASK_FEATURE_BLOCK_ACTIVITY_START_IN_TASK);
                Activity<TestAppActivity> firstActivity = testApp.activities().any().start();
                firstActivity.startLockTask();
                TestAppActivityReference secondActivity = testApp2.activities().any();
                Intent secondActivityIntent = new Intent();
                secondActivityIntent.setComponent(secondActivity.component().componentName());

                firstActivity.activity().startActivity(secondActivityIntent);

                assertThat(sTestApis.activities().foregroundActivity())
                        .isEqualTo(BLOCKED_ACTIVITY_COMPONENT);
            } finally {
                sDeviceState.dpc().devicePolicyManager().setLockTaskPackages(
                        originalLockTaskPackages);
                sDeviceState.dpc().devicePolicyManager().setLockTaskFeatures(
                        originalLockTaskFeatures);
            }
        }
    }

    @Test
    @Postsubmit(reason = "New test")
    @PositivePolicyTest(policy = LockTask.class)
    public void startActivity_inNewTask_blockStartInTask_doesNotStart() {
        String[] originalLockTaskPackages =
                sDeviceState.dpc().devicePolicyManager().getLockTaskPackages();
        int originalLockTaskFeatures =
                sDeviceState.dpc().devicePolicyManager().getLockTaskFeatures();
        try (TestAppInstanceReference testApp =
                     sTestApp.install(sTestApis.users().instrumented());
             TestAppInstanceReference testApp2 =
                     sSecondTestApp.install(sTestApis.users().instrumented())) {
            try {
                sDeviceState.dpc().devicePolicyManager().setLockTaskPackages(
                        new String[]{sTestApp.packageName()});
                sDeviceState.dpc().devicePolicyManager().setLockTaskFeatures(
                        LOCK_TASK_FEATURE_BLOCK_ACTIVITY_START_IN_TASK);
                Activity<TestAppActivity> firstActivity = testApp.activities().any().start();
                firstActivity.startLockTask();
                TestAppActivityReference secondActivity = testApp2.activities().any();
                Intent secondActivityIntent = new Intent();
                secondActivityIntent.setComponent(secondActivity.component().componentName());
                secondActivityIntent.addFlags(Intent.FLAG_ACTIVITY_NEW_TASK);

                firstActivity.activity().startActivity(secondActivityIntent);

                assertThat(sTestApis.activities().foregroundActivity())
                        .isEqualTo(BLOCKED_ACTIVITY_COMPONENT);
            } finally {
                sDeviceState.dpc().devicePolicyManager().setLockTaskPackages(
                        originalLockTaskPackages);
                sDeviceState.dpc().devicePolicyManager().setLockTaskFeatures(
                        originalLockTaskFeatures);
            }
        }
    }

    @Test
    @Postsubmit(reason = "New test")
    @PositivePolicyTest(policy = LockTask.class)
    public void startActivity_fromPermittedPackage_newTask_starts() {
        String[] originalLockTaskPackages =
                sDeviceState.dpc().devicePolicyManager().getLockTaskPackages();

        try (TestAppInstanceReference testApp =
                     sTestApp.install(sTestApis.users().instrumented());
             TestAppInstanceReference testApp2 =
                     sSecondTestApp.install(sTestApis.users().instrumented())) {
            try {
                sDeviceState.dpc().devicePolicyManager().setLockTaskPackages(
                        new String[]{sTestApp.packageName(), sSecondTestApp.packageName()});
                Activity<TestAppActivity> firstActivity = testApp.activities().any().start();
                firstActivity.startLockTask();
                TestAppActivityReference secondActivity = testApp2.activities().any();
                Intent secondActivityIntent = new Intent();
                secondActivityIntent.setComponent(secondActivity.component().componentName());
                secondActivityIntent.addFlags(Intent.FLAG_ACTIVITY_NEW_TASK);

                firstActivity.startActivity(secondActivityIntent);

                assertThat(sTestApis.activities().foregroundActivity())
                        .isEqualTo(secondActivity.component());
            } finally {
                sDeviceState.dpc().devicePolicyManager().setLockTaskPackages(
                        originalLockTaskPackages);
            }
        }
    }

    @Test
    @Postsubmit(reason = "New test")
    @PositivePolicyTest(policy = LockTask.class)
    public void startActivity_fromNonPermittedPackage_newTask_doesNotStart() {
        String[] originalLockTaskPackages =
                sDeviceState.dpc().devicePolicyManager().getLockTaskPackages();

        try (TestAppInstanceReference testApp =
                     sTestApp.install(sTestApis.users().instrumented());
             TestAppInstanceReference testApp2 =
                     sSecondTestApp.install(sTestApis.users().instrumented())) {
            try {
                sDeviceState.dpc().devicePolicyManager().setLockTaskPackages(
                        new String[]{sTestApp.packageName()});
                Activity<TestAppActivity> firstActivity = testApp.activities().any().start();
                firstActivity.startLockTask();
                TestAppActivityReference secondActivity = testApp2.activities().any();
                Intent secondActivityIntent = new Intent();
                secondActivityIntent.setComponent(secondActivity.component().componentName());
                secondActivityIntent.addFlags(Intent.FLAG_ACTIVITY_NEW_TASK);

                firstActivity.activity().startActivity(secondActivityIntent);

                assertThat(sTestApis.activities().foregroundActivity())
                        .isEqualTo(firstActivity.activity().component());
            } finally {
                sDeviceState.dpc().devicePolicyManager().setLockTaskPackages(
                        originalLockTaskPackages);
            }
        }
    }

    @Test
    @Postsubmit(reason = "New test")
    @PositivePolicyTest(policy = LockTask.class)
    public void startActivity_lockTaskEnabledOption_startsInLockTaskMode() {
        String[] originalLockTaskPackages =
                sDeviceState.dpc().devicePolicyManager().getLockTaskPackages();

        try (TestAppInstanceReference testApp =
                     sTestApp.install(sTestApis.users().instrumented())) {
            try {
                sDeviceState.dpc().devicePolicyManager().setLockTaskPackages(
                        new String[]{sTestApp.packageName()});
                Bundle options = ActivityOptions.makeBasic().setLockTaskEnabled(true).toBundle();
                Activity<TestAppActivity> activity = testApp.activities().any().start(options);

                try {
                    assertThat(sTestApis.activities().foregroundActivity()).isEqualTo(
                            activity.activity().component());
                    assertThat(sTestApis.activities().getLockTaskModeState()).isEqualTo(
                            LOCK_TASK_MODE_LOCKED);
                } finally {
                    activity.stopLockTask();
                }
            } finally {
                sDeviceState.dpc().devicePolicyManager().setLockTaskPackages(
                        originalLockTaskPackages);
            }
        }
    }

    @Test
    @Postsubmit(reason = "New test")
    @PositivePolicyTest(policy = LockTask.class)
    public void startActivity_lockTaskEnabledOption_notAllowedPackage_throwsException() {
        String[] originalLockTaskPackages =
                sDeviceState.dpc().devicePolicyManager().getLockTaskPackages();

        try (TestAppInstanceReference testApp =
                     sTestApp.install(sTestApis.users().instrumented())) {
            try {
                sDeviceState.dpc().devicePolicyManager().setLockTaskPackages(
                        new String[]{});
                Bundle options = ActivityOptions.makeBasic().setLockTaskEnabled(true).toBundle();

                assertThrows(SecurityException.class, () -> {
                    testApp.activities().any().start(options);
                });
            } finally {
                sDeviceState.dpc().devicePolicyManager().setLockTaskPackages(
                        originalLockTaskPackages);
            }
        }
    }

    @Test
    @Postsubmit(reason = "New test")
    @PositivePolicyTest(policy = LockTask.class)
    public void startActivity_ifWhitelistedActivity_startsInLockTaskMode() {
        String[] originalLockTaskPackages =
                sDeviceState.dpc().devicePolicyManager().getLockTaskPackages();

        try (TestAppInstanceReference testApp =
                     sLockTaskTestApp.install(sTestApis.users().instrumented())) {
            sDeviceState.dpc().devicePolicyManager().setLockTaskPackages(
                    new String[]{sLockTaskTestApp.packageName()});
            Activity<TestAppActivity> activity = testApp.activities().query()
                    .whereActivity().activityClass().simpleName().isEqualTo("ifwhitelistedactivity")
                    // TODO(scottjonathan): filter for lock task mode - currently we can't check
                    //  this so we just get a fixed package which contains a fixed activity
                    .get().start();

            try {
                assertThat(sTestApis.activities().foregroundActivity()).isEqualTo(
                        activity.activity().component());
                assertThat(sTestApis.activities().getLockTaskModeState()).isEqualTo(
                        LOCK_TASK_MODE_LOCKED);
            } finally {
                activity.stopLockTask();
            }
        } finally {
            sDeviceState.dpc().devicePolicyManager().setLockTaskPackages(
                    originalLockTaskPackages);
        }
    }

    @Test
    @Postsubmit(reason = "New test")
    @PositivePolicyTest(policy = LockTask.class)
    public void startActivity_ifWhitelistedActivity_notWhitelisted_startsNotInLockTaskMode() {
        String[] originalLockTaskPackages =
                sDeviceState.dpc().devicePolicyManager().getLockTaskPackages();

        try (TestAppInstanceReference testApp =
                     sLockTaskTestApp.install(sTestApis.users().instrumented())) {
            sDeviceState.dpc().devicePolicyManager().setLockTaskPackages(
                    new String[]{});
            Activity<TestAppActivity> activity = testApp.activities().query()
                    .whereActivity().activityClass().simpleName().isEqualTo("ifwhitelistedactivity")
                    // TODO(scottjonathan): filter for lock task mode - currently we can't check
                    //  this so we just get a fixed package which contains a fixed activity
                    .get().start();

            try {
                assertThat(sTestApis.activities().foregroundActivity()).isEqualTo(
                        activity.activity().component());
                assertThat(sTestApis.activities().getLockTaskModeState()).isEqualTo(
                        LOCK_TASK_MODE_NONE);
            } finally {
                activity.stopLockTask();
            }
        } finally {
            sDeviceState.dpc().devicePolicyManager().setLockTaskPackages(
                    originalLockTaskPackages);
        }
    }

    @Test
    @Postsubmit(reason = "New test")
    @PositivePolicyTest(policy = LockTask.class)
    public void finish_ifWhitelistedActivity_doesNotFinish() {
        String[] originalLockTaskPackages =
                sDeviceState.dpc().devicePolicyManager().getLockTaskPackages();

        try (TestAppInstanceReference testApp =
                     sLockTaskTestApp.install(sTestApis.users().instrumented())) {
            sDeviceState.dpc().devicePolicyManager().setLockTaskPackages(
                    new String[]{sLockTaskTestApp.packageName()});
            Activity<TestAppActivity> activity = testApp.activities().query()
                    .whereActivity().activityClass().simpleName().isEqualTo("ifwhitelistedactivity")
                    // TODO(scottjonathan): filter for lock task mode - currently we can't check
                    //  this so we just get a fixed package which contains a fixed activity
                    .get().start();

            activity.activity().finish();

            try {
                // We don't actually watch for the Destroyed event because that'd be waiting for a
                // non occurrence of an event which is slow
                assertThat(sTestApis.activities().foregroundActivity()).isEqualTo(
                        activity.activity().component());
                assertThat(sTestApis.activities().getLockTaskModeState()).isEqualTo(
                        LOCK_TASK_MODE_LOCKED);
            } finally {
                activity.stopLockTask();
            }
        } finally {
            sDeviceState.dpc().devicePolicyManager().setLockTaskPackages(
                    originalLockTaskPackages);
        }
    }

    @Test
    @Postsubmit(reason = "New test")
    @PositivePolicyTest(policy = LockTask.class)
    public void setLockTaskPackages_removingExistingIfWhitelistedActivity_stopsTask() {
        String[] originalLockTaskPackages =
                sDeviceState.dpc().devicePolicyManager().getLockTaskPackages();

        try (TestAppInstanceReference testApp =
                     sLockTaskTestApp.install(sTestApis.users().instrumented())) {
            sDeviceState.dpc().devicePolicyManager().setLockTaskPackages(
                    new String[]{sLockTaskTestApp.packageName()});
            Activity<TestAppActivity> activity = testApp.activities().query()
                    .whereActivity().activityClass().simpleName().isEqualTo("ifwhitelistedactivity")
                    // TODO(scottjonathan): filter for lock task mode - currently we can't check
                    //  this so we just get a fixed package which contains a fixed activity
                    .get().start();

            sDeviceState.dpc().devicePolicyManager().setLockTaskPackages(new String[]{});

            EventLogs<ActivityDestroyedEvent> events =
                    ActivityDestroyedEvent.queryPackage(sLockTaskTestApp.packageName())
                            .whereActivity().activityClass().className().isEqualTo(
                            activity.activity().component().className());
            assertThat(events.poll()).isNotNull();
            assertThat(sTestApis.activities().foregroundActivity()).isNotEqualTo(
                    activity.activity().component());
        } finally {
            sDeviceState.dpc().devicePolicyManager().setLockTaskPackages(
                    originalLockTaskPackages);
        }
    }

    @Test
    @Postsubmit(reason = "New test")
    @PositivePolicyTest(policy = LockTask.class)
    @RequireFeature(FEATURE_TELEPHONY)
    // Tests that the default dialer doesn't crash or otherwise misbehave in lock task mode
    public void launchDefaultDialerInLockTaskMode_launches() {
        String[] originalLockTaskPackages =
                sDeviceState.dpc().devicePolicyManager().getLockTaskPackages();
        TelecomManager telecomManager =
                sTestApis.context().instrumentedContext().getSystemService(TelecomManager.class);
        String dialerPackage = telecomManager.getSystemDialerPackage();
        try {
            sDeviceState.dpc().devicePolicyManager().setLockTaskPackages(
                    new String[]{dialerPackage});

            Bundle options = ActivityOptions.makeBasic().setLockTaskEnabled(true).toBundle();
            Intent intent = new Intent(ACTION_DIAL);
            intent.setPackage(dialerPackage);
            intent.setFlags(FLAG_ACTIVITY_NEW_TASK | FLAG_ACTIVITY_CLEAR_TASK);

            sTestApis.context().instrumentedContext().startActivity(intent, options);
            PollingCheck.waitFor(() -> {
                PackageReference pkg = sTestApis.activities().foregroundActivity().packageName();
                if (pkg == null) {
                    return false;
                }
                return pkg.packageName().equals(dialerPackage);
            });

            assertThat(sTestApis.activities().getLockTaskModeState()).isEqualTo(
                    LOCK_TASK_MODE_LOCKED);
        } finally {
            sDeviceState.dpc().devicePolicyManager().setLockTaskPackages(
                    originalLockTaskPackages);
        }
    }

    @Test
    @Postsubmit(reason = "New test")
    @PositivePolicyTest(policy = LockTask.class)
    @RequireFeature(FEATURE_TELEPHONY)
    public void launchEmergencyDialerInLockTaskMode_notWhitelisted_noKeyguardFeature_doesNotLaunch() {
        String[] originalLockTaskPackages =
                sDeviceState.dpc().devicePolicyManager().getLockTaskPackages();
        int originalLockTaskFeatures =
                sDeviceState.dpc().devicePolicyManager().getLockTaskFeatures();
        String emergencyDialerPackageName = getEmergencyDialerPackageName();
        assumeFalse(emergencyDialerPackageName == null);
        try (TestAppInstanceReference testApp =
                     sLockTaskTestApp.install(sTestApis.users().instrumented())) {
            sDeviceState.dpc().devicePolicyManager().setLockTaskPackages(
                    new String[]{sLockTaskTestApp.packageName()});
            sDeviceState.dpc().devicePolicyManager().setLockTaskFeatures(0);
            Activity<TestAppActivity> activity = testApp.activities().any().start();

            try {
                activity.startLockTask();
                Intent intent = new Intent(ACTION_EMERGENCY_DIAL);
                intent.setFlags(FLAG_ACTIVITY_NEW_TASK | FLAG_ACTIVITY_CLEAR_TASK);

                activity.activity().startActivity(intent);

                if (sTestApis.activities().foregroundActivity() != null) {
                    assertThat(sTestApis.activities().foregroundActivity().packageName()).isNotEqualTo(
                            emergencyDialerPackageName);
                }
            } finally {
                activity.stopLockTask();
            }
        } finally {
            sDeviceState.dpc().devicePolicyManager().setLockTaskPackages(
                    originalLockTaskPackages);
            sDeviceState.dpc().devicePolicyManager().setLockTaskFeatures(originalLockTaskFeatures);
        }
    }

    @Test
    @Postsubmit(reason = "New test")
    @PositivePolicyTest(policy = LockTask.class)
    @RequireFeature(FEATURE_TELEPHONY)
    public void launchEmergencyDialerInLockTaskMode_notWhitelisted_keyguardFeature_launches() {
        String[] originalLockTaskPackages =
                sDeviceState.dpc().devicePolicyManager().getLockTaskPackages();
        int originalLockTaskFeatures =
                sDeviceState.dpc().devicePolicyManager().getLockTaskFeatures();
        String emergencyDialerPackageName = getEmergencyDialerPackageName();
        assumeFalse(emergencyDialerPackageName == null);
        try (TestAppInstanceReference testApp =
                     sLockTaskTestApp.install(sTestApis.users().instrumented())) {
            sDeviceState.dpc().devicePolicyManager().setLockTaskPackages(
                    new String[]{sLockTaskTestApp.packageName()});
            sDeviceState.dpc().devicePolicyManager().setLockTaskFeatures(LOCK_TASK_FEATURE_KEYGUARD);
            Activity<TestAppActivity> activity = testApp.activities().any().start();
            try {
                activity.startLockTask();
                Intent intent = new Intent(ACTION_EMERGENCY_DIAL);
                intent.setFlags(FLAG_ACTIVITY_NEW_TASK | FLAG_ACTIVITY_CLEAR_TASK);

                activity.startActivity(intent);

                assertThat(sTestApis.activities().foregroundActivity().packageName())
                        .isEqualTo(sTestApis.packages().find(emergencyDialerPackageName));
                assertThat(sTestApis.activities().getLockTaskModeState()).isEqualTo(
                        LOCK_TASK_MODE_LOCKED);
            } finally {
                activity.stopLockTask();
            }
        } finally {
            sDeviceState.dpc().devicePolicyManager().setLockTaskPackages(
                    originalLockTaskPackages);
            sDeviceState.dpc().devicePolicyManager().setLockTaskFeatures(originalLockTaskFeatures);
        }
    }

    private String getEmergencyDialerPackageName() {
        PackageManager packageManager =
                sTestApis.context().instrumentedContext().getPackageManager();
        Intent intent = new Intent(ACTION_EMERGENCY_DIAL).addFlags(Intent.FLAG_ACTIVITY_NEW_TASK);
        ResolveInfo dialerInfo =
                packageManager.resolveActivity(intent, PackageManager.MATCH_DEFAULT_ONLY);
        return (dialerInfo != null) ? dialerInfo.activityInfo.packageName : null;
    }
}
