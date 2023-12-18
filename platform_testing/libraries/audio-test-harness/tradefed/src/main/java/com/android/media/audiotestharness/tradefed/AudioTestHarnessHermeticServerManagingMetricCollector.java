/*
 * Copyright (C) 2020 The Android Open Source Project
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

package com.android.media.audiotestharness.tradefed;

import com.android.tradefed.device.metric.BaseDeviceMetricCollector;
import com.android.tradefed.device.metric.DeviceMetricData;
import com.android.tradefed.metrics.proto.MetricMeasurement;

import java.util.Map;

/**
 * {@link BaseDeviceMetricCollector} that manages the spin-up and tear-down of hermetic instances of
 * the Audio Test Harness Server.
 */
public class AudioTestHarnessHermeticServerManagingMetricCollector
        extends BaseDeviceMetricCollector {

    @Override
    public void onTestRunStart(DeviceMetricData runData) {
        super.onTestRunStart(runData);
    }

    @Override
    public void onTestRunEnd(
            DeviceMetricData runData, Map<String, MetricMeasurement.Metric> currentRunMetrics) {
        super.onTestRunEnd(runData, currentRunMetrics);
    }
}
