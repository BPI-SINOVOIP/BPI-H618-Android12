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
import android.util.AttributeSet;
import android.view.LayoutInflater;
import android.view.View;
import android.widget.ImageView;
import android.widget.LinearLayout;
import android.widget.TextView;

import androidx.annotation.NonNull;
import androidx.annotation.Nullable;

import com.android.car.ui.sharedlibrary.oemapis.toolbar.TabOEMV1;

import com.chassis.car.ui.sharedlibrary.R;

import java.util.ArrayList;
import java.util.HashMap;
import java.util.List;
import java.util.Map;

/**
 * A view that can show tabs via the {@link #setTabs(List, int)} method.
 */
public class TabLayout extends LinearLayout {
    private List<? extends TabOEMV1> mTabs = new ArrayList<>();
    private TabOEMV1 mSelectedTab;
    private final Map<TabOEMV1, View> mTabViews = new HashMap<>();

    public TabLayout(Context context) {
        super(context);
    }

    public TabLayout(Context context, @Nullable AttributeSet attrs) {
        super(context, attrs);
    }

    public TabLayout(Context context, @Nullable AttributeSet attrs, int defStyleAttr) {
        super(context, attrs, defStyleAttr);
    }

    /**
     * Sets a list of tabs to show.
     *
     * @param tabs The tabs to show.
     * @param selectedTab Which tab is selected.
     */
    public void setTabs(@NonNull List<? extends TabOEMV1> tabs, int selectedTab) {
        removeAllViews();
        mTabViews.clear();
        mSelectedTab = null;
        mTabs = tabs;

        if (tabs.size() > 0) {
            mSelectedTab = tabs.get(selectedTab);
            for (TabOEMV1 tab : tabs) {
                View view = LayoutInflater.from(getContext())
                        .inflate(R.layout.toolbar_tab, this, false);
                addView(view);
                mTabViews.put(tab, view);
                bindTab(tab);
            }
        }
    }

    private void bindTab(TabOEMV1 tab) {
        View view = mTabViews.get(tab);
        if (view == null) {
            return;
        }

        ImageView iconView = view.requireViewById(R.id.car_ui_toolbar_tab_item_icon);
        TextView textView = view.requireViewById(R.id.car_ui_toolbar_tab_item_text);

        view.setOnClickListener(v -> selectTab(tab, true));
        iconView.setImageDrawable(tab.getIcon());
        textView.setText(tab.getTitle());

        boolean selected = tab == mSelectedTab;
        view.setActivated(selected);
        textView.setTextAppearance(selected
                ? R.style.TextAppearance_CarUi_Widget_Toolbar_Tab_Selected
                : R.style.TextAppearance_CarUi_Widget_Toolbar_Tab);
    }

    /**
     * Selects a particular tab.
     */
    private void selectTab(TabOEMV1 tab, boolean sendCallback) {
        if (mSelectedTab == tab) {
            return;
        }

        TabOEMV1 oldSelectedTab = mSelectedTab;
        mSelectedTab = tab;
        bindTab(oldSelectedTab);
        bindTab(mSelectedTab);

        Runnable onSelectedListener = tab.getOnSelectedListener();
        if (onSelectedListener != null && sendCallback) {
            onSelectedListener.run();
        }
    }

    /**
     * Selects a particular tab by its position in the list.
     */
    public void selectTab(int position, boolean sendCallback) {
        selectTab(mTabs.get(position), sendCallback);
    }

    /**
     * Returns if this TabLayout has at least one tab.
     */
    public boolean hasTabs() {
        return mTabs != null && mTabs.size() > 0;
    }
}
