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

package com.android.systemui.car.systembar;

import static android.app.WindowConfiguration.WINDOWING_MODE_FULLSCREEN;

import static com.google.common.truth.Truth.assertThat;

import static org.mockito.ArgumentMatchers.any;
import static org.mockito.ArgumentMatchers.anyInt;
import static org.mockito.ArgumentMatchers.argThat;
import static org.mockito.Mockito.never;
import static org.mockito.Mockito.spy;
import static org.mockito.Mockito.verify;
import static org.mockito.Mockito.when;

import android.app.ActivityManager;
import android.content.Intent;
import android.graphics.drawable.Drawable;
import android.testing.AndroidTestingRunner;
import android.testing.TestableLooper;
import android.view.LayoutInflater;
import android.view.View;
import android.widget.ImageView;
import android.widget.LinearLayout;

import androidx.test.filters.SmallTest;

import com.android.systemui.SysuiTestCase;
import com.android.systemui.car.CarSystemUiTest;
import com.android.systemui.statusbar.AlphaOptimizedImageView;
import com.android.systemui.tests.R;

import org.junit.Before;
import org.junit.Test;
import org.junit.runner.RunWith;
import org.mockito.ArgumentMatcher;

import java.util.List;

@CarSystemUiTest
@RunWith(AndroidTestingRunner.class)
@TestableLooper.RunWithLooper
@SmallTest
public class CarSystemBarButtonTest extends SysuiTestCase {

    private static final String DEFAULT_BUTTON_ACTIVITY_NAME =
            "com.android.car.carlauncher/.CarLauncher";
    private static final String DIALER_BUTTON_ACTIVITY_NAME =
            "com.android.car.dialer/.ui.TelecomActivity";
    private static final String BROADCAST_ACTION_NAME =
            "android.car.intent.action.TOGGLE_HVAC_CONTROLS";

    private ActivityManager mActivityManager;
    // LinearLayout with CarSystemBarButtons with different configurations.
    private LinearLayout mTestView;
    // Does not have any selection state which is the default configuration.
    private CarSystemBarButton mDefaultButton;

    @Before
    public void setUp() {
        mContext = spy(mContext);
        ActivityManager am = mContext.getSystemService(ActivityManager.class);
        mActivityManager = spy(am);
        when(mContext.getSystemService(ActivityManager.class)).thenReturn(mActivityManager);
        mTestView = (LinearLayout) LayoutInflater.from(mContext).inflate(
                R.layout.car_system_bar_button_test, /* root= */ null);
        mDefaultButton = mTestView.findViewById(R.id.default_no_selection_state);
    }

    @Test
    public void onCreate_iconIsVisible() {
        AlphaOptimizedImageView icon = mDefaultButton.findViewById(
                R.id.car_nav_button_icon_image);

        assertThat(icon.getDrawable()).isNotNull();
    }

    @Test
    public void onSelected_selectedIconDefined_togglesIcon() {
        mDefaultButton.setSelected(true);
        Drawable selectedIconDrawable = ((AlphaOptimizedImageView) mDefaultButton.findViewById(
                R.id.car_nav_button_icon_image)).getDrawable();


        mDefaultButton.setSelected(false);
        Drawable unselectedIconDrawable = ((AlphaOptimizedImageView) mDefaultButton.findViewById(
                R.id.car_nav_button_icon_image)).getDrawable();

        assertThat(selectedIconDrawable).isNotEqualTo(unselectedIconDrawable);
    }

    @Test
    public void onSelected_selectedIconUndefined_displaysSameIcon() {
        CarSystemBarButton selectedIconUndefinedButton = mTestView.findViewById(
                R.id.selected_icon_undefined);

        selectedIconUndefinedButton.setSelected(true);
        Drawable selectedIconDrawable = ((AlphaOptimizedImageView) mDefaultButton.findViewById(
                R.id.car_nav_button_icon_image)).getDrawable();


        selectedIconUndefinedButton.setSelected(false);
        Drawable unselectedIconDrawable = ((AlphaOptimizedImageView) mDefaultButton.findViewById(
                R.id.car_nav_button_icon_image)).getDrawable();

        assertThat(selectedIconDrawable).isEqualTo(unselectedIconDrawable);
    }

    @Test
    public void onUnselected_doesNotHighlightWhenSelected_applySelectedAlpha() {
        mDefaultButton.setSelected(false);

        assertThat(mDefaultButton.getAlpha()).isEqualTo(mDefaultButton.getSelectedAlpha());
    }

    @Test
    public void onSelected_doesNotHighlightWhenSelected_applySelectedAlpha() {
        mDefaultButton.setSelected(true);

        assertThat(mDefaultButton.getIconAlpha()).isEqualTo(mDefaultButton.getSelectedAlpha());
    }

    @Test
    public void onUnselected_highlightWhenSelected_applyDefaultUnselectedAlpha() {
        CarSystemBarButton highlightWhenSelectedButton = mTestView.findViewById(
                R.id.highlightable_no_more_button);
        highlightWhenSelectedButton.setSelected(false);

        assertThat(highlightWhenSelectedButton.getIconAlpha()).isEqualTo(
                highlightWhenSelectedButton.getUnselectedAlpha());
    }

    @Test
    public void onSelected_highlightWhenSelected_applyDefaultSelectedAlpha() {
        CarSystemBarButton highlightWhenSelectedButton = mTestView.findViewById(
                R.id.highlightable_no_more_button);
        highlightWhenSelectedButton.setSelected(true);

        assertThat(highlightWhenSelectedButton.getIconAlpha()).isEqualTo(
                highlightWhenSelectedButton.getSelectedAlpha());
    }

    @Test
    public void onSelected_doesNotShowMoreWhenSelected_doesNotShowMoreIcon() {
        mDefaultButton.setSelected(true);
        AlphaOptimizedImageView moreIcon = mDefaultButton.findViewById(
                R.id.car_nav_button_more_icon);

        assertThat(moreIcon.getVisibility()).isEqualTo(View.GONE);
    }

    @Test
    public void onSelected_showMoreWhenSelected_showsMoreIcon() {
        CarSystemBarButton showMoreWhenSelected = mTestView.findViewById(
                R.id.not_highlightable_more_button);
        showMoreWhenSelected.setSelected(true);
        AlphaOptimizedImageView moreIcon = showMoreWhenSelected.findViewById(
                R.id.car_nav_button_more_icon);

        assertThat(moreIcon.getVisibility()).isEqualTo(View.VISIBLE);
    }

    @Test
    public void onUnselected_showMoreWhenSelected_doesNotShowMoreIcon() {
        CarSystemBarButton showMoreWhenSelected = mTestView.findViewById(
                R.id.highlightable_no_more_button);
        showMoreWhenSelected.setSelected(true);
        showMoreWhenSelected.setSelected(false);
        AlphaOptimizedImageView moreIcon = showMoreWhenSelected.findViewById(
                R.id.car_nav_button_more_icon);

        assertThat(moreIcon.getVisibility()).isEqualTo(View.GONE);
    }

    @Test
    public void onUnselected_withAppIcon_showsAppIcon() {
        CarSystemBarButton roleBasedButton = mTestView.findViewById(R.id.role_based_button);
        Drawable appIcon = getContext().getDrawable(R.drawable.ic_android);

        roleBasedButton.setSelected(false);
        roleBasedButton.setAppIcon(appIcon);

        Drawable currentDrawable = ((AlphaOptimizedImageView) roleBasedButton.findViewById(
                R.id.car_nav_button_icon_image)).getDrawable();

        assertThat(currentDrawable).isEqualTo(appIcon);
    }

    @Test
    public void onUnselected_withAppIcon_applyUnselectedAlpha() {
        CarSystemBarButton roleBasedButton = mTestView.findViewById(R.id.role_based_button);

        roleBasedButton.setSelected(false);
        roleBasedButton.setAppIcon(getContext().getDrawable(R.drawable.ic_android));

        assertThat(roleBasedButton.getIconAlpha()).isEqualTo(roleBasedButton.getUnselectedAlpha());
    }

    @Test
    public void onSelected_withAppIcon_showsAppIconWithSelectedAlpha() {
        CarSystemBarButton roleBasedButton = mTestView.findViewById(R.id.role_based_button);
        Drawable appIcon = getContext().getDrawable(R.drawable.ic_android);

        roleBasedButton.setSelected(true);
        roleBasedButton.setAppIcon(appIcon);

        Drawable currentDrawable = ((AlphaOptimizedImageView) roleBasedButton.findViewById(
                R.id.car_nav_button_icon_image)).getDrawable();

        assertThat(currentDrawable).isEqualTo(appIcon);
    }

    @Test
    public void onSelected_withAppIcon_applySelectedAlpha() {
        CarSystemBarButton roleBasedButton = mTestView.findViewById(R.id.role_based_button);

        roleBasedButton.setSelected(true);
        roleBasedButton.setAppIcon(getContext().getDrawable(R.drawable.ic_android));

        assertThat(roleBasedButton.getIconAlpha()).isEqualTo(roleBasedButton.getSelectedAlpha());
    }

    @Test
    public void onClick_launchesIntentActivity() {
        mDefaultButton.performClick();

        assertThat(getCurrentActivityName()).isEqualTo(DEFAULT_BUTTON_ACTIVITY_NAME);

        CarSystemBarButton dialerButton = mTestView.findViewById(R.id.dialer_activity);
        dialerButton.performClick();

        assertThat(getCurrentActivityName()).isEqualTo(DIALER_BUTTON_ACTIVITY_NAME);
    }

    @Test
    public void onLongClick_longIntentDefined_launchesLongIntentActivity() {
        mDefaultButton.performClick();

        assertThat(getCurrentActivityName()).isEqualTo(DEFAULT_BUTTON_ACTIVITY_NAME);

        CarSystemBarButton dialerButton = mTestView.findViewById(
                R.id.long_click_dialer_activity);
        dialerButton.performLongClick();

        assertThat(getCurrentActivityName()).isEqualTo(DIALER_BUTTON_ACTIVITY_NAME);
    }

    @Test
    public void onClick_useBroadcast_broadcastsIntent() {
        CarSystemBarButton button = mTestView.findViewById(R.id.broadcast);
        button.performClick();

        verify(mContext).sendBroadcastAsUser(argThat(new ArgumentMatcher<Intent>() {
            @Override
            public boolean matches(Intent argument) {
                return argument.getAction().equals(BROADCAST_ACTION_NAME);
            }
        }), any());
    }

    @Test
    public void onClick_requestBackstackClear_clearBackStack() {
        CarSystemBarButton dialerButton =
                mTestView.findViewById(R.id.dialer_activity_clear_backstack);

        dialerButton.performClick();

        verify(mActivityManager).moveTaskToFront(anyInt(), anyInt());
    }

    @Test
    public void onClick_useBroadcast_requestBackstackClear_doesNotClearingBackstack() {
        CarSystemBarButton button =
                mTestView.findViewById(R.id.broadcast_try_clear_backstack);

        button.performClick();

        verify(mActivityManager, never()).moveTaskToFront(anyInt(), anyInt());
    }

    @Test
    public void onSetUnseen_hasUnseen_showsUnseenIndicator() {
        mDefaultButton.setUnseen(true);
        ImageView hasUnseenIndicator = mDefaultButton.findViewById(R.id.car_nav_button_unseen_icon);

        assertThat(hasUnseenIndicator.getVisibility()).isEqualTo(View.VISIBLE);
    }

    @Test
    public void onSetUnseen_doesNotHaveUnseen_hidesUnseenIndicator() {
        mDefaultButton.setUnseen(false);
        ImageView hasUnseenIndicator = mDefaultButton.findViewById(R.id.car_nav_button_unseen_icon);

        assertThat(hasUnseenIndicator.getVisibility()).isEqualTo(View.GONE);
    }

    /**
     * Returns the name of the first activity running in fullscreen mode (i.e. excludes things
     * like TaskViews).
     */
    private String getCurrentActivityName() {
        // Check 3 running tasks to be safe in making sure there is at least 1 task running in
        // fullscreen mode.
        List<ActivityManager.RunningTaskInfo> infos = mActivityManager.getRunningTasks(3);
        for (ActivityManager.RunningTaskInfo info : infos) {
            if (info.getWindowingMode() == WINDOWING_MODE_FULLSCREEN) {
                return info.topActivity.flattenToShortString();
            }
        }
        return null;
    }
}
