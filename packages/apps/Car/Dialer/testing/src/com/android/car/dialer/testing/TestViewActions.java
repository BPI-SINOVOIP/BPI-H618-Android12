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

package com.android.car.dialer.testing;

import static org.hamcrest.Matchers.allOf;

import android.view.View;

import androidx.test.espresso.UiController;
import androidx.test.espresso.ViewAction;
import androidx.test.espresso.matcher.ViewMatchers;

import org.hamcrest.Matcher;

/** Custom view actions as a workaround for clicking overlapping UI elements. */
public final class TestViewActions {

    /** A click action by calling the {@link View#performClick()} api. */
    public static ViewAction selfClick() {
        return new ViewAction() {

            @Override
            public Matcher<View> getConstraints() {
                return allOf(ViewMatchers.isDisplayingAtLeast(90), ViewMatchers.isClickable());
            }

            @Override
            public String getDescription() {
                return "Calling View#performClick() api";
            }

            @Override
            public void perform(UiController uiController, View view) {
                view.performClick();
            }
        };
    }
}
