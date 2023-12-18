/*
 * Copyright (C) 2019 The Android Open Source Project
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

package android.net.wifi;

import static org.junit.Assert.assertEquals;
import static org.junit.Assert.fail;
import static org.mockito.Mockito.validateMockitoUsage;

import android.net.wifi.WifiUsabilityStatsEntry.ContentionTimeStats;
import android.net.wifi.WifiUsabilityStatsEntry.RadioStats;
import android.net.wifi.WifiUsabilityStatsEntry.RateStats;
import android.os.Parcel;

import androidx.test.filters.SmallTest;

import org.junit.After;
import org.junit.Before;
import org.junit.Test;
import org.mockito.MockitoAnnotations;

import java.util.NoSuchElementException;


/**
 * Unit tests for {@link android.net.wifi.WifiUsabilityStatsEntry}.
 */
@SmallTest
public class WifiUsabilityStatsEntryTest {

    /**
     * Setup before tests.
     */
    @Before
    public void setUp() throws Exception {
        MockitoAnnotations.initMocks(this);
    }

    /**
     * Clean up after tests.
     */
    @After
    public void cleanup() {
        validateMockitoUsage();
    }

    /**
     * Verify parcel read/write for Wifi usability stats result.
     */
    @Test
    public void verifyStatsResultWriteAndThenRead() throws Exception {
        WifiUsabilityStatsEntry writeResult = createResult();
        WifiUsabilityStatsEntry readResult = parcelWriteRead(writeResult);
        assertWifiUsabilityStatsEntryEquals(writeResult, readResult);
    }

    /**
     * Verify parcel read/write for Wifi usability stats result.
     */
    @Test
    public void getTimeSliceDutyCycleInPercent() throws Exception {
        ContentionTimeStats[] contentionTimeStats = new ContentionTimeStats[4];
        contentionTimeStats[0] = new ContentionTimeStats(1, 2, 3, 4);
        contentionTimeStats[1] = new ContentionTimeStats(5, 6, 7, 8);
        contentionTimeStats[2] = new ContentionTimeStats(9, 10, 11, 12);
        contentionTimeStats[3] = new ContentionTimeStats(13, 14, 15, 16);
        RateStats[] rateStats = new RateStats[2];
        rateStats[0] = new RateStats(1, 3, 4, 7, 9, 11, 13, 15, 17);
        rateStats[1] = new RateStats(2, 2, 3, 8, 10, 12, 14, 16, 18);

        RadioStats[] radioStats = new RadioStats[2];
        radioStats[0] = new RadioStats(0, 10, 11, 12, 13, 14, 15, 16, 17, 18);
        radioStats[1] = new RadioStats(1, 20, 21, 22, 23, 24, 25, 26, 27, 28);

        WifiUsabilityStatsEntry usabilityStatsEntry = new WifiUsabilityStatsEntry(
                0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20, 21, 22,
                32, contentionTimeStats, rateStats, radioStats, 100, true,
                true, true, 23, 24, 25, true);
        assertEquals(32, usabilityStatsEntry.getTimeSliceDutyCycleInPercent());

        WifiUsabilityStatsEntry usabilityStatsEntryWithInvalidDutyCycleValue =
                new WifiUsabilityStatsEntry(
                        0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20,
                        21, 22, -1, contentionTimeStats, rateStats, radioStats, 101, true, true,
                        true, 23, 24, 25, true);
        try {
            usabilityStatsEntryWithInvalidDutyCycleValue.getTimeSliceDutyCycleInPercent();
            fail();
        } catch (NoSuchElementException e) {
            // pass
        }
    }

    /**
     * Write the provided {@link WifiUsabilityStatsEntry} to a parcel and deserialize it.
     */
    private static WifiUsabilityStatsEntry parcelWriteRead(
            WifiUsabilityStatsEntry writeResult) throws Exception {
        Parcel parcel = Parcel.obtain();
        writeResult.writeToParcel(parcel, 0);
        parcel.setDataPosition(0);    // Rewind data position back to the beginning for read.
        return WifiUsabilityStatsEntry.CREATOR.createFromParcel(parcel);
    }

    private static WifiUsabilityStatsEntry createResult() {
        ContentionTimeStats[] contentionTimeStats = new ContentionTimeStats[4];
        contentionTimeStats[0] = new ContentionTimeStats(1, 2, 3, 4);
        contentionTimeStats[1] = new ContentionTimeStats(5, 6, 7, 8);
        contentionTimeStats[2] = new ContentionTimeStats(9, 10, 11, 12);
        contentionTimeStats[3] = new ContentionTimeStats(13, 14, 15, 16);
        RateStats[] rateStats = new RateStats[2];
        rateStats[0] = new RateStats(1, 3, 4, 7, 9, 11, 13, 15, 17);
        rateStats[1] = new RateStats(2, 2, 3, 8, 10, 12, 14, 16, 18);

        RadioStats[] radioStats = new RadioStats[2];
        radioStats[0] = new RadioStats(0, 10, 11, 12, 13, 14, 15, 16, 17, 18);
        radioStats[1] = new RadioStats(1, 20, 21, 22, 23, 24, 25, 26, 27, 28);

        return new WifiUsabilityStatsEntry(
                0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20, 21, 22,
                50, contentionTimeStats, rateStats, radioStats, 102, true,
                true, true, 23, 24, 25, true
        );
    }

    private static void assertWifiUsabilityStatsEntryEquals(
            WifiUsabilityStatsEntry expected,
            WifiUsabilityStatsEntry actual) {
        assertEquals(expected.getTimeStampMillis(), actual.getTimeStampMillis());
        assertEquals(expected.getRssi(), actual.getRssi());
        assertEquals(expected.getLinkSpeedMbps(), actual.getLinkSpeedMbps());
        assertEquals(expected.getTotalTxSuccess(), actual.getTotalTxSuccess());
        assertEquals(expected.getTotalTxRetries(), actual.getTotalTxRetries());
        assertEquals(expected.getTotalTxBad(), actual.getTotalTxBad());
        assertEquals(expected.getTotalRxSuccess(), actual.getTotalRxSuccess());
        assertEquals(expected.getWifiLinkLayerRadioStats().size(),
                actual.getWifiLinkLayerRadioStats().size());
        for (int i = 0; i < expected.getWifiLinkLayerRadioStats().size(); i++) {
            RadioStats expectedRadioStats = expected.getWifiLinkLayerRadioStats().get(i);
            RadioStats actualRadioStats = actual.getWifiLinkLayerRadioStats().get(i);
            assertEquals(expectedRadioStats.getRadioId(),
                    actualRadioStats.getRadioId());
            assertEquals(expectedRadioStats.getTotalRadioOnTimeMillis(),
                    actualRadioStats.getTotalRadioOnTimeMillis());
            assertEquals(expectedRadioStats.getTotalRadioTxTimeMillis(),
                    actualRadioStats.getTotalRadioTxTimeMillis());
            assertEquals(expectedRadioStats.getTotalRadioRxTimeMillis(),
                    actualRadioStats.getTotalRadioRxTimeMillis());
            assertEquals(expectedRadioStats.getTotalScanTimeMillis(),
                    actualRadioStats.getTotalScanTimeMillis());
            assertEquals(expectedRadioStats.getTotalNanScanTimeMillis(),
                    actualRadioStats.getTotalNanScanTimeMillis());
            assertEquals(expectedRadioStats.getTotalBackgroundScanTimeMillis(),
                    actualRadioStats.getTotalBackgroundScanTimeMillis());
            assertEquals(expectedRadioStats.getTotalRoamScanTimeMillis(),
                    actualRadioStats.getTotalRoamScanTimeMillis());
            assertEquals(expectedRadioStats.getTotalPnoScanTimeMillis(),
                    actualRadioStats.getTotalPnoScanTimeMillis());
            assertEquals(expectedRadioStats.getTotalHotspot2ScanTimeMillis(),
                    actualRadioStats.getTotalHotspot2ScanTimeMillis());
        }
        assertEquals(expected.getTotalRadioOnTimeMillis(), actual.getTotalRadioOnTimeMillis());
        assertEquals(expected.getTotalRadioTxTimeMillis(), actual.getTotalRadioTxTimeMillis());
        assertEquals(expected.getTotalRadioRxTimeMillis(), actual.getTotalRadioRxTimeMillis());
        assertEquals(expected.getTotalScanTimeMillis(), actual.getTotalScanTimeMillis());
        assertEquals(expected.getTotalNanScanTimeMillis(), actual.getTotalNanScanTimeMillis());
        assertEquals(expected.getTotalBackgroundScanTimeMillis(),
                actual.getTotalBackgroundScanTimeMillis());
        assertEquals(expected.getTotalRoamScanTimeMillis(), actual.getTotalRoamScanTimeMillis());
        assertEquals(expected.getTotalPnoScanTimeMillis(), actual.getTotalPnoScanTimeMillis());
        assertEquals(expected.getTotalHotspot2ScanTimeMillis(),
                actual.getTotalHotspot2ScanTimeMillis());
        assertEquals(expected.getTotalCcaBusyFreqTimeMillis(),
                actual.getTotalCcaBusyFreqTimeMillis());
        assertEquals(expected.getTotalRadioOnFreqTimeMillis(),
                actual.getTotalRadioOnFreqTimeMillis());
        assertEquals(expected.getTotalBeaconRx(), actual.getTotalBeaconRx());
        assertEquals(expected.getProbeStatusSinceLastUpdate(),
                actual.getProbeStatusSinceLastUpdate());
        assertEquals(expected.getProbeElapsedTimeSinceLastUpdateMillis(),
                actual.getProbeElapsedTimeSinceLastUpdateMillis());
        assertEquals(expected.getProbeMcsRateSinceLastUpdate(),
                actual.getProbeMcsRateSinceLastUpdate());
        assertEquals(expected.getRxLinkSpeedMbps(), actual.getRxLinkSpeedMbps());
        assertEquals(expected.getTimeSliceDutyCycleInPercent(),
                actual.getTimeSliceDutyCycleInPercent());
        assertEquals(
                expected.getContentionTimeStats(WifiUsabilityStatsEntry.WME_ACCESS_CATEGORY_BE)
                        .getContentionTimeMinMicros(),
                actual.getContentionTimeStats(WifiUsabilityStatsEntry.WME_ACCESS_CATEGORY_BE)
                        .getContentionTimeMinMicros());
        assertEquals(
                expected.getContentionTimeStats(WifiUsabilityStatsEntry.WME_ACCESS_CATEGORY_BE)
                        .getContentionTimeMaxMicros(),
                actual.getContentionTimeStats(WifiUsabilityStatsEntry.WME_ACCESS_CATEGORY_BE)
                        .getContentionTimeMaxMicros());
        assertEquals(
                expected.getContentionTimeStats(WifiUsabilityStatsEntry.WME_ACCESS_CATEGORY_BE)
                        .getContentionTimeAvgMicros(),
                actual.getContentionTimeStats(WifiUsabilityStatsEntry.WME_ACCESS_CATEGORY_BE)
                        .getContentionTimeAvgMicros());
        assertEquals(
                expected.getContentionTimeStats(WifiUsabilityStatsEntry.WME_ACCESS_CATEGORY_BE)
                        .getContentionNumSamples(),
                actual.getContentionTimeStats(WifiUsabilityStatsEntry.WME_ACCESS_CATEGORY_BE)
                        .getContentionNumSamples());
        assertEquals(
                expected.getContentionTimeStats(WifiUsabilityStatsEntry.WME_ACCESS_CATEGORY_BK)
                        .getContentionTimeMinMicros(),
                actual.getContentionTimeStats(WifiUsabilityStatsEntry.WME_ACCESS_CATEGORY_BK)
                        .getContentionTimeMinMicros());
        assertEquals(
                expected.getContentionTimeStats(WifiUsabilityStatsEntry.WME_ACCESS_CATEGORY_BK)
                        .getContentionTimeMaxMicros(),
                actual.getContentionTimeStats(WifiUsabilityStatsEntry.WME_ACCESS_CATEGORY_BK)
                        .getContentionTimeMaxMicros());
        assertEquals(
                expected.getContentionTimeStats(WifiUsabilityStatsEntry.WME_ACCESS_CATEGORY_BK)
                        .getContentionTimeAvgMicros(),
                actual.getContentionTimeStats(WifiUsabilityStatsEntry.WME_ACCESS_CATEGORY_BK)
                        .getContentionTimeAvgMicros());
        assertEquals(
                expected.getContentionTimeStats(WifiUsabilityStatsEntry.WME_ACCESS_CATEGORY_BK)
                        .getContentionNumSamples(),
                actual.getContentionTimeStats(WifiUsabilityStatsEntry.WME_ACCESS_CATEGORY_BK)
                        .getContentionNumSamples());
        assertEquals(
                expected.getContentionTimeStats(WifiUsabilityStatsEntry.WME_ACCESS_CATEGORY_VI)
                        .getContentionTimeMinMicros(),
                actual.getContentionTimeStats(WifiUsabilityStatsEntry.WME_ACCESS_CATEGORY_VI)
                        .getContentionTimeMinMicros());
        assertEquals(
                expected.getContentionTimeStats(WifiUsabilityStatsEntry.WME_ACCESS_CATEGORY_VI)
                        .getContentionTimeMaxMicros(),
                actual.getContentionTimeStats(WifiUsabilityStatsEntry.WME_ACCESS_CATEGORY_VI)
                        .getContentionTimeMaxMicros());
        assertEquals(
                expected.getContentionTimeStats(WifiUsabilityStatsEntry.WME_ACCESS_CATEGORY_VI)
                        .getContentionTimeAvgMicros(),
                actual.getContentionTimeStats(WifiUsabilityStatsEntry.WME_ACCESS_CATEGORY_VI)
                        .getContentionTimeAvgMicros());
        assertEquals(
                expected.getContentionTimeStats(WifiUsabilityStatsEntry.WME_ACCESS_CATEGORY_VI)
                        .getContentionNumSamples(),
                actual.getContentionTimeStats(WifiUsabilityStatsEntry.WME_ACCESS_CATEGORY_VI)
                        .getContentionNumSamples());
        assertEquals(
                expected.getContentionTimeStats(WifiUsabilityStatsEntry.WME_ACCESS_CATEGORY_VO)
                        .getContentionTimeMinMicros(),
                actual.getContentionTimeStats(WifiUsabilityStatsEntry.WME_ACCESS_CATEGORY_VO)
                        .getContentionTimeMinMicros());
        assertEquals(
                expected.getContentionTimeStats(WifiUsabilityStatsEntry.WME_ACCESS_CATEGORY_VO)
                        .getContentionTimeMaxMicros(),
                actual.getContentionTimeStats(WifiUsabilityStatsEntry.WME_ACCESS_CATEGORY_VO)
                        .getContentionTimeMaxMicros());
        assertEquals(
                expected.getContentionTimeStats(WifiUsabilityStatsEntry.WME_ACCESS_CATEGORY_VO)
                        .getContentionTimeAvgMicros(),
                actual.getContentionTimeStats(WifiUsabilityStatsEntry.WME_ACCESS_CATEGORY_VO)
                        .getContentionTimeAvgMicros());
        assertEquals(
                expected.getContentionTimeStats(WifiUsabilityStatsEntry.WME_ACCESS_CATEGORY_VO)
                        .getContentionNumSamples(),
                actual.getContentionTimeStats(WifiUsabilityStatsEntry.WME_ACCESS_CATEGORY_VO)
                        .getContentionNumSamples());
        for (int i = 0; i < expected.getRateStats().size(); i++) {
            RateStats expectedStats = expected.getRateStats().get(i);
            RateStats actualStats = actual.getRateStats().get(i);
            assertEquals(expectedStats.getPreamble(), actualStats.getPreamble());
            assertEquals(expectedStats.getNumberOfSpatialStreams(),
                    actualStats.getNumberOfSpatialStreams());
            assertEquals(expectedStats.getBandwidthInMhz(), actualStats.getBandwidthInMhz());
            assertEquals(expectedStats.getRateMcsIdx(), actualStats.getRateMcsIdx());
            assertEquals(expectedStats.getBitRateInKbps(), actualStats.getBitRateInKbps());
            assertEquals(expectedStats.getTxMpdu(), actualStats.getTxMpdu());
            assertEquals(expectedStats.getRxMpdu(), actualStats.getRxMpdu());
            assertEquals(expectedStats.getMpduLost(), actualStats.getMpduLost());
            assertEquals(expectedStats.getRetries(), actualStats.getRetries());
        }
        assertEquals(expected.getChannelUtilizationRatio(), actual.getChannelUtilizationRatio());
        assertEquals(expected.isThroughputSufficient(), actual.isThroughputSufficient());
        assertEquals(expected.isWifiScoringEnabled(), actual.isWifiScoringEnabled());
        assertEquals(expected.isCellularDataAvailable(), actual.isCellularDataAvailable());
        assertEquals(expected.getCellularDataNetworkType(), actual.getCellularDataNetworkType());
        assertEquals(expected.getCellularSignalStrengthDbm(),
                actual.getCellularSignalStrengthDbm());
        assertEquals(expected.getCellularSignalStrengthDb(), actual.getCellularSignalStrengthDb());
        assertEquals(expected.isSameRegisteredCell(), actual.isSameRegisteredCell());
    }
}
