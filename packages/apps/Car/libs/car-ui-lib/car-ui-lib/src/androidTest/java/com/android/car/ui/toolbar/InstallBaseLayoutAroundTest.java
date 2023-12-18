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
package com.android.car.ui.toolbar;

import static androidx.test.espresso.Espresso.onView;
import static androidx.test.espresso.assertion.ViewAssertions.doesNotExist;
import static androidx.test.espresso.assertion.ViewAssertions.matches;
import static androidx.test.espresso.matcher.ViewMatchers.isAssignableFrom;
import static androidx.test.espresso.matcher.ViewMatchers.isDisplayed;
import static androidx.test.espresso.matcher.ViewMatchers.withText;

import static junit.framework.Assert.assertNotNull;
import static junit.framework.Assert.assertNull;
import static junit.framework.Assert.assertTrue;

import androidx.test.ext.junit.rules.ActivityScenarioRule;

import com.android.car.ui.FocusParkingView;
import com.android.car.ui.TrulyEmptyActivity;
import com.android.car.ui.baselayout.Insets;
import com.android.car.ui.core.CarUi;
import com.android.car.ui.sharedlibrarysupport.SharedLibraryFactorySingleton;

import org.junit.Rule;
import org.junit.Test;
import org.junit.runner.RunWith;
import org.junit.runners.Parameterized;

/** A test of {@link com.android.car.ui.core.CarUi#installBaseLayoutAround} */
@RunWith(Parameterized.class)
public class InstallBaseLayoutAroundTest {

    @Parameterized.Parameters
    public static Object[] data() {
        // It's important to do no shared library first, so that the shared library will
        // still be enabled when this test finishes
        return new Object[] { false, true };
    }

    private final boolean mSharedLibEnabled;

    public InstallBaseLayoutAroundTest(boolean sharedLibEnabled) {
        mSharedLibEnabled = sharedLibEnabled;
        SharedLibraryFactorySingleton.setSharedLibEnabled(sharedLibEnabled);
    }

    @Rule
    public final ActivityScenarioRule<TrulyEmptyActivity> mScenarioRule =
            new ActivityScenarioRule<>(TrulyEmptyActivity.class);

    @Test
    public void test_installBaseLayoutAround_createsToolbar() {
        onView(isAssignableFrom(FocusParkingView.class)).check(doesNotExist());

        ToolbarController[] toolbar = new ToolbarController[] { null };
        Insets[] insets = new Insets[] { null };
        mScenarioRule.getScenario().onActivity(activity -> {
            toolbar[0] = CarUi.installBaseLayoutAround(
                    activity.requireViewById(android.R.id.content),
                    i -> insets[0] = i,
                    true);
            if (toolbar[0] != null) {
                toolbar[0].setTitle("Hello, world!");
            }
        });

        assertNotNull(toolbar[0]);
        onView(withText("Hello, world!")).check(matches(isDisplayed()));

        assertNotNull(insets[0]);
        // Technically this doesn't have to be true depending on the OEM's customizations
        assertTrue(insets[0].getTop() > 0);
        onView(isAssignableFrom(FocusParkingView.class)).check(matches(isDisplayed()));
    }

    @Test
    public void test_installBaseLayoutAround_doesntCreateToolbar() {
        onView(isAssignableFrom(FocusParkingView.class)).check(doesNotExist());

        ToolbarController[] toolbar = new ToolbarController[] { null };
        mScenarioRule.getScenario().onActivity(activity ->
                toolbar[0] = CarUi.installBaseLayoutAround(
                        activity.requireViewById(android.R.id.content),
                        i -> {},
                        false));

        assertNull(toolbar[0]);
        onView(isAssignableFrom(FocusParkingView.class)).check(matches(isDisplayed()));
    }

    @Test
    public void test_emptyactivity_doesnthaveinsetsortoolbar() {
        Insets[] insets = new Insets[] { new Insets() };
        mScenarioRule.getScenario().onActivity(activity -> insets[0] = CarUi.getInsets(activity));
        assertNull(insets[0]);

        ToolbarController[] toolbar = new ToolbarController[] { new EmptyToolbarController() };
        mScenarioRule.getScenario().onActivity(activity -> toolbar[0] = CarUi.getToolbar(activity));
        assertNull(toolbar[0]);
    }
}
