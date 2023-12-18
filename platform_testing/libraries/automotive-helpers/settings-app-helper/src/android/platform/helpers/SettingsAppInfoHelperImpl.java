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

import static junit.framework.Assert.assertTrue;

import android.app.Instrumentation;
import android.content.pm.PackageManager;
import android.content.pm.PackageManager.NameNotFoundException;
import android.support.test.uiautomator.By;
import android.support.test.uiautomator.BySelector;
import android.support.test.uiautomator.UiObject2;

public class SettingsAppInfoHelperImpl extends AbstractAutoStandardAppHelper
        implements IAutoAppInfoSettingsHelper {
    public SettingsAppInfoHelperImpl(Instrumentation instr) {
        super(instr);
    }

    /** {@inheritDoc} */
    @Override
    public void open() {
        executeShellCommand(getApplicationConfig(AutoConfigConstants.OPEN_SETTINGS_COMMAND));
    }

    /** {@inheritDoc} */
    @Override
    public String getPackage() {
        return getApplicationConfig(AutoConfigConstants.SETTINGS_PACKAGE);
    }

    /** {@inheritDoc} */
    @Override
    public String getLauncherName() {
        throw new UnsupportedOperationException("Not Supported");
    }

    /** {@inheritDoc} */
    @Override
    public void showAllApps() {
        BySelector selector =
                By.clickable(true)
                        .hasDescendant(
                                getResourceFromConfig(
                                        AutoConfigConstants.SETTINGS,
                                        AutoConfigConstants.APPS_SETTINGS,
                                        AutoConfigConstants.VIEW_ALL));
        UiObject2 show_all_apps_menu = scrollAndFindUiObject(selector, getScrollScreenIndex());
        clickAndWaitForWindowUpdate(
                getApplicationConfig(AutoConfigConstants.SETTINGS_PACKAGE), show_all_apps_menu);
    }

    /** {@inheritDoc} */
    @Override
    public void enableDisableApplication(State state) {
        BySelector enableDisableBtnSelector =
                getResourceFromConfig(
                        AutoConfigConstants.SETTINGS,
                        AutoConfigConstants.APPS_SETTINGS,
                        AutoConfigConstants.ENABLE_DISABLE_BUTTON);
        UiObject2 enableDisableBtn =
                scrollAndFindUiObject(enableDisableBtnSelector, getScrollScreenIndex());
        clickAndWaitForWindowUpdate(
                getApplicationConfig(AutoConfigConstants.SETTINGS_PACKAGE),
                enableDisableBtn.getParent());
        if (state == State.ENABLE) {
            assertTrue(
                    "application is not enabled",
                    enableDisableBtn
                            .getText()
                            .matches(
                                    "(?i)"
                                            + getResourceValue(
                                                    AutoConfigConstants.SETTINGS,
                                                    AutoConfigConstants.APPS_SETTINGS,
                                                    AutoConfigConstants.DISABLE_BUTTON_TEXT)));
        } else {
            BySelector disableAppBtnSelector =
                    getResourceFromConfig(
                            AutoConfigConstants.SETTINGS,
                            AutoConfigConstants.APPS_SETTINGS,
                            AutoConfigConstants.DISABLE_APP_BUTTON);
            UiObject2 disableAppBtn =
                    scrollAndFindUiObject(disableAppBtnSelector, getScrollScreenIndex());
            clickAndWaitForWindowUpdate(
                    getApplicationConfig(AutoConfigConstants.SETTINGS_PACKAGE), disableAppBtn);
            assertTrue(
                    "application is not disabled",
                    enableDisableBtn
                            .getText()
                            .matches(
                                    "(?i)"
                                            + getResourceValue(
                                                    AutoConfigConstants.SETTINGS,
                                                    AutoConfigConstants.APPS_SETTINGS,
                                                    AutoConfigConstants.ENABLE_BUTTON_TEXT)));
        }
    }

    /** {@inheritDoc} */
    @Override
    public boolean isCurrentApplicationRunning() {
        UiObject2 forceStopButton = getForceStopButton();
        if (forceStopButton == null) {
            throw new RuntimeException("Cannot find force stop button");
        }
        return forceStopButton.isEnabled() ? true : false;
    }

    /** {@inheritDoc} */
    @Override
    public void forceStop() {
        UiObject2 forceStopButton = getForceStopButton();
        if (forceStopButton == null) {
            throw new RuntimeException("Cannot find force stop button");
        }
        clickAndWaitForWindowUpdate(
                getApplicationConfig(AutoConfigConstants.SETTINGS_PACKAGE), forceStopButton);
        BySelector okBtnSelector =
                getResourceFromConfig(
                        AutoConfigConstants.SETTINGS,
                        AutoConfigConstants.APPS_SETTINGS,
                        AutoConfigConstants.OK_BUTTON);
        UiObject2 okBtn = findUiObject(okBtnSelector);
        clickAndWaitForWindowUpdate(
                getApplicationConfig(AutoConfigConstants.SETTINGS_PACKAGE), okBtn);
    }

    /** {@inheritDoc} */
    @Override
    public void setAppPermission(String permission, State state) {
        BySelector permissions_selector =
                getResourceFromConfig(
                        AutoConfigConstants.SETTINGS,
                        AutoConfigConstants.APPS_SETTINGS,
                        AutoConfigConstants.PERMISSIONS_MENU);
        UiObject2 permissions_menu =
                scrollAndFindUiObject(permissions_selector, getScrollScreenIndex());
        clickAndWaitForWindowUpdate(
                getApplicationConfig(AutoConfigConstants.SETTINGS_PACKAGE), permissions_menu);
        BySelector permission_selector = By.text(permission);
        UiObject2 permission_menu =
                scrollAndFindUiObject(permission_selector, getScrollScreenIndex());
        clickAndWaitForWindowUpdate(
                getApplicationConfig(AutoConfigConstants.SETTINGS_PACKAGE), permission_menu);
        if (state == State.ENABLE) {
            UiObject2 allow_btn =
                    scrollAndFindUiObject(
                            getResourceFromConfig(
                                    AutoConfigConstants.SETTINGS,
                                    AutoConfigConstants.APPS_SETTINGS,
                                    AutoConfigConstants.ALLOW_BUTTON));
            clickAndWaitForWindowUpdate(
                    getApplicationConfig(AutoConfigConstants.SETTINGS_PACKAGE), allow_btn);
        } else {
            UiObject2 dont_allow_btn =
                    scrollAndFindUiObject(
                            getResourceFromConfig(
                                    AutoConfigConstants.SETTINGS,
                                    AutoConfigConstants.APPS_SETTINGS,
                                    AutoConfigConstants.DONT_ALLOW_BUTTON));
            clickAndWaitForWindowUpdate(
                    getApplicationConfig(AutoConfigConstants.SETTINGS_PACKAGE), dont_allow_btn);
            UiObject2 dont_allow_anyway_btn =
                    scrollAndFindUiObject(
                            getResourceFromConfig(
                                    AutoConfigConstants.SETTINGS,
                                    AutoConfigConstants.APPS_SETTINGS,
                                    AutoConfigConstants.DONT_ALLOW_ANYWAY_BUTTON));
            clickAndWaitForWindowUpdate(
                    getApplicationConfig(AutoConfigConstants.SETTINGS_PACKAGE),
                    dont_allow_anyway_btn);
        }
        pressBack();
        pressBack();
    }

    /** {@inheritDoc} */
    @Override
    public String getCurrentPermissions() {
        BySelector permissions_selector =
                getResourceFromConfig(
                        AutoConfigConstants.SETTINGS,
                        AutoConfigConstants.APPS_SETTINGS,
                        AutoConfigConstants.PERMISSIONS_MENU);
        UiObject2 permission_menu =
                scrollAndFindUiObject(permissions_selector, getScrollScreenIndex());
        String currentPermissions = permission_menu.getParent().getChildren().get(1).getText();
        return currentPermissions;
    }

    /** {@inheritDoc} */
    @Override
    public void selectApp(String application) {
        BySelector selector = By.textContains(application);
        UiObject2 object = scrollAndFindUiObject(selector, getScrollScreenIndex());
        if (object == null) {
            throw new RuntimeException("Cannot find the app menu");
        }
        clickAndWaitForWindowUpdate(
                getApplicationConfig(AutoConfigConstants.SETTINGS_PACKAGE), object);
    }

    private UiObject2 getForceStopButton() {
        BySelector forceStopSelector =
                getResourceFromConfig(
                        AutoConfigConstants.SETTINGS,
                        AutoConfigConstants.APPS_SETTINGS,
                        AutoConfigConstants.FORCE_STOP_BUTTON);
        UiObject2 forceStopButton =
                scrollAndFindUiObject(forceStopSelector, getScrollScreenIndex());
        return forceStopButton;
    }

    private int getScrollScreenIndex() {
        int scrollScreenIndex = 0;
        if (hasSplitScreenSettingsUI()) {
            scrollScreenIndex = 1;
        }
        return scrollScreenIndex;
    }

    /** {@inheritDoc} */
    @Override
    public boolean isApplicationDisabled(String packageName) {
        boolean applicationDisabled = false;
        try {
            PackageManager pm = mInstrumentation.getContext().getPackageManager();
            applicationDisabled = !(pm.getApplicationInfo(packageName, 0).enabled);
        } catch (NameNotFoundException e) {
            throw new RuntimeException(String.format("Failed to find package: %s", packageName), e);
        }
        return applicationDisabled;
    }
}
