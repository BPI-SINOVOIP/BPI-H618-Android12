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

package android.platform.tests;

import android.platform.helpers.AutoConfigConstants;
import android.platform.helpers.AutoUtility;
import android.platform.helpers.HelperAccessor;
import android.platform.helpers.IAutoAccountsHelper;
import android.platform.helpers.IAutoSettingHelper;
import android.platform.test.option.StringOption;
import androidx.test.runner.AndroidJUnit4;

import org.junit.BeforeClass;
import org.junit.After;
import org.junit.Before;
import org.junit.ClassRule;
import org.junit.Test;
import org.junit.runner.RunWith;

import static junit.framework.Assert.assertFalse;
import static junit.framework.Assert.assertTrue;

@RunWith(AndroidJUnit4.class)
public class AccountSettingTest {
    private static final String ACCOUNT_EMAIL = "account-email";
    private static final String ACCOUNT_PASSWORD = "account-password";

    @ClassRule
    public static StringOption mAccountEmail = new StringOption(ACCOUNT_EMAIL).setRequired(true);

    @ClassRule
    public static StringOption mAccountPassword =
            new StringOption(ACCOUNT_PASSWORD).setRequired(true);

    private HelperAccessor<IAutoAccountsHelper> mAccountsHelper;
    private HelperAccessor<IAutoSettingHelper> mSettingHelper;

    public AccountSettingTest() {
        mAccountsHelper = new HelperAccessor<>(IAutoAccountsHelper.class);
        mSettingHelper = new HelperAccessor<>(IAutoSettingHelper.class);
    }

    @BeforeClass
    public static void exitSuw() {
        AutoUtility.exitSuw();
    }

    @Before
    public void openAccountsFacet() {
        mSettingHelper.get().openSetting(AutoConfigConstants.PROFILE_ACCOUNT_SETTINGS);
    }

    @After
    public void goBackToHomeScreen() {
        mSettingHelper.get().goBackToSettingsScreen();
    }

    @Test
    public void testAddRemoveAccount() {
        mAccountsHelper.get().addAccount(mAccountEmail.get(), mAccountPassword.get());
        mSettingHelper.get().openSetting(AutoConfigConstants.PROFILE_ACCOUNT_SETTINGS);
        assertTrue(mAccountsHelper.get().doesEmailExist(mAccountEmail.get()));
        mAccountsHelper.get().removeAccount(mAccountEmail.get());
        assertFalse(mAccountsHelper.get().doesEmailExist(mAccountEmail.get()));
    }
}
