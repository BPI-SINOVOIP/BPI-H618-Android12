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

package com.android.car.dialer.ui.dialpad;

import android.content.Context;

import com.android.car.dialer.livedata.SharedPreferencesLiveData;
import com.android.car.dialer.ui.common.ContactResultsLiveData;
import com.android.car.dialer.ui.search.ContactResultsViewModel;

import javax.inject.Inject;

import dagger.hilt.android.lifecycle.HiltViewModel;
import dagger.hilt.android.qualifiers.ApplicationContext;

/**
 * {link AndroidViewModel} used for type down functionality.
 */
@HiltViewModel
public class TypeDownResultsViewModel extends ContactResultsViewModel {

    @Inject
    public TypeDownResultsViewModel(@ApplicationContext Context context,
            SharedPreferencesLiveData.Factory sharedPreferencesFactory,
            ContactResultsLiveData.Factory contactResultsLiveDataFactory) {
        super(context, sharedPreferencesFactory, contactResultsLiveDataFactory);
    }

    @Override
    public ContactResultsLiveData createContactSearchResultsLiveData(
            ContactResultsLiveData.Factory factory) {
        return factory.create(
                getSearchQueryLiveData(),
                getSharedPreferencesLiveData(),
                /* showOnlyOneEntry = */ false);
    }
}
