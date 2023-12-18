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

import static com.google.common.truth.Truth.assertThat;

import static org.mockito.Mockito.mock;
import static org.mockito.Mockito.verify;

import android.testing.AndroidTestingRunner;
import android.testing.TestableLooper;
import android.testing.TestableResources;
import android.view.View;
import android.view.ViewGroup;

import androidx.test.filters.SmallTest;

import com.android.systemui.R;
import com.android.systemui.SysuiTestCase;
import com.android.systemui.car.CarSystemUiTest;
import com.android.systemui.car.hvac.HvacController;
import com.android.systemui.car.statusbar.UserNameViewController;
import com.android.systemui.plugins.DarkIconDispatcher;
import com.android.systemui.statusbar.FeatureFlags;
import com.android.systemui.statusbar.phone.StatusBarIconController;

import org.junit.Before;
import org.junit.Test;
import org.junit.runner.RunWith;
import org.mockito.Mock;
import org.mockito.MockitoAnnotations;

@CarSystemUiTest
@RunWith(AndroidTestingRunner.class)
@TestableLooper.RunWithLooper
@SmallTest
public class CarSystemBarControllerTest extends SysuiTestCase {

    private static final String TOP_NOTIFICATION_PANEL =
            "com.android.systemui.car.notification.TopNotificationPanelViewMediator";
    private static final String BOTTOM_NOTIFICATION_PANEL =
            "com.android.systemui.car.notification.BottomNotificationPanelViewMediator";
    private CarSystemBarController mCarSystemBar;
    private CarSystemBarViewFactory mCarSystemBarViewFactory;
    private TestableResources mTestableResources;

    @Mock
    private ButtonSelectionStateController mButtonSelectionStateController;
    @Mock
    private ButtonRoleHolderController mButtonRoleHolderController;
    @Mock
    private HvacController mHvacController;
    @Mock
    private UserNameViewController mUserNameViewController;
    @Mock
    private PrivacyChipViewController mPrivacyChipViewController;
    @Mock
    private FeatureFlags mFeatureFlags;
    @Mock
    private StatusBarIconController mIconController;

    @Before
    public void setUp() throws Exception {
        MockitoAnnotations.initMocks(this);
        mCarSystemBarViewFactory = new CarSystemBarViewFactory(
                mContext, mFeatureFlags, mIconController);
        mTestableResources = mContext.getOrCreateTestableResources();

        // Needed to inflate top navigation bar.
        mDependency.injectMockDependency(DarkIconDispatcher.class);
        mDependency.injectMockDependency(StatusBarIconController.class);
    }

    private CarSystemBarController createSystemBarController() {
        return new CarSystemBarController(mContext, mCarSystemBarViewFactory,
                mButtonSelectionStateController, () -> mHvacController,
                () -> mUserNameViewController, () -> mPrivacyChipViewController,
                mButtonRoleHolderController,
                new SystemBarConfigs(mTestableResources.getResources()));
    }

    @Test
    public void testConnectToHvac_callsConnect() {
        mCarSystemBar = createSystemBarController();

        mCarSystemBar.connectToHvac();

        verify(mHvacController).connectToCarService();
    }

    @Test
    public void testRemoveAll_callsHvacControllerRemoveAllComponents() {
        mCarSystemBar = createSystemBarController();

        mCarSystemBar.removeAll();

        verify(mHvacController).removeAllComponents();
    }


    @Test
    public void testRemoveAll_callsButtonRoleHolderControllerRemoveAll() {
        mCarSystemBar = createSystemBarController();

        mCarSystemBar.removeAll();

        verify(mButtonRoleHolderController).removeAll();
    }

    @Test
    public void testRemoveAll_callsButtonSelectionStateControllerRemoveAll() {
        mCarSystemBar = createSystemBarController();

        mCarSystemBar.removeAll();

        verify(mButtonSelectionStateController).removeAll();
    }

    @Test
    public void testRemoveAll_callsPrivacyChipViewControllerRemoveAll() {
        mCarSystemBar = createSystemBarController();

        mCarSystemBar.removeAll();

        verify(mPrivacyChipViewController).removeAll();
    }

    @Test
    public void testGetTopWindow_topDisabled_returnsNull() {
        mTestableResources.addOverride(R.bool.config_enableTopSystemBar, false);
        mTestableResources.addOverride(R.bool.config_enableBottomSystemBar, true);
        // If Top Notification Panel is used but top navigation bar is not enabled, SystemUI is
        // expected to crash.
        mTestableResources.addOverride(R.string.config_notificationPanelViewMediator,
                BOTTOM_NOTIFICATION_PANEL);
        mCarSystemBar = createSystemBarController();

        ViewGroup window = mCarSystemBar.getTopWindow();

        assertThat(window).isNull();
    }

    @Test
    public void testGetTopWindow_topEnabled_returnsWindow() {
        mTestableResources.addOverride(R.bool.config_enableTopSystemBar, true);
        mCarSystemBar = createSystemBarController();

        ViewGroup window = mCarSystemBar.getTopWindow();

        assertThat(window).isNotNull();
    }

    @Test
    public void testGetTopWindow_topEnabled_calledTwice_returnsSameWindow() {
        mTestableResources.addOverride(R.bool.config_enableTopSystemBar, true);
        mCarSystemBar = createSystemBarController();

        ViewGroup window1 = mCarSystemBar.getTopWindow();
        ViewGroup window2 = mCarSystemBar.getTopWindow();

        assertThat(window1).isEqualTo(window2);
    }

    @Test
    public void testGetBottomWindow_bottomDisabled_returnsNull() {
        mTestableResources.addOverride(R.bool.config_enableBottomSystemBar, false);
        mTestableResources.addOverride(R.bool.config_enableTopSystemBar, true);
        // If Bottom Notification Panel is used but bottom navigation bar is not enabled,
        // SystemUI is expected to crash.
        mTestableResources.addOverride(R.string.config_notificationPanelViewMediator,
                TOP_NOTIFICATION_PANEL);
        mCarSystemBar = createSystemBarController();

        ViewGroup window = mCarSystemBar.getBottomWindow();

        assertThat(window).isNull();
    }

    @Test
    public void testGetBottomWindow_bottomEnabled_returnsWindow() {
        mTestableResources.addOverride(R.bool.config_enableBottomSystemBar, true);
        mCarSystemBar = createSystemBarController();

        ViewGroup window = mCarSystemBar.getBottomWindow();

        assertThat(window).isNotNull();
    }

    @Test
    public void testGetBottomWindow_bottomEnabled_calledTwice_returnsSameWindow() {
        mTestableResources.addOverride(R.bool.config_enableBottomSystemBar, true);
        mCarSystemBar = createSystemBarController();

        ViewGroup window1 = mCarSystemBar.getBottomWindow();
        ViewGroup window2 = mCarSystemBar.getBottomWindow();

        assertThat(window1).isEqualTo(window2);
    }

    @Test
    public void testGetLeftWindow_leftDisabled_returnsNull() {
        mTestableResources.addOverride(R.bool.config_enableLeftSystemBar, false);
        mCarSystemBar = createSystemBarController();
        ViewGroup window = mCarSystemBar.getLeftWindow();
        assertThat(window).isNull();
    }

    @Test
    public void testGetLeftWindow_leftEnabled_returnsWindow() {
        mTestableResources.addOverride(R.bool.config_enableLeftSystemBar, true);
        mCarSystemBar = createSystemBarController();

        ViewGroup window = mCarSystemBar.getLeftWindow();

        assertThat(window).isNotNull();
    }

    @Test
    public void testGetLeftWindow_leftEnabled_calledTwice_returnsSameWindow() {
        mTestableResources.addOverride(R.bool.config_enableLeftSystemBar, true);
        mCarSystemBar = createSystemBarController();

        ViewGroup window1 = mCarSystemBar.getLeftWindow();
        ViewGroup window2 = mCarSystemBar.getLeftWindow();

        assertThat(window1).isEqualTo(window2);
    }

    @Test
    public void testGetRightWindow_rightDisabled_returnsNull() {
        mTestableResources.addOverride(R.bool.config_enableRightSystemBar, false);
        mCarSystemBar = createSystemBarController();

        ViewGroup window = mCarSystemBar.getRightWindow();

        assertThat(window).isNull();
    }

    @Test
    public void testGetRightWindow_rightEnabled_returnsWindow() {
        mTestableResources.addOverride(R.bool.config_enableRightSystemBar, true);
        mCarSystemBar = createSystemBarController();

        ViewGroup window = mCarSystemBar.getRightWindow();

        assertThat(window).isNotNull();
    }

    @Test
    public void testGetRightWindow_rightEnabled_calledTwice_returnsSameWindow() {
        mTestableResources.addOverride(R.bool.config_enableRightSystemBar, true);
        mCarSystemBar = createSystemBarController();

        ViewGroup window1 = mCarSystemBar.getRightWindow();
        ViewGroup window2 = mCarSystemBar.getRightWindow();

        assertThat(window1).isEqualTo(window2);
    }

    @Test
    public void testSetTopWindowVisibility_setTrue_isVisible() {
        mTestableResources.addOverride(R.bool.config_enableTopSystemBar, true);
        mCarSystemBar = createSystemBarController();

        ViewGroup window = mCarSystemBar.getTopWindow();
        mCarSystemBar.setTopWindowVisibility(View.VISIBLE);

        assertThat(window.getVisibility()).isEqualTo(View.VISIBLE);
    }

    @Test
    public void testSetTopWindowVisibility_setFalse_isGone() {
        mTestableResources.addOverride(R.bool.config_enableTopSystemBar, true);
        mCarSystemBar = createSystemBarController();

        ViewGroup window = mCarSystemBar.getTopWindow();
        mCarSystemBar.setTopWindowVisibility(View.GONE);

        assertThat(window.getVisibility()).isEqualTo(View.GONE);
    }

    @Test
    public void testSetBottomWindowVisibility_setTrue_isVisible() {
        mTestableResources.addOverride(R.bool.config_enableBottomSystemBar, true);
        mCarSystemBar = createSystemBarController();

        ViewGroup window = mCarSystemBar.getBottomWindow();
        mCarSystemBar.setBottomWindowVisibility(View.VISIBLE);

        assertThat(window.getVisibility()).isEqualTo(View.VISIBLE);
    }

    @Test
    public void testSetBottomWindowVisibility_setFalse_isGone() {
        mTestableResources.addOverride(R.bool.config_enableBottomSystemBar, true);
        mCarSystemBar = createSystemBarController();

        ViewGroup window = mCarSystemBar.getBottomWindow();
        mCarSystemBar.setBottomWindowVisibility(View.GONE);

        assertThat(window.getVisibility()).isEqualTo(View.GONE);
    }

    @Test
    public void testSetLeftWindowVisibility_setTrue_isVisible() {
        mTestableResources.addOverride(R.bool.config_enableLeftSystemBar, true);
        mCarSystemBar = createSystemBarController();

        ViewGroup window = mCarSystemBar.getLeftWindow();
        mCarSystemBar.setLeftWindowVisibility(View.VISIBLE);

        assertThat(window.getVisibility()).isEqualTo(View.VISIBLE);
    }

    @Test
    public void testSetLeftWindowVisibility_setFalse_isGone() {
        mTestableResources.addOverride(R.bool.config_enableLeftSystemBar, true);
        mCarSystemBar = createSystemBarController();

        ViewGroup window = mCarSystemBar.getLeftWindow();
        mCarSystemBar.setLeftWindowVisibility(View.GONE);

        assertThat(window.getVisibility()).isEqualTo(View.GONE);
    }

    @Test
    public void testSetRightWindowVisibility_setTrue_isVisible() {
        mTestableResources.addOverride(R.bool.config_enableRightSystemBar, true);
        mCarSystemBar = createSystemBarController();

        ViewGroup window = mCarSystemBar.getRightWindow();
        mCarSystemBar.setRightWindowVisibility(View.VISIBLE);

        assertThat(window.getVisibility()).isEqualTo(View.VISIBLE);
    }

    @Test
    public void testSetRightWindowVisibility_setFalse_isGone() {
        mTestableResources.addOverride(R.bool.config_enableRightSystemBar, true);
        mCarSystemBar = createSystemBarController();

        ViewGroup window = mCarSystemBar.getRightWindow();
        mCarSystemBar.setRightWindowVisibility(View.GONE);

        assertThat(window.getVisibility()).isEqualTo(View.GONE);
    }

    @Test
    public void testRegisterBottomBarTouchListener_createViewFirst_registrationSuccessful() {
        mTestableResources.addOverride(R.bool.config_enableBottomSystemBar, true);
        mCarSystemBar = createSystemBarController();

        CarSystemBarView bottomBar = mCarSystemBar.getBottomBar(/* isSetUp= */ true);
        View.OnTouchListener controller = bottomBar.getStatusBarWindowTouchListener();
        assertThat(controller).isNull();
        mCarSystemBar.registerBottomBarTouchListener(mock(View.OnTouchListener.class));
        controller = bottomBar.getStatusBarWindowTouchListener();

        assertThat(controller).isNotNull();
    }

    @Test
    public void testRegisterBottomBarTouchListener_registerFirst_registrationSuccessful() {
        mTestableResources.addOverride(R.bool.config_enableBottomSystemBar, true);
        mCarSystemBar = createSystemBarController();

        mCarSystemBar.registerBottomBarTouchListener(mock(View.OnTouchListener.class));
        CarSystemBarView bottomBar = mCarSystemBar.getBottomBar(/* isSetUp= */ true);
        View.OnTouchListener controller = bottomBar.getStatusBarWindowTouchListener();

        assertThat(controller).isNotNull();
    }

    @Test
    public void testRegisterNotificationController_createViewFirst_registrationSuccessful() {
        mTestableResources.addOverride(R.bool.config_enableBottomSystemBar, true);
        mCarSystemBar = createSystemBarController();

        CarSystemBarView bottomBar = mCarSystemBar.getBottomBar(/* isSetUp= */ true);
        CarSystemBarController.NotificationsShadeController controller =
                bottomBar.getNotificationsPanelController();
        assertThat(controller).isNull();
        mCarSystemBar.registerNotificationController(
                mock(CarSystemBarController.NotificationsShadeController.class));
        controller = bottomBar.getNotificationsPanelController();

        assertThat(controller).isNotNull();
    }

    @Test
    public void testRegisterNotificationController_registerFirst_registrationSuccessful() {
        mTestableResources.addOverride(R.bool.config_enableBottomSystemBar, true);
        mCarSystemBar = createSystemBarController();

        mCarSystemBar.registerNotificationController(
                mock(CarSystemBarController.NotificationsShadeController.class));
        CarSystemBarView bottomBar = mCarSystemBar.getBottomBar(/* isSetUp= */ true);
        CarSystemBarController.NotificationsShadeController controller =
                bottomBar.getNotificationsPanelController();

        assertThat(controller).isNotNull();
    }

    @Test
    public void testShowAllNavigationButtons_bottomEnabled_bottomNavigationButtonsVisible() {
        mTestableResources.addOverride(R.bool.config_enableBottomSystemBar, true);
        mCarSystemBar = createSystemBarController();
        CarSystemBarView bottomBar = mCarSystemBar.getBottomBar(/* isSetUp= */ true);
        View bottomNavButtons = bottomBar.findViewById(R.id.nav_buttons);

        mCarSystemBar.showAllNavigationButtons(/* isSetUp= */ true);

        assertThat(bottomNavButtons.getVisibility()).isEqualTo(View.VISIBLE);
    }

    @Test
    public void testShowAllNavigationButtons_bottomEnabled_bottomKeyguardButtonsGone() {
        mTestableResources.addOverride(R.bool.config_enableBottomSystemBar, true);
        mCarSystemBar = createSystemBarController();
        CarSystemBarView bottomBar = mCarSystemBar.getBottomBar(/* isSetUp= */ true);
        View bottomKeyguardButtons = bottomBar.findViewById(R.id.lock_screen_nav_buttons);

        mCarSystemBar.showAllNavigationButtons(/* isSetUp= */ true);

        assertThat(bottomKeyguardButtons.getVisibility()).isEqualTo(View.GONE);
    }

    @Test
    public void testShowAllNavigationButtons_bottomEnabled_bottomOcclusionButtonsGone() {
        mTestableResources.addOverride(R.bool.config_enableBottomSystemBar, true);
        mCarSystemBar = createSystemBarController();
        CarSystemBarView bottomBar = mCarSystemBar.getBottomBar(/* isSetUp= */ true);
        View occlusionButtons = bottomBar.findViewById(R.id.occlusion_buttons);

        mCarSystemBar.showAllNavigationButtons(/* isSetUp= */ true);

        assertThat(occlusionButtons.getVisibility()).isEqualTo(View.GONE);
    }

    @Test
    public void testShowAllKeyguardButtons_bottomEnabled_bottomKeyguardButtonsVisible() {
        mTestableResources.addOverride(R.bool.config_enableBottomSystemBar, true);
        mCarSystemBar = createSystemBarController();
        CarSystemBarView bottomBar = mCarSystemBar.getBottomBar(/* isSetUp= */ true);
        View bottomKeyguardButtons = bottomBar.findViewById(R.id.lock_screen_nav_buttons);

        mCarSystemBar.showAllKeyguardButtons(/* isSetUp= */ true);

        assertThat(bottomKeyguardButtons.getVisibility()).isEqualTo(View.VISIBLE);
    }

    @Test
    public void testShowAllKeyguardButtons_bottomEnabled_bottomNavigationButtonsGone() {
        mTestableResources.addOverride(R.bool.config_enableBottomSystemBar, true);
        mCarSystemBar = createSystemBarController();
        CarSystemBarView bottomBar = mCarSystemBar.getBottomBar(/* isSetUp= */ true);
        View bottomNavButtons = bottomBar.findViewById(R.id.nav_buttons);

        mCarSystemBar.showAllKeyguardButtons(/* isSetUp= */ true);

        assertThat(bottomNavButtons.getVisibility()).isEqualTo(View.GONE);
    }

    @Test
    public void testShowAllKeyguardButtons_bottomEnabled_bottomOcclusionButtonsGone() {
        mTestableResources.addOverride(R.bool.config_enableBottomSystemBar, true);
        mCarSystemBar = createSystemBarController();
        CarSystemBarView bottomBar = mCarSystemBar.getBottomBar(/* isSetUp= */ true);
        View occlusionButtons = bottomBar.findViewById(R.id.occlusion_buttons);

        mCarSystemBar.showAllKeyguardButtons(/* isSetUp= */ true);

        assertThat(occlusionButtons.getVisibility()).isEqualTo(View.GONE);
    }

    @Test
    public void testShowOcclusionButtons_bottomEnabled_bottomOcclusionButtonsVisible() {
        mTestableResources.addOverride(R.bool.config_enableBottomSystemBar, true);
        mCarSystemBar = createSystemBarController();
        CarSystemBarView bottomBar = mCarSystemBar.getBottomBar(/* isSetUp= */ true);
        View occlusionButtons = bottomBar.findViewById(R.id.occlusion_buttons);

        mCarSystemBar.showAllOcclusionButtons(/* isSetUp= */ true);

        assertThat(occlusionButtons.getVisibility()).isEqualTo(View.VISIBLE);
    }

    @Test
    public void testShowOcclusionButtons_bottomEnabled_bottomNavigationButtonsGone() {
        mTestableResources.addOverride(R.bool.config_enableBottomSystemBar, true);
        mCarSystemBar = createSystemBarController();
        CarSystemBarView bottomBar = mCarSystemBar.getBottomBar(/* isSetUp= */ true);
        View bottomNavButtons = bottomBar.findViewById(R.id.nav_buttons);

        mCarSystemBar.showAllOcclusionButtons(/* isSetUp= */ true);

        assertThat(bottomNavButtons.getVisibility()).isEqualTo(View.GONE);
    }

    @Test
    public void testShowOcclusionButtons_bottomEnabled_bottomKeyguardButtonsGone() {
        mTestableResources.addOverride(R.bool.config_enableBottomSystemBar, true);
        mCarSystemBar = createSystemBarController();
        CarSystemBarView bottomBar = mCarSystemBar.getBottomBar(/* isSetUp= */ true);
        View keyguardButtons = bottomBar.findViewById(R.id.lock_screen_nav_buttons);

        mCarSystemBar.showAllOcclusionButtons(/* isSetUp= */ true);

        assertThat(keyguardButtons.getVisibility()).isEqualTo(View.GONE);
    }

    @Test
    public void testToggleAllNotificationsUnseenIndicator_bottomEnabled_hasUnseen_setCorrectly() {
        mTestableResources.addOverride(R.bool.config_enableBottomSystemBar, true);
        mCarSystemBar = createSystemBarController();
        CarSystemBarView bottomBar = mCarSystemBar.getBottomBar(/* isSetUp= */ true);
        CarSystemBarButton notifications = bottomBar.findViewById(R.id.notifications);

        boolean hasUnseen = true;
        mCarSystemBar.toggleAllNotificationsUnseenIndicator(/* isSetUp= */ true,
                hasUnseen);

        assertThat(notifications.getUnseen()).isTrue();
    }

    @Test
    public void testToggleAllNotificationsUnseenIndicator_bottomEnabled_noUnseen_setCorrectly() {
        mTestableResources.addOverride(R.bool.config_enableBottomSystemBar, true);
        mCarSystemBar = createSystemBarController();
        CarSystemBarView bottomBar = mCarSystemBar.getBottomBar(/* isSetUp= */ true);
        CarSystemBarButton notifications = bottomBar.findViewById(R.id.notifications);

        boolean hasUnseen = false;
        mCarSystemBar.toggleAllNotificationsUnseenIndicator(/* isSetUp= */ true,
                hasUnseen);

        assertThat(notifications.getUnseen()).isFalse();
    }
}
