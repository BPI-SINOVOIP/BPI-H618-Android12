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

package com.android.car.ui.preference;

import static androidx.test.espresso.Espresso.onView;
import static androidx.test.espresso.action.ViewActions.click;
import static androidx.test.espresso.action.ViewActions.typeText;
import static androidx.test.espresso.assertion.ViewAssertions.doesNotExist;
import static androidx.test.espresso.assertion.ViewAssertions.matches;
import static androidx.test.espresso.matcher.ViewMatchers.isChecked;
import static androidx.test.espresso.matcher.ViewMatchers.isDisplayed;
import static androidx.test.espresso.matcher.ViewMatchers.isNotChecked;
import static androidx.test.espresso.matcher.ViewMatchers.withContentDescription;
import static androidx.test.espresso.matcher.ViewMatchers.withId;
import static androidx.test.espresso.matcher.ViewMatchers.withText;

import static com.android.car.ui.actions.ViewActions.setProgress;
import static com.android.car.ui.matchers.ViewMatchers.withIndex;

import static junit.framework.Assert.assertFalse;
import static junit.framework.TestCase.assertEquals;
import static junit.framework.TestCase.assertTrue;

import static org.hamcrest.Matchers.not;
import static org.mockito.ArgumentMatchers.any;
import static org.mockito.ArgumentMatchers.anyBoolean;
import static org.mockito.ArgumentMatchers.eq;
import static org.mockito.Mockito.mock;
import static org.mockito.Mockito.times;
import static org.mockito.Mockito.verify;
import static org.mockito.Mockito.when;

import android.app.AlertDialog;
import android.content.DialogInterface;
import android.content.res.Resources;
import android.view.View;

import androidx.preference.CheckBoxPreference;
import androidx.preference.DropDownPreference;
import androidx.preference.EditTextPreference;
import androidx.preference.Preference;
import androidx.test.rule.ActivityTestRule;

import com.android.car.ui.test.R;

import org.junit.Before;
import org.junit.Rule;
import org.junit.Test;

import java.util.HashSet;
import java.util.Set;
import java.util.function.Consumer;

/**
 * Unit tests for {@link CarUiPreference}.
 */
public class PreferenceTest {

    private PreferenceTestActivity mActivity;
    private String[] mEntries;
    private String[] mEntriesValues;

    @Rule
    public ActivityTestRule<PreferenceTestActivity> mActivityRule =
            new ActivityTestRule<>(PreferenceTestActivity.class);

    @Before
    public void setUp() {
        mActivity = mActivityRule.getActivity();
        Resources resources = mActivity.getResources();
        mEntries = resources.getStringArray(R.array.entries);
        mEntriesValues = resources.getStringArray(R.array.entry_values);
    }

    @Test
    public void testListPreference() {
        // Scroll until list preference is visible
        mActivity.runOnUiThread(() -> mActivity.scrollToPreference("list"));

        // Display full screen list preference.
        onView(withText(R.string.title_list_preference)).perform(click());

        Preference.OnPreferenceChangeListener mockListener = mock(
                Preference.OnPreferenceChangeListener.class);
        when(mockListener.onPreferenceChange(any(), any())).thenReturn(true);
        mActivity.setOnPreferenceChangeListener("list", mockListener);

        // Check that no option is initially selected.
        onView(withIndex(withId(R.id.car_ui_list_item_radio_button_widget), 1))
                .check(matches(isNotChecked()));
        onView(withIndex(withId(R.id.car_ui_list_item_radio_button_widget), 2))
                .check(matches(isNotChecked()));

        // Select first option.
        onView(withText(mEntries[0])).perform(click());
        // Check that first option is selected.
        onView(withIndex(withId(R.id.car_ui_list_item_radio_button_widget), 0))
                .check(matches(isChecked()));
        onView(withIndex(withId(R.id.car_ui_list_item_radio_button_widget), 1))
                .check(matches(isNotChecked()));
        onView(withIndex(withId(R.id.car_ui_list_item_radio_button_widget), 2))
                .check(matches(isNotChecked()));

        // Press back to save selection.
        onView(withContentDescription("Back")).perform(click());
        // Verify preference value was updated.
        verify(mockListener, times(1)).onPreferenceChange(any(), eq(mEntriesValues[0]));

        onView(withText(R.string.title_list_preference)).perform(click());

        // Check that first option is selected.
        onView(withIndex(withId(R.id.car_ui_list_item_radio_button_widget), 0))
                .check(matches(isChecked()));
        onView(withIndex(withId(R.id.car_ui_list_item_radio_button_widget), 1))
                .check(matches(isNotChecked()));
        onView(withIndex(withId(R.id.car_ui_list_item_radio_button_widget), 2))
                .check(matches(isNotChecked()));

        // Select second option.
        onView(withText(mEntries[1])).perform(click());
        // Check that second option is selected.
        onView(withIndex(withId(R.id.car_ui_list_item_radio_button_widget), 0))
                .check(matches(isNotChecked()));
        onView(withIndex(withId(R.id.car_ui_list_item_radio_button_widget), 1))
                .check(matches(isChecked()));
        onView(withIndex(withId(R.id.car_ui_list_item_radio_button_widget), 2))
                .check(matches(isNotChecked()));

        // Press back to save selection.
        onView(withContentDescription("Back")).perform(click());
        // Verify preference value was updated.
        verify(mockListener, times(1)).onPreferenceChange(any(), eq(mEntriesValues[1]));
        // Return to list preference screen.
        onView(withText(R.string.title_list_preference)).perform(click());

        // Check that second option is selected.
        onView(withIndex(withId(R.id.car_ui_list_item_radio_button_widget), 0))
                .check(matches(isNotChecked()));
        onView(withIndex(withId(R.id.car_ui_list_item_radio_button_widget), 1))
                .check(matches(isChecked()));
        onView(withIndex(withId(R.id.car_ui_list_item_radio_button_widget), 2))
                .check(matches(isNotChecked()));
    }

    @Test
    public void testListPreference_uxRestricted() {
        // Scroll until list preference is visible
        mActivity.runOnUiThread(() -> mActivity.scrollToPreference("list_ux_restricted"));

        // Check that UX restriction is enabled
        CarUiListPreference preference = (CarUiListPreference) mActivity.findPreference(
                "list_ux_restricted");
        assertTrue(preference.isUxRestricted());

        // Set listener
        Consumer<Preference> clickListener = mock(Consumer.class);
        preference.setOnClickWhileRestrictedListener(clickListener);
        assertEquals(clickListener, preference.getOnClickWhileRestrictedListener());

        // Click on ux restricted preference
        onView(withText(R.string.title_list_preference_ux_restricted)).perform(click());
        verify(clickListener, times(1)).accept(preference);
    }

    @Test
    public void testMultiSelectListPreference() {
        // Scroll until multi-select preference is visible
        mActivity.runOnUiThread(() -> mActivity.scrollToPreference("multi_select_list"));

        // Display full screen list preference.
        onView(withText(R.string.title_multi_list_preference)).perform(click());

        Preference.OnPreferenceChangeListener mockListener = mock(
                Preference.OnPreferenceChangeListener.class);
        when(mockListener.onPreferenceChange(any(), any())).thenReturn(true);
        mActivity.setOnPreferenceChangeListener("multi_select_list", mockListener);

        // Check that no option is initially selected.
        onView(withIndex(withId(R.id.car_ui_list_item_checkbox_widget), 0))
                .check(matches(isNotChecked()));
        onView(withIndex(withId(R.id.car_ui_list_item_checkbox_widget), 1))
                .check(matches(isNotChecked()));
        onView(withIndex(withId(R.id.car_ui_list_item_checkbox_widget), 2))
                .check(matches(isNotChecked()));

        // Select options 1 and 3.
        onView(withText(mEntries[0])).perform(click());
        onView(withText(mEntries[2])).perform(click());

        // Check that selections are correctly reflected.
        onView(withIndex(withId(R.id.car_ui_list_item_checkbox_widget), 0))
                .check(matches(isChecked()));
        onView(withIndex(withId(R.id.car_ui_list_item_checkbox_widget), 1))
                .check(matches(isNotChecked()));
        onView(withIndex(withId(R.id.car_ui_list_item_checkbox_widget), 2))
                .check(matches(isChecked()));

        // Press back to save selection.
        onView(withContentDescription("Back")).perform(click());
        Set<String> expectedUpdate = new HashSet<>();
        expectedUpdate.add(mEntriesValues[0]);
        expectedUpdate.add(mEntriesValues[2]);
        // Verify preference value was updated.
        verify(mockListener, times(1)).onPreferenceChange(any(), eq(expectedUpdate));

        // Return to multi-select list preference screen.
        onView(withText(R.string.title_multi_list_preference)).perform(click());

        // Check that selections are correctly reflected.
        onView(withIndex(withId(R.id.car_ui_list_item_checkbox_widget), 0))
                .check(matches(isChecked()));
        onView(withIndex(withId(R.id.car_ui_list_item_checkbox_widget), 1))
                .check(matches(isNotChecked()));
        onView(withIndex(withId(R.id.car_ui_list_item_checkbox_widget), 2))
                .check(matches(isChecked()));
    }

    @Test
    public void testMultiSelectListPreference_uxRestricted() {
        // Scroll until list preference is visible
        mActivity.runOnUiThread(
                () -> mActivity.scrollToPreference("multi_select_list_ux_restricted"));

        // Check that UX restriction is enabled
        CarUiMultiSelectListPreference preference =
                (CarUiMultiSelectListPreference) mActivity.findPreference(
                        "multi_select_list_ux_restricted");
        assertTrue(preference.isUxRestricted());

        // Set listener
        Consumer<Preference> clickListener = mock(Consumer.class);
        preference.setOnClickWhileRestrictedListener(clickListener);
        assertEquals(clickListener, preference.getOnClickWhileRestrictedListener());

        // Click on ux restricted preference
        onView(withText(R.string.title_multi_list_preference_ux_restricted)).perform(click());
        verify(clickListener, times(1)).accept(preference);
    }

    @Test
    public void testCheckboxPreference() {
        // Create checkbox preference and add it to screen.
        CheckBoxPreference preference = new CheckBoxPreference(mActivity);
        preference.setOrder(0);
        preference.setKey("checkbox");
        preference.setTitle(R.string.title_checkbox_preference);
        preference.setSummary(R.string.summary_compound_button_preference);
        mActivity.addPreference(preference);

        // Scroll until list preference is visible
        mActivity.runOnUiThread(() -> mActivity.scrollToPreference("checkbox"));

        // Check title and summary are displayed as expected.
        onView(withIndex(withId(android.R.id.title), 0)).check(matches(
                withText(mActivity.getString(R.string.title_checkbox_preference))));
        onView(withIndex(withId(android.R.id.summary), 0)).check(matches(
                withText(mActivity.getString(R.string.summary_compound_button_preference))));

        // Ensure checkbox preference is initially not selected.
        onView(withId(android.R.id.checkbox)).check(matches(isNotChecked()));

        Preference.OnPreferenceChangeListener mockListener = mock(
                Preference.OnPreferenceChangeListener.class);
        when(mockListener.onPreferenceChange(any(), any())).thenReturn(true);
        mActivity.setOnPreferenceChangeListener("checkbox", mockListener);

        // Select checkbox preference.
        onView(withText(R.string.title_checkbox_preference)).perform(click());

        // Verify preference value was updated.
        verify(mockListener, times(1)).onPreferenceChange(any(), eq(true));

        // Verify checkbox preference correctly indicates preference is selected.
        onView(withId(android.R.id.checkbox)).check(matches(isChecked()));

        // Un-select checkbox preference.
        onView(withText(R.string.title_checkbox_preference)).perform(click());

        // Verify preference value was updated.
        verify(mockListener, times(1)).onPreferenceChange(any(), eq(false));

        // Verify checkbox preference correctly indicates preference is selected.
        onView(withId(android.R.id.checkbox)).check(matches(isNotChecked()));
    }

    @Test
    public void testSwitchPreference() {
        // Create switch preference and add it to screen.
        CarUiSwitchPreference preference = new CarUiSwitchPreference(mActivity);
        preference.setOrder(0);
        preference.setKey("switch");
        preference.setTitle(R.string.title_switch_preference);
        preference.setSummary(R.string.summary_compound_button_preference);
        mActivity.addPreference(preference);

        // Scroll until list preference is visible
        mActivity.runOnUiThread(() -> mActivity.scrollToPreference("switch"));


        // Check title and summary are displayed as expected.
        onView(withIndex(withId(android.R.id.title), 0)).check(matches(
                withText(mActivity.getString(R.string.title_switch_preference))));
        onView(withIndex(withId(android.R.id.summary), 0)).check(matches(
                withText(mActivity.getString(R.string.summary_compound_button_preference))));

        // Ensure switch preference is initially not selected.
        onView(withId(android.R.id.switch_widget)).check(matches(isNotChecked()));

        Preference.OnPreferenceChangeListener mockListener = mock(
                Preference.OnPreferenceChangeListener.class);
        when(mockListener.onPreferenceChange(any(), any())).thenReturn(true);
        mActivity.setOnPreferenceChangeListener("switch", mockListener);

        // Select switch preference.
        onView(withText(R.string.title_switch_preference)).perform(click());

        // Verify preference value was updated.
        verify(mockListener, times(1)).onPreferenceChange(any(), eq(true));

        // Verify switch preference correctly indicates preference is selected.
        onView(withId(android.R.id.switch_widget)).check(matches(isChecked()));

        // Un-select switch preference.
        onView(withText(R.string.title_switch_preference)).perform(click());

        // Verify preference value was updated.
        verify(mockListener, times(1)).onPreferenceChange(any(), eq(false));

        // Verify switch preference correctly indicates preference is selected.
        onView(withId(android.R.id.switch_widget)).check(matches(isNotChecked()));
    }

    @Test
    public void testSwitchPreference_uxRestricted() {
        // Create switch preference and add it to screen.
        CarUiSwitchPreference preference = new CarUiSwitchPreference(mActivity);
        preference.setOrder(0);
        preference.setKey("switch");
        preference.setTitle(R.string.title_switch_preference);
        preference.setSummary(R.string.summary_compound_button_preference);
        preference.setUxRestricted(true);
        mActivity.addPreference(preference);

        // Scroll until switch preference is visible
        mActivity.runOnUiThread(() -> mActivity.scrollToPreference("switch"));

        // Check title and summary are displayed as expected.
        onView(withIndex(withId(android.R.id.title), 0)).check(matches(
                withText(mActivity.getString(R.string.title_switch_preference))));
        onView(withIndex(withId(android.R.id.summary), 0)).check(matches(
                withText(mActivity.getString(R.string.summary_compound_button_preference))));

        assertTrue(preference.isUxRestricted());

        // Set listener
        Consumer<Preference> clickListener = mock(Consumer.class);
        preference.setOnClickWhileRestrictedListener(clickListener);
        assertEquals(clickListener, preference.getOnClickWhileRestrictedListener());

        // Click on ux restricted preference
        onView(withText(R.string.title_switch_preference)).perform(click());
        verify(clickListener, times(1)).accept(preference);
    }

    @Test
    public void testRadioPreference() {
        // Create radio button preference and add it to screen.
        CarUiRadioButtonPreference preference = new CarUiRadioButtonPreference(mActivity);
        preference.setOrder(0);
        preference.setKey("radio_button");
        preference.setTitle(R.string.title_radio_button_preference);
        preference.setSummary(R.string.summary_compound_button_preference);
        mActivity.addPreference(preference);

        // Scroll until list preference is visible
        mActivity.runOnUiThread(() -> mActivity.scrollToPreference("radio_button"));

        // Check title and summary are displayed as expected.
        onView(withIndex(withId(android.R.id.title), 0)).check(matches(
                withText(mActivity.getString(R.string.title_radio_button_preference))));
        onView(withIndex(withId(android.R.id.summary), 0)).check(matches(
                withText(mActivity.getString(R.string.summary_compound_button_preference))));

        // Ensure radio button preference is initially not selected.
        onView(withId(R.id.radio_button)).check(matches(isNotChecked()));

        Preference.OnPreferenceChangeListener mockListener = mock(
                Preference.OnPreferenceChangeListener.class);
        when(mockListener.onPreferenceChange(any(), any())).thenReturn(true);
        mActivity.setOnPreferenceChangeListener("radio_button", mockListener);

        // Select radio button preference.
        onView(withText(R.string.title_radio_button_preference)).perform(click());

        // Verify preference value was updated.
        verify(mockListener, times(1)).onPreferenceChange(any(), eq(true));

        // Verify radio button preference correctly indicates preference is selected.
        onView(withId(R.id.radio_button)).check(matches(isChecked()));

        // Un-select radio button preference.
        onView(withText(R.string.title_radio_button_preference)).perform(click());

        // Verify preference value was updated.
        verify(mockListener, times(1)).onPreferenceChange(any(), eq(false));

        // Verify radio button preference correctly indicates preference is selected.
        onView(withId(R.id.radio_button)).check(matches(isNotChecked()));
    }

    @Test
    public void testRadioPreference_uxRestricted() {
        // Create radio button preference and add it to screen.
        CarUiRadioButtonPreference preference = new CarUiRadioButtonPreference(mActivity);
        preference.setOrder(0);
        preference.setKey("radio_button");
        preference.setTitle(R.string.title_radio_button_preference);
        preference.setSummary(R.string.summary_compound_button_preference);
        preference.setUxRestricted(true);
        mActivity.addPreference(preference);

        // Scroll until radio button preference is visible
        mActivity.runOnUiThread(() -> mActivity.scrollToPreference("radio_button"));

        // Check title and summary are displayed as expected.
        onView(withIndex(withId(android.R.id.title), 0)).check(matches(
                withText(mActivity.getString(R.string.title_radio_button_preference))));
        onView(withIndex(withId(android.R.id.summary), 0)).check(matches(
                withText(mActivity.getString(R.string.summary_compound_button_preference))));

        assertTrue(preference.isUxRestricted());

        // Set listener
        Consumer<Preference> clickListener = mock(Consumer.class);
        preference.setOnClickWhileRestrictedListener(clickListener);
        assertEquals(clickListener, preference.getOnClickWhileRestrictedListener());

        // Click on ux restricted preference
        onView(withText(R.string.title_radio_button_preference)).perform(click());
        verify(clickListener, times(1)).accept(preference);
    }

    @Test
    public void testDropDownPreference() {
        // Create drop-down preference and add it to screen.
        DropDownPreference preference = new CarUiDropDownPreference(mActivity);
        preference.setKey("dropdown");
        preference.setTitle(R.string.title_dropdown_preference);
        preference.setEntries(mEntries);
        preference.setEntryValues(mEntriesValues);
        preference.setOrder(0);
        mActivity.addPreference(preference);

        // Display full screen list preference.
        onView(withText(R.string.title_dropdown_preference)).perform(click());

        Preference.OnPreferenceChangeListener mockListener = mock(
                Preference.OnPreferenceChangeListener.class);
        when(mockListener.onPreferenceChange(any(), any())).thenReturn(true);
        mActivity.setOnPreferenceChangeListener("dropdown", mockListener);

        // Check that first option is initially selected.
        onView(withIndex(withId(R.id.car_ui_list_item_radio_button_widget), 0))
                .check(matches(isChecked()));
        onView(withIndex(withId(R.id.car_ui_list_item_radio_button_widget), 1))
                .check(matches(isNotChecked()));
        onView(withIndex(withId(R.id.car_ui_list_item_radio_button_widget), 2))
                .check(matches(isNotChecked()));

        // Select third option.
        onView(withText(mEntries[2])).perform(click());
        // Check that first option is selected.
        onView(withIndex(withId(R.id.car_ui_list_item_radio_button_widget), 0))
                .check(matches(isNotChecked()));
        onView(withIndex(withId(R.id.car_ui_list_item_radio_button_widget), 1))
                .check(matches(isNotChecked()));
        onView(withIndex(withId(R.id.car_ui_list_item_radio_button_widget), 2))
                .check(matches(isChecked()));

        // Press back to save selection.
        onView(withContentDescription("Back")).perform(click());
        // Verify preference value was updated.
        verify(mockListener, times(1)).onPreferenceChange(any(), eq(mEntriesValues[2]));

        onView(withText(R.string.title_dropdown_preference)).perform(click());

        // Check that first option is selected.
        onView(withIndex(withId(R.id.car_ui_list_item_radio_button_widget), 0))
                .check(matches(isNotChecked()));
        onView(withIndex(withId(R.id.car_ui_list_item_radio_button_widget), 1))
                .check(matches(isNotChecked()));
        onView(withIndex(withId(R.id.car_ui_list_item_radio_button_widget), 2))
                .check(matches(isChecked()));
    }

    @Test
    public void testDropDownPreference_uxRestricted() {
        // Create drop-down preference and add it to screen.
        CarUiDropDownPreference preference = new CarUiDropDownPreference(mActivity);
        preference.setKey("dropdown");
        preference.setTitle(R.string.title_dropdown_preference);
        preference.setEntries(mEntries);
        preference.setEntryValues(mEntriesValues);
        preference.setOrder(0);
        preference.setUxRestricted(true);
        mActivity.addPreference(preference);

        // Scroll until drop-down preference button preference is visible
        mActivity.runOnUiThread(() -> mActivity.scrollToPreference("dropdown"));

        // Check title is displayed as expected.
        onView(withText(R.string.title_dropdown_preference)).check(matches(isDisplayed()));

        assertTrue(preference.isUxRestricted());

        // Set listener
        Consumer<Preference> clickListener = mock(Consumer.class);
        preference.setOnClickWhileRestrictedListener(clickListener);
        assertEquals(clickListener, preference.getOnClickWhileRestrictedListener());

        // Click on ux restricted preference
        onView(withText(R.string.title_dropdown_preference)).perform(click());
        verify(clickListener, times(1)).accept(preference);
    }

    @Test
    public void testTwoActionPreference() {
        // Create CarUiTwoActionPreference preference and add it to screen.
        CarUiTwoActionPreference preference = new CarUiTwoActionPreference(mActivity);
        preference.setKey("twoaction");
        preference.setTitle(R.string.title_twoaction_preference);
        preference.setSummary(R.string.summary_twoaction_preference);
        preference.setOrder(0);
        preference.setWidgetLayoutResource(R.layout.details_preference_widget);
        mActivity.addPreference(preference);

        // Check title is displayed as expected.
        onView(withText(R.string.title_twoaction_preference)).check(matches(isDisplayed()));

        // Hide second action.
        mActivity.runOnUiThread(() -> preference.showAction(false));

        // Ensure second action isn't displayed.
        onView(withIndex(withId(com.android.car.ui.R.id.action_widget_container), 0)).check(
                matches(not(isDisplayed())));
    }

    @Test
    public void testTwoActionPreference_uxRestricted() {
        // Create CarUiTwoActionPreference preference and add it to screen.
        CarUiTwoActionPreference preference = new CarUiTwoActionPreference(mActivity);
        preference.setKey("twoaction");
        preference.setTitle(R.string.title_twoaction_preference);
        preference.setSummary(R.string.summary_twoaction_preference);
        preference.setOrder(0);
        preference.setWidgetLayoutResource(R.layout.details_preference_widget);
        preference.setUxRestricted(true);
        mActivity.addPreference(preference);

        // Scroll until CarUiTwoActionPreference preference button preference is visible
        mActivity.runOnUiThread(() -> mActivity.scrollToPreference("twoaction"));

        // Check title is displayed as expected.
        onView(withText(R.string.title_twoaction_preference)).check(matches(isDisplayed()));
        onView(withText(R.string.summary_twoaction_preference)).check(matches(isDisplayed()));

        assertTrue(preference.isUxRestricted());

        // Set listener
        Consumer<Preference> clickListener = mock(Consumer.class);
        preference.setOnClickWhileRestrictedListener(clickListener);
        assertEquals(clickListener, preference.getOnClickWhileRestrictedListener());

        // Click on ux restricted preference
        onView(withText(R.string.title_twoaction_preference)).perform(click());
        verify(clickListener, times(1)).accept(preference);
    }

    @Test
    public void testTwoActionIconPreference() {
        // Create CarUiTwoActionIconPreference preference and add it to screen.
        CarUiTwoActionIconPreference preference = new CarUiTwoActionIconPreference(mActivity);
        preference.setKey("twoaction");
        preference.setTitle(R.string.title_twoaction_preference);
        preference.setSummary(R.string.summary_twoaction_preference);
        preference.setOrder(0);
        preference.setSecondaryActionIcon(R.drawable.avd_show_password);
        Runnable clickListener = mock(Runnable.class);
        preference.setOnSecondaryActionClickListener(clickListener);
        mActivity.addPreference(preference);

        // Check title is displayed as expected.
        onView(withText(R.string.title_twoaction_preference)).check(matches(isDisplayed()));
        onView(withText(R.string.summary_twoaction_preference)).check(matches(isDisplayed()));

        // Check that clicks on title doesn't fire icon listener
        onView(withText(R.string.title_twoaction_preference)).perform(click());
        verify(clickListener, times(0)).run();

        // Click on icon.
        onView(withIndex(withId(com.android.car.ui.R.id.car_ui_second_action_container),
                0)).perform(click());
        verify(clickListener, times(1)).run();
    }

    @Test
    public void testTwoActionIconPreference_uxRestricted() {
        // Create CarUiTwoActionIconPreference preference and add it to screen.
        CarUiTwoActionIconPreference preference = new CarUiTwoActionIconPreference(mActivity);
        preference.setKey("twoaction");
        preference.setTitle(R.string.title_twoaction_preference);
        preference.setSummary(R.string.summary_twoaction_preference);
        preference.setOrder(0);
        preference.setSecondaryActionIcon(R.drawable.avd_show_password);
        Runnable clickListener = mock(Runnable.class);
        preference.setOnSecondaryActionClickListener(clickListener);
        preference.setUxRestricted(true);
        mActivity.addPreference(preference);

        // Scroll until CarUiTwoActionIconPreference preference button preference is visible
        mActivity.runOnUiThread(() -> mActivity.scrollToPreference("twoaction"));

        // Check title is displayed as expected.
        onView(withText(R.string.title_twoaction_preference)).check(matches(isDisplayed()));
        onView(withText(R.string.summary_twoaction_preference)).check(matches(isDisplayed()));

        assertTrue(preference.isUxRestricted());

        // Set listener
        Consumer<Preference> restrictedClickListener = mock(Consumer.class);
        preference.setOnClickWhileRestrictedListener(restrictedClickListener);
        assertEquals(restrictedClickListener, preference.getOnClickWhileRestrictedListener());

        // Click on ux restricted preference
        onView(withText(R.string.title_twoaction_preference)).perform(click());
        verify(restrictedClickListener, times(1)).accept(preference);
    }

    @Test
    public void testTwoActionTextPreference() {
        // Create CarUiTwoActionTextPreference preference and add it to screen.
        CarUiTwoActionTextPreference preference = new CarUiTwoActionTextPreference(mActivity);
        preference.setKey("twoaction");
        preference.setTitle(R.string.title_twoaction_preference);
        preference.setSummary(R.string.summary_twoaction_preference);
        preference.setOrder(0);
        preference.setSecondaryActionText(R.string.twoaction_secondary_text);
        Runnable clickListener = mock(Runnable.class);
        preference.setOnSecondaryActionClickListener(clickListener);
        mActivity.addPreference(preference);

        // Check title is displayed as expected.
        onView(withText(R.string.title_twoaction_preference)).check(matches(isDisplayed()));
        onView(withText(R.string.summary_twoaction_preference)).check(matches(isDisplayed()));

        // Check that clicks on title doesn't fire icon listener
        onView(withText(R.string.title_twoaction_preference)).perform(click());
        verify(clickListener, times(0)).run();

        // Click on secondary text.
        onView(withText(R.string.twoaction_secondary_text)).perform(click());
        verify(clickListener, times(1)).run();

        // Click on secondary text when disabled
        mActivity.runOnUiThread(() -> preference.setSecondaryActionEnabled(false));
        onView(withText(R.string.twoaction_secondary_text)).perform(click());
        assertFalse(preference.isSecondaryActionEnabled());
        verify(clickListener, times(1)).run();

        // Make secondary text not visible
        mActivity.runOnUiThread(() -> preference.setSecondaryActionVisible(false));
        onView(withText(R.string.twoaction_secondary_text)).check(matches(not(isDisplayed())));
        assertFalse(preference.isSecondaryActionVisible());

        // Use performSecondaryActionClick()
        mActivity.runOnUiThread(() -> {
            preference.setSecondaryActionVisible(true);
            preference.setSecondaryActionEnabled(true);
        });
        onView(withText(R.string.twoaction_secondary_text)).check(matches(isDisplayed()));
        preference.performSecondaryActionClick();
        verify(clickListener, times(2)).run();
    }

    @Test
    public void testTwoActionTextPreference_uxRestricted() {
        // Create CarUiTwoActionTextPreference preference and add it to screen.
        CarUiTwoActionTextPreference preference = new CarUiTwoActionTextPreference(mActivity);
        preference.setKey("twoaction");
        preference.setTitle(R.string.title_twoaction_preference);
        preference.setSummary(R.string.summary_twoaction_preference);
        preference.setOrder(0);
        preference.setSecondaryActionText(R.string.twoaction_secondary_text);
        Runnable clickListener = mock(Runnable.class);
        preference.setOnSecondaryActionClickListener(clickListener);
        preference.setUxRestricted(true);
        mActivity.addPreference(preference);

        // Scroll until CarUiTwoActionTextPreference preference button preference is visible
        mActivity.runOnUiThread(() -> mActivity.scrollToPreference("twoaction"));

        // Check title is displayed as expected.
        onView(withText(R.string.title_twoaction_preference)).check(matches(isDisplayed()));
        onView(withText(R.string.summary_twoaction_preference)).check(matches(isDisplayed()));

        assertTrue(preference.isUxRestricted());

        // Set listener
        Consumer<Preference> restrictedClickListener = mock(Consumer.class);
        preference.setOnClickWhileRestrictedListener(restrictedClickListener);
        assertEquals(restrictedClickListener, preference.getOnClickWhileRestrictedListener());

        // Click on ux restricted preference
        onView(withText(R.string.title_twoaction_preference)).perform(click());
        verify(restrictedClickListener, times(1)).accept(preference);
    }

    @Test
    public void testTwoActionSwitchPreference() {
        // Create CarUiTwoActionSwitchPreference preference and add it to screen.
        CarUiTwoActionSwitchPreference preference = new CarUiTwoActionSwitchPreference(mActivity);
        preference.setKey("twoaction");
        preference.setTitle(R.string.title_twoaction_preference);
        preference.setSummary(R.string.summary_twoaction_preference);
        preference.setOrder(0);
        Consumer<Boolean> clickListener = mock(Consumer.class);
        preference.setOnSecondaryActionClickListener(clickListener);
        mActivity.addPreference(preference);

        // Check title is displayed as expected.
        onView(withText(R.string.title_twoaction_preference)).check(matches(isDisplayed()));
        onView(withText(R.string.summary_twoaction_preference)).check(matches(isDisplayed()));

        // Check that clicks on title doesn't fire icon listener
        onView(withText(R.string.title_twoaction_preference)).perform(click());
        verify(clickListener, times(0)).accept(anyBoolean());

        // Click on switch.
        onView(withIndex(withId(com.android.car.ui.R.id.car_ui_second_action_container),
                0)).perform(click());
        verify(clickListener, times(1)).accept(true);
    }

    @Test
    public void testTwoActionSwitchPreference_uxRestricted() {
        // Create CarUiTwoActionSwitchPreference preference and add it to screen.
        CarUiTwoActionSwitchPreference preference = new CarUiTwoActionSwitchPreference(mActivity);
        preference.setKey("twoaction");
        preference.setTitle(R.string.title_twoaction_preference);
        preference.setSummary(R.string.summary_twoaction_preference);
        preference.setOrder(0);
        Consumer<Boolean> clickListener = mock(Consumer.class);
        preference.setOnSecondaryActionClickListener(clickListener);
        preference.setUxRestricted(true);
        mActivity.addPreference(preference);

        // Scroll until CarUiTwoActionSwitchPreference preference button preference is visible
        mActivity.runOnUiThread(() -> mActivity.scrollToPreference("twoaction"));

        // Check title is displayed as expected.
        onView(withText(R.string.title_twoaction_preference)).check(matches(isDisplayed()));
        onView(withText(R.string.summary_twoaction_preference)).check(matches(isDisplayed()));

        assertTrue(preference.isUxRestricted());

        // Set listener
        Consumer<Preference> restrictedClickListener = mock(Consumer.class);
        preference.setOnClickWhileRestrictedListener(restrictedClickListener);
        assertEquals(restrictedClickListener, preference.getOnClickWhileRestrictedListener());

        // Click on ux restricted preference
        onView(withText(R.string.title_twoaction_preference)).perform(click());
        verify(restrictedClickListener, times(1)).accept(preference);
    }

    @Test
    public void testEditTextPreference() {
        // Create CarUiEditTextPreference preference and add it to screen.
        CarUiEditTextPreference preference = new CarUiEditTextPreference(mActivity);
        preference.setKey("editText");
        preference.setTitle(R.string.title_edit_text_preference);
        preference.setOrder(0);
        String positiveButtonText = "Ok";
        preference.setPositiveButtonText(positiveButtonText);
        preference.setDialogTitle(R.string.dialog_title_edit_text_preference);
        preference.setSummaryProvider(EditTextPreference.SimpleSummaryProvider.getInstance());
        preference.setDialogIcon(R.drawable.avd_hide_password);
        mActivity.addPreference(preference);

        // Check title is displayed as expected.
        onView(withText(R.string.title_edit_text_preference)).check(matches(isDisplayed()));

        // Click on preference
        onView(withText(R.string.title_edit_text_preference)).perform(click());
        onView(withText(R.string.dialog_title_edit_text_preference)).check(matches(isDisplayed()));

        // Enter value
        String value = "test value";
        onView(withId(android.R.id.edit)).perform(typeText(value));
        onView(withText(positiveButtonText)).perform(click());

        // Confirm value updated by simple summary provider
        onView(withText(value)).check(matches(isDisplayed()));
    }

    @Test
    public void testEditTextPreference_uxRestricted() {
        // Create CarUiEditTextPreference preference and add it to screen.
        CarUiEditTextPreference preference = new CarUiEditTextPreference(mActivity);
        preference.setKey("editText");
        preference.setTitle(R.string.title_edit_text_preference);
        preference.setOrder(0);
        preference.setDialogTitle(R.string.dialog_title_edit_text_preference);
        preference.setUxRestricted(true);
        mActivity.addPreference(preference);

        // Scroll until CarUiEditTextPreference preference button preference is visible
        mActivity.runOnUiThread(() -> mActivity.scrollToPreference("editText"));

        // Check title is displayed as expected.
        onView(withText(R.string.title_edit_text_preference)).check(matches(isDisplayed()));

        assertTrue(preference.isUxRestricted());

        // Set listener
        Consumer<Preference> restrictedClickListener = mock(Consumer.class);
        preference.setOnClickWhileRestrictedListener(restrictedClickListener);
        assertEquals(restrictedClickListener, preference.getOnClickWhileRestrictedListener());

        // Click on ux restricted preference
        onView(withText(R.string.title_edit_text_preference)).perform(click());
        verify(restrictedClickListener, times(1)).accept(preference);
    }

    @Test
    public void testSeekBarPreference() {
        // Create CarUiSeekBarDialogPreference preference and add it to screen.
        CarUiSeekBarDialogPreference preference = new CarUiSeekBarDialogPreference(mActivity);
        preference.setKey("seek_bar");
        preference.setTitle(R.string.title_seek_bar_preference);
        preference.setOrder(0);
        String positiveButtonText = "Ok";
        preference.setPositiveButtonText(positiveButtonText);
        String negativeButtonText = "Cancel";
        preference.setNegativeButtonText(negativeButtonText);
        preference.setDialogTitle(R.string.dialog_title_seek_bar_preference);
        preference.setMaxProgress(20);
        mActivity.addPreference(preference);

        // Check title is displayed as expected.
        onView(withText(R.string.title_seek_bar_preference)).check(matches(isDisplayed()));

        // Click on preference
        onView(withText(R.string.title_seek_bar_preference)).perform(click());
        onView(withText(R.string.dialog_title_seek_bar_preference)).check(matches(isDisplayed()));
        onView(withText(positiveButtonText)).check(matches(isDisplayed()));
        onView(withText(negativeButtonText)).check(matches(isDisplayed()));

        // Confirm progress is set to 0
        assertEquals(0, preference.getProgress());
        assertEquals(20, preference.getMaxProgress());

        // Set progress
        int progress = 10;
        onView(withId(R.id.seek_bar)).perform(setProgress(progress));
        assertEquals(progress, preference.getProgress());
    }

    @Test
    public void testSeekBarPreference_clickOk_valueSaved() {
        // Create CarUiSeekBarDialogPreference preference and add it to screen.
        CarUiSeekBarDialogPreference preference = new CarUiSeekBarDialogPreference(mActivity);
        preference.setKey("seek_bar");
        preference.setTitle(R.string.title_seek_bar_preference);
        preference.setOrder(0);
        String positiveButtonText = "Ok";
        preference.setPositiveButtonText(positiveButtonText);
        String negativeButtonText = "Cancel";
        preference.setNegativeButtonText(negativeButtonText);
        preference.setDialogTitle(R.string.dialog_title_seek_bar_preference);
        preference.setMaxProgress(20);
        mActivity.addPreference(preference);

        // Check title is displayed as expected.
        onView(withText(R.string.title_seek_bar_preference)).check(matches(isDisplayed()));

        // Click on preference, set progress, then hit ok
        onView(withText(R.string.title_seek_bar_preference)).perform(click());
        onView(withId(R.id.seek_bar)).perform(setProgress(10));
        onView(withText(positiveButtonText)).perform(click());
        onView(withText(R.string.dialog_title_seek_bar_preference)).check(doesNotExist());

        // Confirm progress is set to 0
        assertEquals(10, preference.getProgress());
        assertEquals(20, preference.getMaxProgress());
    }

    @Test
    public void testSeekBarPreference_clickCancel_valueNotSaved() {
        // Create CarUiSeekBarDialogPreference preference and add it to screen.
        CarUiSeekBarDialogPreference preference = new CarUiSeekBarDialogPreference(mActivity);
        preference.setKey("seek_bar");
        preference.setTitle(R.string.title_seek_bar_preference);
        preference.setOrder(0);
        String positiveButtonText = "Ok";
        preference.setPositiveButtonText(positiveButtonText);
        String negativeButtonText = "Cancel";
        preference.setNegativeButtonText(negativeButtonText);
        preference.setDialogTitle(R.string.dialog_title_seek_bar_preference);
        preference.setMaxProgress(20);
        mActivity.addPreference(preference);

        // Check title is displayed as expected.
        onView(withText(R.string.title_seek_bar_preference)).check(matches(isDisplayed()));

        // Click on preference, set progress, then hit cancel
        onView(withText(R.string.title_seek_bar_preference)).perform(click());
        onView(withId(R.id.seek_bar)).perform(setProgress(10));
        onView(withText(negativeButtonText)).perform(click());
        onView(withText(R.string.dialog_title_seek_bar_preference)).check(doesNotExist());

        // Confirm progress is set to 0
        assertEquals(0, preference.getProgress());
        assertEquals(20, preference.getMaxProgress());
    }

    @Test
    public void test_defaultDialogFragmentCallbacks_doNothing() {
        DialogFragmentCallbacks callbacks = new DialogFragmentCallbacks() {
        };

        callbacks.onBindDialogView(new View(mActivity));
        callbacks.onClick(new DialogInterface() {
            @Override
            public void cancel() {
            }

            @Override
            public void dismiss() {
            }
        }, 0);
        callbacks.onDialogClosed(true);
        callbacks.onPrepareDialogBuilder(new AlertDialog.Builder(mActivity));
    }

    @Test
    public void testSeekBarPreference_uxRestricted() {
        // Create CarUiSeekBarDialogPreference preference and add it to screen.
        CarUiSeekBarDialogPreference preference = new CarUiSeekBarDialogPreference(mActivity);
        preference.setKey("seek_bar");
        preference.setTitle(R.string.title_seek_bar_preference);
        preference.setOrder(0);
        preference.setPositiveButtonText("Ok");
        preference.setNegativeButtonText("Cancel");
        preference.setDialogTitle(R.string.dialog_title_seek_bar_preference);
        preference.setMaxProgress(20);
        preference.setUxRestricted(true);
        mActivity.addPreference(preference);

        // Scroll until CarUiEditTextPreference preference button preference is visible
        mActivity.runOnUiThread(() -> mActivity.scrollToPreference("seek_bar"));

        // Check title is displayed as expected.
        onView(withText(R.string.title_seek_bar_preference)).check(matches(isDisplayed()));

        assertTrue(preference.isUxRestricted());

        // Set listener
        Consumer<Preference> restrictedClickListener = mock(Consumer.class);
        preference.setOnClickWhileRestrictedListener(restrictedClickListener);
        assertEquals(restrictedClickListener, preference.getOnClickWhileRestrictedListener());

        // Click on ux restricted preference
        onView(withText(R.string.title_seek_bar_preference)).perform(click());
        verify(restrictedClickListener, times(1)).accept(preference);
    }
}
