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

/**
 * Class defining the capabilities of search
 */
public final class SearchCapabilities {

    private final boolean mCanShowSearchResultsView;
    private final boolean mCanShowSearchResultItems;

    /**
     * Returns true if the toolbar can display search result items. One example of this is when the
     * system is configured to display search items in the IME instead of in the app.
     */
    public boolean canShowSearchResultItems() {
        return mCanShowSearchResultItems;
    }

    /**
     * Returns true if the app is allowed to set search results view.
     */
    public boolean canShowSearchResultsView() {
        return mCanShowSearchResultsView;
    }

    private SearchCapabilities(SearchCapabilitiesBuilder builder) {
        mCanShowSearchResultsView = builder.mCanShowSearchResultsView;
        mCanShowSearchResultItems = builder.mCanShowSearchResultItems;
    }

    /**
     * Creates the instance of {@link SearchCapabilities.SearchCapabilitiesBuilder}
     */
    public static SearchCapabilities.SearchCapabilitiesBuilder builder() {
        return new SearchCapabilities.SearchCapabilitiesBuilder();
    }

    /**
     * Builder for {@link SearchCapabilities}
     */
    public static final class SearchCapabilitiesBuilder {

        private boolean mCanShowSearchResultsView;
        private boolean mCanShowSearchResultItems;

        private SearchCapabilitiesBuilder() {

        }

        /**
         * Sets if IME can display the search results item or not.
         */
        public SearchCapabilitiesBuilder setCanShowSearchResultItems(
                boolean canShowSearchResultItems) {
            mCanShowSearchResultItems = canShowSearchResultItems;
            return this;
        }

        /**
         * Sets if IME can display the search results view or not.
         */
        public SearchCapabilitiesBuilder setCanShowSearchResultsView(
                boolean canShowSearchResultsView) {
            mCanShowSearchResultsView = canShowSearchResultsView;
            return this;
        }

        /** Builds a {@link SearchCapabilities} from the current state of the Builder */
        public SearchCapabilities build() {
            return new SearchCapabilities(this);
        }
    }
}
