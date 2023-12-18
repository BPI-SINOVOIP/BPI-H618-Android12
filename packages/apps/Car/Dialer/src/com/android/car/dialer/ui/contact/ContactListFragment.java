/*
 * Copyright (C) 2018 The Android Open Source Project
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

package com.android.car.dialer.ui.contact;

import android.os.Bundle;
import android.view.View;

import androidx.annotation.NonNull;
import androidx.annotation.Nullable;
import androidx.lifecycle.ViewModelProvider;

import com.android.car.dialer.Constants;
import com.android.car.dialer.R;
import com.android.car.dialer.ui.common.DialerListBaseFragment;

import javax.inject.Inject;

import dagger.hilt.android.AndroidEntryPoint;

/**
 * Contact Fragment.
 */
@AndroidEntryPoint(DialerListBaseFragment.class)
public class ContactListFragment extends Hilt_ContactListFragment {
    @Inject ContactListAdapter mContactListAdapter;

    public static ContactListFragment newInstance() {
        return new ContactListFragment();
    }

    @Override
    public void onViewCreated(@NonNull View view, @Nullable Bundle savedInstanceState) {
        super.onViewCreated(view, savedInstanceState);

        getRecyclerView().setAdapter(mContactListAdapter);
        getUxrContentLimiter().setAdapter(mContactListAdapter);

        ContactListViewModel contactListViewModel = new ViewModelProvider(this).get(
                ContactListViewModel.class);
        contactListViewModel.getAllContacts().observe(this, contacts -> {
            if (contacts.isLoading()) {
                showLoading();
            } else if (contacts.getData() == null) {
                showEmpty(Constants.INVALID_RES_ID, R.string.contact_list_empty,
                        R.string.available_after_sync);
            } else {
                mContactListAdapter.setContactList(contacts.getData());
                showContent();
            }
        });
    }
}
