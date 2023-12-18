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

package android.cts.statsdatom.alarm;

import static com.google.common.truth.Truth.assertThat;

import android.app.ProcessStateEnum;
import android.cts.statsdatom.lib.AtomTestUtils;
import android.cts.statsdatom.lib.ConfigUtils;
import android.cts.statsdatom.lib.DeviceUtils;
import android.cts.statsdatom.lib.ReportUtils;

import com.android.os.AtomsProto;
import com.android.os.StatsLog;
import com.android.tradefed.build.IBuildInfo;
import com.android.tradefed.testtype.DeviceTestCase;
import com.android.tradefed.testtype.IBuildReceiver;

import java.util.List;
import java.util.function.Predicate;

public class AlarmStatsTests extends DeviceTestCase implements IBuildReceiver {
    private static final String FEATURE_AUTOMOTIVE = "android.hardware.type.automotive";

    private IBuildInfo mCtsBuild;

    @Override
    protected void setUp() throws Exception {
        super.setUp();
        assertThat(mCtsBuild).isNotNull();
        ConfigUtils.removeConfig(getDevice());
        ReportUtils.clearReports(getDevice());
        DeviceUtils.installStatsdTestApp(getDevice(), mCtsBuild);
        Thread.sleep(AtomTestUtils.WAIT_TIME_LONG);
    }

    @Override
    protected void tearDown() throws Exception {
        ConfigUtils.removeConfig(getDevice());
        ReportUtils.clearReports(getDevice());
        DeviceUtils.uninstallStatsdTestApp(getDevice());
        super.tearDown();
    }

    @Override
    public void setBuild(IBuildInfo buildInfo) {
        mCtsBuild = buildInfo;
    }

    public void testWakeupAlarm() throws Exception {
        // For automotive, all wakeup alarm becomes normal alarm. So this
        // test does not work.
        if (DeviceUtils.hasFeature(getDevice(), FEATURE_AUTOMOTIVE)) return;

        final int atomTag = AtomsProto.Atom.WAKEUP_ALARM_OCCURRED_FIELD_NUMBER;

        ConfigUtils.uploadConfigForPushedAtomWithUid(getDevice(), DeviceUtils.STATSD_ATOM_TEST_PKG,
                atomTag, /*useUidAttributionChain=*/true);
        DeviceUtils.runDeviceTestsOnStatsdApp(getDevice(), ".AtomTests", "testWakeupAlarm");

        List<StatsLog.EventMetricData> data = ReportUtils.getEventMetricDataList(getDevice());
        assertThat(data.size()).isAtLeast(1);
        for (int i = 0; i < data.size(); i++) {
            AtomsProto.WakeupAlarmOccurred wao = data.get(i).getAtom().getWakeupAlarmOccurred();
            assertThat(wao.getTag()).isEqualTo("*walarm*:android.cts.statsdatom.testWakeupAlarm");
            assertThat(wao.getPackageName()).isEqualTo(DeviceUtils.STATSD_ATOM_TEST_PKG);
        }
    }

    public void testAlarmScheduled() throws Exception {
        if (DeviceUtils.hasFeature(getDevice(), FEATURE_AUTOMOTIVE)) return;

        final int atomId = AtomsProto.Atom.ALARM_SCHEDULED_FIELD_NUMBER;

        ConfigUtils.uploadConfigForPushedAtom(getDevice(), DeviceUtils.STATSD_ATOM_TEST_PKG,
                atomId);
        DeviceUtils.runDeviceTestsOnStatsdApp(getDevice(), ".AtomTests", "testAlarmScheduled");

        List<StatsLog.EventMetricData> data = ReportUtils.getEventMetricDataList(getDevice());
        assertThat(data.size()).isAtLeast(2);
        final int uid = DeviceUtils.getStatsdTestAppUid(getDevice());

        int count = 0;
        Predicate<AtomsProto.AlarmScheduled> alarm1 =
                as -> (!as.getIsRtc() && as.getIsExact() && as.getIsWakeup() && !as.getIsRepeating()
                        && !as.getIsAlarmClock() && !as.getIsAllowWhileIdle());
        Predicate<AtomsProto.AlarmScheduled> alarm2 =
                as -> (as.getIsRtc() && !as.getIsExact() && !as.getIsWakeup()
                        && !as.getIsRepeating() && !as.getIsAlarmClock()
                        && !as.getIsAllowWhileIdle());
        for (int i = 0; i < data.size(); i++) {
            AtomsProto.AlarmScheduled as = data.get(i).getAtom().getAlarmScheduled();
            if (as.getCallingUid() != uid) {
                continue;
            }
            assertThat(alarm1.test(as) != alarm2.test(as)).isTrue();
            assertThat(as.getCallingProcessState()).isNoneOf(ProcessStateEnum.PROCESS_STATE_UNKNOWN,
                    ProcessStateEnum.PROCESS_STATE_UNKNOWN_TO_PROTO);
            if (as.getIsExact()) {
                assertThat(as.getExactAlarmAllowedReason()).isAnyOf(
                        AtomsProto.AlarmScheduled.ReasonCode.PERMISSION,
                        AtomsProto.AlarmScheduled.ReasonCode.CHANGE_DISABLED);
            } else {
                assertThat(as.getExactAlarmAllowedReason()).isEqualTo(
                        AtomsProto.AlarmScheduled.ReasonCode.NOT_APPLICABLE);
            }
            count++;
        }
        assertThat(count).isEqualTo(2);
    }

    public void testAlarmBatchDelivered() throws Exception {
        if (DeviceUtils.hasFeature(getDevice(), FEATURE_AUTOMOTIVE)) return;

        final int atomId = AtomsProto.Atom.ALARM_BATCH_DELIVERED_FIELD_NUMBER;

        ConfigUtils.uploadConfigForPushedAtom(getDevice(), DeviceUtils.STATSD_ATOM_TEST_PKG,
                atomId);
        DeviceUtils.runDeviceTestsOnStatsdApp(getDevice(), ".AtomTests", "testWakeupAlarm");

        List<StatsLog.EventMetricData> data = ReportUtils.getEventMetricDataList(getDevice());
        assertThat(data.size()).isAtLeast(1);
        boolean found = false;
        for (int i = 0; i < data.size(); i++) {
            AtomsProto.AlarmBatchDelivered abd = data.get(i).getAtom().getAlarmBatchDelivered();
            found |= abd.getWakeups() >= 1 && abd.getNumAlarms() >= 1;
        }
        assertThat(found).isTrue();
    }

    public void testPendingAlarmInfo() throws Exception {
        if (DeviceUtils.hasFeature(getDevice(), FEATURE_AUTOMOTIVE)) return;

        ConfigUtils.uploadConfigForPulledAtom(getDevice(), DeviceUtils.STATSD_ATOM_TEST_PKG,
                AtomsProto.Atom.PENDING_ALARM_INFO_FIELD_NUMBER);

        // Schedule some alarms
        DeviceUtils.runDeviceTestsOnStatsdApp(getDevice(), ".AtomTests", "testPendingAlarmInfo");
        Thread.sleep(AtomTestUtils.WAIT_TIME_SHORT);
        // Trigger atom pull
        AtomTestUtils.sendAppBreadcrumbReportedAtom(getDevice());
        Thread.sleep(AtomTestUtils.WAIT_TIME_LONG);

        // The list of atoms will be empty if the atom is not supported.
        List<AtomsProto.Atom> atoms = ReportUtils.getGaugeMetricAtoms(getDevice());

        for (AtomsProto.Atom atom : atoms) {
            assertThat(atom.getPendingAlarmInfo().getNumTotal()).isAtLeast(5);
            assertThat(atom.getPendingAlarmInfo().getNumExact()).isAtLeast(2);
            assertThat(atom.getPendingAlarmInfo().getNumWakeup()).isAtLeast(3);
            assertThat(atom.getPendingAlarmInfo().getNumAllowWhileIdle()).isAtLeast(1);
            assertThat(atom.getPendingAlarmInfo().getNumForegroundService()).isAtLeast(2);
            assertThat(atom.getPendingAlarmInfo().getNumActivity()).isAtLeast(1);
            assertThat(atom.getPendingAlarmInfo().getNumService()).isAtLeast(1);
            assertThat(atom.getPendingAlarmInfo().getNumListener()).isAtLeast(1);
            assertThat(atom.getPendingAlarmInfo().getNumIndefiniteFuture()).isAtLeast(1);
            assertThat(atom.getPendingAlarmInfo().getNumRepeating()).isAtLeast(1);
            assertThat(atom.getPendingAlarmInfo().getNumAlarmClock()).isAtLeast(1);
            assertThat(atom.getPendingAlarmInfo().getNumRtc()).isAtLeast(2);
        }
    }
}
