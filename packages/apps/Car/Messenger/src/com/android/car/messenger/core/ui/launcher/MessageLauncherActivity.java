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

package com.android.car.messenger.core.ui.launcher;

import androidx.lifecycle.ViewModelProvider;
import android.os.Bundle;
import androidx.fragment.app.Fragment;
import androidx.fragment.app.FragmentActivity;

import androidx.annotation.NonNull;

import com.android.car.messenger.core.interfaces.AppFactory;
import com.android.car.messenger.core.models.UserAccount;
import com.android.car.messenger.core.ui.conversationlist.ConversationListFragment;
import com.android.car.messenger.core.util.L;
import com.android.car.ui.baselayout.Insets;
import com.android.car.ui.baselayout.InsetsChangedListener;

/**
 * This is the launcher activity for the messaging app. This first routes to{@link
 * ConversationListFragment} or displays an error when no {@link UserAccount} are found.
 */
public class MessageLauncherActivity extends FragmentActivity implements InsetsChangedListener {

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        MessageLauncherViewModel viewModel =
                new ViewModelProvider(this).get(MessageLauncherViewModel.class);

        L.d("In onCreate: MessageLauncher");
        viewModel
                .getAccounts()
                .observe(
                        this,
                        accounts -> {
                            L.d("Total number of accounts: " + accounts.size());
                            // First version only takes one device until multi-account support is
                            // added
                            UserAccount primaryAccount =
                                    !accounts.isEmpty() ? accounts.get(0) : null;
                            String fragmentTag =
                                    ConversationListFragment.getFragmentTag(primaryAccount);
                            Fragment fragment =
                                    getSupportFragmentManager().findFragmentByTag(fragmentTag);
                            if (fragment == null) {
                                fragment = ConversationListFragment.newInstance(primaryAccount);
                            }
                            setContentFragment(fragment, fragmentTag);
                        });
    }

    private void setContentFragment(Fragment fragment, String fragmentTag) {
        getSupportFragmentManager().executePendingTransactions();
        while (getSupportFragmentManager().getBackStackEntryCount() > 0) {
            getSupportFragmentManager().popBackStackImmediate();
        }
        pushContentFragment(fragment, fragmentTag);
    }

    @Override
    protected void onResume() {
        super.onResume();
        L.d("On Resume of Message Activity.");
        AppFactory.get().getDataModel().refreshUserAccounts();
    }

    private void pushContentFragment(
            @NonNull Fragment topContentFragment, @NonNull String fragmentTag) {
        getSupportFragmentManager()
                .beginTransaction()
                .replace(android.R.id.content, topContentFragment, fragmentTag)
                .addToBackStack(fragmentTag)
                .commit();
    }

    @Override
    public void onCarUiInsetsChanged(Insets insets) {
        // Do nothing, this is just a marker that we will handle the insets in fragments.
        // This is only necessary because the fragments are not immediately added to the
        // activity when calling .commit()
    }
}
