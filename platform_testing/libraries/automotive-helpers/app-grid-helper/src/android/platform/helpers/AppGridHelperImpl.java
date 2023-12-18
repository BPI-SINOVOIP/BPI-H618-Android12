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

import android.os.SystemClock;
import android.app.Instrumentation;
import android.support.test.uiautomator.By;
import android.support.test.uiautomator.BySelector;
import android.support.test.uiautomator.UiObject2;

public class AppGridHelperImpl extends AbstractAutoStandardAppHelper implements IAutoAppGridHelper {

    private static final String PAGE_UP_BUTTON_ID = "page_up";
    private static final String PAGE_DOWN_BUTTON_ID = "page_down";

    private static final int UI_RESPONSE_WAIT_MS = 5000;

    private final BySelector PAGE_UP_BUTTON =
            By.res(getApplicationConfig(AutoConfigConstants.APP_GRID_PACKAGE), PAGE_UP_BUTTON_ID);
    private final BySelector PAGE_DOWN_BUTTON =
            By.res(getApplicationConfig(AutoConfigConstants.APP_GRID_PACKAGE), PAGE_DOWN_BUTTON_ID);

    public AppGridHelperImpl(Instrumentation instr) {
        super(instr);
    }

    /** {@inheritDoc} */
    @Override
    public String getPackage() {
        return getApplicationConfig(AutoConfigConstants.APP_GRID_PACKAGE);
    }

    /**
     * Setup expectation: None.
     *
     * <p>Open app grid by pressing app grid facet ony if app grid is not open.
     */
    @Override
    public void open() {
        if (!isAppInForeground()) {
            executeShellCommand(getApplicationConfig(AutoConfigConstants.OPEN_APP_GRID_COMMAND));
            SystemClock.sleep(UI_RESPONSE_WAIT_MS);
        }
    }

    /**
     * Setup expectations: None
     *
     * <p>Check if app grid is in foreground.
     */
    @Override
    public boolean isAppInForeground() {
        return (findUiObject(
                        getResourceFromConfig(
                                AutoConfigConstants.APP_GRID,
                                AutoConfigConstants.APP_GRID_VIEW,
                                AutoConfigConstants.APP_GRID_VIEW_ID))
                != null);
    }

    /**
     * Setup expectation: None.
     *
     * <p>Exit app grid by pressing home facet only if app grid is open.
     */
    @Override
    public void exit() {
        if (isAppInForeground()) {
            mDevice.pressHome();
        }
    }

    @Override
    public void openApp(String appName) {
        UiObject2 app = scrollAndFindUiObject(By.text(appName));
        if (app != null) {
            clickAndWaitForIdleScreen(app);
        } else {
            throw new IllegalStateException(String.format("App %s cannot be found", appName));
        }
    }

    /** {@inherticDoc} */
    @Override
    public boolean isTop() {
        if (isAppInForeground()) {
            UiObject2 pageUp = mDevice.findObject(PAGE_UP_BUTTON);
            if (pageUp != null) {
                return !pageUp.isEnabled();
            } else {
                // Number of apps fits in one page, at top by default
                return true;
            }
        } else {
            throw new IllegalStateException("App grid is not open.");
        }
    }

    /** {@inherticDoc} */
    @Override
    public boolean isBottom() {
        if (isAppInForeground()) {
            UiObject2 pageDown = mDevice.findObject(PAGE_DOWN_BUTTON);
            if (pageDown != null) {
                return !pageDown.isEnabled();
            } else {
                // Number of apps fits in one page, at bottom by default
                return true;
            }
        } else {
            throw new IllegalStateException("App grid is not open.");
        }
    }
}
