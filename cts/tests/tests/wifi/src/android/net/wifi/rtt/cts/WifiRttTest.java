/*
 * Copyright (C) 2018 The Android Open Source Project
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

package android.net.wifi.rtt.cts;

import static org.mockito.Mockito.mock;

import android.net.MacAddress;
import android.net.wifi.ScanResult;
import android.net.wifi.aware.PeerHandle;
import android.net.wifi.cts.WifiBuildCompat;
import android.net.wifi.rtt.RangingRequest;
import android.net.wifi.rtt.RangingResult;
import android.net.wifi.rtt.ResponderLocation;
import android.platform.test.annotations.AppModeFull;

import com.android.compatibility.common.util.DeviceReportLog;
import com.android.compatibility.common.util.ResultType;
import com.android.compatibility.common.util.ResultUnit;

import java.util.ArrayList;
import java.util.Arrays;
import java.util.List;

/**
 * Wi-Fi RTT CTS test: range to all available Access Points which support IEEE 802.11mc.
 */
@AppModeFull(reason = "Cannot get WifiManager in instant app mode")
public class WifiRttTest extends TestBase {
    // Number of scans to do while searching for APs supporting IEEE 802.11mc
    private static final int NUM_SCANS_SEARCHING_FOR_IEEE80211MC_AP = 5;

    // Number of RTT measurements per AP
    private static final int NUM_OF_RTT_ITERATIONS = 10;

    // Maximum failure rate of RTT measurements (percentage)
    private static final int MAX_FAILURE_RATE_PERCENT = 10;

    // Maximum variation from the average measurement (measures consistency)
    private static final int MAX_VARIATION_FROM_AVERAGE_DISTANCE_MM = 2000;

    // Maximum failure rate of one-sided RTT measurements (percentage)
    private static final int MAX_NON11MC_FAILURE_RATE_PERCENT = 40;

    // Minimum valid RSSI value
    private static final int MIN_VALID_RSSI = -100;

    // Valid Mac Address
    private static final MacAddress MAC = MacAddress.fromString("00:01:02:03:04:05");

    // Interval between two ranging request.
    private static final int intervalMs = 200;

    /**
     * Test Wi-Fi RTT ranging operation:
     * - Scan for visible APs for the test AP (which is validated to support IEEE 802.11mc)
     * - Perform N (constant) RTT operations
     * - Validate:
     *   - Failure ratio < threshold (constant)
     *   - Result margin < threshold (constant)
     */
    public void testRangingToTest11mcAp() throws InterruptedException {
        if (!shouldTestWifiRtt(getContext())) {
            return;
        }

        // Scan for IEEE 802.11mc supporting APs
        ScanResult testAp = scanForTest11mcCapableAp(NUM_SCANS_SEARCHING_FOR_IEEE80211MC_AP);
        assertNotNull(
                "Cannot find any test APs which support RTT / IEEE 802.11mc - please verify that "
                        + "your test setup includes them!", testAp);

        // Perform RTT operations
        RangingRequest.Builder builder = new RangingRequest.Builder();
        builder.addAccessPoint(testAp);
        if (WifiBuildCompat.isPlatformOrWifiModuleAtLeastS(getContext())) {
            builder.setRttBurstSize(RangingRequest.getMaxRttBurstSize());
            assertTrue(RangingRequest.getDefaultRttBurstSize()
                    >= RangingRequest.getMinRttBurstSize());
            assertTrue(RangingRequest.getDefaultRttBurstSize()
                    <= RangingRequest.getMaxRttBurstSize());
        }
        RangingRequest request = builder.build();
        if (WifiBuildCompat.isPlatformOrWifiModuleAtLeastS(getContext())) {
            assertEquals(1, request.getRttResponders().size());
        }

        List<RangingResult> allResults = new ArrayList<>();
        int numFailures = 0;
        int distanceSum = 0;
        int distanceMin = 0;
        int distanceMax = 0;
        int[] statuses = new int[NUM_OF_RTT_ITERATIONS];
        int[] distanceMms = new int[NUM_OF_RTT_ITERATIONS];
        int[] distanceStdDevMms = new int[NUM_OF_RTT_ITERATIONS];
        int[] rssis = new int[NUM_OF_RTT_ITERATIONS];
        int[] numAttempted = new int[NUM_OF_RTT_ITERATIONS];
        int[] numSuccessful = new int[NUM_OF_RTT_ITERATIONS];
        long[] timestampsMs = new long[NUM_OF_RTT_ITERATIONS];
        byte[] lastLci = null;
        byte[] lastLcr = null;
        for (int i = 0; i < NUM_OF_RTT_ITERATIONS; ++i) {
            ResultCallback callback = new ResultCallback();
            mWifiRttManager.startRanging(request, mExecutor, callback);
            assertTrue("Wi-Fi RTT results: no callback on iteration " + i,
                    callback.waitForCallback());

            List<RangingResult> currentResults = callback.getResults();
            assertNotNull("Wi-Fi RTT results: null results (onRangingFailure) on iteration " + i,
                    currentResults);
            assertEquals("Wi-Fi RTT results: unexpected # of results (expect 1) on iteration " + i,
                    1, currentResults.size());
            RangingResult result = currentResults.get(0);
            assertEquals("Wi-Fi RTT results: invalid result (wrong BSSID) entry on iteration " + i,
                    result.getMacAddress().toString(), testAp.BSSID);
            assertNull("Wi-Fi RTT results: invalid result (non-null PeerHandle) entry on iteration "
                    + i, result.getPeerHandle());

            allResults.add(result);
            int status = result.getStatus();
            statuses[i] = status;
            if (status == RangingResult.STATUS_SUCCESS) {
                if (WifiBuildCompat.isPlatformOrWifiModuleAtLeastS(getContext())) {
                    assertEquals(
                            "Wi-Fi RTT results: invalid result (wrong rttBurstSize) entry on "
                                    + "iteration "
                                    + i,
                            result.getNumAttemptedMeasurements(),
                            RangingRequest.getMaxRttBurstSize());
                    assertTrue("Wi-Fi RTT results: should be a 802.11MC measurement",
                            result.is80211mcMeasurement());
                }
                distanceSum += result.getDistanceMm();
                if (i == 0) {
                    distanceMin = result.getDistanceMm();
                    distanceMax = result.getDistanceMm();
                } else {
                    distanceMin = Math.min(distanceMin, result.getDistanceMm());
                    distanceMax = Math.max(distanceMax, result.getDistanceMm());
                }

                assertTrue("Wi-Fi RTT results: invalid RSSI on iteration " + i,
                        result.getRssi() >= MIN_VALID_RSSI);

                distanceMms[i - numFailures] = result.getDistanceMm();
                distanceStdDevMms[i - numFailures] = result.getDistanceStdDevMm();
                rssis[i - numFailures] = result.getRssi();
                numAttempted[i - numFailures] = result.getNumAttemptedMeasurements();
                numSuccessful[i - numFailures] = result.getNumSuccessfulMeasurements();
                timestampsMs[i - numFailures] = result.getRangingTimestampMillis();

                byte[] currentLci = result.getLci();
                byte[] currentLcr = result.getLcr();
                if (i - numFailures > 0) {
                    assertTrue("Wi-Fi RTT results: invalid result (LCI mismatch) on iteration " + i,
                            Arrays.equals(currentLci, lastLci));
                    assertTrue("Wi-Fi RTT results: invalid result (LCR mismatch) on iteration " + i,
                            Arrays.equals(currentLcr, lastLcr));
                }
                lastLci = currentLci;
                lastLcr = currentLcr;
            } else {
                numFailures++;
            }
            // Sleep a while to avoid stress AP.
            Thread.sleep(intervalMs);
        }

        // Save results to log
        int numGoodResults = NUM_OF_RTT_ITERATIONS - numFailures;
        DeviceReportLog reportLog = new DeviceReportLog(TAG, "testRangingToTestAp");
        reportLog.addValues("status_codes", statuses, ResultType.NEUTRAL, ResultUnit.NONE);
        reportLog.addValues("distance_mm", Arrays.copyOf(distanceMms, numGoodResults),
                ResultType.NEUTRAL, ResultUnit.NONE);
        reportLog.addValues("distance_stddev_mm", Arrays.copyOf(distanceStdDevMms, numGoodResults),
                ResultType.NEUTRAL, ResultUnit.NONE);
        reportLog.addValues("rssi_dbm", Arrays.copyOf(rssis, numGoodResults), ResultType.NEUTRAL,
                ResultUnit.NONE);
        reportLog.addValues("num_attempted", Arrays.copyOf(numAttempted, numGoodResults),
                ResultType.NEUTRAL, ResultUnit.NONE);
        reportLog.addValues("num_successful", Arrays.copyOf(numSuccessful, numGoodResults),
                ResultType.NEUTRAL, ResultUnit.NONE);
        reportLog.addValues("timestamps", Arrays.copyOf(timestampsMs, numGoodResults),
                ResultType.NEUTRAL, ResultUnit.NONE);
        reportLog.submit();

        // Analyze results
        assertTrue("Wi-Fi RTT failure rate exceeds threshold: FAIL=" + numFailures + ", ITERATIONS="
                        + NUM_OF_RTT_ITERATIONS + ", AP RSSI=" + testAp.level
                        + ", AP SSID=" + testAp.SSID,
                numFailures <= NUM_OF_RTT_ITERATIONS * MAX_FAILURE_RATE_PERCENT / 100);
        if (numFailures != NUM_OF_RTT_ITERATIONS) {
            double distanceAvg = (double) distanceSum / (NUM_OF_RTT_ITERATIONS - numFailures);
            assertTrue("Wi-Fi RTT: Variation (max direction) exceeds threshold, Variation ="
                            + (distanceMax - distanceAvg),
                    (distanceMax - distanceAvg) <= MAX_VARIATION_FROM_AVERAGE_DISTANCE_MM);
            assertTrue("Wi-Fi RTT: Variation (min direction) exceeds threshold, Variation ="
                            + (distanceAvg - distanceMin),
                    (distanceAvg - distanceMin) <= MAX_VARIATION_FROM_AVERAGE_DISTANCE_MM);
            for (int i = 0; i < numGoodResults; ++i) {
                assertNotSame("Number of attempted measurements is 0", 0, numAttempted[i]);
                assertNotSame("Number of successful measurements is 0", 0, numSuccessful[i]);
            }
        }
    }

    /**
     * Validate that when a request contains more range operations than allowed (by API) that we
     * get an exception.
     */
    public void testRequestTooLarge() throws InterruptedException {
        if (!shouldTestWifiRtt(getContext())) {
            return;
        }
        ScanResult testAp = scanForTest11mcCapableAp(NUM_SCANS_SEARCHING_FOR_IEEE80211MC_AP);
        assertNotNull(
                "Cannot find any test APs which support RTT / IEEE 802.11mc - please verify that "
                        + "your test setup includes them!", testAp);

        RangingRequest.Builder builder = new RangingRequest.Builder();
        List<ScanResult> scanResults = new ArrayList<>();
        for (int i = 0; i < RangingRequest.getMaxPeers() - 2; ++i) {
            scanResults.add(testAp);
        }
        builder.addAccessPoints(scanResults);

        ScanResult testApNon80211mc = null;
        if (WifiBuildCompat.isPlatformOrWifiModuleAtLeastS(getContext())) {
            testApNon80211mc = scanForTestNon11mcCapableAp(NUM_SCANS_SEARCHING_FOR_IEEE80211MC_AP);
        }
        if (testApNon80211mc == null) {
            builder.addAccessPoints(List.of(testAp, testAp, testAp));
        } else {
            builder.addNon80211mcCapableAccessPoints(List.of(testApNon80211mc, testApNon80211mc,
                    testApNon80211mc));
        }

        try {
            mWifiRttManager.startRanging(builder.build(), mExecutor, new ResultCallback());
        } catch (IllegalArgumentException e) {
            return;
        }

        fail("Did not receive expected IllegalArgumentException when tried to range to too "
                + "many peers");
    }

    /**
     * Verify ResponderLocation API
     */
    public void testRangingToTestApWithResponderLocation() throws InterruptedException {
        if (!shouldTestWifiRtt(getContext())) {
            return;
        }
        // Scan for IEEE 802.11mc supporting APs
        ScanResult testAp = scanForTest11mcCapableAp(NUM_SCANS_SEARCHING_FOR_IEEE80211MC_AP);
        assertNotNull(
                "Cannot find any test APs which support RTT / IEEE 802.11mc - please verify that "
                        + "your test setup includes them!", testAp);

        // Perform RTT operations
        RangingRequest request = new RangingRequest.Builder().addAccessPoint(testAp).build();
        ResultCallback callback = new ResultCallback();
        mWifiRttManager.startRanging(request, mExecutor, callback);
        assertTrue("Wi-Fi RTT results: no callback! ",
                callback.waitForCallback());

        RangingResult result = callback.getResults().get(0);
        assertEquals("Ranging request not success",
                result.getStatus(), RangingResult.STATUS_SUCCESS);
        ResponderLocation responderLocation = result.getUnverifiedResponderLocation();
        if (responderLocation == null) {
            return;
        }
        assertTrue("ResponderLocation is not valid", responderLocation.isLciSubelementValid());

        // Check LCI related APIs
        int exceptionCount = 0;
        int apiCount = 0;
        try {
            apiCount++;
            responderLocation.getLatitudeUncertainty();
        } catch (IllegalStateException e) {
            exceptionCount++;
        }
        try {
            apiCount++;
            responderLocation.getLatitude();
        } catch (IllegalStateException e) {
            exceptionCount++;
        }
        try {
            apiCount++;
            responderLocation.getLongitudeUncertainty();
        } catch (IllegalStateException e) {
            exceptionCount++;
        }
        try {
            apiCount++;
            responderLocation.getLongitude();
        } catch (IllegalStateException e) {
            exceptionCount++;
        }
        try {
            apiCount++;
            responderLocation.getAltitudeType();
        } catch (IllegalStateException e) {
            exceptionCount++;
        }
        try {
            apiCount++;
            responderLocation.getAltitudeUncertainty();
        } catch (IllegalStateException e) {
            exceptionCount++;
        }
        try {
            apiCount++;
            responderLocation.getAltitude();
        } catch (IllegalStateException e) {
            exceptionCount++;
        }
        try {
            apiCount++;
            responderLocation.getDatum();
        } catch (IllegalStateException e) {
            exceptionCount++;
        }
        try {
            apiCount++;
            responderLocation.getRegisteredLocationAgreementIndication();
        } catch (IllegalStateException e) {
            exceptionCount++;
        }
        try {
            apiCount++;
            responderLocation.getLciVersion();
        } catch (IllegalStateException e) {
            exceptionCount++;
        }
        try {
            apiCount++;
            assertNotNull(responderLocation.toLocation());
        } catch (IllegalStateException e) {
            exceptionCount++;
        }
        // If LCI is not valid, all APIs should throw exception, otherwise no exception.
        assertEquals("Exception number should equal to API number",
                responderLocation.isLciSubelementValid()? 0 : apiCount, exceptionCount);

        // Verify ZaxisSubelement APIs
        apiCount = 0;
        exceptionCount = 0;

        try {
            apiCount++;
            responderLocation.getExpectedToMove();
        } catch (IllegalStateException e) {
            exceptionCount++;
        }

        try {
            apiCount++;
            responderLocation.getFloorNumber();
        } catch (IllegalStateException e) {
            exceptionCount++;
        }

        try {
            apiCount++;
            responderLocation.getHeightAboveFloorMeters();
        } catch (IllegalStateException e) {
            exceptionCount++;
        }

        try {
            apiCount++;
            responderLocation.getHeightAboveFloorUncertaintyMeters();
        } catch (IllegalStateException e) {
            exceptionCount++;
        }
        // If Zaxis is not valid, all APIs should throw exception, otherwise no exception.
        assertEquals("Exception number should equal to API number",
                responderLocation.isZaxisSubelementValid() ? 0 : apiCount, exceptionCount);
        // Verify civic location
        if (responderLocation.toCivicLocationAddress() == null) {
            assertNull(responderLocation.toCivicLocationSparseArray());
        } else {
            assertNotNull(responderLocation.toCivicLocationSparseArray());
        }
        // Verify map image
        if (responderLocation.getMapImageUri() == null) {
            assertNull(responderLocation.getMapImageMimeType());
        } else {
            assertNotNull(responderLocation.getMapImageMimeType());
        }
        boolean extraInfoOnAssociationIndication =
                responderLocation.getExtraInfoOnAssociationIndication();
        assertNotNull("ColocatedBSSID list should be nonNull",
                responderLocation.getColocatedBssids());
    }

    /**
     * Verify ranging request with aware peer Mac address and peer handle.
     */
    public void testAwareRttWithMacAddress() throws InterruptedException {
        if (!(shouldTestWifiRtt(getContext()) && shouldTestWifiAware(getContext()))) {
            return;
        }
        RangingRequest request = new RangingRequest.Builder()
                .addWifiAwarePeer(MAC).build();
        ResultCallback callback = new ResultCallback();
        mWifiRttManager.startRanging(request, mExecutor, callback);
        assertTrue("Wi-Fi RTT results: no callback",
                callback.waitForCallback());
        List<RangingResult> rangingResults = callback.getResults();
        assertNotNull("Wi-Fi RTT results: null results", rangingResults);
        assertEquals(1, rangingResults.size());
        assertEquals(RangingResult.STATUS_FAIL, rangingResults.get(0).getStatus());
    }

    /**
     * Verify ranging request with aware peer handle.
     */
    public void testAwareRttWithPeerHandle() throws InterruptedException {
        if (!(shouldTestWifiRtt(getContext()) && shouldTestWifiAware(getContext()))) {
            return;
        }
        PeerHandle peerHandle = mock(PeerHandle.class);
        RangingRequest request = new RangingRequest.Builder()
                .addWifiAwarePeer(peerHandle).build();
        ResultCallback callback = new ResultCallback();
        mWifiRttManager.startRanging(request, mExecutor, callback);
        assertTrue("Wi-Fi RTT results: no callback",
                callback.waitForCallback());
        List<RangingResult> rangingResults = callback.getResults();
        assertNotNull("Wi-Fi RTT results: null results", rangingResults);
        assertEquals("Invalid peerHandle should return 0 result", 0, rangingResults.size());
    }

    /**
     * Test Wi-Fi One-sided RTT ranging operation:
     * - Scan for visible APs for the test AP (which do not support IEEE 802.11mc) and are operating
     * - in the 5GHz band.
     * - Perform N (constant) RTT operations
     * - Remove outliers while insuring greater than 50% of the results still remain
     * - Validate:
     *   - Failure ratio < threshold (constant)
     *   - Result margin < threshold (constant)
     */
    public void testRangingToTestNon11mcAp() throws InterruptedException {
        if (!shouldTestWifiRtt(getContext())
                || !WifiBuildCompat.isPlatformOrWifiModuleAtLeastS(getContext())) {
            return;
        }

        // Scan for Non-IEEE 802.11mc supporting APs
        ScanResult testAp = scanForTestNon11mcCapableAp(NUM_SCANS_SEARCHING_FOR_IEEE80211MC_AP);
        assertNotNull(
                "Cannot find any test APs which are Non-IEEE 802.11mc - please verify that"
                        + " your test setup includes them!", testAp);

        // Perform RTT operations
        RangingRequest.Builder builder = new RangingRequest.Builder();
        builder.addNon80211mcCapableAccessPoint(testAp);
        builder.setRttBurstSize(RangingRequest.getMaxRttBurstSize());
        RangingRequest request = builder.build();

        List<RangingResult> allResults = new ArrayList<>();
        int numFailures = 0;
        int distanceSum = 0;
        int distanceMin = 0;
        int distanceMax = 0;
        int[] statuses = new int[NUM_OF_RTT_ITERATIONS];
        int[] distanceMms = new int[NUM_OF_RTT_ITERATIONS];
        boolean[] distanceInclusionMap = new boolean[NUM_OF_RTT_ITERATIONS];
        int[] distanceStdDevMms = new int[NUM_OF_RTT_ITERATIONS];
        int[] rssis = new int[NUM_OF_RTT_ITERATIONS];
        int[] numAttempted = new int[NUM_OF_RTT_ITERATIONS];
        int[] numSuccessful = new int[NUM_OF_RTT_ITERATIONS];
        long[] timestampsMs = new long[NUM_OF_RTT_ITERATIONS];
        byte[] lastLci = null;
        byte[] lastLcr = null;
        for (int i = 0; i < NUM_OF_RTT_ITERATIONS; ++i) {
            ResultCallback callback = new ResultCallback();
            mWifiRttManager.startRanging(request, mExecutor, callback);
            assertTrue("Wi-Fi RTT results: no callback on iteration " + i,
                    callback.waitForCallback());

            List<RangingResult> currentResults = callback.getResults();
            assertNotNull(
                    "Wi-Fi RTT results: null results (onRangingFailure) on iteration " + i,
                    currentResults);
            assertEquals(
                    "Wi-Fi RTT results: unexpected # of results (expect 1) on iteration " + i,
                    1, currentResults.size());
            RangingResult result = currentResults.get(0);
            assertEquals(
                    "Wi-Fi RTT results: invalid result (wrong BSSID) entry on iteration " + i,
                    result.getMacAddress().toString(), testAp.BSSID);

            assertNull(
                    "Wi-Fi RTT results: invalid result (non-null PeerHandle) entry on iteration "
                            + i, result.getPeerHandle());

            allResults.add(result);
            int status = result.getStatus();
            statuses[i] = status;
            if (status == RangingResult.STATUS_SUCCESS) {
                assertFalse("Wi-Fi RTT results: should not be a 802.11MC measurement",
                        result.is80211mcMeasurement());
                distanceSum += result.getDistanceMm();

                assertTrue("Wi-Fi RTT results: invalid RSSI on iteration " + i,
                        result.getRssi() >= MIN_VALID_RSSI);

                distanceMms[i - numFailures] = result.getDistanceMm();
                distanceStdDevMms[i - numFailures] = result.getDistanceStdDevMm();
                rssis[i - numFailures] = result.getRssi();
                // For one-sided RTT the number of packets attempted in a burst is not available,
                // So we set the result to be the same as used in the request.
                numAttempted[i - numFailures] = request.getRttBurstSize();
                numSuccessful[i - numFailures] = result.getNumSuccessfulMeasurements();
                timestampsMs[i - numFailures] = result.getRangingTimestampMillis();

                byte[] currentLci = result.getLci();
                byte[] currentLcr = result.getLcr();
                if (i - numFailures > 0) {
                    assertTrue("Wi-Fi RTT results: invalid result (LCI mismatch) on iteration " + i,
                            Arrays.equals(currentLci, lastLci));
                    assertTrue("Wi-Fi RTT results: invalid result (LCR mismatch) on iteration " + i,
                            Arrays.equals(currentLcr, lastLcr));
                }
                lastLci = currentLci;
                lastLcr = currentLcr;
            } else {
                numFailures++;
            }
            // Sleep a while to avoid stress AP.
            Thread.sleep(intervalMs);
        }
        // Save results to log
        int numGoodResults = NUM_OF_RTT_ITERATIONS - numFailures;
        DeviceReportLog reportLog = new DeviceReportLog(TAG, "testRangingToTestAp");
        reportLog.addValues("status_codes", statuses, ResultType.NEUTRAL, ResultUnit.NONE);
        reportLog.addValues("distance_mm", Arrays.copyOf(distanceMms, numGoodResults),
                ResultType.NEUTRAL, ResultUnit.NONE);
        reportLog.addValues("distance_stddev_mm",
                Arrays.copyOf(distanceStdDevMms, numGoodResults),
                ResultType.NEUTRAL, ResultUnit.NONE);
        reportLog.addValues("rssi_dbm", Arrays.copyOf(rssis, numGoodResults),
                ResultType.NEUTRAL,
                ResultUnit.NONE);
        reportLog.addValues("num_attempted", Arrays.copyOf(numAttempted, numGoodResults),
                ResultType.NEUTRAL, ResultUnit.NONE);
        reportLog.addValues("num_successful", Arrays.copyOf(numSuccessful, numGoodResults),
                ResultType.NEUTRAL, ResultUnit.NONE);
        reportLog.addValues("timestamps", Arrays.copyOf(timestampsMs, numGoodResults),
                ResultType.NEUTRAL, ResultUnit.NONE);
        reportLog.submit();

        /** TODO(b/192909380): enable the performance verification after device fix.
            // Analyze results
            assertTrue("Wi-Fi RTT failure rate exceeds threshold: FAIL=" + numFailures
                            + ", ITERATIONS="
                            + NUM_OF_RTT_ITERATIONS + ", AP RSSI=" + testAp.level
                            + ", AP SSID=" + testAp.SSID,
                    numFailures <= NUM_OF_RTT_ITERATIONS * MAX_NON11MC_FAILURE_RATE_PERCENT / 100);

            if (numFailures != NUM_OF_RTT_ITERATIONS) {
                // Calculate an initial average using all measurements to determine distance outliers
                double distanceAvg = (double) distanceSum / (NUM_OF_RTT_ITERATIONS - numFailures);
                // Now figure out the distance outliers and mark them in the distance inclusion map
                int validDistances = 0;
                for (int i = 0; i < (NUM_OF_RTT_ITERATIONS - numFailures); i++) {
                    if (distanceMms[i] - MAX_VARIATION_FROM_AVERAGE_DISTANCE_MM < distanceAvg) {
                        // Distances that are in range for the distribution are included in the map
                        distanceInclusionMap[i] = true;
                        validDistances++;
                    } else {
                        // Distances that are out of range for the distribution are excluded in the map
                        distanceInclusionMap[i] = false;
                    }
                }

                assertTrue("After fails+outlier removal greater that 50% distances must remain: " +
                        NUM_OF_RTT_ITERATIONS / 2, validDistances > NUM_OF_RTT_ITERATIONS / 2);

                // Remove the distance outliers and find the new average, min and max.
                distanceSum = 0;
                distanceMax = Integer.MIN_VALUE;
                distanceMin = Integer.MAX_VALUE;
                for (int i = 0; i < (NUM_OF_RTT_ITERATIONS - numFailures); i++) {
                    if (distanceInclusionMap[i]) {
                        distanceSum += distanceMms[i];
                        distanceMin = Math.min(distanceMin, distanceMms[i]);
                        distanceMax = Math.max(distanceMax, distanceMms[i]);
                    }
                }
                distanceAvg = (double) distanceSum / validDistances;
                assertTrue("Wi-Fi RTT: Variation (max direction) exceeds threshold, Variation ="
                                + (distanceMax - distanceAvg),
                        (distanceMax - distanceAvg) <= MAX_VARIATION_FROM_AVERAGE_DISTANCE_MM);
                assertTrue("Wi-Fi RTT: Variation (min direction) exceeds threshold, Variation ="
                                + (distanceAvg - distanceMin),
                        (distanceAvg - distanceMin) <= MAX_VARIATION_FROM_AVERAGE_DISTANCE_MM);
                for (int i = 0; i < numGoodResults; ++i) {
                    assertNotSame("Number of attempted measurements is 0", 0, numAttempted[i]);
                    assertNotSame("Number of successful measurements is 0", 0, numSuccessful[i]);
                }
         */
    }
}
