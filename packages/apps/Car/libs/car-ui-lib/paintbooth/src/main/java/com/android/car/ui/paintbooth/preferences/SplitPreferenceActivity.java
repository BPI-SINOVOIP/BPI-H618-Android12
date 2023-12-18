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

import android.os.Bundle;

import androidx.annotation.NonNull;
import androidx.appcompat.app.AppCompatActivity;

import com.android.car.ui.baselayout.Insets;
import com.android.car.ui.baselayout.InsetsChangedListener;
import com.android.car.ui.core.CarUi;
import com.android.car.ui.paintbooth.R;
import com.android.car.ui.toolbar.NavButtonMode;
import com.android.car.ui.toolbar.ToolbarController;

/**
 * A demo activity showing off preferences that only take up half the screen, and
 * have their own toolbar independent from the main toolbar.
 */
public class SplitPreferenceActivity extends AppCompatActivity implements InsetsChangedListener {

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.split_preference_activity);

        ToolbarController mainToolbar = CarUi.getToolbar(this);
        if (mainToolbar != null) {
            mainToolbar.setNavButtonMode(NavButtonMode.BACK);
            mainToolbar.setTitle("Split preferences sample");
        }

        // We do this so that the insets are not automatically sent to the fragments.
        // The fragments have their own insets handled by the installBaseLayoutAround().
        CarUi.replaceInsetsChangedListenerWith(this, this);

        SplitPreferenceDemoFragment fragment = new SplitPreferenceDemoFragment();
        ToolbarController subToolbar = CarUi.installBaseLayoutAround(
                requireViewById(R.id.preference_fragment_container),
                fragment, true);
        fragment.setToolbar(subToolbar);

        if (savedInstanceState == null) {
            getSupportFragmentManager()
                    .beginTransaction()
                    .replace(R.id.preference_fragment_container, fragment)
                    .commitNow();
        }
    }

    @Override
    public void onCarUiInsetsChanged(@NonNull Insets insets) {
        requireViewById(android.R.id.content)
                .setPadding(insets.getLeft(), insets.getTop(),
                        insets.getRight(), insets.getBottom());
    }
}
