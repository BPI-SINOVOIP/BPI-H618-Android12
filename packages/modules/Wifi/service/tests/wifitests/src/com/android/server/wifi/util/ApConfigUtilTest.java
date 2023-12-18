/*
 * Copyright (C) 2016 The Android Open Source Project
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

package com.android.server.wifi.util;

import static org.junit.Assert.assertEquals;
import static org.junit.Assert.assertFalse;
import static org.junit.Assert.assertNull;
import static org.junit.Assert.assertTrue;
import static org.junit.Assume.assumeTrue;
import static org.mockito.ArgumentMatchers.anyInt;
import static org.mockito.Mockito.mock;
import static org.mockito.Mockito.when;

import android.content.Context;
import android.content.res.Resources;
import android.net.MacAddress;
import android.net.wifi.CoexUnsafeChannel;
import android.net.wifi.ScanResult;
import android.net.wifi.SoftApCapability;
import android.net.wifi.SoftApConfiguration;
import android.net.wifi.SoftApConfiguration.Builder;
import android.net.wifi.WifiConfiguration;
import android.net.wifi.WifiManager;
import android.net.wifi.WifiScanner;
import android.util.SparseIntArray;

import androidx.test.filters.SmallTest;

import com.android.modules.utils.build.SdkLevel;
import com.android.server.wifi.WifiBaseTest;
import com.android.server.wifi.WifiNative;
import com.android.server.wifi.coex.CoexManager;
import com.android.wifi.resources.R;

import org.junit.Before;
import org.junit.Test;
import org.mockito.Mock;
import org.mockito.MockitoAnnotations;

import java.util.ArrayList;
import java.util.Arrays;

/**
 * Unit tests for {@link com.android.server.wifi.util.ApConfigUtil}.
 */
@SmallTest
public class ApConfigUtilTest extends WifiBaseTest {

    private static final String TEST_COUNTRY_CODE = "TestCountry";

    /**
     * Frequency to channel map. This include some frequencies used outside the US.
     * Representing it using a vector (instead of map) for simplification.  Values at
     * even indices are frequencies and odd indices are channels.
     */
    private static final int[] FREQUENCY_TO_CHANNEL_MAP = {
            2412, SoftApConfiguration.BAND_2GHZ, 1,
            2417, SoftApConfiguration.BAND_2GHZ, 2,
            2422, SoftApConfiguration.BAND_2GHZ, 3,
            2427, SoftApConfiguration.BAND_2GHZ, 4,
            2432, SoftApConfiguration.BAND_2GHZ, 5,
            2437, SoftApConfiguration.BAND_2GHZ, 6,
            2442, SoftApConfiguration.BAND_2GHZ, 7,
            2447, SoftApConfiguration.BAND_2GHZ, 8,
            2452, SoftApConfiguration.BAND_2GHZ, 9,
            2457, SoftApConfiguration.BAND_2GHZ, 10,
            2462, SoftApConfiguration.BAND_2GHZ, 11,
            /* 12, 13 are only legitimate outside the US. */
            2467, SoftApConfiguration.BAND_2GHZ, 12,
            2472, SoftApConfiguration.BAND_2GHZ, 13,
            /* 14 is for Japan, DSSS and CCK only. */
            2484, SoftApConfiguration.BAND_2GHZ, 14,
            /* 34 valid in Japan. */
            5170, SoftApConfiguration.BAND_5GHZ, 34,
            5180, SoftApConfiguration.BAND_5GHZ, 36,
            5190, SoftApConfiguration.BAND_5GHZ, 38,
            5200, SoftApConfiguration.BAND_5GHZ, 40,
            5210, SoftApConfiguration.BAND_5GHZ, 42,
            5220, SoftApConfiguration.BAND_5GHZ, 44,
            5230, SoftApConfiguration.BAND_5GHZ, 46,
            5240, SoftApConfiguration.BAND_5GHZ, 48,
            5260, SoftApConfiguration.BAND_5GHZ, 52,
            5280, SoftApConfiguration.BAND_5GHZ, 56,
            5300, SoftApConfiguration.BAND_5GHZ, 60,
            5320, SoftApConfiguration.BAND_5GHZ, 64,
            5500, SoftApConfiguration.BAND_5GHZ, 100,
            5520, SoftApConfiguration.BAND_5GHZ, 104,
            5540, SoftApConfiguration.BAND_5GHZ, 108,
            5560, SoftApConfiguration.BAND_5GHZ, 112,
            5580, SoftApConfiguration.BAND_5GHZ, 116,
            /* 120, 124, 128 valid in Europe/Japan. */
            5600, SoftApConfiguration.BAND_5GHZ, 120,
            5620, SoftApConfiguration.BAND_5GHZ, 124,
            5640, SoftApConfiguration.BAND_5GHZ, 128,
            /* 132+ valid in US. */
            5660, SoftApConfiguration.BAND_5GHZ, 132,
            5680, SoftApConfiguration.BAND_5GHZ, 136,
            5700, SoftApConfiguration.BAND_5GHZ, 140,
            /* 144 is supported by a subset of WiFi chips. */
            5720, SoftApConfiguration.BAND_5GHZ, 144,
            5745, SoftApConfiguration.BAND_5GHZ, 149,
            5765, SoftApConfiguration.BAND_5GHZ, 153,
            5785, SoftApConfiguration.BAND_5GHZ, 157,
            5805, SoftApConfiguration.BAND_5GHZ, 161,
            5825, SoftApConfiguration.BAND_5GHZ, 165,
            5845, SoftApConfiguration.BAND_5GHZ, 169,
            5865, SoftApConfiguration.BAND_5GHZ, 173,
            /* Now some 6GHz channels */
            5955, SoftApConfiguration.BAND_6GHZ, 1,
            5970, SoftApConfiguration.BAND_6GHZ, 4,
            6110, SoftApConfiguration.BAND_6GHZ, 32,
            /* some 60GHz channels */
            58320, SoftApConfiguration.BAND_60GHZ, 1,
            60480, SoftApConfiguration.BAND_60GHZ, 2,
            62640, SoftApConfiguration.BAND_60GHZ, 3,
            64800, SoftApConfiguration.BAND_60GHZ, 4,
            66960, SoftApConfiguration.BAND_60GHZ, 5,
            69120, SoftApConfiguration.BAND_60GHZ, 6,
    };

    private static final int[] EMPTY_CHANNEL_LIST = {};
    private static final int[] ALLOWED_2G_FREQS = {2462}; //ch# 11
    private static final int[] ALLOWED_5G_FREQS = {5745, 5765}; //ch# 149, 153
    private static final int[] ALLOWED_6G_FREQS = {5945, 5965};
    private static final int[] ALLOWED_60G_FREQS = {58320, 60480}; // ch# 1, 2

    @Mock Context mContext;
    @Mock Resources mResources;
    @Mock WifiNative mWifiNative;
    @Mock CoexManager mCoexManager;

    /**
     * Setup test.
     */
    @Before
    public void setUp() throws Exception {
        MockitoAnnotations.initMocks(this);
    }

    /**
     * verify convert WifiConfiguration Band to SoftApConfigurationBand.
     */
    @Test
    public void convertWifiConfigBandToSoftapConfigBandTest() throws Exception {
        assertEquals(SoftApConfiguration.BAND_2GHZ, ApConfigUtil
                .convertWifiConfigBandToSoftApConfigBand(WifiConfiguration.AP_BAND_2GHZ));
        assertEquals(SoftApConfiguration.BAND_5GHZ, ApConfigUtil
                .convertWifiConfigBandToSoftApConfigBand(WifiConfiguration.AP_BAND_5GHZ));
        assertEquals(SoftApConfiguration.BAND_2GHZ | SoftApConfiguration.BAND_5GHZ, ApConfigUtil
                .convertWifiConfigBandToSoftApConfigBand(WifiConfiguration.AP_BAND_ANY));
    }



    /**
     * Verify isMultiband success
     */
    @Test
    public void isMultibandSuccess() throws Exception {
        assertTrue(ApConfigUtil.isMultiband(SoftApConfiguration.BAND_2GHZ
                  | SoftApConfiguration.BAND_6GHZ));
        assertTrue(ApConfigUtil.isMultiband(SoftApConfiguration.BAND_5GHZ
                  | SoftApConfiguration.BAND_6GHZ));
        assertTrue(ApConfigUtil.isMultiband(SoftApConfiguration.BAND_2GHZ
                  | SoftApConfiguration.BAND_6GHZ));
        assertTrue(ApConfigUtil.isMultiband(SoftApConfiguration.BAND_2GHZ
                  | SoftApConfiguration.BAND_5GHZ | SoftApConfiguration.BAND_6GHZ));
    }

    /**
     * Verify isMultiband failure
     */
    @Test
    public void isMultibandFailure() throws Exception {
        assertFalse(ApConfigUtil.isMultiband(SoftApConfiguration.BAND_2GHZ));
        assertFalse(ApConfigUtil.isMultiband(SoftApConfiguration.BAND_5GHZ));
        assertFalse(ApConfigUtil.isMultiband(SoftApConfiguration.BAND_6GHZ));
    }

    /**
     * Verify containsBand success
     */
    @Test
    public void containsBandSuccess() throws Exception {
        assertTrue(ApConfigUtil.containsBand(SoftApConfiguration.BAND_2GHZ,
                SoftApConfiguration.BAND_2GHZ));
        assertTrue(ApConfigUtil.containsBand(SoftApConfiguration.BAND_2GHZ
                | SoftApConfiguration.BAND_6GHZ, SoftApConfiguration.BAND_2GHZ));
        assertTrue(ApConfigUtil.containsBand(SoftApConfiguration.BAND_2GHZ
                | SoftApConfiguration.BAND_5GHZ | SoftApConfiguration.BAND_6GHZ,
                SoftApConfiguration.BAND_6GHZ));
    }

    /**
     * Verify containsBand failure
     */
    @Test
    public void containsBandFailure() throws Exception {
        assertFalse(ApConfigUtil.containsBand(SoftApConfiguration.BAND_2GHZ
                  | SoftApConfiguration.BAND_5GHZ, SoftApConfiguration.BAND_6GHZ));
        assertFalse(ApConfigUtil.containsBand(SoftApConfiguration.BAND_5GHZ,
                  SoftApConfiguration.BAND_6GHZ));
    }

    /**
     * Verify isBandValidSuccess
     */
    @Test
    public void isBandValidSuccess() throws Exception {
        assertTrue(ApConfigUtil.isBandValid(SoftApConfiguration.BAND_2GHZ));
        assertTrue(ApConfigUtil.isBandValid(SoftApConfiguration.BAND_2GHZ
                  | SoftApConfiguration.BAND_6GHZ));
        assertTrue(ApConfigUtil.isBandValid(SoftApConfiguration.BAND_2GHZ
                  | SoftApConfiguration.BAND_5GHZ | SoftApConfiguration.BAND_6GHZ));
    }

    /**
     * Verify isBandValidFailure
     */
    @Test
    public void isBandValidFailure() throws Exception {
        assertFalse(ApConfigUtil.isBandValid(0));
        assertFalse(ApConfigUtil.isBandValid(SoftApConfiguration.BAND_2GHZ
                  | SoftApConfiguration.BAND_6GHZ | 0x1F));
    }

    /**
     * verify frequency to band conversion for all possible frequencies.
     */
    @Test
    public void convertFrequencytoBand() throws Exception {
        for (int i = 0; i < FREQUENCY_TO_CHANNEL_MAP.length; i += 3) {
            assertEquals(FREQUENCY_TO_CHANNEL_MAP[i + 1],
                    ApConfigUtil.convertFrequencyToBand(
                            FREQUENCY_TO_CHANNEL_MAP[i]));
        }
    }

    /**
     * verify channel/band to frequency conversion for all possible channels.
     */
    @Test
    public void convertChannelToFrequency() throws Exception {
        for (int i = 0; i < FREQUENCY_TO_CHANNEL_MAP.length; i += 3) {
            assertEquals(FREQUENCY_TO_CHANNEL_MAP[i],
                    ApConfigUtil.convertChannelToFrequency(
                            FREQUENCY_TO_CHANNEL_MAP[i + 2], FREQUENCY_TO_CHANNEL_MAP[i + 1]));
        }
    }

    /**
     * Test convert string to channel list
     */
    @Test
    public void testConvertStringToChannelList() throws Exception {
        assertEquals(Arrays.asList(1, 6, 11), ApConfigUtil.convertStringToChannelList("1, 6, 11"));
        assertEquals(Arrays.asList(1, 6, 11), ApConfigUtil.convertStringToChannelList("1,6,11"));
        assertEquals(Arrays.asList(1, 9, 10, 11),
                ApConfigUtil.convertStringToChannelList("1, 9-11"));
        assertEquals(Arrays.asList(1, 6, 7, 10, 11),
                ApConfigUtil.convertStringToChannelList("1,6-7, 10-11"));
        assertEquals(Arrays.asList(1, 11),
                ApConfigUtil.convertStringToChannelList("1,6a,11"));
        assertEquals(Arrays.asList(1, 11), ApConfigUtil.convertStringToChannelList("1,6-3,11"));
        assertEquals(Arrays.asList(1),
                ApConfigUtil.convertStringToChannelList("1, abc , def - rsv"));
    }

    /**
     * Test get available channel freq for band
     */
    /**
     * Verify default channel is used when picking a 2G channel without
     * any allowed 2G channels.
     */
    @Test
    public void chooseApChannel2GBandWithNoAllowedChannel() throws Exception {
        int[] allowed2gChannels = {};
        when(mWifiNative.getChannelsForBand(WifiScanner.WIFI_BAND_24_GHZ))
                .thenReturn(allowed2gChannels);
        int freq = ApConfigUtil.chooseApChannel(SoftApConfiguration.BAND_2GHZ, mWifiNative,
                mCoexManager, mResources);
        assertEquals(ApConfigUtil.DEFAULT_AP_CHANNEL,
                ScanResult.convertFrequencyMhzToChannelIfSupported(freq));
    }

    /**
     * Verify a 2G channel is selected from the list of allowed channels.
     */
    @Test
    public void chooseApChannel2GBandWithAllowedChannels() throws Exception {
        when(mResources.getString(R.string.config_wifiSoftap2gChannelList))
                .thenReturn("1, 6, 11");
        when(mWifiNative.getChannelsForBand(WifiScanner.WIFI_BAND_24_GHZ))
                .thenReturn(ALLOWED_2G_FREQS); // ch#11

        int freq = ApConfigUtil.chooseApChannel(SoftApConfiguration.BAND_2GHZ, mWifiNative,
                mCoexManager, mResources);
        assertEquals(2462, freq);
    }

    /**
     * Verify a 5G channel is selected from the list of allowed channels.
     */
    @Test
    public void chooseApChannel5GBandWithAllowedChannels() throws Exception {
        when(mResources.getString(R.string.config_wifiSoftap5gChannelList))
                .thenReturn("149, 36-100");
        when(mWifiNative.getChannelsForBand(WifiScanner.WIFI_BAND_5_GHZ))
                .thenReturn(ALLOWED_5G_FREQS); //ch# 149, 153

        int freq = ApConfigUtil.chooseApChannel(
                SoftApConfiguration.BAND_5GHZ, mWifiNative, mCoexManager, mResources);
        assertTrue(ArrayUtils.contains(ALLOWED_5G_FREQS, freq));
    }

    /**
     * Verify a 60G channel is selected from the list of allowed channels.
     */
    @Test
    public void chooseApChannel60GBandWithAllowedChannels() throws Exception {
        when(mResources.getString(R.string.config_wifiSoftap60gChannelList))
                .thenReturn("1-2");
        when(mWifiNative.getChannelsForBand(WifiScanner.WIFI_BAND_60_GHZ))
                .thenReturn(ALLOWED_60G_FREQS); //ch# 1, 2

        int freq = ApConfigUtil.chooseApChannel(
                SoftApConfiguration.BAND_60GHZ, mWifiNative, mCoexManager, mResources);
        assertTrue(ArrayUtils.contains(ALLOWED_60G_FREQS, freq));
    }

    /**
     * Verify chooseApChannel failed when selecting a channel in 5GHz band
     * with no channels allowed.
     */
    @Test
    public void chooseApChannel5GBandWithNoAllowedChannels() throws Exception {
        when(mWifiNative.getChannelsForBand(WifiScanner.WIFI_BAND_5_GHZ))
                .thenReturn(EMPTY_CHANNEL_LIST);
        assertEquals(-1, ApConfigUtil.chooseApChannel(SoftApConfiguration.BAND_5GHZ, mWifiNative,
                mCoexManager, mResources));
    }

    /**
     * Verify chooseApChannel will select high band channel.
     */
    @Test
    public void chooseApChannelWillHighBandPrefer() throws Exception {
        when(mResources.getString(R.string.config_wifiSoftap2gChannelList))
                .thenReturn("1, 6, 11");
        when(mWifiNative.getChannelsForBand(WifiScanner.WIFI_BAND_24_GHZ))
                .thenReturn(ALLOWED_2G_FREQS); // ch#11
        when(mResources.getString(R.string.config_wifiSoftap5gChannelList))
                .thenReturn("149, 153");
        when(mWifiNative.getChannelsForBand(WifiScanner.WIFI_BAND_5_GHZ))
                .thenReturn(ALLOWED_5G_FREQS); //ch# 149, 153

        int freq = ApConfigUtil.chooseApChannel(
                SoftApConfiguration.BAND_2GHZ | SoftApConfiguration.BAND_5GHZ,
                mWifiNative, mCoexManager, mResources);
        assertTrue(ArrayUtils.contains(ALLOWED_5G_FREQS, freq));
    }

    /**
     * Verify chooseSoftAp will select a high band safe channel over a higher band unsafe channel.
     */
    @Test
    public void chooseApChannelWithUnsafeChannelsPreferSafe() throws Exception {
        assumeTrue(SdkLevel.isAtLeastS());
        when(mResources.getString(R.string.config_wifiSoftap2gChannelList))
            .thenReturn("1, 6, 11");
        when(mWifiNative.getChannelsForBand(WifiScanner.WIFI_BAND_24_GHZ))
                .thenReturn(ALLOWED_2G_FREQS); // ch#11
        when(mResources.getString(R.string.config_wifiSoftap5gChannelList))
                .thenReturn("149, 153");
        when(mWifiNative.getChannelsForBand(WifiScanner.WIFI_BAND_5_GHZ))
                .thenReturn(ALLOWED_5G_FREQS); //ch# 149, 153
        when(mCoexManager.getCoexUnsafeChannels()).thenReturn(Arrays.asList(
                new CoexUnsafeChannel(WifiScanner.WIFI_BAND_5_GHZ, 149),
                new CoexUnsafeChannel(WifiScanner.WIFI_BAND_5_GHZ, 153)
        ));
        // Test with soft unsafe channels
        when(mCoexManager.getCoexRestrictions()).thenReturn(0);

        int freq = ApConfigUtil.chooseApChannel(
                SoftApConfiguration.BAND_2GHZ | SoftApConfiguration.BAND_5GHZ,
                mWifiNative, mCoexManager, mResources);

        assertTrue(ArrayUtils.contains(ALLOWED_2G_FREQS, freq));

        // Test with hard unsafe channels
        when(mCoexManager.getCoexRestrictions()).thenReturn(WifiManager.COEX_RESTRICTION_SOFTAP);

        freq = ApConfigUtil.chooseApChannel(
                SoftApConfiguration.BAND_2GHZ | SoftApConfiguration.BAND_5GHZ,
                mWifiNative, mCoexManager, mResources);

        assertTrue(ArrayUtils.contains(ALLOWED_2G_FREQS, freq));
    }

    /**
     * Verify chooseSoftAp will select a high band unsafe channel if all channels are soft unsafe.
     */
    @Test
    public void chooseApChannelWithAllSoftUnsafePreferHighBand() throws Exception {
        assumeTrue(SdkLevel.isAtLeastS());
        when(mResources.getString(R.string.config_wifiSoftap2gChannelList))
                .thenReturn("1, 6, 11");
        when(mWifiNative.getChannelsForBand(WifiScanner.WIFI_BAND_24_GHZ))
                .thenReturn(ALLOWED_2G_FREQS); // ch#11
        when(mResources.getString(R.string.config_wifiSoftap5gChannelList))
                .thenReturn("149, 153");
        when(mWifiNative.getChannelsForBand(WifiScanner.WIFI_BAND_5_GHZ))
                .thenReturn(ALLOWED_5G_FREQS); //ch# 149, 153
        when(mCoexManager.getCoexUnsafeChannels()).thenReturn(Arrays.asList(
                new CoexUnsafeChannel(WifiScanner.WIFI_BAND_24_GHZ, 1),
                new CoexUnsafeChannel(WifiScanner.WIFI_BAND_24_GHZ, 6),
                new CoexUnsafeChannel(WifiScanner.WIFI_BAND_24_GHZ, 11),
                new CoexUnsafeChannel(WifiScanner.WIFI_BAND_5_GHZ, 149),
                new CoexUnsafeChannel(WifiScanner.WIFI_BAND_5_GHZ, 153)
        ));
        when(mCoexManager.getCoexRestrictions()).thenReturn(0);

        int freq = ApConfigUtil.chooseApChannel(
                SoftApConfiguration.BAND_2GHZ | SoftApConfiguration.BAND_5GHZ,
                mWifiNative, mCoexManager, mResources);

        assertTrue(ArrayUtils.contains(ALLOWED_5G_FREQS, freq));
    }

    /**
     * Verify chooseSoftAp will select the default channel if all allowed channels are hard unsafe.
     */
    @Test
    public void chooseApChannelWithAllHardUnsafeSelectDefault() throws Exception {
        assumeTrue(SdkLevel.isAtLeastS());
        when(mResources.getString(R.string.config_wifiSoftap2gChannelList))
                .thenReturn("1, 6, 11");
        when(mWifiNative.getChannelsForBand(WifiScanner.WIFI_BAND_24_GHZ))
                .thenReturn(ALLOWED_2G_FREQS); // ch#11
        when(mResources.getString(R.string.config_wifiSoftap5gChannelList))
                .thenReturn("149, 153");
        when(mWifiNative.getChannelsForBand(WifiScanner.WIFI_BAND_5_GHZ))
                .thenReturn(ALLOWED_5G_FREQS); //ch# 149, 153
        when(mCoexManager.getCoexUnsafeChannels()).thenReturn(Arrays.asList(
                new CoexUnsafeChannel(WifiScanner.WIFI_BAND_24_GHZ, 1),
                new CoexUnsafeChannel(WifiScanner.WIFI_BAND_24_GHZ, 6),
                new CoexUnsafeChannel(WifiScanner.WIFI_BAND_24_GHZ, 11),
                new CoexUnsafeChannel(WifiScanner.WIFI_BAND_5_GHZ, 149),
                new CoexUnsafeChannel(WifiScanner.WIFI_BAND_5_GHZ, 153)
        ));
        when(mCoexManager.getCoexRestrictions()).thenReturn(WifiManager.COEX_RESTRICTION_SOFTAP);

        int freq = ApConfigUtil.chooseApChannel(
                SoftApConfiguration.BAND_2GHZ | SoftApConfiguration.BAND_5GHZ,
                mWifiNative, mCoexManager, mResources);

        assertEquals(freq, ApConfigUtil.convertChannelToFrequency(
                ApConfigUtil.DEFAULT_AP_CHANNEL, ApConfigUtil.DEFAULT_AP_BAND));
    }

    /**
     * Verify default band and channel is used when HAL support is
     * not available.
     */
    @Test
    public void updateApChannelConfigWithoutHal() throws Exception {
        Builder configBuilder = new SoftApConfiguration.Builder();
        configBuilder.setChannel(36, SoftApConfiguration.BAND_5GHZ);

        when(mWifiNative.isHalStarted()).thenReturn(false);
        assertEquals(ApConfigUtil.SUCCESS,
                ApConfigUtil.updateApChannelConfig(mWifiNative, mCoexManager, mResources,
                        TEST_COUNTRY_CODE, configBuilder, configBuilder.build(), false));
        /* Verify default band and channel is used. */
        assertEquals(ApConfigUtil.DEFAULT_AP_BAND, configBuilder.build().getBand());
        assertEquals(ApConfigUtil.DEFAULT_AP_CHANNEL, configBuilder.build().getChannel());
    }

    /**
     * Verify updateApChannelConfig will return an error when selecting channel
     * for 5GHz band without country code.
     */
    @Test
    public void updateApChannelConfig5GBandNoCountryCode() throws Exception {
        Builder configBuilder = new SoftApConfiguration.Builder();
        configBuilder.setBand(SoftApConfiguration.BAND_5GHZ);
        when(mWifiNative.isHalStarted()).thenReturn(true);
        assertEquals(ApConfigUtil.ERROR_GENERIC,
                ApConfigUtil.updateApChannelConfig(mWifiNative, mCoexManager, mResources, null,
                        configBuilder, configBuilder.build(), false));
    }

    /**
     * Verify the AP band and channel is not updated if specified.
     */
    @Test
    public void updateApChannelConfigWithChannelSpecified() throws Exception {
        Builder configBuilder = new SoftApConfiguration.Builder();
        configBuilder.setChannel(36, SoftApConfiguration.BAND_5GHZ);
        when(mWifiNative.isHalStarted()).thenReturn(true);
        assertEquals(ApConfigUtil.SUCCESS,
                ApConfigUtil.updateApChannelConfig(mWifiNative, mCoexManager, mResources,
                        TEST_COUNTRY_CODE, configBuilder, configBuilder.build(), false));
        assertEquals(SoftApConfiguration.BAND_5GHZ, configBuilder.build().getBand());
        assertEquals(36, configBuilder.build().getChannel());
    }

    /**
     * Verify updateApChannelConfig will return an error when selecting 5GHz channel
     * without any allowed channels.
     */
    @Test
    public void updateApChannelConfigWith5GBandNoChannelAllowed() throws Exception {
        Builder configBuilder = new SoftApConfiguration.Builder();
        configBuilder.setBand(SoftApConfiguration.BAND_5GHZ);
        when(mWifiNative.isHalStarted()).thenReturn(true);
        when(mWifiNative.getChannelsForBand(WifiScanner.WIFI_BAND_5_GHZ))
                .thenReturn(EMPTY_CHANNEL_LIST);
        assertEquals(ApConfigUtil.ERROR_NO_CHANNEL,
                ApConfigUtil.updateApChannelConfig(mWifiNative, mCoexManager, mResources,
                        TEST_COUNTRY_CODE, configBuilder, configBuilder.build(), false));
    }

    /**
     * Verify updateApChannelConfig will select a channel number that meets OEM restriction
     * when acs is disabled.
     */
    @Test
    public void updateApChannelConfigWithAcsDisabledOemConfigured() throws Exception {
        Builder configBuilder = new SoftApConfiguration.Builder();
        configBuilder.setBand(SoftApConfiguration.BAND_5GHZ | SoftApConfiguration.BAND_2GHZ);
        when(mResources.getString(R.string.config_wifiSoftap2gChannelList))
                .thenReturn("6");
        when(mResources.getString(R.string.config_wifiSoftap5gChannelList))
                .thenReturn("149, 36-100");
        when(mWifiNative.isHalStarted()).thenReturn(true);
        when(mWifiNative.getChannelsForBand(WifiScanner.WIFI_BAND_24_GHZ))
                .thenReturn(ALLOWED_2G_FREQS); // ch# 11
        when(mWifiNative.getChannelsForBand(WifiScanner.WIFI_BAND_5_GHZ))
                .thenReturn(ALLOWED_5G_FREQS); // ch# 149, 153
        when(mWifiNative.isHalStarted()).thenReturn(true);
        assertEquals(ApConfigUtil.SUCCESS,
                ApConfigUtil.updateApChannelConfig(mWifiNative, mCoexManager, mResources,
                        TEST_COUNTRY_CODE, configBuilder, configBuilder.build(), false));
        assertEquals(SoftApConfiguration.BAND_5GHZ, configBuilder.build().getBand());
        assertEquals(149, configBuilder.build().getChannel());
    }

    /**
     * Verify updateApChannelConfig will not select a channel number and band when acs is
     * enabled.
     */
    @Test
    public void updateApChannelConfigWithAcsEnabled() throws Exception {
        Builder configBuilder = new SoftApConfiguration.Builder();
        configBuilder.setBand(SoftApConfiguration.BAND_5GHZ | SoftApConfiguration.BAND_2GHZ);
        when(mWifiNative.isHalStarted()).thenReturn(true);
        when(mWifiNative.getChannelsForBand(anyInt())).thenReturn(new int[0]);
        assertEquals(ApConfigUtil.SUCCESS,
                ApConfigUtil.updateApChannelConfig(mWifiNative, mCoexManager, mResources,
                        TEST_COUNTRY_CODE, configBuilder, configBuilder.build(), true));
        assertEquals(SoftApConfiguration.BAND_5GHZ | SoftApConfiguration.BAND_2GHZ,
                configBuilder.build().getBand());
        assertEquals(0, configBuilder.build().getChannel());
    }

    @Test
    public void testSoftApCapabilityInitWithResourceValue() throws Exception {
        long testFeatures = SoftApCapability.SOFTAP_FEATURE_CLIENT_FORCE_DISCONNECT
                | SoftApCapability.SOFTAP_FEATURE_BAND_6G_SUPPORTED
                | SoftApCapability.SOFTAP_FEATURE_BAND_60G_SUPPORTED;
        SoftApCapability capability = new SoftApCapability(testFeatures);
        int test_max_client = 10;
        capability.setMaxSupportedClients(test_max_client);

        when(mContext.getResources()).thenReturn(mResources);
        when(mResources.getInteger(R.integer.config_wifiHardwareSoftapMaxClientCount))
                .thenReturn(test_max_client);
        when(mResources.getBoolean(R.bool.config_wifi_softap_acs_supported))
                .thenReturn(false);
        when(mResources.getBoolean(R.bool.config_wifiSofapClientForceDisconnectSupported))
                .thenReturn(true);
        when(mResources.getBoolean(R.bool.config_wifi6ghzSupport)).thenReturn(true);
        when(mResources.getBoolean(R.bool.config_wifi60ghzSupport)).thenReturn(true);
        when(mResources.getBoolean(R.bool.config_wifiSoftap6ghzSupported)).thenReturn(true);
        when(mResources.getBoolean(R.bool.config_wifiSoftap60ghzSupported)).thenReturn(true);
        assertEquals(ApConfigUtil.updateCapabilityFromResource(mContext),
                capability);
    }

    @Test
    public void testConvertInvalidWifiConfigurationToSoftApConfiguration() throws Exception {
        WifiConfiguration wifiConfig = new WifiConfiguration();
        wifiConfig.SSID = "AndroidAP";
        wifiConfig.allowedKeyManagement.set(WifiConfiguration.KeyMgmt.WPA2_PSK);
        wifiConfig.preSharedKey = "1233443";
        assertNull(ApConfigUtil.fromWifiConfiguration(wifiConfig));
    }

    @Test
    public void testConvertInvalidKeyMgmtWifiConfigurationToSoftApConfiguration()
            throws Exception {
        WifiConfiguration wifiConfig = new WifiConfiguration();
        wifiConfig.SSID = "AndroidAP";
        wifiConfig.allowedKeyManagement.set(WifiConfiguration.KeyMgmt.WPA_EAP);
        wifiConfig.allowedKeyManagement.set(WifiConfiguration.KeyMgmt.WPA2_PSK);
        wifiConfig.preSharedKey = "12345678";
        assertNull(ApConfigUtil.fromWifiConfiguration(wifiConfig));
    }


    @Test
    public void testCheckConfigurationChangeNeedToRestart() throws Exception {
        MacAddress testBssid = MacAddress.fromString("aa:22:33:44:55:66");
        SoftApConfiguration currentConfig = new SoftApConfiguration.Builder()
                .setSsid("TestSSid")
                .setPassphrase("testpassphrase", SoftApConfiguration.SECURITY_TYPE_WPA2_PSK)
                .setBand(SoftApConfiguration.BAND_2GHZ)
                .setChannel(11, SoftApConfiguration.BAND_2GHZ)
                .setHiddenSsid(true)
                .build();

        // Test no changed
        // DO NOT use copy constructor to copy to test since it's instance is the same.
        SoftApConfiguration newConfig_noChange = new SoftApConfiguration.Builder()
                .setSsid("TestSSid")
                .setPassphrase("testpassphrase", SoftApConfiguration.SECURITY_TYPE_WPA2_PSK)
                .setBand(SoftApConfiguration.BAND_2GHZ)
                .setChannel(11, SoftApConfiguration.BAND_2GHZ)
                .setHiddenSsid(true)
                .build();
        assertFalse(ApConfigUtil.checkConfigurationChangeNeedToRestart(currentConfig,
                newConfig_noChange));

        // Test SSID changed
        SoftApConfiguration newConfig_ssidChanged = new SoftApConfiguration
                .Builder(newConfig_noChange)
                .setSsid("NewTestSSid").build();
        assertTrue(ApConfigUtil.checkConfigurationChangeNeedToRestart(currentConfig,
                newConfig_ssidChanged));
        // Test BSSID changed
        SoftApConfiguration newConfig_bssidChanged = new SoftApConfiguration
                .Builder(newConfig_noChange)
                .setBssid(testBssid).build();
        assertTrue(ApConfigUtil.checkConfigurationChangeNeedToRestart(currentConfig,
                newConfig_bssidChanged));
        // Test Passphrase Changed
        SoftApConfiguration newConfig_passphraseChanged = new SoftApConfiguration
                .Builder(newConfig_noChange)
                .setPassphrase("newtestpassphrase",
                SoftApConfiguration.SECURITY_TYPE_WPA2_PSK).build();
        assertTrue(ApConfigUtil.checkConfigurationChangeNeedToRestart(currentConfig,
                newConfig_passphraseChanged));
        // Test Security Type Changed
        SoftApConfiguration newConfig_securityeChanged = new SoftApConfiguration
                .Builder(newConfig_noChange)
                .setPassphrase("newtestpassphrase",
                SoftApConfiguration.SECURITY_TYPE_WPA3_SAE).build();
        assertTrue(ApConfigUtil.checkConfigurationChangeNeedToRestart(currentConfig,
                newConfig_securityeChanged));
        // Test Channel Changed
        SoftApConfiguration newConfig_channelChanged = new SoftApConfiguration
                .Builder(newConfig_noChange)
                .setChannel(6, SoftApConfiguration.BAND_2GHZ).build();
        assertTrue(ApConfigUtil.checkConfigurationChangeNeedToRestart(currentConfig,
                newConfig_channelChanged));
        // Test Band Changed
        SoftApConfiguration newConfig_bandChanged = new SoftApConfiguration
                .Builder(newConfig_noChange)
                .setBand(SoftApConfiguration.BAND_5GHZ).build();
        assertTrue(ApConfigUtil.checkConfigurationChangeNeedToRestart(currentConfig,
                newConfig_bandChanged));
        if (SdkLevel.isAtLeastS()) {
            // Test Bands Changed
            int[] bands = {SoftApConfiguration.BAND_2GHZ , SoftApConfiguration.BAND_5GHZ};
            SoftApConfiguration newConfig_bandsChanged = new SoftApConfiguration
                    .Builder(newConfig_noChange)
                    .setBands(bands).build();
            assertTrue(ApConfigUtil.checkConfigurationChangeNeedToRestart(currentConfig,
                    newConfig_bandsChanged));
            // Test Channels Changed
            SparseIntArray dual_channels = new SparseIntArray(2);
            dual_channels.put(SoftApConfiguration.BAND_5GHZ, 149);
            dual_channels.put(SoftApConfiguration.BAND_2GHZ, 0);
            SoftApConfiguration newConfig_channelsChanged = new SoftApConfiguration
                    .Builder(newConfig_noChange)
                    .setChannels(dual_channels).build();
            assertTrue(ApConfigUtil.checkConfigurationChangeNeedToRestart(currentConfig,
                    newConfig_channelsChanged));
        }
        // Test isHidden Changed
        SoftApConfiguration newConfig_hiddenChanged = new SoftApConfiguration
                .Builder(newConfig_noChange)
                .setHiddenSsid(false).build();
        assertTrue(ApConfigUtil.checkConfigurationChangeNeedToRestart(currentConfig,
                newConfig_hiddenChanged));
        // Test Others Changed
        SoftApConfiguration newConfig_nonRevalentChanged = new SoftApConfiguration
                .Builder(newConfig_noChange)
                .setMaxNumberOfClients(10)
                .setAutoShutdownEnabled(false)
                .setShutdownTimeoutMillis(500000)
                .setClientControlByUserEnabled(true)
                .setBlockedClientList(new ArrayList<>())
                .setAllowedClientList(new ArrayList<>())
                .build();
        assertFalse(ApConfigUtil.checkConfigurationChangeNeedToRestart(currentConfig,
                newConfig_nonRevalentChanged));

    }

    @Test
    public void testIsAvailableChannelsOnTargetBands() throws Exception {
        assumeTrue(SdkLevel.isAtLeastS());
        SoftApCapability testSoftApCapability = new SoftApCapability(0);
        testSoftApCapability.setSupportedChannelList(
                SoftApConfiguration.BAND_2GHZ, new int[] {1, 2});
        testSoftApCapability.setSupportedChannelList(
                SoftApConfiguration.BAND_5GHZ, new int[] {36, 149});

        int testBand_2_5 = SoftApConfiguration.BAND_2GHZ | SoftApConfiguration.BAND_5GHZ;
        int testBand_2_6 = SoftApConfiguration.BAND_2GHZ | SoftApConfiguration.BAND_6GHZ;
        int testBand_2_60 = SoftApConfiguration.BAND_2GHZ | SoftApConfiguration.BAND_60GHZ;

        assertEquals(testBand_2_5, ApConfigUtil.removeUnavailableBands(
                testSoftApCapability, testBand_2_5, mCoexManager));
        assertEquals(SoftApConfiguration.BAND_2GHZ, ApConfigUtil.removeUnavailableBands(
                testSoftApCapability, testBand_2_6, mCoexManager));
        assertEquals(SoftApConfiguration.BAND_2GHZ, ApConfigUtil.removeUnavailableBands(
                testSoftApCapability, testBand_2_60, mCoexManager));
        // Test with soft unsafe channels
        when(mCoexManager.getCoexRestrictions()).thenReturn(0);
        when(mCoexManager.getCoexUnsafeChannels()).thenReturn(Arrays.asList(
                new CoexUnsafeChannel(WifiScanner.WIFI_BAND_24_GHZ, 1),
                new CoexUnsafeChannel(WifiScanner.WIFI_BAND_5_GHZ, 36),
                new CoexUnsafeChannel(WifiScanner.WIFI_BAND_5_GHZ, 149)
        ));
        assertEquals(testBand_2_5, ApConfigUtil.removeUnavailableBands(
                testSoftApCapability, testBand_2_5, mCoexManager));

        // Test with hard unsafe channels
        when(mCoexManager.getCoexRestrictions()).thenReturn(WifiManager.COEX_RESTRICTION_SOFTAP);
        assertEquals(SoftApConfiguration.BAND_2GHZ, ApConfigUtil.removeUnavailableBands(
                testSoftApCapability, testBand_2_5, mCoexManager));


    }

    @Test
    public void testCheckSupportAllConfiguration() throws Exception {
        SoftApConfiguration.Builder testConfigBuilder = new SoftApConfiguration.Builder();
        SoftApCapability mockSoftApCapability = mock(SoftApCapability.class);
        assertTrue(ApConfigUtil.checkSupportAllConfiguration(testConfigBuilder.build(),
                  mockSoftApCapability));


        // Test client control feature
        when(mockSoftApCapability.areFeaturesSupported(
                SoftApCapability.SOFTAP_FEATURE_CLIENT_FORCE_DISCONNECT)).thenReturn(false);
        // Set max client number
        testConfigBuilder.setMaxNumberOfClients(1);
        assertFalse(ApConfigUtil.checkSupportAllConfiguration(testConfigBuilder.build(),
                mockSoftApCapability));
        // Reset Max client number
        testConfigBuilder.setMaxNumberOfClients(0);
        // Set client control
        testConfigBuilder.setClientControlByUserEnabled(true);
        assertFalse(ApConfigUtil.checkSupportAllConfiguration(testConfigBuilder.build(),
                mockSoftApCapability));
        // Reset client control
        testConfigBuilder.setClientControlByUserEnabled(false);
        //
        testConfigBuilder.setBlockedClientList(new ArrayList<>() {{
                add(MacAddress.fromString("aa:bb:cc:dd:ee:ff")); }});
        assertFalse(ApConfigUtil.checkSupportAllConfiguration(testConfigBuilder.build(),
                mockSoftApCapability));

        // Allow for client control
        when(mockSoftApCapability.areFeaturesSupported(
                SoftApCapability.SOFTAP_FEATURE_CLIENT_FORCE_DISCONNECT)).thenReturn(true);
        assertTrue(ApConfigUtil.checkSupportAllConfiguration(testConfigBuilder.build(),
                mockSoftApCapability));

        // Test WPA3-SAE
        when(mockSoftApCapability.areFeaturesSupported(
                SoftApCapability.SOFTAP_FEATURE_WPA3_SAE)).thenReturn(false);
        testConfigBuilder.setPassphrase("passphrase",
                SoftApConfiguration.SECURITY_TYPE_WPA3_SAE_TRANSITION);
        assertFalse(ApConfigUtil.checkSupportAllConfiguration(testConfigBuilder.build(),
                mockSoftApCapability));
        testConfigBuilder.setPassphrase("passphrase",
                SoftApConfiguration.SECURITY_TYPE_WPA3_SAE);
        assertFalse(ApConfigUtil.checkSupportAllConfiguration(testConfigBuilder.build(),
                mockSoftApCapability));

        // Allow for SAE
        when(mockSoftApCapability.areFeaturesSupported(
                SoftApCapability.SOFTAP_FEATURE_WPA3_SAE)).thenReturn(true);
        assertTrue(ApConfigUtil.checkSupportAllConfiguration(testConfigBuilder.build(),
                mockSoftApCapability));
        if (SdkLevel.isAtLeastS()) {
            // Test 6G or 60G not support
            testConfigBuilder.setChannels(
                    new SparseIntArray(){{
                        put(SoftApConfiguration.BAND_5GHZ, 149);
                        put(SoftApConfiguration.BAND_6GHZ, 2);
                    }});
            assertFalse(ApConfigUtil.checkSupportAllConfiguration(testConfigBuilder.build(),
                    mockSoftApCapability));

            testConfigBuilder.setChannels(
                    new SparseIntArray(){{
                        put(SoftApConfiguration.BAND_5GHZ, 149);
                        put(SoftApConfiguration.BAND_60GHZ, 1);
                    }});
            assertFalse(ApConfigUtil.checkSupportAllConfiguration(testConfigBuilder.build(),
                    mockSoftApCapability));
            // Test ACS not support in bridged mode
            when(mockSoftApCapability.areFeaturesSupported(
                    SoftApCapability.SOFTAP_FEATURE_ACS_OFFLOAD)).thenReturn(false);
            testConfigBuilder.setChannels(
                    new SparseIntArray(){{
                        put(SoftApConfiguration.BAND_5GHZ, 0);
                        put(SoftApConfiguration.BAND_2GHZ, 0);
                    }});
            assertFalse(ApConfigUtil.checkSupportAllConfiguration(testConfigBuilder.build(),
                    mockSoftApCapability));
        }
    }
}
