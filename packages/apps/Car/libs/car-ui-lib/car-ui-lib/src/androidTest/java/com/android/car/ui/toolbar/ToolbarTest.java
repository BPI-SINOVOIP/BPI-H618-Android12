/*
 * Copyright (C) 2020 The Android Open Source Project
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
import static androidx.test.espresso.assertion.ViewAssertions.matches;
import static androidx.test.espresso.matcher.ViewMatchers.isDisplayed;
import static androidx.test.espresso.matcher.ViewMatchers.withContentDescription;
import static androidx.test.espresso.matcher.ViewMatchers.withText;

import static com.android.car.ui.matchers.ViewMatchers.doesNotExistOrIsNotDisplayed;
import static com.android.car.ui.matchers.ViewMatchers.withDrawable;

import static com.google.common.truth.Truth.assertThat;

import static junit.framework.Assert.assertNotNull;
import static junit.framework.TestCase.assertEquals;

import android.content.Context;

import androidx.core.content.ContextCompat;
import androidx.test.ext.junit.rules.ActivityScenarioRule;
import androidx.test.platform.app.InstrumentationRegistry;

import com.android.car.ui.baselayout.Insets;
import com.android.car.ui.core.CarUi;
import com.android.car.ui.sharedlibrarysupport.SharedLibraryFactorySingleton;
import com.android.car.ui.test.R;

import org.junit.Rule;
import org.junit.Test;
import org.junit.runner.RunWith;
import org.junit.runners.Parameterized;

import java.util.function.BiConsumer;
import java.util.function.Consumer;

/** Unit test for {@link ToolbarController}. */
@SuppressWarnings("AndroidJdkLibsChecker")
@RunWith(Parameterized.class)
public class ToolbarTest {

    @Parameterized.Parameters
    public static Object[] data() {
        // It's important to do no shared library first, so that the shared library will
        // still be enabled when this test finishes
        return new Object[] { false, true };
    }

    private final boolean mSharedLibEnabled;

    public ToolbarTest(boolean sharedLibEnabled) {
        mSharedLibEnabled = sharedLibEnabled;
        SharedLibraryFactorySingleton.setSharedLibEnabled(sharedLibEnabled);
    }

    @Rule
    public final ActivityScenarioRule<ToolbarTestActivity> mScenarioRule =
            new ActivityScenarioRule<>(ToolbarTestActivity.class);

    @Test
    public void test_setTitle_displaysTitle() {
        runWithToolbar((toolbar) -> toolbar.setTitle("Test title"));
        onView(withText("Test title")).check(matches(isDisplayed()));

        // withText() uses the target context, not the instrumentation test's context.
        String toolbarTitle = InstrumentationRegistry.getInstrumentation().getContext()
                .getString(R.string.test_string_test_title);
        runWithToolbar(toolbar -> toolbar.setTitle(R.string.test_string_test_title));
        onView(withText(toolbarTitle)).check(matches(isDisplayed()));

        runWithToolbar(toolbar -> toolbar.setTitle(0));
        onView(withText(toolbarTitle)).check(doesNotExistOrIsNotDisplayed());
    }

    /**
     * This is somewhat of a bug, but various tests in other apps rely on this functionality.
     */
    @Test
    public void test_setTitle_null_returns_nonNull() {
        CharSequence[] getTitleResult = new CharSequence[] {"Something obviously incorrect"};
        runWithToolbar((toolbar) -> {
            toolbar.setTitle(null);
            getTitleResult[0] = toolbar.getTitle();
        });

        assertEquals("", getTitleResult[0]);
    }

    /**
     * This is somewhat of a bug, but various tests in other apps rely on this functionality.
     */
    @Test
    public void test_setSubtitle_null_returns_nonNull() {
        CharSequence[] getTitleResult = new CharSequence[] {"Something obviously incorrect"};
        runWithToolbar((toolbar) -> {
            toolbar.setSubtitle(null);
            getTitleResult[0] = toolbar.getSubtitle();
        });

        assertEquals("", getTitleResult[0]);
    }

    @Test
    public void test_setSubtitle_displaysSubtitle() {
        runWithToolbar((toolbar) -> toolbar.setSubtitle("Test subtitle"));
        onView(withText("Test subtitle")).check(matches(isDisplayed()));

        runWithToolbar((toolbar) -> toolbar.setSubtitle(R.string.test_string_test_title));
        onView(withText("Test title!")).check(matches(isDisplayed()));

        runWithToolbar((toolbar) -> toolbar.setSubtitle(0));
        onView(withText("Test title!")).check(doesNotExistOrIsNotDisplayed());
    }

    @Test
    public void setters_and_getters_test() {
        runWithToolbar((toolbar) -> {
            toolbar.setTitle("Foo");
            toolbar.setSearchHint("Foo2");
            toolbar.setShowMenuItemsWhileSearching(true);
            toolbar.setState(Toolbar.State.SUBPAGE);
            toolbar.setNavButtonMode(Toolbar.NavButtonMode.CLOSE);

            assertThat(toolbar.getTitle().toString()).isEqualTo("Foo");
            assertThat(toolbar.getSearchHint().toString()).isEqualTo("Foo2");
            assertThat(toolbar.getShowMenuItemsWhileSearching()).isEqualTo(true);
            assertThat(toolbar.getState()).isEquivalentAccordingToCompareTo(Toolbar.State.SUBPAGE);
            assertThat(toolbar.getNavButtonMode()).isEquivalentAccordingToCompareTo(
                    Toolbar.NavButtonMode.CLOSE);
        });
    }

    @Test
    public void test_setLogo_displaysLogo() {
        runWithToolbar((toolbar) -> toolbar.setLogo(R.drawable.ic_launcher));

        Context context = InstrumentationRegistry.getInstrumentation().getContext();
        onView(withDrawable(context, R.drawable.ic_launcher)).check(matches(isDisplayed()));
    }

    @Test
    public void test_setLogoDrawable_displaysLogo() {
        Context context = InstrumentationRegistry.getInstrumentation().getContext();
        runWithToolbar((toolbar) ->
                toolbar.setLogo(ContextCompat.getDrawable(context, R.drawable.ic_launcher)));

        onView(withDrawable(context, R.drawable.ic_launcher)).check(matches(isDisplayed()));

        runWithToolbar((toolbar) -> toolbar.setLogo(null));

        onView(withDrawable(context, R.drawable.ic_launcher)).check(doesNotExistOrIsNotDisplayed());
    }

    @Test
    public void pressBack_withoutListener_callsActivityOnBack() {
        ToolbarTestActivity[] savedActivity = new ToolbarTestActivity[] { null };
        runWithActivityAndToolbar((activity, toolbar) -> {
            toolbar.setState(Toolbar.State.SUBPAGE);
            savedActivity[0] = activity;
        });

        onView(withContentDescription("Back")).perform(click());

        assertEquals(1, savedActivity[0].getTimesOnBackPressed());
    }

    @Test
    public void pressBack_withListenerThatReturnsFalse_callsActivityOnBack() {
        ToolbarTestActivity[] savedActivity = new ToolbarTestActivity[] { null };
        runWithActivityAndToolbar((activity, toolbar) -> {
            toolbar.setState(Toolbar.State.SUBPAGE);
            toolbar.registerOnBackListener(() -> false);
            savedActivity[0] = activity;
        });

        onView(withContentDescription("Back")).perform(click());

        assertEquals(1, savedActivity[0].getTimesOnBackPressed());
    }

    @Test
    public void pressBack_withListenerThatReturnsTrue_doesntCallActivityOnBack() {
        ToolbarTestActivity[] savedActivity = new ToolbarTestActivity[] { null };
        runWithActivityAndToolbar((activity, toolbar) -> {
            toolbar.setState(Toolbar.State.SUBPAGE);
            toolbar.registerOnBackListener(() -> true);
            savedActivity[0] = activity;
        });

        onView(withContentDescription("Back")).perform(click());

        assertEquals(0, savedActivity[0].getTimesOnBackPressed());
    }

    @Test
    public void pressBack_withUnregisteredListener_doesntCallActivityOnBack() {
        ToolbarTestActivity[] savedActivity = new ToolbarTestActivity[] { null };
        runWithActivityAndToolbar((activity, toolbar) -> {
            toolbar.setState(Toolbar.State.SUBPAGE);
            Toolbar.OnBackListener listener = () -> true;
            toolbar.registerOnBackListener(listener);
            toolbar.registerOnBackListener(listener);
            toolbar.unregisterOnBackListener(listener);
            savedActivity[0] = activity;
        });

        onView(withContentDescription("Back")).perform(click());

        assertEquals(1, savedActivity[0].getTimesOnBackPressed());
    }

    @Test
    public void test_backgroundShown_worksAsExpected() {
        boolean[] backgroundShown = new boolean[] { false };
        runWithToolbar((toolbar) -> backgroundShown[0] = toolbar.getBackgroundShown());

        assertThat(backgroundShown[0]).isTrue();

        runWithToolbar((toolbar) -> {
            toolbar.setBackgroundShown(false);
            backgroundShown[0] = toolbar.getBackgroundShown();
        });

        if (mSharedLibEnabled) {
            // Shared lib doesn't support hiding the background
            // Temporarily disabled while we're not using the shared lib
            // assertThat(backgroundShown[0]).isTrue();
        } else {
            assertThat(backgroundShown[0]).isFalse();
        }
    }

    @Test
    public void test_requireInsets_returnsInsets() {
        Insets[] insets = new Insets[] { null };
        mScenarioRule.getScenario().onActivity(activity ->
                insets[0] = CarUi.requireInsets(activity));
        assertNotNull(insets[0]);
    }

    private void runWithToolbar(Consumer<ToolbarController> toRun) {
        mScenarioRule.getScenario().onActivity(activity -> {
            ToolbarController toolbar = CarUi.requireToolbar(activity);
            toRun.accept(toolbar);
        });
    }

    private void runWithActivityAndToolbar(
            BiConsumer<ToolbarTestActivity, ToolbarController> toRun) {
        mScenarioRule.getScenario().onActivity(activity -> {
            ToolbarController toolbar = CarUi.requireToolbar(activity);
            toRun.accept(activity, toolbar);
        });
    }
}
