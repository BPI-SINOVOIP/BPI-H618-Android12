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

package com.android.car.dialer.ui.contact;

import static androidx.test.espresso.Espresso.onView;
import static androidx.test.espresso.assertion.ViewAssertions.matches;
import static androidx.test.espresso.matcher.ViewMatchers.hasDescendant;
import static androidx.test.espresso.matcher.ViewMatchers.hasSibling;
import static androidx.test.espresso.matcher.ViewMatchers.isClickable;
import static androidx.test.espresso.matcher.ViewMatchers.isNotClickable;
import static androidx.test.espresso.matcher.ViewMatchers.withId;
import static androidx.test.espresso.matcher.ViewMatchers.withText;

import static com.android.car.dialer.testing.TestViewActions.selfClick;
import static com.android.car.dialer.testing.TestViewMatchers.atPosition;
import static com.android.car.dialer.testing.TestViewMatchers.isActivated;

import static org.hamcrest.Matchers.allOf;
import static org.mockito.ArgumentMatchers.any;
import static org.mockito.ArgumentMatchers.eq;
import static org.mockito.Mockito.mock;
import static org.mockito.Mockito.verify;
import static org.mockito.Mockito.when;

import android.content.Context;
import android.os.Bundle;
import android.view.View;

import androidx.annotation.NonNull;
import androidx.annotation.Nullable;
import androidx.fragment.app.Fragment;
import androidx.fragment.app.FragmentManager;
import androidx.lifecycle.LiveData;
import androidx.lifecycle.MutableLiveData;
import androidx.lifecycle.ViewModel;
import androidx.lifecycle.ViewModelProvider;
import androidx.test.core.app.ActivityScenario;
import androidx.test.espresso.contrib.RecyclerViewActions;
import androidx.test.ext.junit.runners.AndroidJUnit4;

import com.android.car.arch.common.FutureData;
import com.android.car.dialer.R;
import com.android.car.dialer.telecom.UiCallManager;
import com.android.car.dialer.testing.TestActivity;
import com.android.car.telephony.common.Contact;
import com.android.car.telephony.common.PhoneNumber;

import org.hamcrest.Matcher;
import org.junit.Before;
import org.junit.Test;
import org.junit.runner.RunWith;
import org.mockito.Mock;
import org.mockito.MockitoAnnotations;

import java.util.Arrays;

@RunWith(AndroidJUnit4.class)
public class ContactDetailsFragmentTest {
    private static final String DISPLAY_NAME = "NAME";
    private static final String[] RAW_NUMBERS = {"6505550000", "6502370000"};

    @Mock
    private Contact mMockContact;
    @Mock
    private PhoneNumber mMockPhoneNumber1;
    @Mock
    private PhoneNumber mMockPhoneNumber2;
    @Mock
    private UiCallManager mMockUiCallManager;

    // This is initialized in the TestContactDetailsFragment#getDefaultViewModelProviderFactory();
    private ContactDetailsViewModel mViewModel;

    private final FragmentManager.FragmentLifecycleCallbacks mCallbacks =
            new FragmentManager.FragmentLifecycleCallbacks() {
                @Override
                public void onFragmentAttached(@NonNull FragmentManager fm, @NonNull Fragment f,
                        @NonNull Context context) {
                    // Set up the mock ContactDetailsViewModel.
                    mViewModel = new ViewModelProvider(f).get(ContactDetailsViewModel.class);
                    LiveData<FutureData<Contact>> contactDetail = new MutableLiveData<>(
                            FutureData.newLoadedData(mMockContact));
                    when(mViewModel.getContactDetails(any())).thenReturn(contactDetail);
                }

                @Override
                public void onFragmentCreated(@NonNull FragmentManager fm, @NonNull Fragment f,
                        @Nullable Bundle savedInstanceState) {
                    if (f instanceof ContactDetailsFragment) {
                        // Set up adapter with the mock UiCallManager.
                        ContactDetailsViewHolder.Factory viewHolderFactory =
                                (v, phoneNumberPresenter) -> new ContactDetailsViewHolder(v,
                                        phoneNumberPresenter, mMockUiCallManager);
                        ((ContactDetailsFragment) f).mContactDetailsAdapterFactory =
                                (contact, phoneNumberPresenter) -> new ContactDetailsAdapter(
                                        f.getContext(), viewHolderFactory, contact,
                                        phoneNumberPresenter);
                        fm.unregisterFragmentLifecycleCallbacks(mCallbacks);
                    }
                }
            };

    @Before
    public void setUp() {
        MockitoAnnotations.initMocks(this);

        when(mMockContact.getDisplayName()).thenReturn(DISPLAY_NAME);
        when(mMockPhoneNumber1.getRawNumber()).thenReturn(RAW_NUMBERS[0]);
        when(mMockPhoneNumber2.getRawNumber()).thenReturn(RAW_NUMBERS[1]);
        when(mMockContact.getNumbers()).thenReturn(
                Arrays.asList(mMockPhoneNumber1, mMockPhoneNumber2));

    }

    @Test
    public void testCreateWithContact() {
        setUpFragment();

        onView(withId(R.id.list_view))
                .perform(RecyclerViewActions.scrollToPosition(0))
                .check(matches(atPosition(0, hasDescendant(
                        allOf(withId(R.id.title), withText(DISPLAY_NAME))))))
                .check(matches(atPosition(0, isNotClickable())));

        verifyPhoneNumber(1);
        verifyPhoneNumber(2);
    }

    private void setUpFragment() {
        ActivityScenario<TestActivity> activityScenario = ActivityScenario.launch(
                TestActivity.class);
        activityScenario.onActivity(activity -> {
            ContactDetailsFragment fragment = new TestContactDetailsFragment();

            activity.getSupportFragmentManager().registerFragmentLifecycleCallbacks(
                    mCallbacks, /* recursive= */false);

            activity.getSupportFragmentManager().beginTransaction().add(
                    R.id.test_fragment_container, fragment).commit();
        });
    }

    /**
     * Verify the phone numbers for the Contact
     */
    private void verifyPhoneNumber(int position) {
        onView(withId(R.id.list_view))
                .perform(RecyclerViewActions.scrollToPosition(position))
                .check(matches(atPosition(position, hasDescendant(
                        allOf(withId(R.id.title), withText(RAW_NUMBERS[position - 1]))))))
                .check(matches(atPosition(position, hasDescendant(
                        allOf(withId(R.id.call_action_id), isClickable())))));

        Matcher<View> callActionViewMatcher = allOf(
                withId(R.id.call_action_id),
                hasDescendant(withText(RAW_NUMBERS[position - 1])));
        onView(callActionViewMatcher).perform(selfClick());
        verify(mMockUiCallManager).placeCall(RAW_NUMBERS[position - 1]);

        onView(allOf(
                withId(R.id.contact_details_favorite_button), hasSibling(callActionViewMatcher)))
                .check(matches(isActivated(false)))
                .perform(selfClick())
                .check(matches(isActivated(true)));
        verify(mViewModel).addToFavorite(eq(mMockContact),
                eq(position == 1 ? mMockPhoneNumber1 : mMockPhoneNumber2));
    }

    /** A test override that creates mock {@link ViewModel}s. */
    public static class TestContactDetailsFragment extends ContactDetailsFragment {

        @Override
        public ViewModelProvider.Factory getDefaultViewModelProviderFactory() {
            return new ViewModelProvider.Factory() {
                @NonNull
                @Override
                public <T extends ViewModel> T create(@NonNull Class<T> aClass) {
                    return mock(aClass);
                }
            };
        }
    }
}
