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

package com.android.eventlib;

import android.util.Log;

import java.io.Serializable;
import java.time.Duration;
import java.time.Instant;

/** Interface to interact with the results of an {@link EventLogsQuery}. */
public abstract class EventLogs<E extends Event> implements Serializable {
    static final Duration DEFAULT_POLL_TIMEOUT = Duration.ofMinutes(5);

    static Instant sEarliestLogTime = Instant.now();

    /**
     * Returns the {@link EventQuerier} to be used to interact with the
     * appropriate {@link Event} store.
     */
    protected abstract EventQuerier<E> getQuerier();

    /**
     * Ensures that future calls to {@link #get()}, {@link #next()}, and {@link #poll()} only return
     * events which are not already logged before this call to {@link #resetLogs()}.
     */
    public static void resetLogs() {
        // We delay 1 ms before and after to separate the cutoff from logs which are
        // triggered immediately by the tests - this makes behaviour more predictable

        try {
            Thread.sleep(1);
        } catch (InterruptedException e) {
            Log.d("EventLogs", "Interrupted when sleeping during resetLogs");
        }

        sEarliestLogTime = Instant.now();

        try {
            Thread.sleep(1);
        } catch (InterruptedException e) {
            Log.d("EventLogs", "Interrupted when sleeping during resetLogs");
        }
    }

    /**
     * Gets the earliest logged event matching the query, if one has been logged by the time the
     * call is made, otherwise returns null.
     */
    public E get() {
        return getQuerier().get(sEarliestLogTime);
    }

    /**
     * Gets the earliest logged event matching the query which has not been returned by a previous
     * call to {@link #next()} or {@link #poll()}, if one has been logged by the time the call is
     * made, otherwise returns null.
     */
    public E next() {
        return getQuerier().next(sEarliestLogTime);
    }

    /**
     * Gets the earliest logged event matching the query which has not be returned by a previous
     * call to {@link #next()} or {@link #poll()}, or blocks until a matching event is logged.
     *
     * <p>This will timeout after {@code timeout} and return null if no matching event is logged.
     */
    public E poll(Duration timeout) {
        return getQuerier().poll(sEarliestLogTime, timeout);
    }

    /**
     * Gets the earliest logged event matching the query which has not be returned by a previous
     * call to {@link #next()} or {@link #poll()}, or blocks until a matching event is logged.
     *
     * <p>This will timeout after {@link #DEFAULT_POLL_TIMEOUT} and return null if no matching
     * event is logged.
     */
    public E poll() {
        return poll(DEFAULT_POLL_TIMEOUT);
    }

    /**
     * Gets the earliest logged event matching the query which has not be returned by a previous
     * call to {@link #next()} or {@link #poll()}, or blocks until a matching event is logged.
     *
     * <p>This will timeout after {@code timeout} and throw an {@link AssertionError} if no
     * matching event is logged.
     */
    public E waitForEvent(Duration timeout) {
        E event = poll(timeout);
        if (event == null) {
            throw new AssertionError("No event was found before timeout");
        }
        return event;
    }

    /**
     * Gets the earliest logged event matching the query which has not be returned by a previous
     * call to {@link #next()} or {@link #poll()}, or blocks until a matching event is logged.
     *
     * <p>This will timeout after {@link #DEFAULT_POLL_TIMEOUT} and throw an {@link AssertionError}
     * if no matching event is logged.
     */
    public E waitForEvent() {
        return waitForEvent(DEFAULT_POLL_TIMEOUT);
    }
}
