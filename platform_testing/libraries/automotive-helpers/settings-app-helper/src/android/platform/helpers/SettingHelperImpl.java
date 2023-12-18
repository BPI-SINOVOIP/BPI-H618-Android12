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
import android.app.UiModeManager;
import android.bluetooth.BluetoothAdapter;
import android.content.Context;
import android.net.wifi.WifiManager;
import android.os.SystemClock;
import android.support.test.uiautomator.By;
import android.support.test.uiautomator.BySelector;
import android.support.test.uiautomator.UiObject2;
import android.support.test.uiautomator.UiObjectNotFoundException;
import android.support.test.uiautomator.UiScrollable;
import android.support.test.uiautomator.UiSelector;
import androidx.test.InstrumentationRegistry;

import java.util.List;
import java.util.regex.Pattern;

public class SettingHelperImpl extends AbstractAutoStandardAppHelper implements IAutoSettingHelper {

    private static final String LOG_TAG = SettingHelperImpl.class.getSimpleName();

    // Wait Time
    private static final int UI_RESPONSE_WAIT_MS = 5000;

    private UiModeManager mUiModeManager;
    private Context mContext;

    public SettingHelperImpl(Instrumentation instr) {
        super(instr);
        mUiModeManager =
                InstrumentationRegistry.getInstrumentation()
                        .getContext()
                        .getSystemService(UiModeManager.class);
        mContext = InstrumentationRegistry.getContext();
    }

    /** {@inheritDoc} */
    @Override
    public void open() {
        openFullSettings();
    }

    /** {@inheritDoc} */
    @Override
    public String getPackage() {
        return getApplicationConfig(AutoConfigConstants.SETTINGS_PACKAGE);
    }

    /** {@inheritDoc} */
    @Override
    public void stopSettingsApplication() {
        String cmd =
                String.format(
                        "am force-stop %s",
                        getApplicationConfig(AutoConfigConstants.SETTINGS_PACKAGE));
        executeShellCommand(cmd);
    }

    /** {@inheritDoc} */
    @Override
    public String getLauncherName() {
        return "Settings";
    }

    /** {@inheritDoc} */
    @Override
    public void openSetting(String setting) {
        openFullSettings();
        openMenuWith(getSettingPath(setting));
        verifyAvailableOptions(setting);
    }

    /** {@inheritDoc} */
    @Override
    public UiObject2 findSettingMenu(String setting) {
        UiObject2 menuObject = null;
        String[] menuOptions = getSettingPath(setting);
        int currentIndex = 0;
        int lastIndex = menuOptions.length - 1;
        for (String menu : menuOptions) {
            int scrollableScreenIndex = 0;
            if (hasSplitScreenSettingsUI() && currentIndex > 0) {
                scrollableScreenIndex = 1;
            }
            menuObject = getMenu(menu, scrollableScreenIndex);
            if (currentIndex == lastIndex) {
                return menuObject;
            }
            clickAndWaitForIdleScreen(menuObject);
            waitForIdle();
            currentIndex++;
        }
        return menuObject;
    }

    @Override
    public void findSettingMenuAndClick(String setting) {
        UiObject2 settingMenu = findSettingMenu(setting);
        if (settingMenu != null) {
            clickAndWaitForIdleScreen(settingMenu);
        } else {
            throw new RuntimeException("Unable to find setting menu");
        }
    }

    @Override
    public String getPageTitleText() {
        UiObject2 pageToolbarTitle = getPageTitle();
        return pageToolbarTitle.getText();
    }

    /** {@inheritDoc} */
    @Override
    public void openFullSettings() {
        executeShellCommand(getApplicationConfig(AutoConfigConstants.OPEN_SETTINGS_COMMAND));
    }

    /** {@inheritDoc} */
    @Override
    public void openQuickSettings() {
        pressHome();
        executeShellCommand(getApplicationConfig(AutoConfigConstants.OPEN_QUICK_SETTINGS_COMMAND));
        UiObject2 settingObject =
                findUiObject(
                        getResourceFromConfig(
                                AutoConfigConstants.SETTINGS,
                                AutoConfigConstants.QUICK_SETTINGS,
                                AutoConfigConstants.OPEN_MORE_SETTINGS));
        if (settingObject == null) {
            throw new RuntimeException("Failed to open quick settings.");
        }
    }

    private void verifyAvailableOptions(String setting) {
        String[] expectedOptions = getSettingOptions(setting);
        if (expectedOptions == null) {
            return;
        }
        for (String option : expectedOptions) {
            if (mDevice.hasObject(By.clickable(false).textContains(option))) {
                continue;
            }
            Pattern menuPattern = Pattern.compile(option, Pattern.CASE_INSENSITIVE);
            BySelector selector = By.text(menuPattern);
            int scrollScreenIndex = 0;
            if (hasSplitScreenSettingsUI()) {
                scrollScreenIndex = 1;
            }
            if (scrollAndFindUiObject(selector, scrollScreenIndex) == null) {
                throw new RuntimeException("Cannot find settings option: " + option);
            }
        }
    }

    /** {@inheritDoc} */
    @Override
    public void turnOnOffWifi(boolean onOff) {
        boolean isOn = isWifiOn();
        if (isOn != onOff) {
            UiObject2 enableOption =
                    findUiObject(
                            getResourceFromConfig(
                                    AutoConfigConstants.SETTINGS,
                                    AutoConfigConstants.NETWORK_AND_INTERNET_SETTINGS,
                                    AutoConfigConstants.TOGGLE_WIFI));
            clickAndWaitForWindowUpdate(
                    getApplicationConfig(AutoConfigConstants.SETTINGS_PACKAGE), enableOption);
        } else {
            throw new RuntimeException("Wi-Fi enabled state is already " + (onOff ? "on" : "off"));
        }
    }

    /** {@inheritDoc} */
    @Override
    public boolean isWifiOn() {
        WifiManager wifi = (WifiManager) this.mContext.getSystemService(Context.WIFI_SERVICE);
        return wifi.isWifiEnabled();
    }

    /** {@inheritDoc} */
    @Override
    public void turnOnOffHotspot(boolean onOff) {
        boolean isOn = isHotspotOn();
        if (isOn != onOff) {
            UiObject2 enableOption =
                    findUiObject(
                            getResourceFromConfig(
                                    AutoConfigConstants.SETTINGS,
                                    AutoConfigConstants.NETWORK_AND_INTERNET_SETTINGS,
                                    AutoConfigConstants.TOGGLE_HOTSPOT));
            clickAndWaitForWindowUpdate(
                    getApplicationConfig(AutoConfigConstants.SETTINGS_PACKAGE), enableOption);
        } else {
            throw new RuntimeException(
                    "Hotspot enabled state is already " + (onOff ? "on" : "off"));
        }
    }

    /** {@inheritDoc} */
    @Override
    public boolean isHotspotOn() {
        UiObject2 enableOption =
                findUiObject(
                        getResourceFromConfig(
                                AutoConfigConstants.SETTINGS,
                                AutoConfigConstants.NETWORK_AND_INTERNET_SETTINGS,
                                AutoConfigConstants.TOGGLE_HOTSPOT));
        return enableOption.isChecked();
    }

    /** {@inheritDoc} */
    @Override
    public void turnOnOffBluetooth(boolean onOff) {
        boolean isOn = isBluetoothOn();
        if (isOn != onOff) {
            UiObject2 enableOption =
                    findUiObject(
                            getResourceFromConfig(
                                    AutoConfigConstants.SETTINGS,
                                    AutoConfigConstants.BLUETOOTH_SETTINGS,
                                    AutoConfigConstants.TOGGLE_BLUETOOTH));
            clickAndWaitForWindowUpdate(
                    getApplicationConfig(AutoConfigConstants.SETTINGS_PACKAGE), enableOption);
        } else {
            throw new RuntimeException(
                    "Bluetooth enabled state is already " + (onOff ? "on" : "off"));
        }
    }

    /** {@inheritDoc} */
    @Override
    public boolean isBluetoothOn() {
        BluetoothAdapter ba = BluetoothAdapter.getDefaultAdapter();
        return ba.isEnabled();
    }

    /** {@inheritDoc} */
    @Override
    public void searchAndSelect(String item) {
        searchAndSelect(item, 0);
    }

    /** {@inheritDoc} */
    @Override
    public void searchAndSelect(String item, int selectedIndex) {
        UiObject2 search_button =
                findUiObject(
                        getResourceFromConfig(
                                AutoConfigConstants.SETTINGS,
                                AutoConfigConstants.FULL_SETTINGS,
                                AutoConfigConstants.SEARCH));
        clickAndWaitForIdleScreen(search_button);
        UiObject2 search_box =
                findUiObject(
                        getResourceFromConfig(
                                AutoConfigConstants.SETTINGS,
                                AutoConfigConstants.FULL_SETTINGS,
                                AutoConfigConstants.SEARCH_BOX));
        search_box.setText(item);
        SystemClock.sleep(UI_RESPONSE_WAIT_MS);
        // close the keyboard to reveal all search results.
        mDevice.pressBack();

        UiObject2 searchResults =
                findUiObject(
                        getResourceFromConfig(
                                AutoConfigConstants.SETTINGS,
                                AutoConfigConstants.FULL_SETTINGS,
                                AutoConfigConstants.SEARCH_RESULTS));
        int numberOfResults = searchResults.getChildren().size();
        if (numberOfResults == 0) {
            throw new RuntimeException("No results found");
        }
        clickAndWaitForIdleScreen(searchResults.getChildren().get(selectedIndex));
        SystemClock.sleep(UI_RESPONSE_WAIT_MS);

        UiObject2 object = findUiObject(By.textContains(item));
        if (object == null) {
            throw new RuntimeException("Opened page does not contain searched item");
        }
    }

    /** {@inheritDoc} */
    @Override
    public boolean isValidPageTitle(String item) {
        UiObject2 pageTitle = getPageTitle();
        return pageTitle.getText().contains(item);
    }

    private UiObject2 getPageTitle() {
        BySelector[] selectors =
                new BySelector[] {
                    getResourceFromConfig(
                            AutoConfigConstants.SETTINGS,
                            AutoConfigConstants.FULL_SETTINGS,
                            AutoConfigConstants.PAGE_TITLE),
                    getResourceFromConfig(
                            AutoConfigConstants.SETTINGS,
                            AutoConfigConstants.APPS_SETTINGS,
                            AutoConfigConstants.PERMISSIONS_PAGE_TITLE)
                };
        for (BySelector selector : selectors) {
            List<UiObject2> pageTitles = findUiObjects(selector);
            if (pageTitles != null && pageTitles.size() > 0) {
                return pageTitles.get(pageTitles.size() - 1);
            }
        }
        throw new RuntimeException("Unable to find page title");
    }

    /** {@inheritDoc} */
    @Override
    public void goBackToSettingsScreen() {
        // count is used to avoid infinite loop in case someone invokes
        // after exiting settings application
        int count = 5;
        while (count > 0
                && isAppInForeground()
                && findUiObject(
                                By.text(
                                        getApplicationConfig(
                                                AutoConfigConstants.SETTINGS_TITLE_TEXT)))
                        == null) {
            pressBack();
            SystemClock.sleep(UI_RESPONSE_WAIT_MS); // to avoid stale object error
            count--;
        }
    }

    /** {@inheritDoc} */
    @Override
    public void openMenuWith(String... menuOptions) {
        for (String menu : menuOptions) {
            Pattern menuPattern = Pattern.compile(menu, Pattern.CASE_INSENSITIVE);
            UiObject2 menuButton = scrollAndFindUiObject(By.text(menuPattern));
            if (menuButton == null) {
                throw new RuntimeException("Unable to find menu item");
            }
            clickAndWaitForIdleScreen(menuButton);
            waitForIdle();
        }
    }

    /**
     * Checks whether a setting menu is enabled or not. When not enabled, the menu item cannot be
     * clicked.
     */
    @Override
    public boolean isSettingMenuEnabled(String menu) {
        boolean isSettingMenuEnabled = false;
        String[] menuOptions = getSettingPath(menu);
        int currentIndex = 0;
        int lastIndex = menuOptions.length - 1;
        for (String menuOption : menuOptions) {
            int scrollableScreenIndex = 0;
            if (hasSplitScreenSettingsUI() && currentIndex > 0) {
                scrollableScreenIndex = 1;
            }
            UiObject2 menuObject = getMenu(menuOption, scrollableScreenIndex);
            if (currentIndex == lastIndex) {
                return menuObject.isEnabled();
            }
            if (!menuObject.isEnabled()) {
                return isSettingMenuEnabled;
            }
            clickAndWaitForIdleScreen(menuObject);
            waitForIdle();
            currentIndex++;
        }
        return isSettingMenuEnabled;
    }

    private UiObject2 getMenu(String menu, int index) {
        Pattern menuPattern = Pattern.compile(menu, Pattern.CASE_INSENSITIVE);
        UiObject2 menuButton = scrollAndFindUiObject(By.text(menuPattern), index);
        if (menuButton == null) {
            throw new RuntimeException("Unable to find menu item");
        }
        return menuButton;
    }

    /** {@inheritDoc} */
    @Override
    public int getValue(String setting) {
        String cmd = String.format("settings get system %s", setting);
        String value = executeShellCommand(cmd);
        return Integer.parseInt(value.replaceAll("\\s", ""));
    }

    /** {@inheritDoc} */
    @Override
    public void setValue(String setting, int value) {
        String cmd = String.format("settings put system %s %d", setting, value);
        executeShellCommand(cmd);
    }

    /** {@inheritDoc} */
    @Override
    public void changeSeekbarLevel(int index, ChangeType changeType) {
        try {
            String seekBar =
                    String.format(
                            "%s:id/%s",
                            getResourcePackage(
                                    AutoConfigConstants.SETTINGS,
                                    AutoConfigConstants.DISPLAY_SETTINGS,
                                    AutoConfigConstants.BRIGHTNESS_LEVEL),
                            getResourceValue(
                                    AutoConfigConstants.SETTINGS,
                                    AutoConfigConstants.DISPLAY_SETTINGS,
                                    AutoConfigConstants.BRIGHTNESS_LEVEL));
            UiScrollable seekbar =
                    new UiScrollable(new UiSelector().resourceId(seekBar).instance(index));
            if (changeType == ChangeType.INCREASE) {
                seekbar.scrollForward(1);
            } else {
                seekbar.scrollBackward(1);
            }
            waitForWindowUpdate(getApplicationConfig(AutoConfigConstants.SETTINGS_PACKAGE));
        } catch (UiObjectNotFoundException exception) {
            throw new RuntimeException("Unable to find seekbar");
        }
    }

    /** {@inheritDoc} */
    @Override
    public void setDayNightMode(DayNightMode mode) {
        if (mode == DayNightMode.DAY_MODE
                        && getDayNightModeStatus().getValue() == mUiModeManager.MODE_NIGHT_YES
                || mode == DayNightMode.NIGHT_MODE
                        && getDayNightModeStatus().getValue() != mUiModeManager.MODE_NIGHT_YES) {
            clickAndWaitForWindowUpdate(
                    getApplicationConfig(AutoConfigConstants.SETTINGS_PACKAGE),
                    getNightModeButton());
        }
    }

    private UiObject2 getNightModeButton() {
        UiObject2 nightModeButton =
                scrollAndFindUiObject(
                        getResourceFromConfig(
                                AutoConfigConstants.SETTINGS,
                                AutoConfigConstants.QUICK_SETTINGS,
                                AutoConfigConstants.NIGHT_MODE));
        if (nightModeButton == null) {
            throw new RuntimeException("Unable to find night mode button");
        }
        return nightModeButton.getParent();
    }

    /** {@inheritDoc} */
    @Override
    public DayNightMode getDayNightModeStatus() {
        return mUiModeManager.getNightMode() == mUiModeManager.MODE_NIGHT_YES
                ? DayNightMode.NIGHT_MODE
                : DayNightMode.DAY_MODE;
    }
}
