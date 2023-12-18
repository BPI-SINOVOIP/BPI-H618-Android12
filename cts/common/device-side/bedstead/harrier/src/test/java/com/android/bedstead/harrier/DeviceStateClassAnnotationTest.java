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

package com.android.bedstead.harrier;

import static com.android.bedstead.nene.users.UserType.MANAGED_PROFILE_TYPE_NAME;
import static com.android.bedstead.nene.users.UserType.SECONDARY_USER_TYPE_NAME;

import static com.google.common.truth.Truth.assertThat;

import com.android.bedstead.harrier.annotations.AfterClass;
import com.android.bedstead.harrier.annotations.BeforeClass;
import com.android.bedstead.harrier.annotations.EnsureHasNoSecondaryUser;
import com.android.bedstead.harrier.annotations.EnsureHasNoWorkProfile;
import com.android.bedstead.harrier.annotations.EnsureHasSecondaryUser;
import com.android.bedstead.harrier.annotations.EnsureHasWorkProfile;
import com.android.bedstead.nene.TestApis;

import org.junit.ClassRule;
import org.junit.Rule;
import org.junit.Test;
import org.junit.runner.RunWith;

@RunWith(BedsteadJUnit4.class)
@EnsureHasWorkProfile
@EnsureHasNoSecondaryUser
public class DeviceStateClassAnnotationTest extends DeviceStateTestParent {

    @ClassRule
    @Rule
    public static final DeviceState sDeviceState = new DeviceState();

    private static final TestApis sTestApis = new TestApis();

    private static boolean sBeforeClassHasRun = false;
    private static boolean sShadowedGrandparentWithoutBeforeClassInTestClassHasRun = false;

    // Shadowing
    @BeforeClass
    public static void shadowedGrandparentBeforeClass() {
        DeviceStateTestGrandparent.shadowedGrandparentBeforeClass();
    }

    // Shadowing
    public static void shadowedGrandparentWithoutBeforeClass() {
        sShadowedGrandparentWithoutBeforeClassInTestClassHasRun = true;
        DeviceStateTestGrandparent.shadowedGrandparentWithoutBeforeClass();
    }

    @BeforeClass
    public static void beforeClass() {
        // We test here that ensureHasWorkProfile is processed before the test method
        // but ensureHasSecondaryUser is not processed

        assertThat(sTestApis.users().findProfileOfType(
                sTestApis.users().supportedType(MANAGED_PROFILE_TYPE_NAME),
                sTestApis.users().instrumented())
        ).isNotNull();
        assertThat(sTestApis.users().findUserOfType(
                sTestApis.users().supportedType(SECONDARY_USER_TYPE_NAME))
        ).isNull();

        // Test that the parent always runs before the child
        assertThat(sParentBeforeClassHasRun).isTrue();

        sBeforeClassHasRun = true;
    }

    @AfterClass
    public static void afterClass() {
        // State is not guaranteed afterClass as changes made by tests can propagate
    }

    @Test
    public void beforeClassHasRunInCorrectOrder() {
        // The @BeforeClass on this class have run
        assertThat(sBeforeClassHasRun).isTrue();
        // The @BeforeClass from an ancestor class has run even though shadowed
        assertThat(sShadowedGrandparentBeforeClassHasRun).isTrue();

        assertThat(sShadowedGrandparentWithoutBeforeClassInTestClassHasRun).isFalse();
        assertThat(sShadowedGrandparentWithoutBeforeClassHasRun).isTrue();
    }

    @Test
    public void ensureHasWorkProfileAnnotationOnClass_workProfileExists() {
        assertThat(sTestApis.users().findProfileOfType(
                sTestApis.users().supportedType(MANAGED_PROFILE_TYPE_NAME),
                sTestApis.users().instrumented())
        ).isNotNull();
    }

    @Test
    @EnsureHasNoWorkProfile
    public void ensureHasNoWorkProfileAnnotation_overridesClassAnnotation() {
        assertThat(sTestApis.users().findProfileOfType(
                sTestApis.users().supportedType(MANAGED_PROFILE_TYPE_NAME),
                sTestApis.users().instrumented())
        ).isNull();
    }

    @Test
    @EnsureHasSecondaryUser
    public void ensureHasSecondaryUserAnnotation_overridesClassAnnotation() {
        assertThat(sTestApis.users().findUserOfType(
                sTestApis.users().supportedType(SECONDARY_USER_TYPE_NAME))
        ).isNotNull();
    }
}
