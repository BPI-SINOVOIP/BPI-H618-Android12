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

package android.net.vcn.cts;

import static android.net.NetworkCapabilities.NET_CAPABILITY_DUN;
import static android.net.NetworkCapabilities.NET_CAPABILITY_INTERNET;
import static android.net.ipsec.ike.IkeSessionParams.IKE_OPTION_MOBIKE;

import static org.junit.Assert.assertArrayEquals;
import static org.junit.Assert.assertEquals;
import static org.junit.Assert.fail;

import android.net.ipsec.ike.IkeSessionParams;
import android.net.ipsec.ike.IkeTunnelConnectionParams;
import android.net.vcn.VcnGatewayConnectionConfig;

import androidx.test.ext.junit.runners.AndroidJUnit4;

import org.junit.Test;
import org.junit.runner.RunWith;

import java.util.concurrent.TimeUnit;

@RunWith(AndroidJUnit4.class)
public class VcnGatewayConnectionConfigTest extends VcnTestBase {
    private static final String VCN_GATEWAY_CONNECTION_NAME = "test-vcn-gateway-connection";
    private static final long[] RETRY_INTERNAL_MILLIS =
            new long[] {
                TimeUnit.SECONDS.toMillis(1),
                TimeUnit.MINUTES.toMillis(1),
                TimeUnit.HOURS.toMillis(1)
            };
    private static final int MAX_MTU = 1360;

    public static VcnGatewayConnectionConfig.Builder buildVcnGatewayConnectionConfigBase() {
        return new VcnGatewayConnectionConfig.Builder(
                        VCN_GATEWAY_CONNECTION_NAME, buildTunnelConnectionParams())
                .addExposedCapability(NET_CAPABILITY_INTERNET)
                .setRetryIntervalsMillis(RETRY_INTERNAL_MILLIS)
                .setMaxMtu(MAX_MTU);
    }

    private static VcnGatewayConnectionConfig buildVcnGatewayConnectionConfig() {
        return buildVcnGatewayConnectionConfigBase().build();
    }

    @Test
    public void testBuildVcnGatewayConnectionConfig() throws Exception {
        final VcnGatewayConnectionConfig gatewayConnConfig = buildVcnGatewayConnectionConfig();

        assertEquals(VCN_GATEWAY_CONNECTION_NAME, gatewayConnConfig.getGatewayConnectionName());
        assertEquals(buildTunnelConnectionParams(), gatewayConnConfig.getTunnelConnectionParams());
        assertArrayEquals(
                new int[] {NET_CAPABILITY_INTERNET}, gatewayConnConfig.getExposedCapabilities());
        assertArrayEquals(RETRY_INTERNAL_MILLIS, gatewayConnConfig.getRetryIntervalsMillis());
    }

    @Test
    public void testBuilderAddRemove() throws Exception {
        final VcnGatewayConnectionConfig gatewayConnConfig =
                buildVcnGatewayConnectionConfigBase()
                        .addExposedCapability(NET_CAPABILITY_DUN)
                        .removeExposedCapability(NET_CAPABILITY_DUN)
                        .build();

        assertArrayEquals(
                new int[] {NET_CAPABILITY_INTERNET}, gatewayConnConfig.getExposedCapabilities());
    }

    @Test
    public void testBuildWithoutMobikeEnabled() {
        final IkeSessionParams ikeParams =
                getIkeSessionParamsBase().removeIkeOption(IKE_OPTION_MOBIKE).build();
        final IkeTunnelConnectionParams tunnelParams = buildTunnelConnectionParams(ikeParams);

        try {
            new VcnGatewayConnectionConfig.Builder(VCN_GATEWAY_CONNECTION_NAME, tunnelParams);
            fail("Expected exception if MOBIKE not configured");
        } catch (IllegalArgumentException e) {
        }
    }
}
