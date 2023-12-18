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

import static android.media.AudioAttributes.USAGE_ALARM;
import static android.media.AudioAttributes.USAGE_ASSISTANCE_NAVIGATION_GUIDANCE;
import static android.media.AudioAttributes.USAGE_MEDIA;
import static android.media.AudioAttributes.USAGE_VOICE_COMMUNICATION;

import android.app.Instrumentation;
import android.app.UiAutomation;
import android.car.Car;
import android.car.media.CarAudioManager;
import android.content.Context;
import android.os.SystemClock;
import android.support.test.uiautomator.By;
import android.support.test.uiautomator.UiObject2;
import androidx.test.InstrumentationRegistry;

import java.util.List;

public class SettingsSoundsHelperImpl extends AbstractAutoStandardAppHelper
        implements IAutoSoundsSettingHelper {
    private static final int UI_RESPONSE_WAIT_MS = 5000;
    private static final int SHORT_UI_RESPONSE_TIME = 1000;
    private static final int VOLUME_FLAGS = 0;
    private static final int USAGE_INVALID = -1;
    private static final int MINIMUM_NUMBER_OF_CHILDREN = 2;

    private Context mContext;
    private CarAudioManager mCarAudioManager;
    private final UiAutomation mUiAutomation =
            InstrumentationRegistry.getInstrumentation().getUiAutomation();

    public SettingsSoundsHelperImpl(Instrumentation instr) {
        super(instr);
        mContext = InstrumentationRegistry.getContext();
        Car car = Car.createCar(mContext);
        mUiAutomation.adoptShellPermissionIdentity(
                "android.car.permission.CAR_CONTROL_AUDIO_VOLUME",
                "android.car.permission.CAR_CONTROL_AUDIO_SETTINGS");
        mCarAudioManager = (CarAudioManager) car.getCarManager(Car.AUDIO_SERVICE);
    }

    /** {@inheritDoc} */
    @Override
    public String getPackage() {
        return getApplicationConfig(AutoConfigConstants.SETTINGS_PACKAGE);
    }

    /** {@inheritDoc} */
    @Override
    public void setVolume(VolumeType volumeType, int index) {
        int audioAttribute = USAGE_INVALID;
        switch (volumeType) {
            case MEDIA:
                audioAttribute = USAGE_MEDIA;
                break;
            case ALARM:
                audioAttribute = USAGE_ALARM;
                break;
            case NAVIGATION:
                audioAttribute = USAGE_ASSISTANCE_NAVIGATION_GUIDANCE;
                break;
            case INCALL:
                audioAttribute = USAGE_VOICE_COMMUNICATION;
                break;
        }
        int volumeGroupId = mCarAudioManager.getVolumeGroupIdForUsage(audioAttribute);
        mCarAudioManager.setGroupVolume(volumeGroupId, index, VOLUME_FLAGS);
    }

    /** {@inheritDoc} */
    @Override
    public int getVolume(VolumeType volumeType) {
        int audioAttribute = USAGE_INVALID;
        switch (volumeType) {
            case MEDIA:
                audioAttribute = USAGE_MEDIA;
                break;
            case ALARM:
                audioAttribute = USAGE_ALARM;
                break;
            case NAVIGATION:
                audioAttribute = USAGE_ASSISTANCE_NAVIGATION_GUIDANCE;
                break;
            case INCALL:
                audioAttribute = USAGE_VOICE_COMMUNICATION;
                break;
        }
        int volumeGroupId = mCarAudioManager.getVolumeGroupIdForUsage(audioAttribute);
        int volume = mCarAudioManager.getGroupVolume(volumeGroupId);
        return volume;
    }

    /** {@inheritDoc} */
    @Override
    public void setSound(SoundType soundType, String sound) {
        String type = "";
        switch (soundType) {
            case ALARM:
                type = "Default alarm sound";
                break;
            case NOTIFICATION:
                type = "Default notification sound";
                break;
            case RINGTONE:
                type = "Phone ringtone";
                break;
        }
        UiObject2 object = scrollAndFindUiObject(By.text(type), getScrollScreenIndex());
        String currentSound = getSound(soundType);
        object.click();
        SystemClock.sleep(SHORT_UI_RESPONSE_TIME);
        boolean scrollDown = false;
        if (currentSound.compareToIgnoreCase(sound) < 0) {
            scrollDown = true;
        }
        UiObject2 soundObject = findUiObject(By.text(sound));
        while (soundObject == null) {
            if (scrollDown) {
                scrollDownOnePage(getScrollScreenIndex());
            } else {
                scrollUpOnePage(getScrollScreenIndex());
            }
            soundObject = findUiObject(By.text(sound));
        }
        if (soundObject == null) {
            throw new RuntimeException(String.format("Unable to find sound %s", sound));
        }
        soundObject.click();
        UiObject2 saveButton =
                findUiObject(
                        getResourceFromConfig(
                                AutoConfigConstants.SETTINGS,
                                AutoConfigConstants.SOUND_SETTINGS,
                                AutoConfigConstants.SAVE_BUTTON));
        saveButton.click();
    }

    /** {@inheritDoc} */
    @Override
    public String getSound(SoundType soundType) {
        String type = "";
        switch (soundType) {
            case ALARM:
                type = "Default alarm sound";
                break;
            case NOTIFICATION:
                type = "Default notification sound";
                break;
            case RINGTONE:
                type = "Phone ringtone";
                break;
        }
        UiObject2 object = scrollAndFindUiObject(By.text(type), getScrollScreenIndex());
        List<UiObject2> list = object.getParent().getChildren();
        if (list.size() < 2) {
            scrollDownOnePage(1);
            list = object.getParent().getChildren();
        }
        UiObject2 summary = list.get(1);
        return summary.getText();
    }

    private int getScrollScreenIndex() {
        int scrollScreenIndex = 0;
        if (hasSplitScreenSettingsUI()) {
            scrollScreenIndex = 1;
        }
        return scrollScreenIndex;
    }
}
