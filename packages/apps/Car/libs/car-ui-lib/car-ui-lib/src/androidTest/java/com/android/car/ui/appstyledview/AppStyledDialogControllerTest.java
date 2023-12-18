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

package com.android.car.ui.appstyledview;

import static androidx.test.espresso.Espresso.onView;
import static androidx.test.espresso.action.ViewActions.click;
import static androidx.test.espresso.assertion.ViewAssertions.matches;
import static androidx.test.espresso.matcher.ViewMatchers.isDisplayed;
import static androidx.test.espresso.matcher.ViewMatchers.withId;
import static androidx.test.espresso.matcher.ViewMatchers.withText;

import static org.mockito.Mockito.mock;
import static org.mockito.Mockito.verify;

import android.app.Dialog;
import android.view.LayoutInflater;
import android.view.View;

import androidx.test.espresso.Root;
import androidx.test.ext.junit.runners.AndroidJUnit4;
import androidx.test.rule.ActivityTestRule;

import com.android.car.ui.TestActivity;
import com.android.car.ui.appstyledview.AppStyledViewController.AppStyledDismissListener;
import com.android.car.ui.appstyledview.AppStyledViewController.AppStyledVCloseClickListener;
import com.android.car.ui.appstyledview.AppStyledViewController.AppStyledViewNavIcon;
import com.android.car.ui.test.R;

import org.hamcrest.Description;
import org.hamcrest.TypeSafeMatcher;
import org.junit.Before;
import org.junit.Rule;
import org.junit.Test;
import org.junit.runner.RunWith;

/**
 * Unit tests for {@link AppStyledDialogController}.
 */
@RunWith(AndroidJUnit4.class)
public class AppStyledDialogControllerTest {

    private AppStyledDialogController mAppStyledDialogController;

    @Rule
    public ActivityTestRule<TestActivity> mActivityRule =
            new ActivityTestRule<>(TestActivity.class);

    @Before
    public void setUp() throws Throwable {

        mActivityRule.runOnUiThread(() -> {
            mAppStyledDialogController = new AppStyledDialogController(mActivityRule.getActivity());
            mAppStyledDialogController
                    .setAppStyledViewController(
                            new AppStyledViewControllerImpl(mActivityRule.getActivity()),
                            mActivityRule.getActivity());
        });
    }

    @Test
    public void show_shouldDisplayDialog() throws Throwable {
        LayoutInflater inflator = LayoutInflater.from(mActivityRule.getActivity());

        View appStyledTestView = inflator.inflate(R.layout.app_styled_view_sample, null,
                false);

        mActivityRule.runOnUiThread(() -> {
            mAppStyledDialogController.setContentView(appStyledTestView);
            mAppStyledDialogController.show();
        });

        String text = "app styled view";
        Dialog dialog = mAppStyledDialogController.getAppStyledDialog();

        onView(withText(text))
                .inRoot(new RootWithDecorMatcher(dialog.getWindow().getDecorView()))
                .check(matches(isDisplayed()));
    }

    @Test
    public void setNavIcon_showCloseIcon() throws Throwable {
        LayoutInflater inflator = LayoutInflater.from(mActivityRule.getActivity());

        View appStyledTestView = inflator.inflate(R.layout.app_styled_view_sample, null,
                false);

        mActivityRule.runOnUiThread(() -> {
            mAppStyledDialogController.setContentView(appStyledTestView);
            mAppStyledDialogController.setNavIcon(AppStyledViewNavIcon.CLOSE);
            mAppStyledDialogController.show();
        });

        Dialog dialog = mAppStyledDialogController.getAppStyledDialog();

        onView(withId(R.id.car_ui_app_styled_view_icon_close))
                .inRoot(new RootWithDecorMatcher(dialog.getWindow().getDecorView()))
                .check(matches(isDisplayed()));
    }

    @Test
    public void setOnCloseClickListener_shouldInvokeCallback() throws Throwable {
        LayoutInflater inflator = LayoutInflater.from(mActivityRule.getActivity());

        View appStyledTestView = inflator.inflate(R.layout.app_styled_view_sample, null,
                false);

        AppStyledVCloseClickListener callback = mock(AppStyledVCloseClickListener.class);

        mActivityRule.runOnUiThread(() -> {
            mAppStyledDialogController.setContentView(appStyledTestView);
            mAppStyledDialogController.setNavIcon(AppStyledViewNavIcon.BACK);
            mAppStyledDialogController.setOnCloseClickListener(callback);
            mAppStyledDialogController.show();
        });

        Dialog dialog = mAppStyledDialogController.getAppStyledDialog();

        onView(withId(R.id.car_ui_app_styled_view_icon_close))
                .inRoot(new RootWithDecorMatcher(dialog.getWindow().getDecorView()))
                .perform(click());

        verify(callback).onClick();
    }

    @Test
    public void setOnDismissListener_shouldInvokeCallback() throws Throwable {
        LayoutInflater inflator = LayoutInflater.from(mActivityRule.getActivity());

        View appStyledTestView = inflator.inflate(R.layout.app_styled_view_sample, null,
                false);

        AppStyledDismissListener callback = mock(AppStyledDismissListener.class);

        mActivityRule.runOnUiThread(() -> {
            mAppStyledDialogController.setContentView(appStyledTestView);
            mAppStyledDialogController.setNavIcon(AppStyledViewNavIcon.BACK);
            mAppStyledDialogController.setOnDismissListener(callback);
            mAppStyledDialogController.show();
        });

        Dialog dialog = mAppStyledDialogController.getAppStyledDialog();

        onView(withId(R.id.car_ui_app_styled_view_icon_close))
                .inRoot(new RootWithDecorMatcher(dialog.getWindow().getDecorView()))
                .perform(click());

        verify(callback).onDismiss();
    }

    private static class RootWithDecorMatcher extends TypeSafeMatcher<Root> {

        private View mView;

        RootWithDecorMatcher(View view) {
            mView = view;
        }

        @Override
        public void describeTo(Description description) {
            description.appendText("is a root with a certain decor");
        }

        @Override
        protected boolean matchesSafely(Root item) {
            return item.getDecorView() == mView;
        }
    }
}
