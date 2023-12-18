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
import static androidx.test.espresso.action.ViewActions.typeText;
import static androidx.test.espresso.assertion.ViewAssertions.matches;
import static androidx.test.espresso.matcher.ViewMatchers.isAssignableFrom;
import static androidx.test.espresso.matcher.ViewMatchers.isDisplayed;
import static androidx.test.espresso.matcher.ViewMatchers.withHint;
import static androidx.test.espresso.matcher.ViewMatchers.withText;

import static com.android.car.ui.matchers.ViewMatchers.doesNotExistOrIsNotDisplayed;
import static com.android.car.ui.matchers.ViewMatchers.withDrawable;

import static org.mockito.Mockito.mock;
import static org.mockito.Mockito.verify;

import android.content.Context;
import android.widget.EditText;

import androidx.test.ext.junit.rules.ActivityScenarioRule;
import androidx.test.platform.app.InstrumentationRegistry;

import com.android.car.ui.core.CarUi;
import com.android.car.ui.sharedlibrarysupport.SharedLibraryFactorySingleton;
import com.android.car.ui.test.R;

import org.junit.Rule;
import org.junit.Test;
import org.junit.runner.RunWith;
import org.junit.runners.Parameterized;

import java.util.function.Consumer;

/** Unit test for the search functionality in {@link ToolbarController}. */
@SuppressWarnings("AndroidJdkLibsChecker")
@RunWith(Parameterized.class)
public class ToolbarSearchTest {
    @Parameterized.Parameters
    public static Object[][] data() {
        // It's important to do no shared library first, so that the shared library will
        // still be enabled when this test finishes
        return new Object[][] {
            new Object[] {false, SearchMode.SEARCH},
            new Object[] {false, SearchMode.EDIT},
            new Object[] {true, SearchMode.SEARCH},
            new Object[] {true, SearchMode.EDIT},
        };
    }

    private final SearchMode mSearchMode;

    public ToolbarSearchTest(boolean sharedLibEnabled, SearchMode searchMode) {
        SharedLibraryFactorySingleton.setSharedLibEnabled(sharedLibEnabled);
        mSearchMode = searchMode;
    }

    @Rule
    public final ActivityScenarioRule<ToolbarTestActivity> mScenarioRule =
            new ActivityScenarioRule<>(ToolbarTestActivity.class);

    @Test
    public void test_setSearchQueryBeforeSearchMode_doesNothing() {
        runWithToolbar(toolbar -> toolbar.setSearchQuery("Hello, world!"));
        onView(withText("Hello, world!")).check(doesNotExistOrIsNotDisplayed());
    }

    @Test
    public void test_setSearchQueryAfterSearchMode_showsQuery() {
        runWithToolbar(toolbar -> {
            toolbar.setSearchMode(mSearchMode);
            toolbar.setSearchQuery("Hello, world!");
        });
        onView(withText("Hello, world!")).check(matches(isDisplayed()));
    }

    @Test
    @SuppressWarnings("unchecked")
    public void test_registerSearchListeners_callsListeners() {
        Consumer<String> searchListener = mock(Consumer.class);
        Runnable searchCompletedListener = mock(Runnable.class);
        runWithToolbar(toolbar -> {
            toolbar.setSearchMode(mSearchMode);
            toolbar.registerSearchListener(searchListener);
            toolbar.registerSearchCompletedListener(searchCompletedListener);
        });
        onView(isAssignableFrom(EditText.class)).perform(typeText("hello\n"));

        verify(searchListener).accept("h");
        verify(searchListener).accept("he");
        verify(searchListener).accept("hel");
        verify(searchListener).accept("hell");
        verify(searchListener).accept("hello");
        verify(searchCompletedListener).run();
    }

    @Test
    public void test_setSearchIcon_showsIcon() {
        runWithToolbar(toolbar -> {
            toolbar.setSearchMode(mSearchMode);
            toolbar.setLogo(R.drawable.ic_launcher);
            toolbar.setSearchIcon(R.drawable.ic_settings_gear);
        });

        Context context = InstrumentationRegistry.getInstrumentation().getContext();
        onView(withDrawable(context, R.drawable.ic_launcher)).check(matches(isDisplayed()));
        if (mSearchMode == SearchMode.SEARCH) {
            onView(withDrawable(context, R.drawable.ic_settings_gear))
                .check(matches(isDisplayed()));
        } else {
            onView(withDrawable(context, R.drawable.ic_settings_gear))
                .check(doesNotExistOrIsNotDisplayed());
        }
    }

    @Test
    public void test_setSearchIconTo0_removesIcon() {
        runWithToolbar(toolbar -> {
            toolbar.setSearchMode(mSearchMode);
            toolbar.setSearchIcon(R.drawable.ic_settings_gear);
            toolbar.setSearchIcon(0);
        });

        Context context = InstrumentationRegistry.getInstrumentation().getContext();
        onView(withDrawable(context, R.drawable.ic_settings_gear))
            .check(doesNotExistOrIsNotDisplayed());
    }

    @Test
    public void test_setSearchModeDisabled_hidesSearchBox() {
        runWithToolbar(toolbar -> toolbar.setSearchMode(mSearchMode));
        onView(isAssignableFrom(EditText.class)).check(matches(isDisplayed()));

        runWithToolbar(toolbar -> toolbar.setSearchMode(SearchMode.DISABLED));
        onView(isAssignableFrom(EditText.class)).check(doesNotExistOrIsNotDisplayed());
    }

    @Test
    public void test_setSearchHint_isDisplayed() {
        runWithToolbar((toolbar) -> {
            toolbar.setSearchHint("Test search hint");
            toolbar.setSearchMode(mSearchMode);
        });

        onView(withHint("Test search hint")).check(matches(isDisplayed()));

        runWithToolbar((toolbar) -> {
            toolbar.setSearchHint(R.string.test_string_test_title);
            toolbar.setSearchMode(mSearchMode);
        });

        onView(withHint("Test title!")).check(matches(isDisplayed()));
    }

    private void runWithToolbar(Consumer<ToolbarController> toRun) {
        mScenarioRule.getScenario().onActivity(activity -> {
            ToolbarController toolbar = CarUi.requireToolbar(activity);
            toRun.accept(toolbar);
        });
    }
}
