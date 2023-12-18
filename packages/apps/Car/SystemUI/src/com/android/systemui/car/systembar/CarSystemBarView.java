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

import android.annotation.IntDef;
import android.content.Context;
import android.util.AttributeSet;
import android.view.MotionEvent;
import android.view.View;
import android.widget.LinearLayout;

import com.android.systemui.R;
import com.android.systemui.car.systembar.CarSystemBarController.NotificationsShadeController;
import com.android.systemui.statusbar.FeatureFlags;
import com.android.systemui.statusbar.phone.StatusBarIconController;

import java.lang.annotation.ElementType;
import java.lang.annotation.Target;

/**
 * A custom system bar for the automotive use case.
 * <p>
 * The system bar in the automotive use case is more like a list of shortcuts, rendered
 * in a linear layout.
 */
public class CarSystemBarView extends LinearLayout {

    @IntDef(value = {BUTTON_TYPE_NAVIGATION, BUTTON_TYPE_KEYGUARD, BUTTON_TYPE_OCCLUSION})
    @Target({ElementType.TYPE_PARAMETER, ElementType.TYPE_USE})
    private @interface ButtonsType {
    }

    public static final int BUTTON_TYPE_NAVIGATION = 0;
    public static final int BUTTON_TYPE_KEYGUARD = 1;
    public static final int BUTTON_TYPE_OCCLUSION = 2;

    private final boolean mConsumeTouchWhenPanelOpen;
    private final boolean mButtonsDraggable;

    private View mNavButtons;
    private CarSystemBarButton mNotificationsButton;
    private NotificationsShadeController mNotificationsShadeController;
    private View mLockScreenButtons;
    private View mOcclusionButtons;
    // used to wire in open/close gestures for notifications
    private OnTouchListener mStatusBarWindowTouchListener;

    public CarSystemBarView(Context context, AttributeSet attrs) {
        super(context, attrs);
        mConsumeTouchWhenPanelOpen = getResources().getBoolean(
                R.bool.config_consumeSystemBarTouchWhenNotificationPanelOpen);
        mButtonsDraggable = getResources().getBoolean(R.bool.config_systemBarButtonsDraggable);
    }

    @Override
    public void onFinishInflate() {
        mNavButtons = findViewById(R.id.nav_buttons);
        mLockScreenButtons = findViewById(R.id.lock_screen_nav_buttons);
        mOcclusionButtons = findViewById(R.id.occlusion_buttons);
        mNotificationsButton = findViewById(R.id.notifications);
        if (mNotificationsButton != null) {
            mNotificationsButton.setOnClickListener(this::onNotificationsClick);
        }
        // Needs to be clickable so that it will receive ACTION_MOVE events.
        setClickable(true);
        // Needs to not be focusable so rotary won't highlight the entire nav bar.
        setFocusable(false);
    }

    void setupIconController(FeatureFlags featureFlags, StatusBarIconController iconController) {
        View mStatusIcons = findViewById(R.id.statusIcons);
        if (mStatusIcons != null) {
            // Attach the controllers for Status icons such as wifi and bluetooth if the standard
            // container is in the view.
            StatusBarIconController.DarkIconManager mDarkIconManager =
                    new StatusBarIconController.DarkIconManager(
                            mStatusIcons.findViewById(R.id.statusIcons), featureFlags);
            mDarkIconManager.setShouldLog(true);
            iconController.addIconGroup(mDarkIconManager);
        }
    }

    // Used to forward touch events even if the touch was initiated from a child component
    @Override
    public boolean onInterceptTouchEvent(MotionEvent ev) {
        if (mStatusBarWindowTouchListener != null) {
            if (!mButtonsDraggable) {
                return false;
            }
            boolean shouldConsumeEvent = mNotificationsShadeController == null ? false
                    : mNotificationsShadeController.isNotificationPanelOpen();

            // Forward touch events to the status bar window so it can drag
            // windows if required (Notification shade)
            mStatusBarWindowTouchListener.onTouch(this, ev);

            if (mConsumeTouchWhenPanelOpen && shouldConsumeEvent) {
                return true;
            }
        }
        return super.onInterceptTouchEvent(ev);
    }

    /** Sets the notifications panel controller. */
    public void setNotificationsPanelController(NotificationsShadeController controller) {
        mNotificationsShadeController = controller;
    }

    /** Gets the notifications panel controller. */
    public NotificationsShadeController getNotificationsPanelController() {
        return mNotificationsShadeController;
    }

    /**
     * Sets a touch listener that will be called from onInterceptTouchEvent and onTouchEvent
     *
     * @param statusBarWindowTouchListener The listener to call from touch and intercept touch
     */
    public void setStatusBarWindowTouchListener(OnTouchListener statusBarWindowTouchListener) {
        mStatusBarWindowTouchListener = statusBarWindowTouchListener;
    }

    /** Gets the touch listener that will be called from onInterceptTouchEvent and onTouchEvent. */
    public OnTouchListener getStatusBarWindowTouchListener() {
        return mStatusBarWindowTouchListener;
    }

    @Override
    public boolean onTouchEvent(MotionEvent event) {
        if (mStatusBarWindowTouchListener != null) {
            mStatusBarWindowTouchListener.onTouch(this, event);
        }
        return super.onTouchEvent(event);
    }

    protected void onNotificationsClick(View v) {
        if (mNotificationsShadeController != null) {
            mNotificationsShadeController.togglePanel();
        }
    }

    /**
     * Shows buttons of the specified {@link ButtonsType}.
     *
     * NOTE: Only one type of buttons can be shown at a time, so showing buttons of one type will
     * hide all buttons of other types.
     *
     * @param buttonsType
     */
    public void showButtonsOfType(@ButtonsType int buttonsType) {
        switch(buttonsType) {
            case BUTTON_TYPE_NAVIGATION:
                setNavigationButtonsVisibility(View.VISIBLE);
                setKeyguardButtonsVisibility(View.GONE);
                setOcclusionButtonsVisibility(View.GONE);
                break;
            case BUTTON_TYPE_KEYGUARD:
                setNavigationButtonsVisibility(View.GONE);
                setKeyguardButtonsVisibility(View.VISIBLE);
                setOcclusionButtonsVisibility(View.GONE);
                break;
            case BUTTON_TYPE_OCCLUSION:
                setNavigationButtonsVisibility(View.GONE);
                setKeyguardButtonsVisibility(View.GONE);
                setOcclusionButtonsVisibility(View.VISIBLE);
                break;
        }
    }

    private void setNavigationButtonsVisibility(int visibility) {
        if (mNavButtons != null) {
            mNavButtons.setVisibility(visibility);
        }
    }

    private void setKeyguardButtonsVisibility(int visibility) {
        if (mLockScreenButtons != null) {
            mLockScreenButtons.setVisibility(visibility);
        }
    }

    private void setOcclusionButtonsVisibility(int visibility) {
        if (mOcclusionButtons != null) {
            mOcclusionButtons.setVisibility(visibility);
        }
    }

    /**
     * Toggles the notification unseen indicator on/off.
     *
     * @param hasUnseen true if the unseen notification count is great than 0.
     */
    public void toggleNotificationUnseenIndicator(Boolean hasUnseen) {
        if (mNotificationsButton == null) return;

        mNotificationsButton.setUnseen(hasUnseen);
    }
}
