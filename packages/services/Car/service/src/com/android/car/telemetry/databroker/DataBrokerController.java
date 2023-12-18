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

import com.android.car.telemetry.TelemetryProto.MetricsConfig;
import com.android.car.telemetry.systemmonitor.SystemMonitor;
import com.android.car.telemetry.systemmonitor.SystemMonitorEvent;

/**
 * DataBrokerController instantiates the DataBroker and manages what Publishers
 * it can read from based current system states and policies.
 */
public class DataBrokerController {

    public static final int TASK_PRIORITY_HI = 0;
    public static final int TASK_PRIORITY_MED = 50;
    public static final int TASK_PRIORITY_LOW = 100;

    private MetricsConfig mMetricsConfig;
    private final DataBroker mDataBroker;
    private final SystemMonitor mSystemMonitor;

    /**
     * Interface for receiving notification that script finished.
     */
    public interface ScriptFinishedCallback {
        /**
         * Listens to script finished event.
         *
         * @param configName the name of the config whose script finished.
         */
        void onScriptFinished(String configName);
    }

    public DataBrokerController(DataBroker dataBroker, SystemMonitor systemMonitor) {
        mDataBroker = dataBroker;
        mDataBroker.setOnScriptFinishedCallback(this::onScriptFinished);
        mSystemMonitor = systemMonitor;
        mSystemMonitor.setSystemMonitorCallback(this::onSystemMonitorEvent);
    }

    /**
     * Listens to new {@link MetricsConfig}.
     *
     * @param metricsConfig the metrics config.
     */
    public void onNewMetricsConfig(MetricsConfig metricsConfig) {
        mMetricsConfig = metricsConfig;
        mDataBroker.addMetricsConfiguration(mMetricsConfig);
    }

    /**
     * Listens to script finished event from {@link DataBroker}.
     *
     * @param configName the name of the config of the finished script.
     */
    public void onScriptFinished(String configName) {
        // TODO(b/192008783): remove finished config from config store
    }

    /**
     * Listens to {@link SystemMonitorEvent} and changes the cut-off priority
     * for {@link DataBroker} such that only tasks with the same or more urgent
     * priority can be run.
     *
     * Highest priority is 0 and lowest is 100.
     *
     * @param event the {@link SystemMonitorEvent} received.
     */
    public void onSystemMonitorEvent(SystemMonitorEvent event) {
        if (event.getCpuUsageLevel() == SystemMonitorEvent.USAGE_LEVEL_HI
                || event.getMemoryUsageLevel() == SystemMonitorEvent.USAGE_LEVEL_HI) {
            mDataBroker.setTaskExecutionPriority(TASK_PRIORITY_HI);
        } else if (event.getCpuUsageLevel() == SystemMonitorEvent.USAGE_LEVEL_MED
                    || event.getMemoryUsageLevel() == SystemMonitorEvent.USAGE_LEVEL_MED) {
            mDataBroker.setTaskExecutionPriority(TASK_PRIORITY_MED);
        } else {
            mDataBroker.setTaskExecutionPriority(TASK_PRIORITY_LOW);
        }
    }
}
