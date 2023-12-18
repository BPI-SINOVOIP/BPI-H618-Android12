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

/**
 * This is a legacy toolbar class, that now no longer functions as a toolbar, but instead
 * just holds deprecated inner classes.
 *
 * @deprecated Instead of creating this class, use Theme.CarUi.WithToolbar, and get access to it
 * via {@link com.android.car.ui.core.CarUi#requireToolbar(android.app.Activity)}
 */
@Deprecated
public final class Toolbar {

    /** Callback that will be issued whenever the height of toolbar is changed. */
    public interface OnHeightChangedListener {
        /**
         * Will be called when the height of the toolbar is changed.
         *
         * @param height new height of the toolbar
         */
        void onHeightChanged(int height);
    }

    /** Back button listener */
    public interface OnBackListener {
        /**
         * Invoked when the user clicks on the back button. By default, the toolbar will call
         * the Activity's {@link android.app.Activity#onBackPressed()}. Returning true from
         * this method will absorb the back press and prevent that behavior.
         */
        boolean onBack();
    }

    /** Tab selection listener */
    public interface OnTabSelectedListener {
        /** Called when a {@link TabLayout.Tab} is selected */
        void onTabSelected(TabLayout.Tab tab);
    }

    /** Search listener */
    public interface OnSearchListener {
        /**
         * Invoked when the user edits a search query.
         *
         * <p>This is called for every letter the user types, and also empty strings if the user
         * erases everything.
         */
        void onSearch(String query);
    }

    /** Search completed listener */
    public interface OnSearchCompletedListener {
        /**
         * Invoked when the user submits a search query by clicking the keyboard's search / done
         * button.
         */
        void onSearchCompleted();
    }

    /** Enum of states the toolbar can be in. Controls what elements of the toolbar are displayed */
    public enum State {
        /**
         * In the HOME state, the logo will be displayed if there is one, and no navigation icon
         * will be displayed. The tab bar will be visible. The title will be displayed if there
         * is space. MenuItems will be displayed.
         */
        HOME,
        /**
         * In the SUBPAGE state, the logo will be replaced with a back button, the tab bar won't
         * be visible. The title and MenuItems will be displayed.
         */
        SUBPAGE,
        /**
         * In the SEARCH state, only the back button and the search bar will be visible.
         */
        SEARCH,
        /**
         * In the EDIT state, the search bar will look like a regular text box, but will be
         * functionally identical to the SEARCH state.
         */
        EDIT,
    }

    /**
     * An enum of possible styles the nav button could be in. All styles will still call
     * {@link OnBackListener#onBack()}.
     */
    public enum NavButtonMode {
        /** A back button */
        BACK,
        /** A close button */
        CLOSE,
        /** A down button, used to indicate that the page will animate down when navigating away */
        DOWN,
        /** Don't show the nav button */
        DISABLED,
    }
}
