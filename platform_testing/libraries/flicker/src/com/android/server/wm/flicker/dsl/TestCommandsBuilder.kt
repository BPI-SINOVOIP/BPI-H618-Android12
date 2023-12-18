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

package com.android.server.wm.flicker.dsl

import com.android.server.wm.flicker.Flicker
import com.android.server.wm.flicker.FlickerDslMarker

/**
 * Placeholder for test [Flicker.setup] and [Flicker.teardown] commands on the Flicker DSL
 */
@FlickerDslMarker
class TestCommandsBuilder private constructor(
    private val testCommands: MutableList<Flicker.() -> Any>,
    private val runCommands: MutableList<Flicker.() -> Any>
) {
    constructor() : this(
        testCommands = mutableListOf<Flicker.() -> Any>(),
        runCommands = mutableListOf<Flicker.() -> Any>()
    )

    /**
     * Copy constructor
     */
    constructor(otherCommands: TestCommandsBuilder) : this(
        otherCommands.testCommands.toMutableList(),
        otherCommands.runCommands.toMutableList()
    )

    /**
     * Commands to execute once for the whole test
     *
     * If used on the context of if [Flicker.setup], the commands are executed before any traces
     * are recorded and other commands are executed
     *
     * If used on the context of if [Flicker.teardown], the commands are executed after all traces
     * are recorded and all other commands are executed.
     *
     * This command can be used multiple times, and the results are appended
     */
    fun test(command: Flicker.() -> Unit) {
        testCommands.add(command)
    }

    /**
     * Commands to execute once for each test repetition
     *
     * If used on the context of if [Flicker.setup], the commands are executed after those defined
     * in [test] but before any traces are recorded and before the test commands [Flicker.execute]
     * are executed
     *
     * If used on the context of if [Flicker.teardown], the commands are executed before those
     * defined in [test] but after all traces are recorded and the test commands from [Flicker.execute]
     * are executed.
     *
     * This command can be used multiple times, and the results are appended
     */
    fun eachRun(command: Flicker.() -> Unit) {
        runCommands.add(command)
    }

    fun buildTestCommands(): List<Flicker.() -> Any> = testCommands

    fun buildRunCommands(): List<Flicker.() -> Any> = runCommands
}