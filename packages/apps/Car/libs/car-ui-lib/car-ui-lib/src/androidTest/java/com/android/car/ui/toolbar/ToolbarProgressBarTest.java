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
import static androidx.test.espresso.assertion.ViewAssertions.matches;
import static androidx.test.espresso.matcher.ViewMatchers.isAssignableFrom;
import static androidx.test.espresso.matcher.ViewMatchers.isDisplayed;

import static com.android.car.ui.matchers.ViewMatchers.hasIndeterminateProgress;

import static com.google.common.truth.Truth.assertThat;

import static org.hamcrest.Matchers.allOf;

import android.widget.ProgressBar;

import androidx.test.ext.junit.rules.ActivityScenarioRule;

import com.android.car.ui.core.CarUi;
import com.android.car.ui.sharedlibrarysupport.SharedLibraryFactorySingleton;

import org.junit.Rule;
import org.junit.Test;
import org.junit.runner.RunWith;
import org.junit.runners.Parameterized;

import java.util.function.Consumer;

@SuppressWarnings("AndroidJdkLibsChecker")
@RunWith(Parameterized.class)
public class ToolbarProgressBarTest {

    @Parameterized.Parameters
    public static Object[] data() {
        // It's important to do no shared library first, so that the shared library will
        // still be enabled when this test finishes
        return new Object[] { false, true };
    }

    private final boolean mSharedLibEnabled;

    public ToolbarProgressBarTest(boolean sharedLibEnabled) {
        mSharedLibEnabled = sharedLibEnabled;
        SharedLibraryFactorySingleton.setSharedLibEnabled(sharedLibEnabled);
    }

    @Rule
    public final ActivityScenarioRule<ToolbarTestActivity> mScenarioRule =
            new ActivityScenarioRule<>(ToolbarTestActivity.class);

    @Test
    public void test_showProgressBar_works() {
        runWithProgressBar((progressBar) -> progressBar.setVisible(true));

        // We don't want to enforce that the shared lib use a ProgressBar class
        if (!mSharedLibEnabled) {
            onView(isAssignableFrom(ProgressBar.class)).check(matches(
                    allOf(isDisplayed(), hasIndeterminateProgress())));
        }
    }

    @Test
    public void test_settersAndGetters_work() {
        boolean[] isVisible = new boolean[] { true };
        boolean[] isIndeterminate = new boolean[] { false };
        int[] min = new int[] { -5 };
        int[] max = new int[] { -5 };
        int[] progress = new int[] { -5 };

        runWithProgressBar((progressBar) -> {
            isVisible[0] = progressBar.isVisible();
            isIndeterminate[0] = progressBar.isIndeterminate();
            min[0] = progressBar.getMin();
            max[0] = progressBar.getMax();
            progress[0] = progressBar.getProgress();
        });

        assertThat(isVisible[0]).isFalse();
        assertThat(isIndeterminate[0]).isTrue();
        assertThat(min[0]).isEqualTo(0);
        assertThat(max[0]).isEqualTo(100);
        assertThat(progress[0]).isEqualTo(0);

        runWithProgressBar((progressBar) -> {
            progressBar.setVisible(true);
            progressBar.setIndeterminate(false);
            progressBar.setMin(50);
            progressBar.setMax(75);
            progressBar.setProgress(55);

            isVisible[0] = progressBar.isVisible();
            isIndeterminate[0] = progressBar.isIndeterminate();
            min[0] = progressBar.getMin();
            max[0] = progressBar.getMax();
            progress[0] = progressBar.getProgress();
        });

        assertThat(isVisible[0]).isTrue();
        assertThat(isIndeterminate[0]).isFalse();
        assertThat(min[0]).isEqualTo(50);
        assertThat(max[0]).isEqualTo(75);
        assertThat(progress[0]).isEqualTo(55);
    }

    private void runWithProgressBar(Consumer<ProgressBarController> toRun) {
        mScenarioRule.getScenario().onActivity(activity -> {
            ToolbarController toolbar = CarUi.requireToolbar(activity);
            toRun.accept(toolbar.getProgressBar());
        });
    }
}
