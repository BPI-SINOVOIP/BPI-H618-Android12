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

package com.android.car;

import static com.google.common.truth.Truth.assertThat;

import android.bluetooth.BluetoothProfile;

import org.junit.Test;
import org.junit.runner.RunWith;
import org.junit.runners.Parameterized;

import java.util.Arrays;
import java.util.Collection;

@RunWith(Parameterized.class)
public final class UtilsGetProfileNameTest {

    private final int mProfile;
    private final String mName;

    public UtilsGetProfileNameTest(int profile, String name) {
        mProfile = profile;
        mName = name;
    }

    @Test
    public void testGetProfileName() {
        String result = Utils.getProfileName(mProfile);

        assertThat(result).contains(String.valueOf(mProfile));
        assertThat(result).ignoringCase().contains(mName);
    }

    @Parameterized.Parameters
    public static Collection provideParams() {
        return Arrays.asList(
            new Object[][] {
                {BluetoothProfile.HEADSET, "HFP Server"},
                {BluetoothProfile.A2DP, "A2DP Source"},
                {BluetoothProfile.HEALTH, "HDP"},
                {BluetoothProfile.HID_HOST, "HID Host"},
                {BluetoothProfile.PAN, "PAN"},
                {BluetoothProfile.PBAP, "PBAP Server"},
                {BluetoothProfile.GATT, "GATT Client"},
                {BluetoothProfile.GATT_SERVER, "GATT Server"},
                {BluetoothProfile.MAP, "MAP Server"},
                {BluetoothProfile.SAP, "SAP"},
                {BluetoothProfile.A2DP_SINK, "A2DP Sink"},
                {BluetoothProfile.AVRCP_CONTROLLER, "AVRCP Controller"},
                {BluetoothProfile.AVRCP, "AVRCP Target"},
                {BluetoothProfile.HEADSET_CLIENT, "HFP Client"},
                {BluetoothProfile.PBAP_CLIENT, "PBAP Client"},
                {BluetoothProfile.MAP_CLIENT, "MAP Client"},
                {BluetoothProfile.HID_DEVICE, "HID Device"},
                {BluetoothProfile.OPP, "OPP"},
                {BluetoothProfile.HEARING_AID, "Hearing Aid"},
                {0, "unknown"},
                {22, "unknown"}
            });
    }
}
