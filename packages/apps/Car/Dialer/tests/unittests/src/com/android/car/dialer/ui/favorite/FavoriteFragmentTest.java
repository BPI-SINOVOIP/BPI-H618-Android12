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

package com.android.car.dialer.ui.favorite;

import static androidx.test.espresso.Espresso.onView;
import static androidx.test.espresso.action.ViewActions.click;
import static androidx.test.espresso.matcher.ViewMatchers.withId;

import static com.google.common.truth.Truth.assertThat;

import static org.mockito.ArgumentMatchers.any;
import static org.mockito.Mockito.mock;
import static org.mockito.Mockito.never;
import static org.mockito.Mockito.verify;
import static org.mockito.Mockito.when;

import androidx.lifecycle.MutableLiveData;
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

import org.junit.Before;
import org.junit.Test;
import org.junit.runner.RunWith;
import org.mockito.ArgumentCaptor;
import org.mockito.Mock;
import org.mockito.MockitoAnnotations;

import java.util.Arrays;
import java.util.List;

@RunWith(AndroidJUnit4.class)
public class FavoriteFragmentTest {
    private static final String RAW_NUMBER = "6502530000";

    private FavoriteFragment mFavoriteFragment;
    @Mock
    private UiCallManager mMockUiCallManager;
    @Mock
    private Contact mMockContact;

    private FavoriteViewModel mFavoriteViewModel;
    @Mock
    private PhoneNumber mMockPhoneNumber;

    ActivityScenario<TestActivity> mActivityScenario;

    @Before
    public void setUp() {
        MockitoAnnotations.initMocks(this);
        startActivity();
    }

    private void startActivity() {
        when(mMockPhoneNumber.getRawNumber()).thenReturn(RAW_NUMBER);
        MutableLiveData<FutureData<List<Object>>> favoriteContacts = new MutableLiveData<>(
                new FutureData<>(false, Arrays.asList(mMockContact)));

        mActivityScenario = ActivityScenario.launch(TestActivity.class);
        mActivityScenario.onActivity(activity -> {
            mFavoriteViewModel = new ViewModelProvider(activity).get(
                    FavoriteViewModel.class);
            when(mFavoriteViewModel.getFavoriteContacts()).thenReturn(favoriteContacts);
            when(mFavoriteViewModel.getSortOrderLiveData()).thenReturn(new MutableLiveData<>(1));

            mFavoriteFragment = FavoriteFragment.newInstance();

            activity.getSupportFragmentManager().beginTransaction().add(
                    R.id.test_fragment_container, mFavoriteFragment).commit();
        });

        mActivityScenario.onActivity(activity -> {
            mFavoriteFragment.mUiCallManager = mMockUiCallManager;
        });
    }

    @Test
    public void testOnItemClick_contactHasPrimaryNumber_placeCall() {
        when(mMockContact.getNumbers()).thenReturn(Arrays.asList(mMockPhoneNumber));
        when(mMockContact.hasPrimaryPhoneNumber()).thenReturn(true);
        when(mMockContact.getPrimaryPhoneNumber()).thenReturn(mMockPhoneNumber);

        onView(withId(R.id.list_view)).perform(
                RecyclerViewActions.actionOnItemAtPosition(0, click()));

        ArgumentCaptor<String> mCaptor = ArgumentCaptor.forClass(String.class);
        verify(mMockUiCallManager).placeCall(mCaptor.capture());
        assertThat(mCaptor.getValue()).isEqualTo(RAW_NUMBER);
    }

    @Test
    public void testOnItemClick_contactHasOnlyOneNumber_placeCall() {
        when(mMockContact.hasPrimaryPhoneNumber()).thenReturn(false);
        when(mMockContact.getNumbers()).thenReturn(Arrays.asList(mMockPhoneNumber));

        onView(withId(R.id.list_view)).perform(
                RecyclerViewActions.actionOnItemAtPosition(0, click()));

        ArgumentCaptor<String> mCaptor = ArgumentCaptor.forClass(String.class);
        verify(mMockUiCallManager).placeCall(mCaptor.capture());
        assertThat(mCaptor.getValue()).isEqualTo(RAW_NUMBER);
    }

    @Test
    public void testOnItemClick_contactHasMultiNumbers_notPlaceCall() {
        when(mMockContact.hasPrimaryPhoneNumber()).thenReturn(false);
        PhoneNumber otherMockPhoneNumber = mock(PhoneNumber.class);
        when(mMockContact.getNumbers()).thenReturn(
                Arrays.asList(mMockPhoneNumber, otherMockPhoneNumber));

        onView(withId(R.id.list_view)).perform(
                RecyclerViewActions.actionOnItemAtPosition(0, click()));

        verify(mMockUiCallManager, never()).placeCall(any());
    }
}
