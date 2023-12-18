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
package com.android.car.ui.paintbooth.preferences;

import androidx.annotation.NonNull;
import androidx.annotation.Nullable;
import androidx.fragment.app.Fragment;

import com.android.car.ui.baselayout.Insets;
import com.android.car.ui.paintbooth.R;
import com.android.car.ui.toolbar.ToolbarController;

/**
 * A PreferenceFragment that can use a custom toolbar via the {@link #setToolbar(ToolbarController)}
 * method.
 */
public class SplitPreferenceDemoFragment extends PreferenceDemoFragment {

    private ToolbarController mToolbar;
    private Insets mInsets;

    @Override
    public void setupToolbar(@NonNull ToolbarController toolbar) {
        toolbar.setLogo(R.drawable.ic_launcher);
        if (getPreferenceScreen() != null) {
            toolbar.setTitle(getPreferenceScreen().getTitle());
        }
    }

    @Override
    public ToolbarController getPreferenceToolbar(@NonNull Fragment fragment) {
        return mToolbar;
    }

    public void setToolbar(ToolbarController toolbar) {
        mToolbar = toolbar;
    }

    @Nullable
    @Override
    protected Insets getPreferenceInsets(@NonNull Fragment fragment) {
        return mInsets;
    }

    @Override
    public void onCarUiInsetsChanged(@NonNull Insets insets) {
        mInsets = insets;
        super.onCarUiInsetsChanged(insets);
    }
}
