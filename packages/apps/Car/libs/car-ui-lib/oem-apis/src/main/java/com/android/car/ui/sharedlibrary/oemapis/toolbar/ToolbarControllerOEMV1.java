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

package com.android.car.ui.sharedlibrary.oemapis.toolbar;

import android.graphics.drawable.Drawable;

import java.util.List;
import java.util.function.Consumer;

/** The OEM interface for a Toolbar. */
@SuppressWarnings("AndroidJdkLibsChecker")
public interface ToolbarControllerOEMV1 {

    /**
     * Sets the title of the toolbar to a String.
     *
     * <p>The title may not always be shown, for example with one row layout with tabs.
     */
    void setTitle(String title);

    /**
     * Sets the subtitle of the toolbar to a String.
     *
     * <p>The title may not always be shown, for example with one row layout with tabs.
     */
    void setSubtitle(String title);

    /**
     * Sets the tab to be shown. The implementation must copy the list once it's passed in,
     * or else the list could be modified from the app when the toolbar wasn't expecting it.
     *
     * @param tabs Nullable. Must not be mutated. List of tabs to show.
     * @param selectedTab The index of the tab that is initially selected.
     */
    void setTabs(List<? extends TabOEMV1> tabs, int selectedTab);

    /**
     * Selects a tab added to this toolbar. See
     * {@link #setTabs(List, int)}. The tab's callback should also be called if {@code sendCallback}
     * is true.
     */
    void selectTab(int position, boolean sendCallback);

    /**
     * Sets the logo to display in this toolbar. If navigation icon is being displayed, this logo
     * will be displayed next to the title.
     */
    void setLogo(Drawable drawable);

    /** Sets the hint for the search bar. */
    void setSearchHint(String hint);

    /**
     * Sets the icon to display in the search box.
     *
     * <p>The icon will be lost on configuration change, make sure to set it in onCreate() or
     * a similar place.
     */
    void setSearchIcon(Drawable d);

    /**
     * Sets the search query.
     */
    void setSearchQuery(String query);

    int SEARCH_MODE_DISABLED = 0;
    int SEARCH_MODE_SEARCH = 1;
    int SEARCH_MODE_EDIT = 2;

    /**
     * Sets the search mode, which is whether or not to display the search bar and how it should
     * look.
     */
    void setSearchMode(int searchMode);

    /**
     * Gets a {@link ImeSearchInterfaceOEMV1}, which contains methods for interfacing
     * with the static library to support showing search results in the IME. It should return
     * the same object every time it's called. It may return null if this feature is not supported.
     */
    ImeSearchInterfaceOEMV1 getImeSearchInterface();

    /** Don't show the nav button */
    int NAV_BUTTON_MODE_DISABLED = 0;
    /** Display the nav button as a back button */
    int NAV_BUTTON_MODE_BACK = 1;
    /** Display the nav button as a close button */
    int NAV_BUTTON_MODE_CLOSE = 2;
    /**
     * Display the nav button as a "down" button.
     * This indicates that pressing it will slide a panel down to close it.
     */
    int NAV_BUTTON_MODE_DOWN = 3;

    /**
     * Sets the nav button mode, which is a certain style to display the nav button in.
     * These styles are all purely visual, and don't affect the behavior of clicking
     * the nav button.
     *
     * See {@link #NAV_BUTTON_MODE_DISABLED}, {@link #NAV_BUTTON_MODE_BACK},
     * {@link #NAV_BUTTON_MODE_CLOSE}, and {@link #NAV_BUTTON_MODE_DOWN}.
     */
    void setNavButtonMode(int mode);

    /**
     * Sets the {@link MenuItemOEMV1 Menuitems} to display.
     */
    void setMenuItems(List<? extends MenuItemOEMV1> items);

    /**
     * Sets a {@link Consumer<String>} to be called whenever the text in the search box
     * changes.
     *
     * Must accept {@code null} to unset the listener.
     */
    void setSearchListener(Consumer<String> listener);

    /**
     * Sets a {@link Runnable} to be called whenever the user indicates that they're done searching.
     * This can be by clicking the search/enter button on the keyboard, or a custom button
     * on the toolbar.
     *
     * Must accept {@code null} to unset the listener.
     */
    void setSearchCompletedListener(Runnable listener);

    /**
     * Sets a {@link Runnable} to be called whenever the back button is pressed.
     *
     * Must accept {@code null} to unset the listener.
     */
    void setBackListener(Runnable listener);

    /** Gets a {@link ProgressBarControllerOEMV1 ProgressBarController} */
    ProgressBarControllerOEMV1 getProgressBar();
}
