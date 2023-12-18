/*
 * Copyright 2021 The Android Open Source Project
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

package android.hardware.hdmi.cts;

import static com.google.common.truth.Truth.assertThat;

import static junit.framework.Assert.fail;

import static org.junit.Assume.assumeTrue;

import android.Manifest;
import android.content.Context;
import android.content.pm.PackageManager;
import android.hardware.hdmi.HdmiControlManager;
import android.hardware.hdmi.HdmiDeviceInfo;
import android.hardware.hdmi.HdmiHotplugEvent;
import android.hardware.hdmi.HdmiPlaybackClient;
import android.hardware.hdmi.HdmiSwitchClient;
import android.hardware.hdmi.HdmiTvClient;
import android.os.SystemProperties;

import androidx.test.ext.junit.runners.AndroidJUnit4;
import androidx.test.filters.SmallTest;
import androidx.test.platform.app.InstrumentationRegistry;

import com.android.compatibility.common.util.AdoptShellPermissionsRule;

import org.junit.Before;
import org.junit.Rule;
import org.junit.Test;
import org.junit.runner.RunWith;

import java.util.Arrays;
import java.util.List;
import java.util.concurrent.CountDownLatch;
import java.util.concurrent.Executors;
import java.util.concurrent.TimeUnit;

@RunWith(AndroidJUnit4.class)
@SmallTest
public class HdmiControlManagerTest {

    private static final int DEVICE_TYPE_SWITCH = 6;
    private static final String STRING_DEVICE_TYPE_TV = "tv";
    private static final String STRING_DEVICE_TYPE_PLAYBACK_DEVICE = "playback_device";
    private static final String STRING_DEVICE_TYPE_AUDIO_SYSTEM = "audio_system";
    private static final String STRING_DEVICE_TYPE_PURE_CEC_SWITCH = "pure_cec_switch";

    private static final int TIMEOUT_HOTPLUG_EVENT_SEC = 3;
    private static final int TIMEOUT_CONTENT_CHANGE_SEC = 13;

    private HdmiControlManager mHdmiControlManager;

    @Rule
    public AdoptShellPermissionsRule mAdoptShellPermissionsRule = new AdoptShellPermissionsRule(
            InstrumentationRegistry.getInstrumentation().getUiAutomation(),
            Manifest.permission.HDMI_CEC);

    @Before
    public void setUp() {
        Context context = InstrumentationRegistry.getInstrumentation().getTargetContext();
        assumeTrue(context.getPackageManager().hasSystemFeature(PackageManager.FEATURE_HDMI_CEC));

        mHdmiControlManager = context.getSystemService(HdmiControlManager.class);
    }

    @Test
    public void testHdmiControlManagerAvailable() {
        assertThat(mHdmiControlManager).isNotNull();
    }

    @Test(expected = SecurityException.class)
    public void testHdmiCecPermissionRequired() {
        InstrumentationRegistry.getInstrumentation().getUiAutomation()
                .dropShellPermissionIdentity();

        mHdmiControlManager.getConnectedDevices();
    }

    @Test
    public void testGetHdmiClient() throws Exception {
        String deviceTypesValue = SystemProperties.get("ro.hdmi.cec_device_types");
        if (deviceTypesValue.isEmpty()) {
            deviceTypesValue = SystemProperties.get("ro.hdmi.device_type");
        }

        List<String> deviceTypes = Arrays.asList(deviceTypesValue.split(","));

        if (deviceTypes.contains(String.valueOf(HdmiDeviceInfo.DEVICE_TV))
                || deviceTypes.contains(STRING_DEVICE_TYPE_TV)) {
            assertThat(mHdmiControlManager.getTvClient()).isInstanceOf(HdmiTvClient.class);
            assertThat(mHdmiControlManager.getClient(HdmiDeviceInfo.DEVICE_TV)).isInstanceOf(
                    HdmiTvClient.class);
        }
        if (deviceTypes.contains(String.valueOf(HdmiDeviceInfo.DEVICE_PLAYBACK))
                || deviceTypes.contains(STRING_DEVICE_TYPE_PLAYBACK_DEVICE)) {
            assertThat(mHdmiControlManager.getPlaybackClient()).isInstanceOf(
                    HdmiPlaybackClient.class);
            assertThat(mHdmiControlManager.getClient(HdmiDeviceInfo.DEVICE_PLAYBACK)).isInstanceOf(
                    HdmiPlaybackClient.class);
        }
        if (deviceTypes.contains(String.valueOf(HdmiDeviceInfo.DEVICE_AUDIO_SYSTEM))
                || deviceTypes.contains(STRING_DEVICE_TYPE_AUDIO_SYSTEM)) {
            assertThat(
                    mHdmiControlManager.getClient(HdmiDeviceInfo.DEVICE_AUDIO_SYSTEM)).isNotNull();
        }

        boolean isSwitchDevice = SystemProperties.getBoolean(
                "ro.hdmi.property_is_device_hdmi_cec_switch", false);
        if (deviceTypes.contains(String.valueOf(DEVICE_TYPE_SWITCH))
                || deviceTypes.contains(STRING_DEVICE_TYPE_PURE_CEC_SWITCH)
                || isSwitchDevice) {
            assertThat(mHdmiControlManager.getSwitchClient()).isInstanceOf(HdmiSwitchClient.class);
            assertThat(mHdmiControlManager.getClient(6)).isInstanceOf(HdmiSwitchClient.class);
        }
    }

    @Test
    public void testHdmiClientType() throws Exception {
        String deviceTypesValue = SystemProperties.get("ro.hdmi.cec_device_types");
        if (deviceTypesValue.isEmpty()) {
            deviceTypesValue = SystemProperties.get("ro.hdmi.device_type");
        }

        List<String> deviceTypes = Arrays.asList(deviceTypesValue.split(","));

        if (deviceTypes.contains(String.valueOf(HdmiDeviceInfo.DEVICE_TV))
                || deviceTypes.contains(STRING_DEVICE_TYPE_TV)) {
            assertThat(mHdmiControlManager.getTvClient().getDeviceType()).isEqualTo(
                    HdmiDeviceInfo.DEVICE_TV);
        }
        if (deviceTypes.contains(String.valueOf(HdmiDeviceInfo.DEVICE_PLAYBACK))
                || deviceTypes.contains(STRING_DEVICE_TYPE_PLAYBACK_DEVICE)) {
            assertThat(mHdmiControlManager.getPlaybackClient().getDeviceType()).isEqualTo(
                    HdmiDeviceInfo.DEVICE_PLAYBACK);
        }

        boolean isSwitchDevice = SystemProperties.getBoolean(
                "ro.hdmi.property_is_device_hdmi_cec_switch", false);

        if (deviceTypes.contains(String.valueOf(DEVICE_TYPE_SWITCH))
                || deviceTypes.contains(STRING_DEVICE_TYPE_PURE_CEC_SWITCH)
                || isSwitchDevice) {
            assertThat(mHdmiControlManager.getSwitchClient().getDeviceType()).isEqualTo(
                    DEVICE_TYPE_SWITCH);
        }
    }

    @Test
    public void testHotplugEventListener() throws Exception {
        CountDownLatch notifyLatch = new CountDownLatch(1);
        HdmiControlManager.HotplugEventListener listener =
                new HdmiControlManager.HotplugEventListener() {
                    @Override
                    public void onReceived(HdmiHotplugEvent event) {
                        notifyLatch.countDown();
                    }
                };
        mHdmiControlManager.addHotplugEventListener(listener);
        if (!notifyLatch.await(TIMEOUT_HOTPLUG_EVENT_SEC, TimeUnit.SECONDS)) {
            fail("Timed out waiting for the initial callback");
        }
        mHdmiControlManager.removeHotplugEventListener(listener);
    }

    @Test
    public void testHotplugEventListener_WithCustomExecutor() throws Exception {
        CountDownLatch notifyLatch = new CountDownLatch(1);
        HdmiControlManager.HotplugEventListener listener =
                new HdmiControlManager.HotplugEventListener() {
                    @Override
                    public void onReceived(HdmiHotplugEvent event) {
                        notifyLatch.countDown();
                    }
                };
        mHdmiControlManager.addHotplugEventListener(Executors.newSingleThreadExecutor(),
                listener);
        if (!notifyLatch.await(TIMEOUT_HOTPLUG_EVENT_SEC, TimeUnit.SECONDS)) {
            fail("Timed out waiting for the initial callback");
        }
        mHdmiControlManager.removeHotplugEventListener(listener);
    }

    @Test
    public void testHdmiCecConfig_HdmiCecEnabled() throws Exception {
        // Save original value
        int originalValue = mHdmiControlManager.getHdmiCecEnabled();
        if (!mHdmiControlManager.getUserCecSettings().contains(
                HdmiControlManager.CEC_SETTING_NAME_HDMI_CEC_ENABLED)) {
            return;
        }
        try {
            for (int value : mHdmiControlManager.getAllowedCecSettingIntValues(
                    HdmiControlManager.CEC_SETTING_NAME_HDMI_CEC_ENABLED)) {
                mHdmiControlManager.setHdmiCecEnabled(value);
                assertThat(mHdmiControlManager.getHdmiCecEnabled()).isEqualTo(value);
            }
        } finally {
            // Restore original value
            mHdmiControlManager.setHdmiCecEnabled(originalValue);
            assertThat(mHdmiControlManager.getHdmiCecEnabled()).isEqualTo(originalValue);
        }
    }

    @Test
    public void testHdmiCecConfig_HdmiCecEnabled_ListenerWithCustomExecutor() throws Exception {
        // Save original value
        int originalValue = mHdmiControlManager.getHdmiCecEnabled();
        assumeTrue("Skipping because option not user-modifiable",
                mHdmiControlManager.getUserCecSettings().contains(
                        HdmiControlManager.CEC_SETTING_NAME_HDMI_CEC_ENABLED));
        CountDownLatch notifyLatch1 = new CountDownLatch(1);
        CountDownLatch notifyLatch2 = new CountDownLatch(2);
        HdmiControlManager.CecSettingChangeListener listener =
                new HdmiControlManager.CecSettingChangeListener() {
                    @Override
                    public void onChange(String setting) {
                        notifyLatch1.countDown();
                        notifyLatch2.countDown();
                        assertThat(setting).isEqualTo(
                                HdmiControlManager.CEC_SETTING_NAME_HDMI_CEC_ENABLED);
                    }
                };
        try {
            mHdmiControlManager.setHdmiCecEnabled(HdmiControlManager.HDMI_CEC_CONTROL_DISABLED);
            TimeUnit.SECONDS.sleep(1);
            mHdmiControlManager.addHdmiCecEnabledChangeListener(
                    Executors.newSingleThreadExecutor(), listener);
            TimeUnit.SECONDS.sleep(3);
            mHdmiControlManager.setHdmiCecEnabled(HdmiControlManager.HDMI_CEC_CONTROL_ENABLED);
            if (!notifyLatch1.await(TIMEOUT_CONTENT_CHANGE_SEC, TimeUnit.SECONDS)) {
                fail("Timed out waiting for the notify callback");
            }
            mHdmiControlManager.removeHdmiCecEnabledChangeListener(listener);
            mHdmiControlManager.setHdmiCecEnabled(HdmiControlManager.HDMI_CEC_CONTROL_DISABLED);
            notifyLatch2.await(TIMEOUT_CONTENT_CHANGE_SEC, TimeUnit.SECONDS);
            assertThat(notifyLatch2.getCount()).isEqualTo(1);
        } finally {
            // Remove listener in case not yet removed.
            mHdmiControlManager.removeHdmiCecEnabledChangeListener(listener);
            // Restore original value
            mHdmiControlManager.setHdmiCecEnabled(originalValue);
            assertThat(mHdmiControlManager.getHdmiCecEnabled()).isEqualTo(originalValue);
        }
    }

    @Test
    public void testHdmiCecConfig_HdmiCecEnabled_Listener() throws Exception {
        // Save original value
        int originalValue = mHdmiControlManager.getHdmiCecEnabled();
        assumeTrue("Skipping because option not user-modifiable",
                mHdmiControlManager.getUserCecSettings().contains(
                        HdmiControlManager.CEC_SETTING_NAME_HDMI_CEC_ENABLED));
        CountDownLatch notifyLatch1 = new CountDownLatch(1);
        CountDownLatch notifyLatch2 = new CountDownLatch(2);
        HdmiControlManager.CecSettingChangeListener listener =
                new HdmiControlManager.CecSettingChangeListener() {
                    @Override
                    public void onChange(String setting) {
                        notifyLatch1.countDown();
                        notifyLatch2.countDown();
                        assertThat(setting).isEqualTo(
                                HdmiControlManager.CEC_SETTING_NAME_HDMI_CEC_ENABLED);
                    }
                };
        try {
            mHdmiControlManager.setHdmiCecEnabled(HdmiControlManager.HDMI_CEC_CONTROL_DISABLED);
            TimeUnit.SECONDS.sleep(1);
            mHdmiControlManager.addHdmiCecEnabledChangeListener(listener);
            TimeUnit.SECONDS.sleep(3);
            mHdmiControlManager.setHdmiCecEnabled(HdmiControlManager.HDMI_CEC_CONTROL_ENABLED);
            if (!notifyLatch1.await(TIMEOUT_CONTENT_CHANGE_SEC, TimeUnit.SECONDS)) {
                fail("Timed out waiting for the notify callback");
            }
            mHdmiControlManager.removeHdmiCecEnabledChangeListener(listener);
            mHdmiControlManager.setHdmiCecEnabled(HdmiControlManager.HDMI_CEC_CONTROL_DISABLED);
            notifyLatch2.await(TIMEOUT_CONTENT_CHANGE_SEC, TimeUnit.SECONDS);
            assertThat(notifyLatch2.getCount()).isEqualTo(1);
        } finally {
            // Remove listener in case not yet removed.
            mHdmiControlManager.removeHdmiCecEnabledChangeListener(listener);
            // Restore original value
            mHdmiControlManager.setHdmiCecEnabled(originalValue);
            assertThat(mHdmiControlManager.getHdmiCecEnabled()).isEqualTo(originalValue);
        }
    }

    @Test
    public void testHdmiCecConfig_HdmiCecVersion() throws Exception {
        // Save original value
        int originalValue = mHdmiControlManager.getHdmiCecVersion();
        if (!mHdmiControlManager.getUserCecSettings().contains(
                HdmiControlManager.CEC_SETTING_NAME_HDMI_CEC_VERSION)) {
            return;
        }
        try {
            for (int value : mHdmiControlManager.getAllowedCecSettingIntValues(
                    HdmiControlManager.CEC_SETTING_NAME_HDMI_CEC_VERSION)) {
                mHdmiControlManager.setHdmiCecVersion(value);
                assertThat(mHdmiControlManager.getHdmiCecVersion()).isEqualTo(value);
            }
        } finally {
            // Restore original value
            mHdmiControlManager.setHdmiCecVersion(originalValue);
            assertThat(mHdmiControlManager.getHdmiCecVersion()).isEqualTo(originalValue);
        }
    }

    @Test
    public void testHdmiCecConfig_HdmiCecVolumeControlEnabled() throws Exception {
        // Save original value
        int originalValue = mHdmiControlManager.getHdmiCecVolumeControlEnabled();
        if (!mHdmiControlManager.getUserCecSettings().contains(
                HdmiControlManager.CEC_SETTING_NAME_VOLUME_CONTROL_MODE)) {
            return;
        }
        try {
            for (int value : mHdmiControlManager.getAllowedCecSettingIntValues(
                    HdmiControlManager.CEC_SETTING_NAME_VOLUME_CONTROL_MODE)) {
                mHdmiControlManager.setHdmiCecVolumeControlEnabled(value);
                assertThat(mHdmiControlManager.getHdmiCecVolumeControlEnabled()).isEqualTo(value);
            }
        } finally {
            // Restore original value
            mHdmiControlManager.setHdmiCecVolumeControlEnabled(originalValue);
            assertThat(mHdmiControlManager.getHdmiCecVolumeControlEnabled()).isEqualTo(
                    originalValue);
        }
    }

    @Test
    public void testHdmiCecConfig_PowerControlMode() throws Exception {
        // Save original value
        String originalValue = mHdmiControlManager.getPowerControlMode();
        if (!mHdmiControlManager.getUserCecSettings().contains(
                HdmiControlManager.CEC_SETTING_NAME_POWER_CONTROL_MODE)) {
            return;
        }
        try {
            for (String value : mHdmiControlManager.getAllowedCecSettingStringValues(
                    HdmiControlManager.CEC_SETTING_NAME_POWER_CONTROL_MODE)) {
                mHdmiControlManager.setPowerControlMode(value);
                assertThat(mHdmiControlManager.getPowerControlMode()).isEqualTo(value);
            }
        } finally {
            // Restore original value
            mHdmiControlManager.setPowerControlMode(originalValue);
            assertThat(mHdmiControlManager.getPowerControlMode()).isEqualTo(originalValue);
        }
    }

    @Test
    public void testHdmiCecConfig_PowerStateChangeOnActiveSourceLost() throws Exception {
        // Save original value
        String originalValue = mHdmiControlManager.getPowerStateChangeOnActiveSourceLost();
        if (!mHdmiControlManager.getUserCecSettings().contains(
                HdmiControlManager.CEC_SETTING_NAME_POWER_STATE_CHANGE_ON_ACTIVE_SOURCE_LOST)) {
            return;
        }
        try {
            for (String value : mHdmiControlManager.getAllowedCecSettingStringValues(
                    HdmiControlManager.CEC_SETTING_NAME_POWER_STATE_CHANGE_ON_ACTIVE_SOURCE_LOST)) {
                mHdmiControlManager.setPowerStateChangeOnActiveSourceLost(value);
                assertThat(mHdmiControlManager.getPowerStateChangeOnActiveSourceLost()).isEqualTo(
                        value);
            }
        } finally {
            // Restore original value
            mHdmiControlManager.setPowerStateChangeOnActiveSourceLost(originalValue);
            assertThat(mHdmiControlManager.getPowerStateChangeOnActiveSourceLost()).isEqualTo(
                    originalValue);
        }
    }

    @Test
    public void testHdmiCecConfig_SystemAudioModeMuting() throws Exception {
        // Save original value
        int originalValue = mHdmiControlManager.getSystemAudioModeMuting();
        if (!mHdmiControlManager.getUserCecSettings().contains(
                HdmiControlManager.CEC_SETTING_NAME_SYSTEM_AUDIO_MODE_MUTING)) {
            return;
        }
        try {
            for (int value : mHdmiControlManager.getAllowedCecSettingIntValues(
                    HdmiControlManager.CEC_SETTING_NAME_SYSTEM_AUDIO_MODE_MUTING)) {
                mHdmiControlManager.setSystemAudioModeMuting(value);
                assertThat(mHdmiControlManager.getSystemAudioModeMuting()).isEqualTo(value);
            }
        } finally {
            // Restore original value
            mHdmiControlManager.setSystemAudioModeMuting(originalValue);
            assertThat(mHdmiControlManager.getSystemAudioModeMuting()).isEqualTo(originalValue);
        }
    }

    @Test
    public void testHdmiCecConfig_TvWakeOnOneTouchPlay() throws Exception {
        // Save original value
        int originalValue = mHdmiControlManager.getTvWakeOnOneTouchPlay();
        if (!mHdmiControlManager.getUserCecSettings().contains(
                HdmiControlManager.CEC_SETTING_NAME_TV_WAKE_ON_ONE_TOUCH_PLAY)) {
            return;
        }
        try {
            for (int value : mHdmiControlManager.getAllowedCecSettingIntValues(
                    HdmiControlManager.CEC_SETTING_NAME_TV_WAKE_ON_ONE_TOUCH_PLAY)) {
                mHdmiControlManager.setTvWakeOnOneTouchPlay(value);
                assertThat(mHdmiControlManager.getTvWakeOnOneTouchPlay()).isEqualTo(value);
            }
        } finally {
            // Restore original value
            mHdmiControlManager.setTvWakeOnOneTouchPlay(originalValue);
            assertThat(mHdmiControlManager.getTvWakeOnOneTouchPlay()).isEqualTo(
                    originalValue);
        }
    }

    @Test
    public void testHdmiCecConfig_TvTvSendStandbyOnSleep() throws Exception {
        // Save original value
        int originalValue = mHdmiControlManager.getTvSendStandbyOnSleep();
        if (!mHdmiControlManager.getUserCecSettings().contains(
                HdmiControlManager.CEC_SETTING_NAME_TV_SEND_STANDBY_ON_SLEEP)) {
            return;
        }
        try {
            for (int value : mHdmiControlManager.getAllowedCecSettingIntValues(
                    HdmiControlManager.CEC_SETTING_NAME_TV_SEND_STANDBY_ON_SLEEP)) {
                mHdmiControlManager.setTvSendStandbyOnSleep(value);
                assertThat(mHdmiControlManager.getTvSendStandbyOnSleep()).isEqualTo(value);
            }
        } finally {
            // Restore original value
            mHdmiControlManager.setTvSendStandbyOnSleep(originalValue);
            assertThat(mHdmiControlManager.getTvSendStandbyOnSleep()).isEqualTo(
                    originalValue);
        }
    }
}
