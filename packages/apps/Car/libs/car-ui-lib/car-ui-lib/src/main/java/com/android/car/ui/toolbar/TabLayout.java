/*
 * Copyright (C) 2019 The Android Open Source Project
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

import static com.android.car.ui.utils.CarUiUtils.requireViewByRefId;

import android.content.Context;
import android.content.res.Resources;
import android.graphics.drawable.Drawable;
import android.util.AttributeSet;
import android.view.LayoutInflater;
import android.view.View;
import android.widget.ImageView;
import android.widget.LinearLayout;
import android.widget.TextView;

import androidx.annotation.LayoutRes;
import androidx.annotation.NonNull;
import androidx.annotation.Nullable;

import com.android.car.ui.R;

import java.util.ArrayList;
import java.util.Collections;
import java.util.List;
import java.util.function.Consumer;

/**
 * Custom tab layout which supports adding tabs dynamically
 *
 * <p>It supports two layout modes:
 * <ul><li>Flexible layout which will fill the width
 * <li>Non-flexible layout which wraps content with a minimum tab width. By setting tab gravity,
 * it can left aligned, right aligned or center aligned.
 *
 * <p>Scrolling function is not supported. If a tab item runs out of the tab layout bound, there
 * is no way to access it. It's better to set the layout mode to flexible in this case.
 *
 * <p>Default tab item inflates from R.layout.car_ui_tab_item, but it also supports custom layout
 * id, by overlaying R.layout.car_ui_tab_item_layout. By doing this, appearance of tab item view
 * can be customized.
 *
 * <p>Touch feedback is using @android:attr/selectableItemBackground.
 */
@SuppressWarnings("AndroidJdkLibsChecker")
public class TabLayout extends LinearLayout {
    @LayoutRes
    private final int mTabLayoutRes;
    @NonNull
    private List<com.android.car.ui.toolbar.Tab> mTabs = Collections.emptyList();
    private int mSelectedTab = -1;

    public TabLayout(@NonNull Context context) {
        this(context, null);
    }

    public TabLayout(@NonNull Context context, @Nullable AttributeSet attrs) {
        this(context, attrs, 0);
    }

    public TabLayout(@NonNull Context context, @Nullable AttributeSet attrs, int defStyle) {
        super(context, attrs, defStyle);
        Resources resources = context.getResources();

        boolean tabFlexibleLayout = resources.getBoolean(R.bool.car_ui_toolbar_tab_flexible_layout);
        mTabLayoutRes = tabFlexibleLayout
                ? R.layout.car_ui_toolbar_tab_item_layout_flexible
                : R.layout.car_ui_toolbar_tab_item_layout;
    }

    /** Sets the tabs to show */
    public void setTabs(List<com.android.car.ui.toolbar.Tab> tabs, int selectedTab) {
        if (tabs == null) {
            mTabs = Collections.emptyList();
        } else {
            mTabs = Collections.unmodifiableList(new ArrayList<>(tabs));
        }
        mSelectedTab = selectedTab;
        recreateViews();
    }

    public List<com.android.car.ui.toolbar.Tab> getTabs() {
        return mTabs;
    }

    /** Returns the currently selected tab, or -1 if no tabs exist */
    public int getSelectedTab() {
        if (mTabs.isEmpty() && mSelectedTab != -1) {
            throw new IllegalStateException("mSelectedTab should've been -1");
        }
        return mSelectedTab;
    }

    /**
     * Returns if this TabLayout has tabs. That is, if the most recent call to
     * {@link #setTabs(List)} contained a non-empty list.
     */
    public boolean hasTabs() {
        return !mTabs.isEmpty();
    }

    /** Set the tab at given position as the current selected tab. */
    public void selectTab(int position) {
        if (position < 0 || position > mTabs.size()) {
            position = mTabs.isEmpty() ? -1 : 0;
        }
        if (position == mSelectedTab) {
            return;
        }

        int oldPosition = mSelectedTab;
        mSelectedTab = position;
        presentTabView(oldPosition);
        presentTabView(position);

        if (position >= 0) {
            com.android.car.ui.toolbar.Tab tab = mTabs.get(position);
            Consumer<com.android.car.ui.toolbar.Tab> listener = tab.getSelectedListener();
            if (listener != null) {
                listener.accept(tab);
            }
        }
    }

    private void recreateViews() {
        removeAllViews();
        for (int i = 0; i < mTabs.size(); i++) {
            View tabView = LayoutInflater.from(getContext())
                    .inflate(mTabLayoutRes, this, false);
            addView(tabView);
            presentTabView(i);
        }
    }

    private void presentTabView(int position) {
        if (position < 0 || position > mTabs.size()) {
            return;
        }
        View tabView = getChildAt(position);
        com.android.car.ui.toolbar.Tab tab = mTabs.get(position);
        ImageView iconView = requireViewByRefId(tabView, R.id.car_ui_toolbar_tab_item_icon);
        TextView textView = requireViewByRefId(tabView, R.id.car_ui_toolbar_tab_item_text);

        tabView.setOnClickListener(view -> selectTab(position));
        textView.setText(tab.getText());
        iconView.setImageDrawable(tab.getIcon());
        tabView.setActivated(position == mSelectedTab);
        textView.setTextAppearance(position == mSelectedTab
                ? R.style.TextAppearance_CarUi_Widget_Toolbar_Tab_Selected
                : R.style.TextAppearance_CarUi_Widget_Toolbar_Tab);
    }

    /**
     * Tab entity.
     *
     * @deprecated Use {@link com.android.car.ui.toolbar.Tab} instead.
     */
    @Deprecated
    public static class Tab {
        private final Drawable mIcon;
        private final CharSequence mText;
        private boolean mIsSelected;

        public Tab(@Nullable Drawable icon, @Nullable CharSequence text) {
            mIcon = icon;
            mText = text;
        }

        /** Set tab text. */
        protected void bindText(TextView textView) {
            textView.setText(mText);
        }

        /**
         * Do not use, this method is here for the shared library adapters, which cannot
         * call the protected version due to being in a different classloader.
         */
        public final void bindTextPublic(TextView textView) {
            bindText(textView);
        }

        /** Set icon drawable. TODO(b/139444064): revise this api.*/
        protected void bindIcon(ImageView imageView) {
            imageView.setImageDrawable(mIcon);
        }

        /**
         * Do not use, this method is here for the shared library adapters, which cannot
         * call the protected version due to being in a different classloader.
         */
        public final void bindIconPublic(ImageView imageView) {
            bindIcon(imageView);
        }
    }
}
