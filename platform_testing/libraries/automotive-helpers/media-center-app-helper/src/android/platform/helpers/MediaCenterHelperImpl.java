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
import android.app.UiAutomation;
import android.content.Context;
import android.media.session.MediaController;
import android.media.session.MediaSessionManager;
import android.media.session.PlaybackState;
import android.os.SystemClock;
import android.platform.helpers.exceptions.UnknownUiException;
import android.support.test.uiautomator.By;
import android.support.test.uiautomator.Direction;
import android.support.test.uiautomator.UiObject2;
import android.support.test.uiautomator.UiObject;
import android.support.test.uiautomator.UiObjectNotFoundException;
import android.support.test.uiautomator.UiScrollable;
import android.support.test.uiautomator.UiSelector;

import java.util.List;

public class MediaCenterHelperImpl extends AbstractAutoStandardAppHelper
        implements IAutoMediaHelper {
    // Wait Time
    private static final int UI_RESPONSE_WAIT_MS = 5000;
    private static final int SHORT_RESPONSE_WAIT_MS = 1000;

    private static final String MEDIA_LAUNCH_COMMAND =
            "am start -a android.car.intent.action.MEDIA_TEMPLATE -e "
                    + "android.car.intent.extra.MEDIA_COMPONENT ";

    private MediaSessionManager mMediaSessionManager;
    private UiAutomation mUiAutomation;

    public MediaCenterHelperImpl(Instrumentation instr) {
        super(instr);
        mUiAutomation = instr.getUiAutomation();
        mUiAutomation.adoptShellPermissionIdentity("android.permission.MEDIA_CONTENT_CONTROL");
        mMediaSessionManager =
                (MediaSessionManager)
                        instr.getContext().getSystemService(Context.MEDIA_SESSION_SERVICE);
    }

    /** {@inheritDoc} */
    @Override
    public String getPackage() {
        return getApplicationConfig(AutoConfigConstants.MEDIA_CENTER_PACKAGE);
    }

    /** {@inheritDoc} */
    public void open() {
        openMediaApp(getApplicationConfig(AutoConfigConstants.MEDIA_ACTIVITY));
    }

    private void openMediaApp(String packagename) {
        pressHome();
        waitForIdle();
        executeShellCommand(MEDIA_LAUNCH_COMMAND + packagename);
    }

    /** {@inheritDoc} */
    public void playMedia() {
        if (!isPlaying()) {
            UiObject2 playButton =
                    findUiObject(
                            getResourceFromConfig(
                                    AutoConfigConstants.MEDIA_CENTER,
                                    AutoConfigConstants.MEDIA_CENTER_SCREEN,
                                    AutoConfigConstants.PLAY_PAUSE_BUTTON));
            if (playButton == null) {
                throw new UnknownUiException("Unable to find play/pause button");
            }
            clickAndWaitForIdleScreen(playButton);
            SystemClock.sleep(UI_RESPONSE_WAIT_MS);
        }
    }

    /** {@inheritDoc} */
    public void playPauseMediaFromHomeScreen() {
        UiObject2 playButton =
                findUiObject(
                        getResourceFromConfig(
                                AutoConfigConstants.MEDIA_CENTER,
                                AutoConfigConstants.MEDIA_CENTER_ON_HOME_SCREEN,
                                AutoConfigConstants.PLAY_PAUSE_BUTTON));
        if (playButton == null) {
            throw new UnknownUiException("Unable to find play button from home screen");
        }
        clickAndWaitForIdleScreen(playButton);
        SystemClock.sleep(UI_RESPONSE_WAIT_MS);
    }

    /** {@inheritDoc} */
    public void pauseMedia() {
        if (isPlaying()) {
            UiObject2 pauseButton =
                    findUiObject(
                            getResourceFromConfig(
                                    AutoConfigConstants.MEDIA_CENTER,
                                    AutoConfigConstants.MEDIA_CENTER_SCREEN,
                                    AutoConfigConstants.PLAY_PAUSE_BUTTON));
            if (pauseButton == null) {
                throw new UnknownUiException("Unable to find pause button");
            }
            clickAndWaitForIdleScreen(pauseButton);
            SystemClock.sleep(UI_RESPONSE_WAIT_MS);
        }
    }

    /** {@inheritDoc} */
    public void clickNextTrack() {
        UiObject2 nextTrackButton =
                findUiObject(
                        getResourceFromConfig(
                                AutoConfigConstants.MEDIA_CENTER,
                                AutoConfigConstants.MEDIA_CENTER_SCREEN,
                                AutoConfigConstants.NEXT_BUTTON));
        if (nextTrackButton == null) {
            throw new UnknownUiException("Unable to find next track button");
        }
        clickAndWaitForIdleScreen(nextTrackButton);
        SystemClock.sleep(UI_RESPONSE_WAIT_MS);
    }

    /** {@inheritDoc} */
    public void clickNextTrackFromHomeScreen() {
        UiObject2 nextTrackButton =
                findUiObject(
                        getResourceFromConfig(
                                AutoConfigConstants.MEDIA_CENTER,
                                AutoConfigConstants.MEDIA_CENTER_ON_HOME_SCREEN,
                                AutoConfigConstants.NEXT_BUTTON));
        if (nextTrackButton == null) {
            throw new UnknownUiException("Unable to find next track button from home screen");
        }
        clickAndWaitForIdleScreen(nextTrackButton);
        SystemClock.sleep(UI_RESPONSE_WAIT_MS);
    }

    /** {@inheritDoc} */
    public void clickPreviousTrack() {
        UiObject2 previousTrackButton =
                findUiObject(
                        getResourceFromConfig(
                                AutoConfigConstants.MEDIA_CENTER,
                                AutoConfigConstants.MEDIA_CENTER_SCREEN,
                                AutoConfigConstants.PREVIOUS_BUTTON));
        if (previousTrackButton == null) {
            throw new UnknownUiException("Unable to find previous track button");
        }
        clickAndWaitForIdleScreen(previousTrackButton);
        SystemClock.sleep(UI_RESPONSE_WAIT_MS);
    }

    /** {@inheritDoc} */
    public void clickPreviousTrackFromHomeScreen() {
        UiObject2 previousTrackButton =
                findUiObject(
                        getResourceFromConfig(
                                AutoConfigConstants.MEDIA_CENTER,
                                AutoConfigConstants.MEDIA_CENTER_ON_HOME_SCREEN,
                                AutoConfigConstants.PREVIOUS_BUTTON));
        if (previousTrackButton == null) {
            throw new UnknownUiException("Unable to find previous track button");
        }
        clickAndWaitForIdleScreen(previousTrackButton);
        SystemClock.sleep(UI_RESPONSE_WAIT_MS);
    }

    /** {@inheritDoc} */
    public void clickShuffleAll() {
        UiObject2 shufflePlaylistButton =
                findUiObject(
                        getResourceFromConfig(
                                AutoConfigConstants.MEDIA_CENTER,
                                AutoConfigConstants.MEDIA_CENTER_SCREEN,
                                AutoConfigConstants.SHUFFLE_BUTTON));
        if (shufflePlaylistButton == null) {
            throw new UnknownUiException("Unable to find shuffle playlist button");
        }
        clickAndWaitForIdleScreen(shufflePlaylistButton);
    }

    /** Click the nth instance among the visible menu items */
    public void clickMenuItem(int instance) {
        if (!mDevice.hasObject(By.scrollable(true))) {
            // Menu is not open
            return;
        }
        UiScrollable menuList = new UiScrollable(new UiSelector().scrollable(true));
        menuList.setAsVerticalList();
        try {
            UiObject menuListItem =
                    menuList.getChildByInstance(new UiSelector().clickable(true), instance);
            menuListItem.clickAndWaitForNewWindow(UI_RESPONSE_WAIT_MS);
            waitForIdle();
        } catch (UiObjectNotFoundException e) {
            throw new UnknownUiException("Unable to find menu item", e);
        }
    }

    /** {@inheritDoc} */
    public void openMenuWith(String... menuOptions) {
        for (String menu : menuOptions) {
            UiObject2 menuButton = findUiObject(By.text(menu));
            if (menuButton != null) {
                clickAndWaitForIdleScreen(menuButton);
                waitForGone(By.text(menu));
            } else {
                try {
                    UiObject menuItem_object = selectByName(menu);
                    menuItem_object.clickAndWaitForNewWindow();
                } catch (UiObjectNotFoundException exception) {
                    throw new UnknownUiException("Unable to find the menu item");
                }
            }
            SystemClock.sleep(SHORT_RESPONSE_WAIT_MS);
        }
    }

    /** {@inheritDoc} */
    public void openNowPlayingWith(String trackName) {
        UiObject2 nowPlayingButton =
                findUiObject(
                        getResourceFromConfig(
                                AutoConfigConstants.MEDIA_CENTER,
                                AutoConfigConstants.MEDIA_CENTER_SCREEN,
                                AutoConfigConstants.PLAY_QUEUE_BUTTON));
        if (nowPlayingButton == null) {
            throw new UnknownUiException("Unable to find Now playing button");
        }
        clickAndWaitForIdleScreen(nowPlayingButton);
        waitForWindowUpdate(getApplicationConfig(AutoConfigConstants.MEDIA_CENTER_PACKAGE));
        UiObject2 playTrackName = findUiObject(By.text(trackName));
        if (playTrackName != null) {
            clickAndWaitForIdleScreen(playTrackName);
        } else {
            try {
                UiObject menuItem_object = selectByName(trackName);
                menuItem_object.clickAndWaitForNewWindow();
            } catch (UiObjectNotFoundException exception) {
                throw new UnknownUiException("Unable to find the trackname from Now playing list");
            }
        }
    }

    /** {@inheritDoc} */
    public String getMediaTrackName() {
        String track;
        UiObject2 mediaControl =
                findUiObject(
                        getResourceFromConfig(
                                AutoConfigConstants.MEDIA_CENTER,
                                AutoConfigConstants.MEDIA_CENTER_SCREEN,
                                AutoConfigConstants.MINIMIZED_MEDIA_CONTROLS));
        if (mediaControl != null) {
            track = getMediaTrackNameFromMinimizedControl();
        } else {
            track = getMediaTrackNameFromPlayback();
        }
        return track;
    }

    /** {@inheritDoc} */
    public String getMediaTrackNameFromHomeScreen() {
        String trackName;
        UiObject2 trackNameText =
                findUiObject(
                        getResourceFromConfig(
                                AutoConfigConstants.MEDIA_CENTER,
                                AutoConfigConstants.MEDIA_CENTER_ON_HOME_SCREEN,
                                AutoConfigConstants.TRACK_NAME));
        if (trackNameText == null) {
            throw new UnknownUiException("Unable to find track name from Home Screen");
        }
        trackName = trackNameText.getText();
        return trackName;
    }

    private String getMediaTrackNameFromMinimizedControl() {
        String trackName;
        UiObject2 trackNameText =
                findUiObject(
                        getResourceFromConfig(
                                AutoConfigConstants.MEDIA_CENTER,
                                AutoConfigConstants.MEDIA_CENTER_SCREEN,
                                AutoConfigConstants.TRACK_NAME_MINIMIZED_CONTROL));
        if (trackNameText == null) {
            throw new UnknownUiException("Unable to find track name from minimized control");
        }
        trackName = trackNameText.getText();
        return trackName;
    }

    private String getMediaTrackNameFromPlayback() {
        String trackName;
        waitForIdle();
        UiObject2 trackNameText =
                findUiObject(
                        getResourceFromConfig(
                                AutoConfigConstants.MEDIA_CENTER,
                                AutoConfigConstants.MEDIA_CENTER_SCREEN,
                                AutoConfigConstants.TRACK_NAME));
        if (trackNameText == null) {
            throw new UnknownUiException("Unable to find track name from now playing");
        }
        trackName = trackNameText.getText();
        return trackName;
    }

    /** {@inheritDoc} */
    public void goBackToMediaHomePage() {
        minimizeNowPlaying();
        UiObject2 back_btn =
                findUiObject(
                        getResourceFromConfig(
                                AutoConfigConstants.MEDIA_CENTER,
                                AutoConfigConstants.MEDIA_CENTER_SCREEN,
                                AutoConfigConstants.BACK_BUTTON));
        while (back_btn != null) {
            clickAndWaitForIdleScreen(back_btn);
            back_btn =
                    findUiObject(
                            getResourceFromConfig(
                                    AutoConfigConstants.MEDIA_CENTER,
                                    AutoConfigConstants.MEDIA_CENTER_SCREEN,
                                    AutoConfigConstants.BACK_BUTTON));
        }
    }

    /** Minimize the Now Playing window. */
    private void minimizeNowPlaying() {
        UiObject2 trackNameText =
                findUiObject(
                        getResourceFromConfig(
                                AutoConfigConstants.MEDIA_CENTER,
                                AutoConfigConstants.MEDIA_CENTER_SCREEN,
                                AutoConfigConstants.TRACK_NAME));
        if (trackNameText != null) {
            trackNameText.swipe(Direction.DOWN, 1.0f, 500);
        }
    }

    /**
     * Scrolls through the list in search of the provided menu
     *
     * @param menu : menu to search
     * @return UiObject found for the menu searched
     */
    private UiObject selectByName(String menu) throws UiObjectNotFoundException {
        UiObject menuListItem = null;
        UiScrollable menuList = new UiScrollable(new UiSelector().scrollable(true));
        menuList.setAsVerticalList();
        menuListItem =
                menuList.getChildByText(
                        new UiSelector().className(android.widget.TextView.class.getName()), menu);
        mDevice.waitForIdle();
        return menuListItem;
    }

    /** {@inheritDoc} */
    @Override
    public boolean isPlaying() {
        List<MediaController> controllers = mMediaSessionManager.getActiveSessions(null);
        if (controllers.size() == 0) {
            throw new RuntimeException("Unable to find Media Controller");
        }
        PlaybackState state = controllers.get(0).getPlaybackState();
        return state.getState() == PlaybackState.STATE_PLAYING;
    }
}
