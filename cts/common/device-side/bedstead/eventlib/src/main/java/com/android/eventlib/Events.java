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
import android.util.Log;

import java.io.FileInputStream;
import java.io.FileNotFoundException;
import java.io.FileOutputStream;
import java.io.IOException;
import java.nio.ByteBuffer;
import java.time.Duration;
import java.time.Instant;
import java.util.ArrayList;
import java.util.Collections;
import java.util.List;
import java.util.Set;
import java.util.WeakHashMap;
import java.util.concurrent.ExecutorService;
import java.util.concurrent.Executors;
import java.util.concurrent.ThreadPoolExecutor;

/** Event store for the current package. */
class Events {

    private static final String TAG = "Events";
    private static final String EVENT_LOG_FILE_NAME = "Events";
    private static final Duration MAX_LOG_AGE = Duration.ofMinutes(5);
    private static final int BYTES_PER_INT = 4;

    private static final ExecutorService sExecutor = Executors.newSingleThreadExecutor();

    /** Interface used to be informed when new events are logged. */
    interface EventListener {
        void onNewEvent(Event e);
    }

    private static Events mInstance;

    static Events getInstance(Context context) {
        if (mInstance == null) {
            synchronized (Events.class) {
                if (mInstance == null) {
                    mInstance = new Events(context.getApplicationContext());
                    mInstance.initialiseFiles();
                }
            }
        }
        return mInstance;
    }

    private final Context mContext; // ApplicationContext
    private FileOutputStream mOutputStream;

    private Events(Context context) {
        this.mContext = context;
    }

    private void initialiseFiles() {
//        sExecutor.execute(() -> {
//            loadEventsFromFile();
//            try {
//                mOutputStream = mContext.openFileOutput(EVENT_LOG_FILE_NAME, Context.MODE_PRIVATE);
//                // We clear the file and write the logs again so we can exclude old logs
//                // This avoids the file growing without limit
//                writeAllEventsToFile();
//            } catch (FileNotFoundException e) {
//                throw new IllegalStateException("Could not write event log", e);
//            }
//        });
    }

    private void loadEventsFromFile() {
        Instant now = Instant.now();
        try (FileInputStream fileInputStream = mContext.openFileInput(EVENT_LOG_FILE_NAME)) {
            Event event = readEvent(fileInputStream);

            while (event != null) {
                if (event.mTimestamp.plus(MAX_LOG_AGE).isBefore(now)) {
                    continue;
                }
                mEventList.add(event);
                event = readEvent(fileInputStream);
            }
        } catch (FileNotFoundException e) {
            // Ignore this exception as if there's no file there's nothing to load
        } catch (IOException e) {
            Log.e(TAG, "Error when loading events from file", e);
        }
    }

    private void writeAllEventsToFile() {
        for (Event event : mEventList) {
            writeEventToFile(event);
        }
    }

    private Event readEvent(FileInputStream fileInputStream) throws IOException {
        if (fileInputStream.available() < BYTES_PER_INT) {
            return null;
        }
        byte[] sizeBytes = new byte[BYTES_PER_INT];
        fileInputStream.read(sizeBytes);

        int size = ByteBuffer.wrap(sizeBytes).getInt();

        byte[] eventBytes = new byte[size];
        fileInputStream.read(eventBytes);

        return Event.fromBytes(eventBytes);
    }

    /** Saves the event so it can be queried. */
    void log(Event event) {
        sExecutor.execute(() -> {
            Log.d(TAG, event.toString());
            synchronized (mEventList) {
                mEventList.add(event); // TODO: This should be made immutable before adding
            }
            triggerEventListeners(event);
        });
    }

    private void writeEventToFile(Event event) {
        try {
            byte[] eventBytes = event.toBytes();
            mOutputStream.write(
                    ByteBuffer.allocate(BYTES_PER_INT).putInt(eventBytes.length).array());
            mOutputStream.write(eventBytes);
        } catch (IOException e) {
            throw new IllegalStateException("Error writing event to log", e);
        }
    }

    private final List<Event> mEventList = new ArrayList<>();
    // This is a weak set so we don't retain listeners from old tests
    private final Set<EventListener> mEventListeners
            = Collections.newSetFromMap(new WeakHashMap<>());

    /** Get all logged events. */
    public List<Event> getEvents() {
        synchronized (mEventList) {
            return mEventList;
        }
    }

    /** Register an {@link EventListener} to be called when a new {@link Event} is logged. */
    public void registerEventListener(EventListener listener) {
        synchronized (Events.class) {
            mEventListeners.add(listener);
        }
    }

    private void triggerEventListeners(Event event) {
        synchronized (Events.class) {
            for (EventListener listener : mEventListeners) {
                listener.onNewEvent(event);
            }
        }
    }

}
