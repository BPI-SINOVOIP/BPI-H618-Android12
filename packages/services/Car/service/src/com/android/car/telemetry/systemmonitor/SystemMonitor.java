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

package com.android.car.telemetry.systemmonitor;

/**
 * SystemMonitor monitors system states and report to listeners when there are
 * important changes.
 */
public class SystemMonitor {

    private SystemMonitorCallback mCallback;

    /**
     * Interface for receiving notifications about system monitor changes.
     */
    public interface SystemMonitorCallback {
        /**
         * Listens to system monitor event.
         *
         * @param event the system monitor event.
         */
        void onSystemMonitorEvent(SystemMonitorEvent event);
    }

    /**
     * Sets the callback to notify of system state changes.
     *
     * @param callback the callback to nofify state changes on.
     */
    public void setSystemMonitorCallback(SystemMonitorCallback callback) {
        mCallback = callback;
    }
}
