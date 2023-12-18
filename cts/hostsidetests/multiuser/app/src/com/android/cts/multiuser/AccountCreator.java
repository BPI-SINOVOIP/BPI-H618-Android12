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
package com.android.cts.multiuser;

import static com.google.common.truth.Truth.assertThat;

import android.accounts.Account;
import android.accounts.AccountManager;
import android.accounts.AccountManagerFuture;
import android.content.Context;
import android.os.Bundle;
import android.util.Log;

import androidx.test.InstrumentationRegistry;

import org.junit.Before;
import org.junit.Test;
import org.junit.runner.RunWith;
import org.junit.runners.JUnit4;

@RunWith(JUnit4.class)
public class AccountCreator {
    private static final String TAG = "MultiuserAccountCreator";

    private Context mContext;

    @Before
    public void setUp() throws Exception {
        mContext = InstrumentationRegistry.getContext();
    }

    @Test
    public void addMockAccountForCurrentUser() throws Exception {
        Log.i(TAG, "Running addMockAccountForCurrentUser");
        final AccountManager accountManager = mContext.getSystemService(AccountManager.class);

        Log.i(TAG, "Adding account");
        final AccountManagerFuture<Bundle> future = accountManager.addAccount(
                MockAuthenticator.ACCOUNT_TYPE, null, null, null, null, null, null);

        Log.i(TAG, "Waiting for account to be added");
        final Bundle result = future.getResult();

        Log.i(TAG, "Checking that adding account was successful");
        assertThat(result.getString(AccountManager.KEY_ACCOUNT_TYPE)).isEqualTo(
                MockAuthenticator.ACCOUNT_TYPE);
        assertThat(result.getString(AccountManager.KEY_ACCOUNT_NAME)).isEqualTo(
                MockAuthenticator.ACCOUNT_NAME);
        final Account[] accounts = accountManager.getAccounts();
        assertThat(accounts).hasLength(1);
        assertThat(accountManager.getAccountsByType(MockAuthenticator.ACCOUNT_TYPE)).hasLength(1);

        Log.i(TAG, "Successfully added account; all is good");
    }
}
