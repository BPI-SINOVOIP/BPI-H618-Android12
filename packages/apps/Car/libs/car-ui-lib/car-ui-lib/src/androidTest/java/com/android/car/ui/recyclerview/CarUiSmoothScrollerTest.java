/*
 * Copyright 2021 The Android Open Source Project
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
package com.android.car.ui.recyclerview;

import static androidx.recyclerview.widget.LinearSmoothScroller.SNAP_TO_START;

import static com.google.common.truth.Truth.assertThat;

import androidx.test.core.app.ActivityScenario;
import androidx.test.ext.junit.rules.ActivityScenarioRule;

import com.android.car.ui.TestActivity;

import org.junit.Before;
import org.junit.Rule;
import org.junit.Test;

/**
 * Unit tests for {@link CarUiSmoothScroller}.
 */
public class CarUiSmoothScrollerTest {

    @Rule
    public ActivityScenarioRule<TestActivity> mActivityRule =
            new ActivityScenarioRule<>(TestActivity.class);

    private ActivityScenario<TestActivity> mScenario;

    private TestActivity mActivity;
    private CarUiSmoothScroller mCarUiSmoothScroller;

    @Before
    public void setUp() {
        mScenario = mActivityRule.getScenario();
        mScenario.onActivity(activity -> {
            mActivity = activity;
            mCarUiSmoothScroller = new CarUiSmoothScroller(mActivity);
        });
    }

    @Test
    public void calculateTimeForScrolling_shouldInitializeAllValues() {
        assertThat(mCarUiSmoothScroller.mMillisecondsPerInch).isNotEqualTo(0);
        assertThat(mCarUiSmoothScroller.mDecelerationTimeDivisor).isNotEqualTo(0);
        assertThat(mCarUiSmoothScroller.mMillisecondsPerPixel).isNotEqualTo(0);
        assertThat(mCarUiSmoothScroller.mInterpolator).isNotNull();
        assertThat(mCarUiSmoothScroller.mDensityDpi).isNotEqualTo(0);
    }

    @Test
    public void getVerticalSnapPreference_shouldReturnSnapToStart() {
        assertThat(mCarUiSmoothScroller.getVerticalSnapPreference()).isEqualTo(SNAP_TO_START);
    }

    @Test
    public void calculateTimeForScrolling_shouldReturnMultiplierOfMillisecondsPerPixel() {
        assertThat(mCarUiSmoothScroller.calculateTimeForScrolling(20)).isEqualTo(
                (int) Math.ceil(Math.abs(20) * mCarUiSmoothScroller.mMillisecondsPerPixel));
    }

    @Test
    public void calculateTimeForDeceleration_shouldReturnNotBeZero() {
        assertThat(mCarUiSmoothScroller.calculateTimeForDeceleration(20)).isNotEqualTo(0);
    }
}
