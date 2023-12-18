/*
 * Copyright (C) 2020 The Android Open Source Project
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

package com.android.server.wifi;

import android.annotation.NonNull;
import android.util.Log;

import com.android.server.wifi.util.WifiConfigStoreEncryptionUtil;
import com.android.server.wifi.util.XmlUtil;

import org.xmlpull.v1.XmlPullParser;
import org.xmlpull.v1.XmlPullParserException;
import org.xmlpull.v1.XmlSerializer;

import java.io.IOException;
import java.util.HashMap;
import java.util.Map;

/**
 * This class performs serialization and parsing of XML data block that contain the map of carrier
 * Wi-Fi offloading settings info.
 */
public class WifiCarrierInfoStoreManagerData implements WifiConfigStore.StoreData {
    private static final String TAG = "WifiCarrierInfoStoreManagerData";
    private static final String XML_TAG_SECTION_HEADER =
            "WifiCarrierInfoStoreManagerDataStores";
    private static final String XML_TAG_MERGED_CARRIER_NETWORK_OFFLOAD_MAP =
            "MergedCarrierNetworkOffloadMap";
    private static final String XML_TAG_UNMERGED_CARRIER_NETWORK_OFFLOAD_MAP =
            "UnmergedCarrierNetworkOffloadMap";

    /**
     * Interface define the data source for the carrier IMSI protection exemption map store data.
     */
    public interface DataSource {

        /**
         * Retrieve the merged carrier network offload map from the data source to serialize to
         * disk.
         */
        Map<Integer, Boolean> toSerializeMergedCarrierNetworkOffloadMap();

        /**
         * Retrieve the unmerged carrier network offload map from the data source to serialize to
         * disk.
         */
        Map<Integer, Boolean> toSerializeUnmergedCarrierNetworkOffloadMap();

        /**
         * Should be called when serialize is completed.
         */
        void serializeComplete();


        /**
         * Set the merged carrier network offload map in the data source after deserialize them
         * from disk.
         */
        void fromMergedCarrierNetworkOffloadMapDeserialized(
                Map<Integer, Boolean> carrierOffloadMap);

        /**
         * Set the unmerged carrier network offload map in the data source after serializing them
         * from disk.
         */
        void fromUnmergedCarrierNetworkOffloadMapDeserialized(
                Map<Integer, Boolean> subscriptionOffloadMap);

        /**
         * Clear internal data structure in preparation for user switch or initial store read.
         */
        void reset();

        /**
         * Indicates whether there is new data to serialize.
         */
        boolean hasNewDataToSerialize();
    }

    private final DataSource mDataSource;

    /**
     * Set the data source fot store data.
     */
    public WifiCarrierInfoStoreManagerData(@NonNull DataSource dataSource) {
        mDataSource = dataSource;
    }

    @Override
    public void serializeData(XmlSerializer out, WifiConfigStoreEncryptionUtil encryptionUtil)
            throws XmlPullParserException, IOException {
        XmlUtil.writeNextValue(out, XML_TAG_MERGED_CARRIER_NETWORK_OFFLOAD_MAP,
                integerMapToStringMap(mDataSource.toSerializeMergedCarrierNetworkOffloadMap()));
        XmlUtil.writeNextValue(out, XML_TAG_UNMERGED_CARRIER_NETWORK_OFFLOAD_MAP,
                integerMapToStringMap(mDataSource.toSerializeUnmergedCarrierNetworkOffloadMap()));
        mDataSource.serializeComplete();
    }

    @Override
    public void deserializeData(XmlPullParser in, int outerTagDepth, int version,
            WifiConfigStoreEncryptionUtil encryptionUtil)
            throws XmlPullParserException, IOException {
        mDataSource.reset();
        if (in != null) {
            parseWifiCarrierInfoStoreMaps(in, outerTagDepth);
        }
    }

    private void parseWifiCarrierInfoStoreMaps(XmlPullParser in,
            int outerTagDepth)
            throws XmlPullParserException, IOException {
        while (!XmlUtil.isNextSectionEnd(in, outerTagDepth)) {
            String[] valueName = new String[1];
            Object value = XmlUtil.readCurrentValue(in, valueName);
            if (valueName[0] == null) {
                throw new XmlPullParserException("Missing value name");
            }
            switch (valueName[0]) {
                case XML_TAG_MERGED_CARRIER_NETWORK_OFFLOAD_MAP:
                    if (value instanceof Map) {
                        mDataSource.fromMergedCarrierNetworkOffloadMapDeserialized(
                                stringMapToIntegerMap((Map<String, Boolean>) value));
                    }
                    break;
                case XML_TAG_UNMERGED_CARRIER_NETWORK_OFFLOAD_MAP:
                    if (value instanceof Map) {
                        mDataSource.fromUnmergedCarrierNetworkOffloadMapDeserialized(
                                stringMapToIntegerMap((Map<String, Boolean>) value));
                    }
                    break;
                default:
                    Log.w(TAG, "Unknown tag under "
                            + XML_TAG_SECTION_HEADER
                            + ": " + valueName[0]);
                    break;
            }
        }
    }

    private Map<String, Boolean> integerMapToStringMap(Map<Integer, Boolean> input) {
        Map<String, Boolean> output = new HashMap<>();
        if (input == null) {
            return output;
        }
        for (Map.Entry<Integer, Boolean> entry : input.entrySet()) {
            output.put(Integer.toString(entry.getKey()), entry.getValue());
        }
        return output;
    }

    private Map<Integer, Boolean> stringMapToIntegerMap(Map<String, Boolean> input) {
        Map<Integer, Boolean> output = new HashMap<>();
        if (input == null) {
            return output;
        }
        for (Map.Entry<String, Boolean> entry : input.entrySet()) {
            try {
                output.put(Integer.valueOf(entry.getKey()), entry.getValue());
            } catch (NumberFormatException e) {
                Log.e(TAG, "Failed to Integer convert: " + entry.getKey());
            }
        }
        return output;
    }

    @Override
    public void resetData() {
        mDataSource.reset();
    }

    @Override
    public boolean hasNewDataToSerialize() {
        return mDataSource.hasNewDataToSerialize();
    }

    @Override
    public String getName() {
        return XML_TAG_SECTION_HEADER;
    }

    @Override
    public int getStoreFileId() {
        // User general store.
        return WifiConfigStore.STORE_FILE_SHARED_GENERAL;
    }
}
