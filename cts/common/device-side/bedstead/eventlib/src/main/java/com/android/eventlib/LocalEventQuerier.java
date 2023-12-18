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

import android.content.Context;

import java.time.Duration;
import java.time.Instant;
import java.util.concurrent.BlockingDeque;
import java.util.concurrent.LinkedBlockingDeque;
import java.util.concurrent.TimeUnit;


/**
 * Implementation of {@link EventQuerier} which queries data about the current package.
 */
public class LocalEventQuerier<E extends Event, F extends EventLogsQuery> implements EventQuerier<E>, Events.EventListener {
    private final EventLogsQuery<E, F> mEventLogsQuery;
    private final Events mEvents;
    private final BlockingDeque<Event> mFetchedEvents;
    private int skippedGet = 0;

    LocalEventQuerier(Context context, EventLogsQuery<E, F> eventLogsQuery) {
        mEventLogsQuery = eventLogsQuery;
        mEvents = Events.getInstance(context);
        mFetchedEvents = new LinkedBlockingDeque<>(mEvents.getEvents());
        mEvents.registerEventListener(this);
    }

    @Override
    public E get(Instant earliestLogTime) {
        int skipped = 0;
        for (Event event : mEvents.getEvents()) {
            if (mEventLogsQuery.eventClass().isInstance(event)) {
                if (event.mTimestamp.isBefore(earliestLogTime)) {
                    continue;
                } else if (skipped++ < skippedGet) {
                    continue;
                }

                E typedEvent = (E) event;
                if (mEventLogsQuery.filterAll(typedEvent)) {
                    return typedEvent;
                }
            }
        }
        return null;
    }

    /**
     * Same as {@link #get(Instant)} but incremements the number of skipped results.
     *
     * <p>This should be used when the current result from {@link #get(Instant)} does not pass
     * additional filters.
     */
    public E getNext(Instant earliestLogTime) {
        skippedGet += 1;
        return get(earliestLogTime);
    }

    @Override
    public E next(Instant earliestLogTime) {
        while (!mFetchedEvents.isEmpty()) {
            Event event = mFetchedEvents.removeFirst();

            if (mEventLogsQuery.eventClass().isInstance(event)) {
                if (event.mTimestamp.isBefore(earliestLogTime)) {
                    continue;
                }

                E typedEvent = (E) event;
                if (mEventLogsQuery.filterAll(typedEvent)) {
                    return typedEvent;
                }
            }
        }
        return null;
    }

    @Override
    public E poll(Instant earliestLogTime, Duration timeout) {
        Instant endTime = Instant.now().plus(timeout);
        while (true) {
            Event event = null;
            try {
                Duration remainingTimeout = Duration.between(Instant.now(), endTime);
                event = mFetchedEvents.pollFirst(remainingTimeout.toMillis(), TimeUnit.MILLISECONDS);
            } catch (InterruptedException e) {
                return null;
            }

            if (event == null) {
                // Timed out waiting for event
                return null;
            }

            if (mEventLogsQuery.eventClass().isInstance(event)) {
                if (event.mTimestamp.isBefore(earliestLogTime)) {
                    continue;
                }

                E typedEvent = (E) event;
                if (mEventLogsQuery.filterAll(typedEvent)) {
                    return typedEvent;
                }
            }
        }
    }

    @Override
    public void onNewEvent(Event event) {
        mFetchedEvents.addLast(event);
    }
}
