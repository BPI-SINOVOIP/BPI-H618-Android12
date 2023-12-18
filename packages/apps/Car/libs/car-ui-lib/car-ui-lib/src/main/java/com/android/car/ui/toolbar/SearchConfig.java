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

import androidx.annotation.Nullable;

import com.android.car.ui.imewidescreen.CarUiImeSearchListItem;
import com.android.car.ui.recyclerview.CarUiListItem;

import java.util.ArrayList;
import java.util.Collections;
import java.util.List;

/**
 * Class used to provide data for IME widescreen search results.
 */
public final class SearchConfig {

    @Nullable
    private final View mSearchResultsView;
    @Nullable
    private final Drawable mSearchResultsInputViewIcon;
    @Nullable
    private final List<? extends CarUiImeSearchListItem> mSearchResultItems;

    /**
     * Returns the view set by {@link SearchConfigBuilder#setSearchResultsView(View)}
     */
    @Nullable
    public View getSearchResultsView() {
        return mSearchResultsView;
    }

    /**
     * Returns the icon set by {@link SearchConfigBuilder#setSearchResultsInputViewIcon} )}
     */
    @Nullable
    public Drawable getSearchResultsInputViewIcon() {
        return mSearchResultsInputViewIcon;
    }

    /**
     * Returns the search results set by {@link SearchConfigBuilder#setSearchResultItems(List)}
     */
    @Nullable
    public List<? extends CarUiImeSearchListItem> getSearchResultItems() {
        return mSearchResultItems;
    }

    private SearchConfig(SearchConfigBuilder builder) {
        mSearchResultItems = builder.mSearchResultItems;
        mSearchResultsInputViewIcon = builder.mSearchResultsInputViewIcon;
        mSearchResultsView = builder.mSearchResultsView;
    }

    /**
     * Creates the instance of {@link SearchConfigBuilder}
     */
    public static SearchConfigBuilder builder() {
        return new SearchConfigBuilder();
    }

    /**
     * Builder for {@link SearchConfig}
     */
    public static final class SearchConfigBuilder {

        @Nullable
        private View mSearchResultsView;
        @Nullable
        private Drawable mSearchResultsInputViewIcon;
        @Nullable
        private List<? extends CarUiImeSearchListItem> mSearchResultItems;

        private SearchConfigBuilder() {

        }

        /**
         * Set the icon to be displayed within the input field of IME window.
         */
        public SearchConfigBuilder setSearchResultsInputViewIcon(@Nullable Drawable drawable) {
            mSearchResultsInputViewIcon = drawable;
            return this;
        }

        /**
         * Sets list of search item {@link CarUiListItem} to be displayed in the IMS
         * template. This method should be called when system is running in a wide screen mode. Apps
         * can check that by using
         * {@link com.android.car.ui.toolbar.ToolbarController##canShowSearchResultItems()}
         * Else, this method will throw an {@link IllegalStateException}
         */
        public SearchConfigBuilder setSearchResultItems(
                @Nullable List<? extends CarUiImeSearchListItem> searchItems) {
            if (searchItems == null || searchItems.size() == 0) {
                mSearchResultItems = null;
            } else {
                mSearchResultItems = Collections.unmodifiableList(new ArrayList<>(searchItems));
            }
            return this;
        }

        /**
         * Add a view within a container that will animate with the wide screen IME to display
         * search
         * results.
         *
         * <p>Note: Apps can only call this method if the package name is allowed via OEM to render
         * their view.  To check if the application have the permission to do so or not first call
         * {@link com.android.car.ui.toolbar.ToolbarController#canShowSearchResultsView()}.
         * If the app is not allowed this method will throw an {@link IllegalStateException}
         *
         * @param view to be added in the container.
         */
        public SearchConfigBuilder setSearchResultsView(@Nullable View view) {
            mSearchResultsView = view;
            return this;
        }

        /** Builds a {@link SearchConfig} from the current state of the Builder */
        public SearchConfig build() {
            if (mSearchResultsView != null && mSearchResultItems != null) {
                throw new RuntimeException(
                        "Both searchResultItems and searchResultsView can't be set at the same "
                                + "time");
            }
            return new SearchConfig(this);
        }
    }
}
