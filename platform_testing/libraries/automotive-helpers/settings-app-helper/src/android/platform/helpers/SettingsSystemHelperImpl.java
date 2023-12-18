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
import android.content.res.Resources;
import android.support.test.uiautomator.By;
import android.support.test.uiautomator.BySelector;
import android.support.test.uiautomator.UiObject2;
import android.support.test.uiautomator.UiScrollable;
import android.support.test.uiautomator.UiSelector;

import java.text.SimpleDateFormat;

import java.time.Month;
import java.util.Date;

public class SettingsSystemHelperImpl extends AbstractAutoStandardAppHelper
        implements IAutoSystemSettingsHelper {
    public SettingsSystemHelperImpl(Instrumentation instr) {
        super(instr);
    }
    /** {@inheritDoc} */
    @Override
    public String getPackage() {
        return getApplicationConfig(AutoConfigConstants.SETTINGS_PACKAGE);
    }

    /** {@inheritDoc} */
    @Override
    public void setDisplayLanguage(String language) {
        openLanguageMenu();
        BySelector languageSelector = By.clickable(true).hasDescendant(By.textStartsWith(language));
        UiObject2 languageObject = getMenu(languageSelector);
        clickAndWaitForWindowUpdate(
                getApplicationConfig(AutoConfigConstants.SETTINGS_PACKAGE), languageObject);
        String systemLanguage =
                Resources.getSystem().getConfiguration().getLocales().get(0).getDisplayLanguage();
        if (!language.toLowerCase().contains(systemLanguage.toLowerCase())) {
            throw new RuntimeException("System language is different from selected language");
        }
    }

    /** {@inheritDoc} */
    @Override
    public String getCurrentLanguage() {
        UiObject2 object =
                getBtnByText(
                        getResourceValue(
                                AutoConfigConstants.SETTINGS,
                                AutoConfigConstants.SYSTEM_SETTINGS,
                                AutoConfigConstants.LANGUAGES_MENU),
                        getResourceValue(
                                AutoConfigConstants.SETTINGS,
                                AutoConfigConstants.SYSTEM_SETTINGS,
                                AutoConfigConstants.LANGUAGES_MENU_IN_SELECTED_LANGUAGE));
        String currentLanguage = getSummeryText(object);
        return currentLanguage;
    }

    /** {@inheritDoc} */
    @Override
    public String getDeviceModel() {
        openAboutMenu();
        UiObject2 object =
                getMenu(
                        getResourceFromConfig(
                                AutoConfigConstants.SETTINGS,
                                AutoConfigConstants.SYSTEM_SETTINGS,
                                AutoConfigConstants.DEVICE_MODEL));
        String modelName = getSummeryText(object);
        return modelName;
    }

    /** {@inheritDoc} */
    @Override
    public String getAndroidVersion() {
        openAboutMenu();
        UiObject2 object =
                getMenu(
                        getResourceFromConfig(
                                AutoConfigConstants.SETTINGS,
                                AutoConfigConstants.SYSTEM_SETTINGS,
                                AutoConfigConstants.ANDROID_VERSION));
        String androidVersion = getSummeryText(object);
        return androidVersion;
    }

    /** {@inheritDoc} */
    @Override
    public Date getAndroidSecurityPatchLevel() {
        openAboutMenu();
        UiObject2 object =
                getMenu(
                        getResourceFromConfig(
                                AutoConfigConstants.SETTINGS,
                                AutoConfigConstants.SYSTEM_SETTINGS,
                                AutoConfigConstants.ANDROID_SECURITY_PATCH_LEVEL));
        String androidSecurityPatchLevel = getSummeryText(object);
        Date patchDate = parseDate(androidSecurityPatchLevel, "MMMM dd, yyyy");
        if (patchDate == null) {
            patchDate = parseDate(androidSecurityPatchLevel, "dd MMMM yyyy");
        }
        if (patchDate == null) {
            throw new RuntimeException("Cannot find date from UI");
        }
        return formatDate(patchDate, "MMMM dd, yyyy"); // return locale independent date
    }

    private Date formatDate(Date date, String format) {
        SimpleDateFormat dateFormatter = new SimpleDateFormat(format);
        String dateString = dateFormatter.format(date);
        String[] arr = dateString.split(" ");
        int year = Integer.valueOf(arr[2]);
        int month = Month.valueOf(arr[0].toUpperCase()).getValue() - 1;
        int day = Integer.valueOf(arr[1].substring(0, arr[1].length() - 1));
        return new Date(year, month, day);
    }

    private Date parseDate(String date, String format) {
        Date parsedDate = null;
        try {
            SimpleDateFormat dateFormatter = new SimpleDateFormat(format);
            parsedDate = dateFormatter.parse(date);
        } catch (Exception e) {
            // do nothing
        }
        return parsedDate;
    }

    /** {@inheritDoc} */
    @Override
    public String getKernelVersion() {
        openAboutMenu();
        UiObject2 object =
                getMenu(
                        getResourceFromConfig(
                                AutoConfigConstants.SETTINGS,
                                AutoConfigConstants.SYSTEM_SETTINGS,
                                AutoConfigConstants.KERNEL_VERSION));
        String kernelVersion = getSummeryText(object);
        return kernelVersion;
    }

    /** {@inheritDoc} */
    @Override
    public String getBuildNumber() {
        openAboutMenu();
        UiObject2 object =
                getMenu(
                        getResourceFromConfig(
                                AutoConfigConstants.SETTINGS,
                                AutoConfigConstants.SYSTEM_SETTINGS,
                                AutoConfigConstants.BUILD_NUMBER));
        String buildNumber = getSummeryText(object);
        return buildNumber;
    }

    /** {@inheritDoc} */
    @Override
    public void resetNetwork() {
        openResetOptionsMenu();
        BySelector resetNetworkSelector =
                By.clickable(true)
                        .hasDescendant(
                                getResourceFromConfig(
                                        AutoConfigConstants.SETTINGS,
                                        AutoConfigConstants.SYSTEM_SETTINGS,
                                        AutoConfigConstants.RESET_NETWORK));
        UiObject2 resetNetworkMenu = getMenu(resetNetworkSelector);
        clickAndWaitForWindowUpdate(
                getApplicationConfig(AutoConfigConstants.SETTINGS_PACKAGE), resetNetworkMenu);
        BySelector resetSettingsSelector =
                getResourceFromConfig(
                        AutoConfigConstants.SETTINGS,
                        AutoConfigConstants.SYSTEM_SETTINGS,
                        AutoConfigConstants.RESET_SETTINGS);
        UiObject2 resetSettingsButton1 = getMenu(resetSettingsSelector);
        clickAndWaitForWindowUpdate(
                getApplicationConfig(AutoConfigConstants.SETTINGS_PACKAGE), resetSettingsButton1);
        UiObject2 resetSettingsButton2 =
                getMenu(
                        getResourceFromConfig(
                                AutoConfigConstants.SETTINGS,
                                AutoConfigConstants.SYSTEM_SETTINGS,
                                AutoConfigConstants.RESET_SETTINGS));
        clickAndWaitForWindowUpdate(
                getApplicationConfig(AutoConfigConstants.SETTINGS_PACKAGE), resetSettingsButton2);
    }

    /** {@inheritDoc} */
    @Override
    public void resetAppPreferences() {
        openResetOptionsMenu();
        BySelector selector =
                By.clickable(true)
                        .hasDescendant(
                                getResourceFromConfig(
                                        AutoConfigConstants.SETTINGS,
                                        AutoConfigConstants.SYSTEM_SETTINGS,
                                        AutoConfigConstants.RESET_APP_PREFERENCES));
        UiObject2 object = getMenu(selector);
        clickAndWaitForWindowUpdate(
                getApplicationConfig(AutoConfigConstants.SETTINGS_PACKAGE), object);
        BySelector reset_apps_selector =
                getResourceFromConfig(
                        AutoConfigConstants.SETTINGS,
                        AutoConfigConstants.SYSTEM_SETTINGS,
                        AutoConfigConstants.RESET_APPS);
        UiObject2 reset_apps_button = getMenu(reset_apps_selector);
        clickAndWaitForWindowUpdate(
                getApplicationConfig(AutoConfigConstants.SETTINGS_PACKAGE), reset_apps_button);
    }

    /** {@inheritDoc} */
    @Override
    public void openLanguagesInputMenu() {
        UiObject2 languagesInputMenu =
                getMenu(
                        getResourceFromConfig(
                                AutoConfigConstants.SETTINGS,
                                AutoConfigConstants.SYSTEM_SETTINGS,
                                AutoConfigConstants.LANGUAGES_AND_INPUT_MENU));
        clickAndWaitForWindowUpdate(
                getApplicationConfig(AutoConfigConstants.SETTINGS_PACKAGE), languagesInputMenu);
    }

    private String getSummeryText(UiObject2 object) {
        UiObject2 parent = object.getParent();
        if (parent.getChildren().size() < 2) {
            UiSelector uiSelector =
                    new UiSelector()
                            .className(
                                    getResourceValue(
                                            AutoConfigConstants.SETTINGS,
                                            AutoConfigConstants.SYSTEM_SETTINGS,
                                            AutoConfigConstants.RECYCLER_VIEW_WIDGET));
            UiScrollable scrollable = new UiScrollable(uiSelector);
            try {
                scrollable.scrollForward();
            } catch (Exception e) {
                throw new RuntimeException(e);
            }
        }
        return object.getParent().getChildren().get(1).getText();
    }

    private void openResetOptionsMenu() {
        UiObject2 resetOptionMenu =
                getMenu(
                        getResourceFromConfig(
                                AutoConfigConstants.SETTINGS,
                                AutoConfigConstants.SYSTEM_SETTINGS,
                                AutoConfigConstants.RESET_OPTIONS_MENU));
        clickAndWaitForWindowUpdate(
                getApplicationConfig(AutoConfigConstants.SETTINGS_PACKAGE), resetOptionMenu);
    }

    private void openAboutMenu() {
        UiObject2 aboutMenu =
                getMenu(
                        getResourceFromConfig(
                                AutoConfigConstants.SETTINGS,
                                AutoConfigConstants.SYSTEM_SETTINGS,
                                AutoConfigConstants.ABOUT_MENU));
        clickAndWaitForWindowUpdate(
                getApplicationConfig(AutoConfigConstants.SETTINGS_PACKAGE), aboutMenu);
    }

    private void openLanguageMenu() {
        UiObject2 languageMenu =
                getBtnByText(
                        getResourceValue(
                                AutoConfigConstants.SETTINGS,
                                AutoConfigConstants.SYSTEM_SETTINGS,
                                AutoConfigConstants.LANGUAGES_MENU),
                        getResourceValue(
                                AutoConfigConstants.SETTINGS,
                                AutoConfigConstants.SYSTEM_SETTINGS,
                                AutoConfigConstants.LANGUAGES_MENU_IN_SELECTED_LANGUAGE));
        clickAndWaitForWindowUpdate(
                getApplicationConfig(AutoConfigConstants.SETTINGS_PACKAGE), languageMenu);
    }

    private UiObject2 getBtnByText(String... texts) {
        for (String text : texts) {
            BySelector btnSelector = By.text(text);
            UiObject2 btn = findUiObject(btnSelector);
            if (btn != null) {
                return btn;
            }
        }
        throw new RuntimeException("Cannot find button");
    }

    private UiObject2 getMenu(BySelector selector) {
        UiObject2 object = scrollAndFindUiObject(selector, getScrollScreenIndex());
        if (object == null) {
            throw new RuntimeException(
                    String.format("Unable to find UI Elemenet %s.", selector.toString()));
        }
        return object;
    }

    private int getScrollScreenIndex() {
        int scrollScreenIndex = 0;
        if (hasSplitScreenSettingsUI()) {
            scrollScreenIndex = 1;
        }
        return scrollScreenIndex;
    }
}
