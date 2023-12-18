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

import android.content.Context;
import android.view.View;
import android.view.ViewGroup;

import androidx.annotation.Nullable;

import com.android.systemui.car.hvac.HvacController;
import com.android.systemui.car.statusbar.UserNameViewController;
import com.android.systemui.dagger.SysUISingleton;

import javax.inject.Inject;

import dagger.Lazy;

/** A single class which controls the navigation bar views. */
@SysUISingleton
public class CarSystemBarController {

    private final Context mContext;
    private final CarSystemBarViewFactory mCarSystemBarViewFactory;
    private final ButtonSelectionStateController mButtonSelectionStateController;
    private final ButtonRoleHolderController mButtonRoleHolderController;
    private final Lazy<HvacController> mHvacControllerLazy;
    private final Lazy<UserNameViewController> mUserNameViewControllerLazy;
    private final Lazy<PrivacyChipViewController> mPrivacyChipViewControllerLazy;

    private boolean mShowTop;
    private boolean mShowBottom;
    private boolean mShowLeft;
    private boolean mShowRight;

    private View.OnTouchListener mTopBarTouchListener;
    private View.OnTouchListener mBottomBarTouchListener;
    private View.OnTouchListener mLeftBarTouchListener;
    private View.OnTouchListener mRightBarTouchListener;
    private NotificationsShadeController mNotificationsShadeController;

    private CarSystemBarView mTopView;
    private CarSystemBarView mBottomView;
    private CarSystemBarView mLeftView;
    private CarSystemBarView mRightView;

    @Inject
    public CarSystemBarController(Context context,
            CarSystemBarViewFactory carSystemBarViewFactory,
            ButtonSelectionStateController buttonSelectionStateController,
            Lazy<HvacController> hvacControllerLazy,
            Lazy<UserNameViewController> userNameViewControllerLazy,
            Lazy<PrivacyChipViewController> privacyChipViewControllerLazy,
            ButtonRoleHolderController buttonRoleHolderController,
            SystemBarConfigs systemBarConfigs) {
        mContext = context;
        mCarSystemBarViewFactory = carSystemBarViewFactory;
        mButtonSelectionStateController = buttonSelectionStateController;
        mHvacControllerLazy = hvacControllerLazy;
        mUserNameViewControllerLazy = userNameViewControllerLazy;
        mPrivacyChipViewControllerLazy = privacyChipViewControllerLazy;
        mButtonRoleHolderController = buttonRoleHolderController;

        // Read configuration.
        mShowTop = systemBarConfigs.getEnabledStatusBySide(SystemBarConfigs.TOP);
        mShowBottom = systemBarConfigs.getEnabledStatusBySide(SystemBarConfigs.BOTTOM);
        mShowLeft = systemBarConfigs.getEnabledStatusBySide(SystemBarConfigs.LEFT);
        mShowRight = systemBarConfigs.getEnabledStatusBySide(SystemBarConfigs.RIGHT);
    }

    /**
     * Hides all system bars.
     */
    public void hideBars() {
        setTopWindowVisibility(View.GONE);
        setBottomWindowVisibility(View.GONE);
        setLeftWindowVisibility(View.GONE);
        setRightWindowVisibility(View.GONE);
    }

    /**
     * Shows all system bars.
     */
    public void showBars() {
        setTopWindowVisibility(View.VISIBLE);
        setBottomWindowVisibility(View.VISIBLE);
        setLeftWindowVisibility(View.VISIBLE);
        setRightWindowVisibility(View.VISIBLE);
    }

    /** Connect to hvac service. */
    public void connectToHvac() {
        mHvacControllerLazy.get().connectToCarService();
    }

    /** Clean up */
    public void removeAll() {
        mHvacControllerLazy.get().removeAllComponents();
        mButtonSelectionStateController.removeAll();
        mButtonRoleHolderController.removeAll();
        mUserNameViewControllerLazy.get().removeAll();
        mPrivacyChipViewControllerLazy.get().removeAll();
    }

    /** Gets the top window if configured to do so. */
    @Nullable
    public ViewGroup getTopWindow() {
        return mShowTop ? mCarSystemBarViewFactory.getTopWindow() : null;
    }

    /** Gets the bottom window if configured to do so. */
    @Nullable
    public ViewGroup getBottomWindow() {
        return mShowBottom ? mCarSystemBarViewFactory.getBottomWindow() : null;
    }

    /** Gets the left window if configured to do so. */
    @Nullable
    public ViewGroup getLeftWindow() {
        return mShowLeft ? mCarSystemBarViewFactory.getLeftWindow() : null;
    }

    /** Gets the right window if configured to do so. */
    @Nullable
    public ViewGroup getRightWindow() {
        return mShowRight ? mCarSystemBarViewFactory.getRightWindow() : null;
    }

    /** Toggles the top nav bar visibility. */
    public boolean setTopWindowVisibility(@View.Visibility int visibility) {
        return setWindowVisibility(getTopWindow(), visibility);
    }

    /** Toggles the bottom nav bar visibility. */
    public boolean setBottomWindowVisibility(@View.Visibility int visibility) {
        return setWindowVisibility(getBottomWindow(), visibility);
    }

    /** Toggles the left nav bar visibility. */
    public boolean setLeftWindowVisibility(@View.Visibility int visibility) {
        return setWindowVisibility(getLeftWindow(), visibility);
    }

    /** Toggles the right nav bar visibility. */
    public boolean setRightWindowVisibility(@View.Visibility int visibility) {
        return setWindowVisibility(getRightWindow(), visibility);
    }

    private boolean setWindowVisibility(ViewGroup window, @View.Visibility int visibility) {
        if (window == null) {
            return false;
        }

        if (window.getVisibility() == visibility) {
            return false;
        }

        window.setVisibility(visibility);
        return true;
    }

    /** Gets the top navigation bar with the appropriate listeners set. */
    @Nullable
    public CarSystemBarView getTopBar(boolean isSetUp) {
        if (!mShowTop) {
            return null;
        }

        mTopView = mCarSystemBarViewFactory.getTopBar(isSetUp);
        setupBar(mTopView, mTopBarTouchListener, mNotificationsShadeController);
        return mTopView;
    }

    /** Gets the bottom navigation bar with the appropriate listeners set. */
    @Nullable
    public CarSystemBarView getBottomBar(boolean isSetUp) {
        if (!mShowBottom) {
            return null;
        }

        mBottomView = mCarSystemBarViewFactory.getBottomBar(isSetUp);
        setupBar(mBottomView, mBottomBarTouchListener, mNotificationsShadeController);
        return mBottomView;
    }

    /** Gets the left navigation bar with the appropriate listeners set. */
    @Nullable
    public CarSystemBarView getLeftBar(boolean isSetUp) {
        if (!mShowLeft) {
            return null;
        }

        mLeftView = mCarSystemBarViewFactory.getLeftBar(isSetUp);
        setupBar(mLeftView, mLeftBarTouchListener, mNotificationsShadeController);
        return mLeftView;
    }

    /** Gets the right navigation bar with the appropriate listeners set. */
    @Nullable
    public CarSystemBarView getRightBar(boolean isSetUp) {
        if (!mShowRight) {
            return null;
        }

        mRightView = mCarSystemBarViewFactory.getRightBar(isSetUp);
        setupBar(mRightView, mRightBarTouchListener, mNotificationsShadeController);
        return mRightView;
    }

    private void setupBar(CarSystemBarView view, View.OnTouchListener statusBarTouchListener,
            NotificationsShadeController notifShadeController) {
        view.setStatusBarWindowTouchListener(statusBarTouchListener);
        view.setNotificationsPanelController(notifShadeController);
        mButtonSelectionStateController.addAllButtonsWithSelectionState(view);
        mButtonRoleHolderController.addAllButtonsWithRoleName(view);
        mHvacControllerLazy.get().addTemperatureViewToController(view);
        mUserNameViewControllerLazy.get().addUserNameView(view);
        mPrivacyChipViewControllerLazy.get().addPrivacyChipView(view);
    }

    /** Sets a touch listener for the top navigation bar. */
    public void registerTopBarTouchListener(View.OnTouchListener listener) {
        mTopBarTouchListener = listener;
        if (mTopView != null) {
            mTopView.setStatusBarWindowTouchListener(mTopBarTouchListener);
        }
    }

    /** Sets a touch listener for the bottom navigation bar. */
    public void registerBottomBarTouchListener(View.OnTouchListener listener) {
        mBottomBarTouchListener = listener;
        if (mBottomView != null) {
            mBottomView.setStatusBarWindowTouchListener(mBottomBarTouchListener);
        }
    }

    /** Sets a touch listener for the left navigation bar. */
    public void registerLeftBarTouchListener(View.OnTouchListener listener) {
        mLeftBarTouchListener = listener;
        if (mLeftView != null) {
            mLeftView.setStatusBarWindowTouchListener(mLeftBarTouchListener);
        }
    }

    /** Sets a touch listener for the right navigation bar. */
    public void registerRightBarTouchListener(View.OnTouchListener listener) {
        mRightBarTouchListener = listener;
        if (mRightView != null) {
            mRightView.setStatusBarWindowTouchListener(mRightBarTouchListener);
        }
    }

    /** Sets a notification controller which toggles the notification panel. */
    public void registerNotificationController(
            NotificationsShadeController notificationsShadeController) {
        mNotificationsShadeController = notificationsShadeController;
        if (mTopView != null) {
            mTopView.setNotificationsPanelController(mNotificationsShadeController);
        }
        if (mBottomView != null) {
            mBottomView.setNotificationsPanelController(mNotificationsShadeController);
        }
        if (mLeftView != null) {
            mLeftView.setNotificationsPanelController(mNotificationsShadeController);
        }
        if (mRightView != null) {
            mRightView.setNotificationsPanelController(mNotificationsShadeController);
        }
    }

    /**
     *  Shows all of the navigation buttons on the valid instances of
     *  {@link CarSystemBarView}.
     */
    public void showAllNavigationButtons(boolean isSetUp) {
        checkAllBars(isSetUp);
        if (mTopView != null) {
            mTopView.showButtonsOfType(CarSystemBarView.BUTTON_TYPE_NAVIGATION);
        }
        if (mBottomView != null) {
            mBottomView.showButtonsOfType(CarSystemBarView.BUTTON_TYPE_NAVIGATION);
        }
        if (mLeftView != null) {
            mLeftView.showButtonsOfType(CarSystemBarView.BUTTON_TYPE_NAVIGATION);
        }
        if (mRightView != null) {
            mRightView.showButtonsOfType(CarSystemBarView.BUTTON_TYPE_NAVIGATION);
        }
    }

    /**
     * Shows all of the keyguard specific buttons on the valid instances of
     * {@link CarSystemBarView}.
     */
    public void showAllKeyguardButtons(boolean isSetUp) {
        checkAllBars(isSetUp);
        if (mTopView != null) {
            mTopView.showButtonsOfType(CarSystemBarView.BUTTON_TYPE_KEYGUARD);
        }
        if (mBottomView != null) {
            mBottomView.showButtonsOfType(CarSystemBarView.BUTTON_TYPE_KEYGUARD);
        }
        if (mLeftView != null) {
            mLeftView.showButtonsOfType(CarSystemBarView.BUTTON_TYPE_KEYGUARD);
        }
        if (mRightView != null) {
            mRightView.showButtonsOfType(CarSystemBarView.BUTTON_TYPE_KEYGUARD);
        }
    }

    /**
     *  Shows all of the occlusion state buttons on the valid instances of
     *  {@link CarSystemBarView}.
     */
    public void showAllOcclusionButtons(boolean isSetUp) {
        checkAllBars(isSetUp);
        if (mTopView != null) {
            mTopView.showButtonsOfType(CarSystemBarView.BUTTON_TYPE_OCCLUSION);
        }
        if (mBottomView != null) {
            mBottomView.showButtonsOfType(CarSystemBarView.BUTTON_TYPE_OCCLUSION);
        }
        if (mLeftView != null) {
            mLeftView.showButtonsOfType(CarSystemBarView.BUTTON_TYPE_OCCLUSION);
        }
        if (mRightView != null) {
            mRightView.showButtonsOfType(CarSystemBarView.BUTTON_TYPE_OCCLUSION);
        }
    }

    /** Toggles whether the notifications icon has an unseen indicator or not. */
    public void toggleAllNotificationsUnseenIndicator(boolean isSetUp, boolean hasUnseen) {
        checkAllBars(isSetUp);
        if (mTopView != null) {
            mTopView.toggleNotificationUnseenIndicator(hasUnseen);
        }
        if (mBottomView != null) {
            mBottomView.toggleNotificationUnseenIndicator(hasUnseen);
        }
        if (mLeftView != null) {
            mLeftView.toggleNotificationUnseenIndicator(hasUnseen);
        }
        if (mRightView != null) {
            mRightView.toggleNotificationUnseenIndicator(hasUnseen);
        }
    }

    /** Interface for controlling the notifications shade. */
    public interface NotificationsShadeController {
        /** Toggles the visibility of the notifications shade. */
        void togglePanel();

        /** Returns {@code true} if the panel is open. */
        boolean isNotificationPanelOpen();
    }

    private void checkAllBars(boolean isSetUp) {
        mTopView = getTopBar(isSetUp);
        mBottomView = getBottomBar(isSetUp);
        mLeftView = getLeftBar(isSetUp);
        mRightView = getRightBar(isSetUp);
    }
}
