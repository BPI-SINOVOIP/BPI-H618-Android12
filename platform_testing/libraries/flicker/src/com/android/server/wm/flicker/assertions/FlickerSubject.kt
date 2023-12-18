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

package com.android.server.wm.flicker.assertions

import com.android.server.wm.flicker.traces.FlickerSubjectException
import com.google.common.truth.Fact
import com.google.common.truth.FailureMetadata
import com.google.common.truth.StandardSubjectBuilder
import com.google.common.truth.Subject

/**
 * Base subject for flicker assertions
 */
abstract class FlickerSubject(
    protected val fm: FailureMetadata,
    data: Any?
) : Subject(fm, data) {
    abstract val defaultFacts: String
    abstract fun clone(): FlickerSubject

    /**
     * Fails an assertion on a subject
     *
     * @param reason for the failure
     */
    open fun fail(reason: List<Fact>): FlickerSubject = apply {
        require(reason.isNotEmpty()) { "Failure should contain at least 1 fact" }
        val facts = reason.drop(1).toTypedArray()
        failWithoutActual(reason.first(), *facts)
    }

    fun fail(reason: Fact, vararg rest: Fact): FlickerSubject = apply {
        val facts = mutableListOf(reason)
            .also { it.addAll(rest) }
        fail(facts)
    }

    /**
     * Fails an assertion on a subject
     *
     * @param reason for the failure
     */
    fun fail(reason: Fact): FlickerSubject = apply {
        fail(listOf(reason))
    }

    /**
     * Fails an assertion on a subject
     *
     * @param reason for the failure
     */
    fun fail(reason: String): FlickerSubject = apply {
        fail(Fact.fact("Reason", reason))
    }

    /**
     * Fails an assertion on a subject
     *
     * @param reason for the failure
     * @param value for the failure
     */
    fun fail(reason: String, value: Any): FlickerSubject = apply {
        fail(Fact.fact(reason, value))
    }

    /**
     * Fails an assertion on a subject
     *
     * @param reason for the failure
     */
    fun fail(reason: Throwable) {
        if (reason is FlickerSubjectException) {
            throw reason
        } else {
            throw FlickerSubjectException(this, reason)
        }
    }

    /**
     * Function to make external assertions using the subjects
     * Necessary because check is protected and final in the Truth library
     */
    fun verify(message: String): StandardSubjectBuilder = check(message)
}