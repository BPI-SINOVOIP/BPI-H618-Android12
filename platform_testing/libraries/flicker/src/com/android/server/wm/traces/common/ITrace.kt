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

package com.android.server.wm.traces.common

interface ITrace<Entry : ITraceEntry> {
    val entries: List<Entry>
    val source: String
    val sourceChecksum: String

    fun getEntry(timestamp: Long): Entry {
        return entries.firstOrNull { it.timestamp == timestamp }
                ?: throw RuntimeException("Entry does not exist for timestamp $timestamp")
    }

    fun hasSource(): Boolean = source.isNotEmpty()
}