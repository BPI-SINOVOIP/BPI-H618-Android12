/*
 * Copyright (C) 2018 The Android Open Source Project
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

import android.app.Notification;
import android.support.test.uiautomator.BySelector;
import android.support.test.uiautomator.Direction;
import android.support.test.uiautomator.UiObject2;

import androidx.annotation.Nullable;

/** An App Helper interface for the Notification. */
public interface INotificationHelper extends IAppHelper {

    String UI_PACKAGE_NAME_SYSUI = "com.android.systemui";
    String UI_PACKAGE_NAME_ANDROID = "android";
    String UI_NOTIFICATION_ID = "status_bar_latest_event_content";
    String NOTIFICATION_TITLE_TEXT = "TEST NOTIFICATION";
    String NOTIFICATION_CONTENT_TEXT = "Test notification content";
    String NOTIFICATION_BIG_TEXT =
            "lorem ipsum dolor sit amet\n"
                    + "lorem ipsum dolor sit amet\n"
                    + "lorem ipsum dolor sit amet\n"
                    + "lorem ipsum dolor sit amet";
    String NOTIFICATION_CHANNEL_NAME = "Test Channel";
    String EXPAND_BUTTON_ID = "expand_button";
    String BUBBLE_BUTTON = "bubble_button";
    String APP_ICON_ID = "icon";

    /**
     * Setup expectations: Notification shade opened.
     *
     * <p>Opens a notification from notification shade.
     *
     * @param index The index of the notification to open.
     */
    default void openNotificationbyIndex(int index) {
        throw new UnsupportedOperationException("Not yet implemented.");
    }

    /**
     * Setup Expectations: None
     *
     * <p>Posts a number of notifications to the device. Successive calls to this should post new
     * notifications to those previously posted. Note that this may fail if the helper has surpassed
     * the system-defined limit for per-package notifications.
     *
     * @param count The number of notifications to post.
     */
    default void postNotifications(int count) {
        throw new UnsupportedOperationException("Not yet implemented.");
    }

    /**
     * Setup Expectations: Shade is open
     *
     * <p>Posts a notification using {@link android.app.Notification.BigTextStyle}.
     *
     * @param pkg App to launch, when clicking on notification.
     */
    default UiObject2 postBigTextNotification(@Nullable String pkg) {
        throw new UnsupportedOperationException("Not yet implemented.");
    }

    /**
     * Setup Expectations: Shade is open
     *
     * <p>Posts a notification using {@link android.app.Notification.BigPictureStyle}.
     *
     * @param pkg App to launch, when clicking on notification.
     */
    default UiObject2 postBigPictureNotification(String pkg) {
        throw new UnsupportedOperationException("Not yet implemented.");
    }

    /**
     * Setup Expectations: Shade is open
     *
     * <p>Posts a notification using {@link android.app.Notification.MessagingStyle}.
     *
     * @param pkg App to launch, when clicking on notification.
     */
    default UiObject2 postMessagingStyleNotification(String pkg) {
        throw new UnsupportedOperationException("Not yet implemented.");
    }

    /**
     * Setup Expectations: Shade is open
     *
     * <p>Posts a bubble notification. This notification is associated with a conversation shortcut,
     * a BubbleMetadata, and in {@link android.app.Notification.MessagingStyle}.
     *
     * @param senderName Name of notification sender.
     * @param count How many bubble notifications to send.
     */
    default void postBubbleNotification(String senderName, int count) {
        throw new UnsupportedOperationException("Not yet implemented.");
    }

    /**
     * Return notification if found by text.
     *
     * @param text Text that notification contains.
     */
    default UiObject2 getNotificationByText(String text) {
        throw new UnsupportedOperationException("Not yet implemented.");
    }

    /**
     * Setup Expectations: Shade is open
     *
     * <p>Posts a conversation notification. This notification is associated with a conversation
     * shortcut and in {@link android.app.Notification.MessagingStyle}.
     *
     * @param pkg App to launch, when clicking on notification.
     */
    default UiObject2 postConversationNotification(String pkg) {
        throw new UnsupportedOperationException("Not yet implemented.");
    }

    /**
     * Setup Expectations: None
     *
     * <p>Posts a number of notifications to the device with a package to launch. Successive calls
     * to this should post new notifications in addition to those previously posted. Note that this
     * may fail if the helper has surpassed the system-defined limit for per-package notifications.
     *
     * @param count The number of notifications to post.
     * @param pkg The application that will be launched by notifications.
     */
    default void postNotifications(int count, String pkg) {
        throw new UnsupportedOperationException("Not yet implemented.");
    }

    /**
     * Setup Expectations: None
     *
     * <p>Posts a number of notifications to the device with a package to launch. Successive calls
     * to this should post new notifications in addition to those previously posted. Note that this
     * may fail if the helper has surpassed the system-defined limit for per-package notifications.
     *
     * @param count The number of notifications to post.
     * @param pkg The application that will be launched by notifications.
     * @param interrupting If notification should make sounds and be on top section of the shade.
     */
    default void postNotifications(int count, String pkg, boolean interrupting) {
        throw new UnsupportedOperationException("Not yet implemented.");
    }

    /**
     * Setup Expectations: None
     *
     * <p>Posts notification.
     *
     * @param builder Builder for notification to post.
     */
    default void postNotification(Notification.Builder builder) {
        throw new UnsupportedOperationException("Not yet implemented.");
    }

    /**
     * Setup Expectations: None
     *
     * <p>Cancel any notifications posted by this helper.
     */
    default void cancelNotifications() {
        throw new UnsupportedOperationException("Not yet implemented.");
    }

    /**
     * Setup Expectations: Notification shade opened.
     *
     * <p>Scrolls to the bottom of the notification shade and taps the "clear all" button if
     * present.
     */
    default void clearAllNotifications() {
        throw new UnsupportedOperationException("Not yet implemented.");
    }

    /**
     * Setup Expectations: Notification snoozing is enabled.
     *
     * <p>Empty the notification shade by first cancelling all of the test app's notifications, then
     * snoozing all other notifications temporarily. Fails if any notifications are left in the
     * shade.
     *
     * <p>Because unsnoozing can fail from command line, snoozing is implemented with a fixed time,
     * so tests will take at least as long as the time limit given, plus some buffer built into this
     * utility, to ensure that this test's snoozing will not interfere with other tests.
     */
    default void runWithEmptyNotificationShade(Runnable task, long taskTimeLimit) {
        throw new UnsupportedOperationException("Not yet implemented.");
    }

    /**
     * Setup expectations: Notification shade opened.
     *
     * <p>Opens the first notification by the specified title and checks if the expected application
     * is in foreground or not
     *
     * @param title The title of the notification to open.
     * @param expectedPkg The foreground application after opening a notification. Won't check the
     *     foreground application if the application is null
     */
    default void openNotificationByTitle(String title, String expectedPkg) {
        throw new UnsupportedOperationException("Not yet implemented.");
    }

    /**
     * Setup expectations: Notification shade opened.
     *
     * <p>Taps the chevron or swipes down on the specified notification and checks if the expanded
     * view contains the expected text.
     *
     * @param notification Notification that should be expanded.
     * @param dragging By swiping down when {@code true}, by tapping the chevron otherwise.
     */
    default void expandNotification(UiObject2 notification, boolean dragging) {
        throw new UnsupportedOperationException("Not yet implemented.");
    }

    /**
     * Sets expected app name for notifications.
     *
     * @param appName Package name.
     */
    default void setAppName(String appName) {
        throw new UnsupportedOperationException("Not yet implemented.");
    }

    /**
     * Long press on notification to show its hidden menu (a.k.a. guts)
     *
     * @param notification Notification.
     */
    default void showGuts(UiObject2 notification) {
        throw new UnsupportedOperationException("Not yet implemented.");
    }

    /**
     * Taps the "Done" button on the notification guts.
     *
     * @param notification Notification.
     */
    default void hideGuts(UiObject2 notification) {
        throw new UnsupportedOperationException("Not yet implemented.");
    }

    /**
     * Setup expectations: Notification shade opened.
     *
     * <p>Find the screenshot notification; expand the notification if it's collapsed and click on
     * the "share" button.
     */
    default void shareScreenshotFromNotification(BySelector pageSelector) {
        throw new UnsupportedOperationException("Not yet implemented.");
    }

    /**
     * Setup expectation: On the expanding notification screen.
     *
     * <p>Get the UiObject2 of expanding notification screen.
     */
    default UiObject2 getNotificationShadeScrollContainer() {
        throw new UnsupportedOperationException("Not yet implemented.");
    }

    /**
     * Setup expectation: On the expanding notification screen.
     *
     * <p>Get the UiObject2 of Quick Settings container. Quick settings container works both in
     * expanded and collapsed state, that is contains both QuickQuickSettings and QuickSettings
     */
    default UiObject2 getQuickSettingsContainer() {
        throw new UnsupportedOperationException("Not yet implemented.");
    }

    /**
     * Scroll feeds on Notifications screen
     *
     * <p>Setup expectations: Notification is open with lots of notifications.
     *
     * @param container The container with scrollable elements.
     * @param dir The direction of the fling, must be UP or DOWN.
     */
    default void flingFeed(UiObject2 container, Direction dir) {
        throw new UnsupportedOperationException("Not yet implemented.");
    }

    /**
     * Scroll feeds on Notifications screen and implement it via "fling" API.
     *
     * <p>Setup expectations: Notification is open with lots of notifications.
     *
     * @param container The container with scrollable elements.
     * @param dir The direction of the scroll, must be UP or DOWN.
     * @param speed The speed of fling.
     */
    default void flingFeed(UiObject2 container, Direction dir, int speed) {
        throw new UnsupportedOperationException("Not yet implemented.");
    }

    /**
     * Setup expectation: on the launcher home screen.
     *
     * <p>Open the notification shade by swiping on the home screen.
     */
    default void swipeToOpen() {
        throw new UnsupportedOperationException("Not yet implemented.");
    }

    /**
     * Scroll feeds on Notifications screen and implement it by "swipe" API to control the distance.
     *
     * <p>Setup expectations: Notification drawer is open with lots of notifications.
     *
     * @param container the container with scrollable elements.
     * @param dir the direction to scroll, must be UP or DOWN.
     * @param percent The distance to scroll as a percentage of the page's visible size.
     */
    default void scrollFeed(UiObject2 container, Direction dir, float percent) {
        throw new UnsupportedOperationException("Not yet implemented.");
    }
}
