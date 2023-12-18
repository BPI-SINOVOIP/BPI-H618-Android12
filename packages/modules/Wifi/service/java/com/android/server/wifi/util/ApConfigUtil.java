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

import android.annotation.NonNull;
import android.annotation.Nullable;
import android.content.Context;
import android.content.res.Resources;
import android.net.wifi.CoexUnsafeChannel;
import android.net.wifi.ScanResult;
import android.net.wifi.SoftApCapability;
import android.net.wifi.SoftApConfiguration;
import android.net.wifi.SoftApConfiguration.BandType;
import android.net.wifi.SoftApInfo;
import android.net.wifi.WifiClient;
import android.net.wifi.WifiConfiguration;
import android.net.wifi.WifiManager;
import android.net.wifi.WifiScanner;
import android.text.TextUtils;
import android.util.Log;
import android.util.SparseArray;

import com.android.modules.utils.build.SdkLevel;
import com.android.server.wifi.WifiNative;
import com.android.server.wifi.coex.CoexManager;
import com.android.wifi.resources.R;

import java.util.ArrayList;
import java.util.HashMap;
import java.util.HashSet;
import java.util.List;
import java.util.Map;
import java.util.Objects;
import java.util.Random;
import java.util.Set;


/**
 * Provide utility functions for updating soft AP related configuration.
 */
public class ApConfigUtil {
    private static final String TAG = "ApConfigUtil";

    public static final int INVALID_VALUE_FOR_BAND_OR_CHANNEL = -1;
    public static final int DEFAULT_AP_BAND = SoftApConfiguration.BAND_2GHZ;
    public static final int DEFAULT_AP_CHANNEL = 6;
    public static final int HIGHEST_2G_AP_CHANNEL = 14;

    /* Return code for updateConfiguration. */
    public static final int SUCCESS = 0;
    public static final int ERROR_NO_CHANNEL = 1;
    public static final int ERROR_GENERIC = 2;
    public static final int ERROR_UNSUPPORTED_CONFIGURATION = 3;

    /* Random number generator used for AP channel selection. */
    private static final Random sRandom = new Random();

    /**
     * Valid Global Operating classes in each wifi band
     * Reference: Table E-4 in IEEE Std 802.11-2016.
     */
    private static final SparseArray<int[]> sBandToOperatingClass = new SparseArray<>();
    static {
        sBandToOperatingClass.append(SoftApConfiguration.BAND_2GHZ, new int[]{81, 82, 83, 84});
        sBandToOperatingClass.append(SoftApConfiguration.BAND_5GHZ, new int[]{115, 116, 117, 118,
                119, 120, 121, 122, 123, 124, 125, 126, 127, 128, 129, 130});
        sBandToOperatingClass.append(SoftApConfiguration.BAND_6GHZ, new int[]{131, 132, 133, 134,
                135, 136});
    }

    /**
     * Helper function to get the band corresponding to the operating class.
     *
     * @param operatingClass Global operating class.
     * @return band, -1 if no match.
     *
     */
    public static int getBandFromOperatingClass(int operatingClass) {
        for (int i = 0; i < sBandToOperatingClass.size(); i++) {
            int band = sBandToOperatingClass.keyAt(i);
            int[] operatingClasses = sBandToOperatingClass.get(band);

            for (int j = 0; j < operatingClasses.length; j++) {
                if (operatingClasses[j] == operatingClass) {
                    return band;
                }
            }
        }
        return -1;
    }

    /**
     * Convert band from SoftApConfiguration.BandType to WifiScanner.WifiBand
     * @param band in SoftApConfiguration.BandType
     * @return band in WifiScanner.WifiBand
     */
    public static @WifiScanner.WifiBand int apConfig2wifiScannerBand(@BandType int band) {
        switch(band) {
            case SoftApConfiguration.BAND_2GHZ:
                return WifiScanner.WIFI_BAND_24_GHZ;
            case SoftApConfiguration.BAND_5GHZ:
                return WifiScanner.WIFI_BAND_5_GHZ;
            case SoftApConfiguration.BAND_6GHZ:
                return WifiScanner.WIFI_BAND_6_GHZ;
            case SoftApConfiguration.BAND_60GHZ:
                return WifiScanner.WIFI_BAND_60_GHZ;
            default:
                return WifiScanner.WIFI_BAND_UNSPECIFIED;
        }
    }

    /**
     * Convert channel/band to frequency.
     * Note: the utility does not perform any regulatory domain compliance.
     * @param channel number to convert
     * @param band of channel to convert
     * @return center frequency in Mhz of the channel, -1 if no match
     */
    public static int convertChannelToFrequency(int channel, @BandType int band) {
        return ScanResult.convertChannelToFrequencyMhzIfSupported(channel,
                apConfig2wifiScannerBand(band));
    }

    /**
     * Convert frequency to band.
     * Note: the utility does not perform any regulatory domain compliance.
     * @param frequency frequency to convert
     * @return band, -1 if no match
     */
    public static int convertFrequencyToBand(int frequency) {
        if (ScanResult.is24GHz(frequency)) {
            return SoftApConfiguration.BAND_2GHZ;
        } else if (ScanResult.is5GHz(frequency)) {
            return SoftApConfiguration.BAND_5GHZ;
        } else if (ScanResult.is6GHz(frequency)) {
            return SoftApConfiguration.BAND_6GHZ;
        } else if (ScanResult.is60GHz(frequency)) {
            return SoftApConfiguration.BAND_60GHZ;
        }

        return -1;
    }

    /**
     * Convert band from WifiConfiguration into SoftApConfiguration
     *
     * @param wifiConfigBand band encoded as WifiConfiguration.AP_BAND_xxxx
     * @return band as encoded as SoftApConfiguration.BAND_xxx
     */
    public static int convertWifiConfigBandToSoftApConfigBand(int wifiConfigBand) {
        switch (wifiConfigBand) {
            case WifiConfiguration.AP_BAND_2GHZ:
                return SoftApConfiguration.BAND_2GHZ;
            case WifiConfiguration.AP_BAND_5GHZ:
                return SoftApConfiguration.BAND_5GHZ;
            case WifiConfiguration.AP_BAND_ANY:
                return SoftApConfiguration.BAND_2GHZ | SoftApConfiguration.BAND_5GHZ;
            default:
                return SoftApConfiguration.BAND_2GHZ;
        }
    }

    /**
     * Add 2.4Ghz to target band when 2.4Ghz SoftAp supported.
     *
     * @param targetBand The band is needed to add 2.4G.
     * @return The band includes 2.4Ghz when 2.4G SoftAp supported.
     */
    public static @BandType int append24GToBandIf24GSupported(@BandType int targetBand,
            Context context) {
        if (isBandSupported(SoftApConfiguration.BAND_2GHZ, context)) {
            return targetBand | SoftApConfiguration.BAND_2GHZ;
        }
        return targetBand;
    }

    /**
     * Checks if band is a valid combination of {link  SoftApConfiguration#BandType} values
     */
    public static boolean isBandValid(@BandType int band) {
        int bandAny = SoftApConfiguration.BAND_2GHZ | SoftApConfiguration.BAND_5GHZ
                | SoftApConfiguration.BAND_6GHZ | SoftApConfiguration.BAND_60GHZ;
        return ((band != 0) && ((band & ~bandAny) == 0));
    }

    /**
     * Check if the band contains a certain sub-band
     *
     * @param band The combination of bands to validate
     * @param testBand the test band to validate on
     * @return true if band contains testBand, false otherwise
     */
    public static boolean containsBand(@BandType int band, @BandType int testBand) {
        return ((band & testBand) != 0);
    }

    /**
     * Checks if band contains multiple sub-bands
     * @param band a combination of sub-bands
     * @return true if band has multiple sub-bands, false otherwise
     */
    public static boolean isMultiband(@BandType int band) {
        return ((band & (band - 1)) != 0);
    }


    /**
     * Checks whether or not band configuration is supported.
     * @param apBand a combination of the bands
     * @param context the caller context used to get value from resource file.
     * @return true if band is supported, false otherwise
     */
    public static boolean isBandSupported(@BandType int apBand, Context context) {
        if (!isBandValid(apBand)) {
            Log.e(TAG, "Invalid SoftAp band. ");
            return false;
        }

        if (containsBand(apBand, SoftApConfiguration.BAND_2GHZ)
                && !isSoftAp24GhzSupported(context)) {
            Log.e(TAG, "Can not start softAp with 2GHz band, not supported.");
            return false;
        }

        if (containsBand(apBand, SoftApConfiguration.BAND_5GHZ)
                && !isSoftAp5GhzSupported(context)) {
            Log.e(TAG, "Can not start softAp with 5GHz band, not supported.");
            return false;
        }

        if (containsBand(apBand, SoftApConfiguration.BAND_6GHZ)
                && !isSoftAp6GhzSupported(context)) {
            Log.e(TAG, "Can not start softAp with 6GHz band, not supported.");
            return false;
        }

        if (containsBand(apBand, SoftApConfiguration.BAND_60GHZ)
                && !isSoftAp60GhzSupported(context)) {
            Log.e(TAG, "Can not start softAp with 6GHz band, not supported.");
            return false;
        }

        return true;
    }

    /**
     * Convert string to channel list
     * Format of the list is a comma separated channel numbers, or range of channel numbers
     * Example, "34-48, 149".
     * @param channelString for a comma separated channel numbers, or range of channel numbers
     *        such as "34-48, 149"
     * @return list of channel numbers
     */
    public static List<Integer> convertStringToChannelList(String channelString) {
        if (channelString == null) {
            return null;
        }

        List<Integer> channelList = new ArrayList<Integer>();

        for (String channelRange : channelString.split(",")) {
            try {
                if (channelRange.contains("-")) {
                    String[] channels = channelRange.split("-");
                    if (channels.length != 2) {
                        Log.e(TAG, "Unrecognized channel range, Length is " + channels.length);
                        continue;
                    }
                    int start = Integer.parseInt(channels[0].trim());
                    int end = Integer.parseInt(channels[1].trim());
                    if (start > end) {
                        Log.e(TAG, "Invalid channel range, from " + start + " to " + end);
                        continue;
                    }

                    for (int channel = start; channel <= end; channel++) {
                        channelList.add(channel);
                    }
                } else {
                    channelList.add(Integer.parseInt(channelRange.trim()));
                }
            } catch (NumberFormatException e) {
                // Ignore malformed string
                Log.e(TAG, "Malformed channel value detected: " + e);
                continue;
            }
        }
        return channelList;
    }

    /**
     * Returns the unsafe channels frequency from coex module.
     *
     * @param coexManager reference used to get unsafe channels to avoid for coex.
     */
    @NonNull
    public static Set<Integer> getUnsafeChannelFreqsFromCoex(@NonNull CoexManager coexManager) {
        Set<Integer> unsafeFreqs = new HashSet<>();
        if (SdkLevel.isAtLeastS()) {
            for (CoexUnsafeChannel unsafeChannel : coexManager.getCoexUnsafeChannels()) {
                unsafeFreqs.add(ScanResult.convertChannelToFrequencyMhzIfSupported(
                        unsafeChannel.getChannel(), unsafeChannel.getBand()));
            }
        }
        return unsafeFreqs;
    }

    /**
     * Get channels or frequencies for band that are allowed by both regulatory
     * and OEM configuration.
     *
     * @param band to get channels for
     * @param wifiNative reference used to get regulatory restrictionsimport java.util.Arrays;
     * @param resources used to get OEM restrictions
     * @param inFrequencyMHz true to convert channel to frequency.
     * @return A list of frequencies that are allowed, null on error.
     */
    public static List<Integer> getAvailableChannelFreqsForBand(
            @BandType int band, WifiNative wifiNative, Resources resources,
            boolean inFrequencyMHz) {
        if (!isBandValid(band) || isMultiband(band)) {
            return null;
        }

        List<Integer> configuredList;
        int scannerBand;
        switch (band) {
            case SoftApConfiguration.BAND_2GHZ:
                configuredList = convertStringToChannelList(resources.getString(
                        R.string.config_wifiSoftap2gChannelList));
                scannerBand = WifiScanner.WIFI_BAND_24_GHZ;
                break;
            case SoftApConfiguration.BAND_5GHZ:
                configuredList = convertStringToChannelList(resources.getString(
                        R.string.config_wifiSoftap5gChannelList));
                scannerBand = WifiScanner.WIFI_BAND_5_GHZ;
                break;
            case SoftApConfiguration.BAND_6GHZ:
                configuredList = convertStringToChannelList(resources.getString(
                        R.string.config_wifiSoftap6gChannelList));
                scannerBand = WifiScanner.WIFI_BAND_6_GHZ;
                break;
            case SoftApConfiguration.BAND_60GHZ:
                configuredList = convertStringToChannelList(resources.getString(
                        R.string.config_wifiSoftap60gChannelList));
                scannerBand = WifiScanner.WIFI_BAND_60_GHZ;
                break;
            default:
                return null;
        }

        // Get the allowed list of channel frequencies in MHz
        int[] regulatoryArray = wifiNative.getChannelsForBand(scannerBand);
        List<Integer> regulatoryList = new ArrayList<Integer>();
        for (int freq : regulatoryArray) {
            if (inFrequencyMHz) {
                regulatoryList.add(freq);
            } else {
                regulatoryList.add(ScanResult.convertFrequencyMhzToChannelIfSupported(freq));
            }
        }

        if (configuredList == null || configuredList.isEmpty()) {
            return regulatoryList;
        }
        List<Integer> filteredList = new ArrayList<Integer>();
        // Otherwise, filter the configured list
        for (int channel : configuredList) {
            if (inFrequencyMHz) {
                int channelFreq = convertChannelToFrequency(channel, band);
                if (regulatoryList.contains(channelFreq)) {
                    filteredList.add(channelFreq);
                }
            } else if (regulatoryList.contains(channel)) {
                filteredList.add(channel);
            }
        }
        return filteredList;
    }

    /**
     * Return a channel frequency for AP setup based on the frequency band.
     * @param apBand one or combination of the values of SoftApConfiguration.BAND_*.
     * @param wifiNative reference used to collect regulatory restrictions.
     * @param coexManager reference used to get unsafe channels to avoid for coex.
     * @param resources the resources to use to get configured allowed channels.
     * @return a valid channel frequency on success, -1 on failure.
     */
    public static int chooseApChannel(int apBand, @NonNull WifiNative wifiNative,
            @NonNull CoexManager coexManager, @NonNull Resources resources) {
        if (!isBandValid(apBand)) {
            Log.e(TAG, "Invalid band: " + apBand);
            return -1;
        }

        Set<Integer> unsafeFreqs = new HashSet<>();
        if (SdkLevel.isAtLeastS()) {
            unsafeFreqs = getUnsafeChannelFreqsFromCoex(coexManager);
        }
        final int[] bandPreferences = new int[]{
                SoftApConfiguration.BAND_60GHZ,
                SoftApConfiguration.BAND_6GHZ,
                SoftApConfiguration.BAND_5GHZ,
                SoftApConfiguration.BAND_2GHZ};
        int selectedUnsafeFreq = 0;
        for (int band : bandPreferences) {
            if ((apBand & band) == 0) {
                continue;
            }
            final List<Integer> availableFreqs =
                    getAvailableChannelFreqsForBand(band, wifiNative, resources, true);
            if (availableFreqs == null || availableFreqs.isEmpty()) {
                continue;
            }
            // Separate the available freqs by safe and unsafe.
            List<Integer> availableSafeFreqs = new ArrayList<>();
            List<Integer> availableUnsafeFreqs = new ArrayList<>();
            for (int freq : availableFreqs) {
                if (unsafeFreqs.contains(freq)) {
                    availableUnsafeFreqs.add(freq);
                } else {
                    availableSafeFreqs.add(freq);
                }
            }
            // If there are safe freqs available for this band, randomly select one.
            if (!availableSafeFreqs.isEmpty()) {
                return availableSafeFreqs.get(sRandom.nextInt(availableSafeFreqs.size()));
            } else if (!availableUnsafeFreqs.isEmpty() && selectedUnsafeFreq == 0) {
                // Save an unsafe freq from the first preferred band to fall back on later.
                selectedUnsafeFreq = availableUnsafeFreqs.get(
                        sRandom.nextInt(availableUnsafeFreqs.size()));
            }
        }
        // If all available channels are soft unsafe, select a random one of the highest band.
        boolean isHardUnsafe = false;
        if (SdkLevel.isAtLeastS()) {
            isHardUnsafe =
                    (coexManager.getCoexRestrictions() & WifiManager.COEX_RESTRICTION_SOFTAP) != 0;
        }
        if (!isHardUnsafe && selectedUnsafeFreq != 0) {
            return selectedUnsafeFreq;
        }

        // If all available channels are hard unsafe, select the default AP channel.
        if (containsBand(apBand, DEFAULT_AP_BAND)) {
            final int defaultChannelFreq = convertChannelToFrequency(DEFAULT_AP_CHANNEL,
                    DEFAULT_AP_BAND);
            Log.e(TAG, "Allowed channel list not specified, selecting default channel");
            if (isHardUnsafe && unsafeFreqs.contains(defaultChannelFreq)) {
                Log.e(TAG, "Default channel is hard restricted due to coex");
            }
            return defaultChannelFreq;
        }
        Log.e(TAG, "No available channels");
        return -1;
    }

    /**
     * Remove unavailable bands from the input band and return the resulting
     * (remaining) available bands. Unavailable bands are those which don't have channels available.
     *
     * @param capability SoftApCapability which inidcates supported channel list.
     * @param targetBand The target band which plan to enable
     * @param coexManager reference to CoexManager
     *
     * @return the available band which removed the unsupported band.
     *         0 when all of the band is not supported.
     */
    public static @BandType int removeUnavailableBands(SoftApCapability capability,
            @NonNull int targetBand, CoexManager coexManager) {
        int availableBand = targetBand;
        for (int band : SoftApConfiguration.BAND_TYPES) {
            Set<Integer> availableChannelFreqsList = new HashSet<>();
            if ((targetBand & band) != 0) {
                for (int channel : capability.getSupportedChannelList(band)) {
                    availableChannelFreqsList.add(convertChannelToFrequency(channel, band));
                }
                // Only remove hard unsafe channels
                if (SdkLevel.isAtLeastS()
                        && (coexManager.getCoexRestrictions() & WifiManager.COEX_RESTRICTION_SOFTAP)
                        != 0) {
                    availableChannelFreqsList.removeAll(getUnsafeChannelFreqsFromCoex(coexManager));
                }
                if (availableChannelFreqsList.size() == 0) {
                    availableBand &= ~band;
                }
            }
        }
        return availableBand;
    }

    /**
     * Remove all unsupported bands from the input band and return the resulting
     * (remaining) support bands. Unsupported bands are those which don't have channels available.
     *
     * @param context The caller context used to get value from resource file.
     * @param band The target band which plan to enable
     *
     * @return the available band which removed the unsupported band.
     *         0 when all of the band is not supported.
     */
    public static @BandType int removeUnsupportedBands(Context context,
            @NonNull int band) {
        int availableBand = band;
        if (((band & SoftApConfiguration.BAND_2GHZ) != 0) && !isSoftAp24GhzSupported(context)) {
            availableBand &= ~SoftApConfiguration.BAND_2GHZ;
        }
        if (((band & SoftApConfiguration.BAND_5GHZ) != 0) && !isSoftAp5GhzSupported(context)) {
            availableBand &= ~SoftApConfiguration.BAND_5GHZ;
        }
        if (((band & SoftApConfiguration.BAND_6GHZ) != 0) && !isSoftAp6GhzSupported(context)) {
            availableBand &= ~SoftApConfiguration.BAND_6GHZ;
        }
        if (((band & SoftApConfiguration.BAND_60GHZ) != 0) && !isSoftAp60GhzSupported(context)) {
            availableBand &= ~SoftApConfiguration.BAND_60GHZ;
        }
        return availableBand;
    }

    /**
     * Update AP band and channel based on the provided country code and band.
     * This will also set
     * @param wifiNative reference to WifiNative
     * @param coexManager reference to CoexManager
     * @param resources the resources to use to get configured allowed channels.
     * @param countryCode country code
     * @param config configuration to update
     * @return an integer result code
     */
    public static int updateApChannelConfig(WifiNative wifiNative,
            @NonNull CoexManager coexManager,
            Resources resources,
            String countryCode,
            SoftApConfiguration.Builder configBuilder,
            SoftApConfiguration config,
            boolean acsEnabled) {
        /* Use default band and channel for device without HAL. */
        if (!wifiNative.isHalStarted()) {
            configBuilder.setChannel(DEFAULT_AP_CHANNEL, DEFAULT_AP_BAND);
            return SUCCESS;
        }

        /* Country code is mandatory for 5GHz band. */
        if (config.getBand() == SoftApConfiguration.BAND_5GHZ
                && countryCode == null) {
            Log.e(TAG, "5GHz band is not allowed without country code");
            return ERROR_GENERIC;
        }

        /* Select a channel if it is not specified and ACS is not enabled */
        if ((config.getChannel() == 0) && !acsEnabled) {
            int freq = chooseApChannel(config.getBand(), wifiNative, coexManager, resources);
            if (freq == -1) {
                /* We're not able to get channel from wificond. */
                Log.e(TAG, "Failed to get available channel.");
                return ERROR_NO_CHANNEL;
            }
            configBuilder.setChannel(
                    ScanResult.convertFrequencyMhzToChannelIfSupported(freq),
                    convertFrequencyToBand(freq));
        }

        return SUCCESS;
    }

    /**
     * Helper function for converting WifiConfiguration to SoftApConfiguration.
     *
     * Only Support None and WPA2 configuration conversion.
     * Note that WifiConfiguration only Supports 2GHz, 5GHz, 2GHz+5GHz bands,
     * so conversion is limited to these bands.
     *
     * @param wifiConfig the WifiConfiguration which need to convert.
     * @return the SoftApConfiguration if wifiConfig is valid, null otherwise.
     */
    @Nullable
    public static SoftApConfiguration fromWifiConfiguration(
            @NonNull WifiConfiguration wifiConfig) {
        SoftApConfiguration.Builder configBuilder = new SoftApConfiguration.Builder();
        try {
            configBuilder.setSsid(wifiConfig.SSID);
            if (wifiConfig.getAuthType() == WifiConfiguration.KeyMgmt.WPA2_PSK) {
                configBuilder.setPassphrase(wifiConfig.preSharedKey,
                        SoftApConfiguration.SECURITY_TYPE_WPA2_PSK);
            }
            configBuilder.setHiddenSsid(wifiConfig.hiddenSSID);

            int band;
            switch (wifiConfig.apBand) {
                case WifiConfiguration.AP_BAND_2GHZ:
                    band = SoftApConfiguration.BAND_2GHZ;
                    break;
                case WifiConfiguration.AP_BAND_5GHZ:
                    band = SoftApConfiguration.BAND_5GHZ;
                    break;
                case WifiConfiguration.AP_BAND_60GHZ:
                    band = SoftApConfiguration.BAND_60GHZ;
                    break;
                default:
                    // WifiConfiguration.AP_BAND_ANY means only 2GHz and 5GHz bands
                    band = SoftApConfiguration.BAND_2GHZ | SoftApConfiguration.BAND_5GHZ;
                    break;
            }
            if (wifiConfig.apChannel == 0) {
                configBuilder.setBand(band);
            } else {
                configBuilder.setChannel(wifiConfig.apChannel, band);
            }
        } catch (IllegalArgumentException iae) {
            Log.e(TAG, "Invalid WifiConfiguration" + iae);
            return null;
        } catch (IllegalStateException ise) {
            Log.e(TAG, "Invalid WifiConfiguration" + ise);
            return null;
        }
        return configBuilder.build();
    }

    /**
     * Helper function to creating SoftApCapability instance with initial field from resource file.
     *
     * @param context the caller context used to get value from resource file.
     * @return SoftApCapability which updated the feature support or not from resource.
     */
    @NonNull
    public static SoftApCapability updateCapabilityFromResource(@NonNull Context context) {
        long features = 0;
        if (isAcsSupported(context)) {
            Log.d(TAG, "Update Softap capability, add acs feature support");
            features |= SoftApCapability.SOFTAP_FEATURE_ACS_OFFLOAD;
        }

        if (isClientForceDisconnectSupported(context)) {
            Log.d(TAG, "Update Softap capability, add client control feature support");
            features |= SoftApCapability.SOFTAP_FEATURE_CLIENT_FORCE_DISCONNECT;
        }

        if (isWpa3SaeSupported(context)) {
            Log.d(TAG, "Update Softap capability, add SAE feature support");
            features |= SoftApCapability.SOFTAP_FEATURE_WPA3_SAE;
        }

        if (isMacCustomizationSupported(context)) {
            Log.d(TAG, "Update Softap capability, add MAC customization support");
            features |= SoftApCapability.SOFTAP_FEATURE_MAC_ADDRESS_CUSTOMIZATION;
        }

        if (isSoftAp24GhzSupported(context)) {
            Log.d(TAG, "Update Softap capability, add 2.4G support");
            features |= SoftApCapability.SOFTAP_FEATURE_BAND_24G_SUPPORTED;
        }

        if (isSoftAp5GhzSupported(context)) {
            Log.d(TAG, "Update Softap capability, add 5G support");
            features |= SoftApCapability.SOFTAP_FEATURE_BAND_5G_SUPPORTED;
        }

        if (isSoftAp6GhzSupported(context)) {
            Log.d(TAG, "Update Softap capability, add 6G support");
            features |= SoftApCapability.SOFTAP_FEATURE_BAND_6G_SUPPORTED;
        }

        if (isSoftAp60GhzSupported(context)) {
            Log.d(TAG, "Update Softap capability, add 60G support");
            features |= SoftApCapability.SOFTAP_FEATURE_BAND_60G_SUPPORTED;
        }

        SoftApCapability capability = new SoftApCapability(features);
        int hardwareSupportedMaxClient = context.getResources().getInteger(
                R.integer.config_wifiHardwareSoftapMaxClientCount);
        if (hardwareSupportedMaxClient > 0) {
            Log.d(TAG, "Update Softap capability, max client = " + hardwareSupportedMaxClient);
            capability.setMaxSupportedClients(hardwareSupportedMaxClient);
        }

        return capability;
    }

    /**
     * Helper function to get device support AP MAC randomization or not.
     *
     * @param context the caller context used to get value from resource file.
     * @return true if supported, false otherwise.
     */
    public static boolean isApMacRandomizationSupported(@NonNull Context context) {
        return context.getResources().getBoolean(
                    R.bool.config_wifi_ap_mac_randomization_supported);
    }

    /**
     * Helper function to get HAL support bridged AP or not.
     *
     * @param context the caller context used to get value from resource file.
     * @return true if supported, false otherwise.
     */
    public static boolean isBridgedModeSupported(@NonNull Context context) {
        return SdkLevel.isAtLeastS() && context.getResources().getBoolean(
                    R.bool.config_wifiBridgedSoftApSupported);
    }

    /**
     * Helper function to get HAL support client force disconnect or not.
     *
     * @param context the caller context used to get value from resource file.
     * @return true if supported, false otherwise.
     */
    public static boolean isClientForceDisconnectSupported(@NonNull Context context) {
        return context.getResources().getBoolean(
                R.bool.config_wifiSofapClientForceDisconnectSupported);
    }

    /**
     * Helper function to get SAE support or not.
     *
     * @param context the caller context used to get value from resource file.
     * @return true if supported, false otherwise.
     */
    public static boolean isWpa3SaeSupported(@NonNull Context context) {
        return context.getResources().getBoolean(
                R.bool.config_wifi_softap_sae_supported);
    }

    /**
     * Helper function to get ACS support or not.
     *
     * @param context the caller context used to get value from resource file.
     * @return true if supported, false otherwise.
     */
    public static boolean isAcsSupported(@NonNull Context context) {
        return context.getResources().getBoolean(
                R.bool.config_wifi_softap_acs_supported);
    }

    /**
     * Helper function to get MAC Address customization or not.
     *
     * @param context the caller context used to get value from resource file.
     * @return true if supported, false otherwise.
     */
    public static boolean isMacCustomizationSupported(@NonNull Context context) {
        return context.getResources().getBoolean(
                R.bool.config_wifiSoftapMacAddressCustomizationSupported);
    }

    /**
     * Helper function to get whether or not 2.4G Soft AP support.
     *
     * @param context the caller context used to get value from resource file.
     * @return true if supported, false otherwise.
     */
    public static boolean isSoftAp24GhzSupported(@NonNull Context context) {
        return context.getResources().getBoolean(R.bool.config_wifi24ghzSupport)
                && context.getResources().getBoolean(
                R.bool.config_wifiSoftap24ghzSupported);
    }

    /**
     * Helper function to get whether or not 5G Soft AP support.
     *
     * @param context the caller context used to get value from resource file.
     * @return true if supported, false otherwise.
     */
    public static boolean isSoftAp5GhzSupported(@NonNull Context context) {
        return context.getResources().getBoolean(R.bool.config_wifi5ghzSupport)
                && context.getResources().getBoolean(
                R.bool.config_wifiSoftap5ghzSupported);
    }

    /**
     * Helper function to get whether or not 6G Soft AP support
     *
     * @param context the caller context used to get value from resource file.
     * @return true if supported, false otherwise.
     */
    public static boolean isSoftAp6GhzSupported(@NonNull Context context) {
        return context.getResources().getBoolean(R.bool.config_wifi6ghzSupport)
                && context.getResources().getBoolean(
                R.bool.config_wifiSoftap6ghzSupported);
    }

    /**
     * Helper function to get whether or not 60G Soft AP support.
     *
     * @param context the caller context used to get value from resource file.
     * @return true if supported, false otherwise.
     */
    public static boolean isSoftAp60GhzSupported(@NonNull Context context) {
        return context.getResources().getBoolean(R.bool.config_wifi60ghzSupport)
                && context.getResources().getBoolean(
                R.bool.config_wifiSoftap60ghzSupported);
    }

    /**
     * Helper function to get whether or not dynamic country code update is supported when Soft AP
     * enabled.
     *
     * @param context the caller context used to get value from resource file.
     * @return true if supported, false otherwise.
     */
    public static boolean isSoftApDynamicCountryCodeSupported(@NonNull Context context) {
        return context.getResources().getBoolean(
                R.bool.config_wifiSoftApDynamicCountryCodeUpdateSupported);
    }

    /**
     * Helper function for comparing two SoftApConfiguration.
     *
     * @param currentConfig the original configuration.
     * @param newConfig the new configuration which plan to apply.
     * @return true if the difference between the two configurations requires a restart to apply,
     *         false otherwise.
     */
    public static boolean checkConfigurationChangeNeedToRestart(
            SoftApConfiguration currentConfig, SoftApConfiguration newConfig) {
        return !Objects.equals(currentConfig.getSsid(), newConfig.getSsid())
                || !Objects.equals(currentConfig.getBssid(), newConfig.getBssid())
                || currentConfig.getSecurityType() != newConfig.getSecurityType()
                || !Objects.equals(currentConfig.getPassphrase(), newConfig.getPassphrase())
                || currentConfig.isHiddenSsid() != newConfig.isHiddenSsid()
                || currentConfig.getBand() != newConfig.getBand()
                || currentConfig.getChannel() != newConfig.getChannel()
                || (SdkLevel.isAtLeastS() && !currentConfig.getChannels().toString()
                        .equals(newConfig.getChannels().toString()));
    }


    /**
     * Helper function for checking all of the configuration are supported or not.
     *
     * @param config target configuration want to check.
     * @param capability the capability which indicate feature support or not.
     * @return true if supported, false otherwise.
     */
    public static boolean checkSupportAllConfiguration(SoftApConfiguration config,
            SoftApCapability capability) {
        if (!capability.areFeaturesSupported(
                SoftApCapability.SOFTAP_FEATURE_CLIENT_FORCE_DISCONNECT)
                && (config.getMaxNumberOfClients() != 0 || config.isClientControlByUserEnabled()
                || config.getBlockedClientList().size() != 0)) {
            Log.d(TAG, "Error, Client control requires HAL support");
            return false;
        }

        if (!capability.areFeaturesSupported(SoftApCapability.SOFTAP_FEATURE_WPA3_SAE)
                && (config.getSecurityType()
                == SoftApConfiguration.SECURITY_TYPE_WPA3_SAE_TRANSITION
                || config.getSecurityType() == SoftApConfiguration.SECURITY_TYPE_WPA3_SAE)) {
            Log.d(TAG, "Error, SAE requires HAL support");
            return false;
        }

        // The bands length should always 1 in R. Adding SdkLevel.isAtLeastS for lint check only.
        if (config.getBands().length > 1 && SdkLevel.isAtLeastS()) {
            int[] bands = config.getBands();
            if ((bands[0] & SoftApConfiguration.BAND_6GHZ) != 0
                    || (bands[0] & SoftApConfiguration.BAND_60GHZ) != 0
                    || (bands[1] & SoftApConfiguration.BAND_6GHZ) != 0
                    || (bands[1] & SoftApConfiguration.BAND_60GHZ) != 0) {
                Log.d(TAG, "Error, dual APs doesn't support on 6GHz and 60GHz");
                return false;
            }
            if (!capability.areFeaturesSupported(SoftApCapability.SOFTAP_FEATURE_ACS_OFFLOAD)
                    && (config.getChannels().valueAt(0) == 0
                    || config.getChannels().valueAt(1) == 0)) {
                Log.d(TAG, "Error, dual APs requires HAL ACS support when channel isn't specified");
                return false;
            }
        }
        return true;
    }


    /**
     * Check if need to provide freq range for ACS.
     *
     * @param band in SoftApConfiguration.BandType
     * @return true when freq ranges is needed, otherwise false.
     */
    public static boolean isSendFreqRangesNeeded(@BandType int band, Context context) {
        // Fist we check if one of the selected bands has restrictions in the overlay file.
        // Note,
        //   - We store the config string here for future use, hence we need to check all bands.
        //   - If there is no OEM restriction, we store the full band
        boolean retVal = false;
        String channelList = "";
        if ((band & SoftApConfiguration.BAND_2GHZ) != 0) {
            channelList =
                context.getResources().getString(R.string.config_wifiSoftap2gChannelList);
            if (!TextUtils.isEmpty(channelList)) {
                retVal = true;
            }
        }

        if ((band & SoftApConfiguration.BAND_5GHZ) != 0) {
            channelList =
                context.getResources().getString(R.string.config_wifiSoftap5gChannelList);
            if (!TextUtils.isEmpty(channelList)) {
                retVal = true;
            }
        }

        if ((band & SoftApConfiguration.BAND_6GHZ) != 0) {
            channelList =
                context.getResources().getString(R.string.config_wifiSoftap6gChannelList);
            if (!TextUtils.isEmpty(channelList)) {
                retVal = true;
            }
        }

        // If any of the selected band has restriction in the overlay file, we return true.
        if (retVal) {
            return true;
        }

        // Next, if only one of 5G or 6G is selected, then we need freqList to separate them
        // Since there is no other way.
        if (((band & SoftApConfiguration.BAND_5GHZ) != 0)
                && ((band & SoftApConfiguration.BAND_6GHZ) == 0)) {
            return true;
        }
        if (((band & SoftApConfiguration.BAND_5GHZ) == 0)
                && ((band & SoftApConfiguration.BAND_6GHZ) != 0)) {
            return true;
        }

        // In all other cases, we don't need to set the freqList
        return false;
    }

    /**
     * Deep copy for object Map<String, SoftApInfo>
     */
    public static Map<String, SoftApInfo> deepCopyForSoftApInfoMap(
            Map<String, SoftApInfo> originalMap) {
        if (originalMap == null) {
            return null;
        }
        Map<String, SoftApInfo> deepCopyMap = new HashMap<String, SoftApInfo>();
        for (Map.Entry<String, SoftApInfo> entry: originalMap.entrySet()) {
            deepCopyMap.put(entry.getKey(), new SoftApInfo(entry.getValue()));
        }
        return deepCopyMap;
    }

    /**
     * Deep copy for object Map<String, List<WifiClient>>
     */
    public static Map<String, List<WifiClient>> deepCopyForWifiClientListMap(
            Map<String, List<WifiClient>> originalMap) {
        if (originalMap == null) {
            return null;
        }
        Map<String, List<WifiClient>> deepCopyMap = new HashMap<String, List<WifiClient>>();
        for (Map.Entry<String, List<WifiClient>> entry: originalMap.entrySet()) {
            List<WifiClient> clients = new ArrayList<>();
            for (WifiClient client : entry.getValue()) {
                clients.add(new WifiClient(client.getMacAddress(),
                        client.getApInstanceIdentifier()));
            }
            deepCopyMap.put(entry.getKey(), clients);
        }
        return deepCopyMap;
    }
}
