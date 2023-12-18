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
package com.android.car.ui.toolbar;

import android.graphics.drawable.Drawable;
import android.view.View;

import androidx.annotation.NonNull;
import androidx.annotation.Nullable;

import com.android.car.ui.imewidescreen.CarUiImeSearchListItem;

import java.util.List;
import java.util.function.Consumer;
import java.util.function.Supplier;

/**
 * A class that implements ToolbarController but does nothing, just to easily get a non-null
 * ToolbarController
 */
public class EmptyToolbarController implements ToolbarController {
    @Override
    public void setTitle(int title) {

    }

    @Override
    public void setTitle(CharSequence title) {

    }

    @Override
    public CharSequence getTitle() {
        return null;
    }

    @Override
    public void setSubtitle(int title) {

    }

    @Override
    public void setSubtitle(CharSequence title) {

    }

    @Override
    public CharSequence getSubtitle() {
        return null;
    }

    @Override
    public void setTabs(@Nullable List<Tab> tabs) {

    }

    @Override
    public void setTabs(@Nullable List<Tab> tabs,
            int selectedTab) {

    }

    @Override
    public List<Tab> getTabs() {
        return null;
    }

    @Override
    public int getTabCount() {
        return 0;
    }

    @Override
    public int getTabPosition(TabLayout.Tab tab) {
        return 0;
    }

    @Override
    public void addTab(TabLayout.Tab tab) {

    }

    @Override
    public void clearAllTabs() {

    }

    @Override
    public TabLayout.Tab getTab(int position) {
        return null;
    }

    @Override
    public void selectTab(int position) {

    }

    @Override
    public int getSelectedTab() {
        return 0;
    }

    @Override
    public void setShowTabsInSubpage(boolean showTabs) {

    }

    @Override
    public boolean getShowTabsInSubpage() {
        return false;
    }

    @Override
    public void setLogo(int resId) {

    }

    @Override
    public void setLogo(Drawable drawable) {

    }

    @Override
    public void setSearchHint(int resId) {

    }

    @Override
    public void setSearchHint(CharSequence hint) {

    }

    @Override
    public CharSequence getSearchHint() {
        return null;
    }

    @Override
    public void setSearchIcon(int resId) {

    }

    @Override
    public void setSearchIcon(Drawable d) {

    }

    @Override
    public void setSearchMode(SearchMode mode) {

    }

    @Override
    public void setNavButtonMode(Toolbar.NavButtonMode style) {

    }

    @Override
    public void setNavButtonMode(NavButtonMode mode) {

    }

    @Override
    public Toolbar.NavButtonMode getNavButtonMode() {
        return null;
    }

    @Override
    public void setBackgroundShown(boolean shown) {

    }

    @Override
    public boolean getBackgroundShown() {
        return false;
    }

    @Override
    public void setMenuItems(@Nullable List<MenuItem> items) {

    }

    @Override
    public List<MenuItem> setMenuItems(int resId) {
        return null;
    }

    @NonNull
    @Override
    public List<MenuItem> getMenuItems() {
        return null;
    }

    @Nullable
    @Override
    public MenuItem findMenuItemById(int id) {
        return null;
    }

    @NonNull
    @Override
    public MenuItem requireMenuItemById(int id) {
        return null;
    }

    @Override
    public void setShowMenuItemsWhileSearching(boolean showMenuItems) {

    }

    @Override
    public boolean getShowMenuItemsWhileSearching() {
        return false;
    }

    @Override
    public void setSearchQuery(String query) {

    }

    @Override
    public void setState(Toolbar.State state) {

    }

    @Override
    public Toolbar.State getState() {
        return null;
    }

    @Override
    public boolean isStateSet() {
        return false;
    }

    @Override
    public void registerOnTabSelectedListener(Toolbar.OnTabSelectedListener listener) {

    }

    @Override
    public boolean unregisterOnTabSelectedListener(Toolbar.OnTabSelectedListener listener) {
        return false;
    }

    @Override
    public void registerOnSearchListener(Toolbar.OnSearchListener listener) {

    }

    @Override
    public boolean unregisterOnSearchListener(Toolbar.OnSearchListener listener) {
        return false;
    }

    @Override
    public void registerSearchListener(Consumer<String> listener) {

    }

    @Override
    public boolean unregisterSearchListener(Consumer<String> listener) {
        return false;
    }

    @Override
    public void setSearchConfig(@Nullable SearchConfig searchConfig) {

    }

    @Override
    public SearchCapabilities getSearchCapabilities() {
        return null;
    }

    @Override
    public boolean canShowSearchResultItems() {
        return false;
    }

    @Override
    public boolean canShowSearchResultsView() {
        return false;
    }

    @Override
    public void setSearchResultsView(@Nullable View view) {

    }

    @Override
    public void setSearchResultsInputViewIcon(@NonNull Drawable drawable) {

    }

    @Override
    public void setSearchResultItems(List<? extends CarUiImeSearchListItem> searchItems) {

    }

    @Override
    public void registerOnSearchCompletedListener(Toolbar.OnSearchCompletedListener listener) {

    }

    @Override
    public boolean unregisterOnSearchCompletedListener(Toolbar.OnSearchCompletedListener listener) {
        return false;
    }

    @Override
    public void registerSearchCompletedListener(Runnable listener) {

    }

    @Override
    public boolean unregisterSearchCompletedListener(Runnable listener) {
        return false;
    }

    @Override
    public void registerOnBackListener(Toolbar.OnBackListener listener) {

    }

    @Override
    public boolean unregisterOnBackListener(Toolbar.OnBackListener listener) {
        return false;
    }

    @Override
    public void registerBackListener(Supplier<Boolean> listener) {

    }

    @Override
    public boolean unregisterBackListener(Supplier<Boolean> listener) {
        return false;
    }

    @Override
    public ProgressBarController getProgressBar() {
        return null;
    }
}
