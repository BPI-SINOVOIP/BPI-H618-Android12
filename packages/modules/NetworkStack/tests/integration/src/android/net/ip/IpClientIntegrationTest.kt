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

package android.net.ip

import android.net.ipmemorystore.NetworkAttributes
import android.util.ArrayMap
import org.mockito.Mockito.any
import org.mockito.ArgumentCaptor
import org.mockito.Mockito.eq
import org.mockito.Mockito.never
import org.mockito.Mockito.timeout
import org.mockito.Mockito.verify

/**
 * Tests for IpClient, run with signature permissions.
 */
class IpClientIntegrationTest : IpClientIntegrationTestCommon() {
    private val mEnabledFeatures = ArrayMap<String, Boolean>()

    override fun makeIIpClient(ifaceName: String, cb: IIpClientCallbacks): IIpClient {
        return mIpc.makeConnector()
    }

    override fun useNetworkStackSignature() = true

    override fun isFeatureEnabled(name: String, defaultEnabled: Boolean): Boolean {
        return mEnabledFeatures.get(name) ?: defaultEnabled
    }

    override fun setFeatureEnabled(name: String, enabled: Boolean) {
        mEnabledFeatures.put(name, enabled)
    }

    override fun getStoredNetworkAttributes(l2Key: String, timeout: Long): NetworkAttributes {
        val networkAttributesCaptor = ArgumentCaptor.forClass(NetworkAttributes::class.java)

        verify(mIpMemoryStore, timeout(timeout))
                .storeNetworkAttributes(eq(l2Key), networkAttributesCaptor.capture(), any())
        return networkAttributesCaptor.value
    }

    override fun assertIpMemoryNeverStoreNetworkAttributes(l2Key: String, timeout: Long) {
        verify(mIpMemoryStore, never()).storeNetworkAttributes(eq(l2Key), any(), any())
    }
}
