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

package com.android.wallpaper.util;

import android.animation.Animator;
import android.animation.AnimatorSet;
import android.animation.ObjectAnimator;
import android.animation.ValueAnimator;
import android.content.res.TypedArray;
import android.graphics.Insets;
import android.graphics.Point;
import android.graphics.Rect;
import android.view.Gravity;
import android.view.SurfaceView;
import android.view.View;
import android.view.WindowInsets;
import android.widget.Button;
import android.widget.FrameLayout;
import android.widget.ImageButton;
import android.widget.TextView;
import android.widget.Toolbar;

import androidx.cardview.widget.CardView;

import com.android.wallpaper.R;
import com.android.wallpaper.picker.TouchForwardingLayout;

import com.google.android.material.appbar.AppBarLayout;

/**
 * A class storing information about a preview fragment's full-screen layout.
 *
 * Used for {@code ImagePreviewFragment} and {@code LivePreviewFragment}.
 */
public class FullScreenAnimation {

    private final View mView;
    private final TouchForwardingLayout mTouchForwardingLayout;
    private final SurfaceView mWorkspaceSurface;
    private boolean mIsFullScreen = false;

    private boolean mScaleIsSet = false;
    private boolean mWorkspaceVisibility = true;
    private float mOffsetY;
    private float mScale;
    private float mDefaultRadius;
    private int mWorkspaceWidth;
    private int mWorkspaceHeight;
    private float mBottomActionBarTranslation;
    private float mFullScreenButtonsTranslation;
    private int mStatusBarHeight;
    private int mNavigationBarHeight;
    private FullScreenStatusListener mFullScreenStatusListener;

    private static final float HIDE_ICONS_TOP_RATIO = 0.2f;

    private boolean mIsHomeSelected = true;

    /**
     * Options for the full-screen text color.
     *
     * {@code DEFAULT} represents the default text color.
     * {@code DARK} represents a text color that is dark, and should be used when the wallpaper
     *              supports dark text.
     * {@code LIGHT} represents a text color that is light, and should be used when the wallpaper
     *               does not support dark text.
     */
    public enum FullScreenTextColor {
        DEFAULT,
        DARK,
        LIGHT
    }

    FullScreenTextColor mFullScreenTextColor = FullScreenTextColor.DEFAULT;
    private int mCurrentTextColor;

    /** Callback for full screen status. */
    public interface FullScreenStatusListener {
        /** Gets called at animation end when full screen status gets changed. */
        void onFullScreenStatusChange(boolean isFullScreen);
    }

    /**
     * Constructor.
     *
     * @param view The view containing all relevant UI elements. Equal to {@code mRootView}.
     */
    public FullScreenAnimation(View view) {
        mView = view;
        mTouchForwardingLayout = view.findViewById(R.id.touch_forwarding_layout);
        mWorkspaceSurface = view.findViewById(R.id.workspace_surface);
        mCurrentTextColor = ResourceUtils.getColorAttr(
                view.getContext(),
                android.R.attr.textColorPrimary);
    }

    /**
     * Returns if the preview layout is currently in full screen.
     *
     * @return whether the preview layout is currently in full screen.
     */
    public boolean isFullScreen() {
        return mIsFullScreen;
    }

    /**
     * Informs this object whether the home tab is selected.
     *
     * Used to determine the visibility of {@code lock_screen_preview_container}.
     *
     * @param isHomeSelected whether the home tab is selected.
     */
    public void setIsHomeSelected(boolean isHomeSelected) {
        mIsHomeSelected = isHomeSelected;
    }

    private int getStatusBarHeight() {
        return mStatusBarHeight;
    }

    private int getNavigationBarHeight() {
        return mNavigationBarHeight;
    }

    private int getAttributeDimension(int resId) {
        final TypedArray attributes = mView.getContext().getTheme().obtainStyledAttributes(
                new int[]{resId});
        int dimension = attributes.getDimensionPixelSize(0, 0);
        attributes.recycle();
        return dimension;
    }

    private void setViewMargins(int viewId, float marginTop, float marginBottom,
            boolean separatedTabs) {
        FrameLayout.LayoutParams layoutParams = new FrameLayout.LayoutParams(
                FrameLayout.LayoutParams.MATCH_PARENT,
                separatedTabs ? FrameLayout.LayoutParams.WRAP_CONTENT
                        : FrameLayout.LayoutParams.MATCH_PARENT);

        layoutParams.setMargins(0, Math.round(marginTop), 0, Math.round(marginBottom));

        if (separatedTabs) {
            layoutParams.gravity = Gravity.BOTTOM;
        }

        mView.findViewById(viewId).setLayoutParams(layoutParams);
    }

    /** Sets a {@param listener} to listen full screen state changes. */
    public void setFullScreenStatusListener(FullScreenStatusListener listener) {
        mFullScreenStatusListener = listener;
    }

    /**
     * Informs the {@code FullScreenAnimation} object about the window insets of the current
     * window.
     *
     * Called by a {@code View.OnApplyWindowInsetsListener} defined in {@code PreviewFragment}.
     *
     * @param windowInsets the window insets of the current window.
     */
    public void setWindowInsets(WindowInsets windowInsets) {
        Insets insets = windowInsets.getInsetsIgnoringVisibility(
                WindowInsets.Type.systemBars()
        );

        mStatusBarHeight = insets.top;
        mNavigationBarHeight = insets.bottom;
    }

    /**
     * Place UI elements in the correct locations.
     *
     * Takes status bar and navigation bar into account.
     */
    public void placeViews() {
        setViewMargins(R.id.screen_preview_layout,
                getStatusBarHeight() + getAttributeDimension(R.attr.actionBarSize),
                getNavigationBarHeight()
                        + mView.getResources().getDimension(R.dimen.bottom_actions_height)
                        + mView.getResources().getDimension(R.dimen.separated_tabs_height),
                false);
        setViewMargins(R.id.bottom_action_bar_container,
                0,
                getNavigationBarHeight(),
                false);
        setViewMargins(R.id.separated_tabs_container,
                0,
                getNavigationBarHeight()
                        + mView.getResources().getDimension(R.dimen.bottom_actions_height),
                true);
        ensureToolbarIsCorrectlyLocated();
    }

    /**
     * Ensures that the bottom action bar is in the correct location.
     *
     * Called by {@code onBottomActionBarReady}, so that the bottom action bar is correctly located
     * when it is redrawn.
     */
    public void ensureBottomActionBarIsCorrectlyLocated() {
        float targetTranslation = mIsFullScreen ? mBottomActionBarTranslation : 0;
        mView.findViewById(R.id.bottom_actionbar).setTranslationY(targetTranslation);
    }

    /**
     * Ensures that the toolbar is in the correct location.
     *
     * Called by {@code placeViews}, {@code ImageWallpaperColorThemePreviewFragment#updateToolBar},
     * and @{code LiveWallpaperColorThemePreviewFragment#updateToolBar}, so that the toolbar is
     * correctly located when it is redrawn.
     */
    public void ensureToolbarIsCorrectlyLocated() {
        AppBarLayout.LayoutParams layoutParams = new AppBarLayout.LayoutParams(
                AppBarLayout.LayoutParams.MATCH_PARENT,
                AppBarLayout.LayoutParams.MATCH_PARENT);

        layoutParams.setMargins(0, getStatusBarHeight(), 0, 0);

        mView.findViewById(R.id.toolbar).setLayoutParams(layoutParams);
    }

    /**
     * Ensures that the text and the navigation button on the toolbar is given the correct color.
     *
     * Called by {@code updateToolBar}.
     */
    public void ensureToolbarIsCorrectlyColored() {
        TextView textView = mView.findViewById(R.id.custom_toolbar_title);
        textView.setTextColor(mCurrentTextColor);

        Toolbar toolbar = mView.findViewById(R.id.toolbar);
        // It may be null because there's no back arrow in some cases. For example: no back arrow
        // for Photos launching case.
        ImageButton button = (ImageButton) toolbar.getNavigationView();
        if (button != null) {
            button.setColorFilter(mCurrentTextColor);
        }
    }

    /**
     * Sets the text color used for the "Preview" caption in full screen mode.
     *
     * @param fullScreenTextColor The desired color for the "Preview" caption in full screen mode.
     */
    public void setFullScreenTextColor(FullScreenTextColor fullScreenTextColor) {
        mFullScreenTextColor = fullScreenTextColor;

        animateColor(mIsFullScreen);
    }

    /**
     * Sets the visibility of the workspace surface (containing icons from the home screen) and
     * the elements unique to the lock screen (date and time).
     *
     * Called when the "Hide UI Preview" button is clicked.
     *
     * @param visible {@code true} if the icons should be shown;
     *                {@code false} if they should be hidden.
     */
    public void setWorkspaceVisibility(boolean visible) {
        // Not using [setVisibility], because it creates a "jump".
        if (visible) {
            mWorkspaceSurface.setClipBounds(new Rect(
                    0,
                    Math.round(mWorkspaceHeight * HIDE_ICONS_TOP_RATIO),
                    mWorkspaceWidth,
                    mWorkspaceHeight + Math.round(mFullScreenButtonsTranslation / mScale)));
            mView.findViewById(R.id.lock_screen_preview_container).setVisibility(View.VISIBLE);
        } else {
            int half = mWorkspaceHeight / 2;
            mWorkspaceSurface.setClipBounds(new Rect(
                    0,
                    half,
                    mWorkspaceWidth,
                    half + 1));
            mView.findViewById(R.id.lock_screen_preview_container).setVisibility(View.INVISIBLE);
        }
        if (mIsHomeSelected) {
            mView.findViewById(R.id.lock_screen_preview_container).setVisibility(View.INVISIBLE);
        }
        mWorkspaceVisibility = visible;
    }

    /**
     * Returns the visibility of the workspace surface (containing icons from the home screen).
     *
     * @return the visibility of the workspace surface.
     */
    public boolean getWorkspaceVisibility() {
        return mWorkspaceVisibility;
    }

    private void animateColor(boolean toFullScreen) {
        TextView textView = mView.findViewById(R.id.custom_toolbar_title);

        int targetColor;
        if (!toFullScreen || mFullScreenTextColor == FullScreenTextColor.DEFAULT) {
            targetColor = ResourceUtils.getColorAttr(
                    mView.getContext(),
                    android.R.attr.textColorPrimary);
        } else if (mFullScreenTextColor == FullScreenTextColor.DARK) {
            targetColor = mView.getContext().getColor(android.R.color.black);
        } else {
            targetColor = mView.getContext().getColor(android.R.color.white);
        }

        if (targetColor == mCurrentTextColor) {
            return;
        }

        Toolbar toolbar = mView.findViewById(R.id.toolbar);
        ImageButton button = (ImageButton) toolbar.getNavigationView();

        ValueAnimator colorAnimator = ValueAnimator.ofArgb(mCurrentTextColor, targetColor);
        colorAnimator.addUpdateListener(animation -> {
            int color = (int) animation.getAnimatedValue();
            textView.setTextColor(color);
            // It may be null because there's no back arrow in some cases. For example: no back
            // arrow for Photos launching case.
            if (button != null) {
                button.setColorFilter(color);
            }
        });
        colorAnimator.start();

        mCurrentTextColor = targetColor;
    }

    /**
     * Animates the layout to or from fullscreen.
     *
     * @param toFullScreen {@code true} if animating into the full screen layout;
     *                     {@code false} if animating out of the full screen layout.
     */
    public void startAnimation(boolean toFullScreen) {
        // If there is no need to animate, return.
        if (toFullScreen == mIsFullScreen) {
            return;
        }

        // If the scale is not set, compute the location and size of frame layout.
        if (!mScaleIsSet) {
            int[] loc = new int[2];
            mTouchForwardingLayout.getLocationInWindow(loc);

            ScreenSizeCalculator screenSizeCalculator = ScreenSizeCalculator.getInstance();
            Point screenSize = screenSizeCalculator.getScreenSize(mView.getDisplay());
            int screenWidth = screenSize.x;
            int screenHeight = screenSize.y;

            mOffsetY = (float) (screenHeight / 2.0
                    - (loc[1] + mTouchForwardingLayout.getHeight() / 2.0));

            mScale = Math.max(
                    screenWidth / (float) mTouchForwardingLayout.getWidth(),
                    screenHeight / (float) mTouchForwardingLayout.getHeight());

            mDefaultRadius = ((CardView) mWorkspaceSurface.getParent()).getRadius();

            mWorkspaceSurface.setEnableSurfaceClipping(true);

            mWorkspaceWidth = mWorkspaceSurface.getWidth();
            mWorkspaceHeight = mWorkspaceSurface.getHeight();

            mBottomActionBarTranslation = getNavigationBarHeight()
                    + mView.getResources().getDimension(R.dimen.bottom_actions_height)
                    + mView.getResources().getDimension(R.dimen.separated_tabs_height);

            mFullScreenButtonsTranslation = -(getNavigationBarHeight()
                    + mView.getResources().getDimension(
                            R.dimen.fullscreen_preview_button_margin_bottom)
                    + mView.getResources().getDimension(R.dimen.separated_tabs_height));

            mScaleIsSet = true;
        }

        // Perform animations.

        // Rounding animation.
        // Animated version of ((CardView) mWorkspaceSurface.getParent()).setRadius(0);
        float fromRadius = toFullScreen ? mDefaultRadius : 0f;
        float toRadius = toFullScreen ? 0f : mDefaultRadius;

        ValueAnimator animationRounding = ValueAnimator.ofFloat(fromRadius, toRadius);
        animationRounding.addUpdateListener(animation -> {
            ((CardView) mWorkspaceSurface.getParent()).setRadius(
                    (float) animation.getAnimatedValue());
        });

        // Animation to hide some of the home screen icons.
        float fromTop = toFullScreen ? 0f : HIDE_ICONS_TOP_RATIO;
        float toTop = toFullScreen ? HIDE_ICONS_TOP_RATIO : 0f;
        float fromBottom = toFullScreen ? 0 : mFullScreenButtonsTranslation / mScale;
        float toBottom = toFullScreen ? mFullScreenButtonsTranslation / mScale : 0;

        ValueAnimator animationHide = ValueAnimator.ofFloat(0f, 1f);
        animationHide.addUpdateListener(animation -> {
            float t = (float) animation.getAnimatedValue();
            float top = fromTop + t * (toTop - fromTop);
            float bottom = fromBottom + t * (toBottom - fromBottom);
            mWorkspaceSurface.setClipBounds(new Rect(
                    0,
                    Math.round(mWorkspaceHeight * top),
                    mWorkspaceWidth,
                    mWorkspaceHeight + Math.round(bottom)));
        });

        // Other animations.
        float scale = toFullScreen ? mScale : 1f;
        float offsetY = toFullScreen ? mOffsetY : 0f;
        float bottomActionBarTranslation = toFullScreen ? mBottomActionBarTranslation : 0;
        float fullScreenButtonsTranslation = toFullScreen ? mFullScreenButtonsTranslation : 0;
        View frameLayout = mView.findViewById(R.id.screen_preview_layout);

        AnimatorSet animatorSet = new AnimatorSet();
        animatorSet.playTogether(
                ObjectAnimator.ofFloat(frameLayout, "scaleX", scale),
                ObjectAnimator.ofFloat(frameLayout, "scaleY", scale),
                ObjectAnimator.ofFloat(frameLayout, "translationY", offsetY),
                ObjectAnimator.ofFloat(mView.findViewById(R.id.bottom_actionbar),
                        "translationY", bottomActionBarTranslation),
                ObjectAnimator.ofFloat(mView.findViewById(R.id.separated_tabs_container),
                        "translationY", bottomActionBarTranslation),
                ObjectAnimator.ofFloat(mView.findViewById(R.id.fullscreen_buttons_container),
                        "translationY", fullScreenButtonsTranslation),
                animationRounding,
                animationHide
        );
        animatorSet.addListener(new Animator.AnimatorListener() {
            @Override
            public void onAnimationCancel(Animator animator) {}

            @Override
            public void onAnimationEnd(Animator animator) {
                if (mFullScreenStatusListener != null) {
                    mFullScreenStatusListener.onFullScreenStatusChange(toFullScreen);
                }
            }

            @Override
            public void onAnimationRepeat(Animator animator) {}

            @Override
            public void onAnimationStart(Animator animator) {}
        });
        animatorSet.start();

        animateColor(toFullScreen);

        // Changes appearances of some elements.
        mWorkspaceVisibility = true;

        if (toFullScreen) {
            ((Button) mView.findViewById(R.id.hide_ui_preview_button)).setText(
                    R.string.hide_ui_preview_text
            );
        }

        mView.findViewById(R.id.lock_screen_preview_container).setVisibility(View.VISIBLE);
        if (mIsHomeSelected) {
            mView.findViewById(R.id.lock_screen_preview_container)
                    .setVisibility(View.INVISIBLE);
        }

        mIsFullScreen = toFullScreen;
    }
}
