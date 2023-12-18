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

package com.android.permissioncontroller.permission.ui.handheld

import androidx.test.rule.ActivityTestRule
import com.android.permissioncontroller.permission.ui.UiBaseTest
import org.junit.Assume.assumeFalse
import org.junit.Before
import org.junit.Rule
import org.junit.rules.TestRule

abstract class HandheldUiBaseTest : UiBaseTest() {
    /**
     * This is not great, but we should not run the [TestRules][TestRule] on TVs.
     * We skip the test themselves [by failing an assumption][assumeFalse] in [Before] methods,
     * but the TestRules are executed before the Before methods.
     * [BeforeClass][org.junit.BeforeClass] does run before TestRules do, but failing am assumption
     * there fails the test instead of skipping it.
     */
    @Rule
    fun activityRule() = if (isTelevision) noOpTestRule else provideActivityRule()

    abstract fun provideActivityRule(): ActivityTestRule<*>

    @Before
    fun assumeNotTelevision() = assumeFalse(isTelevision)
}