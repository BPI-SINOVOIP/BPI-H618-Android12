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

package com.android.car.ui.toolbar;

import static com.android.car.ui.utils.CarUiUtils.charSequenceToString;
import static com.android.car.ui.utils.CarUiUtils.convertList;

import static java.util.stream.Collectors.toList;

import android.app.Activity;
import android.content.Context;
import android.graphics.drawable.Drawable;
import android.text.TextUtils;
import android.util.Log;
import android.view.View;

import androidx.annotation.DrawableRes;
import androidx.annotation.IdRes;
import androidx.annotation.NonNull;
import androidx.annotation.Nullable;
import androidx.core.content.ContextCompat;

import com.android.car.ui.imewidescreen.CarUiImeSearchListItem;
import com.android.car.ui.sharedlibrary.oemapis.toolbar.ImeSearchInterfaceOEMV1;
import com.android.car.ui.sharedlibrary.oemapis.toolbar.ToolbarControllerOEMV1;
import com.android.car.ui.toolbar.Toolbar.OnBackListener;
import com.android.car.ui.toolbar.Toolbar.OnSearchCompletedListener;
import com.android.car.ui.toolbar.Toolbar.OnSearchListener;
import com.android.car.ui.toolbar.Toolbar.OnTabSelectedListener;
import com.android.car.ui.toolbar.Toolbar.State;
import com.android.car.ui.utils.CarUiUtils;

import java.util.ArrayList;
import java.util.Collections;
import java.util.HashSet;
import java.util.List;
import java.util.Objects;
import java.util.Set;
import java.util.function.Consumer;
import java.util.function.Supplier;

/**
 * Adapts a {@link com.android.car.ui.sharedlibrary.oemapis.toolbar.ToolbarControllerOEMV1}
 * into a {@link ToolbarController}
 */
@SuppressWarnings("AndroidJdkLibsChecker")
public final class ToolbarControllerAdapterV1 implements ToolbarController {

    private static final String TAG = ToolbarControllerAdapterV1.class.getName();

    private final ToolbarControllerOEMV1 mOemToolbar;
    private final Context mContext;

    private ToolbarAdapterState mAdapterState = new ToolbarAdapterState();
    private final Set<OnTabSelectedListener> mOnTabSelectedListeners = new HashSet<>();
    private final Set<OnBackListener> mDeprecatedBackListeners = new HashSet<>();
    private final Set<Supplier<Boolean>> mBackListeners = new HashSet<>();
    private final Set<OnSearchListener> mDeprecatedSearchListeners = new HashSet<>();
    private final Set<OnSearchCompletedListener> mDeprecatedSearchCompletedListeners =
            new HashSet<>();
    private final Set<Consumer<String>> mSearchListeners = new HashSet<>();
    private final Set<Runnable> mSearchCompletedListeners = new HashSet<>();
    private final ProgressBarControllerAdapterV1 mProgressBar;
    private String mSearchHint;
    private final SearchConfig.SearchConfigBuilder mSearchConfigBuilder;
    private List<MenuItem> mClientMenuItems = Collections.emptyList();
    private final List<DeprecatedTabWrapper> mDeprecatedTabs = new ArrayList<>();
    private final SearchWidescreenController mSearchWidescreenController;
    private final boolean mSupportsImeSearch;

    public ToolbarControllerAdapterV1(
            @NonNull Context context,
            @NonNull ToolbarControllerOEMV1 oemToolbar) {
        mOemToolbar = oemToolbar;
        mProgressBar = new ProgressBarControllerAdapterV1(mOemToolbar.getProgressBar());
        mContext = context;
        mSearchConfigBuilder = SearchConfig.builder();
        Activity activity = CarUiUtils.getActivity(mContext);

        mSearchWidescreenController = new SearchWidescreenController(context);
        ImeSearchInterfaceOEMV1 imeSearchInterface = mOemToolbar.getImeSearchInterface();
        mSupportsImeSearch = imeSearchInterface != null;
        if (imeSearchInterface != null) {
            imeSearchInterface.setOnPrivateImeCommandListener(
                    mSearchWidescreenController.getOnPrivateImeCommandListener());
            imeSearchInterface.setSearchTextViewConsumer(mSearchWidescreenController::setTextView);
        }

        oemToolbar.setBackListener(() -> {
            boolean handled = false;
            for (OnBackListener listener : mDeprecatedBackListeners) {
                handled |= listener.onBack();
            }
            for (Supplier<Boolean> listener : mBackListeners) {
                handled |= listener.get();
            }
            if (!handled && activity != null) {
                activity.onBackPressed();
            }
        });

        oemToolbar.setSearchListener(query -> {
            for (OnSearchListener listener : mDeprecatedSearchListeners) {
                listener.onSearch(query);
            }
            for (Consumer<String> listener : mSearchListeners) {
                listener.accept(query);
            }
        });
        oemToolbar.setSearchCompletedListener(() -> {
            for (OnSearchCompletedListener listener : mDeprecatedSearchCompletedListeners) {
                listener.onSearchCompleted();
            }
            for (Runnable listener : mSearchCompletedListeners) {
                listener.run();
            }
        });
    }

    @Override
    public void setTitle(int title) {
        setTitle(title == 0 ? null : mContext.getString(title));
    }

    @Override
    public void setTitle(CharSequence title) {
        update(mAdapterState.copy().setTitle(charSequenceToString(title)).build());
    }

    @Override
    public CharSequence getTitle() {
        return mAdapterState.getTitle();
    }

    @Override
    public void setSubtitle(int title) {
        setSubtitle(title == 0 ? null : mContext.getString(title));
    }

    @Override
    public void setSubtitle(CharSequence subtitle) {
        update(mAdapterState.copy().setSubtitle(charSequenceToString(subtitle)).build());
    }

    @Override
    public CharSequence getSubtitle() {
        return mAdapterState.getSubtitle();
    }

    @Override
    public void setTabs(@Nullable List<Tab> tabs) {
        setTabs(tabs, 0);
    }

    @Override
    public void setTabs(@Nullable List<Tab> tabs, int selectedTab) {
        mDeprecatedTabs.clear();
        if (tabs == null || tabs.isEmpty()) {
            selectedTab = 0;
        } else if (selectedTab < 0 || selectedTab >= tabs.size()) {
            throw new IllegalArgumentException("Tab position is invalid: " + selectedTab);
        }
        update(mAdapterState.copy()
                .setTabs(convertList(tabs, TabAdapterV1::new))
                .setSelectedTab(selectedTab)
                .build());
    }

    @Override
    public List<Tab> getTabs() {
        return Collections.unmodifiableList(mAdapterState.getTabs().stream()
                .map(TabAdapterV1::getClientTab)
                .collect(toList()));
    }

    @Override
    public int getTabCount() {
        return mDeprecatedTabs.size();
    }

    @Override
    public int getTabPosition(TabLayout.Tab tab) {
        for (int i = 0; i < mDeprecatedTabs.size(); i++) {
            if (mDeprecatedTabs.get(i).getDeprecatedTab() == tab) {
                return i;
            }
        }
        return -1;
    }

    @Override
    public void addTab(TabLayout.Tab clientTab) {
        mDeprecatedTabs.add(new DeprecatedTabWrapper(mContext, clientTab,
                this::updateModernTabsFromDeprecatedOnes, (tab) -> {
            for (Toolbar.OnTabSelectedListener listener : mOnTabSelectedListeners) {
                listener.onTabSelected(clientTab);
            }
        }));
        updateModernTabsFromDeprecatedOnes();
    }

    private void updateModernTabsFromDeprecatedOnes() {
        List<Tab> modernTabs = new ArrayList<>();

        for (DeprecatedTabWrapper tab : mDeprecatedTabs) {
            modernTabs.add(tab.getModernTab());
        }

        update(mAdapterState.copy().setTabs(convertList(modernTabs, TabAdapterV1::new)).build());
    }

    @Override
    public void clearAllTabs() {
        setTabs(Collections.emptyList());
    }

    @Override
    public TabLayout.Tab getTab(int position) {
        return mDeprecatedTabs.get(position).getDeprecatedTab();
    }

    @Override
    public void selectTab(int position) {
        if (position < 0 || position >= mAdapterState.getTabs().size()) {
            throw new IllegalArgumentException("Tab position is invalid: " + position);
        }
        update(mAdapterState.copy()
                .setSelectedTab(position)
                .build());
    }

    @Override
    public int getSelectedTab() {
        int selectedTab = mAdapterState.getSelectedTab();
        if (mAdapterState.getTabs().isEmpty() && selectedTab != -1) {
            throw new IllegalStateException("mSelectedTab should've been -1");
        }
        return selectedTab;
    }

    @Override
    public void setShowTabsInSubpage(boolean showTabs) {
        update(mAdapterState.copy().setShowTabsInSubpage(showTabs).build());
    }

    @Override
    public boolean getShowTabsInSubpage() {
        return mAdapterState.getShowTabsInSubpage();
    }

    @Override
    public void setLogo(@IdRes int resId) {
        setLogo(getDrawable(resId));
    }

    @Override
    public void setLogo(Drawable drawable) {
        update(mAdapterState.copy().setLogo(drawable).build());
    }

    @Override
    public void setSearchHint(int resId) {
        setSearchHint(mContext.getString(resId));
    }

    @Override
    public void setSearchHint(CharSequence hint) {
        mSearchHint = charSequenceToString(hint);
        mOemToolbar.setSearchHint(mSearchHint);
    }

    @Override
    public CharSequence getSearchHint() {
        return mSearchHint;
    }

    @Override
    public void setSearchIcon(int resId) {
        setSearchIcon(getDrawable(resId));
    }

    @Override
    public void setSearchIcon(Drawable d) {
        mOemToolbar.setSearchIcon(d);
    }

    @Override
    public void setSearchMode(SearchMode mode) {
        update(mAdapterState.copy().setSearchMode(mode).build());
    }

    @Override
    public void setNavButtonMode(Toolbar.NavButtonMode style) {
        switch (style) {
            case BACK:
                setNavButtonMode(NavButtonMode.BACK);
                break;
            case DOWN:
                setNavButtonMode(NavButtonMode.DOWN);
                break;
            case CLOSE:
                setNavButtonMode(NavButtonMode.CLOSE);
                break;
            case DISABLED:
            default:
                setNavButtonMode(NavButtonMode.DISABLED);
                break;
        }
    }

    @Override
    public void setNavButtonMode(NavButtonMode mode) {
        update(mAdapterState.copy().setNavButtonMode(mode).build());
    }

    @Override
    public Toolbar.NavButtonMode getNavButtonMode() {
        NavButtonMode mode = mAdapterState.getNavButtonMode();
        switch (mode) {
            case BACK:
                return Toolbar.NavButtonMode.BACK;
            case DOWN:
                return Toolbar.NavButtonMode.DOWN;
            case CLOSE:
                return Toolbar.NavButtonMode.CLOSE;
            case DISABLED:
            default:
                return Toolbar.NavButtonMode.DISABLED;
        }
    }

    @Override
    public void setBackgroundShown(boolean shown) {
        Log.w(TAG, "Unsupported operation setBackgroundShown() called, ignoring");
    }

    @Override
    public boolean getBackgroundShown() {
        return true;
    }

    @Override
    public void setMenuItems(@Nullable List<MenuItem> items) {
        mClientMenuItems = items;
        update(mAdapterState.copy()
                .setMenuItems(convertList(items, MenuItemAdapterV1::new))
                .build());
    }

    @Override
    public List<MenuItem> setMenuItems(int resId) {
        List<MenuItem> menuItems = MenuItemXmlParserUtil.readMenuItemList(mContext, resId);
        setMenuItems(menuItems);
        return menuItems;
    }

    @NonNull
    @Override
    public List<MenuItem> getMenuItems() {
        return mClientMenuItems;
    }

    @Nullable
    @Override
    public MenuItem findMenuItemById(int id) {
        for (MenuItem item : getMenuItems()) {
            if (item.getId() == id) {
                return item;
            }
        }
        return null;
    }

    @NonNull
    @Override
    public MenuItem requireMenuItemById(int id) {
        MenuItem result = findMenuItemById(id);

        if (result == null) {
            throw new IllegalArgumentException("ID does not reference a MenuItem on this Toolbar");
        }

        return result;
    }

    @Override
    public void setShowMenuItemsWhileSearching(boolean showMenuItems) {
        update(mAdapterState.copy().setShowMenuItemsWhileSearching(showMenuItems).build());
    }

    @Override
    public boolean getShowMenuItemsWhileSearching() {
        return mAdapterState.getShowMenuItemsWhileSearching();
    }

    @Override
    public void setSearchQuery(String query) {
        mOemToolbar.setSearchQuery(query);
    }

    @Override
    public void setState(State state) {
        update(mAdapterState.copy().setState(state).build());
    }

    /**
     * This method takes a new {@link ToolbarAdapterState} and compares it to the current
     * {@link #mAdapterState}. It then sends any differences it detects to the shared library
     * toolbar.
     *
     * This is also the core of the logic that adapts from the client's toolbar interface to
     * the OEM apis toolbar interface. For example, when you are in the HOME state and add tabs,
     * it will call setTitle(null) on the shared library toolbar. This is because the client
     * interface
     */
    private void update(ToolbarAdapterState newAdapterState) {
        ToolbarAdapterState oldAdapterState = mAdapterState;
        mAdapterState = newAdapterState;

        if (!TextUtils.equals(newAdapterState.getShownTitle(), oldAdapterState.getShownTitle())) {
            mOemToolbar.setTitle(newAdapterState.getTitle());
        }
        if (!TextUtils.equals(newAdapterState.getShownSubtitle(),
                oldAdapterState.getShownSubtitle())) {
            mOemToolbar.setSubtitle(newAdapterState.getSubtitle());
        }

        // This first check just checks if the logo is changing nullity, the second one checks
        // if it's actually a different image.
        if (newAdapterState.getShownLogo() != oldAdapterState.getShownLogo()) {
            mOemToolbar.setLogo(newAdapterState.getShownLogo());
        } else if (newAdapterState.getShownLogo() != null && newAdapterState.getLogoDirty()) {
            mOemToolbar.setLogo(newAdapterState.getShownLogo());
        }

        if (newAdapterState.getSearchMode() != oldAdapterState.getSearchMode()) {
            switch (newAdapterState.getSearchMode()) {
                case SEARCH:
                    mOemToolbar.setSearchMode(ToolbarControllerOEMV1.SEARCH_MODE_SEARCH);
                    break;
                case EDIT:
                    mOemToolbar.setSearchMode(ToolbarControllerOEMV1.SEARCH_MODE_EDIT);
                    break;
                default:
                    mOemToolbar.setSearchMode(ToolbarControllerOEMV1.SEARCH_MODE_DISABLED);
            }
        }

        if (oldAdapterState.getNavButtonMode() != newAdapterState.getNavButtonMode()) {
            if (newAdapterState.getNavButtonMode() == NavButtonMode.DISABLED) {
                mOemToolbar.setNavButtonMode(ToolbarControllerOEMV1.NAV_BUTTON_MODE_DISABLED);
            } else if (newAdapterState.getNavButtonMode() == NavButtonMode.CLOSE) {
                mOemToolbar.setNavButtonMode(ToolbarControllerOEMV1.NAV_BUTTON_MODE_CLOSE);
            } else if (newAdapterState.getNavButtonMode() == NavButtonMode.DOWN) {
                mOemToolbar.setNavButtonMode(ToolbarControllerOEMV1.NAV_BUTTON_MODE_DOWN);
            } else {
                mOemToolbar.setNavButtonMode(ToolbarControllerOEMV1.NAV_BUTTON_MODE_BACK);
            }
        }

        boolean gainingTabs = newAdapterState.hasTabs() && !oldAdapterState.hasTabs();
        boolean losingTabs = !newAdapterState.hasTabs() && oldAdapterState.hasTabs();
        if (gainingTabs) {
            mOemToolbar.setTabs(newAdapterState.getTabs()
                    .stream()
                    .map(TabAdapterV1::getSharedLibraryTab)
                    .collect(toList()),
                    newAdapterState.getSelectedTab());
        } else if (losingTabs) {
            mOemToolbar.setTabs(Collections.emptyList(), -1);
        } else if (newAdapterState.hasTabs() && newAdapterState.getTabsDirty()) {
            mOemToolbar.setTabs(newAdapterState.getTabs()
                            .stream()
                            .map(TabAdapterV1::getSharedLibraryTab)
                            .collect(toList()),
                    newAdapterState.getSelectedTab());
        } else if (newAdapterState.hasTabs()
                && newAdapterState.getSelectedTab() != oldAdapterState.getSelectedTab()) {
            mOemToolbar.selectTab(newAdapterState.getSelectedTab(), true);
        }

        if (!Objects.equals(
                newAdapterState.getShownMenuItems(), oldAdapterState.getShownMenuItems())) {
            mOemToolbar.setMenuItems(newAdapterState.getShownMenuItems());
        }
    }

    @Override
    public State getState() {
        return mAdapterState.getState();
    }

    @Override
    public boolean isStateSet() {
        return mAdapterState.isStateSet();
    }

    @Override
    public void registerOnTabSelectedListener(OnTabSelectedListener listener) {
        mOnTabSelectedListeners.add(listener);
    }

    @Override
    public boolean unregisterOnTabSelectedListener(OnTabSelectedListener listener) {
        return mOnTabSelectedListeners.remove(listener);
    }

    @Override
    public void registerOnSearchListener(OnSearchListener listener) {
        mDeprecatedSearchListeners.add(listener);
    }

    @Override
    public boolean unregisterOnSearchListener(OnSearchListener listener) {
        return mDeprecatedSearchListeners.remove(listener);
    }

    @Override
    public void registerSearchListener(Consumer<String> listener) {
        mSearchListeners.add(listener);
    }

    @Override
    public boolean unregisterSearchListener(Consumer<String> listener) {
        return mSearchListeners.remove(listener);
    }

    @Override
    public void setSearchConfig(SearchConfig searchConfig) {
        mSearchWidescreenController.setSearchConfig(searchConfig);
    }

    @Override
    public SearchCapabilities getSearchCapabilities() {
        if (!mSupportsImeSearch) {
            return SearchCapabilities.builder().build();
        } else {
            return mSearchWidescreenController.getSearchCapabilities();
        }
    }

    @Override
    public boolean canShowSearchResultItems() {
        return getSearchCapabilities().canShowSearchResultItems();
    }

    @Override
    public boolean canShowSearchResultsView() {
        return getSearchCapabilities().canShowSearchResultsView();
    }

    @Override
    public void setSearchResultsView(View view) {
        mSearchConfigBuilder.setSearchResultsView(view);
        setSearchConfig(mSearchConfigBuilder.build());
    }

    @Override
    public void setSearchResultsInputViewIcon(Drawable drawable) {
        mSearchConfigBuilder.setSearchResultsInputViewIcon(drawable);
        setSearchConfig(mSearchConfigBuilder.build());
    }

    @Override
    public void setSearchResultItems(List<? extends CarUiImeSearchListItem> searchItems) {
        mSearchConfigBuilder.setSearchResultItems(searchItems);
        setSearchConfig(mSearchConfigBuilder.build());
    }

    @Override
    public void registerOnSearchCompletedListener(OnSearchCompletedListener listener) {
        mDeprecatedSearchCompletedListeners.add(listener);
    }

    @Override
    public boolean unregisterOnSearchCompletedListener(OnSearchCompletedListener listener) {
        return mDeprecatedSearchCompletedListeners.remove(listener);
    }

    @Override
    public void registerSearchCompletedListener(Runnable listener) {
        mSearchCompletedListeners.add(listener);
    }

    @Override
    public boolean unregisterSearchCompletedListener(Runnable listener) {
        return mSearchCompletedListeners.remove(listener);
    }

    @Override
    public void registerOnBackListener(OnBackListener listener) {
        mDeprecatedBackListeners.add(listener);
    }

    @Override
    public boolean unregisterOnBackListener(OnBackListener listener) {
        return mDeprecatedBackListeners.remove(listener);
    }

    @Override
    public void registerBackListener(Supplier<Boolean> listener) {
        mBackListeners.add(listener);
    }

    @Override
    public boolean unregisterBackListener(Supplier<Boolean> listener) {
        return mBackListeners.remove(listener);
    }

    @Override
    public ProgressBarController getProgressBar() {
        return mProgressBar;
    }

    private Drawable getDrawable(@DrawableRes int drawable) {
        if (drawable == 0) {
            return null;
        } else {
            return ContextCompat.getDrawable(mContext, drawable);
        }
    }

    private static class ToolbarAdapterState {
        private final State mState;
        private final boolean mStateSet;
        private final boolean mShowTabsInSubpage;
        @NonNull
        private final List<TabAdapterV1> mTabs;
        @NonNull
        private final List<MenuItemAdapterV1> mMenuItems;
        private final int mSelectedTab;
        private final String mTitle;
        private final String mSubtitle;
        private final Drawable mLogo;
        private final boolean mShowMenuItemsWhileSearching;
        private final boolean mTabsDirty;
        private final boolean mLogoDirty;
        private final NavButtonMode mNavButtonMode;
        private final SearchMode mSearchMode;

        ToolbarAdapterState() {
            mState = State.HOME;
            mStateSet = false;
            mShowTabsInSubpage = false;
            mTabs = Collections.emptyList();
            mMenuItems = Collections.emptyList();
            mSelectedTab = -1;
            mTitle = null;
            mSubtitle = null;
            mLogo = null;
            mShowMenuItemsWhileSearching = false;
            mTabsDirty = false;
            mLogoDirty = false;
            mNavButtonMode = NavButtonMode.DISABLED;
            mSearchMode = SearchMode.DISABLED;
        }

        private ToolbarAdapterState(Builder builder) {
            mState = builder.mState;
            mStateSet = builder.mStateSet;
            mShowTabsInSubpage = builder.mShowTabsInSubpage;
            mTabs = builder.mTabs;
            mMenuItems = builder.mMenuItems;
            mSelectedTab = builder.mSelectedTab;
            mTitle = builder.mTitle;
            mSubtitle = builder.mSubtitle;
            mLogo = builder.mLogo;
            mShowMenuItemsWhileSearching = builder.mShowMenuItemsWhileSearching;
            mTabsDirty = builder.mTabsDirty;
            mLogoDirty = builder.mLogoDirty;
            mNavButtonMode = builder.mNavButtonMode;
            mSearchMode = builder.mSearchMode;
        }

        public State getState() {
            return mState;
        }

        public boolean isStateSet() {
            return mStateSet;
        }

        public boolean getShowTabsInSubpage() {
            return mShowTabsInSubpage;
        }

        @NonNull
        public List<TabAdapterV1> getTabs() {
            return mTabs;
        }

        public boolean getTabsDirty() {
            return mTabsDirty;
        }

        public int getSelectedTab() {
            return mSelectedTab;
        }

        public String getTitle() {
            return mTitle == null ? "" : mTitle;
        }

        public String getShownTitle() {
            if (mStateSet && (mState != State.HOME && mState != State.SUBPAGE)) {
                return "";
            }
            return mTitle == null ? "" : mTitle;
        }

        public String getSubtitle() {
            return mSubtitle == null ? "" : mSubtitle;
        }

        public String getShownSubtitle() {
            if (mStateSet && (mState != State.HOME && mState != State.SUBPAGE)) {
                return "";
            }
            return mSubtitle == null ? "" : mSubtitle;
        }

        public Drawable getLogo() {
            return mLogo;
        }

        public Drawable getShownLogo() {
            if (mStateSet && (mState != State.HOME && mState != State.SUBPAGE)) {
                return null;
            }
            return mLogo;
        }

        public boolean getLogoDirty() {
            return mLogoDirty;
        }

        public boolean getShowMenuItemsWhileSearching() {
            return mShowMenuItemsWhileSearching;
        }

        public NavButtonMode getNavButtonMode() {
            if (mStateSet && mNavButtonMode == NavButtonMode.DISABLED
                    && mState != State.HOME) {
                return NavButtonMode.BACK;
            }
            return mNavButtonMode;
        }

        private boolean hasTabs() {
            if (!mStateSet) {
                return !getTabs().isEmpty();
            }
            return (mState == State.HOME || (mState == State.SUBPAGE && getShowTabsInSubpage()))
                    && !getTabs().isEmpty();
        }

        private List<MenuItemAdapterV1> getShownMenuItems() {
            SearchMode searchMode = getSearchMode();
            if (searchMode == SearchMode.EDIT) {
                return mShowMenuItemsWhileSearching ? mMenuItems : Collections.emptyList();
            } else if (searchMode == SearchMode.SEARCH) {
                return mShowMenuItemsWhileSearching
                        ? mMenuItems.stream().filter(i -> !i.isSearch()).collect(toList())
                        : Collections.emptyList();
            } else {
                return mMenuItems;
            }
        }

        private SearchMode getSearchMode() {
            if (mStateSet) {
                if (mState == State.SEARCH) {
                    return SearchMode.SEARCH;
                } else if (mState == State.EDIT) {
                    return SearchMode.EDIT;
                } else {
                    return SearchMode.DISABLED;
                }
            }
            return mSearchMode;
        }

        public Builder copy() {
            return new Builder(this);
        }

        public static class Builder {
            private final ToolbarAdapterState mStateClonedFrom;
            private boolean mWasChanged = false;
            private State mState;
            private boolean mStateSet;
            private boolean mShowTabsInSubpage;
            @NonNull
            private List<TabAdapterV1> mTabs;
            @NonNull
            private List<MenuItemAdapterV1> mMenuItems;
            private int mSelectedTab;
            private String mTitle;
            private String mSubtitle;
            private Drawable mLogo;
            private boolean mShowMenuItemsWhileSearching;
            private boolean mTabsDirty = false;
            private boolean mLogoDirty = false;
            private NavButtonMode mNavButtonMode;
            private SearchMode mSearchMode;

            Builder(ToolbarAdapterState state) {
                mStateClonedFrom = state;
                mState = state.mState;
                mStateSet = state.mStateSet;
                mShowTabsInSubpage = state.mShowTabsInSubpage;
                mTabs = state.mTabs;
                mMenuItems = state.mMenuItems;
                mShowMenuItemsWhileSearching = state.mShowMenuItemsWhileSearching;
                mSelectedTab = state.mSelectedTab;
                mTitle = state.mTitle;
                mSubtitle = state.mSubtitle;
                mLogo = state.mLogo;
                mNavButtonMode = state.mNavButtonMode;
                mSearchMode = state.mSearchMode;
            }

            public ToolbarAdapterState build() {
                if (!mWasChanged) {
                    return mStateClonedFrom;
                } else {
                    return new ToolbarAdapterState(this);
                }
            }

            public Builder setState(State state) {
                if (mSearchMode != SearchMode.DISABLED) {
                    throw new IllegalStateException("Cannot use setSearchMode() with setState()");
                }
                if (state != mState) {
                    mState = state;
                    mStateSet = true;
                    mWasChanged = true;
                }
                return this;
            }

            public Builder setShowTabsInSubpage(boolean showTabsInSubpage) {
                if (mShowTabsInSubpage != showTabsInSubpage) {
                    mShowTabsInSubpage = showTabsInSubpage;
                    mWasChanged = true;
                }
                return this;
            }

            public Builder setTabs(
                    @Nullable List<TabAdapterV1> tabs) {
                if (tabs == null) {
                    tabs = Collections.emptyList();
                }
                if (!Objects.equals(tabs, mTabs)) {
                    mTabs = Collections.unmodifiableList(tabs);
                    mSelectedTab = mTabs.isEmpty() ? -1 : 0;
                    mWasChanged = true;
                    mTabsDirty = true;
                }
                return this;
            }

            public Builder addTab(@NonNull TabAdapterV1 tab) {
                List<TabAdapterV1> newTabs = new ArrayList<>(mTabs);
                newTabs.add(tab);
                mTabs = Collections.unmodifiableList(newTabs);
                mWasChanged = true;
                mTabsDirty = true;
                return this;
            }

            public Builder setSelectedTab(int selectedTab) {
                if (mSelectedTab != selectedTab) {
                    mSelectedTab = selectedTab;
                    mWasChanged = true;
                }
                return this;
            }

            public Builder setTitle(String title) {
                if (!Objects.equals(mTitle, title)) {
                    mTitle = title;
                    mWasChanged = true;
                }
                return this;
            }

            public Builder setSubtitle(String subtitle) {
                if (!Objects.equals(mSubtitle, subtitle)) {
                    mSubtitle = subtitle;
                    mWasChanged = true;
                }
                return this;
            }

            public Builder setLogo(Drawable logo) {
                if (mLogo != logo) {
                    mLogo = logo;
                    mWasChanged = true;
                    mLogoDirty = true;
                }
                return this;
            }

            public Builder setShowMenuItemsWhileSearching(boolean showMenuItemsWhileSearching) {
                if (mShowMenuItemsWhileSearching != showMenuItemsWhileSearching) {
                    mShowMenuItemsWhileSearching = showMenuItemsWhileSearching;
                    mWasChanged = true;
                }
                return this;
            }

            public Builder setMenuItems(List<MenuItemAdapterV1> menuItems) {
                if (menuItems == null) {
                    menuItems = Collections.emptyList();
                }

                if (!Objects.equals(mMenuItems, menuItems)) {
                    mMenuItems = menuItems;
                    mWasChanged = true;
                }
                return this;
            }

            public Builder setNavButtonMode(NavButtonMode newMode) {
                if (newMode != mNavButtonMode) {
                    mNavButtonMode = newMode;
                    mWasChanged = true;
                }
                return this;
            }

            public Builder setSearchMode(SearchMode searchMode) {
                if (mStateSet) {
                    throw new IllegalStateException("Cannot use setSearchMode() with setState()");
                }
                if (mSearchMode != searchMode) {
                    mSearchMode = searchMode;
                    mWasChanged = true;
                }
                return this;
            }
        }
    }
}
