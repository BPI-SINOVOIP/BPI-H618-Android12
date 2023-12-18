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
import android.content.ContentResolver;
import android.support.test.uiautomator.By;
import android.support.test.uiautomator.BySelector;
import android.support.test.uiautomator.UiObject;
import android.support.test.uiautomator.UiObject2;
import android.support.test.uiautomator.UiScrollable;
import android.support.test.uiautomator.UiSelector;
import android.text.format.DateFormat;
import java.time.format.TextStyle;
import java.time.LocalDate;
import java.time.Month;
import java.util.List;
import java.util.Locale;

public class SettingsDateTimeHelperImpl extends AbstractAutoStandardAppHelper
        implements IAutoDateTimeSettingsHelper {
    private static final Locale LOCALE = Locale.ENGLISH;
    private static final TextStyle TEXT_STYLE = TextStyle.SHORT;

    public SettingsDateTimeHelperImpl(Instrumentation instr) {
        super(instr);
    }

    /** {@inheritDoc} */
    @Override
    public String getPackage() {
        return getApplicationConfig(AutoConfigConstants.SETTINGS_PACKAGE);
    }

    /** {@inheritDoc} */
    @Override
    public void setDate(LocalDate date) {
        UiObject2 autoDateTimeSwitchWidget = getAutoDateTimeSwitchWidget();
        UiObject2 autoDateTimeMenu =
                getMenu(
                        getResourceFromConfig(
                                AutoConfigConstants.SETTINGS,
                                AutoConfigConstants.DATE_AND_TIME_SETTINGS,
                                AutoConfigConstants.SET_TIME_AUTOMATICALLY));
        if (autoDateTimeSwitchWidget.isChecked()) {
            clickAndWaitForWindowUpdate(
                    getApplicationConfig(AutoConfigConstants.SETTINGS_PACKAGE), autoDateTimeMenu);
        }
        UiObject2 setDateMenu = getSetDateMenu();
        assertTrue("set date menu is not clickable", setDateMenu.isEnabled()); // from UI
        clickAndWaitForWindowUpdate(
                getApplicationConfig(AutoConfigConstants.SETTINGS_PACKAGE), setDateMenu);
        assertTrue(
                "automatic date & time is not switched off",
                !isAutomaticOn("auto_time")); // from API
        int year = date.getYear();
        Month month = date.getMonth();
        int day = date.getDayOfMonth();
        String month_string = month.getDisplayName(TEXT_STYLE, LOCALE);
        String day_string = "";
        if (day < 10) {
            day_string = "0" + String.valueOf(day);
        } else {
            day_string = "" + String.valueOf(day);
        }
        String year_string = "" + year;
        setCalendar(1, day_string);
        setCalendar(0, month_string);
        setCalendar(2, year_string);
        pressBack();
    }

    private void setCalendar(int index, String s) {
        UiSelector selector =
                new UiSelector()
                        .className(
                                getResourceValue(
                                        AutoConfigConstants.SETTINGS,
                                        AutoConfigConstants.DATE_AND_TIME_SETTINGS,
                                        AutoConfigConstants.NUMBER_PICKER_WIDGET))
                        .index(index);
        boolean scrollForwards = true;
        if (index == 2) {
            UiSelector yearSelector =
                    selector.childSelector(
                            new UiSelector()
                                    .className(
                                            getResourceValue(
                                                    AutoConfigConstants.SETTINGS,
                                                    AutoConfigConstants.DATE_AND_TIME_SETTINGS,
                                                    AutoConfigConstants.EDIT_TEXT_WIDGET)));
            String curYear = "";
            try {
                curYear = new UiObject(yearSelector).getText();
            } catch (Exception e) {
                throw new RuntimeException(e);
            }
            if (Integer.valueOf(curYear) > Integer.valueOf(s)) scrollForwards = false;
        }
        scrollToObjectInPicker(index, s, scrollForwards);
    }

    /** {@inheritDoc} */
    @Override
    public LocalDate getDate() {
        UiObject2 obj = getSetDateMenu();
        if (obj == null) {
            throw new RuntimeException("Unable to find set date menu.");
        }
        String uiDate = getMenuSummaryText(obj);
        String[] arr = uiDate.split(" ");
        if (arr.length != 3) {
            throw new RuntimeException("Cannot find date from UI");
        }
        int year = Integer.valueOf(arr[2]);
        int month = Month.valueOf(arr[0].toUpperCase()).getValue();
        int day = Integer.valueOf(arr[1].substring(0, arr[1].length() - 1));
        return LocalDate.of(year, month, day);
    }

    /** {@inheritDoc} */
    @Override
    public void setTimeInTwelveHourFormat(int hour, int minute, boolean am) {
        // check Automatic date & time switch is turned off
        UiObject2 autoDateTimeSwitchWidget = getAutoDateTimeSwitchWidget();
        UiObject2 autoDateTimeMenu =
                getMenu(
                        getResourceFromConfig(
                                AutoConfigConstants.SETTINGS,
                                AutoConfigConstants.DATE_AND_TIME_SETTINGS,
                                AutoConfigConstants.SET_TIME_AUTOMATICALLY));
        if (autoDateTimeSwitchWidget.isChecked()) {
            clickAndWaitForWindowUpdate(
                    getApplicationConfig(AutoConfigConstants.SETTINGS_PACKAGE), autoDateTimeMenu);
        }
        // check 24-hour format switch is turned off
        if (isTwentyFourHourFormatEnabled()) {
            toggleTwentyFourHourFormatSwitch();
        }

        UiObject2 setTimeMenu = getSetTimeMenu();
        assertTrue("set time menu is not clickable", setTimeMenu.isEnabled()); // from UI
        clickAndWaitForWindowUpdate(
                getApplicationConfig(AutoConfigConstants.SETTINGS_PACKAGE), setTimeMenu);
        assertTrue(
                "automatic date & time is not switched off",
                !isAutomaticOn("auto_time")); // from API
        String minute_string = "" + minute;
        String am_pm = "";
        am_pm = am ? "AM" : "PM";
        if (minute < 10) {
            minute_string = "0" + minute;
        }
        setTime(2, minute_string);
        setTime(0, String.valueOf(hour));
        setTime(1, am_pm);
        pressBack();
    }

    /** {@inheritDoc} */
    @Override
    public void setTimeInTwentyFourHourFormat(int hour, int minute) {
        // check Automatic date & time switch is turned off
        UiObject2 autoDateTimeSwitchWidget = getAutoDateTimeSwitchWidget();
        UiObject2 autoDateTimeMenu =
                getMenu(
                        getResourceFromConfig(
                                AutoConfigConstants.SETTINGS,
                                AutoConfigConstants.DATE_AND_TIME_SETTINGS,
                                AutoConfigConstants.SET_TIME_AUTOMATICALLY));
        if (autoDateTimeSwitchWidget.isChecked()) {
            clickAndWaitForWindowUpdate(
                    getApplicationConfig(AutoConfigConstants.SETTINGS_PACKAGE), autoDateTimeMenu);
        }
        // check 24-hour format switch is turned on
        if (!isTwentyFourHourFormatEnabled()) {
            toggleTwentyFourHourFormatSwitch();
        }

        UiObject2 setTimeMenu = getSetTimeMenu();
        assertTrue("set time menu is not clickable", setTimeMenu.isEnabled()); // from UI
        clickAndWaitForWindowUpdate(
                getApplicationConfig(AutoConfigConstants.SETTINGS_PACKAGE), setTimeMenu);
        assertTrue(
                "automatic date & time is not switched off",
                !isAutomaticOn("auto_time")); // from API
        String minute_string = "" + minute;
        if (minute < 10) {
            minute_string = "0" + minute;
        }
        String hour_string = "" + hour;
        if (hour < 10) {
            hour_string = "0" + hour;
        }
        setTime(2, minute_string);
        setTime(0, hour_string);
        pressBack();
    }

    private void setTime(int index, String s) {
        UiSelector selector =
                new UiSelector()
                        .className(
                                getResourceValue(
                                        AutoConfigConstants.SETTINGS,
                                        AutoConfigConstants.DATE_AND_TIME_SETTINGS,
                                        AutoConfigConstants.NUMBER_PICKER_WIDGET))
                        .index(index);
        boolean scrollForwards = true;
        String curAM_PM;
        if (index == 1) {
            UiSelector am_pm_Selector =
                    selector.childSelector(
                            new UiSelector()
                                    .className(
                                            getResourceValue(
                                                    AutoConfigConstants.SETTINGS,
                                                    AutoConfigConstants.DATE_AND_TIME_SETTINGS,
                                                    AutoConfigConstants.EDIT_TEXT_WIDGET)));
            try {
                curAM_PM = new UiObject(am_pm_Selector).getText();
            } catch (Exception e) {
                throw new RuntimeException(e);
            }
            if (curAM_PM.equals("PM")) scrollForwards = false;
        }
        scrollToObjectInPicker(index, s, scrollForwards);
    }

    private void scrollToObjectInPicker(int index, String s, boolean scrollForwards) {
        UiSelector selector =
                new UiSelector()
                        .className(
                                getResourceValue(
                                        AutoConfigConstants.SETTINGS,
                                        AutoConfigConstants.DATE_AND_TIME_SETTINGS,
                                        AutoConfigConstants.NUMBER_PICKER_WIDGET))
                        .index(index);
        UiScrollable scrollable = new UiScrollable(selector);
        scrollable.setAsVerticalList();
        UiObject2 obj =
                findUiObject(
                        By.text(s)
                                .clazz(
                                        getResourceValue(
                                                AutoConfigConstants.SETTINGS,
                                                AutoConfigConstants.DATE_AND_TIME_SETTINGS,
                                                AutoConfigConstants.EDIT_TEXT_WIDGET)));
        while (obj == null) {
            try {
                if (scrollForwards) {
                    scrollable.scrollForward();
                } else {
                    scrollable.scrollBackward();
                }
            } catch (Exception e) {
                throw new RuntimeException(e);
            }
            obj =
                    findUiObject(
                            By.text(s)
                                    .clazz(
                                            getResourceValue(
                                                    AutoConfigConstants.SETTINGS,
                                                    AutoConfigConstants.DATE_AND_TIME_SETTINGS,
                                                    AutoConfigConstants.EDIT_TEXT_WIDGET)));
        }
        if (obj == null) throw new RuntimeException("cannot find value in the picker");
    }

    /** {@inheritDoc} */
    @Override
    public String getTime() {
        UiObject2 obj = getSetTimeMenu();
        if (obj == null) {
            throw new RuntimeException("Unable to find time menu.");
        }
        String uiTime = getMenuSummaryText(obj);
        return uiTime;
    }

    /** {@inheritDoc} */
    @Override
    public void setTimeZone(String timezone) {
        UiObject2 autoTimeZoneSwitchWidget = getAutoTimeZoneSwitchWidget();
        UiObject2 autoTimeZoneMenu =
                getMenu(
                        getResourceFromConfig(
                                AutoConfigConstants.SETTINGS,
                                AutoConfigConstants.DATE_AND_TIME_SETTINGS,
                                AutoConfigConstants.SET_TIME_ZONE_AUTOMATICALLY));
        if (getAutoTimeZoneSwitchWidget().isChecked()) {
            clickAndWaitForWindowUpdate(
                    getApplicationConfig(AutoConfigConstants.SETTINGS_PACKAGE), autoTimeZoneMenu);
        }
        UiObject2 selectTimeZoneMenu = getSelectTimeZoneMenu();
        assertTrue(
                "select time zone menu is not clickable",
                selectTimeZoneMenu.isEnabled()); // from UI
        assertTrue(
                "automatic time zone is not switched off",
                !isAutomaticOn("auto_time_zone")); // from API
        clickAndWaitForWindowUpdate(
                getApplicationConfig(AutoConfigConstants.SETTINGS_PACKAGE), selectTimeZoneMenu);
        BySelector selector = By.clickable(true).hasDescendant(By.text(timezone));
        UiObject2 object = scrollAndFindUiObject(selector, getScrollScreenIndex());
        if (object == null) {
            throw new RuntimeException(String.format("Unable to find timezone %s", timezone));
        }
        clickAndWaitForWindowUpdate(
                getApplicationConfig(AutoConfigConstants.SETTINGS_PACKAGE), object);
    }

    /** {@inheritDoc} */
    @Override
    public boolean toggleTwentyFourHourFormatSwitch() {
        UiObject2 twentyFourHourFormatSwitch = getUseTwentyFourHourFormatSwitchWidget();
        if (twentyFourHourFormatSwitch.isChecked()) {
            assertTrue(
                    "System time format is different from UI format",
                    DateFormat.is24HourFormat(mInstrumentation.getContext()));
        } else {
            assertTrue(
                    "System time format is different from UI format",
                    !DateFormat.is24HourFormat(mInstrumentation.getContext()));
        }
        clickAndWaitForWindowUpdate(
                getApplicationConfig(AutoConfigConstants.SETTINGS_PACKAGE),
                twentyFourHourFormatSwitch);
        return true;
    }

    /** {@inheritDoc} */
    @Override
    public String getTimeZone() {
        UiObject2 obj = getSelectTimeZoneMenu();
        if (obj == null) {
            throw new RuntimeException("Unable to find timezone menu.");
        }
        String timeZone = getMenuSummaryText(obj);
        return timeZone;
    }

    /** {@inheritDoc} */
    @Override
    public boolean isTwentyFourHourFormatEnabled() {
        UiObject2 twentyFourHourFormatSwitchWidget = getUseTwentyFourHourFormatSwitchWidget();
        return twentyFourHourFormatSwitchWidget.isChecked();
    }

    private UiObject2 getSetDateMenu() {
        return getMenu(
                getResourceFromConfig(
                        AutoConfigConstants.SETTINGS,
                        AutoConfigConstants.DATE_AND_TIME_SETTINGS,
                        AutoConfigConstants.SET_DATE));
    }

    private UiObject2 getSetTimeMenu() {
        return getMenu(
                getResourceFromConfig(
                        AutoConfigConstants.SETTINGS,
                        AutoConfigConstants.DATE_AND_TIME_SETTINGS,
                        AutoConfigConstants.SET_TIME));
    }

    private UiObject2 getTwentyFourFormatMenu() {
        return getMenu(
                getResourceFromConfig(
                        AutoConfigConstants.SETTINGS,
                        AutoConfigConstants.DATE_AND_TIME_SETTINGS,
                        AutoConfigConstants.USE_24_HOUR_FORMAT));
    }

    private UiObject2 getSelectTimeZoneMenu() {
        return getMenu(
                getResourceFromConfig(
                        AutoConfigConstants.SETTINGS,
                        AutoConfigConstants.DATE_AND_TIME_SETTINGS,
                        AutoConfigConstants.SELECT_TIME_ZONE));
    }

    private UiObject2 getMenu(BySelector bySelector) {
        BySelector selector = By.clickable(true).hasDescendant(bySelector);
        return scrollAndFindUiObject(selector, getScrollScreenIndex());
    }

    private UiObject2 getAutoDateTimeSwitchWidget() {
        return getSwitchWidget(
                getResourceFromConfig(
                        AutoConfigConstants.SETTINGS,
                        AutoConfigConstants.DATE_AND_TIME_SETTINGS,
                        AutoConfigConstants.SET_TIME_AUTOMATICALLY));
    }

    private UiObject2 getAutoTimeZoneSwitchWidget() {
        return getSwitchWidget(
                getResourceFromConfig(
                        AutoConfigConstants.SETTINGS,
                        AutoConfigConstants.DATE_AND_TIME_SETTINGS,
                        AutoConfigConstants.SET_TIME_ZONE_AUTOMATICALLY));
    }

    private UiObject2 getUseTwentyFourHourFormatSwitchWidget() {
        return getSwitchWidget(
                getResourceFromConfig(
                        AutoConfigConstants.SETTINGS,
                        AutoConfigConstants.DATE_AND_TIME_SETTINGS,
                        AutoConfigConstants.USE_24_HOUR_FORMAT));
    }

    private UiObject2 getSwitchWidget(BySelector bySelector) {
        BySelector selector = By.hasDescendant(bySelector);
        UiObject2 object = scrollAndFindUiObject(selector, getScrollScreenIndex());
        List<UiObject2> list = object.getParent().getChildren();
        UiObject2 switchWidget = list.get(1).getChildren().get(0);
        return switchWidget;
    }

    private int getScrollScreenIndex() {
        int scrollScreenIndex = 0;
        if (hasSplitScreenSettingsUI()) {
            scrollScreenIndex = 1;
        }
        return scrollScreenIndex;
    }

    private String getMenuSummaryText(UiObject2 obj) {
        return obj.getChildren().get(0).getChildren().get(1).getText();
    }

    private boolean isAutomaticOn(String name) {
        ContentResolver cr = mInstrumentation.getContext().getContentResolver();
        int status = 0;
        try {
            status = android.provider.Settings.Global.getInt(cr, name);
        } catch (Exception e) {
            throw new RuntimeException(e);
        }
        return status == 1 ? true : false;
    }
}
