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
import static androidx.test.espresso.assertion.ViewAssertions.doesNotExist;
import static androidx.test.espresso.assertion.ViewAssertions.matches;
import static androidx.test.espresso.matcher.ViewMatchers.isAssignableFrom;
import static androidx.test.espresso.matcher.ViewMatchers.isChecked;
import static androidx.test.espresso.matcher.ViewMatchers.isDisplayed;
import static androidx.test.espresso.matcher.ViewMatchers.isRoot;
import static androidx.test.espresso.matcher.ViewMatchers.withContentDescription;
import static androidx.test.espresso.matcher.ViewMatchers.withText;

import static com.android.car.ui.actions.ViewActions.waitForView;
import static com.android.car.ui.matchers.ViewMatchers.doesNotExistOrIsNotDisplayed;

import static com.google.common.truth.Truth.assertThat;

import static org.hamcrest.core.IsNot.not;
import static org.mockito.Mockito.mock;
import static org.mockito.Mockito.times;
import static org.mockito.Mockito.verify;

import android.car.drivingstate.CarUxRestrictions;
import android.content.Context;
import android.view.View;
import android.widget.Switch;

import androidx.annotation.DrawableRes;
import androidx.test.ext.junit.rules.ActivityScenarioRule;
import androidx.test.platform.app.InstrumentationRegistry;

import com.android.car.ui.core.CarUi;
import com.android.car.ui.matchers.ViewMatchers;
import com.android.car.ui.sharedlibrarysupport.SharedLibraryFactorySingleton;
import com.android.car.ui.test.R;

import org.hamcrest.Matcher;
import org.hamcrest.Matchers;
import org.junit.Rule;
import org.junit.Test;
import org.junit.runner.RunWith;
import org.junit.runners.Parameterized;

import java.util.ArrayList;
import java.util.Arrays;
import java.util.Collections;
import java.util.List;
import java.util.function.BiConsumer;
import java.util.function.Consumer;

@SuppressWarnings("AndroidJdkLibsChecker")
@RunWith(Parameterized.class)
public class ToolbarMenuItemsTest {
    @Parameterized.Parameters
    public static Object[] data() {
        // It's important to do no shared library first, so that the shared library will
        // still be enabled when this test finishes
        return new Object[] { false, true };
    }

    private final boolean mSharedLibEnabled;

    public ToolbarMenuItemsTest(boolean sharedLibEnabled) {
        mSharedLibEnabled = sharedLibEnabled;
        SharedLibraryFactorySingleton.setSharedLibEnabled(sharedLibEnabled);
    }

    @Rule
    public final ActivityScenarioRule<ToolbarTestActivity> mScenarioRule =
            new ActivityScenarioRule<>(ToolbarTestActivity.class);

    @Test
    public void menuItems_findMenuItemById_shouldWork() {
        MenuItem[] menuItem = new MenuItem[] { null };
        MenuItem[] foundMenuItem = new MenuItem[] { null };
        runWithActivityAndToolbar((activity, toolbar) -> {
            menuItem[0] = MenuItem.builder(activity)
                    .setTitle("Button!")
                    .setId(5)
                    .build();
            toolbar.setMenuItems(Collections.singletonList(menuItem[0]));
            foundMenuItem[0] = toolbar.requireMenuItemById(5);
        });

        assertThat(foundMenuItem[0]).isSameInstanceAs(menuItem[0]);
    }

    @Test
    public void menuItems_text_shouldShow() {
        MenuItem.OnClickListener callback = mock(MenuItem.OnClickListener.class);
        MenuItem[] menuItem = new MenuItem[] { null };
        runWithActivityAndToolbar((activity, toolbar) -> {
            menuItem[0] = MenuItem.builder(activity)
                    .setTitle("Button!")
                    .setOnClickListener(callback)
                    .build();
            toolbar.setMenuItems(Collections.singletonList(menuItem[0]));
        });

        waitForViewWithText("Button!");

        onView(withText("Button!")).perform(click());

        verify(callback).onClick(menuItem[0]);
    }

    @Test
    public void menuItems_icon_shouldShow() {
        MenuItem.OnClickListener callback = mock(MenuItem.OnClickListener.class);
        MenuItem[] menuItem = new MenuItem[] { null };
        runWithActivityAndToolbar((activity, toolbar) -> {
            menuItem[0] = MenuItem.builder(activity)
                    .setTitle("test_menuitem")
                    .setIcon(R.drawable.ic_add)
                    .setOnClickListener(callback)
                    .build();
            toolbar.setMenuItems(Collections.singletonList(menuItem[0]));
        });

        assertThat(menuItem[0].isVisible()).isTrue();
        assertThat(menuItem[0].isTinted()).isTrue();
        assertThat(menuItem[0].isActivatable()).isFalse();
        assertThat(menuItem[0].isCheckable()).isFalse();
        assertThat(menuItem[0].getUxRestrictions()).isEqualTo(
                CarUxRestrictions.UX_RESTRICTIONS_BASELINE);

        onView(isRoot()).perform(waitForView(withContentDescription("test_menuitem"), 500));
        onView(withText("test_menuitem")).check(doesNotExistOrIsNotDisplayed());
        onView(withContentDescription("test_menuitem")).perform(click());

        verify(callback).onClick(menuItem[0]);
    }

    @Test
    public void menuItems_textAndIcon_shouldShow() {
        MenuItem.OnClickListener callback = mock(MenuItem.OnClickListener.class);
        MenuItem[] menuItem = new MenuItem[] { null };
        runWithActivityAndToolbar((activity, toolbar) -> {
            menuItem[0] = MenuItem.builder(activity)
                    .setTitle("Test!")
                    .setIcon(R.drawable.ic_add)
                    .setShowIconAndTitle(true)
                    .setOnClickListener(callback)
                    .build();
            toolbar.setMenuItems(Collections.singletonList(menuItem[0]));
        });

        onView(isRoot()).perform(waitForView(withText("Test!"), 500));
        onView(withText("Test!")).perform(click());

        verify(callback).onClick(menuItem[0]);

        runWithActivityAndToolbar((activity, toolbar) -> {
            menuItem[0].setTitle(R.string.test_string_test_title);
            menuItem[0].setIcon(R.drawable.ic_launcher);
        });

        onView(withText("Test!")).check(doesNotExistOrIsNotDisplayed());
        onView(withText("Test title!")).check(matches(isDisplayed()));
    }

    @Test
    public void menuItems_removeIcon_shouldShowText() {
        MenuItem[] menuItem = new MenuItem[] { null };
        runWithActivityAndToolbar((activity, toolbar) -> {
            menuItem[0] = MenuItem.builder(activity)
                    .setTitle("Test!")
                    .setIcon(R.drawable.ic_add)
                    .build();
            toolbar.setMenuItems(Collections.singletonList(menuItem[0]));
        });

        onView(isRoot()).perform(waitForView(withContentDescription("Test!"), 500));
        onView(withContentDescription("Test!")).check(matches(isDisplayed()));
        onView(withText("Test!")).check(doesNotExistOrIsNotDisplayed());

        runWithActivityAndToolbar((activity, toolbar) -> menuItem[0].setIcon(0));

        onView(withText("Test!")).check(matches(isDisplayed()));
    }

    @Test
    public void menuItems_switch_shouldShow() {
        MenuItem.OnClickListener callback = mock(MenuItem.OnClickListener.class);
        MenuItem[] menuItem = new MenuItem[] { null };
        runWithActivityAndToolbar((activity, toolbar) -> {
            menuItem[0] = MenuItem.builder(activity)
                    .setCheckable()
                    .setOnClickListener(callback)
                    .build();
            toolbar.setMenuItems(Collections.singletonList(menuItem[0]));
        });

        assertThat(menuItem[0].isCheckable()).isTrue();
        assertThat(menuItem[0].isChecked()).isFalse();

        onView(isRoot()).perform(waitForView(isSwitch(), 500));
        onView(isSwitch()).perform(click());

        verify(callback).onClick(menuItem[0]);
        onView(isSwitch()).check(matches(isChecked()));

        assertThat(menuItem[0].isCheckable()).isTrue();
        assertThat(menuItem[0].isChecked()).isTrue();
    }

    @Test
    public void menuItems_overflowText_shouldWork() {
        MenuItem.OnClickListener callback = mock(MenuItem.OnClickListener.class);
        MenuItem[] menuItem = new MenuItem[] { null };
        runWithActivityAndToolbar((activity, toolbar) -> {
            menuItem[0] = MenuItem.builder(activity)
                    .setTitle("Test title!")
                    .setDisplayBehavior(MenuItem.DisplayBehavior.NEVER)
                    .setOnClickListener(callback)
                    .build();
            toolbar.setMenuItems(Collections.singletonList(menuItem[0]));
        });

        // Open overflow menu, then click on the MenuItem
        onView(isRoot()).perform(waitForView(withContentDescription("Overflow")));
        onView(withContentDescription("Overflow")).perform(click());
        onView(withText("Test title!")).perform(click());
        verify(callback).onClick(menuItem[0]);

        // TODO(b/188925810): this currently isn't supported in the referencedesign shared library.
        if (!mSharedLibEnabled) {
            // Open overflow menu, change MenuItem's title, then click on the MenuItem
            onView(withContentDescription("Overflow")).perform(click());
            runWithToolbar(toolbar -> menuItem[0].setTitle("Test title 2!"));
            onView(withText("Test title 2!")).perform(click());
            verify(callback, times(2)).onClick(menuItem[0]);
        }
    }

    @Test
    public void menuItems_overflowIconAndText_shouldWork() {
        MenuItem.OnClickListener callback = mock(MenuItem.OnClickListener.class);
        MenuItem[] menuItem = new MenuItem[] { null };
        runWithActivityAndToolbar((activity, toolbar) -> {
            menuItem[0] = MenuItem.builder(activity)
                    .setTitle("Test title!")
                    .setIcon(R.drawable.ic_add)
                    .setDisplayBehavior(MenuItem.DisplayBehavior.NEVER)
                    .setOnClickListener(callback)
                    .build();
            toolbar.setMenuItems(Collections.singletonList(menuItem[0]));
        });

        onView(isRoot()).perform(waitForView(withContentDescription("Overflow")));
        onView(withContentDescription("Overflow")).perform(click());

        onView(withText("Test title!")).perform(click());

        verify(callback).onClick(menuItem[0]);
    }

    @Test
    public void menuItems_getMenuItems_returnsSameMenuItems() {
        List<MenuItem> menuItems = new ArrayList<>();
        runWithActivityAndToolbar((activity, toolbar) -> {
            menuItems.add(MenuItem.builder(activity)
                    .setTitle("Menu Item 1")
                    .build());
            menuItems.add(MenuItem.builder(activity)
                    .setTitle("Menu Item 2")
                    .build());
            toolbar.setMenuItems(menuItems);
        });

        onView(isRoot()).perform(waitForView(withText("Menu Item 1")));

        boolean[] equal = new boolean[] { false };
        runWithActivityAndToolbar((activity, toolbar) ->
                equal[0] = menuItems.equals(toolbar.getMenuItems()));

        assertThat(equal[0]).isTrue();
    }

    @Test
    public void menuItems_null_shouldRemoveExistingMenuItems() {
        runWithActivityAndToolbar((activity, toolbar) ->
                toolbar.setMenuItems(Arrays.asList(
                        MenuItem.builder(activity)
                                .setTitle("Button!")
                                .build(),
                        MenuItem.builder(activity)
                                .setTitle("Button2!")
                                .build()
                )));
        waitForViewWithText("Button!");
        waitForViewWithText("Button2!");

        onView(withText("Button!")).check(matches(isDisplayed()));
        onView(withText("Button2!")).check(matches(isDisplayed()));

        runWithToolbar((toolbar) -> toolbar.setMenuItems(null));

        onView(withText("Button!")).check(doesNotExist());
        onView(withText("Button2!")).check(doesNotExist());
    }

    @Test
    public void menuItems_setVisibility_shouldHide() {
        MenuItem[] menuItem = new MenuItem[] { null };
        runWithActivityAndToolbar((activity, toolbar) -> {
            menuItem[0] = MenuItem.builder(activity)
                    .setTitle("Button!")
                    .build();

            toolbar.setMenuItems(Collections.singletonList(menuItem[0]));
        });

        waitForViewWithText("Button!");
        onView(withText("Button!")).check(matches(isDisplayed()));

        runWithToolbar((toolbar) -> menuItem[0].setVisible(false));

        onView(withText("Button!")).check(matches(not(isDisplayed())));
    }

    @Test
    public void menuItems_setEnabled_shouldDisable() {
        MenuItem.OnClickListener callback = mock(MenuItem.OnClickListener.class);
        MenuItem[] menuItem = new MenuItem[] { null };
        runWithActivityAndToolbar((activity, toolbar) -> {
            menuItem[0] = MenuItem.builder(activity)
                    .setTitle("Test!")
                    .setOnClickListener(callback)
                    .build();
            toolbar.setMenuItems(Collections.singletonList(menuItem[0]));
        });

        onView(isRoot()).perform(waitForView(withText("Test!"), 500));
        onView(withText("Test!")).perform(click());

        verify(callback, times(1)).onClick(menuItem[0]);

        runWithActivityAndToolbar((activity, toolbar) -> menuItem[0].setEnabled(false));

        onView(withText("Test!")).perform(click());

        verify(callback, times(1)).onClick(menuItem[0]);
    }

    @Test
    public void menuItems_activatable_test() {
        MenuItem.OnClickListener callback = mock(MenuItem.OnClickListener.class);
        MenuItem[] menuItem = new MenuItem[] { null };
        runWithActivityAndToolbar((activity, toolbar) -> {
            menuItem[0] = MenuItem.builder(activity)
                    .setTitle("Test!")
                    .setIcon(R.drawable.ic_add)
                    .setActivatable()
                    .setOnClickListener(callback)
                    .build();
            toolbar.setMenuItems(Collections.singletonList(menuItem[0]));
        });

        onView(isRoot()).perform(waitForView(withContentDescription("Test!"), 500));

        assertThat(menuItem[0].isActivated()).isFalse();

        runWithActivityAndToolbar((activity, toolbar) -> menuItem[0].setActivated(true));

        assertThat(menuItem[0].isActivated()).isTrue();
    }

    @Test
    public void menuItems_changeOnClickListener_shouldCallNewListener() {
        MenuItem.OnClickListener callback1 = mock(MenuItem.OnClickListener.class);
        MenuItem.OnClickListener callback2 = mock(MenuItem.OnClickListener.class);
        MenuItem[] menuItem = new MenuItem[] { null };
        runWithActivityAndToolbar((activity, toolbar) -> {
            menuItem[0] = MenuItem.builder(activity)
                    .setTitle("Button!")
                    .setOnClickListener(callback1)
                    .build();
            toolbar.setMenuItems(Collections.singletonList(menuItem[0]));
        });

        waitForViewWithText("Button!");
        onView(withText("Button!")).perform(click());
        verify(callback1, times(1)).onClick(menuItem[0]);
        verify(callback2, times(0)).onClick(menuItem[0]);

        runWithActivityAndToolbar((activity, toolbar) -> menuItem[0].setOnClickListener(callback2));

        onView(withText("Button!")).perform(click());
        verify(callback1, times(1)).onClick(menuItem[0]);
        verify(callback2, times(1)).onClick(menuItem[0]);
    }


    @Test
    public void menuItems_searchScreen_shouldHideMenuItems() {
        runWithActivityAndToolbar((activity, toolbar) -> {
            toolbar.setMenuItems(Arrays.asList(
                    MenuItem.builder(activity)
                            .setToSearch()
                            .build(),
                    MenuItem.builder(activity)
                            .setTitle("Button!")
                            .build()));
            toolbar.setShowMenuItemsWhileSearching(true);
            toolbar.setState(Toolbar.State.SEARCH);
        });

        waitForViewWithText("Button!");

        // Even if not hiding MenuItems while searching, the search MenuItem should still be hidden
        onView(withText("Button!")).check(matches(isDisplayed()));
        onView(withContentDescription(R.string.car_ui_toolbar_menu_item_search_title))
                .check(doesNotExistOrIsNotDisplayed());

        runWithToolbar((toolbar) -> toolbar.setShowMenuItemsWhileSearching(false));

        // All menuitems should be hidden if we're hiding menuitems while searching
        onView(withText("Button!")).check(doesNotExistOrIsNotDisplayed());
        onView(withContentDescription(R.string.car_ui_toolbar_menu_item_search_title))
                .check(doesNotExistOrIsNotDisplayed());
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

    private void waitForViewWithText(String text) {
        onView(isRoot()).perform(waitForView(withText(text), 500));
    }

    private Matcher<View> withDrawable(@DrawableRes int drawable) {
        Context context = InstrumentationRegistry.getInstrumentation().getContext();
        return ViewMatchers.withDrawable(context, com.android.car.ui.test.R.drawable.ic_launcher);
    }

    private Matcher<View> isSwitch() {
        return Matchers.allOf(isDisplayed(), isAssignableFrom(Switch.class));
    }
}
