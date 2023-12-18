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

import static org.junit.Assert.assertEquals;
import static org.mockito.Mockito.inOrder;
import static org.mockito.Mockito.verify;
import static org.mockito.Mockito.verifyNoMoreInteractions;
import static org.mockito.Mockito.when;

import android.util.Xml;

import com.android.internal.util.FastXmlSerializer;

import org.junit.Before;
import org.junit.Test;
import org.mockito.ArgumentCaptor;
import org.mockito.InOrder;
import org.mockito.Mock;
import org.mockito.MockitoAnnotations;
import org.xmlpull.v1.XmlPullParser;
import org.xmlpull.v1.XmlSerializer;

import java.io.ByteArrayInputStream;
import java.io.ByteArrayOutputStream;
import java.nio.charset.StandardCharsets;
import java.util.HashMap;
import java.util.Map;

public class WifiCarrierInfoStoreManagerDataTest {
    private static final int TEST_CARRIER_ID = 1911;
    private static final int TEST_SUB_ID = 3;

    private @Mock WifiCarrierInfoStoreManagerData.DataSource mDataSource;
    private WifiCarrierInfoStoreManagerData mWifiCarrierInfoStoreManagerData;

    private final Map<Integer, Boolean> mImsiPrivacyProtectionExemptionMap = new HashMap<>();
    private final Map<Integer, Boolean> mMergedCarrierOffloadMap = new HashMap<>();
    private final Map<Integer, Boolean> mUnmergedCarrierOffloadMap = new HashMap<>();

    @Before
    public void setUp() throws Exception {
        MockitoAnnotations.initMocks(this);
        mWifiCarrierInfoStoreManagerData =
                new WifiCarrierInfoStoreManagerData(mDataSource);
    }

    /**
     * Helper function for serializing configuration data to a XML block.
     */
    private byte[] serializeData() throws Exception {
        final XmlSerializer out = new FastXmlSerializer();
        final ByteArrayOutputStream outputStream = new ByteArrayOutputStream();
        out.setOutput(outputStream, StandardCharsets.UTF_8.name());
        mWifiCarrierInfoStoreManagerData.serializeData(out, null);
        out.flush();
        return outputStream.toByteArray();
    }

    /**
     * Helper function for parsing configuration data from a XML block.
     */
    private void deserializeData(byte[] data) throws Exception {
        final XmlPullParser in = Xml.newPullParser();
        final ByteArrayInputStream inputStream = new ByteArrayInputStream(data);
        in.setInput(inputStream, StandardCharsets.UTF_8.name());
        mWifiCarrierInfoStoreManagerData.deserializeData(in, in.getDepth(),
                WifiConfigStore.ENCRYPT_CREDENTIALS_CONFIG_STORE_DATA_VERSION, null);
    }

    /**
     * Verify store file Id.
     */
    @Test
    public void verifyStoreFileId() throws Exception {
        assertEquals(WifiConfigStore.STORE_FILE_SHARED_GENERAL,
                mWifiCarrierInfoStoreManagerData.getStoreFileId());
    }

    /**
     * Verify serialize and deserialize Protection exemption map.
     */
    @Test
    public void testSerializeDeserialize() throws Exception {
        mImsiPrivacyProtectionExemptionMap.put(TEST_CARRIER_ID, true);
        mMergedCarrierOffloadMap.put(TEST_SUB_ID, true);
        mUnmergedCarrierOffloadMap.put(TEST_SUB_ID, false);
        assertSerializeDeserialize();
    }

    /**
     * Verify serialize and deserialize empty protection exemption map.
     */
    @Test
    public void testSerializeDeserializeEmpty() throws Exception {
        mImsiPrivacyProtectionExemptionMap.clear();
        mMergedCarrierOffloadMap.clear();
        mUnmergedCarrierOffloadMap.clear();
        assertSerializeDeserialize();
    }

    @Test
    public void testDeserializeOnNewDeviceOrNewUser() throws Exception {
        InOrder inOrder = inOrder(mDataSource);
        mWifiCarrierInfoStoreManagerData.deserializeData(null, 0,
                WifiConfigStore.ENCRYPT_CREDENTIALS_CONFIG_STORE_DATA_VERSION, null);
        inOrder.verify(mDataSource).reset();
        verifyNoMoreInteractions(mDataSource);
    }


    private void assertSerializeDeserialize() throws Exception {
        InOrder inOrder = inOrder(mDataSource);
        // Setup the data to serialize.
        when(mDataSource.toSerializeMergedCarrierNetworkOffloadMap()).thenReturn(
                mMergedCarrierOffloadMap);
        when(mDataSource.toSerializeUnmergedCarrierNetworkOffloadMap())
                .thenReturn(mUnmergedCarrierOffloadMap);

        // Serialize/deserialize data.
        deserializeData(serializeData());
        inOrder.verify(mDataSource).serializeComplete();
        // Verify the deserialized data.
        ArgumentCaptor<HashMap> deserializedMap =
                ArgumentCaptor.forClass(HashMap.class);

        verify(mDataSource)
                .fromMergedCarrierNetworkOffloadMapDeserialized(deserializedMap.capture());
        assertEquals(mMergedCarrierOffloadMap,
                deserializedMap.getValue());

        verify(mDataSource)
                .fromUnmergedCarrierNetworkOffloadMapDeserialized(deserializedMap.capture());
        assertEquals(mUnmergedCarrierOffloadMap,
                deserializedMap.getValue());
    }
}
