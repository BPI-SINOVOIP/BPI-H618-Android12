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

package com.android.server.wm.flicker

/**
 * Execute the transitions of a flicker test and caches the results.
 *
 * Return cached results instead of re-executing the transitions if possible.
 *
 * @param runner Actual runner to execute the test
 */
class TransitionRunnerCached @JvmOverloads constructor(
    private val runner: TransitionRunner = TransitionRunner()
) : TransitionRunner() {
    private var result: FlickerResult? = null

    /**
     * {@inheritDoc}
     */
    override fun run(flicker: Flicker): FlickerResult {
        if (result?.isEmpty() != false) {
            result = runner.run(flicker)
        }

        return result ?: error("Result should not be empty")
    }

    /**
     * {@inheritDoc}
     */
    override fun createTag(flicker: Flicker, tag: String) {
        runner.createTag(flicker, tag)
    }

    /**
     * {@inheritDoc}
     */
    override fun cleanUp() {
        super.cleanUp()
        result?.cleanUp()
        result = null
    }
}