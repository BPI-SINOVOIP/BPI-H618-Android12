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

package com.google.android.iwlan.epdg;

import static org.junit.Assert.*;
import static org.mockito.Mockito.*;

import android.content.Context;
import android.net.Network;
import android.os.PersistableBundle;
import android.telephony.CarrierConfigManager;
import android.telephony.CellIdentityGsm;
import android.telephony.CellIdentityLte;
import android.telephony.CellIdentityNr;
import android.telephony.CellIdentityWcdma;
import android.telephony.CellInfo;
import android.telephony.CellInfoGsm;
import android.telephony.CellInfoLte;
import android.telephony.CellInfoNr;
import android.telephony.CellInfoWcdma;
import android.telephony.SubscriptionInfo;
import android.telephony.SubscriptionManager;
import android.telephony.TelephonyManager;
import android.util.Log;

import com.google.android.iwlan.IwlanError;

import org.junit.Before;
import org.junit.Rule;
import org.junit.Test;
import org.junit.runner.RunWith;
import org.junit.runners.JUnit4;
import org.mockito.Mock;
import org.mockito.junit.MockitoJUnit;
import org.mockito.junit.MockitoRule;

import java.net.InetAddress;
import java.util.*;
import java.util.concurrent.CountDownLatch;
import java.util.concurrent.TimeUnit;

@RunWith(JUnit4.class)
public class EpdgSelectorTest {
    private static final String TAG = "EpdgSelectorTest";
    private EpdgSelector mEpdgSelector;
    public static final int DEFAULT_SLOT_INDEX = 0;

    private static final String TEST_IP_ADDRESS = "127.0.0.1";
    private static final String TEST_IP_ADDRESS_1 = "127.0.0.2";
    private static final String TEST_IP_ADDRESS_2 = "127.0.0.3";
    private static final String TEST_IPV6_ADDRESS = "0000:0000:0000:0000:0000:0000:0000:0001";

    private static int testPcoIdIPv6 = 0xFF01;
    private static int testPcoIdIPv4 = 0xFF02;

    private String testPcoString = "testPcoData";
    private byte[] pcoData = testPcoString.getBytes();
    private List<String> ehplmnList = new ArrayList<String>();

    @Rule public final MockitoRule mockito = MockitoJUnit.rule();

    @Mock private Context mMockContext;
    @Mock private Network mMockNetwork;
    @Mock private SubscriptionManager mMockSubscriptionManager;
    @Mock private SubscriptionInfo mMockSubscriptionInfo;
    @Mock private CarrierConfigManager mMockCarrierConfigManager;
    @Mock private TelephonyManager mMockTelephonyManager;

    @Mock private CellInfoGsm mMockCellInfoGsm;
    @Mock private CellIdentityGsm mMockCellIdentityGsm;
    @Mock private CellInfoWcdma mMockCellInfoWcdma;
    @Mock private CellIdentityWcdma mMockCellIdentityWcdma;
    @Mock private CellInfoLte mMockCellInfoLte;
    @Mock private CellIdentityLte mMockCellIdentityLte;
    @Mock private CellInfoNr mMockCellInfoNr;
    @Mock private CellIdentityNr mMockCellIdentityNr;

    private PersistableBundle mTestBundle;

    @Before
    public void setUp() throws Exception {
        mEpdgSelector = new EpdgSelector(mMockContext, DEFAULT_SLOT_INDEX);

        when(mMockContext.getSystemService(eq(SubscriptionManager.class)))
                .thenReturn(mMockSubscriptionManager);

        when(mMockSubscriptionManager.getActiveSubscriptionInfoForSimSlotIndex(anyInt()))
                .thenReturn(mMockSubscriptionInfo);

        when(mMockSubscriptionInfo.getMccString()).thenReturn("311");

        when(mMockSubscriptionInfo.getMncString()).thenReturn("120");

        when(mMockContext.getSystemService(eq(TelephonyManager.class)))
                .thenReturn(mMockTelephonyManager);

        when(mMockTelephonyManager.createForSubscriptionId(anyInt()))
                .thenReturn(mMockTelephonyManager);

        ehplmnList.add("300120");
        when(mMockTelephonyManager.getEquivalentHomePlmns()).thenReturn(ehplmnList);

        // Mock carrier configs with test bundle
        mTestBundle = new PersistableBundle();
        when(mMockContext.getSystemService(eq(CarrierConfigManager.class)))
                .thenReturn(mMockCarrierConfigManager);
        when(mMockCarrierConfigManager.getConfigForSubId(anyInt())).thenReturn(mTestBundle);
    }

    @Test
    public void testStaticMethodPass() throws Exception {
        // Set Network.getAllByName mock
        final String testStaticAddress = "epdg.epc.mnc088.mcc888.pub.3gppnetwork.org";
        when(mMockNetwork.getAllByName(eq(testStaticAddress)))
                .thenReturn(new InetAddress[] {InetAddress.getByName(TEST_IP_ADDRESS)});

        // Set carrier config mock
        mTestBundle.putIntArray(
                CarrierConfigManager.Iwlan.KEY_EPDG_ADDRESS_PRIORITY_INT_ARRAY,
                new int[] {CarrierConfigManager.Iwlan.EPDG_ADDRESS_STATIC});
        mTestBundle.putString(
                CarrierConfigManager.Iwlan.KEY_EPDG_STATIC_ADDRESS_STRING, testStaticAddress);

        ArrayList<InetAddress> testInetAddresses =
                getValidatedServerListWithDefaultParams(false /*isEmergency*/);

        InetAddress expectedAddress = InetAddress.getByName(TEST_IP_ADDRESS);

        assertEquals(testInetAddresses.size(), 1);
        assertEquals(testInetAddresses.get(0), expectedAddress);
    }

    @Test
    public void testPlmnResolutionMethod() throws Exception {
        testPlmnResolutionMethod(false);
    }

    @Test
    public void testPlmnResolutionMethodForEmergency() throws Exception {
        testPlmnResolutionMethod(true);
    }

    @Test
    public void testPlmnResolutionMethodWithNoPlmnInCarrierConfig() throws Exception {
        // setUp() fills default values for mcc-mnc
        String expectedFqdn1 = "epdg.epc.mnc120.mcc311.pub.3gppnetwork.org";
        when(mMockNetwork.getAllByName(eq(expectedFqdn1)))
                .thenReturn(new InetAddress[] {InetAddress.getByName(TEST_IP_ADDRESS_1)});

        ArrayList<InetAddress> testInetAddresses =
                getValidatedServerListWithDefaultParams(false /*isEmergency*/);

        assertEquals(testInetAddresses.size(), 1);
        assertTrue(testInetAddresses.contains(InetAddress.getByName(TEST_IP_ADDRESS_1)));
    }

    private void testPlmnResolutionMethod(boolean isEmergency) throws Exception {
        String expectedFqdn1 =
                (isEmergency)
                        ? "sos.epdg.epc.mnc480.mcc310.pub.3gppnetwork.org"
                        : "epdg.epc.mnc480.mcc310.pub.3gppnetwork.org";
        String expectedFqdn2 =
                (isEmergency)
                        ? "sos.epdg.epc.mnc120.mcc300.pub.3gppnetwork.org"
                        : "epdg.epc.mnc120.mcc300.pub.3gppnetwork.org";
        String expectedFqdn3 =
                (isEmergency)
                        ? "sos.epdg.epc.mnc120.mcc311.pub.3gppnetwork.org"
                        : "epdg.epc.mnc120.mcc311.pub.3gppnetwork.org";

        mTestBundle.putIntArray(
                CarrierConfigManager.Iwlan.KEY_EPDG_ADDRESS_PRIORITY_INT_ARRAY,
                new int[] {CarrierConfigManager.Iwlan.EPDG_ADDRESS_PLMN});
        mTestBundle.putStringArray(
                CarrierConfigManager.Iwlan.KEY_MCC_MNCS_STRING_ARRAY,
                new String[] {"310-480", "300-120", "311-120"});

        when(mMockNetwork.getAllByName(eq(expectedFqdn1)))
                .thenReturn(new InetAddress[] {InetAddress.getByName(TEST_IP_ADDRESS_1)});
        when(mMockNetwork.getAllByName(eq(expectedFqdn2)))
                .thenReturn(new InetAddress[] {InetAddress.getByName(TEST_IP_ADDRESS_2)});
        when(mMockNetwork.getAllByName(eq(expectedFqdn3)))
                .thenReturn(new InetAddress[] {InetAddress.getByName(TEST_IP_ADDRESS)});

        ArrayList<InetAddress> testInetAddresses =
                getValidatedServerListWithDefaultParams(isEmergency);

        verify(mMockNetwork).getAllByName(expectedFqdn1);
        verify(mMockNetwork).getAllByName(expectedFqdn2);
        verify(mMockNetwork).getAllByName(expectedFqdn3);

        assertEquals(testInetAddresses.size(), 3);
        assertEquals(testInetAddresses.get(0), InetAddress.getByName(TEST_IP_ADDRESS));
        assertEquals(testInetAddresses.get(1), InetAddress.getByName(TEST_IP_ADDRESS_2));
        assertEquals(testInetAddresses.get(2), InetAddress.getByName(TEST_IP_ADDRESS_1));
    }

    @Test
    public void testCarrierConfigStaticAddressList() throws Exception {
        // Set Network.getAllByName mock
        final String addr1 = "epdg.epc.mnc480.mcc310.pub.3gppnetwork.org";
        final String addr2 = "epdg.epc.mnc120.mcc300.pub.3gppnetwork.org";
        final String addr3 = "epdg.epc.mnc120.mcc311.pub.3gppnetwork.org";
        final String testStaticAddress = addr1 + "," + addr2 + "," + addr3;
        when(mMockNetwork.getAllByName(eq(addr1)))
                .thenReturn(new InetAddress[] {InetAddress.getByName(TEST_IP_ADDRESS_1)});
        when(mMockNetwork.getAllByName(eq(addr2)))
                .thenReturn(new InetAddress[] {InetAddress.getByName(TEST_IP_ADDRESS_2)});
        when(mMockNetwork.getAllByName(eq(addr3)))
                .thenReturn(new InetAddress[] {InetAddress.getByName(TEST_IP_ADDRESS)});

        // Set carrier config mock
        mTestBundle.putIntArray(
                CarrierConfigManager.Iwlan.KEY_EPDG_ADDRESS_PRIORITY_INT_ARRAY,
                new int[] {CarrierConfigManager.Iwlan.EPDG_ADDRESS_STATIC});
        mTestBundle.putString(
                CarrierConfigManager.Iwlan.KEY_EPDG_STATIC_ADDRESS_STRING, testStaticAddress);

        ArrayList<InetAddress> testInetAddresses =
                getValidatedServerListWithDefaultParams(false /*isEmergency*/);

        assertEquals(testInetAddresses.size(), 3);
        assertEquals(testInetAddresses.get(0), InetAddress.getByName(TEST_IP_ADDRESS_1));
        assertEquals(testInetAddresses.get(1), InetAddress.getByName(TEST_IP_ADDRESS_2));
        assertEquals(testInetAddresses.get(2), InetAddress.getByName(TEST_IP_ADDRESS));
    }

    private ArrayList<InetAddress> getValidatedServerListWithDefaultParams(boolean isEmergency)
            throws Exception {
        ArrayList<InetAddress> testInetAddresses = new ArrayList<InetAddress>();
        final CountDownLatch latch = new CountDownLatch(1);
        IwlanError ret =
                mEpdgSelector.getValidatedServerList(
                        1234,
                        EpdgSelector.PROTO_FILTER_IPV4V6,
                        false /*isRoaming*/,
                        isEmergency,
                        mMockNetwork,
                        new EpdgSelector.EpdgSelectorCallback() {
                            @Override
                            public void onServerListChanged(
                                    int transactionId, ArrayList<InetAddress> validIPList) {
                                assertEquals(transactionId, 1234);

                                for (InetAddress mInetAddress : validIPList) {
                                    testInetAddresses.add(mInetAddress);
                                }
                                Log.d(TAG, "onServerListChanged received");
                                latch.countDown();
                            }

                            @Override
                            public void onError(int transactionId, IwlanError epdgSelectorError) {
                                Log.d(TAG, "onError received");
                                latch.countDown();
                            }
                        });

        assertEquals(ret.getErrorType(), IwlanError.NO_ERROR);
        latch.await(1, TimeUnit.SECONDS);
        return testInetAddresses;
    }

    @Test
    public void testSetPcoData() throws Exception {
        addTestPcoIdsToTestConfigBundle();

        boolean retIPv6 = mEpdgSelector.setPcoData(testPcoIdIPv6, pcoData);
        boolean retIPv4 = mEpdgSelector.setPcoData(testPcoIdIPv4, pcoData);
        boolean retIncorrect = mEpdgSelector.setPcoData(0xFF00, pcoData);

        assertTrue(retIPv6);
        assertTrue(retIPv4);
        assertFalse(retIncorrect);
    }

    @Test
    public void testPcoResolutionMethod() throws Exception {
        mTestBundle.putIntArray(
                CarrierConfigManager.Iwlan.KEY_EPDG_ADDRESS_PRIORITY_INT_ARRAY,
                new int[] {CarrierConfigManager.Iwlan.EPDG_ADDRESS_PCO});
        addTestPcoIdsToTestConfigBundle();

        mEpdgSelector.clearPcoData();
        boolean retIPv6 =
                mEpdgSelector.setPcoData(
                        testPcoIdIPv6, InetAddress.getByName(TEST_IPV6_ADDRESS).getAddress());
        boolean retIPv4 =
                mEpdgSelector.setPcoData(
                        testPcoIdIPv4, InetAddress.getByName(TEST_IP_ADDRESS).getAddress());

        ArrayList<InetAddress> testInetAddresses =
                getValidatedServerListWithDefaultParams(false /* isEmergency */);

        assertEquals(testInetAddresses.size(), 2);
        assertTrue(testInetAddresses.contains(InetAddress.getByName(TEST_IP_ADDRESS)));
        assertTrue(testInetAddresses.contains(InetAddress.getByName(TEST_IPV6_ADDRESS)));
    }

    private void addTestPcoIdsToTestConfigBundle() {
        mTestBundle.putInt(CarrierConfigManager.Iwlan.KEY_EPDG_PCO_ID_IPV6_INT, testPcoIdIPv6);
        mTestBundle.putInt(CarrierConfigManager.Iwlan.KEY_EPDG_PCO_ID_IPV4_INT, testPcoIdIPv4);
    }

    @Test
    public void testCellularResolutionMethod() throws Exception {
        testCellularResolutionMethod(false);
    }

    @Test
    public void testCellularResolutionMethodForEmergency() throws Exception {
        testCellularResolutionMethod(true);
    }

    private void testCellularResolutionMethod(boolean isEmergency) throws Exception {
        int testMcc = 311;
        int testMnc = 120;
        String testMccString = "311";
        String testMncString = "120";
        int testLac = 65484;
        int testTac = 65484;
        int testNrTac = 16764074;
        String fqdn1_emergency = "lacffcc.sos.epdg.epc.mnc120.mcc311.pub.3gppnetwork.org";
        String fqdn1 = "lacffcc.epdg.epc.mnc120.mcc311.pub.3gppnetwork.org";
        String fqdn2_emergency =
                "tac-lbcc.tac-hbff.tac.sos.epdg.epc.mnc120.mcc311.pub.3gppnetwork.org";
        String fqdn2 = "tac-lbcc.tac-hbff.tac.epdg.epc.mnc120.mcc311.pub.3gppnetwork.org";
        String fqdn3_emergency =
                "tac-lbaa.tac-mbcc.tac-hbff.5gstac.sos.epdg.epc.mnc120.mcc311.pub.3gppnetwork.org";
        String fqdn3 =
                "tac-lbaa.tac-mbcc.tac-hbff.5gstac.epdg.epc.mnc120.mcc311.pub.3gppnetwork.org";
        List<CellInfo> fakeCellInfoArray = new ArrayList<CellInfo>();

        mTestBundle.putIntArray(
                CarrierConfigManager.Iwlan.KEY_EPDG_ADDRESS_PRIORITY_INT_ARRAY,
                new int[] {CarrierConfigManager.Iwlan.EPDG_ADDRESS_CELLULAR_LOC});

        // Set cell info mock
        fakeCellInfoArray.add(mMockCellInfoGsm);
        when(mMockCellInfoGsm.isRegistered()).thenReturn(true);
        when(mMockCellInfoGsm.getCellIdentity()).thenReturn(mMockCellIdentityGsm);
        when(mMockCellIdentityGsm.getMcc()).thenReturn(testMcc);
        when(mMockCellIdentityGsm.getMnc()).thenReturn(testMnc);
        when(mMockCellIdentityGsm.getLac()).thenReturn(testLac);

        fakeCellInfoArray.add(mMockCellInfoWcdma);
        when(mMockCellInfoWcdma.isRegistered()).thenReturn(true);
        when(mMockCellInfoWcdma.getCellIdentity()).thenReturn(mMockCellIdentityWcdma);
        when(mMockCellIdentityWcdma.getMcc()).thenReturn(testMcc);
        when(mMockCellIdentityWcdma.getMnc()).thenReturn(testMnc);
        when(mMockCellIdentityWcdma.getLac()).thenReturn(testLac);

        fakeCellInfoArray.add(mMockCellInfoLte);
        when(mMockCellInfoLte.isRegistered()).thenReturn(true);
        when(mMockCellInfoLte.getCellIdentity()).thenReturn(mMockCellIdentityLte);
        when(mMockCellIdentityLte.getMcc()).thenReturn(testMcc);
        when(mMockCellIdentityLte.getMnc()).thenReturn(testMnc);
        when(mMockCellIdentityLte.getTac()).thenReturn(testTac);

        fakeCellInfoArray.add(mMockCellInfoNr);
        when(mMockCellInfoNr.isRegistered()).thenReturn(true);
        when(mMockCellInfoNr.getCellIdentity()).thenReturn(mMockCellIdentityNr);
        when(mMockCellIdentityNr.getMccString()).thenReturn(testMccString);
        when(mMockCellIdentityNr.getMncString()).thenReturn(testMncString);
        when(mMockCellIdentityNr.getTac()).thenReturn(testNrTac);

        when(mMockTelephonyManager.getAllCellInfo()).thenReturn(fakeCellInfoArray);

        String expectedFqdn1 = (isEmergency) ? fqdn1_emergency : fqdn1;
        String expectedFqdn2 = (isEmergency) ? fqdn2_emergency : fqdn2;
        String expectedFqdn3 = (isEmergency) ? fqdn3_emergency : fqdn3;

        when(mMockNetwork.getAllByName(eq(expectedFqdn1)))
                .thenReturn(new InetAddress[] {InetAddress.getByName(TEST_IP_ADDRESS)});
        when(mMockNetwork.getAllByName(eq(expectedFqdn2)))
                .thenReturn(new InetAddress[] {InetAddress.getByName(TEST_IP_ADDRESS_1)});
        when(mMockNetwork.getAllByName(eq(expectedFqdn3)))
                .thenReturn(new InetAddress[] {InetAddress.getByName(TEST_IP_ADDRESS_2)});

        ArrayList<InetAddress> testInetAddresses =
                getValidatedServerListWithDefaultParams(isEmergency);

        verify(mMockNetwork, times(2)).getAllByName(expectedFqdn1);
        verify(mMockNetwork).getAllByName(expectedFqdn2);
        verify(mMockNetwork).getAllByName(expectedFqdn3);

        assertEquals(testInetAddresses.size(), 3);
        assertEquals(testInetAddresses.get(0), InetAddress.getByName(TEST_IP_ADDRESS));
        assertEquals(testInetAddresses.get(1), InetAddress.getByName(TEST_IP_ADDRESS_1));
        assertEquals(testInetAddresses.get(2), InetAddress.getByName(TEST_IP_ADDRESS_2));
    }
}
