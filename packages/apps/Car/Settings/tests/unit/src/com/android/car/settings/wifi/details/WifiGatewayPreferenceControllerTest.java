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

package com.android.car.settings.wifi.details;

import static com.google.common.truth.Truth.assertThat;

import static org.mockito.Mockito.when;

import android.car.drivingstate.CarUxRestrictions;
import android.content.Context;
import android.net.IpPrefix;
import android.net.LinkProperties;
import android.net.RouteInfo;

import androidx.lifecycle.LifecycleOwner;
import androidx.test.core.app.ApplicationProvider;
import androidx.test.ext.junit.runners.AndroidJUnit4;

import com.android.car.settings.common.FragmentController;
import com.android.car.settings.common.PreferenceControllerTestUtil;
import com.android.car.settings.testutils.TestLifecycleOwner;
import com.android.wifitrackerlib.WifiEntry;

import org.junit.Before;
import org.junit.Test;
import org.junit.runner.RunWith;
import org.mockito.Mock;
import org.mockito.MockitoAnnotations;

import java.net.InetAddress;
import java.util.Arrays;

@RunWith(AndroidJUnit4.class)
public class WifiGatewayPreferenceControllerTest {
    private static final String GATE_WAY = "gateway";

    private Context mContext = ApplicationProvider.getApplicationContext();
    private LifecycleOwner mLifecycleOwner;
    private WifiDetailsPreference mPreference;
    private WifiGatewayPreferenceController mPreferenceController;
    private CarUxRestrictions mCarUxRestrictions;

    @Mock
    private FragmentController mFragmentController;
    @Mock
    private WifiEntry mMockWifiEntry;
    @Mock
    private WifiInfoProvider mMockWifiInfoProvider;
    @Mock
    private LinkProperties mMockLinkProperties;
    @Mock
    private RouteInfo mMockRouteInfo;
    @Mock
    private InetAddress mMockInetAddress;
    @Mock
    private IpPrefix mMockIpPrefix;

    @Before
    public void setUp() {
        MockitoAnnotations.initMocks(this);
        mLifecycleOwner = new TestLifecycleOwner();

        mCarUxRestrictions = new CarUxRestrictions.Builder(/* reqOpt= */ true,
                CarUxRestrictions.UX_RESTRICTIONS_BASELINE, /* timestamp= */ 0).build();

        mPreference = new WifiDetailsPreference(mContext);
        mPreferenceController = new WifiGatewayPreferenceController(mContext,
                /* preferenceKey= */ "key", mFragmentController, mCarUxRestrictions);
        mPreferenceController.init(mMockWifiEntry, mMockWifiInfoProvider);
        PreferenceControllerTestUtil.assignPreference(mPreferenceController, mPreference);
        when(mMockWifiInfoProvider.getLinkProperties()).thenReturn(mMockLinkProperties);
    }

    @Test
    public void onCreate_shouldHaveDetailTextSet() {
        when(mMockWifiEntry.getConnectedState()).thenReturn(WifiEntry.CONNECTED_STATE_CONNECTED);
        when(mMockLinkProperties.getRoutes()).thenReturn(Arrays.asList(mMockRouteInfo));
        when(mMockRouteInfo.isDefaultRoute()).thenReturn(true);
        try {
            when(mMockRouteInfo.getDestination()).thenReturn(mMockIpPrefix);
            when(mMockIpPrefix.getAddress()).thenReturn(
                    InetAddress.getByAddress(new byte[]{4, 3, 2, 1}));
        } catch (Exception e) { }
        when(mMockRouteInfo.hasGateway()).thenReturn(true);
        when(mMockRouteInfo.getGateway()).thenReturn(mMockInetAddress);
        when(mMockInetAddress.getHostAddress()).thenReturn(GATE_WAY);

        mPreferenceController.onCreate(mLifecycleOwner);
        assertThat(mPreference.isVisible()).isTrue();
        assertThat(mPreference.getDetailText()).isEqualTo(GATE_WAY);
    }

    @Test
    public void onWifiChanged_isNotActive_preferenceNotVisible() {
        when(mMockWifiEntry.getConnectedState()).thenReturn(WifiEntry.CONNECTED_STATE_DISCONNECTED);
        mPreferenceController.onCreate(mLifecycleOwner);

        assertThat(mPreference.isVisible()).isFalse();
    }
}
