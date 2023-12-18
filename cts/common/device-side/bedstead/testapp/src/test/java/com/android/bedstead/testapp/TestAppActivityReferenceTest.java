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

package com.android.bedstead.testapp;

import static com.google.common.truth.Truth.assertThat;

import com.android.bedstead.harrier.BedsteadJUnit4;
import com.android.bedstead.harrier.DeviceState;
import com.android.bedstead.harrier.annotations.parameterized.IncludeRunOnPrimaryUser;
import com.android.bedstead.harrier.annotations.parameterized.IncludeRunOnSecondaryUser;
import com.android.bedstead.harrier.annotations.parameterized.IncludeRunOnProfileOwnerProfile;
import com.android.bedstead.nene.TestApis;
import com.android.bedstead.nene.activities.Activity;
import com.android.bedstead.nene.users.UserReference;

import org.junit.Before;
import org.junit.ClassRule;
import org.junit.Rule;
import org.junit.Test;
import org.junit.runner.RunWith;

@RunWith(BedsteadJUnit4.class)
public class TestAppActivityReferenceTest {

    private static final TestApis sTestApis = new TestApis();
    private static final UserReference sUser = sTestApis.users().instrumented();

    @ClassRule @Rule
    public static final DeviceState sDeviceState = new DeviceState();

    private TestAppProvider mTestAppProvider;

    @Before
    public void setup() {
        mTestAppProvider = new TestAppProvider();
    }

    @Test
    @IncludeRunOnPrimaryUser
    @IncludeRunOnSecondaryUser
    @IncludeRunOnProfileOwnerProfile
    public void start_activityIsStarted() {
        TestApp testApp = mTestAppProvider.any(); // TODO(scottjonathan): specify must have activity
        try (TestAppInstanceReference testAppInstance = testApp.install(sUser)) {
            Activity<TestAppActivity> activity = testAppInstance.activities().any().start();

            assertThat(sTestApis.activities().foregroundActivity()).isEqualTo(
                    activity.activity().component());
        }
    }

    @Test
    public void remote_executes() {
        TestApp testApp = mTestAppProvider.any(); // TODO(scottjonathan): specify must have activity
        try (TestAppInstanceReference testAppInstance = testApp.install(sUser)) {
            Activity<TestAppActivity> activity = testAppInstance.activities().any().start();

            assertThat(activity.activity().isFinishing()).isFalse();
        }
    }
}
