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
import android.support.test.uiautomator.Direction;
import android.support.test.uiautomator.UiObject2;

/** Helper for Notifications on Automotive device */
public class AutoNotificationHelperImpl extends AbstractAutoStandardAppHelper
        implements IAutoNotificationHelper {

    private static final int UI_RESPONSE_WAIT_MS = 1000;

    public AutoNotificationHelperImpl(Instrumentation instr) {
        super(instr);
    }

    /**
     * Setup expectation: None.
     *
     * <p>Open notification, do nothing if notification is already open.
     */
    @Override
    public void open() {
        if (!isAppInForeground()) {
            executeShellCommand(
                    getApplicationConfig(AutoConfigConstants.OPEN_NOTIFICATIONS_COMMAND));
            SystemClock.sleep(UI_RESPONSE_WAIT_MS);
        }
    }

    /**
     * Setup expectations: None
     *
     * <p>Check if notification app is in foreground by checking if the notification list exists.
     */
    @Override
    public boolean isAppInForeground() {
        return (findUiObject(
                        getResourceFromConfig(
                                AutoConfigConstants.NOTIFICATIONS,
                                AutoConfigConstants.EXPANDED_NOTIFICATIONS_SCREEN,
                                AutoConfigConstants.NOTIFICATION_VIEW))
                != null);
    }

    /** {@inheritDoc} */
    @Override
    public void tapClearAllBtn() {
        scrollThroughNotifications();
        if (clearAllBtnExist()) {
            UiObject2 clear_all_btn =
                    findUiObject(
                            getResourceFromConfig(
                                    AutoConfigConstants.NOTIFICATIONS,
                                    AutoConfigConstants.EXPANDED_NOTIFICATIONS_SCREEN,
                                    AutoConfigConstants.CLEAR_ALL_BUTTON));
            clickAndWaitForIdleScreen(clear_all_btn);
        } else {
            throw new RuntimeException("Cannot find Clear All button");
        }
    }

    /** {@inheritDoc} */
    @Override
    public boolean checkNotificationExists(String title) {
        executeShellCommand(getApplicationConfig(AutoConfigConstants.OPEN_NOTIFICATIONS_COMMAND));
        UiObject2 notification_list =
                findUiObject(
                        getResourceFromConfig(
                                        AutoConfigConstants.NOTIFICATIONS,
                                        AutoConfigConstants.EXPANDED_NOTIFICATIONS_SCREEN,
                                        AutoConfigConstants.NOTIFICATION_LIST)
                                .clazz(
                                        getApplicationConfig(
                                                AutoConfigConstants.RECYCLER_VIEW_CLASS)));
        UiObject2 postedNotification = findUiObject(By.text(title));
        // This scrolls the notification list until the notification is found
        // or reaches to the bottom when "Clear All" button is presented.
        while (postedNotification == null && isAppInForeground()) {
            if (clearAllBtnExist()) {
                break;
            }
            notification_list.scroll(Direction.DOWN, 20, 300);
            postedNotification = findUiObject(By.text(title));
        }
        return postedNotification != null;
    }

    /** {@inheritDoc} */
    @Override
    public void removeNotification(String title) {
        waitForGone(By.text(title));
        executeShellCommand(getApplicationConfig(AutoConfigConstants.OPEN_NOTIFICATIONS_COMMAND));
        UiObject2 postedNotification = findUiObject(By.text(title));
        postedNotification.swipe(Direction.LEFT, 1.0f);
    }

    /** {@inheritDoc} */
    @Override
    public void openNotification() {
        UiObject2 statusBar =
                findUiObject(
                        getResourceFromConfig(
                                AutoConfigConstants.NOTIFICATIONS,
                                AutoConfigConstants.EXPANDED_NOTIFICATIONS_SCREEN,
                                AutoConfigConstants.STATUS_BAR));
        statusBar.swipe(Direction.DOWN, 1.0f, 500);
    }

    private void scrollThroughNotifications() {
        executeShellCommand(getApplicationConfig(AutoConfigConstants.OPEN_NOTIFICATIONS_COMMAND));
        UiObject2 notification_list =
                findUiObject(
                        getResourceFromConfig(
                                AutoConfigConstants.NOTIFICATIONS,
                                AutoConfigConstants.EXPANDED_NOTIFICATIONS_SCREEN,
                                AutoConfigConstants.NOTIFICATION_LIST));
        int max_swipe = 5;
        while (max_swipe > 0 && isAppInForeground()) {
            if (clearAllBtnExist()) {
                break;
            } else {
                max_swipe--;
            }
            notification_list.scroll(Direction.DOWN, 20, 300);
        }
    }

    private boolean clearAllBtnExist() {
        UiObject2 clear_all_btn =
                findUiObject(
                        getResourceFromConfig(
                                AutoConfigConstants.NOTIFICATIONS,
                                AutoConfigConstants.EXPANDED_NOTIFICATIONS_SCREEN,
                                AutoConfigConstants.CLEAR_ALL_BUTTON));
        return clear_all_btn != null;
    }
}
