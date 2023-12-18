/*
 * Copyright 2019 The Android Open Source Project
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

package android.media.tv.tuner.cts;

import static org.junit.Assert.assertEquals;
import static org.junit.Assert.assertFalse;
import static org.junit.Assert.assertNotEquals;
import static org.junit.Assert.assertNotNull;
import static org.junit.Assert.assertTrue;

import android.content.Context;
import android.content.pm.PackageManager;
import android.media.tv.tuner.DemuxCapabilities;
import android.media.tv.tuner.Descrambler;
import android.media.tv.tuner.Lnb;
import android.media.tv.tuner.LnbCallback;
import android.media.tv.tuner.Tuner;
import android.media.tv.tuner.TunerVersionChecker;
import android.media.tv.tuner.dvr.DvrPlayback;
import android.media.tv.tuner.dvr.DvrRecorder;
import android.media.tv.tuner.dvr.OnPlaybackStatusChangedListener;
import android.media.tv.tuner.dvr.OnRecordStatusChangedListener;
import android.media.tv.tuner.filter.AlpFilterConfiguration;
import android.media.tv.tuner.filter.AudioDescriptor;
import android.media.tv.tuner.filter.AvSettings;
import android.media.tv.tuner.filter.DownloadEvent;
import android.media.tv.tuner.filter.DownloadSettings;
import android.media.tv.tuner.filter.Filter;
import android.media.tv.tuner.filter.FilterCallback;
import android.media.tv.tuner.filter.FilterConfiguration;
import android.media.tv.tuner.filter.FilterEvent;
import android.media.tv.tuner.filter.IpCidChangeEvent;
import android.media.tv.tuner.filter.IpFilterConfiguration;
import android.media.tv.tuner.filter.IpPayloadEvent;
import android.media.tv.tuner.filter.MediaEvent;
import android.media.tv.tuner.filter.MmtpFilterConfiguration;
import android.media.tv.tuner.filter.MmtpRecordEvent;
import android.media.tv.tuner.filter.PesEvent;
import android.media.tv.tuner.filter.PesSettings;
import android.media.tv.tuner.filter.RecordSettings;
import android.media.tv.tuner.filter.RestartEvent;
import android.media.tv.tuner.filter.ScramblingStatusEvent;
import android.media.tv.tuner.filter.SectionEvent;
import android.media.tv.tuner.filter.SectionSettingsWithSectionBits;
import android.media.tv.tuner.filter.SectionSettingsWithTableInfo;
import android.media.tv.tuner.filter.Settings;
import android.media.tv.tuner.filter.TemiEvent;
import android.media.tv.tuner.filter.TimeFilter;
import android.media.tv.tuner.filter.TlvFilterConfiguration;
import android.media.tv.tuner.filter.TsFilterConfiguration;
import android.media.tv.tuner.filter.TsRecordEvent;
import android.media.tv.tuner.frontend.AnalogFrontendCapabilities;
import android.media.tv.tuner.frontend.AnalogFrontendSettings;
import android.media.tv.tuner.frontend.Atsc3FrontendCapabilities;
import android.media.tv.tuner.frontend.Atsc3FrontendSettings;
import android.media.tv.tuner.frontend.Atsc3PlpInfo;
import android.media.tv.tuner.frontend.AtscFrontendCapabilities;
import android.media.tv.tuner.frontend.AtscFrontendSettings;
import android.media.tv.tuner.frontend.DtmbFrontendCapabilities;
import android.media.tv.tuner.frontend.DtmbFrontendSettings;
import android.media.tv.tuner.frontend.DvbcFrontendCapabilities;
import android.media.tv.tuner.frontend.DvbcFrontendSettings;
import android.media.tv.tuner.frontend.DvbsFrontendCapabilities;
import android.media.tv.tuner.frontend.DvbsFrontendSettings;
import android.media.tv.tuner.frontend.DvbtFrontendCapabilities;
import android.media.tv.tuner.frontend.DvbtFrontendSettings;
import android.media.tv.tuner.frontend.FrontendCapabilities;
import android.media.tv.tuner.frontend.FrontendInfo;
import android.media.tv.tuner.frontend.FrontendSettings;
import android.media.tv.tuner.frontend.FrontendStatus;
import android.media.tv.tuner.frontend.FrontendStatus.Atsc3PlpTuningInfo;
import android.media.tv.tuner.frontend.Isdbs3FrontendCapabilities;
import android.media.tv.tuner.frontend.Isdbs3FrontendSettings;
import android.media.tv.tuner.frontend.IsdbsFrontendCapabilities;
import android.media.tv.tuner.frontend.IsdbsFrontendSettings;
import android.media.tv.tuner.frontend.IsdbtFrontendCapabilities;
import android.media.tv.tuner.frontend.IsdbtFrontendSettings;
import android.media.tv.tuner.frontend.OnTuneEventListener;
import android.media.tv.tuner.frontend.ScanCallback;

import androidx.test.InstrumentationRegistry;
import androidx.test.filters.SmallTest;
import androidx.test.runner.AndroidJUnit4;

import com.android.compatibility.common.util.RequiredFeatureRule;

import org.junit.After;
import org.junit.Before;
import org.junit.Ignore;
import org.junit.Rule;
import org.junit.Test;
import org.junit.runner.RunWith;

import java.util.List;
import java.util.concurrent.CountDownLatch;
import java.util.concurrent.Executor;
import java.util.concurrent.TimeUnit;

@RunWith(AndroidJUnit4.class)
@SmallTest
public class TunerTest {
    private static final String TAG = "MediaTunerTest";

    @Rule
    public RequiredFeatureRule featureRule = new RequiredFeatureRule(
            PackageManager.FEATURE_TUNER);

    private static final int TIMEOUT_MS = 10000;

    private Context mContext;
    private Tuner mTuner;
    private CountDownLatch mLockLatch = new CountDownLatch(1);

    @Before
    public void setUp() throws Exception {
        mContext = InstrumentationRegistry.getTargetContext();
        InstrumentationRegistry
                .getInstrumentation().getUiAutomation().adoptShellPermissionIdentity();
        mTuner = new Tuner(mContext, null, 100);
    }

    @After
    public void tearDown() {
        if (mTuner != null) {
          mTuner.close();
          mTuner = null;
        }
    }

    @Test
    public void testTunerConstructor() throws Exception {
        assertNotNull(mTuner);
    }

    @Test
    public void testTunerVersion() {
        assertNotNull(mTuner);
        int version = TunerVersionChecker.getTunerVersion();
        assertTrue(version >= TunerVersionChecker.TUNER_VERSION_1_0);
        assertTrue(version <= TunerVersionChecker.TUNER_VERSION_1_1);
    }

    @Test
    public void testTuning() throws Exception {
        List<Integer> ids = mTuner.getFrontendIds();
        assertFalse(ids.isEmpty());

        FrontendInfo info = mTuner.getFrontendInfoById(ids.get(0));
        int res = mTuner.tune(createFrontendSettings(info));
        assertEquals(Tuner.RESULT_SUCCESS, res);
        assertEquals(Tuner.RESULT_SUCCESS, mTuner.setLnaEnabled(false));
        res = mTuner.cancelTuning();
        assertEquals(Tuner.RESULT_SUCCESS, res);
    }

    @Test
    public void testScanning() throws Exception {
        List<Integer> ids = mTuner.getFrontendIds();
        assertFalse(ids.isEmpty());
        for (int id : ids) {
            FrontendInfo info = mTuner.getFrontendInfoById(id);
            if (info != null) {
                mLockLatch = new CountDownLatch(1);
                int res = mTuner.scan(
                        createFrontendSettings(info),
                        Tuner.SCAN_TYPE_AUTO,
                        getExecutor(),
                        getScanCallback());
               assertEquals(Tuner.RESULT_SUCCESS, res);
               assertTrue(mLockLatch.await(TIMEOUT_MS, TimeUnit.MILLISECONDS));
               res = mTuner.cancelScanning();
               assertEquals(Tuner.RESULT_SUCCESS, res);
            }
        }
        mLockLatch = null;
    }

    @Test
    public void testFrontendStatus() throws Exception {
        List<Integer> ids = mTuner.getFrontendIds();
        assertFalse(ids.isEmpty());

        for (int id : ids) {
            if (mTuner == null) {
                mTuner = new Tuner(mContext, null, 100);
            }
            FrontendInfo info = mTuner.getFrontendInfoById(id);
            int res = mTuner.tune(createFrontendSettings(info));

            int[] statusCapabilities = info.getStatusCapabilities();
            assertNotNull(statusCapabilities);
            FrontendStatus status = mTuner.getFrontendStatus(statusCapabilities);
            assertNotNull(status);

            for (int i = 0; i < statusCapabilities.length; i++) {
                switch (statusCapabilities[i]) {
                    case FrontendStatus.FRONTEND_STATUS_TYPE_DEMOD_LOCK:
                        status.isDemodLocked();
                        break;
                    case FrontendStatus.FRONTEND_STATUS_TYPE_SNR:
                        status.getSnr();
                        break;
                    case FrontendStatus.FRONTEND_STATUS_TYPE_BER:
                        status.getBer();
                        break;
                    case FrontendStatus.FRONTEND_STATUS_TYPE_PER:
                        status.getPer();
                        break;
                    case FrontendStatus.FRONTEND_STATUS_TYPE_PRE_BER:
                        status.getPerBer();
                        break;
                    case FrontendStatus.FRONTEND_STATUS_TYPE_SIGNAL_QUALITY:
                        status.getSignalQuality();
                        break;
                    case FrontendStatus.FRONTEND_STATUS_TYPE_SIGNAL_STRENGTH:
                        status.getSignalStrength();
                        break;
                    case FrontendStatus.FRONTEND_STATUS_TYPE_SYMBOL_RATE:
                        status.getSymbolRate();
                        break;
                    case FrontendStatus.FRONTEND_STATUS_TYPE_FEC:
                        status.getInnerFec();
                        break;
                    case FrontendStatus.FRONTEND_STATUS_TYPE_MODULATION:
                        status.getModulation();
                        break;
                    case FrontendStatus.FRONTEND_STATUS_TYPE_SPECTRAL:
                        status.getSpectralInversion();
                        break;
                    case FrontendStatus.FRONTEND_STATUS_TYPE_LNB_VOLTAGE:
                        status.getLnbVoltage();
                        break;
                    case FrontendStatus.FRONTEND_STATUS_TYPE_PLP_ID:
                        status.getPlpId();
                        break;
                    case FrontendStatus.FRONTEND_STATUS_TYPE_EWBS:
                        status.isEwbs();
                        break;
                    case FrontendStatus.FRONTEND_STATUS_TYPE_AGC:
                        status.getAgc();
                        break;
                    case FrontendStatus.FRONTEND_STATUS_TYPE_LNA:
                        status.isLnaOn();
                        break;
                    case FrontendStatus.FRONTEND_STATUS_TYPE_LAYER_ERROR:
                        status.getLayerErrors();
                        break;
                    case FrontendStatus.FRONTEND_STATUS_TYPE_MER:
                        status.getMer();
                        break;
                    case FrontendStatus.FRONTEND_STATUS_TYPE_FREQ_OFFSET:
                        status.getFreqOffset();
                        break;
                    case FrontendStatus.FRONTEND_STATUS_TYPE_HIERARCHY:
                        status.getHierarchy();
                        break;
                    case FrontendStatus.FRONTEND_STATUS_TYPE_RF_LOCK:
                        status.isRfLocked();
                        break;
                    case FrontendStatus.FRONTEND_STATUS_TYPE_ATSC3_PLP_INFO:
                        Atsc3PlpTuningInfo[] tuningInfos = status.getAtsc3PlpTuningInfo();
                        if (tuningInfos != null) {
                            for (Atsc3PlpTuningInfo tuningInfo : tuningInfos) {
                                tuningInfo.getPlpId();
                                tuningInfo.isLocked();
                                tuningInfo.getUec();
                            }
                        }
                        break;
                    case FrontendStatus.FRONTEND_STATUS_TYPE_BERS:
                        status.getBers();
                        break;
                    case FrontendStatus.FRONTEND_STATUS_TYPE_CODERATES:
                        status.getCodeRates();
                        break;
                    case FrontendStatus.FRONTEND_STATUS_TYPE_BANDWIDTH:
                        status.getBandwidth();
                        break;
                    case FrontendStatus.FRONTEND_STATUS_TYPE_GUARD_INTERVAL:
                        status.getGuardInterval();
                        break;
                    case FrontendStatus.FRONTEND_STATUS_TYPE_TRANSMISSION_MODE:
                        status.getTransmissionMode();
                        break;
                    case FrontendStatus.FRONTEND_STATUS_TYPE_UEC:
                        status.getUec();
                        break;
                    case FrontendStatus.FRONTEND_STATUS_TYPE_T2_SYSTEM_ID:
                        status.getSystemId();
                        break;
                    case FrontendStatus.FRONTEND_STATUS_TYPE_INTERLEAVINGS:
                        status.getInterleaving();
                        break;
                    case FrontendStatus.FRONTEND_STATUS_TYPE_ISDBT_SEGMENTS:
                        status.getIsdbtSegment();
                        break;
                    case FrontendStatus.FRONTEND_STATUS_TYPE_TS_DATA_RATES:
                        status.getTsDataRate();
                        break;
                    case FrontendStatus.FRONTEND_STATUS_TYPE_MODULATIONS_EXT:
                        status.getExtendedModulations();
                        break;
                    case FrontendStatus.FRONTEND_STATUS_TYPE_ROLL_OFF:
                        status.getRollOff();
                        break;
                    case FrontendStatus.FRONTEND_STATUS_TYPE_IS_MISO_ENABLED:
                        status.isMisoEnabled();
                        break;
                    case FrontendStatus.FRONTEND_STATUS_TYPE_IS_LINEAR:
                        status.isLinear();
                        break;
                    case FrontendStatus.FRONTEND_STATUS_TYPE_IS_SHORT_FRAMES_ENABLED:
                        status.isShortFramesEnabled();
                        break;
                }
            }
            mTuner.close();
            mTuner = null;
        }
    }

    @Test
    public void testLnb() throws Exception {
        Lnb lnb = mTuner.openLnb(getExecutor(), getLnbCallback());
        if (lnb == null) return;
        assertEquals(lnb.setVoltage(Lnb.VOLTAGE_5V), Tuner.RESULT_SUCCESS);
        assertEquals(lnb.setTone(Lnb.TONE_NONE), Tuner.RESULT_SUCCESS);
        assertEquals(
                lnb.setSatellitePosition(Lnb.POSITION_A), Tuner.RESULT_SUCCESS);
        lnb.sendDiseqcMessage(new byte[] {1, 2});
        lnb.close();
    }

    @Test
    public void testOpenLnbByname() throws Exception {
        Lnb lnb = mTuner.openLnbByName("default", getExecutor(), getLnbCallback());
        if (lnb != null) {
            lnb.close();
        }
    }

    @Test
    public void testCiCam() throws Exception {
    // open filter to get demux resource
        mTuner.openFilter(
                Filter.TYPE_TS, Filter.SUBTYPE_SECTION, 1000, getExecutor(), getFilterCallback());

        mTuner.connectCiCam(1);
        mTuner.disconnectCiCam();
    }

    @Test
    public void testFrontendToCiCam() throws Exception {
        // tune to get frontend resource
        List<Integer> ids = mTuner.getFrontendIds();
        assertFalse(ids.isEmpty());
        FrontendInfo info = mTuner.getFrontendInfoById(ids.get(0));
        int res = mTuner.tune(createFrontendSettings(info));
        assertEquals(Tuner.RESULT_SUCCESS, res);

        if (TunerVersionChecker.isHigherOrEqualVersionTo(TunerVersionChecker.TUNER_VERSION_1_1)) {
            // TODO: get real CiCam id from MediaCas
            res = mTuner.connectFrontendToCiCam(0);
        } else {
            assertEquals(Tuner.INVALID_LTS_ID, mTuner.connectFrontendToCiCam(0));
        }

        if (res != Tuner.INVALID_LTS_ID) {
            assertEquals(mTuner.disconnectFrontendToCiCam(0), Tuner.RESULT_SUCCESS);
        } else {
            // Make sure the connectFrontendToCiCam only fails because the current device
            // does not support connecting frontend to cicam
            assertEquals(mTuner.disconnectFrontendToCiCam(0), Tuner.RESULT_UNAVAILABLE);
        }
    }

    @Test
    public void testAvSyncId() throws Exception {
    // open filter to get demux resource
        Filter f = mTuner.openFilter(
                Filter.TYPE_TS, Filter.SUBTYPE_AUDIO, 1000, getExecutor(), getFilterCallback());
        Settings settings = AvSettings
                .builder(Filter.TYPE_TS, true)
                .setPassthrough(false)
                .setAudioStreamType(AvSettings.AUDIO_STREAM_TYPE_MPEG1)
                .build();
        FilterConfiguration config = TsFilterConfiguration
                .builder()
                .setTpid(10)
                .setSettings(settings)
                .build();
        f.configure(config);
        int id = mTuner.getAvSyncHwId(f);
        if (id != Tuner.INVALID_AV_SYNC_ID) {
            assertNotEquals(Tuner.INVALID_TIMESTAMP, mTuner.getAvSyncTime(id));
        }
    }

    @Test
    public void testReadFilter() throws Exception {
        Filter f = mTuner.openFilter(
                Filter.TYPE_TS, Filter.SUBTYPE_SECTION, 1000, getExecutor(), getFilterCallback());
        assertNotNull(f);
        assertNotEquals(Tuner.INVALID_FILTER_ID, f.getId());
        if (TunerVersionChecker.isHigherOrEqualVersionTo(TunerVersionChecker.TUNER_VERSION_1_1)) {
            assertNotEquals(Tuner.INVALID_FILTER_ID_LONG, f.getIdLong());
        } else {
            assertEquals(Tuner.INVALID_FILTER_ID_LONG, f.getIdLong());
        }

        Settings settings = SectionSettingsWithTableInfo
                .builder(Filter.TYPE_TS)
                .setTableId(2)
                .setVersion(1)
                .setCrcEnabled(true)
                .setRaw(false)
                .setRepeat(false)
                .build();
        FilterConfiguration config = TsFilterConfiguration
                .builder()
                .setTpid(10)
                .setSettings(settings)
                .build();
        f.configure(config);
        f.setMonitorEventMask(
                Filter.MONITOR_EVENT_SCRAMBLING_STATUS | Filter.MONITOR_EVENT_IP_CID_CHANGE);

        // Tune a frontend before start the filter
        List<Integer> ids = mTuner.getFrontendIds();
        assertFalse(ids.isEmpty());

        FrontendInfo info = mTuner.getFrontendInfoById(ids.get(0));
        int res = mTuner.tune(createFrontendSettings(info));
        assertEquals(Tuner.RESULT_SUCCESS, res);

        f.start();
        f.flush();
        f.read(new byte[3], 0, 3);
        f.stop();
        f.close();

        res = mTuner.cancelTuning();
        assertEquals(Tuner.RESULT_SUCCESS, res);
    }

    @Test
    public void testAudioFilterStreamTypeConfig() throws Exception {
        Filter f = mTuner.openFilter(
                Filter.TYPE_TS, Filter.SUBTYPE_AUDIO, 1000, getExecutor(), getFilterCallback());
        assertNotNull(f);
        assertNotEquals(Tuner.INVALID_FILTER_ID, f.getId());

        Settings settings = AvSettings
                .builder(Filter.TYPE_TS, true)
                .setPassthrough(false)
                .setAudioStreamType(AvSettings.AUDIO_STREAM_TYPE_MPEG1)
                .build();
        FilterConfiguration config = TsFilterConfiguration
                .builder()
                .setTpid(10)
                .setSettings(settings)
                .build();
        f.configure(config);

        // Tune a frontend before start the filter
        List<Integer> ids = mTuner.getFrontendIds();
        assertFalse(ids.isEmpty());

        FrontendInfo info = mTuner.getFrontendInfoById(ids.get(0));
        int res = mTuner.tune(createFrontendSettings(info));
        assertEquals(Tuner.RESULT_SUCCESS, res);

        f.start();
        f.flush();
        f.stop();
        f.close();

        res = mTuner.cancelTuning();
        assertEquals(Tuner.RESULT_SUCCESS, res);
    }

    @Test
    public void testTimeFilter() throws Exception {
        if (!mTuner.getDemuxCapabilities().isTimeFilterSupported()) return;
        TimeFilter f = mTuner.openTimeFilter();
        assertNotNull(f);
        f.setCurrentTimestamp(0);
        assertNotEquals(Tuner.INVALID_TIMESTAMP, f.getTimeStamp());
        assertNotEquals(Tuner.INVALID_TIMESTAMP, f.getSourceTime());
        f.clearTimestamp();
        f.close();
    }

    @Test
    public void testIpFilter() throws Exception {
        Filter f = mTuner.openFilter(
                Filter.TYPE_IP, Filter.SUBTYPE_IP, 1000, getExecutor(), getFilterCallback());
        assertNotNull(f);
        assertNotEquals(Tuner.INVALID_FILTER_ID, f.getId());

        FilterConfiguration config = IpFilterConfiguration
                .builder()
                .setSrcIpAddress(new byte[] {(byte) 0xC0, (byte) 0xA8, 0, 1})
                .setDstIpAddress(new byte[] {(byte) 0xC0, (byte) 0xA8, 3, 4})
                .setSrcPort(33)
                .setDstPort(23)
                .setPassthrough(false)
                .setSettings(null)
                .setIpFilterContextId(1)
                .build();
        f.configure(config);

        // Tune a frontend before start the filter
        List<Integer> ids = mTuner.getFrontendIds();
        assertFalse(ids.isEmpty());

        FrontendInfo info = mTuner.getFrontendInfoById(ids.get(0));
        int res = mTuner.tune(createFrontendSettings(info));
        assertEquals(Tuner.RESULT_SUCCESS, res);

        f.start();
        f.stop();
        f.close();

        res = mTuner.cancelTuning();
        assertEquals(Tuner.RESULT_SUCCESS, res);
    }

    @Test
    public void testAlpSectionFilterConfig() throws Exception {
        Filter f = mTuner.openFilter(
                Filter.TYPE_ALP, Filter.SUBTYPE_SECTION, 1000, getExecutor(), getFilterCallback());
        assertNotNull(f);
        assertNotEquals(Tuner.INVALID_FILTER_ID, f.getId());

        SectionSettingsWithSectionBits settings =
                SectionSettingsWithSectionBits
                        .builder(Filter.TYPE_TS)
                        .setCrcEnabled(true)
                        .setRepeat(false)
                        .setRaw(false)
                        .setFilter(new byte[]{2, 3, 4})
                        .setMask(new byte[]{7, 6, 5, 4})
                        .setMode(new byte[]{22, 55, 33})
                        .build();
        AlpFilterConfiguration config =
                AlpFilterConfiguration
                        .builder()
                        .setPacketType(AlpFilterConfiguration.PACKET_TYPE_COMPRESSED)
                        .setLengthType(AlpFilterConfiguration.LENGTH_TYPE_WITH_ADDITIONAL_HEADER)
                        .setSettings(settings)
                        .build();
        f.configure(config);
        f.start();
        f.stop();
        f.close();
    }

    @Test
    public void testMmtpPesFilterConfig() throws Exception {
        Filter f = mTuner.openFilter(
                Filter.TYPE_MMTP, Filter.SUBTYPE_PES, 1000, getExecutor(), getFilterCallback());
        assertNotNull(f);
        assertNotEquals(Tuner.INVALID_FILTER_ID, f.getId());

        PesSettings settings =
                PesSettings
                        .builder(Filter.TYPE_TS)
                        .setStreamId(3)
                        .setRaw(false)
                        .build();
        MmtpFilterConfiguration config =
                MmtpFilterConfiguration
                        .builder()
                        .setMmtpPacketId(3)
                        .setSettings(settings)
                        .build();
        f.configure(config);
        f.start();
        f.stop();
        f.close();
    }

    @Test
    public void testMmtpDownloadFilterConfig() throws Exception {
        Filter f = mTuner.openFilter(
                Filter.TYPE_MMTP, Filter.SUBTYPE_DOWNLOAD,
                1000, getExecutor(), getFilterCallback());
        assertNotNull(f);
        assertNotEquals(Tuner.INVALID_FILTER_ID, f.getId());

        DownloadSettings settings =
                DownloadSettings
                        .builder(Filter.TYPE_MMTP)
                        .setDownloadId(2)
                        .build();
        MmtpFilterConfiguration config =
                MmtpFilterConfiguration
                        .builder()
                        .setMmtpPacketId(3)
                        .setSettings(settings)
                        .build();
        f.configure(config);
        f.start();
        f.stop();
        f.close();
    }

    @Test
    public void testTsAvFilterConfig() throws Exception {
        Filter f = mTuner.openFilter(
                Filter.TYPE_TS, Filter.SUBTYPE_AUDIO, 1000, getExecutor(), getFilterCallback());
        assertNotNull(f);
        assertNotEquals(Tuner.INVALID_FILTER_ID, f.getId());

        AvSettings settings =
                AvSettings
                        .builder(Filter.TYPE_TS, true) // is Audio
                        .setPassthrough(false)
                        .setAudioStreamType(AvSettings.AUDIO_STREAM_TYPE_MPEG1)
                        .build();
        TsFilterConfiguration config =
                TsFilterConfiguration
                        .builder()
                        .setTpid(521)
                        .setSettings(settings)
                        .build();
        f.configure(config);
        f.start();
        f.stop();
        f.close();
    }

    @Test
    public void testTsRecordFilterConfig() throws Exception {
        Filter f = mTuner.openFilter(
                Filter.TYPE_TS, Filter.SUBTYPE_RECORD, 1000, getExecutor(), getFilterCallback());
        assertNotNull(f);
        assertNotEquals(Tuner.INVALID_FILTER_ID, f.getId());

        RecordSettings settings =
                RecordSettings
                        .builder(Filter.TYPE_TS)
                        .setTsIndexMask(
                                RecordSettings.TS_INDEX_FIRST_PACKET
                                        | RecordSettings.TS_INDEX_PRIVATE_DATA)
                        .setScIndexType(RecordSettings.INDEX_TYPE_SC)
                        .setScIndexMask(RecordSettings.SC_INDEX_B_SLICE)
                        .build();
        TsFilterConfiguration config =
                TsFilterConfiguration
                        .builder()
                        .setTpid(521)
                        .setSettings(settings)
                        .build();
        f.configure(config);
        f.start();
        f.stop();
        f.close();
    }

    @Test
    public void testTlvTlvFilterConfig() throws Exception {
        Filter f = mTuner.openFilter(
                Filter.TYPE_TLV, Filter.SUBTYPE_TLV, 1000, getExecutor(), getFilterCallback());
        assertNotNull(f);
        assertNotEquals(Tuner.INVALID_FILTER_ID, f.getId());

        TlvFilterConfiguration config =
                TlvFilterConfiguration
                        .builder()
                        .setPacketType(TlvFilterConfiguration.PACKET_TYPE_IPV4)
                        .setCompressedIpPacket(true)
                        .setPassthrough(false)
                        .setSettings(null)
                        .build();
        f.configure(config);
        f.start();
        f.stop();
        f.close();
    }

    @Test
    public void testDescrambler() throws Exception {
        Descrambler d = mTuner.openDescrambler();
        byte[] keyToken = new byte[] {1, 3, 2};
        assertNotNull(d);
        Filter f = mTuner.openFilter(
                Filter.TYPE_TS, Filter.SUBTYPE_SECTION, 1000, getExecutor(), getFilterCallback());
        assertTrue(d.isValidKeyToken(keyToken));
        d.setKeyToken(keyToken);
        d.addPid(Descrambler.PID_TYPE_T, 1, f);
        d.removePid(Descrambler.PID_TYPE_T, 1, f);
        f.close();
        d.close();
    }

    @Test
    public void testDescramblerKeyTokenValidator() throws Exception {
        byte[] invalidToken = new byte[17];
        byte[] validToken = new byte[] {1, 3, 2};
        assertTrue(Descrambler.isValidKeyToken(validToken));
        assertTrue(Descrambler.isValidKeyToken(Tuner.VOID_KEYTOKEN));
        assertFalse(Descrambler.isValidKeyToken(invalidToken));
    }

    @Test
    public void testOpenDvrRecorder() throws Exception {
        DvrRecorder d = mTuner.openDvrRecorder(100, getExecutor(), getRecordListener());
        assertNotNull(d);
        d.close();
    }

    @Test
    public void testOpenDvPlayback() throws Exception {
        DvrPlayback d = mTuner.openDvrPlayback(100, getExecutor(), getPlaybackListener());
        assertNotNull(d);
        d.close();
    }

    @Test
    public void testDemuxCapabilities() throws Exception {
        DemuxCapabilities d = mTuner.getDemuxCapabilities();
        assertNotNull(d);

        d.getDemuxCount();
        d.getRecordCount();
        d.getPlaybackCount();
        d.getTsFilterCount();
        d.getSectionFilterCount();
        d.getAudioFilterCount();
        d.getVideoFilterCount();
        d.getPesFilterCount();
        d.getPcrFilterCount();
        d.getSectionFilterLength();
        d.getFilterCapabilities();
        d.getLinkCapabilities();
        d.isTimeFilterSupported();
    }

    @Test
    public void testResourceLostListener() throws Exception {
        mTuner.setResourceLostListener(getExecutor(), new Tuner.OnResourceLostListener() {
            @Override
            public void onResourceLost(Tuner tuner) {
            }
        });
        mTuner.clearResourceLostListener();
    }

    @Test
    public void testOnTuneEventListener() throws Exception {
        mTuner.setOnTuneEventListener(getExecutor(), new OnTuneEventListener() {
            @Override
            public void onTuneEvent(int tuneEvent) {
            }
        });
        mTuner.clearOnTuneEventListener();
    }

    @Test
    public void testUpdateResourcePriority() throws Exception {
        mTuner.updateResourcePriority(100, 20);
    }

    @Test
    public void testShareFrontendFromTuner() throws Exception {
        Tuner other = new Tuner(mContext, null, 100);
        List<Integer> ids = other.getFrontendIds();
        assertFalse(ids.isEmpty());
        FrontendInfo info = other.getFrontendInfoById(ids.get(0));

        // call tune() to open frontend resource
        int res = other.tune(createFrontendSettings(info));
        assertEquals(Tuner.RESULT_SUCCESS, res);
        assertNotNull(other.getFrontendInfo());
        mTuner.shareFrontendFromTuner(other);
        other.close();
    }

    private boolean hasTuner() {
        return mContext.getPackageManager().hasSystemFeature("android.hardware.tv.tuner");
    }

    private Executor getExecutor() {
        return Runnable::run;
    }

    private LnbCallback getLnbCallback() {
        return new LnbCallback() {
            @Override
            public void onEvent(int lnbEventType) {}
            @Override
            public void onDiseqcMessage(byte[] diseqcMessage) {}
        };
    }

    private FilterCallback getFilterCallback() {
        return new FilterCallback() {
            @Override
            public void onFilterEvent(Filter filter, FilterEvent[] events) {
                for (FilterEvent e : events) {
                    if (e instanceof DownloadEvent) {
                        testDownloadEvent(filter, (DownloadEvent) e);
                    } else if (e instanceof IpPayloadEvent) {
                        testIpPayloadEvent(filter, (IpPayloadEvent) e);
                    } else if (e instanceof MediaEvent) {
                        testMediaEvent(filter, (MediaEvent) e);
                    } else if (e instanceof MmtpRecordEvent) {
                        testMmtpRecordEvent(filter, (MmtpRecordEvent) e);
                    } else if (e instanceof PesEvent) {
                        testPesEvent(filter, (PesEvent) e);
                    } else if (e instanceof SectionEvent) {
                        testSectionEvent(filter, (SectionEvent) e);
                    } else if (e instanceof TemiEvent) {
                        testTemiEvent(filter, (TemiEvent) e);
                    } else if (e instanceof TsRecordEvent) {
                        testTsRecordEvent(filter, (TsRecordEvent) e);
                    } else if (e instanceof ScramblingStatusEvent) {
                        testScramblingStatusEvent(filter, (ScramblingStatusEvent) e);
                    } else if (e instanceof IpCidChangeEvent) {
                        testIpCidChangeEvent(filter, (IpCidChangeEvent) e);
                    } else if (e instanceof RestartEvent) {
                        testRestartEvent(filter, (RestartEvent) e);
                    }
                }
            }
            @Override
            public void onFilterStatusChanged(Filter filter, int status) {}
        };
    }

    private void testDownloadEvent(Filter filter, DownloadEvent e) {
        e.getItemId();
        e.getMpuSequenceNumber();
        e.getItemFragmentIndex();
        e.getLastItemFragmentIndex();
        long length = e.getDataLength();
        if (length > 0) {
            byte[] buffer = new byte[(int) length];
            assertNotEquals(0, filter.read(buffer, 0, length));
        }
    }

    private void testIpPayloadEvent(Filter filter, IpPayloadEvent e) {
        long length = e.getDataLength();
        if (length > 0) {
            byte[] buffer = new byte[(int) length];
            assertNotEquals(0, filter.read(buffer, 0, length));
        }
    }

    private void testMediaEvent(Filter filter, MediaEvent e) {
        e.getStreamId();
        e.isPtsPresent();
        e.getPts();
        e.getDataLength();
        e.getOffset();
        e.getLinearBlock();
        e.isSecureMemory();
        e.getAvDataId();
        e.getAudioHandle();
        e.getMpuSequenceNumber();
        e.isPrivateData();
        AudioDescriptor ad = e.getExtraMetaData();
        if (ad != null) {
            ad.getAdFade();
            ad.getAdPan();
            ad.getAdVersionTextTag();
            ad.getAdGainCenter();
            ad.getAdGainFront();
            ad.getAdGainSurround();
        }
        e.release();
    }

    private void testMmtpRecordEvent(Filter filter, MmtpRecordEvent e) {
        e.getScHevcIndexMask();
        e.getDataLength();
        int mpuSequenceNumber = e.getMpuSequenceNumber();
        long pts = e.getPts();
        int firstMbInSlice = e.getFirstMacroblockInSlice();
        int tsIndexMask = e.getTsIndexMask();
        if (!TunerVersionChecker.isHigherOrEqualVersionTo(TunerVersionChecker.TUNER_VERSION_1_1)) {
            assertEquals(mpuSequenceNumber, Tuner.INVALID_MMTP_RECORD_EVENT_MPT_SEQUENCE_NUM);
            assertEquals(pts, Tuner.INVALID_TIMESTAMP);
            assertEquals(firstMbInSlice, Tuner.INVALID_FIRST_MACROBLOCK_IN_SLICE);
            assertEquals(tsIndexMask, 0);
        }
    }

    private void testPesEvent(Filter filter, PesEvent e) {
        e.getStreamId();
        e.getMpuSequenceNumber();
        long length = e.getDataLength();
        if (length > 0) {
            byte[] buffer = new byte[(int) length];
            assertNotEquals(0, filter.read(buffer, 0, length));
        }
    }

    private void testSectionEvent(Filter filter, SectionEvent e) {
        e.getTableId();
        e.getVersion();
        e.getSectionNumber();
        long length = e.getDataLength();
        if (length > 0) {
            byte[] buffer = new byte[(int) length];
            assertNotEquals(0, filter.read(buffer, 0, length));
        }
    }

    private void testTemiEvent(Filter filter, TemiEvent e) {
        e.getPts();
        e.getDescriptorTag();
        e.getDescriptorData();
    }

    private void testTsRecordEvent(Filter filter, TsRecordEvent e) {
        e.getPacketId();
        e.getTsIndexMask();
        e.getScIndexMask();
        e.getDataLength();
        long pts = e.getPts();
        int firstMbInSlice = e.getFirstMacroblockInSlice();
        if (!TunerVersionChecker.isHigherOrEqualVersionTo(TunerVersionChecker.TUNER_VERSION_1_1)) {
            assertEquals(pts, Tuner.INVALID_TIMESTAMP);
            assertEquals(firstMbInSlice, Tuner.INVALID_FIRST_MACROBLOCK_IN_SLICE);
        }
    }

    private void testScramblingStatusEvent(Filter filter, ScramblingStatusEvent e) {
        e.getScramblingStatus();
    }

    private void testIpCidChangeEvent(Filter filter, IpCidChangeEvent e) {
        e.getIpCid();
    }

    private void testRestartEvent(Filter filter, RestartEvent e) {
        e.getStartId();
    }

    private OnRecordStatusChangedListener getRecordListener() {
        return new OnRecordStatusChangedListener() {
            @Override
            public void onRecordStatusChanged(int status) {}
        };
    }

    private OnPlaybackStatusChangedListener getPlaybackListener() {
        return new OnPlaybackStatusChangedListener() {
            @Override
            public void onPlaybackStatusChanged(int status) {}
        };
    }

    private FrontendSettings createFrontendSettings(FrontendInfo info) {
            FrontendCapabilities caps = info.getFrontendCapabilities();
            int minFreq = info.getFrequencyRange().getLower();
            int maxFreq = info.getFrequencyRange().getUpper();
            FrontendCapabilities feCaps = info.getFrontendCapabilities();
            switch(info.getType()) {
                case FrontendSettings.TYPE_ANALOG: {
                    AnalogFrontendCapabilities analogCaps = (AnalogFrontendCapabilities) caps;
                    int signalType = getFirstCapable(analogCaps.getSignalTypeCapability());
                    int sif = getFirstCapable(analogCaps.getSifStandardCapability());
                    return AnalogFrontendSettings
                            .builder()
                            .setFrequency(minFreq)
                            .setSignalType(signalType)
                            .setSifStandard(sif)
                            .build();
                }
                case FrontendSettings.TYPE_ATSC3: {
                    Atsc3FrontendCapabilities atsc3Caps = (Atsc3FrontendCapabilities) caps;
                    int bandwidth = getFirstCapable(atsc3Caps.getBandwidthCapability());
                    int demod = getFirstCapable(atsc3Caps.getDemodOutputFormatCapability());
                    Atsc3FrontendSettings settings =
                            Atsc3FrontendSettings
                                    .builder()
                                    .setFrequency(minFreq)
                                    .setBandwidth(bandwidth)
                                    .setDemodOutputFormat(demod)
                                    .build();
                    settings.setEndFrequency(maxFreq);
                    return settings;
                }
                case FrontendSettings.TYPE_ATSC: {
                    AtscFrontendCapabilities atscCaps = (AtscFrontendCapabilities) caps;
                    int modulation = getFirstCapable(atscCaps.getModulationCapability());
                    return AtscFrontendSettings
                            .builder()
                            .setFrequency(minFreq)
                            .setModulation(modulation)
                            .build();
                }
                case FrontendSettings.TYPE_DVBC: {
                    DvbcFrontendCapabilities dvbcCaps = (DvbcFrontendCapabilities) caps;
                    int modulation = getFirstCapable(dvbcCaps.getModulationCapability());
                    int fec = getFirstCapable(dvbcCaps.getFecCapability());
                    int annex = getFirstCapable(dvbcCaps.getAnnexCapability());
                    DvbcFrontendSettings settings =
                            DvbcFrontendSettings
                                    .builder()
                                    .setFrequency(minFreq)
                                    .setModulation(modulation)
                                    .setInnerFec(fec)
                                    .setAnnex(annex)
                                    .build();
                    settings.setEndFrequency(maxFreq);
                    return settings;
                }
                case FrontendSettings.TYPE_DVBS: {
                    DvbsFrontendCapabilities dvbsCaps = (DvbsFrontendCapabilities) caps;
                    int modulation = getFirstCapable(dvbsCaps.getModulationCapability());
                    int standard = getFirstCapable(dvbsCaps.getStandardCapability());
                    DvbsFrontendSettings settings =
                            DvbsFrontendSettings
                                    .builder()
                                    .setFrequency(minFreq)
                                    .setModulation(modulation)
                                    .setStandard(standard)
                                    .build();
                    settings.setEndFrequency(maxFreq);
                    return settings;
                }
                case FrontendSettings.TYPE_DVBT: {
                    DvbtFrontendCapabilities dvbtCaps = (DvbtFrontendCapabilities) caps;
                    int transmission = getFirstCapable(dvbtCaps.getTransmissionModeCapability());
                    int bandwidth = getFirstCapable(dvbtCaps.getBandwidthCapability());
                    int constellation = getFirstCapable(dvbtCaps.getConstellationCapability());
                    int codeRate = getFirstCapable(dvbtCaps.getCodeRateCapability());
                    int hierarchy = getFirstCapable(dvbtCaps.getHierarchyCapability());
                    int guardInterval = getFirstCapable(dvbtCaps.getGuardIntervalCapability());
                    return DvbtFrontendSettings
                            .builder()
                            .setFrequency(minFreq)
                            .setTransmissionMode(transmission)
                            .setBandwidth(bandwidth)
                            .setConstellation(constellation)
                            .setHierarchy(hierarchy)
                            .setHighPriorityCodeRate(codeRate)
                            .setLowPriorityCodeRate(codeRate)
                            .setGuardInterval(guardInterval)
                            .setStandard(DvbtFrontendSettings.STANDARD_T)
                            .setMiso(false)
                            .build();
                }
                case FrontendSettings.TYPE_ISDBS3: {
                    Isdbs3FrontendCapabilities isdbs3Caps = (Isdbs3FrontendCapabilities) caps;
                    int modulation = getFirstCapable(isdbs3Caps.getModulationCapability());
                    int codeRate = getFirstCapable(isdbs3Caps.getCodeRateCapability());
                    return Isdbs3FrontendSettings
                            .builder()
                            .setFrequency(minFreq)
                            .setModulation(modulation)
                            .setCodeRate(codeRate)
                            .build();
                }
                case FrontendSettings.TYPE_ISDBS: {
                    IsdbsFrontendCapabilities isdbsCaps = (IsdbsFrontendCapabilities) caps;
                    int modulation = getFirstCapable(isdbsCaps.getModulationCapability());
                    int codeRate = getFirstCapable(isdbsCaps.getCodeRateCapability());
                    return IsdbsFrontendSettings
                            .builder()
                            .setFrequency(minFreq)
                            .setModulation(modulation)
                            .setCodeRate(codeRate)
                            .build();
                }
                case FrontendSettings.TYPE_ISDBT: {
                    IsdbtFrontendCapabilities isdbtCaps = (IsdbtFrontendCapabilities) caps;
                    int mode = getFirstCapable(isdbtCaps.getModeCapability());
                    int bandwidth = getFirstCapable(isdbtCaps.getBandwidthCapability());
                    int modulation = getFirstCapable(isdbtCaps.getModulationCapability());
                    int codeRate = getFirstCapable(isdbtCaps.getCodeRateCapability());
                    int guardInterval = getFirstCapable(isdbtCaps.getGuardIntervalCapability());
                    return IsdbtFrontendSettings
                            .builder()
                            .setFrequency(minFreq)
                            .setModulation(modulation)
                            .setBandwidth(bandwidth)
                            .setMode(mode)
                            .setCodeRate(codeRate)
                            .setGuardInterval(guardInterval)
                            .build();
                }
                case FrontendSettings.TYPE_DTMB: {
                    DtmbFrontendCapabilities dtmbCaps = (DtmbFrontendCapabilities) caps;
                    int modulation = getFirstCapable(dtmbCaps.getModulationCapability());
                    int transmissionMode = getFirstCapable(
                            dtmbCaps.getTransmissionModeCapability());
                    int guardInterval = getFirstCapable(dtmbCaps.getGuardIntervalCapability());
                    int timeInterleaveMode = getFirstCapable(
                            dtmbCaps.getTimeInterleaveModeCapability());
                    int codeRate = getFirstCapable(dtmbCaps.getCodeRateCapability());
                    int bandwidth = getFirstCapable(dtmbCaps.getBandwidthCapability());
                    DtmbFrontendSettings settings =
                            DtmbFrontendSettings
                                    .builder()
                                    .setFrequency(minFreq)
                                    .setModulation(modulation)
                                    .setTransmissionMode(transmissionMode)
                                    .setBandwidth(bandwidth)
                                    .setCodeRate(codeRate)
                                    .setGuardInterval(guardInterval)
                                    .setTimeInterleaveMode(timeInterleaveMode)
                                    .build();
                    settings.setEndFrequency(maxFreq);
                    return settings;
                }
                default:
                    break;
            }
        return null;
    }

    private int getFirstCapable(int caps) {
        if (caps == 0) return 0;
        int mask = 1;
        while ((mask & caps) == 0) {
            mask = mask << 1;
        }
        return (mask & caps);
    }

    private long getFirstCapable(long caps) {
        if (caps == 0) return 0;
        long mask = 1;
        while ((mask & caps) == 0) {
            mask = mask << 1;
        }
        return (mask & caps);
    }

    private ScanCallback getScanCallback() {
        return new ScanCallback() {
            @Override
            public void onLocked() {
                if (mLockLatch != null) {
                    mLockLatch.countDown();
                }
            }

            @Override
            public void onScanStopped() {}

            @Override
            public void onProgress(int percent) {}

            @Override
            public void onFrequenciesReported(int[] frequency) {}

            @Override
            public void onSymbolRatesReported(int[] rate) {}

            @Override
            public void onPlpIdsReported(int[] plpIds) {}

            @Override
            public void onGroupIdsReported(int[] groupIds) {}

            @Override
            public void onInputStreamIdsReported(int[] inputStreamIds) {}

            @Override
            public void onDvbsStandardReported(int dvbsStandard) {}

            @Override
            public void onDvbtStandardReported(int dvbtStandard) {}

            @Override
            public void onAnalogSifStandardReported(int sif) {}

            @Override
            public void onAtsc3PlpInfosReported(Atsc3PlpInfo[] atsc3PlpInfos) {
                for (Atsc3PlpInfo info : atsc3PlpInfos) {
                    if (info != null) {
                        info.getPlpId();
                        info.getLlsFlag();
                    }
                }
            }

            @Override
            public void onHierarchyReported(int hierarchy) {}

            @Override
            public void onSignalTypeReported(int signalType) {}

            @Override
            public void onModulationReported(int modulation) {
                ScanCallback.super.onModulationReported(modulation);
            }

            @Override
            public void onPriorityReported(boolean isHighPriority) {
                ScanCallback.super.onPriorityReported(isHighPriority);
            }

            @Override
            public void onDvbcAnnexReported(int dvbcAnnext) {
                ScanCallback.super.onDvbcAnnexReported(dvbcAnnext);
            }
        };
    }
}
