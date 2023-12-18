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
package com.android.car.ui.sharedlibrarysupport;

import static com.android.car.ui.utils.CarUiUtils.requireViewByRefId;

import android.app.Activity;
import android.content.Context;
import android.os.Build;
import android.util.AttributeSet;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;
import android.widget.FrameLayout;

import androidx.annotation.LayoutRes;
import androidx.annotation.NonNull;
import androidx.annotation.Nullable;
import androidx.fragment.app.Fragment;

import com.android.car.ui.R;
import com.android.car.ui.appstyledview.AppStyledViewController;
import com.android.car.ui.appstyledview.AppStyledViewControllerImpl;
import com.android.car.ui.baselayout.Insets;
import com.android.car.ui.baselayout.InsetsChangedListener;
import com.android.car.ui.recyclerview.CarUiListItem;
import com.android.car.ui.recyclerview.CarUiListItemAdapter;
import com.android.car.ui.recyclerview.CarUiRecyclerView;
import com.android.car.ui.recyclerview.CarUiRecyclerViewImpl;
import com.android.car.ui.toolbar.ToolbarController;
import com.android.car.ui.toolbar.ToolbarControllerImpl;
import com.android.car.ui.widget.CarUiTextView;
import com.android.car.ui.widget.CarUiTextViewImpl;

import java.util.List;

/**
 * This is the stub implementation of {@link SharedLibraryFactory}, used when there is no shared
 * library installed on the system. It delegates to the static library implementation of the
 * necessary components.
 * <p>
 * Do not use from client apps, for car-ui-lib internal use only.
 */
//TODO(b/179092760) Find a way to prevent apps from using this
public final class SharedLibraryFactoryStub implements SharedLibraryFactory {

    private final Context mContext;

    public SharedLibraryFactoryStub(Context context) {
        mContext = context;
    }

    @Nullable
    @Override
    public ToolbarController installBaseLayoutAround(View contentView,
            InsetsChangedListener insetsChangedListener,
            boolean toolbarEnabled,
            boolean fullscreen) {
        boolean legacyToolbar = Build.VERSION.SDK_INT <= Build.VERSION_CODES.Q;
        @LayoutRes final int baseLayoutRes;

        if (toolbarEnabled) {
            if (legacyToolbar) {
                boolean twoRow = contentView.getResources()
                        .getBoolean(R.bool.car_ui_toolbar_tabs_on_second_row);
                baseLayoutRes = twoRow
                        ? R.layout.car_ui_base_layout_toolbar_legacy_two_row
                        : R.layout.car_ui_base_layout_toolbar_legacy;
            } else {
                baseLayoutRes = R.layout.car_ui_base_layout_toolbar;
            }
        } else {
            baseLayoutRes = R.layout.car_ui_base_layout;
        }

        View baseLayout = LayoutInflater.from(contentView.getContext())
                .inflate(baseLayoutRes, null, false);

        // Replace the app's content view with a base layout
        ViewGroup contentViewParent = (ViewGroup) contentView.getParent();
        int contentIndex = contentViewParent.indexOfChild(contentView);
        contentViewParent.removeView(contentView);
        contentViewParent.addView(baseLayout, contentIndex, contentView.getLayoutParams());

        // Add the app's content view to the baseLayout's content view container
        FrameLayout contentViewContainer = requireViewByRefId(baseLayout,
                R.id.car_ui_base_layout_content_container);
        contentViewContainer.addView(contentView, new FrameLayout.LayoutParams(
                ViewGroup.LayoutParams.MATCH_PARENT,
                ViewGroup.LayoutParams.MATCH_PARENT));

        ToolbarController toolbarController = null;
        if (toolbarEnabled) {
            if (legacyToolbar) {
                toolbarController = new ToolbarControllerImpl(
                        requireViewByRefId(baseLayout, R.id.car_ui_toolbar));
            } else {
                toolbarController = new ToolbarControllerImpl(baseLayout);
            }
        }

        InsetsUpdater insetsUpdater = new InsetsUpdater(baseLayout, contentView);
        insetsUpdater.replaceInsetsChangedListenerWith(insetsChangedListener);

        return toolbarController;
    }

    @NonNull
    @Override
    public CarUiTextView createTextView(Context context, AttributeSet attrs) {
        return new CarUiTextViewImpl(context, attrs);
    }

    @Override
    public AppStyledViewController createAppStyledView(Context activityContext) {
        return new AppStyledViewControllerImpl(activityContext);
    }

    /**
     * InsetsUpdater waits for layout changes, and when there is one, calculates the appropriate
     * insets into the content view.
     *
     * <p>It then calls {@link InsetsChangedListener#onCarUiInsetsChanged(Insets)} on the
     * {@link Activity} and any {@link Fragment Fragments} the Activity might have. If none of the
     * Activity/Fragments implement {@link InsetsChangedListener}, it will set padding on the
     * content view equal to the insets.
     */
    public static final class InsetsUpdater {
        // These tags mark views that should overlay the content view in the base layout.
        // OEMs should add them to views in their base layout, ie: android:tag="car_ui_left_inset"
        // Apps will then be able to draw under these views, but will be encouraged to not put
        // any user-interactable content there.
        private static final String LEFT_INSET_TAG = "car_ui_left_inset";
        private static final String RIGHT_INSET_TAG = "car_ui_right_inset";
        private static final String TOP_INSET_TAG = "car_ui_top_inset";
        private static final String BOTTOM_INSET_TAG = "car_ui_bottom_inset";

        private final View mContentView;
        private final View mContentViewContainer; // Equivalent to mContentView except in Media
        private final View mLeftInsetView;
        private final View mRightInsetView;
        private final View mTopInsetView;
        private final View mBottomInsetView;
        private InsetsChangedListener mInsetsChangedListenerDelegate;

        @NonNull
        private Insets mInsets = new Insets();

        /**
         * Constructs an InsetsUpdater that calculates and dispatches insets to an {@link
         * Activity}.
         *
         * @param baseLayout  The root view of the base layout
         * @param contentView The android.R.id.content View
         */
        InsetsUpdater(
                @NonNull View baseLayout,
                @NonNull View contentView) {
            mContentView = contentView;
            mContentViewContainer = requireViewByRefId(baseLayout,
                    R.id.car_ui_base_layout_content_container);

            mLeftInsetView = baseLayout.findViewWithTag(LEFT_INSET_TAG);
            mRightInsetView = baseLayout.findViewWithTag(RIGHT_INSET_TAG);
            mTopInsetView = baseLayout.findViewWithTag(TOP_INSET_TAG);
            mBottomInsetView = baseLayout.findViewWithTag(BOTTOM_INSET_TAG);

            final View.OnLayoutChangeListener layoutChangeListener =
                    (View v, int left, int top, int right, int bottom,
                            int oldLeft, int oldTop, int oldRight, int oldBottom) -> {
                        if (left != oldLeft || top != oldTop
                                || right != oldRight || bottom != oldBottom) {
                            recalcInsets();
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

        public void replaceInsetsChangedListenerWith(InsetsChangedListener listener) {
            mInsetsChangedListenerDelegate = listener;
        }

        /**
         * Recalculate the amount of insets we need, and then dispatch them.
         */
        private void recalcInsets() {

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
            Insets insets = new Insets(left, top, right, bottom);

            if (!insets.equals(mInsets)) {
                mInsets = insets;
                dispatchNewInsets(insets);
            }
        }

        /**
         * Dispatch the new {@link Insets} to the {@link InsetsChangedListener} IIF there is one,
         * otherwise dispatch the new {@link Insets} to the {@link Activity} and all of its {@link
         * Fragment Fragments}. If none of those implement {@link InsetsChangedListener}, we will
         * set the value of the insets as padding on the content view.
         *
         * @param insets The newly-changed insets.
         */
        /* package */ void dispatchNewInsets(Insets insets) {
            mInsets = insets;


            if (mInsetsChangedListenerDelegate != null) {
                mInsetsChangedListenerDelegate.onCarUiInsetsChanged(insets);
            } else {
                mContentView.setPadding(insets.getLeft(), insets.getTop(),
                        insets.getRight(), insets.getBottom());
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

    @Override
    public CarUiRecyclerView createRecyclerView(Context context, AttributeSet attrs) {
        return new CarUiRecyclerViewImpl(context, attrs);
    }

    @Override
    public CarUiListItemAdapter createListItemAdapter(List<? extends CarUiListItem> items) {
        return new CarUiListItemAdapter(items);
    }
}
