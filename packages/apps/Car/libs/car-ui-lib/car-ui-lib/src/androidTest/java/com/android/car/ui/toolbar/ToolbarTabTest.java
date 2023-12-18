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

import static com.android.car.ui.matchers.ViewMatchers.doesNotExistOrIsNotDisplayed;

import static com.google.common.truth.Truth.assertThat;

import static org.mockito.Mockito.mock;
import static org.mockito.Mockito.times;
import static org.mockito.Mockito.verify;

import android.content.Context;

import androidx.core.content.ContextCompat;
import androidx.test.ext.junit.rules.ActivityScenarioRule;
import androidx.test.platform.app.InstrumentationRegistry;

import com.android.car.ui.core.CarUi;
import com.android.car.ui.sharedlibrarysupport.SharedLibraryFactorySingleton;
import com.android.car.ui.test.R;

import org.junit.Rule;
import org.junit.Test;
import org.junit.runner.RunWith;
import org.junit.runners.Parameterized;

import java.util.Arrays;
import java.util.List;
import java.util.function.Consumer;

@SuppressWarnings("AndroidJdkLibsChecker")
@RunWith(Parameterized.class)
public class ToolbarTabTest {

    @Parameterized.Parameters
    public static Object[] data() {
        // It's important to do no shared library first, so that the shared library will
        // still be enabled when this test finishes
        return new Object[] { false, true };
    }

    public ToolbarTabTest(boolean sharedLibEnabled) {
        SharedLibraryFactorySingleton.setSharedLibEnabled(sharedLibEnabled);
    }

    @Rule
    public final ActivityScenarioRule<ToolbarTestActivity> mScenarioRule =
            new ActivityScenarioRule<>(ToolbarTestActivity.class);

    @Test
    public void test_basicTab_settersAndGetters() {
        Context context = InstrumentationRegistry.getInstrumentation().getContext();
        Consumer<Tab> selectedListener = tab -> {};
        Tab tab = Tab.builder()
                .setText("Foo")
                .setIcon(ContextCompat.getDrawable(context, R.drawable.ic_add))
                .setSelectedListener(selectedListener)
                .build();
        assertThat(tab.getText()).isEqualTo("Foo");
        assertThat(tab.getIcon()).isNotNull();
        assertThat(tab.getSelectedListener()).isSameInstanceAs(selectedListener);
    }

    @Test
    @SuppressWarnings("unchecked")
    public void test_setTabs_areShown() {
        Consumer<Tab> tabSelectedListener = mock(Consumer.class);
        List<Tab> tabs = Arrays.asList(
                Tab.builder()
                        .setText("Tab 1")
                        .setSelectedListener(tabSelectedListener)
                        .build(),
                Tab.builder()
                        .setText("Tab 2")
                        .setSelectedListener(tabSelectedListener)
                        .build());
        runWithToolbar(toolbar -> toolbar.setTabs(tabs));

        onView(withText("Tab 1")).perform(click());
        onView(withText("Tab 1")).perform(click());
        onView(withText("Tab 2")).perform(click());
        onView(withText("Tab 1")).perform(click());
        onView(withText("Tab 2")).perform(click());
        onView(withText("Tab 1")).perform(click());
        onView(withText("Tab 1")).perform(click());

        // Tab listeners are not recalled when the tab is already selected
        verify(tabSelectedListener, times(2)).accept(tabs.get(0));
        verify(tabSelectedListener, times(2)).accept(tabs.get(1));
    }

    @Test
    @SuppressWarnings("unchecked")
    public void test_selectTab_works() {
        Consumer<Tab> tabSelectedListener = mock(Consumer.class);
        List<Tab> tabs = Arrays.asList(
                Tab.builder()
                        .setText("Tab 1")
                        .setSelectedListener(tabSelectedListener)
                        .build(),
                Tab.builder()
                        .setText("Tab 2")
                        .setSelectedListener(tabSelectedListener)
                        .build());
        runWithToolbar(toolbar -> {
            toolbar.setTabs(tabs);
            toolbar.selectTab(1);
        });

        verify(tabSelectedListener, times(1)).accept(tabs.get(1));
    }

    @Test
    public void test_clearAllTabs_works() {
        runWithToolbar(toolbar -> {
            toolbar.setTabs(Arrays.asList(
                    Tab.builder()
                            .setText("Tab 1")
                            .build(),
                    Tab.builder()
                            .setText("Tab 2")
                            .build()));
            toolbar.clearAllTabs();
        });

        onView(withText("Tab 1")).check(doesNotExistOrIsNotDisplayed());
        onView(withText("Tab 2")).check(doesNotExistOrIsNotDisplayed());
    }

    private void runWithToolbar(Consumer<ToolbarController> toRun) {
        mScenarioRule.getScenario().onActivity(activity -> {
            ToolbarController toolbar = CarUi.requireToolbar(activity);
            toRun.accept(toolbar);
        });
    }
}
