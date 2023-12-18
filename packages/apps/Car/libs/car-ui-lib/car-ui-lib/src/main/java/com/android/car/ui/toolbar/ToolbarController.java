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

import android.graphics.drawable.Drawable;
import android.view.View;

import androidx.annotation.DrawableRes;
import androidx.annotation.NonNull;
import androidx.annotation.Nullable;
import androidx.annotation.StringRes;
import androidx.annotation.XmlRes;

import com.android.car.ui.imewidescreen.CarUiImeSearchListItem;
import com.android.car.ui.recyclerview.CarUiListItem;
import com.android.car.ui.toolbar.SearchConfig.SearchConfigBuilder;

import java.util.List;
import java.util.function.Consumer;
import java.util.function.Supplier;

/**
 * An interface for accessing a Chassis Toolbar, regardless of how the underlying
 * views are represented.
 */
@SuppressWarnings("AndroidJdkLibsChecker")
public interface ToolbarController {

    /**
     * Sets the title of the toolbar to a string resource.
     *
     * <p>The title may not always be shown, for example with one row layout with tabs.
     */
    void setTitle(@StringRes int title);

    /**
     * Sets the title of the toolbar to a CharSequence.
     *
     * <p>The title may not always be shown, for example with one row layout with tabs.
     */
    void setTitle(CharSequence title);

    /**
     * Gets the current toolbar title.
     */
    CharSequence getTitle();

    /**
     * Sets the subtitle of the toolbar to a string resource.
     *
     * <p>The title may not always be shown, for example with one row layout with tabs.
     */
    void setSubtitle(@StringRes int title);

    /**
     * Sets the subtitle of the toolbar to a CharSequence.
     *
     * <p>The title may not always be shown, for example with one row layout with tabs.
     */
    void setSubtitle(CharSequence title);

    /**
     * Gets the current toolbar subtitle.
     */
    CharSequence getSubtitle();

    /**
     * Sets the tabs to display.
     *
     * @param tabs A list of {@link Tab}
     * @see #setTabs(List, int)
     */
    void setTabs(@Nullable List<Tab> tabs);

    /**
     * Sets the tabs to display. This version will also take the index of a tab to start selected.
     * This will not cause a callback to the tab's selected listener, unlike using
     * {@link #setTabs(List)} followed by {@link #selectTab(int)}.
     *
     * @param tabs A list of {@link Tab}
     * @param selectedTab The index of the tab to be initially selected
     */
    void setTabs(@Nullable List<Tab> tabs, int selectedTab);

    /**
     * Gets the list of tabs being displayed. The returned list will be unmodifiable.
     */
    List<Tab> getTabs();

    /**
     * Gets the number of tabs in the toolbar. The tabs can be retrieved using
     * {@link #getTab(int)}.
     *
     * @deprecated Use {@link #getTabs getTabs().size()} instead.
     */
    @Deprecated
    int getTabCount();

    /**
     * Gets the index of the tab.
     *
     * @deprecated Use {@link #getTabs getTabs().indexOf(tab)} instead.
     */
    @Deprecated
    int getTabPosition(TabLayout.Tab tab);

    /**
     * Adds a tab to this toolbar. You can listen for when it is selected via
     * {@link #registerOnTabSelectedListener(Toolbar.OnTabSelectedListener)}.
     *
     * @deprecated Use {@link #setTabs(List)} instead.
     */
    @Deprecated
    void addTab(TabLayout.Tab tab);

    /**
     * Removes all the tabs.
     *
     * @deprecated Use {{@link #setTabs(List)}} instead, passing it
     * {@link java.util.Collections#emptyList()}
     */
    @Deprecated
    void clearAllTabs();

    /**
     * Gets a tab added to this toolbar. See
     * {@link #addTab(TabLayout.Tab)}.
     *
     * @deprecated Use {@link #getTabs getTabs().get(position)} instead.
     */
    @Deprecated
    TabLayout.Tab getTab(int position);

    /**
     * Selects a tab added to this toolbar.
     *
     * @see #setTabs(List)
     */
    void selectTab(int position);

    /**
     * Returns the index of the currently selected tab. If there are no tabs, this method returns
     * -1.
     */
    int getSelectedTab();

    /**
     * Sets whether or not tabs should also be shown in the SUBPAGE {@link Toolbar.State}.
     *
     * @deprecated When not using {@link #setState(Toolbar.State)}, this method is not necessary.
     * Simply add tabs to the toolbar when they should be shown.
     */
    @Deprecated
    void setShowTabsInSubpage(boolean showTabs);

    /**
     * Gets whether or not tabs should also be shown in the SUBPAGE {@link Toolbar.State}.
     *
     * @deprecated When not using {@link #setState(Toolbar.State)},
     * {@link #setShowTabsInSubpage(boolean)} is not necessary.
     * Simply add tabs to the toolbar when they should be shown.
     */
    @Deprecated
    boolean getShowTabsInSubpage();

    /**
     * Sets the logo to display in this toolbar. If navigation icon is being displayed, this logo
     * will be displayed next to the title.
     */
    void setLogo(@DrawableRes int resId);

    /**
     * Sets the logo to display in this toolbar. If navigation icon is being displayed, this logo
     * will be displayed next to the title.
     */
    void setLogo(Drawable drawable);

    /** Sets the hint for the search bar. */
    void setSearchHint(@StringRes int resId);

    /** Sets the hint for the search bar. */
    void setSearchHint(CharSequence hint);

    /** Gets the search hint */
    CharSequence getSearchHint();

    /**
     * Sets the icon to display in the search box.
     *
     * <p>The icon will be lost on configuration change, make sure to set it in onCreate() or
     * a similar place.
     */
    void setSearchIcon(@DrawableRes int resId);

    /**
     * Sets the icon to display in the search box.
     *
     * <p>The icon will be lost on configuration change, make sure to set it in onCreate() or
     * a similar place.
     */
    void setSearchIcon(Drawable d);

    /**
     * Sets the search mode, which can enable/disable the search bar in the toolbar.
     *
     * See {@link SearchMode}.
     */
    void setSearchMode(SearchMode mode);

    /**
     * Sets the {@link Toolbar.NavButtonMode}
     *
     * @deprecated Use {@link #setNavButtonMode(NavButtonMode)} instead.
     */
    @Deprecated
    void setNavButtonMode(Toolbar.NavButtonMode style);

    /** Sets the {@link NavButtonMode} */
    void setNavButtonMode(NavButtonMode mode);

    /**
     * Gets the {@link Toolbar.NavButtonMode}.
     *
     * @deprecated No equivalent replacement. Maintain the current mode locally in the app if
     * needed. If this is for a test, you can use this espresso code to check that the toolbar
     * back button is shown:
     * {@code onView(withContentDescription("Back")).check(matches(isDisplayed()));}
     */
    @Deprecated
    Toolbar.NavButtonMode getNavButtonMode();

    /** Show/hide the background. When hidden, the toolbar is completely transparent. */
    void setBackgroundShown(boolean shown);

    /** Returns true is the toolbar background is shown */
    boolean getBackgroundShown();

    /**
     * Sets the {@link MenuItem Menuitems} to display.
     */
    void setMenuItems(@Nullable List<MenuItem> items);

    /**
     * Sets the {@link MenuItem Menuitems} to display to a list defined in XML.
     *
     * <p>If this method is called twice with the same argument (and {@link #setMenuItems(List)}
     * wasn't called), nothing will happen the second time, even if the MenuItems were changed.
     *
     * <p>The XML file must have one <MenuItems> tag, with a variable number of <MenuItem>
     * child tags. See CarUiToolbarMenuItem in CarUi's attrs.xml for a list of available attributes.
     *
     * Example:
     * <pre>
     * <MenuItems>
     *     <MenuItem
     *         app:title="Foo"/>
     *     <MenuItem
     *         app:title="Bar"
     *         app:icon="@drawable/ic_tracklist"
     *         app:onClick="xmlMenuItemClicked"/>
     *     <MenuItem
     *         app:title="Bar"
     *         app:checkable="true"
     *         app:uxRestrictions="FULLY_RESTRICTED"
     *         app:onClick="xmlMenuItemClicked"/>
     * </MenuItems>
     * </pre>
     *
     * @return The MenuItems that were loaded from XML.
     * @see #setMenuItems(List)
     */
    List<MenuItem> setMenuItems(@XmlRes int resId);

    /** Gets the {@link MenuItem MenuItems} currently displayed */
    @NonNull
    List<MenuItem> getMenuItems();

    /** Gets a {@link MenuItem} by id. */
    @Nullable
    MenuItem findMenuItemById(int id);

    /** Gets a {@link MenuItem} by id. Will throw an IllegalArgumentException if not found. */
    @NonNull
    MenuItem requireMenuItemById(int id);

    /**
     * Set whether or not to show the {@link MenuItem MenuItems} while searching. Default false.
     * Even if this is set to true, the {@link MenuItem} created by
     * {@link MenuItem.Builder#setToSearch()} will still be hidden.
     */
    void setShowMenuItemsWhileSearching(boolean showMenuItems);

    /** Returns if {@link MenuItem MenuItems} are shown while searching */
    boolean getShowMenuItemsWhileSearching();

    /**
     * Sets the search query.
     */
    void setSearchQuery(String query);

    /**
     * Sets the state of the toolbar. This will show/hide the appropriate elements of the toolbar
     * for the desired state.
     *
     * @deprecated Instead of using setState(), simply add the elements you want to see when
     * you want to see them. The back button visibility can be controlled with the
     * {@link com.android.car.ui.toolbar.Toolbar.NavButtonMode#DISABLED} enum,
     * and the search bar can also be controlled with the {@link SearchMode} enum.
     */
    @Deprecated
    void setState(Toolbar.State state);

    /**
     * Gets the current {@link Toolbar.State} of the toolbar.
     *
     * @deprecated See {@link #setState(Toolbar.State)} for details.
     */
    @Deprecated
    Toolbar.State getState();

    /**
     * Returns whether or not the state of the toolbar was previously set.
     *
     * @deprecated See {@link #setState(Toolbar.State)} for details.
     */
    @Deprecated
    boolean isStateSet();

    /**
     * Registers a new {@link Toolbar.OnTabSelectedListener} to the list of listeners.
     *
     * @deprecated The tabs set via {@link #setTabs(List)} have their own selected callbacks.
     */
    @Deprecated
    void registerOnTabSelectedListener(Toolbar.OnTabSelectedListener listener);

    /**
     * Unregisters an existing {@link Toolbar.OnTabSelectedListener} from the list of listeners.
     *
     * @deprecated The tabs set via {@link #setTabs(List)} have their own selected callbacks.
     */
    @Deprecated
    boolean unregisterOnTabSelectedListener(Toolbar.OnTabSelectedListener listener);

    /** Registers a new {@link Toolbar.OnSearchListener} to the list of listeners. */
    void registerOnSearchListener(Toolbar.OnSearchListener listener);

    /** Unregisters an existing {@link Toolbar.OnSearchListener} from the list of listeners. */
    boolean unregisterOnSearchListener(Toolbar.OnSearchListener listener);

    /** Registers a new {@link Consumer} to the list of listeners. */
    void registerSearchListener(Consumer<String> listener);

    /** Unregisters an existing {@link Consumer} from the list of listeners. */
    boolean unregisterSearchListener(Consumer<String> listener);

    /**
     * Sets the search info to be displayed within the widescreen IME.
     */
    void setSearchConfig(@Nullable SearchConfig searchConfig);

    /**
     * Returns the capabilities of what toolbar can do with Search
     */
    SearchCapabilities getSearchCapabilities();

    /**
     * Returns true if the toolbar can display search result items. One example of this is when the
     * system is configured to display search items in the IME instead of in the app.
     *
     * @deprecated See {@link SearchCapabilities#canShowSearchResultItems()} for details.
     */
    @Deprecated
    boolean canShowSearchResultItems();

    /**
     * Returns true if the app is allowed to set search results view.
     *
     * @deprecated See {@link SearchCapabilities#canShowSearchResultsView()} for details.
     */
    @Deprecated
    boolean canShowSearchResultsView();

    /**
     * Add a view within a container that will animate with the wide screen IME to display search
     * results.
     *
     * <p>Note: Apps can only call this method if the package name is allowed via OEM to render
     * their view.  To check if the application have the permission to do so or not first call
     * {@link SearchCapabilities#canShowSearchResultsView()}. If the app is not allowed this
     * method
     * will throw an {@link IllegalStateException}
     *
     * @param view to be added in the container.
     * @deprecated See {@link SearchConfigBuilder#setSearchResultsView(View)} for details.
     */
    @Deprecated
    void setSearchResultsView(@Nullable View view);

    /**
     * Set the icon to be displayed within the input field of IME window.
     *
     * @deprecated See {@link SearchConfigBuilder#setSearchResultsInputViewIcon(Drawable)} )} for
     * details.
     */
    @Deprecated
    void setSearchResultsInputViewIcon(@NonNull Drawable drawable);

    /**
     * Sets list of search item {@link CarUiListItem} to be displayed in the IMS
     * template. This method should be called when system is running in a wide screen mode. Apps
     * can check that by using {@link SearchCapabilities#canShowSearchResultItems()}
     * Else, this method will throw an {@link IllegalStateException}
     *
     * @deprecated See {@link SearchConfigBuilder#setSearchResultItems(List)} )} for details.
     */
    @Deprecated
    void setSearchResultItems(List<? extends CarUiImeSearchListItem> searchItems);

    /**
     * Registers a new {@link Toolbar.OnSearchCompletedListener} to the list of listeners.
     *
     * @deprecated Use {@link #registerSearchCompletedListener(Runnable)} instead.
     */
    @Deprecated
    void registerOnSearchCompletedListener(Toolbar.OnSearchCompletedListener listener);

    /**
     * Unregisters an existing {@link Toolbar.OnSearchCompletedListener} from the list of
     * listeners.
     *
     * @deprecated Use {@link #unregisterSearchCompletedListener(Runnable)} instead.
     */
    @Deprecated
    boolean unregisterOnSearchCompletedListener(Toolbar.OnSearchCompletedListener listener);

    /** Registers a new {@link Runnable} to the list of listeners. */
    void registerSearchCompletedListener(Runnable listener);

    /**
     * Unregisters an existing {@link Runnable} from the list of
     * listeners.
     */
    boolean unregisterSearchCompletedListener(Runnable listener);

    /**
     * Registers a new {@link Toolbar.OnBackListener} to the list of listeners.
     *
     * @deprecated Use {@link #registerBackListener(Supplier)} instead.
     */
    @Deprecated
    void registerOnBackListener(Toolbar.OnBackListener listener);

    /**
     * Unregisters an existing {@link Toolbar.OnBackListener} from the list of listeners.
     *
     * @deprecated Use {@link #unregisterBackListener(Supplier)} instead.
     */
    @Deprecated
    boolean unregisterOnBackListener(Toolbar.OnBackListener listener);

    /** Registers a new {@link Supplier<Boolean>} to the list of listeners. */
    void registerBackListener(Supplier<Boolean> listener);

    /** Unregisters an existing {@link Runnable} from the list of listeners. */
    boolean unregisterBackListener(Supplier<Boolean> listener);

    /** Gets a {@link ProgressBarController} */
    ProgressBarController getProgressBar();
}
