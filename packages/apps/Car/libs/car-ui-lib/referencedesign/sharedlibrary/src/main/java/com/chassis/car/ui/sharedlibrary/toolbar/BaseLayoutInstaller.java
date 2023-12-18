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
package com.chassis.car.ui.sharedlibrary.toolbar;

import android.content.Context;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;
import android.view.ViewTreeObserver;
import android.widget.FrameLayout;
import android.widget.LinearLayout;

import androidx.annotation.LayoutRes;
import androidx.annotation.Nullable;

import com.android.car.ui.sharedlibrary.oemapis.FocusAreaOEMV1;
import com.android.car.ui.sharedlibrary.oemapis.FocusParkingViewOEMV1;
import com.android.car.ui.sharedlibrary.oemapis.InsetsOEMV1;
import com.android.car.ui.sharedlibrary.oemapis.toolbar.ToolbarControllerOEMV1;

import com.chassis.car.ui.sharedlibrary.R;

import java.util.function.Consumer;
import java.util.function.Function;

/**
 * A helper class for implementing installBaseLayoutAround from
 * {@link com.android.car.ui.sharedlibrary.oemapis.SharedLibraryFactoryOEMV1}
 */
@SuppressWarnings("AndroidJdkLibsChecker")
public class BaseLayoutInstaller {
    /**
     * Implementation of installBaseLayoutAround from
     * {@link com.android.car.ui.sharedlibrary.oemapis.SharedLibraryFactoryOEMV1}
     */
    public static ToolbarControllerOEMV1 installBaseLayoutAround(
            Context sharedLibraryContext,
            View contentView,
            Consumer<InsetsOEMV1> insetsChangedListener,
            boolean toolbarEnabled,
            boolean fullscreen,
            @Nullable Function<Context, FocusParkingViewOEMV1> focusParkingViewFactory,
            @Nullable Function<Context, FocusAreaOEMV1> focusAreaFactory) {

        Context activityContext = contentView.getContext();

        // Add the configuration from the activity context to the shared library context,
        // or else when inflating views with the shared library context, they won't have access
        // to stuff like the screen size. It will also cause a StrictMode violation without this.
        sharedLibraryContext = sharedLibraryContext.createConfigurationContext(
                activityContext.getResources().getConfiguration());

        @LayoutRes int layout = toolbarEnabled
                ? R.layout.base_layout_toolbar
                : R.layout.base_layout;
        FrameLayout baseLayout = (FrameLayout) LayoutInflater.from(sharedLibraryContext).inflate(
                layout, null, false);

        // Replace the app's content view with a base layout
        ViewGroup contentViewParent = (ViewGroup) contentView.getParent();
        int contentIndex = contentViewParent.indexOfChild(contentView);
        contentViewParent.removeView(contentView);
        contentViewParent.addView(baseLayout, contentIndex, contentView.getLayoutParams());

        // Add the app's content view to the baseLayout's content view container
        FrameLayout contentViewContainer = baseLayout.requireViewById(
                R.id.base_layout_content_container);
        contentViewContainer.addView(contentView, new FrameLayout.LayoutParams(
                ViewGroup.LayoutParams.MATCH_PARENT,
                ViewGroup.LayoutParams.MATCH_PARENT));

        // Add FocusParkingView to base layout.
        // Make sure to use the application context here, not the shared library context,
        // as the implementation of FocusParkingView/FocusArea is in the static car-ui-lib.
        if (focusParkingViewFactory != null) {
            View focusParkingView = focusParkingViewFactory.apply(activityContext).getView();
            if (focusParkingView != null) {
                baseLayout.addView(focusParkingView, 0,
                        new FrameLayout.LayoutParams(
                                ViewGroup.LayoutParams.WRAP_CONTENT,
                                ViewGroup.LayoutParams.WRAP_CONTENT));
            }
        }

        // Replace toolbar_background with a FocusArea.
        // Note that FocusArea must be created like this, as opposed to using a custom
        // layout inflater factory and specifying it in XML, because a LayoutInflater will
        // use a parent view's context for creating all the child views. We have to create
        // the FocusArea using the app's context, so that it can access it's resources,
        // but we want children of the FocusArea to use the shared library context, so we can
        // access shared library resources.
        if (focusAreaFactory != null && toolbarEnabled) {
            LinearLayout focusArea = focusAreaFactory.apply(activityContext).getView();
            if (focusArea != null) {
                View toolbar = baseLayout.requireViewById(R.id.toolbar_background);
                int toolbarIndex = baseLayout.indexOfChild(toolbar);
                baseLayout.removeView(toolbar);
                baseLayout.addView(focusArea, toolbarIndex, toolbar.getLayoutParams());
                focusArea.addView(toolbar, new LinearLayout.LayoutParams(
                        ViewGroup.LayoutParams.MATCH_PARENT, ViewGroup.LayoutParams.MATCH_PARENT));
            }
        }


        ToolbarControllerOEMV1 toolbarController = null;
        if (toolbarEnabled) {
            toolbarController = new ToolbarControllerImpl(
                    baseLayout, sharedLibraryContext, activityContext);
        }

        InsetsUpdater updater = new InsetsUpdater(baseLayout, contentView);
        updater.replaceInsetsChangedListenerWith(insetsChangedListener);
        updater.installListeners();

        return toolbarController;
    }

    /**
     * InsetsUpdater waits for layout changes, and when there is one, calculates the appropriate
     * insets into the content view.
     */
    private static final class InsetsUpdater implements ViewTreeObserver.OnGlobalLayoutListener {
        // These tags mark views that should overlay the content view in the base layout.
        // Apps will then be able to draw under these views, but will be encouraged to not put
        // any user-interactable content there.
        private static final String LEFT_INSET_TAG = "shared_lib_left_inset";
        private static final String RIGHT_INSET_TAG = "shared_lib_right_inset";
        private static final String TOP_INSET_TAG = "shared_lib_top_inset";
        private static final String BOTTOM_INSET_TAG = "shared_lib_bottom_inset";

        private final View mContentView;
        private final View mContentViewContainer; // Equivalent to mContentView except in Media
        private final View mLeftInsetView;
        private final View mRightInsetView;
        private final View mTopInsetView;
        private final View mBottomInsetView;
        private Consumer<InsetsOEMV1> mInsetsChangedListenerDelegate;

        private boolean mInsetsDirty = true;
        private InsetsOEMV1 mInsets = new InsetsOEMV1();

        /**
         * Constructs an InsetsUpdater that calculates and dispatches insets to the method provided
         * via {@link #replaceInsetsChangedListenerWith(Consumer)}.
         *
         * @param baseLayout  The root view of the base layout
         * @param contentView The android.R.id.content View
         */
        InsetsUpdater(
                View baseLayout,
                View contentView) {
            mContentView = contentView;
            mContentViewContainer = baseLayout.requireViewById(R.id.base_layout_content_container);

            mLeftInsetView = baseLayout.findViewWithTag(LEFT_INSET_TAG);
            mRightInsetView = baseLayout.findViewWithTag(RIGHT_INSET_TAG);
            mTopInsetView = baseLayout.findViewWithTag(TOP_INSET_TAG);
            mBottomInsetView = baseLayout.findViewWithTag(BOTTOM_INSET_TAG);

            final View.OnLayoutChangeListener layoutChangeListener =
                    (View v, int left, int top, int right, int bottom,
                            int oldLeft, int oldTop, int oldRight, int oldBottom) -> {
                        if (left != oldLeft || top != oldTop
                                || right != oldRight || bottom != oldBottom) {
                            mInsetsDirty = true;
                        }
                    };

            if (mLeftInsetView != null) {
                mLeftInsetView.addOnLayoutChangeListener(layoutChangeListener);
            }
            if (mRightInsetView != null) {
                mRightInsetView.addOnLayoutChangeListener(layoutChangeListener);
            }
            if (mTopInsetView != null) {
                mTopInsetView.addOnLayoutChangeListener(layoutChangeListener);
            }
            if (mBottomInsetView != null) {
                mBottomInsetView.addOnLayoutChangeListener(layoutChangeListener);
            }
            contentView.addOnLayoutChangeListener(layoutChangeListener);
            mContentViewContainer.addOnLayoutChangeListener(layoutChangeListener);
        }

        /**
         * Install a global layout listener, during which the insets will be recalculated and
         * dispatched.
         */
        public void installListeners() {
            // The global layout listener will run after all the individual layout change listeners
            // so that we only updateInsets once per layout, even if multiple inset views changed
            mContentView.getRootView().getViewTreeObserver()
                    .addOnGlobalLayoutListener(this);
        }

        InsetsOEMV1 getInsets() {
            return mInsets;
        }

        // TODO remove this method / cleanup this class
        public void replaceInsetsChangedListenerWith(Consumer<InsetsOEMV1> listener) {
            mInsetsChangedListenerDelegate = listener;
        }

        /**
         * onGlobalLayout() should recalculate the amount of insets we need, and then dispatch them.
         */
        @Override
        public void onGlobalLayout() {
            if (!mInsetsDirty) {
                return;
            }

            // Calculate how much each inset view overlays the content view

            // These initial values are for Media Center's implementation of base layouts.
            // They should evaluate to 0 in all other apps, because the content view and content
            // view container have the same size and position there.
            int top = Math.max(0,
                    getTopOfView(mContentViewContainer) - getTopOfView(mContentView));
            int left = Math.max(0,
                    getLeftOfView(mContentViewContainer) - getLeftOfView(mContentView));
            int right = Math.max(0,
                    getRightOfView(mContentView) - getRightOfView(mContentViewContainer));
            int bottom = Math.max(0,
                    getBottomOfView(mContentView) - getBottomOfView(mContentViewContainer));
            if (mTopInsetView != null) {
                top += Math.max(0,
                        getBottomOfView(mTopInsetView) - getTopOfView(mContentViewContainer));
            }
            if (mBottomInsetView != null) {
                bottom += Math.max(0,
                        getBottomOfView(mContentViewContainer) - getTopOfView(mBottomInsetView));
            }
            if (mLeftInsetView != null) {
                left += Math.max(0,
                        getRightOfView(mLeftInsetView) - getLeftOfView(mContentViewContainer));
            }
            if (mRightInsetView != null) {
                right += Math.max(0,
                        getRightOfView(mContentViewContainer) - getLeftOfView(mRightInsetView));
            }
            InsetsOEMV1 insets = new InsetsOEMV1(left, top, right, bottom);

            mInsetsDirty = false;
            if (!insets.equals(mInsets)) {
                mInsets = insets;
                if (mInsetsChangedListenerDelegate != null) {
                    mInsetsChangedListenerDelegate.accept(insets);
                }
            }
        }

        private static int getLeftOfView(View v) {
            int[] position = new int[2];
            v.getLocationOnScreen(position);
            return position[0];
        }

        private static int getRightOfView(View v) {
            int[] position = new int[2];
            v.getLocationOnScreen(position);
            return position[0] + v.getWidth();
        }

        private static int getTopOfView(View v) {
            int[] position = new int[2];
            v.getLocationOnScreen(position);
            return position[1];
        }

        private static int getBottomOfView(View v) {
            int[] position = new int[2];
            v.getLocationOnScreen(position);
            return position[1] + v.getHeight();
        }
    }
}
