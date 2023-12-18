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

package android.platform.helpers;

import android.app.Instrumentation;
import android.os.SystemClock;
import android.support.test.uiautomator.By;
import android.support.test.uiautomator.BySelector;
import android.support.test.uiautomator.UiObject2;

/** Implementation of {@link IAutoAccountsHelper} to support tests of account settings */
public class SettingAccountsHelperImpl extends AbstractAutoStandardAppHelper
        implements IAutoAccountsHelper {

    // Wait Time
    private static final int UI_RESPONSE_WAIT_MS = 5000;

    public SettingAccountsHelperImpl(Instrumentation instr) {
        super(instr);
    }

    /** {@inheritDoc} */
    @Override
    public String getPackage() {
        return getApplicationConfig(AutoConfigConstants.SETTINGS_PACKAGE);
    }

    /** {@inheritDoc} */
    @Override
    public void addAccount(String email, String password) {
        if (!doesEmailExist(email)) {
            goToSignInPage();
            inputAccount(email);
            inputPassowrd(password);
            UiObject2 doneButtonObject =
                    scrollAndFindUiObject(
                            getResourceFromConfig(
                                    AutoConfigConstants.SETTINGS,
                                    AutoConfigConstants.PROFILE_ACCOUNT_SETTINGS,
                                    AutoConfigConstants.DONE_BUTTON));
            if (doneButtonObject == null) {
                throw new RuntimeException("Unable to find Done button.");
            }
            clickAndWaitForIdleScreen(doneButtonObject);
        }
    }

    private void goToSignInPage() {
        UiObject2 addAccountObject =
                scrollAndFindUiObject(
                        getResourceFromConfig(
                                AutoConfigConstants.SETTINGS,
                                AutoConfigConstants.PROFILE_ACCOUNT_SETTINGS,
                                AutoConfigConstants.ADD_ACCOUNT));
        if (addAccountObject == null) {
            throw new RuntimeException("Unable to find Add Account button.");
        }
        clickAndWaitForIdleScreen(addAccountObject);
        UiObject2 signInOnCarScreen =
                scrollAndFindUiObject(
                        By.clickable(true)
                                .hasDescendant(
                                        getResourceFromConfig(
                                                AutoConfigConstants.SETTINGS,
                                                AutoConfigConstants.PROFILE_ACCOUNT_SETTINGS,
                                                AutoConfigConstants.SIGN_IN_ON_CAR_SCREEN)));
        if (signInOnCarScreen == null) {
            throw new RuntimeException("Unable to find Sign In on Car Screen button.");
        }
        clickAndWaitForIdleScreen(signInOnCarScreen);
        scrollAndFindUiObject(
                getResourceFromConfig(
                        AutoConfigConstants.SETTINGS,
                        AutoConfigConstants.PROFILE_ACCOUNT_SETTINGS,
                        AutoConfigConstants.GOOGLE_SIGN_IN_SCREEN));
    }

    /** {@inheritDoc} */
    @Override
    public void removeAccount(String email) {
        if (doesEmailExist(email)) {
            BySelector accountSelector = By.text(email);
            UiObject2 accountObject =
                    scrollAndFindUiObject(accountSelector, getScrollScreenIndex())
                            .getParent()
                            .getParent();
            clickAndWaitForIdleScreen(accountObject);
            UiObject2 removeButtonObject =
                    scrollAndFindUiObject(
                            getResourceFromConfig(
                                    AutoConfigConstants.SETTINGS,
                                    AutoConfigConstants.PROFILE_ACCOUNT_SETTINGS,
                                    AutoConfigConstants.REMOVE_BUTTON));
            if (removeButtonObject == null) {
                throw new RuntimeException("Unable to find Remove button.");
            }
            clickAndWaitForIdleScreen(removeButtonObject);
            UiObject2 confirmObject =
                    scrollAndFindUiObject(
                            getResourceFromConfig(
                                    AutoConfigConstants.SETTINGS,
                                    AutoConfigConstants.PROFILE_ACCOUNT_SETTINGS,
                                    AutoConfigConstants.REMOVE_ACCOUNT_BUTTON));
            if (removeButtonObject == null) {
                throw new RuntimeException("Unable to find Remove Account button.");
            }
            clickAndWaitForIdleScreen(confirmObject);
            waitForGone(accountSelector);
        }
    }

    /** {@inheritDoc} */
    @Override
    public boolean doesEmailExist(String email) {
        UiObject2 accountObject = scrollAndFindUiObject(By.text(email), getScrollScreenIndex());
        return accountObject != null;
    }

    private int getScrollScreenIndex() {
        int scrollScreenIndex = 0;
        if (hasSplitScreenSettingsUI()) {
            scrollScreenIndex = 1;
        }
        return scrollScreenIndex;
    }

    private void inputAccount(String account) {
        inputText(account, false);
    }

    private void inputPassowrd(String password) {
        inputText(password, true);
    }

    private void inputText(String text, boolean isPassword) {
        BySelector selector =
                getResourceFromConfig(
                        AutoConfigConstants.SETTINGS,
                        AutoConfigConstants.PROFILE_ACCOUNT_SETTINGS,
                        AutoConfigConstants.ENTER_EMAIL);
        if (isPassword) {
            selector =
                    getResourceFromConfig(
                            AutoConfigConstants.SETTINGS,
                            AutoConfigConstants.PROFILE_ACCOUNT_SETTINGS,
                            AutoConfigConstants.ENTER_PASSWORD);
        }
        UiObject2 input = scrollAndFindUiObject(selector);
        if (input == null) {
            throw new RuntimeException(
                    String.format("%s input is not present", selector.toString()));
        }
        input.setText(text);
        scrollAndFindUiObject(By.text(text).focused(false));
        UiObject2 nextButtonObject =
                scrollAndFindUiObject(
                        getResourceFromConfig(
                                AutoConfigConstants.SETTINGS,
                                AutoConfigConstants.PROFILE_ACCOUNT_SETTINGS,
                                AutoConfigConstants.NEXT_BUTTON));
        if (nextButtonObject == null) {
            throw new RuntimeException("Unable to find Next button.");
        }
        clickAndWaitForGone(nextButtonObject, By.text(text));
        SystemClock.sleep(UI_RESPONSE_WAIT_MS); // to avoid stale object error
    }
}
