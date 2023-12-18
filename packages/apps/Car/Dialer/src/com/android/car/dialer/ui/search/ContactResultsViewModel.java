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

package com.android.car.dialer.ui.search;

import android.content.Context;
import android.text.TextUtils;

import androidx.lifecycle.LiveData;
import androidx.lifecycle.MutableLiveData;

import com.android.car.dialer.livedata.SharedPreferencesLiveData;
import com.android.car.dialer.ui.common.ContactResultsLiveData;
import com.android.car.dialer.ui.common.DialerListViewModel;

import java.util.List;

import javax.inject.Inject;

import dagger.hilt.android.lifecycle.HiltViewModel;
import dagger.hilt.android.qualifiers.ApplicationContext;

/**
 * {link AndroidViewModel} used for search functionality.
 */
@HiltViewModel
public class ContactResultsViewModel extends DialerListViewModel {

    private final ContactResultsLiveData mContactSearchResultsLiveData;
    private final MutableLiveData<String> mSearchQueryLiveData;

    @Inject
    public ContactResultsViewModel(@ApplicationContext Context context,
            SharedPreferencesLiveData.Factory sharedPreferencesFactory,
            ContactResultsLiveData.Factory contactResultsLiveDataFactory) {
        super(context, sharedPreferencesFactory);
        mSearchQueryLiveData = new MutableLiveData<>();
        mContactSearchResultsLiveData = createContactSearchResultsLiveData(
                contactResultsLiveDataFactory);
    }

    /** Creates the {@link ContactResultsLiveData} representing the contact search results. */
    public ContactResultsLiveData createContactSearchResultsLiveData(
            ContactResultsLiveData.Factory factory) {
        return factory.create(
                getSearchQueryLiveData(),
                getSharedPreferencesLiveData(),
                /* showOnlyOneEntry = */true);
    }

    /**
     * Sets search query.
     */
    public void setSearchQuery(String searchQuery) {
        if (TextUtils.equals(mSearchQueryLiveData.getValue(), searchQuery)) {
            return;
        }

        mSearchQueryLiveData.setValue(searchQuery);
    }

    /**
     * Returns live data of search results.
     */
    public LiveData<List<ContactResultsLiveData.ContactResultListItem>> getContactSearchResults() {
        return mContactSearchResultsLiveData;
    }

    /**
     * Returns search query.
     */
    public String getSearchQuery() {
        return mSearchQueryLiveData.getValue();
    }

    /**
     * Returns Search Query LiveData.
     */
    public MutableLiveData<String> getSearchQueryLiveData() {
        return mSearchQueryLiveData;
    }
}
