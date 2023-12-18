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

package com.android.server.wm.flicker.monitor

import android.util.EventLog
import android.util.EventLog.Event
import com.android.server.wm.flicker.FlickerRunResult
import com.android.server.wm.flicker.traces.eventlog.FocusEvent
import com.android.server.wm.flicker.traces.eventlog.FocusEvent.Focus
import java.io.IOException
import java.util.UUID

/**
 * Collects event logs during transitions.
 */
open class EventLogMonitor : ITransitionMonitor {
    private var _logs = listOf<Event>()
    private lateinit var _logSeparator: String

    /**
     * Inserts a log separator so we can always find the starting point from where to evaluate
     * following logs.
     *
     * @return Unique log separator.
     */
    private fun separateLogs(): String {
        val logSeparator = UUID.randomUUID().toString()
        EventLog.writeEvent(EVENT_LOG_SEPARATOR_TAG, logSeparator)
        return logSeparator
    }

    private fun getEventLogs(vararg tags: Int): List<Event> {
        val events = mutableListOf<Event>()
        val searchTags = tags.copyOf(tags.size + 1)
        searchTags[searchTags.size - 1] = EVENT_LOG_SEPARATOR_TAG
        try {
            EventLog.readEvents(searchTags, events)
        } catch (e: IOException) {
            throw RuntimeException(e)
        }
        return events.filter { it.data != null }.dropWhile {
            it.tag != EVENT_LOG_SEPARATOR_TAG || it.data.toString() != _logSeparator
        }.drop(1)
    }

    override fun start() {
        // Insert event log marker
        _logSeparator = separateLogs()
    }

    override fun stop() {
        if (!::_logSeparator.isInitialized) {
            _logs = emptyList()
            return
        }
        // Read event log from log marker till end
        _logs = getEventLogs(EVENT_LOG_INPUT_FOCUS_TAG)
    }

    override fun save(testTag: String, flickerRunResultBuilder: FlickerRunResult.Builder) {
        flickerRunResultBuilder.eventLog = _logs.mapNotNull { event ->
            val timestamp = event.timeNanos
            val log = (event.data as Array<*>).map { it as String }
            if (log.size != 2) {
                throw RuntimeException("Error reading from eventlog $log")
            }
            val focusState =
                    when {
                        log[0].contains(" entering ") -> Focus.GAINED
                        log[0].contains(" leaving ") -> Focus.LOST
                        else -> Focus.REQUESTED
                    }
            // parse window from 'Focus [entering|leaving] [window name]'
            // by dropping the first two words
            var expectedWhiteSpace = 2
            val window = log[0].dropWhile { !it.isWhitespace() || --expectedWhiteSpace > 0 }
                    .drop(1)
            val reason = log[1].removePrefix("reason=")
            FocusEvent(timestamp, window, focusState, reason)
                    .takeIf { focusState != Focus.REQUESTED }
        }
    }

    private companion object {
        const val EVENT_LOG_SEPARATOR_TAG = 42
        const val EVENT_LOG_INPUT_FOCUS_TAG = 62001
    }
}
