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

package android.cts.statsdatom.perfetto;

import static com.google.common.truth.Truth.assertThat;

import android.cts.statsdatom.lib.AtomTestUtils;
import android.cts.statsdatom.lib.ConfigUtils;
import android.cts.statsdatom.lib.DeviceUtils;
import android.cts.statsdatom.lib.ReportUtils;

import com.android.internal.os.StatsdConfigProto;
import com.android.internal.os.StatsdConfigProto.Alert;
import com.android.internal.os.StatsdConfigProto.FieldMatcher;
import com.android.internal.os.StatsdConfigProto.PerfettoDetails;
import com.android.internal.os.StatsdConfigProto.StatsdConfig;
import com.android.internal.os.StatsdConfigProto.Subscription;
import com.android.internal.os.StatsdConfigProto.TimeUnit;
import com.android.internal.os.StatsdConfigProto.ValueMetric;
import com.android.os.AtomsProto;
import com.android.os.AtomsProto.AppBreadcrumbReported;
import com.android.os.AtomsProto.Atom;
import com.android.os.AtomsProto.PerfettoTrigger;
import com.android.os.AtomsProto.PerfettoUploaded;
import com.android.os.StatsLog.EventMetricData;
import com.android.tradefed.build.IBuildInfo;
import com.android.tradefed.testtype.DeviceTestCase;
import com.android.tradefed.testtype.IBuildReceiver;
import com.android.tradefed.util.CommandResult;
import com.android.tradefed.util.CommandStatus;

import com.google.protobuf.ByteString;

import perfetto.protos.PerfettoConfig.DataSourceConfig;
import perfetto.protos.PerfettoConfig.FtraceConfig;
import perfetto.protos.PerfettoConfig.TraceConfig;

import java.util.ArrayList;
import java.util.List;
import java.util.Random;

public class PerfettoTests extends DeviceTestCase implements IBuildReceiver {

    private static final int WAIT_AFTER_START_PERFETTO_MS = 2000;

    // Config constants
    // These were chosen to match the statsd <-> Perfetto CTS integration
    // test.
    private static final int APP_BREADCRUMB_REPORTED_MATCH_START_ID = 1;
    private static final int METRIC_ID = 8;
    private static final int ALERT_ID = 11;
    private static final int SUBSCRIPTION_ID_PERFETTO = 42;

    @Override
    protected void setUp() throws Exception {
        super.setUp();
        ConfigUtils.removeConfig(getDevice());
        ReportUtils.clearReports(getDevice());
        Thread.sleep(AtomTestUtils.WAIT_TIME_LONG);
    }

    @Override
    protected void tearDown() throws Exception {
        ConfigUtils.removeConfig(getDevice());
        ReportUtils.clearReports(getDevice());
        super.tearDown();
    }

    @Override
    public void setBuild(IBuildInfo buildInfo) {
    }

    public void testPerfettoUploadedAtoms() throws Exception {
        if (DeviceUtils.hasFeature(getDevice(), DeviceUtils.FEATURE_WATCH)) return;
        resetPerfettoGuardrails();

        StatsdConfig.Builder config = getStatsdConfig();
        ConfigUtils.addEventMetric(config, AtomsProto.Atom.PERFETTO_UPLOADED_FIELD_NUMBER);
        ConfigUtils.uploadConfig(getDevice(), config);

        startPerfettoTrace();
        Thread.sleep(WAIT_AFTER_START_PERFETTO_MS);

        // While the trace would not have finished in this time, we expect at least
        // the trace to have been started.
        List<EventMetricData> data = ReportUtils.getEventMetricDataList(getDevice());
        assertThat(extractPerfettoUploadedEvents(data))
                .containsAtLeast(
                        PerfettoUploaded.Event.PERFETTO_TRACE_BEGIN,
                        PerfettoUploaded.Event.PERFETTO_ON_CONNECT,
                        PerfettoUploaded.Event.PERFETTO_TRACED_ENABLE_TRACING,
                        PerfettoUploaded.Event.PERFETTO_TRACED_START_TRACING);
    }

    public void testPerfettoTriggerAtoms() throws Exception {
        if (DeviceUtils.hasFeature(getDevice(), DeviceUtils.FEATURE_WATCH)) return;

        StatsdConfig.Builder config = ConfigUtils.createConfigBuilder("AID_SHELL");
        ConfigUtils.addEventMetric(config, AtomsProto.Atom.PERFETTO_TRIGGER_FIELD_NUMBER);
        ConfigUtils.uploadConfig(getDevice(), config);

        runTriggerPerfetto();
        Thread.sleep(AtomTestUtils.WAIT_TIME_LONG);

        List<EventMetricData> data = ReportUtils.getEventMetricDataList(getDevice());
        assertThat(data).hasSize(1);
        assertThat(extractPerfettoTriggerEvents(data))
                .containsExactly(
                        PerfettoTrigger.Event.PERFETTO_TRIGGER_PERFETTO_TRIGGER);
    }

    /**
     * Returns a protobuf-encoded perfetto config that enables the kernel ftrace tracer with
     * sched_switch for 10 seconds.
     */
    private ByteString getPerfettoConfig() {
        TraceConfig.Builder builder = TraceConfig.newBuilder();

        TraceConfig.BufferConfig buffer =
                TraceConfig.BufferConfig.newBuilder().setSizeKb(128).build();
        builder.addBuffers(buffer);

        FtraceConfig ftraceConfig =
                FtraceConfig.newBuilder().addFtraceEvents("sched/sched_switch").build();
        DataSourceConfig dataSourceConfig =
                DataSourceConfig.newBuilder()
                        .setName("linux.ftrace")
                        .setTargetBuffer(0)
                        .setFtraceConfig(ftraceConfig)
                        .build();
        TraceConfig.DataSource dataSource =
                TraceConfig.DataSource.newBuilder().setConfig(dataSourceConfig).build();
        builder.addDataSources(dataSource);

        builder.setDurationMs(3000);
        builder.setAllowUserBuildTracing(true);

        TraceConfig.IncidentReportConfig incident =
                TraceConfig.IncidentReportConfig.newBuilder()
                        .setSkipIncidentd(true)
                        .build();
        builder.setIncidentReportConfig(incident);

        // To avoid being hit with guardrails firing in multiple test runs back
        // to back, we set a unique session key for each config.
        Random random = new Random();
        StringBuilder sessionNameBuilder = new StringBuilder("statsd-cts-atom-");
        sessionNameBuilder.append(random.nextInt() & Integer.MAX_VALUE);
        builder.setUniqueSessionName(sessionNameBuilder.toString());

        return builder.build().toByteString();
    }

    private List<PerfettoUploaded.Event> extractPerfettoUploadedEvents(
            List<EventMetricData> input) {
        List<PerfettoUploaded.Event> output = new ArrayList<>();
        for (EventMetricData data : input) {
            output.add(data.getAtom().getPerfettoUploaded().getEvent());
        }
        return output;
    }

    private List<PerfettoTrigger.Event> extractPerfettoTriggerEvents(
            List<EventMetricData> input) {
        List<PerfettoTrigger.Event> output = new ArrayList<>();
        for (EventMetricData data : input) {
            output.add(data.getAtom().getPerfettoTrigger().getEvent());
        }
        return output;
    }

    /**
     * Resets the state of the Perfetto guardrails. This avoids that the test fails if it's run too
     * close of for too many times and hits the upload limit.
     */
    private void runTriggerPerfetto() throws Exception {
        final String cmd = "trigger_perfetto cts.test.trigger";
        CommandResult cr = getDevice().executeShellV2Command(cmd);
        if (cr.getStatus() != CommandStatus.SUCCESS)
            throw new Exception(
                    String.format(
                            "Error while executing %s: %s %s",
                            cmd, cr.getStdout(), cr.getStderr()));
    }

    /**
     * Resets the state of the Perfetto guardrails. This avoids that the test fails if it's run too
     * close of for too many times and hits the upload limit.
     */
    private void resetPerfettoGuardrails() throws Exception {
        final String cmd = "perfetto --reset-guardrails";
        CommandResult cr = getDevice().executeShellV2Command(cmd);
        if (cr.getStatus() != CommandStatus.SUCCESS)
            throw new Exception(
                    String.format(
                            "Error while executing %s: %s %s",
                            cmd, cr.getStdout(), cr.getStderr()));
    }

    private void startPerfettoTrace() throws Exception {
        getDevice()
                .executeShellCommand(
                        String.format(
                                "cmd stats log-app-breadcrumb %d %d",
                                1, AppBreadcrumbReported.State.START.ordinal()));
    }

    private final StatsdConfig.Builder getStatsdConfig() throws Exception {
        return ConfigUtils.createConfigBuilder("AID_NOBODY")
                .addSubscription(
                        Subscription.newBuilder()
                                .setId(SUBSCRIPTION_ID_PERFETTO)
                                .setRuleType(Subscription.RuleType.ALERT)
                                .setRuleId(ALERT_ID)
                                .setPerfettoDetails(
                                        PerfettoDetails.newBuilder()
                                                .setTraceConfig(getPerfettoConfig())))
                .addValueMetric(
                        ValueMetric.newBuilder()
                                .setId(METRIC_ID)
                                .setWhat(APP_BREADCRUMB_REPORTED_MATCH_START_ID)
                                .setBucket(TimeUnit.ONE_MINUTE)
                                // Get the label field's value:
                                .setValueField(
                                        FieldMatcher.newBuilder()
                                                .setField(Atom.APP_BREADCRUMB_REPORTED_FIELD_NUMBER)
                                                .addChild(
                                                        FieldMatcher.newBuilder()
                                                                .setField(
                                                                        AppBreadcrumbReported
                                                                                .LABEL_FIELD_NUMBER))))
                .addAtomMatcher(
                        StatsdConfigProto.AtomMatcher.newBuilder()
                                .setId(APP_BREADCRUMB_REPORTED_MATCH_START_ID)
                                .setSimpleAtomMatcher(
                                        StatsdConfigProto.SimpleAtomMatcher.newBuilder()
                                                .setAtomId(
                                                        Atom.APP_BREADCRUMB_REPORTED_FIELD_NUMBER)
                                                .addFieldValueMatcher(
                                                        ConfigUtils.createFvm(
                                                                        AppBreadcrumbReported
                                                                                .STATE_FIELD_NUMBER)
                                                                .setEqInt(
                                                                        AppBreadcrumbReported.State
                                                                                .START
                                                                                .ordinal()))))
                .addAlert(
                        Alert.newBuilder()
                                .setId(ALERT_ID)
                                .setMetricId(METRIC_ID)
                                .setNumBuckets(4)
                                .setRefractoryPeriodSecs(0)
                                .setTriggerIfSumGt(0))
                .addNoReportMetric(METRIC_ID);
    }
}
