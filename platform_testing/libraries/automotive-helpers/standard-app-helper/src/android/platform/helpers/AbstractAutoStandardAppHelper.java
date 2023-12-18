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
import android.content.ActivityNotFoundException;
import android.os.SystemClock;
import android.util.Log;

import android.graphics.Rect;

import android.support.test.uiautomator.By;
import android.support.test.uiautomator.BySelector;
import android.support.test.uiautomator.Direction;
import android.support.test.uiautomator.UiDevice;
import android.support.test.uiautomator.UiObject2;
import android.support.test.uiautomator.Until;

import java.io.IOException;
import java.util.List;
import java.util.regex.Pattern;

public abstract class AbstractAutoStandardAppHelper extends AbstractStandardAppHelper {
    private static final String LOG_TAG = AbstractAutoStandardAppHelper.class.getSimpleName();

    protected Instrumentation mInstrumentation;
    protected UiDevice mDevice;

    private AutoJsonUtility mAutoJsonUtil;

    private static final int UI_RESPONSE_WAIT_MS = 5000;
    private static final float DEFAULT_SCROLL_PERCENT = 100f;
    private static final int DEFAULT_SCROLL_TIME_MS = 500;

    private static final int MAX_SCROLLS = 5;

    public AbstractAutoStandardAppHelper(Instrumentation instrumentation) {
        super(instrumentation);
        mInstrumentation = instrumentation;
        mDevice = UiDevice.getInstance(instrumentation);
        mAutoJsonUtil = AutoJsonUtility.getInstance();
    }

    /** {@inheritDoc} */
    @Override
    public void open() {
        // Launch the application as normal.
        String pkg = getPackage();

        String output = null;
        try {
            Log.i(LOG_TAG, String.format("Sending command to launch: %s", pkg));
            mInstrumentation.getContext().startActivity(getOpenAppIntent());
        } catch (ActivityNotFoundException e) {
            throw new RuntimeException(String.format("Failed to find package: %s", pkg), e);
        }

        // Ensure the package is in the foreground for success.
        if (!mDevice.wait(Until.hasObject(By.pkg(pkg).depth(0)), 30000)) {
            throw new IllegalStateException(
                    String.format("Did not find package, %s, in foreground.", pkg));
        }
    }

    /** {@inheritDoc} */
    @Override
    public void exit() {
        pressHome();
        waitForIdle();
    }

    /** {@inheritDoc} */
    @Override
    public void dismissInitialDialogs() {
        // Nothing to dismiss
    }

    /** {@inheritDoc} */
    @Override
    public String getLauncherName() {
        throw new UnsupportedOperationException("Operation not supported.");
    }

    /** {@inheritDoc} */
    @Override
    public String getPackage() {
        throw new UnsupportedOperationException("Operation not supported.");
    }

    /**
     * Executes a shell command on device, and return the standard output in string.
     *
     * @param command the command to run
     * @return the standard output of the command, or empty string if failed without throwing an
     *     IOException
     */
    protected String executeShellCommand(String command) {
        try {
            return mDevice.executeShellCommand(command);
        } catch (IOException e) {
            // ignore
            Log.e(
                    LOG_TAG,
                    String.format(
                            "The shell command failed to run: %s exception: %s",
                            command, e.getMessage()));
            return "";
        }
    }

    /** Press Home Button on the Device */
    protected void pressHome() {
        mDevice.pressHome();
    }

    /** Press Back Button on the Device */
    protected void pressBack() {
        mDevice.pressBack();
    }

    /** Press Enter Button on the Device */
    protected void pressEnter() {
        mDevice.pressEnter();
    }

    /** Press power button */
    protected void pressPowerButton() {
        executeShellCommand("input keyevent KEYCODE_POWER");
        SystemClock.sleep(UI_RESPONSE_WAIT_MS);
    }

    /** Wait for the device to be idle */
    protected void waitForIdle() {
        mDevice.waitForIdle();
    }

    /** Wait for the given selector to be gone */
    protected void waitForGone(BySelector selector) {
        mDevice.wait(Until.gone(selector), UI_RESPONSE_WAIT_MS);
    }

    /** Wait for window change on the device */
    protected void waitForWindowUpdate(String applicationPackage) {
        mDevice.waitForWindowUpdate(applicationPackage, UI_RESPONSE_WAIT_MS);
    }

    /**
     * Scroll in given direction by specified percent of the whole scrollable region in given time.
     *
     * @param direction The direction in which to perform scrolling, it's either up or down.
     * @param percent The percentage of the whole scrollable region by which to scroll, ranging from
     *     0 - 100. For instance, percent = 50 would scroll up/down by half of the screen.
     * @param timeMs The duration in milliseconds to perform the scrolling gesture.
     * @param index Index required for split screen.
     */
    private boolean scroll(Direction direction, float percent, long timeMs, int index) {
        boolean canScrollMoreInGivenDircetion = false;
        List<UiObject2> upButtons =
                findUiObjects(
                        getResourceFromConfig(
                                AutoConfigConstants.SETTINGS,
                                AutoConfigConstants.FULL_SETTINGS,
                                AutoConfigConstants.UP_BUTTON));
        List<UiObject2> downButtons =
                findUiObjects(
                        getResourceFromConfig(
                                AutoConfigConstants.SETTINGS,
                                AutoConfigConstants.FULL_SETTINGS,
                                AutoConfigConstants.DOWN_BUTTON));
        List<UiObject2> scrollableObjects = findUiObjects(By.scrollable(true));
        if (scrollableObjects == null || upButtons == null || scrollableObjects.size() == 0) {
            return canScrollMoreInGivenDircetion;
        }
        if (upButtons.size() == 1 || (scrollableObjects.size() - 1) < index) {
            // reset index as it is invalid
            index = 0;
        }
        if (upButtons != null) {
            UiObject2 upButton = upButtons.get(index);
            UiObject2 downButton = downButtons.get(index);
            if (direction == Direction.UP) {
                clickAndWaitForIdleScreen(upButton);
            } else if (direction == Direction.DOWN) {
                clickAndWaitForIdleScreen(downButton);
            }
        } else {
            UiObject2 scrollable = scrollableObjects.get(index);
            if (scrollable != null) {
                scrollable.setGestureMargins(
                        getScrollableMargin(scrollable, false), // left
                        getScrollableMargin(scrollable, true), // top
                        getScrollableMargin(scrollable, false), // right
                        getScrollableMargin(scrollable, true)); // bottom
                int scrollSpeed = getScrollSpeed(scrollable, timeMs);
                canScrollMoreInGivenDircetion =
                        scrollable.scroll(direction, percent / 100, scrollSpeed);
            }
        }
        return canScrollMoreInGivenDircetion;
    }

    /**
     * Return the margin for scrolling.
     *
     * @param scrollable The given scrollable object to scroll through.
     * @param isVertical If true, then vertical else horizontal
     */
    private int getScrollableMargin(UiObject2 scrollable, boolean isVertical) {
        Rect bounds = scrollable.getVisibleBounds();
        int margin = (int) (Math.abs(bounds.width()) / 4);
        if (isVertical) {
            margin = (int) (Math.abs(bounds.height()) / 4);
        }
        return margin;
    }

    /**
     * Return the scroll speed such that it takes given time for the device to scroll through the
     * whole scrollable region(i.e. from the top of the scrollable region to bottom).
     *
     * @param scrollable The given scrollable object to scroll through.
     * @param timeMs The duration in milliseconds to perform the scrolling gesture.
     */
    private int getScrollSpeed(UiObject2 scrollable, long timeMs) {
        Rect bounds = scrollable.getVisibleBounds();
        double timeSeconds = (double) timeMs / 1000;
        int scrollSpeed = (int) (bounds.height() / timeSeconds);
        return scrollSpeed;
    }

    /**
     * Scroll down from the top of the scrollable region to bottom of the scrollable region (i.e. by
     * one page).
     */
    public boolean scrollDownOnePage() {
        return scrollDownOnePage(0);
    }

    /**
     * Scroll down from the top of the scrollable region to bottom of the scrollable region (i.e. by
     * one page). Index - required for split screen
     */
    protected boolean scrollDownOnePage(int index) {
        return scroll(Direction.DOWN, DEFAULT_SCROLL_PERCENT, DEFAULT_SCROLL_TIME_MS, index);
    }

    /**
     * Scroll up from the bottom of the scrollable region to top of the scrollable region (i.e. by
     * one page).
     */
    public boolean scrollUpOnePage() {
        return scrollUpOnePage(0);
    }

    /**
     * Scroll up from the bottom of the scrollable region to top of the scrollable region (i.e. by
     * one page). Index - required for split screen
     */
    protected boolean scrollUpOnePage(int index) {
        return scroll(Direction.UP, DEFAULT_SCROLL_PERCENT, DEFAULT_SCROLL_TIME_MS, index);
    }

    /** Find UI Object in a Scrollable view */
    protected UiObject2 scrollAndFindUiObject(BySelector selector) {
        return scrollAndFindUiObject(selector, 0);
    }

    /** Find UI Object in a Scrollable view with Index ( required for split screen ) */
    protected UiObject2 scrollAndFindUiObject(BySelector selector, int index) {
        if (selector == null) {
            return null;
        }
        // Find the object on current page
        UiObject2 uiObject = findUiObject(selector);
        if (uiObject != null) {
            return uiObject;
        }
        // Scroll To Top
        scrollToTop(index);
        // Find UI Object on the first page
        uiObject = findUiObject(selector);
        // Try finding UI Object until it's found or all the pages are checked
        int scrollCount = 0;
        while (uiObject == null && scrollCount < MAX_SCROLLS) {
            // Scroll down to next page
            scrollDownOnePage(index);

            // Find UI Object
            uiObject = findUiObject(selector);

            scrollCount++;
        }
        return uiObject;
    }

    /** Scroll to top of the scrollable region. */
    protected void scrollToTop() {
        scrollToTop(0);
    }

    /** Scroll to top of the scrollable region with index. ( Required for Split Screen ) */
    protected void scrollToTop(int index) {
        int scrollCount = 0;
        while (scrollCount < MAX_SCROLLS) {
            scrollUpOnePage(index);
            scrollCount++;
        }
    }

    /** Find the UI Object that matches the given selector */
    protected UiObject2 findUiObject(BySelector selector) {
        if (selector == null) {
            return null;
        }
        UiObject2 uiObject = mDevice.wait(Until.findObject(selector), UI_RESPONSE_WAIT_MS);
        return uiObject;
    }

    /** Find the list of UI object that matches the given selector */
    protected List<UiObject2> findUiObjects(BySelector selector) {
        if (selector == null) {
            return null;
        }
        List<UiObject2> uiObjects = mDevice.wait(Until.findObjects(selector), UI_RESPONSE_WAIT_MS);
        return uiObjects;
    }

    /** Find the list of UI object that matches the given selector for given depth */
    protected List<UiObject2> findUiObjects(BySelector selector, int depth) {
        if (selector == null) {
            return null;
        }
        List<UiObject2> uiObjects =
                mDevice.wait(Until.findObjects(selector.maxDepth(depth)), UI_RESPONSE_WAIT_MS);
        return uiObjects;
    }

    /**
     * This method is used to click on an UiObject2 and wait for device idle after click.
     *
     * @param uiObject UiObject2 to click.
     */
    protected void clickAndWaitForIdleScreen(UiObject2 uiObject2) {
        uiObject2.click();
        waitForIdle();
    }

    /**
     * This method is used to click on an UiObject2 and wait for window update.
     *
     * @param appPackage application package for window update
     * @param uiObject2 UiObject2 to click.
     */
    protected void clickAndWaitForWindowUpdate(String appPackage, UiObject2 uiObject2) {
        uiObject2.click();
        waitForWindowUpdate(appPackage);
        waitForIdle();
    }

    /**
     * This method is used to click on an UiObject2 and wait until it is gone
     *
     * @param uiObject2 uiObject to be clicked
     * @param selector BySelector to be gone
     */
    protected void clickAndWaitForGone(UiObject2 uiObject2, BySelector selector) {
        uiObject2.click();
        waitForGone(selector);
    }

    /**
     * This method is used check if given object is visible on the device screen
     *
     * @param selector BySelector to be gone
     */
    protected boolean hasUiObject(BySelector selector) {
        return mDevice.hasObject(selector);
    }

    /** Get path for the given setting */
    protected String[] getSettingPath(String setting) {
        return mAutoJsonUtil.getSettingPath(setting);
    }

    /** Get available options for given settings */
    protected String[] getSettingOptions(String setting) {
        return mAutoJsonUtil.getSettingOptions(setting);
    }

    /** Get application config value for given configuration */
    protected String getApplicationConfig(String config) {
        return mAutoJsonUtil.getApplicationConfig(config);
    }

    /** Get resource for given configuration resource in given application */
    protected BySelector getResourceFromConfig(
            String appName, String appConfig, String appResource) {
        AutoConfigResource configResource =
                mAutoJsonUtil.getResourceFromConfig(appName, appConfig, appResource);

        // RESOURCE_ID
        if (configResource != null
                && AutoConfigConstants.RESOURCE_ID.equals(configResource.getResourceType())) {
            return By.res(configResource.getResourcePackage(), configResource.getResourceValue());
        }

        // TEXT
        if (configResource != null
                && AutoConfigConstants.TEXT.equals(configResource.getResourceType())) {
            return By.text(
                    Pattern.compile(configResource.getResourceValue(), Pattern.CASE_INSENSITIVE));
        }

        // TEXT_CONTAINS
        if (configResource != null
                && AutoConfigConstants.TEXT_CONTAINS.equals(configResource.getResourceType())) {
            return By.textContains(configResource.getResourceValue());
        }

        // DESCRIPTION
        if (configResource != null
                && AutoConfigConstants.DESCRIPTION.equals(configResource.getResourceType())) {
            return By.desc(
                    Pattern.compile(configResource.getResourceValue(), Pattern.CASE_INSENSITIVE));
        }

        // CLASS
        if (configResource != null
                && AutoConfigConstants.CLASS.equals(configResource.getResourceType())) {
            if (configResource.getResourcePackage() != null
                    && !configResource.getResourcePackage().isEmpty()) {
                return By.clazz(
                        configResource.getResourcePackage(), configResource.getResourceValue());
            }
            return By.clazz(configResource.getResourceValue());
        }

        return null;
    }

    /** Get resource value for given configuration resource in given application */
    protected String getResourceValue(String appName, String appConfig, String appResource) {
        AutoConfigResource configResource =
                mAutoJsonUtil.getResourceFromConfig(appName, appConfig, appResource);

        if (configResource != null) {
            return configResource.getResourceValue();
        }

        return null;
    }

    /** Get resource package for given configuration resource in given application */
    protected String getResourcePackage(String appName, String appConfig, String appResource) {
        AutoConfigResource configResource =
                mAutoJsonUtil.getResourceFromConfig(appName, appConfig, appResource);

        if (configResource != null) {
            return configResource.getResourcePackage();
        }

        return null;
    }

    /** Check for Split Screen UI in Settings Application */
    protected boolean hasSplitScreenSettingsUI() {
        boolean isSplitScreen = false;
        if ("TRUE"
                .equalsIgnoreCase(
                        mAutoJsonUtil.getApplicationConfig(AutoConfigConstants.SPLIT_SCREEN_UI))) {
            isSplitScreen = true;
        }
        return isSplitScreen;
    }
}
