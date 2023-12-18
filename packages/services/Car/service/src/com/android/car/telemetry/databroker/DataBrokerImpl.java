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

import android.util.ArrayMap;
import android.util.Slog;

import com.android.car.CarLog;
import com.android.car.telemetry.TelemetryProto;
import com.android.car.telemetry.TelemetryProto.MetricsConfig;
import com.android.car.telemetry.publisher.AbstractPublisher;
import com.android.car.telemetry.publisher.PublisherFactory;
import com.android.internal.annotations.VisibleForTesting;

import java.util.ArrayList;
import java.util.List;
import java.util.Map;

/**
 * Implementation of the data path component of CarTelemetryService. Forwards the published data
 * from publishers to consumers subject to the Controller's decision.
 */
public class DataBrokerImpl implements DataBroker {

    // Maps MetricsConfig's name to its subscriptions. This map is useful when removing a
    // MetricsConfig.
    private final Map<String, List<DataSubscriber>> mSubscriptionMap = new ArrayMap<>();

    private DataBrokerController.ScriptFinishedCallback mScriptFinishedCallback;
    private final PublisherFactory mPublisherFactory;

    public DataBrokerImpl(PublisherFactory publisherFactory) {
        mPublisherFactory = publisherFactory;
    }

    // current task priority, used to determine which data can be processed
    private int mTaskExecutionPriority;

    @Override
    public boolean addMetricsConfiguration(MetricsConfig metricsConfig) {
        // if metricsConfig already exists, it should not be added again
        if (mSubscriptionMap.containsKey(metricsConfig.getName())) {
            return false;
        }
        // Create the subscribers for this metrics configuration
        List<DataSubscriber> dataSubscribers = new ArrayList<>();
        for (TelemetryProto.Subscriber subscriber : metricsConfig.getSubscribersList()) {
            // protobuf publisher to a concrete Publisher
            AbstractPublisher publisher = mPublisherFactory.getPublisher(
                    subscriber.getPublisher().getPublisherCase());

            // create DataSubscriber from TelemetryProto.Subscriber
            DataSubscriber dataSubscriber = new DataSubscriber(metricsConfig, subscriber);
            dataSubscribers.add(dataSubscriber);

            try {
                // The publisher will start sending data to the subscriber.
                // TODO(b/191378559): handle bad configs
                publisher.addDataSubscriber(dataSubscriber);
            } catch (IllegalArgumentException e) {
                Slog.w(CarLog.TAG_TELEMETRY, "Invalid config", e);
                return false;
            }
        }
        mSubscriptionMap.put(metricsConfig.getName(), dataSubscribers);
        return true;
    }

    @Override
    public boolean removeMetricsConfiguration(MetricsConfig metricsConfig) {
        if (!mSubscriptionMap.containsKey(metricsConfig.getName())) {
            return false;
        }
        // get the subscriptions associated with this MetricsConfig, remove it from the map
        List<DataSubscriber> dataSubscribers = mSubscriptionMap.remove(metricsConfig.getName());
        // for each subscriber, remove it from publishers
        for (DataSubscriber subscriber : dataSubscribers) {
            AbstractPublisher publisher = mPublisherFactory.getPublisher(
                    subscriber.getPublisherParam().getPublisherCase());
            try {
                publisher.removeDataSubscriber(subscriber);
            } catch (IllegalArgumentException e) {
                // It shouldn't happen, but if happens, let's just log it.
                Slog.w(CarLog.TAG_TELEMETRY, "Failed to remove subscriber from publisher", e);
            }
            // TODO(b/187743369): remove related tasks from the queue
        }
        return true;
    }

    @Override
    public void setOnScriptFinishedCallback(DataBrokerController.ScriptFinishedCallback callback) {
        mScriptFinishedCallback = callback;
    }

    @Override
    public void setTaskExecutionPriority(int priority) {
        mTaskExecutionPriority = priority;
    }

    @VisibleForTesting
    Map<String, List<DataSubscriber>> getSubscriptionMap() {
        return mSubscriptionMap;
    }
}
