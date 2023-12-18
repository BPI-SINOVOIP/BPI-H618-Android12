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

package com.android.car.ui.actions;

import android.view.View;
import android.widget.SeekBar;

import androidx.test.espresso.UiController;
import androidx.test.espresso.ViewAction;
import androidx.test.espresso.matcher.ViewMatchers;

import org.hamcrest.Matcher;

public class SetProgressViewAction implements ViewAction {

    private int mProgress;

    public SetProgressViewAction(int progress) {
        mProgress = progress;
    }

    @Override
    public void perform(UiController uiController, View view) {
        SeekBar seekBar = (SeekBar) view;
        seekBar.setProgress(mProgress);
    }

    @Override
    public String getDescription() {
        return "Set a progress on a SeekBar";
    }

    @Override
    public Matcher<View> getConstraints() {
        return ViewMatchers.isAssignableFrom(SeekBar.class);
    }
}
