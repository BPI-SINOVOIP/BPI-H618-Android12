/*
 * Copyright 2020 The Android Open Source Project
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

package android.hdmicec.cts.common;

import static com.google.common.truth.Truth.assertWithMessage;

import static org.junit.Assert.fail;

import android.hdmicec.cts.BaseHdmiCecCtsTest.CecRules;
import android.hdmicec.cts.HdmiCecConstants;

import com.android.tradefed.config.Option;
import com.android.tradefed.config.OptionClass;
import com.android.tradefed.device.DeviceNotAvailableException;
import com.android.tradefed.device.ITestDevice;
import com.android.tradefed.testtype.DeviceJUnit4ClassRunner;
import com.android.tradefed.testtype.junit4.BaseHostJUnit4Test;

import org.junit.Rule;
import org.junit.Test;
import org.junit.rules.RuleChain;
import org.junit.runner.RunWith;

import java.util.ArrayList;
import java.util.Arrays;
import java.util.List;
import java.util.Scanner;

/** Tests to see that a valid HDMI CEC device type is declared by the device. */
@RunWith(DeviceJUnit4ClassRunner.class)
@OptionClass(alias="hdmi-cec-cts-test")
public final class HdmiCecDeviceTypeTest extends BaseHostJUnit4Test {

    @Rule
    public RuleChain ruleChain = RuleChain.outerRule(CecRules.requiresLeanback(this));

    @Option(name = HdmiCecConstants.PHYSICAL_ADDRESS_NAME,
        description = "HDMI CEC physical address of the DUT",
        mandatory = false)
    /*
     * This variable is defined only to enable the physical address to be passed as a CLI argument
     * to the module, it won't be used.
     */
    public static int dutPhysicalAddress = HdmiCecConstants.DEFAULT_PHYSICAL_ADDRESS;

    int deviceTvOnly = setBit(HdmiCecConstants.CEC_DEVICE_TYPE_TV);
    int devicePlaybackOnly = setBit(HdmiCecConstants.CEC_DEVICE_TYPE_PLAYBACK_DEVICE);
    int deviceComboTvSwitch =
            setBit(HdmiCecConstants.CEC_DEVICE_TYPE_TV)
                    | setBit(HdmiCecConstants.CEC_DEVICE_TYPE_SWITCH);
    int deviceComboPlaybackAudioSystem =
            setBit(HdmiCecConstants.CEC_DEVICE_TYPE_PLAYBACK_DEVICE)
                    | setBit(HdmiCecConstants.CEC_DEVICE_TYPE_AUDIO_SYSTEM);
    int deviceComboPlaybackAudioSystemSwitch =
            setBit(HdmiCecConstants.CEC_DEVICE_TYPE_PLAYBACK_DEVICE)
                    | setBit(HdmiCecConstants.CEC_DEVICE_TYPE_AUDIO_SYSTEM)
                    | setBit(HdmiCecConstants.CEC_DEVICE_TYPE_SWITCH);

    private final List<Integer> allowedDeviceCombos =
            new ArrayList<>(
                    Arrays.asList(
                            deviceTvOnly,
                            devicePlaybackOnly,
                            deviceComboTvSwitch,
                            deviceComboPlaybackAudioSystem,
                            deviceComboPlaybackAudioSystemSwitch));

    /**
     * Tests that the device declares a valid HDMI CEC device type.
     */
    @Test
    public void checkHdmiCecDeviceType() throws Exception {
        int deviceTypes = getAllDeviceTypes(getDevice());

        assertWithMessage("Incorrect device combination")
                .that(deviceTypes)
                .isIn(allowedDeviceCombos);
    }

    private int getAllDeviceTypes(ITestDevice device) {
        int deviceTypes = 0;
        String deviceType = "";
        boolean usingStringDeviceTypes = true;
        try {
            deviceType = device.executeShellCommand("getprop ro.hdmi.cec_device_types").trim();
            if (deviceType.isEmpty()) {
                usingStringDeviceTypes = false;
                deviceType = device.executeShellCommand("getprop ro.hdmi.device_type").trim();
            }
        } catch (DeviceNotAvailableException dnae) {
            return 0;
        }

        String[] cecDevices = deviceType.split(",");
        for (String cecDevice : cecDevices) {
            if (!cecDevice.equals("")) {
                if (usingStringDeviceTypes) {
                    deviceTypes |= setBit(stringToIntDeviceType(cecDevice));
                } else {
                    deviceTypes |= setBit(Integer.parseInt(cecDevice));
                }
            }
        }

        return deviceTypes;
    }

    private int setBit(int value) {
        return (1 << value);
    }

    private int stringToIntDeviceType(String value) {
        switch (value) {
            case "tv":
                return HdmiCecConstants.CEC_DEVICE_TYPE_TV;
            case "recording_device":
                return HdmiCecConstants.CEC_DEVICE_TYPE_RECORDER;
            case "reserved":
                return HdmiCecConstants.CEC_DEVICE_TYPE_RESERVED;
            case "tuner":
                return HdmiCecConstants.CEC_DEVICE_TYPE_TUNER;
            case "playback_device":
                return HdmiCecConstants.CEC_DEVICE_TYPE_PLAYBACK_DEVICE;
            case "audio_system":
                return HdmiCecConstants.CEC_DEVICE_TYPE_AUDIO_SYSTEM;
            case "pure_cec_switch":
                return HdmiCecConstants.CEC_DEVICE_TYPE_SWITCH;
            case "video_processor":
                return HdmiCecConstants.CEC_DEVICE_TYPE_VIDEO_PROCESSOR;
            default:
                fail("Unrecognized device type: " + value);
                return 0; // Prevent compiler error
        }
    }
}
