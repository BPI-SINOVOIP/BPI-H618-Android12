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
import static androidx.test.espresso.action.ViewActions.click;
import static androidx.test.espresso.matcher.ViewMatchers.withText;

import static com.google.common.truth.Truth.assertThat;

import static org.mockito.Mockito.mock;
import static org.mockito.Mockito.times;
import static org.mockito.Mockito.verify;

import androidx.test.ext.junit.rules.ActivityScenarioRule;

import com.android.car.ui.core.CarUi;
import com.android.car.ui.sharedlibrarysupport.SharedLibraryFactorySingleton;

import org.junit.Rule;
import org.junit.Test;
import org.junit.runner.RunWith;
import org.junit.runners.Parameterized;

import java.util.Arrays;
import java.util.List;
import java.util.function.Consumer;

@SuppressWarnings("AndroidJdkLibsChecker")
@RunWith(Parameterized.class)
public class ToolbarTabDeprecatedTest {

    @Parameterized.Parameters
    public static Object[] data() {
        // It's important to do no shared library first, so that the shared library will
        // still be enabled when this test finishes
        return new Object[] { false, true };
    }

    public ToolbarTabDeprecatedTest(boolean sharedLibEnabled) {
        SharedLibraryFactorySingleton.setSharedLibEnabled(sharedLibEnabled);
    }

    @Rule
    public final ActivityScenarioRule<ToolbarTestActivity> mScenarioRule =
            new ActivityScenarioRule<>(ToolbarTestActivity.class);

    @Test
    public void test_addTab_works() {
        Toolbar.OnTabSelectedListener tabSelectedListener =
                mock(Toolbar.OnTabSelectedListener.class);

        List<TabLayout.Tab> tabs = Arrays.asList(
                new TabLayout.Tab(null, "Tab 1"),
                new TabLayout.Tab(null, "Tab 2"));
        runWithToolbar(toolbar -> {
            toolbar.addTab(tabs.get(0));
            toolbar.addTab(tabs.get(1));
            toolbar.registerOnTabSelectedListener(tabSelectedListener);
        });

        onView(withText("Tab 1")).perform(click());
        onView(withText("Tab 1")).perform(click());
        onView(withText("Tab 2")).perform(click());
        onView(withText("Tab 1")).perform(click());
        onView(withText("Tab 2")).perform(click());
        onView(withText("Tab 1")).perform(click());
        onView(withText("Tab 1")).perform(click());

        // Tab listeners are not recalled when the tab is already selected
        verify(tabSelectedListener, times(2)).onTabSelected(tabs.get(0));
        verify(tabSelectedListener, times(2)).onTabSelected(tabs.get(1));
    }

    @Test
    public void test_getTabCountAndClearTabs_works() {
        int[] tabCount = new int[] { -1 };

        runWithToolbar(toolbar -> tabCount[0] = toolbar.getTabCount());
        assertThat(tabCount[0]).isEqualTo(0);

        runWithToolbar(toolbar -> {
            toolbar.addTab(new TabLayout.Tab(null, "Tab 1"));
            toolbar.addTab(new TabLayout.Tab(null, "Tab 2"));
            tabCount[0] = toolbar.getTabCount();
        });
        assertThat(tabCount[0]).isEqualTo(2);

        runWithToolbar(toolbar -> {
            toolbar.clearAllTabs();
            tabCount[0] = toolbar.getTabCount();
        });
        assertThat(tabCount[0]).isEqualTo(0);
    }

    private void runWithToolbar(Consumer<ToolbarController> toRun) {
        mScenarioRule.getScenario().onActivity(activity -> {
            ToolbarController toolbar = CarUi.requireToolbar(activity);
            toRun.accept(toolbar);
        });
    }
}
