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

package com.android.car.settings.common;

import static com.google.common.truth.Truth.assertThat;

import static org.mockito.ArgumentMatchers.any;
import static org.mockito.Mockito.never;
import static org.mockito.Mockito.verify;

import android.content.Context;
import android.view.LayoutInflater;
import android.widget.FrameLayout;

import androidx.preference.Preference;
import androidx.preference.PreferenceViewHolder;
import androidx.test.core.app.ApplicationProvider;
import androidx.test.ext.junit.runners.AndroidJUnit4;

import com.android.car.settings.R;

import org.junit.Before;
import org.junit.Test;
import org.junit.runner.RunWith;
import org.mockito.Mock;
import org.mockito.MockitoAnnotations;

import java.util.function.Consumer;

@RunWith(AndroidJUnit4.class)
public class ClickableWhileDisabledSwitchPreferenceTest {

    private Context mContext = ApplicationProvider.getApplicationContext();
    private FrameLayout mWidgetView;
    private ClickableWhileDisabledSwitchPreference mPref;
    private PreferenceViewHolder mHolder;

    @Mock
    private Consumer<Preference> mPreferenceConsumer;

    @Before
    public void setUp() {
        MockitoAnnotations.initMocks(this);

        LayoutInflater inflater = LayoutInflater.from(mContext);
        mHolder = PreferenceViewHolder.createInstanceForTests(inflater.inflate(
                R.layout.car_ui_preference, null));
        mWidgetView = mHolder.itemView.findViewById(android.R.id.widget_frame);
        inflater.inflate(R.layout.car_ui_preference_widget_switch, mWidgetView,
                /* attachToRoot= */ true);
        mPref = new ClickableWhileDisabledSwitchPreference(mContext);
        mPref.setDisabledClickListener(mPreferenceConsumer);
    }

    @Test
    public void isDisabled_isClickable() {
        mPref.setEnabled(false);
        mPref.onBindViewHolder(mHolder);

        assertThat(mHolder.itemView.isClickable()).isTrue();
    }

    @Test
    public void onClick_isDisabled_handleWithDisabledClickHandler() {
        mPref.setEnabled(false);
        mPref.onBindViewHolder(mHolder);

        mPref.performClick();

        verify(mPreferenceConsumer).accept(any(Preference.class));
    }

    @Test
    public void onClick_isEnabled_handleWithRegularClickHandler() {
        mPref.setEnabled(true);
        mPref.onBindViewHolder(mHolder);

        mPref.performClick();

        verify(mPreferenceConsumer, never()).accept(any(Preference.class));
    }

    @Test
    public void onBindViewHolder_setContentDescription_shouldSetDescription() {
        String description = "test contents description";
        mPref.setContentDescription(description);

        mPref.onBindViewHolder(mHolder);

        assertThat(mHolder.itemView.getContentDescription()).isEqualTo(description);
    }
}
