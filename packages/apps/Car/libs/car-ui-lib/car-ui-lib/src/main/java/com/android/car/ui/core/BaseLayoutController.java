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
package com.android.car.ui.core;

import static com.android.car.ui.utils.CarUiUtils.requireViewByRefId;

import android.app.Activity;
import android.content.res.TypedArray;
import android.view.View;

import androidx.annotation.NonNull;
import androidx.annotation.Nullable;
import androidx.fragment.app.Fragment;
import androidx.fragment.app.FragmentActivity;

import com.android.car.ui.R;
import com.android.car.ui.baselayout.Insets;
import com.android.car.ui.baselayout.InsetsChangedListener;
import com.android.car.ui.sharedlibrarysupport.SharedLibraryFactorySingleton;
import com.android.car.ui.toolbar.ToolbarController;

import java.util.Map;
import java.util.WeakHashMap;

/**
 * BaseLayoutController accepts an {@link Activity} and sets up the base layout inside of it.
 * It also exposes a {@link ToolbarController} to access the toolbar. This may be null if
 * used with a base layout without a Toolbar.
 */
public final class BaseLayoutController {

    private static final Map<Activity, BaseLayoutController> sBaseLayoutMap = new WeakHashMap<>();

    private InsetsUpdater mInsetsUpdater;

    /**
     * Gets a BaseLayoutController for the given {@link Activity}. Must have called
     * {@link #build(Activity)} with the same activity earlier, otherwise will return null.
     */
    @Nullable
    /* package */ static BaseLayoutController getBaseLayoutController(@Nullable Activity activity) {
        return sBaseLayoutMap.get(activity);
    }

    @Nullable
    private ToolbarController mToolbarController;

    private BaseLayoutController(Activity activity) {
        installBaseLayout(activity);
    }

    /**
     * Create a new BaseLayoutController for the given {@link Activity}.
     *
     * <p>You can get a reference to it by calling {@link #getBaseLayoutController(Activity)}.
     */
    /* package */
    static void build(Activity activity) {
        if (getThemeBoolean(activity, R.attr.carUiBaseLayout)) {
            sBaseLayoutMap.put(activity, new BaseLayoutController(activity));
        }
    }

    /**
     * Destroy the BaseLayoutController for the given {@link Activity}.
     */
    /* package */
    static void destroy(Activity activity) {
        sBaseLayoutMap.remove(activity);
    }

    /**
     * Gets the {@link ToolbarController} for activities created with carUiBaseLayout and
     * carUiToolbar set to true.
     */
    @Nullable
    /* package */ ToolbarController getToolbarController() {
        return mToolbarController;
    }

    /* package */ Insets getInsets() {
        return mInsetsUpdater.getInsets();
    }

    /* package */ void dispatchNewInsets(Insets insets) {
        mInsetsUpdater.onCarUiInsetsChanged(insets);
    }

    /* package */ void replaceInsetsChangedListenerWith(InsetsChangedListener listener) {
        mInsetsUpdater.replaceInsetsChangedListenerWith(listener);
    }

    /**
     * Installs the base layout into an activity, moving its content view under the base layout.
     *
     * <p>This function must be called during the onCreate() of the {@link Activity}.
     *
     * @param activity The {@link Activity} to install a base layout in.
     */
    private void installBaseLayout(Activity activity) {
        boolean toolbarEnabled = getThemeBoolean(activity, R.attr.carUiToolbar);

        View contentView =
                requireViewByRefId(activity.getWindow().getDecorView(), android.R.id.content);

        mInsetsUpdater = new InsetsUpdater(activity, contentView);
        mToolbarController = SharedLibraryFactorySingleton.get(activity)
                .installBaseLayoutAround(
                        contentView,
                        mInsetsUpdater,
                        toolbarEnabled,
                        true);
    }

    /**
     * Gets the boolean value of an Attribute from an {@link Activity Activity's}
     * {@link android.content.res.Resources.Theme}.
     */
    private static boolean getThemeBoolean(Activity activity, int attr) {
        TypedArray a = activity.getTheme().obtainStyledAttributes(new int[]{attr});

        try {
            return a.getBoolean(0, false);
        } finally {
            a.recycle();
        }
    }

    /**
     * InsetsUpdater waits for layout changes, and when there is one, calculates the appropriate
     * insets into the content view.
     *
     * <p>It then calls {@link InsetsChangedListener#onCarUiInsetsChanged(Insets)} on the
     * {@link Activity} and any {@link Fragment Fragments} the Activity might have. If
     * none of the Activity/Fragments implement {@link InsetsChangedListener}, it will set
     * padding on the content view equal to the insets.
     */
    public static final class InsetsUpdater implements InsetsChangedListener {
        private InsetsChangedListener mInsetsChangedListenerDelegate;

        @Nullable
        private Activity mActivity;
        private View mContentView;

        @NonNull
        private Insets mInsets = new Insets();

        /**
         * Constructs an InsetsUpdater that calculates and dispatches insets to an {@link Activity}.
         *
         * @param activity    The activity that is using base layouts. Used to dispatch insets to if
         *                    it implements {@link InsetsChangedListener}
         * @param contentView The android.R.id.content View
         */
        InsetsUpdater(
                @Nullable Activity activity,
                @NonNull View contentView) {
            mActivity = activity;
            mContentView = contentView;
        }

        @NonNull
        Insets getInsets() {
            return mInsets;
        }

        public void replaceInsetsChangedListenerWith(InsetsChangedListener listener) {
            mInsetsChangedListenerDelegate = listener;
        }

        @Override
        public void onCarUiInsetsChanged(@NonNull Insets insets) {
            if (mInsets.equals(insets)) {
                return;
            }

            mInsets = insets;

            boolean handled = false;
            if (mInsetsChangedListenerDelegate != null) {
                mInsetsChangedListenerDelegate.onCarUiInsetsChanged(insets);
                handled = true;
            } else {
                // If an explicit InsetsChangedListener is not provided,
                // pass the insets to activities and fragments
                if (mActivity instanceof InsetsChangedListener) {
                    ((InsetsChangedListener) mActivity).onCarUiInsetsChanged(insets);
                    handled = true;
                }

                if (mActivity instanceof FragmentActivity) {
                    for (Fragment fragment : ((FragmentActivity) mActivity)
                            .getSupportFragmentManager().getFragments()) {
                        if (fragment instanceof InsetsChangedListener) {
                            ((InsetsChangedListener) fragment).onCarUiInsetsChanged(insets);
                            handled = true;
                        }
                    }
                }
            }

            if (!handled) {
                mContentView.setPadding(insets.getLeft(), insets.getTop(),
                        insets.getRight(), insets.getBottom());
            }
        }
    }
}
