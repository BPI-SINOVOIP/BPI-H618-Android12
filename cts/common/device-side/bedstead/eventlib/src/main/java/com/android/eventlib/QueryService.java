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

import android.app.Service;
import android.content.Intent;
import android.os.Bundle;
import android.os.IBinder;

import java.time.Duration;
import java.time.Instant;
import java.util.concurrent.ConcurrentHashMap;

/**
 * Implementation of {@link IQueryService}.
 */
public final class QueryService extends Service {

    public static final String EARLIEST_LOG_TIME_KEY = "EARLIEST_LOG_TIME";
    public static final String QUERIER_KEY = "QUERIER";
    public static final String EVENT_KEY = "EVENT";
    public static final String TIMEOUT_KEY = "TIMEOUT";

    private static class QueryClient {
        final EventLogsQuery<?, ?> query;
        final LocalEventQuerier<?, ?> querier;

        public QueryClient(EventLogsQuery<?, ?> query, LocalEventQuerier<?, ?> querier) {
            this.query = query;
            this.querier = querier;
        }
    }

    private final IQueryService.Stub binder = new IQueryService.Stub() {

        // Map of all initialised clients, to keep track of progress when using poll/next
        private final ConcurrentHashMap<Long, QueryClient> clients = new ConcurrentHashMap<>();

        @Override
        public void init(long id, Bundle data) {
            EventLogsQuery<?, ?> query = (EventLogsQuery<?, ?>) data.getSerializable(QUERIER_KEY);
            LocalEventQuerier<?, ?> querier =
                    new LocalEventQuerier<>(getApplicationContext(), query);

            clients.put(id, new QueryClient(query, querier));
        }

        @Override
        public Bundle get(long id, Bundle data) {
            Instant earliestLogtime = (Instant) data.getSerializable(EARLIEST_LOG_TIME_KEY);
            Event e = clients.get(id).querier.get(earliestLogtime);
            return prepareReturnBundle(e);
        }

        @Override
        public Bundle getNext(long id, Bundle data) {
            Instant earliestLogtime = (Instant) data.getSerializable(EARLIEST_LOG_TIME_KEY);
            Event e = clients.get(id).querier.getNext(earliestLogtime);
            return prepareReturnBundle(e);
        }

        @Override
        public Bundle next(long id, Bundle data) {
            Instant earliestLogtime = (Instant) data.getSerializable(EARLIEST_LOG_TIME_KEY);
            Event e = clients.get(id).querier.next(earliestLogtime);
            return prepareReturnBundle(e);
        }

        @Override
        public Bundle poll(long id, Bundle data) {
            Instant earliestLogtime = (Instant) data.getSerializable(EARLIEST_LOG_TIME_KEY);
            Duration timeoutDuration = (Duration) data.getSerializable(TIMEOUT_KEY);
            Event e = clients.get(id).querier.poll(earliestLogtime, timeoutDuration);
            return prepareReturnBundle(e);
        }

        private Bundle prepareReturnBundle(Event event) {
            Bundle responseBundle = new Bundle();
            if (event != null) {
                responseBundle.putSerializable(EVENT_KEY, event);
            }

            return responseBundle;
        }
    };

    @Override
    public IBinder onBind(Intent intent) {
        return binder;
    }
}
