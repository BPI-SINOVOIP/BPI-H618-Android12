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

package com.android.car.telemetry.databroker;

import static com.google.common.truth.Truth.assertThat;

import static org.mockito.Mockito.any;
import static org.mockito.Mockito.atLeastOnce;
import static org.mockito.Mockito.verify;

import com.android.car.telemetry.TelemetryProto;
import com.android.car.telemetry.systemmonitor.SystemMonitor;
import com.android.car.telemetry.systemmonitor.SystemMonitorEvent;

import org.junit.Test;
import org.junit.runner.RunWith;
import org.mockito.ArgumentCaptor;
import org.mockito.Captor;
import org.mockito.InjectMocks;
import org.mockito.Mock;
import org.mockito.junit.MockitoJUnitRunner;

@RunWith(MockitoJUnitRunner.class)
public class DataBrokerControllerUnitTest {

    @Mock private DataBroker mMockDataBroker;

    @Mock private SystemMonitor mMockSystemMonitor;

    @Captor ArgumentCaptor<TelemetryProto.MetricsConfig> mConfigCaptor;

    @Captor ArgumentCaptor<Integer> mPriorityCaptor;

    @InjectMocks private DataBrokerController mController;

    private static final TelemetryProto.Publisher PUBLISHER =
            TelemetryProto.Publisher.newBuilder()
                                    .setVehicleProperty(
                                        TelemetryProto.VehiclePropertyPublisher
                                            .newBuilder()
                                            .setReadRate(1)
                                            .setVehiclePropertyId(1000))
                                    .build();
    private static final TelemetryProto.Subscriber SUBSCRIBER =
            TelemetryProto.Subscriber.newBuilder()
                                     .setHandler("handler_func")
                                     .setPublisher(PUBLISHER)
                                     .build();
    private static final TelemetryProto.MetricsConfig CONFIG =
            TelemetryProto.MetricsConfig.newBuilder()
                          .setName("config_name")
                          .setVersion(1)
                          .setScript("function init() end")
                          .addSubscribers(SUBSCRIBER)
                          .build();

    @Test
    public void testOnNewConfig_configPassedToDataBroker() {
        mController.onNewMetricsConfig(CONFIG);

        verify(mMockDataBroker).addMetricsConfiguration(mConfigCaptor.capture());
        assertThat(mConfigCaptor.getValue()).isEqualTo(CONFIG);
    }

    @Test
    public void testOnInit_setsOnScriptFinishedCallback() {
        // Checks that mMockDataBroker's setOnScriptFinishedCallback is called after it's injected
        // into controller's constructor with @InjectMocks
        verify(mMockDataBroker).setOnScriptFinishedCallback(
                any(DataBrokerController.ScriptFinishedCallback.class));
    }

    @Test
    public void testOnSystemEvent_setDataBrokerPriorityCorrectlyForHighCpuUsage() {
        SystemMonitorEvent highCpuEvent = new SystemMonitorEvent();
        highCpuEvent.setCpuUsageLevel(SystemMonitorEvent.USAGE_LEVEL_HI);
        highCpuEvent.setMemoryUsageLevel(SystemMonitorEvent.USAGE_LEVEL_LOW);

        mController.onSystemMonitorEvent(highCpuEvent);

        verify(mMockDataBroker, atLeastOnce())
            .setTaskExecutionPriority(mPriorityCaptor.capture());
        assertThat(mPriorityCaptor.getValue())
            .isEqualTo(DataBrokerController.TASK_PRIORITY_HI);
    }

    @Test
    public void testOnSystemEvent_setDataBrokerPriorityCorrectlyForHighMemUsage() {
        SystemMonitorEvent highMemEvent = new SystemMonitorEvent();
        highMemEvent.setCpuUsageLevel(SystemMonitorEvent.USAGE_LEVEL_LOW);
        highMemEvent.setMemoryUsageLevel(SystemMonitorEvent.USAGE_LEVEL_HI);

        mController.onSystemMonitorEvent(highMemEvent);

        verify(mMockDataBroker, atLeastOnce())
            .setTaskExecutionPriority(mPriorityCaptor.capture());
        assertThat(mPriorityCaptor.getValue())
            .isEqualTo(DataBrokerController.TASK_PRIORITY_HI);
    }

    @Test
    public void testOnSystemEvent_setDataBrokerPriorityCorrectlyForMedCpuUsage() {
        SystemMonitorEvent medCpuEvent = new SystemMonitorEvent();
        medCpuEvent.setCpuUsageLevel(SystemMonitorEvent.USAGE_LEVEL_MED);
        medCpuEvent.setMemoryUsageLevel(SystemMonitorEvent.USAGE_LEVEL_LOW);

        mController.onSystemMonitorEvent(medCpuEvent);

        verify(mMockDataBroker, atLeastOnce())
            .setTaskExecutionPriority(mPriorityCaptor.capture());
        assertThat(mPriorityCaptor.getValue())
            .isEqualTo(DataBrokerController.TASK_PRIORITY_MED);
    }

    @Test
    public void testOnSystemEvent_setDataBrokerPriorityCorrectlyForMedMemUsage() {
        SystemMonitorEvent medMemEvent = new SystemMonitorEvent();
        medMemEvent.setCpuUsageLevel(SystemMonitorEvent.USAGE_LEVEL_LOW);
        medMemEvent.setMemoryUsageLevel(SystemMonitorEvent.USAGE_LEVEL_MED);

        mController.onSystemMonitorEvent(medMemEvent);

        verify(mMockDataBroker, atLeastOnce())
            .setTaskExecutionPriority(mPriorityCaptor.capture());
        assertThat(mPriorityCaptor.getValue())
            .isEqualTo(DataBrokerController.TASK_PRIORITY_MED);
    }

    @Test
    public void testOnSystemEvent_setDataBrokerPriorityCorrectlyForLowUsage() {
        SystemMonitorEvent lowUsageEvent = new SystemMonitorEvent();
        lowUsageEvent.setCpuUsageLevel(SystemMonitorEvent.USAGE_LEVEL_LOW);
        lowUsageEvent.setMemoryUsageLevel(SystemMonitorEvent.USAGE_LEVEL_LOW);

        mController.onSystemMonitorEvent(lowUsageEvent);

        verify(mMockDataBroker, atLeastOnce())
            .setTaskExecutionPriority(mPriorityCaptor.capture());
        assertThat(mPriorityCaptor.getValue())
            .isEqualTo(DataBrokerController.TASK_PRIORITY_LOW);
    }
}
